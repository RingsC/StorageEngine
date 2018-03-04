/*-------------------------------------------------------------------------
 *
 * globals.c
 *	  global variable declarations
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/init/globals.c
 *
 * NOTES
 *	  Globals used all over the place should be declared here and not
 *	  in other modules.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#ifndef FOUNDER_XDB_SE
#include "catalog/objectaccess.h"
#endif //FOUNDER_XDB_SE
#include "libpq/pqcomm.h"
#include "miscadmin.h"
#include "storage/backendid.h"
#include <pthread.h>


THREAD_LOCAL ProtocolVersion FrontendProtocol;

THREAD_LOCAL volatile bool InterruptPending = false;
THREAD_LOCAL volatile bool QueryCancelPending = false;
THREAD_LOCAL volatile bool ProcDiePending = false;
THREAD_LOCAL volatile bool ImmediateInterruptOK = false;
THREAD_LOCAL volatile uint32 InterruptHoldoffCount = 0;
THREAD_LOCAL volatile uint32 CritSectionCount = 0;

THREAD_LOCAL pthread_t	MyProcPid;
THREAD_LOCAL pg_time_t	MyStartTime;
THREAD_LOCAL struct Port *MyProcPort;
THREAD_LOCAL long		MyCancelKey;
THREAD_LOCAL int			MyPMChildSlot;

/*
 * DataDir is the absolute path to the top level of the PGDATA directory tree.
 * Except during early startup, this is also the server's working directory;
 * most code therefore can simply use relative paths and not reference DataDir
 * explicitly.
 */
char	   *DataDir = NULL;

#ifndef FOUNDER_XDB_SE
THREAD_LOCAL char		OutputFileName[MAXPGPATH];	/* debugging output file */

THREAD_LOCAL char		my_exec_path[MAXPGPATH];	/* full path to my executable */
THREAD_LOCAL char		pkglib_path[MAXPGPATH];		/* full path to lib directory */
#endif //FOUNDER_XDB_SE

#ifdef EXEC_BACKEND
THREAD_LOCAL char		postgres_exec_path[MAXPGPATH];		/* full path to backend */

/* note: currently this is not valid in backend processes */
#endif

THREAD_LOCAL BackendId	MyBackendId = InvalidBackendId;

THREAD_LOCAL Oid			MyDatabaseId = InvalidOid;

THREAD_LOCAL Oid			MyDatabaseTableSpace = InvalidOid;

/*
 * DatabasePath is the path (relative to DataDir) of my database's
 * primary directory, ie, its directory in the default tablespace.
 */
THREAD_LOCAL char	   *DatabasePath = NULL;

pthread_t		PostmasterPid = 0;

/*
 * IsPostmasterEnvironment is true in a postmaster process and any postmaster
 * child process; it is false in a standalone process (bootstrap or
 * standalone backend).  IsUnderPostmaster is true in postmaster child
 * processes.  Note that "child process" includes all children, not only
 * regular backends.  These should be set correctly as early as possible
 * in the execution of a process, so that error handling will do the right
 * things if an error should occur during process initialization.
 *
 * These are initialized for the bootstrap/standalone case.
 */
THREAD_LOCAL bool		IsPostmasterEnvironment = false;
THREAD_LOCAL bool		IsUnderPostmaster = false;
THREAD_LOCAL bool		IsBinaryUpgrade = false;

THREAD_LOCAL bool		ExitOnAnyError = false;

THREAD_LOCAL int			DateStyle = USE_ISO_DATES;
THREAD_LOCAL int			DateOrder = DATEORDER_MDY;
THREAD_LOCAL int			IntervalStyle = INTSTYLE_POSTGRES;
THREAD_LOCAL bool		HasCTZSet = false;
THREAD_LOCAL int			CTimeZone = 0;

bool		enableFsync = true;
THREAD_LOCAL bool		allowSystemTableMods = false;
int			work_mem = 20480;
int			bitmap_mem = 10240;
int			maintenance_work_mem = 16384;

/*
 * Primary determinants of sizes of shared-memory structures.  MaxBackends is
 * MaxConnections + autovacuum_max_workers + 1 (it is computed by the GUC
 * assign hooks for those variables):
 */
int			NBuffers = 16384;
int			bufferPoolSize = 256*1024;
int			MaxBackends = 100;
int			MaxConnections = 90;

THREAD_LOCAL int			VacuumCostPageHit = 1;		/* GUC parameters for vacuum */
THREAD_LOCAL int			VacuumCostPageMiss = 10;
THREAD_LOCAL int			VacuumCostPageDirty = 20;
THREAD_LOCAL int			VacuumCostLimit = 200;
THREAD_LOCAL int			VacuumCostDelay = 0;

THREAD_LOCAL int			VacuumCostBalance = 0;		/* working state for vacuum */
THREAD_LOCAL bool		VacuumCostActive = false;

THREAD_LOCAL int			GinFuzzySearchLimit = 0;



THREAD_LOCAL int	tcp_keepalives_idle;
THREAD_LOCAL int	tcp_keepalives_interval;
THREAD_LOCAL int	tcp_keepalives_count;
/*
 * Hook on object accesses.  This is intended as infrastructure for security
 * and logging plugins.
 */
#ifndef FOUNDER_XDB_SE 
object_access_hook_type object_access_hook = NULL;
#endif //FOUNDER_XDB_SE
