/*-------------------------------------------------------------------------
 *
 * win32_latch.c
 *	  Routines for inter-process latches
 *
 * See unix_latch.c for header comments for the exported functions;
 * the API presented here is supposed to be the same as there.
 *
 * The Windows implementation uses Windows events that are inherited by
 * all postmaster child processes.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/port/win32_latch.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include "miscadmin.h"
//#include "replication/walsender.h"
#include "storage/latch.h"
#include "storage/shmem.h"
#include "port/thread_commu.h"

void
InitLatch(volatile Latch *latch)
{
	latch->is_set = false;
	latch->owner_pid = MyProcPid;
	latch->is_shared = false;

	latch->event = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (latch->event == NULL)
		ereport(ERROR,
                (errcode(ERRCODE_EVENT_CREATE_FAILURE),
                errmsg("CreateEvent failed: error code %d", (int) GetLastError())));
}

void
InitSharedLatch(volatile Latch *latch)
{
	SECURITY_ATTRIBUTES sa;

	latch->is_set = false;
	latch->owner_pid = 0;
	latch->is_shared = true;

	/*
	 * Set up security attributes to specify that the events are inherited.
	 */
	ZeroMemory(&sa, sizeof(sa));
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	latch->event = CreateEvent(&sa, TRUE, FALSE, NULL);
	if (latch->event == NULL)
		ereport(ERROR,
                (errcode(ERRCODE_EVENT_CREATE_FAILURE),
                errmsg("CreateEvent failed: error code %d", (int) GetLastError())));
}

void
OwnLatch(volatile Latch *latch)
{
	/* Sanity checks */
	Assert(latch->is_shared);
	if (latch->owner_pid != 0)
		ereport(ERROR,
                (errcode(ERRCODE_OBJECT_IN_USE),
                errmsg("latch already owned")));

	latch->owner_pid = MyProcPid;
}

void
DisownLatch(volatile Latch *latch)
{
	Assert(latch->is_shared);
	Assert(latch->owner_pid == MyProcPid);

	latch->owner_pid = 0;
}

bool
WaitLatch(volatile Latch *latch, long timeout)
{
	return WaitLatchOrSocket(latch, PGINVALID_SOCKET, false, false, timeout) > 0;
}

int
WaitLatchOrSocket(volatile Latch *latch, pgsocket sock, bool forRead,
				  bool forWrite, long timeout)
{
	DWORD		rc;
	HANDLE		events[3];
	HANDLE		latchevent;
	HANDLE		sockevent = WSA_INVALID_EVENT;	/* silence compiler */
	int			numevents;
	int			result = 0;

	if (latch->owner_pid != MyProcPid)
		ereport(ERROR,
                (errcode(ERRCODE_OBJECT_IN_USE),
                errmsg("cannot wait on a latch owned by another process")));

	latchevent = latch->event;

	events[0] = latchevent;
#ifndef FOUNDER_XDB_SE
	events[1] = pgwin32_signal_event;
#else
	events[1] = pg_getthread_condition();
#endif
	numevents = 2;
	if (sock != PGINVALID_SOCKET && (forRead || forWrite))
	{
		int			flags = 0;

		if (forRead)
			flags |= FD_READ;
		if (forWrite)
			flags |= FD_WRITE;

		sockevent = WSACreateEvent();
		WSAEventSelect(sock, sockevent, flags);
		events[numevents++] = sockevent;
	}

	for (;;)
	{
		/*
		 * Reset the event, and check if the latch is set already. If someone
		 * sets the latch between this and the WaitForMultipleObjects() call
		 * below, the setter will set the event and WaitForMultipleObjects()
		 * will return immediately.
		 */
		if (!ResetEvent(latchevent))
			ereport(ERROR,
                    (errcode(ERRCODE_EVENT_RESET_FAILURE),
                    errmsg("ResetEvent failed: error code %d", (int) GetLastError())));
		if (latch->is_set)
		{
			result = 1;
			break;
		}

		rc = WaitForMultipleObjects(numevents, events, FALSE,
							   (timeout >= 0) ? timeout : INFINITE);
		if (rc == WAIT_FAILED)
			ereport(ERROR,
                    (errcode(ERRCODE_EVENT_WAIT_MULTI_OBJECT_FAILURE),
                    errmsg("WaitForMultipleObjects() failed: error code %d", (int) GetLastError())));
		else if (rc == WAIT_TIMEOUT)
		{
			result = 0;
			break;
		}
		else if (rc == WAIT_OBJECT_0 + 1)
		{
#ifndef FOUNDER_XDB_SE
			pgwin32_dispatch_queued_signals();
#else
			pg_process_signals();
#endif
		}
		else if (rc == WAIT_OBJECT_0 + 2)
		{
			WSANETWORKEVENTS resEvents;

			Assert(sock != PGINVALID_SOCKET);

			ZeroMemory(&resEvents, sizeof(resEvents));
			if (WSAEnumNetworkEvents(sock, sockevent, &resEvents) == SOCKET_ERROR)
				ereport(FATAL,
				        (errcode(ERRCODE_INTERNAL_ERROR),
						errmsg_internal("failed to enumerate network events: %i", (int) GetLastError())));

			if ((forRead && resEvents.lNetworkEvents & FD_READ) ||
				(forWrite && resEvents.lNetworkEvents & FD_WRITE))
				result = 2;
			break;
		}
		else if (rc != WAIT_OBJECT_0)
			ereport(ERROR,
                    (errcode(ERRCODE_EVENT_WAIT_MULTI_OBJECT_FAILURE),
                    errmsg("unexpected return code from WaitForMultipleObjects(): %d", (int) rc)));
	}

	/* Clean up the handle we created for the socket */
	if (sock != PGINVALID_SOCKET && (forRead || forWrite))
	{
		WSAEventSelect(sock, sockevent, 0);
		WSACloseEvent(sockevent);
	}

	return result;
}

void
SetLatch(volatile Latch *latch)
{
	HANDLE		handle;

	/* Quick exit if already set */
	if (latch->is_set)
		return;

	latch->is_set = true;

	/*
	 * See if anyone's waiting for the latch. It can be the current process if
	 * we're in a signal handler.
	 *
	 * Use a local variable here just in case somebody changes the event field
	 * concurrently (which really should not happen).
	 */
	handle = latch->event;
	if (handle)
	{
		SetEvent(handle);

		/*
		 * Note that we silently ignore any errors. We might be in a signal
		 * handler or other critical path where it's not safe to call elog().
		 */
	}
}

void
ResetLatch(volatile Latch *latch)
{
	/* Only the owner should reset the latch */
	Assert(latch->owner_pid == MyProcPid);

	latch->is_set = false;
}
