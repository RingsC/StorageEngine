/*-------------------------------------------------------------------------
 *
 * walreceiverfuncs.c
 *
 * This file contains functions used by the startup process to communicate
 * with the walreceiver process. Functions implementing walreceiver itself
 * are in walreceiver.c.
 *
 * Portions Copyright (c) 2010-2011, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  src/backend/replication/walreceiverfuncs.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#include "access/xlog_internal.h"
#include "replication/walreceiver.h"
#include "storage/fd.h"
#include "storage/pmsignal.h"
#include "storage/shmem.h"
#include "utils/guc.h"
#include "port/thread_commu.h"

THREAD_LOCAL WalRcvData *WalRcv = NULL;

#ifdef FOUNDER_XDB_SE

static
void ReSetPrimaryConnInfo(char *newAddress, int newPort);

#endif //FOUNDER_XDB_SE

/*
 * How long to wait for walreceiver to start up after requesting
 * postmaster to launch it. In seconds.
 */
#define WALRCV_STARTUP_TIMEOUT 10

/* Report shared memory space needed by WalRcvShmemInit */
Size
WalRcvShmemSize(void)
{
	Size		size = 0;

	size = add_size(size, sizeof(WalRcvData));

	return size;
}

/* Allocate and initialize walreceiver-related shared memory */
void
WalRcvShmemInit(void)
{
	bool		found;

	WalRcv = (WalRcvData *)
		ShmemInitStruct("Wal Receiver Ctl", WalRcvShmemSize(), &found);

	if (!found)
	{
		/* First time through, so initialize */
		MemSet(WalRcv, 0, WalRcvShmemSize());
		WalRcv->walRcvState = WALRCV_STOPPED;
		SpinLockInit(&WalRcv->mutex);
#ifdef FOUNDER_XDB_SE
		InitSharedLatch(&WalRcv->latch);
#endif
	}
}

#ifdef FOUNDER_XDB_SE
/* Is walreceiver running (or starting up)? */
bool
WalRcvRunning(void)
#else
/* Is walreceiver in progress (or starting up)? */
bool
WalRcvInProgress(void)
#endif
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	WalRcvState state;
	pg_time_t	startTime;

	SpinLockAcquire(&walrcv->mutex);

	state = walrcv->walRcvState;
	startTime = walrcv->startTime;

	SpinLockRelease(&walrcv->mutex);

	/*
	 * If it has taken too long for walreceiver to start up, give up. Setting
	 * the state to STOPPED ensures that if walreceiver later does start up
	 * after all, it will see that it's not supposed to be running and die
	 * without doing anything.
	 */
	if (state == WALRCV_STARTING)
	{
		pg_time_t	now = (pg_time_t) time(NULL);

		if ((now - startTime) > WALRCV_STARTUP_TIMEOUT)
		{
			SpinLockAcquire(&walrcv->mutex);

			if (walrcv->walRcvState == WALRCV_STARTING)
				state = walrcv->walRcvState = WALRCV_STOPPED;

			SpinLockRelease(&walrcv->mutex);
		}
	}

	if (state != WALRCV_STOPPED)
		return true;
	else
		return false;
}

#ifdef FOUNDER_XDB_SE
/*
 * Is walreceiver running and streaming (or at least attempting to connect,
 * or starting up)?
 */
bool
WalRcvStreaming(void)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	WalRcvState state;
	pg_time_t	startTime;

	SpinLockAcquire(&walrcv->mutex);

	state = walrcv->walRcvState;
	startTime = walrcv->startTime;

	SpinLockRelease(&walrcv->mutex);

	/*
	 * If it has taken too long for walreceiver to start up, give up. Setting
	 * the state to STOPPED ensures that if walreceiver later does start up
	 * after all, it will see that it's not supposed to be running and die
	 * without doing anything.
	 */
	if (state == WALRCV_STARTING)
	{
		pg_time_t	now = (pg_time_t) time(NULL);

		if ((now - startTime) > WALRCV_STARTUP_TIMEOUT)
		{
			SpinLockAcquire(&walrcv->mutex);

			/*
			 * After wait WALRCV_STARTUP_TIMEOUT time, it still in WALRCV_STARTING, we assume
			 * that it can not start up. Set the state to WALRCV_STOPPING, and startup process will
			 * wait wal receiver shut down at RequestXLogStreaming, to avoid create two wal receiver
			 * process. This differece with pg to set WALRCV_STOPPED state.
			 */
			if (walrcv->walRcvState == WALRCV_STARTING)
				state = walrcv->walRcvState = WALRCV_STOPPING;

			SpinLockRelease(&walrcv->mutex);
		}
	}

	if (state == WALRCV_STREAMING || state == WALRCV_STARTING ||
		state == WALRCV_RESTARTING)
		return true;
	else
		return false;
}
#endif

/*
 * Stop walreceiver (if running) and wait for it to die.
 */
void
ShutdownWalRcv(void)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	pthread_t walrcvpid = 0;

	/*
	 * Request walreceiver to stop. Walreceiver will switch to WALRCV_STOPPED
	 * mode once it's finished, and will also request postmaster to not
	 * restart itself.
	 */
	SpinLockAcquire(&walrcv->mutex);
	switch (walrcv->walRcvState)
	{
		case WALRCV_STOPPED:
			break;

		case WALRCV_STARTING:
			walrcv->walRcvState = WALRCV_STOPPED;
			break;

#ifdef FOUNDER_XDB_SE
		case WALRCV_STREAMING:
		case WALRCV_WAITING:
		case WALRCV_RESTARTING:
#else
		case WALRCV_RUNNING:
#endif
			walrcv->walRcvState = WALRCV_STOPPING;
			/* fall through */
		case WALRCV_STOPPING:
			walrcvpid = walrcv->pid;
			break;
	}
	SpinLockRelease(&walrcv->mutex);

	/*
	 * Signal walreceiver process if it was still running.
	 */
	if (walrcvpid != 0)
		pg_wakeup(walrcvpid, SIGTERM);

	/*
	 * Wait for walreceiver to acknowledge its death by setting state to
	 * WALRCV_STOPPED.
	 */
#ifdef FOUNDER_XDB_SE
	while (WalRcvRunning())
#else
	while (WalRcvInProgress())
#endif
	{
		/*
		 * This possibly-long loop needs to handle interrupts of startup
		 * process.
		 */
		HandleStartupProcInterrupts();

		pg_sleep(100000);		/* 100ms */
	}

	elog(LOG,"shutdown walrecerver");
}

/*
 * Request postmaster to start walreceiver.
 *
 * recptr indicates the position where streaming should begin, and conninfo
 * is a libpq connection string to use.
 */
#ifdef FOUNDER_XDB_SE
extern void fxdb_walreceiver_forkexec(void);

void
RequestXLogStreaming(TimeLineID tli, XLogRecPtr recptr, const char *conninfo)
#else
void
RequestXLogStreaming(XLogRecPtr recptr, const char *conninfo)
#endif
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	pg_time_t	now = (pg_time_t) time(NULL);

#ifdef FOUNDER_XDB_SE
	XLogRecPtr originalptr;
	bool	   launch = false;
	
	copyXLogRecPtr(originalptr, recptr);	
	ereport(LOG,(errmsg("request xlog streaming from %X/%X on timeline %u",
						recptr.xlogid, recptr.xrecoff, tli)));
#endif //FOUNDER_XDB_SE

	/*
	 * We always start at the beginning of the segment. That prevents a broken
	 * segment (i.e., with no records in the first half of a segment) from
	 * being created by XLOG streaming, which might cause trouble later on if
	 * the segment is e.g archived.
	 */
	if (recptr.xrecoff % XLogSegSize != 0)
		recptr.xrecoff -= recptr.xrecoff % XLogSegSize;

	SpinLockAcquire(&walrcv->mutex);

#ifdef FOUNDER_XDB_SE
	/*
	 * If walreceiver is stopping, we wait it to stop and then return. It maybe happen,
	 * because startup proccess don't wait walreceiver status to stopped, while startup
	 * proccess get a SIGCHLD signal to shut down walreceiver.
	 */
	if (walrcv->walRcvState == WALRCV_STOPPING)
	{
		SpinLockRelease(&walrcv->mutex);

		/*
		 * Wait for walreceiver to acknowledge its death by setting state to
		 * WALRCV_STOPPED.
		 */
		while (WalRcvRunning())
		{
			/*
			 * This possibly-long loop needs to handle interrupts of startup
			 * process.
			 */
			HandleStartupProcInterrupts();
			pg_sleep(100000);		/* 100ms */
		}
		return;
	}

	/* It better be stopped before we try to restart it */
	Assert(walrcv->walRcvState == WALRCV_STOPPED || 
			walrcv->walRcvState == WALRCV_WAITING);
#else
	/* It better be stopped before we try to restart it */
	Assert(walrcv->walRcvState == WALRCV_STOPPED);
#endif

	if (conninfo != NULL)
		strlcpy((char *) walrcv->conninfo, conninfo, MAXCONNINFO);
	else
		walrcv->conninfo[0] = '\0';

#ifdef FOUNDER_XDB_SE
	if (walrcv->walRcvState == WALRCV_STOPPED)
	{
		launch = true;
		walrcv->walRcvState = WALRCV_STARTING;
	}
	else
		walrcv->walRcvState = WALRCV_RESTARTING;
#else
	walrcv->walRcvState = WALRCV_STARTING;
#endif
	walrcv->startTime = now;

	/*
	 * If this is the first startup of walreceiver, we initialize receivedUpto
	 * and latestChunkStart to receiveStart.
	 */
	if (walrcv->receiveStart.xlogid == 0 &&
		walrcv->receiveStart.xrecoff == 0)
	{
	    copyXLogRecPtr(walrcv->receivedUpto, recptr);
		copyXLogRecPtr(walrcv->latestChunkStart, recptr);
	}
	copyXLogRecPtr(walrcv->receiveStart, recptr);	
#ifdef FOUNDER_XDB_SE
	copyXLogRecPtr(walrcv->receiveRequest, originalptr);
	walrcv->receiveStartTLI = tli;
#endif

	SpinLockRelease(&walrcv->mutex);

#ifdef FOUNDER_XDB_SE
	if(!launch)
		SetLatch(&walrcv->latch);
	else
#endif
		fxdb_walreceiver_forkexec();

	//SendPostmasterSignal(PMSIGNAL_START_WALRECEIVER);
}

/*
 * Returns the last+1 byte position that walreceiver has written.
 *
 * Optionally, returns the previous chunk start, that is the first byte
 * written in the most recent walreceiver flush cycle.	Callers not
 * interested in that value may pass NULL for latestChunkStart.
 */
#ifdef FOUNDER_XDB_SE
XLogRecPtr
GetWalRcvWriteRecPtr(XLogRecPtr *latestChunkStart, TimeLineID *receiveTLI)
#else
XLogRecPtr
GetWalRcvWriteRecPtr(XLogRecPtr *latestChunkStart)
#endif
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	XLogRecPtr	recptr;

	SpinLockAcquire(&walrcv->mutex);
	copyXLogRecPtr(recptr, walrcv->receivedUpto);
	if (latestChunkStart)
		copyXLogRecPtr((*latestChunkStart), walrcv->latestChunkStart);
	
#ifdef FOUNDER_XDB_SE
	if (receiveTLI)
		*receiveTLI = walrcv->receivedTLI;
#endif
	SpinLockRelease(&walrcv->mutex);

	return recptr;
}

#ifdef FOUNDER_XDB_SE

static
void ReSetPrimaryConnInfo(char *newAddress, int newPort)
{
	extern char *PrimaryConnInfo;
	extern char *app_name;

	char connInfo[1024];
	memset(connInfo, 0, 1024);

	if (app_name)
		sprintf(connInfo, "host=%s port=%d application_name=%s", newAddress, newPort, app_name);
	else
		sprintf(connInfo, "host=%s port=%d", newAddress, newPort);

	int len = (int)strlen(connInfo) + 1;

	if(PrimaryConnInfo)
		free(PrimaryConnInfo);

	PrimaryConnInfo = (char*) malloc(len);
	memset(PrimaryConnInfo, 0, len);

	strcpy(PrimaryConnInfo, connInfo);
}

bool CheckForNewMaster()
{
	extern bool ha_GetElectionResult(bool *, char **, uint16 *);
	extern void MarkStartWalreceiverListener(bool need);
	extern void DestoryLORelList();

	bool iAmMaster = false;
	char *masterAddress = NULL;
	uint16 masterPort = 0;

	if(ha_GetElectionResult(&iAmMaster, &masterAddress, &masterPort))
	{
		if(!iAmMaster)
		{
			Assert(masterAddress != NULL && masterPort != 0);

			/* I am not master, so just only set "PrimaryConnInfo" point to new master. */
			ReSetPrimaryConnInfo(masterAddress, masterPort);
			free(masterAddress);
		} else
		{
			/* I am master, shut down recovery and start up receiver listener. */
			elog(LOG,"Switch to new master");
			MarkStartWalreceiverListener(true);
			RequestRecoveryFinish();
			DestoryLORelList();
		}

		return true;
	}

	return false;
}

extern TransactionId ReadNewTransactionId(void);

void CheckStandbyUpdateError(const char *msg)
{
	if(RecoveryInProgress())
	{
		ereport(ERROR,
			(errcode(ERRCODE_READ_ONLY_SQL_TRANSACTION),
			errmsg(msg)));
	}
}

bool
WalRcvHadCatchupPrimary(void)
{
	/* use volatile pointer to prevent code rearrangement */
	volatile WalRcvData *walrcv = WalRcv;
	bool res = false;
	
	SpinLockAcquire(&walrcv->mutex);
	res = walrcv->hadCatchupPrimary;
	SpinLockRelease(&walrcv->mutex);

	return res;
}

/*
 * Init the walrcv->receivedUpto firstly before find new master.
 * Do this for ha can get valid lsn where slave had reveived to, 
 * even if no master exist.
 */
void
WalRcvInitReceivedUpto(XLogRecPtr recptr)
{
	volatile WalRcvData *walrcv = WalRcv;

	/* See more detail in RequestXLogStreaming about why we start from segment head. */
	if (recptr.xrecoff % XLogSegSize != 0)
		recptr.xrecoff -= recptr.xrecoff % XLogSegSize;
	
	SpinLockAcquire(&walrcv->mutex);
	if ((walrcv->receiveStart.xlogid == 0 &&
		walrcv->receiveStart.xrecoff == 0) &&
		XLByteLT(walrcv->receivedUpto, recptr))
		copyXLogRecPtr(walrcv->receivedUpto, recptr);
	SpinLockRelease(&walrcv->mutex);
}
#endif //FOUNDER-XDB_SE
