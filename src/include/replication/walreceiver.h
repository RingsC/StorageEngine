/*-------------------------------------------------------------------------
 *
 * walreceiver.h
 *	  Exports from replication/walreceiverfuncs.c.
 *
 * Portions Copyright (c) 2010-2011, PostgreSQL Global Development Group
 *
 * src/include/replication/walreceiver.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef _WALRECEIVER_H
#define _WALRECEIVER_H

#include "access/xlogdefs.h"
#include "storage/spin.h"
#include "pgtime.h"
#include "pthread.h"
#ifdef FOUNDER_XDB_SE
#include "storage/latch.h"
#endif

extern THREAD_LOCAL bool am_walreceiver;
extern int	wal_receiver_status_interval;
extern bool hot_standby_feedback;

/*
 * MAXCONNINFO: maximum size of a connection string.
 *
 * XXX: Should this move to pg_config_manual.h?
 */
#define MAXCONNINFO		1024

/*
 * Values for WalRcv->walRcvState.
 */
typedef enum
{
	WALRCV_STOPPED,				/* stopped and mustn't start up again */
	WALRCV_STARTING,			/* launched, but the process hasn't
								 * initialized yet */
#ifndef FOUNDER_XDB_SE
	WALRCV_RUNNING,				/* walreceiver is running */
#else
	WALRCV_STREAMING,			/* walreceiver is streaming */
	WALRCV_WAITING,				/* stopped streaming, waiting for orders */
	WALRCV_RESTARTING,			/* asked to restart streaming */
#endif
	WALRCV_STOPPING				/* requested to stop, but still running */
} WalRcvState;

/* Shared memory area for management of walreceiver process */
typedef struct
{
	/*
	 * PID of currently active walreceiver process, its current state and
	 * start time (actually, the time at which it was requested to be
	 * started).
	 */
	pthread_t	pid;
	WalRcvState walRcvState;
	pg_time_t	startTime;

	/*
	 * receiveStart is the first byte position that will be received. When
	 * startup process starts the walreceiver, it sets receiveStart to the
	 * point where it wants the streaming to begin.
	 */
	XLogRecPtr	receiveStart;

#ifdef FOUNDER_XDB_SE
	/*
	 * receiveStartTLI indicate the first timeline that will be received. 
	 * When startup process starts the walreceiver, it sets receiveStartTLI
	 * to the point where it wants the streaming to begin.
	 */
	TimeLineID	receiveStartTLI;
	XLogRecPtr  receiveRequest;

	/* indicate we had catch up primary in the past or not.*/
	bool hadCatchupPrimary;
#endif //FOUNDER_XDB_SE

	/*
	 * receivedUpto-1 is the last byte position that has already been
	 * received.  At the first startup of walreceiver, receivedUpto is set to
	 * receiveStart. After that, walreceiver updates this whenever it flushes
	 * the received WAL to disk.
	 */
	XLogRecPtr	receivedUpto;
#ifdef FOUNDER_XDB_SE
	/*
	 * receivedTLI is the timeline receivedUpto came from.	At the first
	 * startup of walreceiver, these are set to receiveStart and
	 * receiveStartTLI. After that, walreceiver updates these whenever it
	 * flushes the received WAL to disk.
	 */
	TimeLineID	receivedTLI;
#endif
	/*
	 * latestChunkStart is the starting byte position of the current "batch"
	 * of received WAL.  It's actually the same as the previous value of
	 * receivedUpto before the last flush to disk.	Startup process can use
	 * this to detect whether it's keeping up or not.
	 */
	XLogRecPtr	latestChunkStart;

	/*
	 * connection string; is used for walreceiver to connect with the primary.
	 */
	char		conninfo[MAXCONNINFO];

	slock_t		mutex;			/* locks shared variables shown above */

#ifdef FOUNDER_XDB_SE
	/*
	 * Latch used by startup process to wake up walreceiver after telling it
	 * where to start streaming (after setting receiveStart and
	 * receiveStartTLI).
	 */
	Latch		latch;
#endif
} WalRcvData;

extern THREAD_LOCAL WalRcvData *WalRcv;

/* libpqwalreceiver hooks */
#ifndef FOUNDER_XDB_SE
typedef bool (*walrcv_connect_type) (char *conninfo, XLogRecPtr startpoint);
extern PGDLLIMPORT walrcv_connect_type walrcv_connect;
#else
typedef void (*walrcv_connect_type) (char *conninfo);
extern THREAD_LOCAL walrcv_connect_type walrcv_connect;

typedef void (*walrcv_identify_system_type) (TimeLineID *primary_tli);
extern THREAD_LOCAL walrcv_identify_system_type walrcv_identify_system;

typedef void (*walrcv_readtimelinehistoryfile_type) (TimeLineID tli, char **filename, char **content, int *size);
extern THREAD_LOCAL walrcv_readtimelinehistoryfile_type walrcv_readtimelinehistoryfile;

typedef bool (*walrcv_startstreaming_type) (TimeLineID tli, XLogRecPtr startpoint, XLogRecPtr recptr);
extern THREAD_LOCAL walrcv_startstreaming_type walrcv_startstreaming;

typedef void (*walrcv_endstreaming_type) (void);
extern THREAD_LOCAL walrcv_endstreaming_type walrcv_endstreaming;
#endif //FOUNDER_XDB_SE

#ifndef FOUNDER_XDB_SE
typedef bool (*walrcv_receive_type) (int timeout, unsigned char *type,
												 char **buffer, int *len);
#else
typedef int (*walrcv_receive_type) (int timeout, char **buffer);
#endif
extern THREAD_LOCAL walrcv_receive_type walrcv_receive;

typedef void (*walrcv_send_type) (const char *buffer, int nbytes);
extern THREAD_LOCAL walrcv_send_type walrcv_send;

typedef void (*walrcv_disconnect_type) (void);
extern THREAD_LOCAL walrcv_disconnect_type walrcv_disconnect;

/* prototypes for functions in walreceiver.c */
extern void WalReceiverMain(void);

/* prototypes for functions in walreceiverfuncs.c */
extern Size WalRcvShmemSize(void);
extern void WalRcvShmemInit(void);
extern void ShutdownWalRcv(void);
#ifndef FOUNDER_XDB_SE
extern bool WalRcvInProgress(void);
extern void RequestXLogStreaming(XLogRecPtr recptr, const char *conninfo);
extern XLogRecPtr GetWalRcvWriteRecPtr(XLogRecPtr *latestChunkStart);
#else
extern bool CheckForNewMaster();
extern bool WalRcvStreaming(void);
extern bool WalRcvRunning(void);
extern void RequestXLogStreaming(TimeLineID tli, XLogRecPtr recptr, const char *conninfo);
extern XLogRecPtr GetWalRcvWriteRecPtr(XLogRecPtr *latestChunkStart, TimeLineID *receiveTLI);
extern void WalRcvInitReceivedUpto(XLogRecPtr recptr);

#endif //FOUNDER_XDB_SE

#endif   /* _WALRECEIVER_H */
