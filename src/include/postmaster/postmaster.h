/*-------------------------------------------------------------------------
 *
 * postmaster.h
 *	  Exports from postmaster/postmaster.c.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/postmaster/postmaster.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef _POSTMASTER_H
#define _POSTMASTER_H
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef USE_BONJOUR
#include <dns_sd.h>
#endif

#include "access/transam.h"
#include "access/xlog.h"
#include "catalog/pg_control.h"
#include "lib/dllist.h"
#ifndef FOUNDER_XDB_SE
#include "bootstrap/bootstrap.h"
#include "libpq/auth.h"


#else 
#include "libpq/ip.h"
#include "libpq/libpq.h"
#include "libpq/libpq-be.h"
#include "storage/xdb_storage_ipc.h"
#endif //FOUNDER_XDB_SE
#ifndef FOUNDER_XDB_SE
#include "libpq/pg_signal.h"
#else
#include "port/thread_commu.h"
#endif
#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/autovacuum.h"
#include "postmaster/fork_process.h"
#include "postmaster/pgarch.h"
#include "postmaster/postmaster.h"
#include "postmaster/syslogger.h"
#ifndef FOUNDER_XDB_SE
#include "replication/walsender.h"
#endif //FOUNDER_XDB_SE
#include "storage/fd.h"
#include "storage/ipc.h"
#include "storage/pg_shmem.h"
#include "storage/pmsignal.h"
#include "storage/proc.h"
#include "tcop/tcopprot.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/memutils.h"
#ifndef FOUNDER_XDB_SE
#include "utils/ps_status.h"
#endif
#ifdef EXEC_BACKEND
#include "storage/spin.h"
#endif

#ifdef FOUNDER_XDB_SE

#define INFO_PRINTF(stream, fmt, ...) \
	char t_msg[1024]; \
	memset(t_msg, 0, 1024); \
	snprintf(t_msg, 1024, fmt, ##__VA_ARGS__); \
	int t_msg_len = (int)strlen(t_msg); \
	snprintf(t_msg + t_msg_len, 1024 - t_msg_len, "\n"); \
	write(fileno(stream), t_msg, (int)strlen(t_msg))


#endif //FOUNDER_XDB_SE

#ifdef WIN32
typedef unsigned		thid_t;
typedef struct fxdb_thread_t
{
    thid_t				thid;
		HANDLE				os_handle;
} fxdb_thread_t;
#define fxdb_thread_id(_t)	((thid_t) (_t).thid)
#define fxdb_thread_handle(_t)	((HANDLE) (_t).os_handle)

#else
#include <pthread.h>
typedef pthread_t		thid_t;

typedef struct fxdb_thread_t
{
	pthread_t			os_handle;
	pthread_t			thid;
} fxdb_thread_t;

#define fxdb_thread_id(_t)	((thid_t) (_t).os_handle)

#endif
/* GUC options */

typedef union LWLockPadded LWLock;

typedef struct bkend
{
	pthread_t	pid;			/* process id of backend */
	long		cancel_key;		/* cancel key for cancels for this backend */
	int			child_slot;		/* PMChildSlot for this backend, if any */
	bool		is_autovacuum;	/* is it an autovacuum process? */
	bool		dead_end;		/* is it going to send an error and quit? */
	Dlelem		elem;			/* list link in BackendList */
} Backend;

enum ThreadType 
{
	backend,
	bgwriter,
	pgstat,
	vacuum_launcher,
	vacuum_worker,
	bootstrap,
	postmaster,
	archiver,
	walsender,
	walreceiver,
	xlogstarter,
	walwriter,
	replha,
};

enum RunningMode
{
	 BootStrapMode = 101399,
	 SingleUserMode = 201399,
	 MultiUserMode = 301399
};

typedef struct
{
	Port	*	port;
	ThreadType  MyThreadType;
#ifndef FOUNDER_XDB_SE
	BackendId	MyBackendId;
	char		DataDir[MAXPGPATH];
	long		MyCancelKey;
	int			MyPMChildSlot;
	unsigned long UsedShmemSegID;
	void	   *UsedShmemSegAddr;
	slock_t    *ShmemLock;
	VariableCache ShmemVariableCache;
	Backend    *ShmemBackendArray;
	LWLock	   *LWLockArray;
	slock_t    *ProcStructLock;
	PROC_HDR   *ProcGlobal;
	PGPROC	   *AuxiliaryProcs;
	PMSignalData *PMSignalState;
	pthread_t	PostmasterPid;
	TimestampTz PgStartTime;
	TimestampTz PgReloadTime;
	bool		redirection_done;
#ifdef WIN32
	HANDLE		PostmasterHandle;
	HANDLE		initial_signal_pipe;
	HANDLE		syslogPipe[2];
#else
	int			syslogPipe[2];
#endif

	char		my_exec_path[MAXPGPATH];
	char		pkglib_path[MAXPGPATH];
	char		ExtraOptions[MAXPGPATH];
#endif //FOUNDER_XDB_SE
} BackendParameters;

extern THREAD_LOCAL bool EnableSSL;
extern THREAD_LOCAL bool SilentMode;
extern THREAD_LOCAL int	ReservedBackends;
extern  int	PostPortNumber;
extern THREAD_LOCAL int	Unix_socket_permissions;
extern THREAD_LOCAL char *Unix_socket_group;
extern THREAD_LOCAL char *UnixSocketDir;
extern THREAD_LOCAL char *ListenAddresses;
extern THREAD_LOCAL bool ClientAuthInProgress;
extern THREAD_LOCAL int	PreAuthDelay;
extern THREAD_LOCAL int	AuthenticationTimeout;
extern THREAD_LOCAL bool Log_connections;
extern THREAD_LOCAL bool log_hostname;
extern THREAD_LOCAL bool enable_bonjour;
extern THREAD_LOCAL char *bonjour_name;
extern THREAD_LOCAL bool restart_after_crash;

#ifdef FOUNDER_XDB_SE
extern THREAD_LOCAL bool ErrorCauseExit;
#endif //FOUNDER_XDB_SE

#ifdef WIN32
extern THREAD_LOCAL HANDLE PostmasterHandle;
#endif

extern const char *progname;
#ifndef FOUNDER_XDB_SE
extern int	PostmasterMain(int argc, char *argv[]);
#endif
extern void ClosePostmasterPorts(bool am_syslogger);

extern int	MaxLivePostmasterChildren(void);

#ifdef EXEC_BACKEND
#ifndef FOUNDER_XDB_SE
extern pid_t postmaster_forkexec(int argc, char *argv[]);
#endif
extern int	SubPostmasterMain(int argc, char *argv[]);
extern void *fxdb_SubPostmaster_Main(void *params);

extern void fxdb_save_thread_variables(BackendParameters *param,BackendId id,ThreadType tt);
extern void fxdb_read_thread_variables(BackendParameters *param);

extern Size ShmemBackendArraySize(void);
extern void ShmemBackendArrayAllocation(void);
#endif

#ifdef FOUNDER_XDB_SE
extern void KillStorageEngine(int errCode);
#endif //FOUNDER_XDB_SE

#endif   /* _POSTMASTER_H */
