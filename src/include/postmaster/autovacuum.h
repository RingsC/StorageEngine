/*-------------------------------------------------------------------------
 *
 * autovacuum.h
 *	  header file for integrated autovacuum daemon
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/postmaster/autovacuum.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef AUTOVACUUM_H
#define AUTOVACUUM_H

#include "storage/lock.h"

/* GUC variables */
extern bool autovacuum_start_daemon;
extern int	autovacuum_max_workers;
extern int	autovacuum_naptime;
extern int autovacuum_vac_thresh;
extern double autovacuum_vac_scale;
extern int autovacuum_anl_thresh;
extern double autovacuum_anl_scale;
extern int autovacuum_freeze_max_age;
extern int	autovacuum_vac_cost_delay;
extern int	autovacuum_vac_cost_limit;

/* autovacuum launcher PID, only valid when worker is shutting down */
extern THREAD_LOCAL int	AutovacuumLauncherPid;

extern int	Log_autovacuum_min_duration;

#ifdef FOUNDER_XDB_SE

#define DEFAULT_AUTOVAC_WORKER_NUM 3
#define DEFAULT_AUTOVAC_NAPTIME 20 * 60
#define DEFAULT_AUTOVAC_SCALE 0.20000000000000001
#define DEFAULT_AUTOVAC_THRESH 50
#define DEFAULT_AUTOVAC_FREEZE_MAX_AGE 200000000

#endif //FOUNDER_XDB_SE

/* Status inquiry functions */
extern bool AutoVacuumingActive(void);
extern bool IsAutoVacuumLauncherProcess(void);
extern bool IsAutoVacuumWorkerProcess(void);

#define IsAnyAutoVacuumProcess() \
	(IsAutoVacuumLauncherProcess() || IsAutoVacuumWorkerProcess())

/* Functions to start autovacuum process, called from postmaster */
extern void autovac_init(void);
#ifndef FOUNDER_XDB_SE
extern int	StartAutoVacLauncher(void);
#endif
extern int	StartAutoVacWorker(void);

/* called from postmaster when a worker could not be forked */
extern void AutoVacWorkerFailed(void);

/* autovacuum cost-delay balancer */
extern void AutoVacuumUpdateDelay(void);

#ifdef EXEC_BACKEND
extern void AutoVacLauncherMain(int argc, char *argv[]);
extern void AutoVacWorkerMain(int argc, char *argv[]);
extern void AutovacuumWorkerIAm(void);
extern void AutovacuumLauncherIAm(void);
#endif

/* shared memory stuff */
extern Size AutoVacuumShmemSize(void);
extern void AutoVacuumShmemInit(void);

#endif   /* AUTOVACUUM_H */
