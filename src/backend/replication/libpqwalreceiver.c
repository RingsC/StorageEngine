/*-------------------------------------------------------------------------
 *
 * libpqwalreceiver.c
 *
 * This file contains the libpq-specific parts of walreceiver. It's
 * loaded as a dynamic module to avoid linking the main server binary with
 * libpq.
 *
 * Portions Copyright (c) 2010-2011, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/replication/libpqwalreceiver/libpqwalreceiver.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <unistd.h>
#include <sys/time.h>

#include "libpq/libpq-fe.h"
#include "access/xlog.h"
#include "miscadmin.h"
#include "replication/walreceiver.h"
#include "utils/builtins.h"

#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

PG_MODULE_MAGIC;

void		_PG_init(void);

/* Current connection to the primary, if any */
static THREAD_LOCAL PGconn *streamConn = NULL;

/* Buffer for currently read records */
static THREAD_LOCAL char *recvBuf = NULL;

/* Prototypes for interface functions */
#ifndef FOUNDER_XDB_SE
static bool libpqrcv_connect(char *conninfo, XLogRecPtr startpoint);
static bool libpqrcv_receive(int timeout, unsigned char *type,
				 char **buffer, int *len);
#else
static void libpqrcv_connect(char *conninfo);
static void libpqrcv_identify_system(TimeLineID *primary_tli);
static void libpqrcv_readtimelinehistoryfile(TimeLineID tli, char **filename, char **content, int *len);
static bool libpqrcv_startstreaming(TimeLineID tli, XLogRecPtr startpoint, XLogRecPtr requestpoint);
static void libpqrcv_endstreaming(void);
static int libpqrcv_receive(int timeout, char **buffer);
static int PQResultGetErrorCode(PGresult *res);

#endif
static void libpqrcv_send(const char *buffer, int nbytes);
static void libpqrcv_disconnect(void);

/* Prototypes for private functions */
static bool libpq_select(int timeout_ms);
static PGresult *libpqrcv_PQexec(const char *query);

/*
 * Module load callback
 */
void
_PG_init(void)
{
	/* Tell walreceiver how to reach us */
#ifndef FOUNDER_XDB_SE
	if (walrcv_connect != NULL || walrcv_receive != NULL ||
		walrcv_send != NULL || walrcv_disconnect != NULL)
#else
	if (walrcv_connect != NULL || walrcv_identify_system != NULL ||
		walrcv_readtimelinehistoryfile != NULL ||
		walrcv_startstreaming != NULL || walrcv_endstreaming != NULL ||
		walrcv_receive != NULL || walrcv_send != NULL ||
		walrcv_disconnect != NULL)
#endif
		ereport(ERROR,
                (errcode(ERRCODE_LIBPQZ_WAL_RECIVER_RELOAD),
                errmsg("libpqwalreceiver already loaded")));
	walrcv_connect = libpqrcv_connect;
#ifdef FOUNDER_XDB_SE
	walrcv_identify_system = libpqrcv_identify_system;
	walrcv_readtimelinehistoryfile = libpqrcv_readtimelinehistoryfile;
	walrcv_startstreaming = libpqrcv_startstreaming;
	walrcv_endstreaming = libpqrcv_endstreaming;
#endif
	walrcv_receive = libpqrcv_receive;
	walrcv_send = libpqrcv_send;
	walrcv_disconnect = libpqrcv_disconnect;
}

#ifdef FOUNDER_XDB_SE
static void
libpqrcv_connect(char *conninfo)
{
	char		conninfo_repl[MAXCONNINFO + 75];

	/*
	 * Connect using deliberately undocumented parameter: replication. The
	 * database name is ignored by the server in replication mode, but specify
	 * "replication" for .pgpass lookup.
	 */
	snprintf(conninfo_repl, sizeof(conninfo_repl),
			 "%s dbname=replication replication=true fallback_application_name=walreceiver",
			 conninfo);

	streamConn = PQconnectdb(conninfo_repl);
	if (PQstatus(streamConn) != CONNECTION_OK)
		ereport(ERROR,
				(errcode(ERRCODE_CONNECTION_FAILURE),
					errmsg("could not connect to the primary server: %s",
						PQerrorMessage(streamConn))));
}

/*
 * Check that primary's system identifier matches ours, and fetch the current
 * timeline ID of the primary.
 */
static void
libpqrcv_identify_system(TimeLineID *primary_tli)
{
	PGresult   *res;
	char	   *primary_sysid;
	char		standby_sysid[32];

	/*
	 * Get the system identifier and timeline ID as a DataRow message from the
	 * primary server.
	 */
	res = libpqrcv_PQexec("IDENTIFY_SYSTEM");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		ereport(ERROR,
				(errcode(ERRCODE_NO_DATA_FOUND),
					errmsg("could not receive storage engine identifier and timeline ID from "
						"the primary server: %s",
						PQerrorMessage(streamConn))));
	}
	if (PQnfields(res) != 3 || PQntuples(res) != 1)
	{
		int 		ntuples = PQntuples(res);
		int 		nfields = PQnfields(res);

		PQclear(res);
		ereport(ERROR,
				(errcode(ERRCODE_PROTOCOL_VIOLATION),
					errmsg("invalid response from primary server"),
					errdetail("Expected 1 tuple with 3 fields, got %d tuples with %d fields.",
						   ntuples, nfields)));
	}
	primary_sysid = PQgetvalue(res, 0, 0);
	*primary_tli = pg_atoi(PQgetvalue(res, 0, 1), 4, 0);

	/*
	 * Confirm that the system identifier of the primary is the same as ours.
	 */
	snprintf(standby_sysid, sizeof(standby_sysid), UINT64_FORMAT,
			 GetSystemIdentifier());
	if (strcmp(primary_sysid, standby_sysid) != 0)
	{
		PQclear(res);
		ereport(ERROR,
				(errcode(ERRCODE_SYSID_DIFF_BETWEEN_PRIMARY_AND_STANDBY),
					errmsg("storage engine identifier differs between the primary and standby"),
				 	errdetail("The primary's identifier is %s, the standby's identifier is %s.",
						   primary_sysid, standby_sysid)));
	}
	PQclear(res);
}

/*
 * Start streaming WAL data from given startpoint and timeline.
 *
 * Returns true if we switched successfully to copy-both mode. False
 * means the server received the command and executed it successfully, but
 * didn't switch to copy-mode.  That means that there was no WAL on the
 * requested timeline and starting point, because the server switched to
 * another timeline at or before the requested starting point. On failure,
 * throws an ERROR.
 */
static bool
libpqrcv_startstreaming(TimeLineID tli, XLogRecPtr startpoint, XLogRecPtr requestpoint)
{
	char		cmd[64];
	PGresult   *res;

	/* Start streaming from the point requested by startup process */
	snprintf(cmd, sizeof(cmd), "START_REPLICATION %X/%X/%X/%X TIMELINE %u",
			 	startpoint.xlogid, startpoint.xrecoff,
			 	requestpoint.xlogid, requestpoint.xrecoff, tli);
	res = libpqrcv_PQexec(cmd);

	if (PQresultStatus(res) == PGRES_COMMAND_OK)
	{
		PQclear(res);
		return false;
	}
	else if (PQresultStatus(res) != PGRES_COPY_BOTH)
	{
		int ecode;
		
		ecode = PQResultGetErrorCode(res);
		PQclear(res);
		ereport(ERROR,
				(errcode(ecode == 0 ? ERRCODE_COPY_IO_DATA_TRANSFER_FAILURE : ecode),
					errmsg("could not start WAL streaming: %s",
						PQerrorMessage(streamConn))));
	}
	
	PQclear(res);
	ereport(LOG,
		(errmsg("streaming replication successfully connected to primary")));
	
	return true;
}

/*
 * Stop streaming WAL data.
 */
static void
libpqrcv_endstreaming(void)
{
	PGresult   *res;

	if (PQputCopyEnd(streamConn, NULL) <= 0 || PQflush(streamConn))
	{
		int ecode;

		res = PQgetResult(streamConn);
		ecode = PQResultGetErrorCode(res);
		PQclear(res);
		
		ereport(ERROR,
				(errcode(ecode),
				errmsg("could not send end-of-streaming message to primary: %s",
						PQerrorMessage(streamConn))));
	}

	/* Read the command result after COPY is finished */

	while ((res = PQgetResult(streamConn)) != NULL)
	{
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
			int ecode;
			
			ecode = PQResultGetErrorCode(res);
			PQclear(res);
			ereport(ERROR,
					(errcode(ecode),
					errmsg("error reading result of streaming command: %s",
							PQerrorMessage(streamConn))));
		}
		/*
		 * If we had not yet received CopyDone from the backend, PGRES_COPY_IN
		 * is also possible. However, at the moment this function is only
		 * called after receiving CopyDone from the backend - the walreceiver
		 * never terminates replication on its own initiative.
		 */

		PQclear(res);
	}
}

/*
 * Fetch the timeline history file for 'tli' from primary.
 */
static void
libpqrcv_readtimelinehistoryfile(TimeLineID tli,
								 char **filename, char **content, int *len)
{
	PGresult   *res;
	char		cmd[64];

	/*
	 * Request the primary to send over the history file for given timeline.
	 */
	snprintf(cmd, sizeof(cmd), "TIMELINE_HISTORY %u", tli);
	res = libpqrcv_PQexec(cmd);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		ereport(ERROR,
				(errmsg("could not receive timeline history file from "
						"the primary server: %s",
						PQerrorMessage(streamConn))));
	}
	if (PQnfields(res) != 2 || PQntuples(res) != 1)
	{
		int 		ntuples = PQntuples(res);
		int 		nfields = PQnfields(res);

		PQclear(res);
		ereport(ERROR,
				(errmsg("invalid response from primary server"),
				 errdetail("Expected 1 tuple with 2 fields, got %d tuples with %d fields.",
						   ntuples, nfields)));
	}
	*filename = pstrdup(PQgetvalue(res, 0, 0));

	*len = PQgetlength(res, 0, 1);
	*content = (char *)palloc(*len);
	memcpy(*content, PQgetvalue(res, 0, 1), *len);
	PQclear(res);
}

/*
 * Receive a message available from XLOG stream, blocking for
 * maximum of 'timeout' ms.
 *
 * Returns:
 *
 *	 If data was received, returns the length of the data. *buffer is set to
 *	 point to a buffer holding the received message. The buffer is only valid
 *	 until the next libpqrcv_* call.
 *
 *	 0 if no data was available within timeout, or wait was interrupted
 *	 by signal.
 *
 *   -1 if the server ended the COPY.
 *
 * ereports on error.
 */
static int
libpqrcv_receive(int timeout, char **buffer)
{
	int			rawlen;

	if (recvBuf != NULL)
		PQfreemem(recvBuf);
	recvBuf = NULL;

	/* Try to receive a CopyData message */
	rawlen = PQgetCopyData(streamConn, &recvBuf, 1);
	if (rawlen == 0)
	{
		/*
		 * No data available yet. If the caller requested to block, wait for
		 * more data to arrive.
		 */
		if (timeout > 0)
		{
			if (!libpq_select(timeout))
				return 0;
		}

		if (PQconsumeInput(streamConn) == 0)
			ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                    	errmsg("could not receive data from WAL stream: %s",
							PQerrorMessage(streamConn))));

		/* Now that we've consumed some input, try again */
		rawlen = PQgetCopyData(streamConn, &recvBuf, 1);
		if (rawlen == 0)
			return 0;
	}
	if (rawlen == -1)			/* end-of-streaming or error */
	{
		PGresult   *res;

		res = PQgetResult(streamConn);
		if (PQresultStatus(res) == PGRES_COMMAND_OK ||
			PQresultStatus(res) == PGRES_COPY_IN)
		{
			PQclear(res);
			return -1;
		}
		else
		{
			int ecode;

			ecode = PQResultGetErrorCode(res);
			PQclear(res);
			ereport(ERROR,
                (errcode(ecode == 0 ? ERRCODE_RCV_DATA_FRM_WAL_FAILURE : ecode),
                	errmsg("could not receive data from WAL stream: %s",
							PQerrorMessage(streamConn))));
		}
	}
	if (rawlen < -1)
	{
		PGresult   *res;
		int ecode;

		res = PQgetResult(streamConn);
		ecode = PQResultGetErrorCode(res);
		PQclear(res);
		ereport(ERROR,
                (errcode(ecode == 0 ? ERRCODE_RCV_DATA_FRM_WAL_FAILURE : ecode),
                	errmsg("could not receive data from WAL stream: %s",
						PQerrorMessage(streamConn))));
	}

	/* Return received messages to caller */
	*buffer = recvBuf;
	return rawlen;
}

static int
PQResultGetErrorCode(PGresult *res)
{
	char *codestr = NULL;
	int ecode = 0;
	
	if (res == NULL)
		return 0;

	codestr = PQresultErrorField(res, PG_DIAG_SQLSTATE);
	if (codestr == NULL)
		return 0;
	
	ecode = MAKE_SQLSTATE(codestr[0], codestr[1], codestr[2], codestr[3], codestr[4]); 
	switch (ecode)
	{
		case ERRCODE_UNDEFINED_FILE:
			ecode = ERRCODE_RECOVERY_TARGET_XLOG_HAD_REMOVED;
			break;
		case ERRCODE_RECOVERY_TARGET_TIMELINE_DISMATCH:
			break;
		case ERRCODE_RECOVERY_REQUEST_POINT_AHEAD_START:
			break;
		default:
			ecode = 0;
			break;
	}
	
	return ecode;
}
#else
/*
 * Establish the connection to the primary server for XLOG streaming
 */
static bool
libpqrcv_connect(char *conninfo, XLogRecPtr startpoint)
{
	char		conninfo_repl[MAXCONNINFO + 75];
	char	   *primary_sysid;
	char		standby_sysid[32];
	TimeLineID	primary_tli;
	TimeLineID	standby_tli;
	PGresult   *res;
	char		cmd[64];

	/*
	 * Connect using deliberately undocumented parameter: replication. The
	 * database name is ignored by the server in replication mode, but specify
	 * "replication" for .pgpass lookup.
	 */
	snprintf(conninfo_repl, sizeof(conninfo_repl),
			 "%s dbname=replication replication=true fallback_application_name=walreceiver",
			 conninfo);

	streamConn = PQconnectdb(conninfo_repl);
	if (PQstatus(streamConn) != CONNECTION_OK)
		ereport(ERROR,
                (errcode(ERRCODE_CONNECTION_FAILURE),
                errmsg("could not connect to the primary server: %s",
						                PQerrorMessage(streamConn))));

	/*
	 * Get the system identifier and timeline ID as a DataRow message from the
	 * primary server.
	 */
	res = libpqrcv_PQexec("IDENTIFY_SYSTEM");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		ereport(ERROR,
                (errcode(ERRCODE_NO_DATA_FOUND),
                errmsg("could not receive storage engine identifier and timeline ID from "
						                "the primary server: %s",
						                PQerrorMessage(streamConn))));
	}
	if (PQnfields(res) != 3 || PQntuples(res) != 1)
	{
		int			ntuples = PQntuples(res);
		int			nfields = PQnfields(res);

		PQclear(res);
		ereport(ERROR,
                (errcode(ERRCODE_PROTOCOL_VIOLATION),
                errmsg("invalid response from primary server"),
				                 errdetail("Expected 1 tuple with 3 fields, got %d tuples with %d fields.",
						                   ntuples, nfields)));
	}
	primary_sysid = PQgetvalue(res, 0, 0);
	primary_tli = pg_atoi(PQgetvalue(res, 0, 1), 4, 0);

	/*
	 * Confirm that the system identifier of the primary is the same as ours.
	 */
	snprintf(standby_sysid, sizeof(standby_sysid), UINT64_FORMAT,
			 GetSystemIdentifier());
	if (strcmp(primary_sysid, standby_sysid) != 0)
	{
		PQclear(res);
		ereport(ERROR,
(errcode(ERRCODE_SYSID_DIFF_BETWEEN_PRIMARY_AND_STANDBY),
errmsg("storage engine identifier differs between the primary and standby"),
				 errdetail("The primary's identifier is %s, the standby's identifier is %s.",
						   primary_sysid, standby_sysid)));
	}

	/*
	 * Confirm that the current timeline of the primary is the same as the
	 * recovery target timeline.
	 */
	standby_tli = GetRecoveryTargetTLI();
	PQclear(res);
	if (primary_tli != standby_tli)
		ereport(ERROR,
(errcode(ERRCODE_RECOVERY_TARGET_TIMELINE_DISMATCH),
errmsg("timeline %u of the primary does not match recovery target timeline %u",
						primary_tli, standby_tli)));
	ThisTimeLineID = primary_tli;

	/* Start streaming from the point requested by startup process */
	snprintf(cmd, sizeof(cmd), "START_REPLICATION %X/%X",
			 startpoint.xlogid, startpoint.xrecoff);
	res = libpqrcv_PQexec(cmd);
	if (PQresultStatus(res) != PGRES_COPY_BOTH)
	{
		PQclear(res);
		ereport(ERROR,
(errcode(ERRCODE_COPY_IO_DATA_TRANSFER_FAILURE),
errmsg("could not start WAL streaming: %s",
						PQerrorMessage(streamConn))));
	}
	PQclear(res);

	ereport(LOG,
		(errmsg("streaming replication successfully connected to primary")));

	return true;
}
#endif

/*
 * Wait until we can read WAL stream, or timeout.
 *
 * Returns true if data has become available for reading, false if timed out
 * or interrupted by signal.
 *
 * This is based on pqSocketCheck.
 */
static bool
libpq_select(int timeout_ms)
{
	int			ret;

	Assert(streamConn != NULL);
	if (PQsocket(streamConn) < 0)
		ereport(ERROR,
				(errcode_for_socket_access(),
				 errmsg("socket not open")));

	/* We use poll(2) if available, otherwise select(2) */
	{
#ifdef HAVE_POLL
		struct pollfd input_fd;

		input_fd.fd = PQsocket(streamConn);
		input_fd.events = POLLIN | POLLERR;
		input_fd.revents = 0;

		ret = poll(&input_fd, 1, timeout_ms);
#else							/* !HAVE_POLL */

		fd_set		input_mask;
		struct timeval timeout;
		struct timeval *ptr_timeout;

		FD_ZERO(&input_mask);
		FD_SET(PQsocket(streamConn), &input_mask);

		if (timeout_ms < 0)
			ptr_timeout = NULL;
		else
		{
			timeout.tv_sec = timeout_ms / 1000;
			timeout.tv_usec = (timeout_ms % 1000) * 1000;
			ptr_timeout = &timeout;
		}

		ret = select(PQsocket(streamConn) + 1, &input_mask,
					 NULL, NULL, ptr_timeout);
#endif   /* HAVE_POLL */
	}

	if (ret == 0 || (ret < 0 && errno == EINTR))
		return false;
	if (ret < 0)
		ereport(ERROR,
				(errcode_for_socket_access(),
				 errmsg("select() failed: %m")));
	return true;
}

/*
 * Send a query and wait for the results by using the asynchronous libpq
 * functions and the backend version of select().
 *
 * We must not use the regular blocking libpq functions like PQexec()
 * since they are uninterruptible by signals on some platforms, such as
 * Windows.
 *
 * We must also not use vanilla select() here since it cannot handle the
 * signal emulation layer on Windows.
 *
 * The function is modeled on PQexec() in libpq, but only implements
 * those parts that are in use in the walreceiver.
 *
 * Queries are always executed on the connection in streamConn.
 */
static PGresult *
libpqrcv_PQexec(const char *query)
{
	PGresult   *result = NULL;
	PGresult   *lastResult = NULL;
	
#ifdef FOUNDER_XDB_SE
	if (query)
		ereport(LOG, (errmsg("replication command: \"%s\"", query)));
#endif

	/*
	 * PQexec() silently discards any prior query results on the connection.
	 * This is not required for walreceiver since it's expected that walsender
	 * won't generate any such junk results.
	 */

	/*
	 * Submit a query. Since we don't use non-blocking mode, this also can
	 * block. But its risk is relatively small, so we ignore that for now.
	 */
	if (!PQsendQuery(streamConn, query))
		return NULL;

	for (;;)
	{
		/*
		 * Receive data until PQgetResult is ready to get the result without
		 * blocking.
		 */
		while (PQisBusy(streamConn))
		{
			/*
			 * We don't need to break down the sleep into smaller increments,
			 * and check for interrupts after each nap, since we can just
			 * elog(FATAL) within SIGTERM signal handler if the signal arrives
			 * in the middle of establishment of replication connection.
			 */
			if (!libpq_select(-1))
				continue;		/* interrupted */
			if (PQconsumeInput(streamConn) == 0)
				return lastResult;	/* trouble */
		}

		/*
		 * Emulate the PQexec()'s behavior of returning the last result when
		 * there are many. Since walsender will never generate multiple
		 * results, we skip the concatenation of error messages.
		 */
		result = PQgetResult(streamConn);
		if (result == NULL)
			break;				/* query is complete */

		PQclear(lastResult);
		lastResult = result;

		if (PQresultStatus(lastResult) == PGRES_COPY_IN ||
			PQresultStatus(lastResult) == PGRES_COPY_OUT ||
			PQresultStatus(lastResult) == PGRES_COPY_BOTH ||
			PQstatus(streamConn) == CONNECTION_BAD)
			break;
	}

	return lastResult;
}

/*
 * Disconnect connection to primary, if any.
 */
static void
libpqrcv_disconnect(void)
{
	PQfinish(streamConn);
	streamConn = NULL;
}

#ifndef FOUNDER_XDB_SE
/*
 * Receive a message available from XLOG stream, blocking for
 * maximum of 'timeout' ms.
 *
 * Returns:
 *
 *	 True if data was received. *type, *buffer and *len are set to
 *	 the type of the received data, buffer holding it, and length,
 *	 respectively.
 *
 *	 False if no data was available within timeout, or wait was interrupted
 *	 by signal.
 *
 * The buffer returned is only valid until the next call of this function or
 * libpq_connect/disconnect.
 *
 * ereports on error.
 */
static bool
libpqrcv_receive(int timeout, unsigned char *type, char **buffer, int *len)
{
	int			rawlen;

	if (recvBuf != NULL)
		PQfreemem(recvBuf);
	recvBuf = NULL;

	/* Try to receive a CopyData message */
	rawlen = PQgetCopyData(streamConn, &recvBuf, 1);
	if (rawlen == 0)
	{
		/*
		 * No data available yet. If the caller requested to block, wait for
		 * more data to arrive.
		 */
		if (timeout > 0)
		{
			if (!libpq_select(timeout))
				return false;
		}

		if (PQconsumeInput(streamConn) == 0)
			ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                    errmsg("could not receive data from WAL stream: %s",
							                    PQerrorMessage(streamConn))));

		/* Now that we've consumed some input, try again */
		rawlen = PQgetCopyData(streamConn, &recvBuf, 1);
		if (rawlen == 0)
			return false;
	}
	if (rawlen == -1)			/* end-of-streaming or error */
	{
		PGresult   *res;

		res = PQgetResult(streamConn);
		if (PQresultStatus(res) == PGRES_COMMAND_OK)
		{
			PQclear(res);
			ereport(ERROR,
                    (errcode(ERRCODE_REPLICATION_TERMINATED_BY_PRIM_SERVER),
                    errmsg("replication terminated by primary server")));
		}
		PQclear(res);
		ereport(ERROR,
                (errcode(ERRCODE_RCV_DATA_FRM_WAL_FAILURE),
                errmsg("could not receive data from WAL stream: %s",
						                PQerrorMessage(streamConn))));
	}
	if (rawlen < -1)
		ereport(ERROR,
                (errcode(ERRCODE_RCV_DATA_FRM_WAL_FAILURE),
                errmsg("could not receive data from WAL stream: %s",
						                PQerrorMessage(streamConn))));

	/* Return received messages to caller */
	*type = *((unsigned char *) recvBuf);
	*buffer = recvBuf + sizeof(*type);
	*len = rawlen - sizeof(*type);

	return true;
}
#endif

/*
 * Send a message to XLOG stream.
 *
 * ereports on error.
 */
static void
libpqrcv_send(const char *buffer, int nbytes)
{
	if (PQputCopyData(streamConn, buffer, nbytes) <= 0 ||
		PQflush(streamConn))
		ereport(ERROR,
                (errcode(ERRCODE_SEND_DATA_TO_WAL_FAILURE),
                errmsg("could not send data to WAL stream: %s",
						                PQerrorMessage(streamConn))));
}

/*
 * pg_atoi: convert string to integer
 *
 * allows any number of leading or trailing whitespace characters.
 *
 * 'size' is the sizeof() the desired integral result (1, 2, or 4 bytes).
 *
 * c, if not 0, is a terminator character that may appear after the
 * integer (plus whitespace).  If 0, the string must end after the integer.
 *
 * Unlike plain atoi(), this will throw ereport() upon bad input format or
 * overflow.
 */
int32
pg_atoi(char *s, int size, int c)
{
	long		l;
	char	   *badp;

	/*
	 * Some versions of strtol treat the empty string as an error, but some
	 * seem not to.  Make an explicit test to be sure we catch it.
	 */
	if (s == NULL)
		ereport(ERROR,
                (errcode(ERRCODE_ZERO_LENGTH_CHARACTER_STRING),
                errmsg("NULL pointer")));
	if (*s == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for integer: \"%s\"",
						s)));

	errno = 0;
	l = strtol(s, &badp, 10);

	/* We made no progress parsing the string, so bail out */
	if (s == badp)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for integer: \"%s\"",
						s)));

	switch (size)
	{
		case sizeof(int32):
			if (errno == ERANGE
#if defined(HAVE_LONG_INT_64)
			/* won't get ERANGE on these with 64-bit longs... */
				|| l < INT_MIN || l > INT_MAX
#endif
				)
				ereport(ERROR,
						(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				errmsg("value \"%s\" is out of range for type integer", s)));
			break;
		case sizeof(int16):
			if (errno == ERANGE || l < SHRT_MIN || l > SHRT_MAX)
				ereport(ERROR,
						(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				errmsg("value \"%s\" is out of range for type smallint", s)));
			break;
		case sizeof(int8):
			if (errno == ERANGE || l < SCHAR_MIN || l > SCHAR_MAX)
				ereport(ERROR,
						(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				errmsg("value \"%s\" is out of range for 8-bit integer", s)));
			break;
		default:
			ereport(ERROR,
                    (errcode(ERRCODE_MOST_SPECIFIC_TYPE_MISMATCH),
                    errmsg("unsupported result size: %d", size)));
	}

	/*
	 * Skip any trailing whitespace; if anything but whitespace remains before
	 * the terminating character, bail out
	 */
	while (*badp && *badp != c && isspace((unsigned char) *badp))
		badp++;

	if (*badp && *badp != c)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for integer: \"%s\"",
						s)));

	return (int32) l;
}
