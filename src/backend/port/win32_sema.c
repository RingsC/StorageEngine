/*-------------------------------------------------------------------------
 *
 * win32_sema.c
 *	  Microsoft Windows Win32 Semaphores Emulation
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *	  src/backend/port/win32_sema.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "port/thread_commu.h"
#include "miscadmin.h"
#include "storage/ipc.h"
#include "storage/pg_sema.h"
#include <pthread.h>
static THREAD_LOCAL HANDLE *mySemSet;		/* IDs of sema sets acquired so far */
static THREAD_LOCAL int	numSems;			/* number of sema sets acquired so far */
static THREAD_LOCAL int	maxSems;			/* allocated size of mySemaSet array */

static void ReleaseSemaphores(int code, Datum arg);

/*
 * PGReserveSemaphores --- initialize semaphore support
 *
 * In the Win32 implementation, we acquire semaphores on-demand; the
 * maxSemas parameter is just used to size the array that keeps track of
 * acquired semas for subsequent releasing.  We use anonymous semaphores
 * so the semaphores are automatically freed when the last referencing
 * process exits.
 */
void
PGReserveSemaphores(int maxSemas, int port)
{
	mySemSet = (HANDLE *) malloc(maxSemas * sizeof(HANDLE));
	if (mySemSet == NULL)
		ereport(PANIC,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg("out of memory")));
	numSems = 0;
	maxSems = maxSemas;

	on_shmem_exit(ReleaseSemaphores, 0);
}

/*
 * Release semaphores at shutdown or shmem reinitialization
 *
 * (called as an on_shmem_exit callback, hence funny argument list)
 */
static void
ReleaseSemaphores(int code, Datum arg)
{
	int			i;

	for (i = 0; i < numSems; i++)
		CloseHandle(mySemSet[i]);
	free(mySemSet);
}

/*
 * PGSemaphoreCreate
 *
 * Initialize a PGSemaphore structure to represent a sema with count 1
 */
void
PGSemaphoreCreate(PGSemaphore sema)
{
	HANDLE		cur_handle;
	SECURITY_ATTRIBUTES sec_attrs;

	/* Can't do this in a backend, because static state is postmaster's */
	Assert(!IsUnderPostmaster);

	if (numSems >= maxSems)
		ereport(PANIC,
                (errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
                errmsg("too many semaphores created")));

	ZeroMemory(&sec_attrs, sizeof(sec_attrs));
	sec_attrs.nLength = sizeof(sec_attrs);
	sec_attrs.lpSecurityDescriptor = NULL;
	sec_attrs.bInheritHandle = TRUE;

	/* We don't need a named semaphore */
	cur_handle = CreateSemaphore(&sec_attrs, 1, 32767, NULL);
	if (cur_handle)
	{
		/* Successfully done */
		*sema = cur_handle;
		mySemSet[numSems++] = cur_handle;
	}
	else
		ereport(PANIC,
				(errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("could not create semaphore: error code %d", (int) GetLastError())));
}

/*
 * PGSemaphoreReset
 *
 * Reset a previously-initialized PGSemaphore to have count 0
 */
void
PGSemaphoreReset(PGSemaphore sema)
{
	/*
	 * There's no direct API for this in Win32, so we have to ratchet the
	 * semaphore down to 0 with repeated trylock's.
	 */
	while (PGSemaphoreTryLock(sema));
}

/*
 * PGSemaphoreLock
 *
 * Lock a semaphore (decrement count), blocking if count would be < 0.
 * Serve the interrupt if interruptOK is true.
 */
void
PGSemaphoreLock(PGSemaphore sema, bool interruptOK)
{
	DWORD		ret;
	HANDLE		wh[2];

	wh[0] = *sema;
#ifndef FOUNDER_XDB_SE
	wh[1] = pgwin32_signal_event;
#else
	wh[1] = pg_getthread_condition();;
#endif

	/*
	 * As in other implementations of PGSemaphoreLock, we need to check for
	 * cancel/die interrupts each time through the loop.  But here, there is
	 * no hidden magic about whether the syscall will internally service a
	 * signal --- we do that ourselves.
	 */
	do
	{
		ImmediateInterruptOK = interruptOK;
		CHECK_FOR_INTERRUPTS();

		errno = 0;
		ret = WaitForMultipleObjectsEx(2, wh, FALSE, INFINITE, TRUE);

		if (ret == WAIT_OBJECT_0)
		{
			/* We got it! */
			return;
		}
		else if (ret == WAIT_OBJECT_0 + 1)
		{
			/* Signal event is set - we have a signal to deliver */
#ifndef FOUNDER_XDB_SE
			pgwin32_dispatch_queued_signals();
#else
			pg_process_signals();
#endif
			errno = EINTR;
		}
		else
			/* Otherwise we are in trouble */
			errno = EIDRM;

		ImmediateInterruptOK = false;
	} while (errno == EINTR);

	if (errno != 0)
		ereport(FATAL,
		        (errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("could not lock semaphore: error code %d", (int) GetLastError())));
}

/*
 * PGSemaphoreUnlock
 *
 * Unlock a semaphore (increment count)
 */
void
PGSemaphoreUnlock(PGSemaphore sema)
{
	if (!ReleaseSemaphore(*sema, 1, NULL))
		ereport(FATAL,
		        (errcode(ERRCODE_INTERNAL_ERROR),
				errmsg("could not unlock semaphore: error code %d", (int) GetLastError())));
}

/*
 * PGSemaphoreTryLock
 *
 * Lock a semaphore only if able to do so without blocking
 */
bool
PGSemaphoreTryLock(PGSemaphore sema)
{
	DWORD		ret;

	ret = WaitForSingleObject(*sema, 0);

	if (ret == WAIT_OBJECT_0)
	{
		/* We got it! */
		return true;
	}
	else if (ret == WAIT_TIMEOUT)
	{
		/* Can't get it */
		errno = EAGAIN;
		return false;
	}

	/* Otherwise we are in trouble */
	ereport(FATAL,
	        (errcode(ERRCODE_INTERNAL_ERROR),
			errmsg("could not try-lock semaphore: error code %d", (int) GetLastError())));

	/* keep compiler quiet */
	return false;
}
