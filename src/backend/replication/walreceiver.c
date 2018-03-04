/*-------------------------------------------------------------------------
 *
 * walreceiver.c
 *
 * The WAL receiver process (walreceiver) is new as of Postgres 9.0. It
 * is the process in the standby server that takes charge of receiving
 * XLOG records from a primary server during streaming replication.
 *
 * When the startup process determines that it's time to start streaming,
 * it instructs postmaster to start walreceiver. Walreceiver first connects
 * to the primary server (it will be served by a walsender process
 * in the primary server), and then keeps receiving XLOG records and
 * writing them to the disk as long as the connection is alive. As XLOG
 * records are received and flushed to disk, it updates the
 * WalRcv->receivedUpto variable in shared memory, to inform the startup
 * process of how far it can proceed with XLOG replay.
 *
 * If the primary server ends streaming, but doesn't disconnect, walreceiver
 * goes into "waiting" mode, and waits for the startup process to give new
 * instructions. The startup process will treat that the same as
 * disconnection, and will rescan the archive/pg_xlog directory. But when the
 * startup process wants to try streaming replication again, it will just
 * nudge the existing walreceiver process that's waiting, instead of launching
 * a new one.
 *
 * Normal termination is by SIGTERM, which instructs the walreceiver to
 * exit(0). Emergency termination is by SIGQUIT; like any postmaster child
 * process, the walreceiver will simply abort and exit on SIGQUIT. A close
 * of the connection and a FATAL error are treated not as a crash but as
 * normal operation.
 *
 * This file contains the server-facing parts of walreceiver. The libpq-
 * specific parts are in the libpqwalreceiver module. It's loaded
 * dynamically to avoid linking the server with libpq.
 *
 * Portions Copyright (c) 2010-2011, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/replication/walreceiver.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <signal.h>
#include <unistd.h>

#ifdef FOUNDER_XDB_SE
#include "access/timeline.h"
#endif
#include "access/transam.h"
#include "access/xlog_internal.h"
#include "libpq/pqsignal.h"
#include "miscadmin.h"
#include "replication/walprotocol.h"
#include "replication/walreceiver.h"
#include "storage/ipc.h"
#include "storage/pmsignal.h"
#include "storage/procarray.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/memutils.h"
//#include "utils/ps_status.h"
#include "utils/resowner.h"
#include "port/thread_commu.h"

/* Global variable to indicate if this process is a walreceiver process */
THREAD_LOCAL bool		am_walreceiver;

/* GUC variable */
int			wal_receiver_status_interval = 10;
bool		hot_standby_feedback;

/* libpqreceiver hooks to these when loaded */
THREAD_LOCAL walrcv_connect_type walrcv_connect = NULL;
#ifdef FOUNDER_XDB_SE
THREAD_LOCAL walrcv_identify_system_type walrcv_identify_system = NULL;
THREAD_LOCAL walrcv_startstreaming_type walrcv_startstreaming = NULL;
THREAD_LOCAL walrcv_endstreaming_type walrcv_endstreaming = NULL;
THREAD_LOCAL walrcv_readtimelinehistoryfile_type walrcv_readtimelinehistoryfile = NULL;
#endif
THREAD_LOCAL walrcv_receive_type walrcv_receive = NULL;
THREAD_LOCAL walrcv_send_type walrcv_send = NULL;
THREAD_LOCAL walrcv_disconnect_type walrcv_disconnect = NULL;

#define NAPTIME_PER_CYCLE 100	/* max sleep time between cycles (100ms) */

/*
 * These variables are used similarly to openLogFile/Id/Seg/Off,
 * but for walreceiver to write the XLOG.
 */
static THREAD_LOCAL int	recvFile = -1;
#ifdef FOUNDER_XDB_SE
static THREAD_LOCAL TimeLineID recvFileTLI = 0;
#endif
static THREAD_LOCAL uint32 recvId = 0;
static THREAD_LOCAL uint32 recvSeg = 0;
static THREAD_LOCAL uint32 recvOff = 0;

/*
 * Flags set by interrupt handlers of walreceiver for later service in the
 * main loop.
 */
static THREAD_LOCAL volatile sig_atomic_t got_SIGHUP = false;
static THREAD_LOCAL volatile sig_atomic_t got_SIGTERM = false;

/*
 * LogstreamResult indicates the byte positions that we have already
 * written/fsynced.
 */
static  THREAD_LOCAL struct
{
	XLogRecPtr	Write;			/* last byte + 1 written out in the standby */
	XLogRecPtr	Flush;			/* last byte + 1 flushed in the standby */
}	LogstreamResult;

static THREAD_LOCAL StandbyReplyMessage reply_message;
static THREAD_LOCAL StandbyHSFeedbackMessage feedback_message;

/*
 * About SIGTERM handling:
 *
 * We can't just exit(1) within SIGTERM signal handler, because the signal
 * might arrive in the middle of some critical operation, like while we're
 * holding a spinlock. We also can't just set a flag in signal handler and
 * check it in the main loop, because we perform some blocking operations
 * like libpqrcv_PQexec(), which can take a long time to finish.
 *
 * We use a combined approach: When WalRcvImmediateInterruptOK is true, it's
 * safe for the signal handler to elog(FATAL) immediately. Otherwise it just
 * sets got_SIGTERM flag, which is checked in the main loop when convenient.
 *
 * This is very much like what regular backends do with ImmediateInterruptOK,
 * ProcessInterrupts() etc.
 */
static THREAD_LOCAL volatile bool WalRcvImmediateInterruptOK = false;

/* Prototypes for private functions */
static bool ProcessWalRcvInterrupts(void);
static void EnableWalRcvImmediateExit(void);
static void DisableWalRcvImmediateExit(void);
#ifdef FOUNDER_XDB_SE
static void WalRcvFetchTimeLineHistoryFiles(TimeLineID first, TimeLineID last);
static bool WalRcvWaitForStartPosition(XLogRecPtr *startpoint, XLogRecPtr *requestpoint, TimeLineID *startpointTLI);
#endif
static void WalRcvDie(int code, Datum arg);
static void XLogWalRcvProcessMsg(unsigned char type, char *buf, Size len);
static void XLogWalRcvWrite(char *buf, Size nbytes, XLogRecPtr recptr);
static void XLogWalRcvFlush(bool dying);
static void XLogWalRcvSendReply(void);
static void XLogWalRcvSendHSFeedback(void);

/* Signal handlers */
static void WalRcvSigHupHandler(SIGNAL_ARGS);
static void WalRcvShutdownHandler(SIGNAL_ARGS);
static void WalRcvQuickDieHandler(SIGNAL_ARGS);
#ifdef FOUNDER_XDB_SE
static void WalRcvSigUsr1Handler(SIGNAL_ARGS);
#endif

static bool
ProcessWalRcvInterrupts(void)
{
	/*
	 * Although walreceiver interrupt handling doesn't use the same scheme as
	 * regular backends, call CHECK_FOR_INTERRUPTS() to make sure we receive
	 * any incoming signals on Win32.
	 */
	CHECK_FOR_INTERRUPTS();

	if (got_SIGTERM)
	{
		WalRcvImmediateInterruptOK = false;
		ereport(WARNING,
				(errcode(ERRCODE_ADMIN_SHUTDOWN),
				 errmsg("terminating walreceiver process due to administrator command")));
	}
	return got_SIGTERM != 0;
}

static void
EnableWalRcvImmediateExit(void)
{
	WalRcvImmediateInterruptOK = true;
	ProcessWalRcvInterrupts();
}

static void
DisableWalRcvImmediateExit(void)
{
	WalRcvImmediateInterruptOK = false;
	ProcessWalRcvInterrupts();
}

#ifndef FOUNDER_XDB_SE
/* Main entry point for walreceiver process */
void
WalReceiverMain(void)
{
	char		conninfo[MAXCONNINFO];
	XLogRecPtr	startpoint;

	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;

	am_walreceiver = true;

	/*
	 * WalRcv should be set up already (if we are a backend, we inherit this
	 * by fork() or EXEC_BACKEND mechanism from the postmaster).
	 */
	Assert(walrcv != NULL);

	/*
	 * Mark walreceiver as running in shared memory.
	 *
	 * Do this as early as possible, so that if we fail later on, we'll set
	 * state to STOPPED. If we die before this, the startup process will keep
	 * waiting for us to start up, until it times out.
	 */
	SpinLockAcquire(&walrcv->mutex);
	Assert(walrcv->pid == 0);
	switch (walrcv->walRcvState)
	{
		case WALRCV_STOPPING:
			/* If we've already been requested to stop, don't start up. */
			walrcv->walRcvState = WALRCV_STOPPED;
			/* fall through */

		case WALRCV_STOPPED:
			SpinLockRelease(&walrcv->mutex);
			proc_exit(0);
			return ;

		case WALRCV_STARTING:
			/* The usual case */
			break;

		case WALRCV_RUNNING:
			/* Shouldn't happen */
			ereport(PANIC,
                    (errcode(ERRCODE_INTERNAL_ERROR),
                    errmsg("walreceiver still running according to shared memory state")));
	}
	/* Advertise our PID so that the startup process can kill us */
	walrcv->pid = MyProcPid;
	walrcv->walRcvState = WALRCV_RUNNING;

	/* Fetch information required to start streaming */
	strlcpy(conninfo, (char *) walrcv->conninfo, MAXCONNINFO);
	copyXLogRecPtr(startpoint, walrcv->receiveStart);
	SpinLockRelease(&walrcv->mutex);

	/* Arrange to clean up at walreceiver exit */
	on_shmem_exit(WalRcvDie, 0);

	/*
	 * If possible, make this process a group leader, so that the postmaster
	 * can signal any child processes too.	(walreceiver probably never has
	 * any child processes, but for consistency we make all postmaster child
	 * processes do this.)
	 */
#ifdef HAVE_SETSID
	if (setsid() < 0)
		elog(FATAL, "setsid() failed: %m");
#endif

	/* Properly accept or ignore signals the postmaster might send us */
	pg_signal(SIGHUP, WalRcvSigHupHandler);		/* set flag to read config
												 * file */
	pg_signal(SIGINT, SIG_IGN);
	pg_signal(SIGTERM, WalRcvShutdownHandler);	/* request shutdown */
	pg_signal(SIGQUIT, WalRcvQuickDieHandler);	/* hard crash time */
	pg_signal(SIGALRM, SIG_IGN);
	pg_signal(SIGPIPE, SIG_IGN);
	pg_signal(SIGUSR1, SIG_IGN);
	pg_signal(SIGUSR2, SIG_IGN);

	/* Reset some signals that are accepted by postmaster but not here */
	pg_signal(SIGCHLD, SIG_DFL);
	pg_signal(SIGTTIN, SIG_DFL);
	pg_signal(SIGTTOU, SIG_DFL);
	pg_signal(SIGCONT, SIG_DFL);
	pg_signal(SIGWINCH, SIG_DFL);

	/* We allow SIGQUIT (quickdie) at all times */
	//sigdelset(&BlockSig, SIGQUIT);

	/* Load the libpq-specific functions */
	//load_file("libpqwalreceiver", false);
	extern  void		_PG_init(void);
	_PG_init();
	if (walrcv_connect == NULL || walrcv_receive == NULL ||
		walrcv_send == NULL || walrcv_disconnect == NULL)
		ereport(ERROR,
                (errcode(ERRCODE_LIBPQ_INIT_WRONG),
                errmsg("libpqwalreceiver didn't initialize correctly")));

	/*
	 * Create a resource owner to keep track of our resources (not clear that
	 * we need this, but may as well have one).
	 */
	CurrentResourceOwner = ResourceOwnerCreate(NULL, "Wal Receiver");

	/* Unblock signals (they were blocked when the postmaster forked us) */
	//PG_SETMASK(&UnBlockSig);

	/* Establish the connection to the primary for XLOG streaming */
	EnableWalRcvImmediateExit();
	walrcv_connect(conninfo, startpoint);
	DisableWalRcvImmediateExit();

	/* Loop until end-of-streaming or error */
	for (;;)
	{
		unsigned char type;
		char	   *buf;
		int			len;

		/*
		 * Emergency bailout if postmaster has died.  This is to avoid the
		 * necessity for manual cleanup of all postmaster children.
		 */
		/*if (!PostmasterIsAlive(true))
			exit(1);*/

		/*
		 * Exit walreceiver if we're not in recovery. This should not happen,
		 * but cross-check the status here.
		 */
		if (!RecoveryInProgress())
			ereport(FATAL,
			        (errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("cannot continue WAL streaming, recovery has already ended")));

		/* Process any requests or signals received recently */
		if(ProcessWalRcvInterrupts())
		{
			proc_exit(0);
			return;
		}

		if (got_SIGHUP)
		{
			got_SIGHUP = false;
//			ProcessConfigFile(PGC_SIGHUP);
		}

		/* Wait a while for data to arrive */
		if (walrcv_receive(NAPTIME_PER_CYCLE, &type, &buf, &len))
		{
			/* Accept the received data, and process it */
			XLogWalRcvProcessMsg(type, buf, len);

			/* Receive any more data we can without sleeping */
			while (walrcv_receive(0, &type, &buf, &len))
				XLogWalRcvProcessMsg(type, buf, len);

			/* Let the master know that we received some data. */
			XLogWalRcvSendReply();

			/*
			 * If we've written some records, flush them to disk and let the
			 * startup process and primary server know about them.
			 */
			XLogWalRcvFlush(false);
		}
		else
		{
			/*
			 * We didn't receive anything new, but send a status update to the
			 * master anyway, to report any progress in applying WAL.
			 */
			XLogWalRcvSendReply();
			XLogWalRcvSendHSFeedback();
		}
	}
}
#else
extern  void		_PG_init(void);

/*
 * Main entry point for walreceiver process 
 */
void
WalReceiverMain(void)
{
	char		conninfo[MAXCONNINFO];
	XLogRecPtr	startpoint;
	XLogRecPtr  requestpoint;
	TimeLineID	startpointTLI;
	TimeLineID	primaryTLI;
	bool		first_stream;
	
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	am_walreceiver = true;
	
	ereport(LOG,
		(errmsg("walreceiver started:%d\n", (int)MyProcPid)));

	/*
	 * WalRcv should be set up already (if we are a backend, we inherit this
	 * by fork() or EXEC_BACKEND mechanism from the postmaster).
	 */
	Assert(walrcv != NULL);

	/*
	 * Mark walreceiver as running in shared memory.
	 *
	 * Do this as early as possible, so that if we fail later on, we'll set
	 * state to STOPPED. If we die before this, the startup process will keep
	 * waiting for us to start up, until it times out.
	 */
	SpinLockAcquire(&walrcv->mutex);
	Assert(walrcv->pid == 0);
	switch (walrcv->walRcvState)
	{
		case WALRCV_STOPPING:
			/* If we've already been requested to stop, don't start up. */
			walrcv->walRcvState = WALRCV_STOPPED;
			/* fall through */

		case WALRCV_STOPPED:
			SpinLockRelease(&walrcv->mutex);
			proc_exit(0);
			return;

		case WALRCV_STARTING:
			/* The usual case */
			break;
			
		case WALRCV_WAITING:
		case WALRCV_STREAMING:
		case WALRCV_RESTARTING:
		default:
			/* Shouldn't happen */
			SpinLockRelease(&walrcv->mutex);
			ereport(PANIC,
                    (errcode(ERRCODE_INTERNAL_ERROR),
						errmsg("walreceiver still running according to shared memory state")));
	}
	
	/* Advertise our PID so that the startup process can kill us */
	walrcv->pid = MyProcPid;
	walrcv->walRcvState = WALRCV_STREAMING;
	
	/* Fetch information required to start streaming */
	strlcpy(conninfo, (char *) walrcv->conninfo, MAXCONNINFO);
	copyXLogRecPtr(startpoint, walrcv->receiveStart);
	copyXLogRecPtr(requestpoint, walrcv->receiveRequest);
	startpointTLI = walrcv->receiveStartTLI;
	SpinLockRelease(&walrcv->mutex);
	
	/* Arrange to clean up at walreceiver exit */
	on_shmem_exit(WalRcvDie, 0);
	OwnLatch(&walrcv->latch);
	
	/* Properly accept or ignore signals the postmaster might send us */
	pg_signal(SIGHUP, WalRcvSigHupHandler);		/* set flag to read config
												 * file */
	pg_signal(SIGINT, SIG_IGN);
	pg_signal(SIGTERM, WalRcvShutdownHandler);	/* request shutdown */
	pg_signal(SIGQUIT, WalRcvQuickDieHandler);	/* hard crash time */
	pg_signal(SIGALRM, SIG_IGN);
	pg_signal(SIGPIPE, SIG_IGN);
	pg_signal(SIGUSR1, WalRcvSigUsr1Handler);
	pg_signal(SIGUSR2, SIG_IGN);

	/* Reset some signals that are accepted by postmaster but not here */
	pg_signal(SIGCHLD, SIG_DFL);
	pg_signal(SIGTTIN, SIG_DFL);
	pg_signal(SIGTTOU, SIG_DFL);
	pg_signal(SIGCONT, SIG_DFL);
	pg_signal(SIGWINCH, SIG_DFL);

	_PG_init();
	if (walrcv_connect == NULL || walrcv_startstreaming == NULL ||
		walrcv_endstreaming == NULL ||
		walrcv_identify_system == NULL ||
		walrcv_readtimelinehistoryfile == NULL ||
		walrcv_receive == NULL || walrcv_send == NULL ||
		walrcv_disconnect == NULL)
		ereport(ERROR, (errcode(ERRCODE_LIBPQ_INIT_WRONG),
						errmsg("libpqwalreceiver didn't initialize correctly")));
						
	/*
	 * Create a resource owner to keep track of our resources (not clear that
	 * we need this, but may as well have one).
	 */
	CurrentResourceOwner = ResourceOwnerCreate(NULL, "Wal Receiver");

	/* Establish the connection to the primary for XLOG streaming */
	EnableWalRcvImmediateExit();
	walrcv_connect(conninfo);
	DisableWalRcvImmediateExit();
	
	first_stream = true;
	for (;;)
	{
		/*
		 * Check that we're connected to a valid server using the
		 * IDENTIFY_SYSTEM replication command,
		 */
		EnableWalRcvImmediateExit();
		walrcv_identify_system(&primaryTLI);
		DisableWalRcvImmediateExit();
		
		/*
		 * Confirm that the current timeline of the primary is the same or
		 * ahead of ours.
		 */
		if (primaryTLI < startpointTLI)
			ereport(ERROR,
					(errcode(ERRCODE_RECOVERY_TARGET_TIMELINE_DISMATCH),
					errmsg("highest timeline %u of the primary is behind recovery timeline %u",
							primaryTLI, startpointTLI)));
							
		/*
		 * Get any missing history files. We do this always, even when we're
		 * not interested in that timeline, so that if we're promoted to become
		 * the master later on, we don't select the same timeline that was
		 * already used in the current master. This isn't bullet-proof - you'll
		 * need some external software to manage your cluster if you need to
		 * ensure that a unique timeline id is chosen in every case, but let's
		 * avoid the confusion of timeline id collisions where we can.
		 */
		WalRcvFetchTimeLineHistoryFiles(startpointTLI + 1, primaryTLI);
		
		/*
		 * Start streaming.
		 *
		 * We'll try to start at the requested starting point and timeline,
		 * even if it's different from the server's latest timeline. In case
		 * we've already reached the end of the old timeline, the server will
		 * finish the streaming immediately, and we will go back to await
		 * orders from the startup process. If recovery_target_timeline is
		 * 'latest', the startup process will scan pg_xlog and find the new
		 * history file, bump recovery target timeline, and ask us to restart
		 * on the new timeline.
		 */
		ThisTimeLineID = startpointTLI;
		if (walrcv_startstreaming(startpointTLI, startpoint, requestpoint))
		{
			bool endofwal = false;
			
			if (first_stream)
				ereport(LOG,
						(errmsg("started streaming WAL from primary at %X/%X on timeline %u",
								startpoint.xlogid, startpoint.xrecoff,
								startpointTLI)));
			else
				ereport(LOG,
						(errmsg("restarted WAL streaming at %X/%X on timeline %u",
								startpoint.xlogid, startpoint.xrecoff,
								startpointTLI)));
			first_stream = false;
			
			/* Loop until end-of-streaming or error */
			while (!endofwal)
			{
				char	   *buf;
				int			len;
				
				/*
				 * Exit walreceiver if we're not in recovery. This should not happen,
				 * but cross-check the status here.
				 */
				if (!RecoveryInProgress())
					ereport(FATAL,
							(errcode(ERRCODE_INTERNAL_ERROR),
							errmsg("cannot continue WAL streaming, recovery has already ended")));
							
				/* Process any requests or signals received recently */
				if(ProcessWalRcvInterrupts())
				{
					proc_exit(0);
					return;
				}
				
				if (got_SIGHUP)
				{
					got_SIGHUP = false;
					//ProcessConfigFile(PGC_SIGHUP);
				}
				
				/* Wait a while for data to arrive */
				len = walrcv_receive(NAPTIME_PER_CYCLE, &buf);
				if (len != 0)
				{
					/*
					 * Process the received data, and any subsequent data we
					 * can read without blocking.
					 */
					for(;;)
					{
						if (len > 0)
						{
							/* Something was received from master*/
							XLogWalRcvProcessMsg(buf[0], &buf[1], len - 1);
						}
						else if (len == 0)
							break;
						else if (len < 0)
						{
							ereport(LOG,
									(errmsg("end of WAL reached on timeline %u", startpointTLI)));
							endofwal = true;
							break;
						}
						
						len = walrcv_receive(0, &buf);
					}
					
					/* Let the master know that we received some data. */
					XLogWalRcvSendReply();
					
					/*
					 * If we've written some records, flush them to disk and let the
					 * startup process and primary server know about them.
					 */
					XLogWalRcvFlush(false);
				}
				else
				{
					/*
					 * We didn't receive anything new, but send a status update to the
					 * master anyway, to report any progress in applying WAL.
					 */
					XLogWalRcvSendReply();
					XLogWalRcvSendHSFeedback();
				}
			}
			
			/*
			 * The backend finished streaming. Exit streaming COPY-mode from
			 * our side, too.
			 */
			EnableWalRcvImmediateExit();
			walrcv_endstreaming();
			DisableWalRcvImmediateExit();
		}
		else
			ereport(LOG,
					(errmsg("primary server contains no more WAL on requested timeline %u",
							startpointTLI)));
		
		/*
		 * End of WAL reached on the requested timeline. Close the last
		 * segment, and await for new orders from the startup process.
		 */
		if (recvFile >= 0)
		{
			XLogWalRcvFlush(false);
			if (close(recvFile) != 0)
			{
				char xlogname[MAXPGPATH];

				XLogFileName(xlogname,recvFileTLI, recvId, recvSeg);
				ereport(PANIC,
						(errcode_for_file_access(),
						 errmsg("could not close log segment %s: %m",xlogname)));
			}
		}
		recvFile = -1;

		ereport(LOG, (errmsg("walreceiver ended streaming and awaits new instructions")));
		if (!WalRcvWaitForStartPosition(&startpoint, &requestpoint, &startpointTLI))
			break;
	}

	// some error happen, exit thread
	ereport(LOG, (errmsg("walreceiver exit")));
	proc_exit(0);
}

/*
 * Wait for startup process to set receiveStart and receiveStartTLI.
 */
static bool
WalRcvWaitForStartPosition(XLogRecPtr *startpoint, XLogRecPtr *requestpoint, TimeLineID *startpointTLI)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	int			state;

	SpinLockAcquire(&walrcv->mutex);
	state = walrcv->walRcvState;
	if (state != WALRCV_STREAMING)
	{
		SpinLockRelease(&walrcv->mutex);
		if (state == WALRCV_STOPPING)
			return false;
		else
			elog(FATAL, "unexpected walreceiver state");
	}
	walrcv->walRcvState = WALRCV_WAITING;
	copyXLogRecPtr(walrcv->receiveStart, InvalidXLogRecPtr);
	walrcv->receiveStartTLI = 0;
	SpinLockRelease(&walrcv->mutex);

	/*
	 * nudge startup process to notice that we've stopped streaming and are
	 * now waiting for instructions.
	 */
	WakeupRecovery();
	for (;;)
	{
		ResetLatch(&walrcv->latch);

		/*
		 * Emergency bailout if postmaster has died.  This is to avoid the
		 * necessity for manual cleanup of all postmaster children.
		 */
		if (!PostmasterIsAlive(false))
			return false;

		ProcessWalRcvInterrupts();

		SpinLockAcquire(&walrcv->mutex);
		Assert(walrcv->walRcvState == WALRCV_RESTARTING ||
			   walrcv->walRcvState == WALRCV_WAITING ||
			   walrcv->walRcvState == WALRCV_STOPPING);
		if (walrcv->walRcvState == WALRCV_RESTARTING)
		{
			/* we don't expect primary_conninfo to change */
			copyXLogRecPtr(*startpoint, walrcv->receiveStart);
			copyXLogRecPtr(*requestpoint, walrcv->receiveRequest);
			*startpointTLI = walrcv->receiveStartTLI;
			walrcv->walRcvState = WALRCV_STREAMING;
			SpinLockRelease(&walrcv->mutex);
			break;
		}
		if (walrcv->walRcvState == WALRCV_STOPPING)
		{
			/*
			 * We should've received SIGTERM if the startup process wants
			 * us to die, but might as well check it here too.
			 */
			SpinLockRelease(&walrcv->mutex);
			return false;
		}
		SpinLockRelease(&walrcv->mutex);

		/* wait 1s */
		WaitLatch(&walrcv->latch, 1000L);
	}

	return true;
}

/*
 * Fetch any missing timeline history files between 'first' and 'last'
 * (inclusive) from the server.
 */
static void
WalRcvFetchTimeLineHistoryFiles(TimeLineID first, TimeLineID last)
{
	TimeLineID tli;

	for (tli = first; tli <= last; tli++)
	{
		if (!existsTimeLineHistory(tli))
		{
			char	   *fname;
			char	   *content;
			int 		len;
			char		expectedfname[MAXFNAMELEN];

			ereport(LOG,
					(errmsg("fetching timeline history file for timeline %u from primary server",
							tli)));

			EnableWalRcvImmediateExit();
			walrcv_readtimelinehistoryfile(tli, &fname, &content, &len);
			DisableWalRcvImmediateExit();

			/*
			 * Check that the filename on the master matches what we calculated
			 * ourselves. This is just a sanity check, it should always match.
			 */
			TLHistoryFileName(expectedfname, tli);
			if (strcmp(fname, expectedfname) != 0)
				ereport(ERROR,
						(errcode(ERRCODE_PROTOCOL_VIOLATION),
						 errmsg_internal("primary reported unexpected filename for timeline history file of timeline %u",
										 tli)));

			/*
			 * Write the file to pg_xlog.
			 */
			writeTimeLineHistoryFile(tli, content, len);

			pfree(fname);
			pfree(content);
		}
	}
}
#endif

/*
 * Mark us as STOPPED in shared memory at exit.
 */
static void
WalRcvDie(int code, Datum arg)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;

	/* Ensure that all WAL records received are flushed to disk */
	XLogWalRcvFlush(true);
#ifdef FOUNDER_XDB_SE
	DisownLatch(&walrcv->latch);
#endif
	
	SpinLockAcquire(&walrcv->mutex);
#ifdef FOUNDER_XDB_SE
	Assert(walrcv->walRcvState == WALRCV_STREAMING ||
		   walrcv->walRcvState == WALRCV_RESTARTING ||
		   walrcv->walRcvState == WALRCV_STARTING ||
		   walrcv->walRcvState == WALRCV_WAITING ||
		   walrcv->walRcvState == WALRCV_STOPPING);
#else
	Assert(walrcv->walRcvState == WALRCV_RUNNING ||
		   walrcv->walRcvState == WALRCV_STOPPING);
#endif

	walrcv->walRcvState = WALRCV_STOPPED;
	walrcv->pid = 0;
	SpinLockRelease(&walrcv->mutex);

	/* Terminate the connection gracefully. */
	if (walrcv_disconnect != NULL)
		walrcv_disconnect();

#ifdef FOUNDER_XDB_SE
	if (recvFile >= 0)
		close(recvFile);
	
	/* Wake up the startup process to notice promptly that we're gone */
	WakeupRecovery();
#endif
}

#ifdef FOUNDER_XDB_SE
/* SIGHUP: set flag to re-read config file at next convenient time */
static void
WalRcvSigHupHandler(SIGNAL_ARGS)
{
	got_SIGHUP = true;
}
#endif

/* SIGUSR1: used by latch mechanism */
static void
WalRcvSigUsr1Handler(SIGNAL_ARGS)
{
	latch_sigusr1_handler();
}

/* SIGTERM: set flag for main loop, or shutdown immediately if safe */
static void
WalRcvShutdownHandler(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_SIGTERM = true;
	
#ifdef FOUNDER_XDB_SE
	SetLatch(&WalRcv->latch);
#endif

#ifndef FOUNDER_XDB_SE
	/* Don't joggle the elbow of proc_exit */
	if (!proc_exit_inprogress && WalRcvImmediateInterruptOK)
		ProcessWalRcvInterrupts();
#else
	/*
	 * For avoiding to deaklock in signal handle, we don't call ProcessWalRcvInterrupts.
	 * It is ok, the signal will be handle after shutdown handler.
	 */
	if (!proc_exit_inprogress && WalRcvImmediateInterruptOK)
	{
		WalRcvImmediateInterruptOK = false;
		ereport(WARNING,
				(errcode(ERRCODE_ADMIN_SHUTDOWN),
				 errmsg("terminating walreceiver process due to administrator command")));
	}
#endif

	errno = save_errno;
}

/*
 * WalRcvQuickDieHandler() occurs when signalled SIGQUIT by the postmaster.
 *
 * Some backend has bought the farm, so we need to stop what we're doing and
 * exit.
 */
static void
WalRcvQuickDieHandler(SIGNAL_ARGS)
{
	/*
	 * We DO NOT want to run proc_exit() callbacks -- we're here because
	 * shared memory may be corrupted, so we don't want to try to clean up our
	 * transaction.  Just nail the windows shut and get out of town.  Now that
	 * there's an atexit callback to prevent third-party code from breaking
	 * things by calling exit() directly, we have to reset the callbacks
	 * explicitly to make this work as intended.
	 */
	on_exit_reset();

	/*
	 * Note we do exit(2) not exit(0).	This is to force the postmaster into a
	 * system reset cycle if some idiot DBA sends a manual SIGQUIT to a random
	 * backend.  This is necessary precisely because we don't clean up our
	 * shared memory state.  (The "dead man switch" mechanism in pmsignal.c
	 * should ensure the postmaster sees this as a crash, too, but no harm in
	 * being doubly sure.)
	 */
	exit(2);
}

/*
 * Accept the message from XLOG stream, and process it.
 */
static void
XLogWalRcvProcessMsg(unsigned char type, char *buf, Size len)
{
	switch (type)
	{
		case 'w':				/* WAL records */
			{
				WalDataMessageHeader msghdr;

				if (len < sizeof(WalDataMessageHeader))
					ereport(ERROR,
							(errcode(ERRCODE_PROTOCOL_VIOLATION),
							 errmsg_internal("invalid WAL message received from primary")));
				/* memcpy is required here for alignment reasons */
				memcpy(&msghdr, buf, sizeof(WalDataMessageHeader));
				buf += sizeof(WalDataMessageHeader);
				len -= sizeof(WalDataMessageHeader);

				XLogWalRcvWrite(buf, len, msghdr.dataStart);
				break;
			}

		case 's':
			{
				/* use volatile pointer to prevent code rearrangement */
				volatile WalRcvData *walrcv = WalRcv;				

				SpinLockAcquire(&walrcv->mutex);
				walrcv->hadCatchupPrimary = true;
				SpinLockRelease(&walrcv->mutex);
				ereport(LOG, (errmsg("standby has now caught up with primary")));
				break;
			}
		
		default:
			ereport(ERROR,
					(errcode(ERRCODE_PROTOCOL_VIOLATION),
					 errmsg_internal("invalid replication message type %d",
									 type)));
	}
}

/*
 * Write XLOG data to disk.
 */
static void
XLogWalRcvWrite(char *buf, Size nbytes, XLogRecPtr recptr)
{
	int			startoff;
	int			byteswritten;

	while (nbytes > 0)
	{
		int			segbytes;

		if (recvFile < 0 || !XLByteInSeg(recptr, recvId, recvSeg))
		{
			bool		use_existent;

			/*
			 * fsync() and close current file before we switch to next one. We
			 * would otherwise have to reopen this file to fsync it later
			 */
			if (recvFile >= 0)
			{
				XLogWalRcvFlush(false);

				/*
				 * XLOG segment files will be re-read by recovery in startup
				 * process soon, so we don't advise the OS to release cache
				 * pages associated with the file like XLogFileClose() does.
				 */
				if (close(recvFile) != 0)
#ifdef FOUNDER_XDB_SE
				{
					char xlogfile[MAXPGPATH];

					XLogFileName(xlogfile, recvFileTLI, recvId, recvSeg);
					ereport(PANIC,
							(errcode_for_file_access(),
							 errmsg("could not close log segment %s: %m", xlogfile)));
				}
#else
					ereport(PANIC,
							(errcode_for_file_access(),
						errmsg("could not close log file %u, segment %u: %m",
							   recvId, recvSeg)));
#endif
			}
			recvFile = -1;

			/* Create/use new log file */
			XLByteToSeg(recptr, recvId, recvSeg);
			use_existent = true;
			recvFile = XLogFileInit(recvId, recvSeg, &use_existent, true);
			recvOff = 0;
#ifdef FOUNDER_XDB_SE
			recvFileTLI = ThisTimeLineID;
#endif
		}

		/* Calculate the start offset of the received logs */
		startoff = recptr.xrecoff % XLogSegSize;

		if (startoff + nbytes > XLogSegSize)
			segbytes = XLogSegSize - startoff;
		else
			segbytes = (int)nbytes;

		/* Need to seek in the file? */
		if (recvOff != (uint32)startoff)
		{
			if (lseek(recvFile, (off_t) startoff, SEEK_SET) < 0)
#ifdef FOUNDER_XDB_SE
			{
				char xlogfile[MAXPGPATH];

				XLogFileName(xlogfile, recvFileTLI, recvId, recvSeg);
				ereport(PANIC,
					(errcode_for_file_access(),
					 errmsg("could not seek in log segment %s, to offset %u: %m",
							xlogfile, startoff)));
			}
#else
				ereport(PANIC,
						(errcode_for_file_access(),
						 errmsg("could not seek in log file %u, "
								"segment %u to offset %u: %m",
								recvId, recvSeg, startoff)));
#endif
			recvOff = startoff;
		}

		/* OK to write the logs */
		errno = 0;

		byteswritten = write(recvFile, buf, segbytes);
		if (byteswritten <= 0)
		{
#ifdef FOUNDER_XDB_SE
		char xlogfile[MAXPGPATH];
		XLogFileName(xlogfile, recvFileTLI, recvId, recvSeg);
#endif

			/* if write didn't set errno, assume no disk space */
			if (errno == 0)
				errno = ENOSPC;
#ifdef FOUNDER_XDB_SE
			ereport(PANIC,
					(errcode_for_file_access(),
					 errmsg("could not write to log segment %s "
							"at offset %u, length %lu: %m",
							xlogfile, recvOff, (unsigned long) segbytes)));	
#else
			ereport(PANIC,
					(errcode_for_file_access(),
					 errmsg("could not write to log file %u, segment %u "
							"at offset %u, length %lu: %m",
							recvId, recvSeg,
							recvOff, (unsigned long) segbytes)));
#endif
		}

		/* Update state for write */
		XLByteAdvance(recptr, byteswritten);

		recvOff += byteswritten;
		nbytes -= byteswritten;
		buf += byteswritten;

		LogstreamResult.Write = recptr;
	}
}

/*
 * Flush the log to disk.
 *
 * If we're in the midst of dying, it's unwise to do anything that might throw
 * an error, so we skip sending a reply in that case.
 */
static void
XLogWalRcvFlush(bool dying)
{
	if (XLByteLT(LogstreamResult.Flush, LogstreamResult.Write))
	{
		/* use volatile pointer to prevent code rearrangement */
		volatile WalRcvData *walrcv = WalRcv;

		issue_xlog_fsync(recvFile, recvId, recvSeg);
		LogstreamResult.Flush = LogstreamResult.Write;
		
		/* Update shared-memory status */
		SpinLockAcquire(&walrcv->mutex);
		if (XLByteLT(walrcv->receivedUpto, LogstreamResult.Flush))
		{
			copyXLogRecPtr(walrcv->latestChunkStart, walrcv->receivedUpto);
			copyXLogRecPtr(walrcv->receivedUpto, LogstreamResult.Flush);
#ifdef FOUNDER_XDB_SE
			walrcv->receivedTLI = ThisTimeLineID;

			elog(LOG,"Walreceiver received upto %X/%X on timeline %u",
						walrcv->receivedUpto.xlogid,
						walrcv->receivedUpto.xrecoff,
						recvFileTLI);
#endif
		}
		SpinLockRelease(&walrcv->mutex);

		/* Signal the startup process that new WAL has arrived */
		WakeupRecovery();

		///* Report XLOG streaming progress in PS display */
		//if (update_process_title)
		//{
		//	char		activitymsg[50];

		//	snprintf(activitymsg, sizeof(activitymsg), "streaming %X/%X",
		//			 LogstreamResult.Write.xlogid,
		//			 LogstreamResult.Write.xrecoff);
		//	set_ps_display(activitymsg, false);
		//}

		/* Also let the master know that we made some progress */
		if (!dying)
		{
			XLogWalRcvSendReply();
			XLogWalRcvSendHSFeedback();
		}
	}
}

/*
 * Send reply message to primary, indicating our current XLOG positions and
 * the current time.
 */
static void
XLogWalRcvSendReply(void)
{
	char		buf[sizeof(StandbyReplyMessage) + 1];
	TimestampTz now;

	/*
	 * If the user doesn't want status to be reported to the master, be sure
	 * to exit before doing anything at all.
	 */
	if (wal_receiver_status_interval <= 0)
		return;

	/* Get current timestamp. */
	now = GetCurrentTimestamp();

	/*
	 * We can compare the write and flush positions to the last message we
	 * sent without taking any lock, but the apply position requires a spin
	 * lock, so we don't check that unless something else has changed or 10
	 * seconds have passed.  This means that the apply log position will
	 * appear, from the master's point of view, to lag slightly, but since
	 * this is only for reporting purposes and only on idle systems, that's
	 * probably OK.
	 */
	if (XLByteEQ(reply_message.write, LogstreamResult.Write)
		&& XLByteEQ(reply_message.flush, LogstreamResult.Flush)
		&& !TimestampDifferenceExceeds(reply_message.sendTime, now,
									   wal_receiver_status_interval * 1000))
		return;

	/* Construct a new message */
	reply_message.write = LogstreamResult.Write;
	reply_message.flush = LogstreamResult.Flush;
	reply_message.apply = GetXLogReplayRecPtr();
	reply_message.sendTime = now;

	elog(DEBUG2, "sending write %X/%X flush %X/%X apply %X/%X",
		 reply_message.write.xlogid, reply_message.write.xrecoff,
		 reply_message.flush.xlogid, reply_message.flush.xrecoff,
		 reply_message.apply.xlogid, reply_message.apply.xrecoff);

	/* Prepend with the message type and send it. */
	buf[0] = 'r';
	memcpy(&buf[1], &reply_message, sizeof(StandbyReplyMessage));
	walrcv_send(buf, sizeof(StandbyReplyMessage) + 1);
}

/*
 * Send hot standby feedback message to primary, plus the current time,
 * in case they don't have a watch.
 */
static void
XLogWalRcvSendHSFeedback(void)
{
	char		buf[sizeof(StandbyHSFeedbackMessage) + 1];
	TimestampTz now;
	TransactionId nextXid;
	uint32		nextEpoch;
	TransactionId xmin;

	/*
	 * If the user doesn't want status to be reported to the master, be sure
	 * to exit before doing anything at all.
	 */
	if (wal_receiver_status_interval <= 0 || !hot_standby_feedback)
		return;

	/* Get current timestamp. */
	now = GetCurrentTimestamp();

	/*
	 * Send feedback at most once per wal_receiver_status_interval.
	 */
	if (!TimestampDifferenceExceeds(feedback_message.sendTime, now,
									wal_receiver_status_interval * 1000))
		return;

	/*
	 * If Hot Standby is not yet active there is nothing to send. Check this
	 * after the interval has expired to reduce number of calls.
	 */
	if (!HotStandbyActive())
		return;

	/*
	 * Make the expensive call to get the oldest xmin once we are certain
	 * everything else has been checked.
	 */
	xmin = GetOldestXmin(true, false);

	/*
	 * Get epoch and adjust if nextXid and oldestXmin are different sides of
	 * the epoch boundary.
	 */
	GetNextXidAndEpoch(&nextXid, &nextEpoch);
	if (nextXid < xmin)
		nextEpoch--;

	/*
	 * Always send feedback message.
	 */
	feedback_message.sendTime = now;
	feedback_message.xmin = xmin;
	feedback_message.epoch = nextEpoch;

	elog(DEBUG2, "sending hot standby feedback xmin %u epoch %u",
		 feedback_message.xmin,
		 feedback_message.epoch);

	/* Prepend with the message type and send it. */
	buf[0] = 'h';
	memcpy(&buf[1], &feedback_message, sizeof(StandbyHSFeedbackMessage));
	walrcv_send(buf, sizeof(StandbyHSFeedbackMessage) + 1);
}
