#include "postgres.h"
#include <sys/stat.h>

#include "miscadmin.h"
#include "storage/ipc.h"
#include "utils/memutils.h"
#include "utils/tqual.h"
#include "storage/xdb_file.h"
#include "storage/fd.h"
#include "catalog/xdb_catalog.h"
#include "access/xdb_common.h"
#include "postmaster/xdb_main.h"
#include "access/xlog.h"
#include "access/xlog_internal.h"
#include "postmaster/postmaster.h"
#include "port/thread_commu.h"
#include "postmaster/walwriter.h"
#include "postmaster/bgwriter.h"
#include "postmaster/autovacuum.h"
#include "replication/walprotocol.h"

extern void SharedInvalBackendInit(bool);
extern void GetFirstSnapShot();
extern void init_meta_colinfo();
extern int pgarch_start(void);
extern void CheckForSyncParam();
extern void GetMaxTablespaceId();
extern void smgrinit();
extern void initTopXact();
extern void InitBufferPoolAccess();
extern void RelationCacheInitialize(void);
extern void InitFileAccess(void);
extern void InitProcess(void);
extern void InitTransactionArray( void );
extern void StartupXLOG(void);
extern void ShutdownXLOG(int, Datum);
extern bool RecoveryInProgress(void);
extern void fxdb_create_catalog();
extern pid_t get_bgwriter_thid();
extern void startWalreceiverListener(bool force);
extern void fxdb_vacuum_launcher_forkexec();
extern void RebuildDBInfo(bool isInXactBlock);
void 
fxdb_walwriter_forkexec( void );
static void *pg_malloc(size_t size);
static bool mkdatadir(const char *subdir, const char *pg_datda);
static void checkDataDir(void);
static void fxdb_bgwriter_forkexec();
static void fxdb_pgstat_forkexec();
static void InitDataDir(const char *datadir);
void* walsender_thread_func(void *params);
void fxdb_walsender_forkexec(Port * port);
void* walreceiver_thread_func(void *params);
void fxdb_walreceiver_forkexec(void);
static void* bgwriter_thread_func(void *);
static void* vacuum_launcher_thread_func(void *);
static void* vacuum_worker_thread_func(void *);
static void* pgstat_thread_func(void *);
static void set_pgstat_parameters();
static void set_autovacuum_parameters(storage_params *param);
static void setParams(storage_params * params);
static void copyString(char * src, char **dest);
static void fxdb_xlogstarter_forkexec();
static void* xlogstarter_thread_func(void *params);
static void waitForXlogstarter();
static void xlogstarterFinished(int i);
static void start_bgwriter(int i);
static fxdb_thread_t *unregiste_worker(thid_t key);
static void registe_worker(fxdb_thread_t *hworker);
/* non-static func */
void fxdb_vacuum_worker_forkexec();
static void shutdownAllWalsenders();
static void shut_down_thread(fxdb_thread_t *th_handle, int sig,const char* msg = NULL);
static void shut_down_thread_noblock(fxdb_thread_t *th_handle, int sig, const char* msg = NULL);
static void wait_thread_shutdown(fxdb_thread_t *th_handle, const char* msg = NULL);

int	log_min_messages = ERROR;
int log_min_error_statement = ERROR;
fxdb_thread_t *bgwriter_handle = NULL;
fxdb_thread_t *xlog_handle = NULL;
fxdb_thread_t *walreceiverLsnr_handle = NULL;
fxdb_thread_t *walreceiver_handle = NULL;
fxdb_thread_t *pgstat_handle = NULL;
fxdb_thread_t *vacuum_launcher_handle = NULL;
fxdb_thread_t *repha_handle = NULL;
fxdb_thread_t *lsm_merge_handle = NULL;
fxdb_thread_t *lsm_freeze_handle = NULL;
pthread_t bg_writer_tid = 0;
pthread_t postmasterTid = 0;

bool isSingleMode = false;

static bool needListener = false;

bool bInterrupt = false;
int  interruptCode = 0; 

char *app_name = NULL;

THREAD_LOCAL char *application_name = NULL;

struct _walsender_s
{
	fxdb_thread_t **handle;
	slock_t sender_lock;
	int senders;
} walsender_handle = {NULL, 0};

extern bool autovacuum_start_daemon;

typedef struct 
{
	pthread_mutex_t vacuum_worker_mutex;
	HTAB *h_cache;
} ThreadHandleArr;

#if defined(_MSC_VER)
static ThreadHandleArr worker_arr = {{NULL}, NULL};
#else
static ThreadHandleArr worker_arr = {PTHREAD_MUTEX_INITIALIZER,NULL};
#endif


//for PgArch thread
extern fxdb_thread_t *PgArchHandler;
fxdb_thread_t *PgWalWriterHandler = NULL;
bool assert_enabled = true;
bool xlogStarterFinished = false;

extern int		NBuffers;
extern int		work_mem;
extern int		maintenance_work_mem;
extern int		bufferPoolSize;


//archive
extern bool		XLogArchiveMode;
extern char		*XLogArchiveCommand;
//recovery
extern char		*recoveryRestoreCommand;
extern char		*recoveryEndCommand;
extern char		*archiveCleanupCommand;
extern RecoveryTargetType recoveryTarget;
extern TransactionId recoveryTargetXid;
extern TimestampTz recoveryTargetTime;
extern TimeLineID recoveryTargetTLI;
extern bool recoveryTargetIsLatest;
extern char *recoveryTargetName;

//replication 
//master
extern int			PostPortNumber;
extern int			max_wal_senders;
extern int			WalSndDelay; 
extern int			replication_timeout;
extern char	   *SyncRepStandbyNames;
//slave
extern bool		StandbyMode;
extern char		*PrimaryConnInfo;
extern char		*TriggerFile;
//storage
extern double	fileIncrement;


#define swap_assign(condition, arg1, arg2) \
	if(condition) { \
	(arg1) = (arg2); \
	} else { \
	(arg2) = (arg1); \
	}

#define swap_copy(condition, copy_func, arg1, arg2) \
	if(condition) { \
	copy_func(arg1, &(arg2)); \
	} else { \
	copy_func(arg2, &(arg1)); \
	}

static 
void initWorkerCache()
{
	Assert(worker_arr.h_cache == NULL);

	pthread_mutex_init(&worker_arr.vacuum_worker_mutex ,NULL);

	HASHCTL		ctl;

	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(thid_t);
	ctl.entrysize = sizeof(fxdb_thread_t);

	/** hash style Oid  */
	ctl.hash = tag_hash;
	worker_arr.h_cache = 
		hash_create("Worker Cache", autovacuum_max_workers, &ctl, HASH_ELEM | HASH_FUNCTION);
}

static void waitForXlogstarter()
{
	while(!xlogStarterFinished)
	{
		pg_sleep(100000);

		if (bInterrupt)
		{
			shut_down_thread(walreceiverLsnr_handle, SIGTERM, "shutdown receiver listener");
			shut_down_thread(xlog_handle, SIGTERM, "shutdown recovery");
			shut_down_thread(bgwriter_handle, SIGUSR2, "shutdown bgwriter");
			shut_down_thread(repha_handle, SIGUSR2, "shutdown replication ha");

			switch (interruptCode)
			{
				case ERRCODE_RECOVERY_TARGET_TIMELINE_DISMATCH:
					ereport(ERROR, 
								(errcode(ERRCODE_RECOVERY_TARGET_TIMELINE_DISMATCH), 
						         errmsg("Recovery target timeline dismatch.")));
					break;
				case ERRCODE_RECOVERY_TARGET_XLOG_HAD_REMOVED:
					ereport(ERROR, 
								(errcode(ERRCODE_RECOVERY_TARGET_XLOG_HAD_REMOVED),
								errmsg("Request xlog segment had removed.")));
					break;
				case ERRCODE_SYSID_DIFF_BETWEEN_PRIMARY_AND_STANDBY:
					ereport(ERROR, 
								(errcode(ERRCODE_SYSID_DIFF_BETWEEN_PRIMARY_AND_STANDBY),
								errmsg("Storage engine identifier differs between the primary and standby")));
					break;
				case ERRCODE_RECOVERY_REQUEST_POINT_AHEAD_START:
					ereport(ERROR,
								(errcode(ERRCODE_RECOVERY_REQUEST_POINT_AHEAD_START),
								errmsg("Request replaction point ahead of primary start point.")));
					break;
				case ERRCODE_REPHA_TWO_MASTERS:
					ereport(ERROR,
						(errcode(ERRCODE_REPHA_TWO_MASTERS),
						errmsg("Got two masters.")));
					break;

				default:
					ereport(ERROR, (errmsg("Shutdown server threads.")));
			}			
		}		
	}
}

void PostPortAssignAppname(Port *port)
{
	Assert(port != NULL);

	if(port->guc_options == NULL)
		return;

	ListCell *lcell = list_head(port->guc_options);

	while (lcell)
	{
		char	   *name;
		char	   *value;

		name = (char*)lfirst(lcell);
		lcell = lnext(lcell);

		value = (char*)lfirst(lcell);
		lcell = lnext(lcell);

		if(strcmp(name, "application_name") == 0)
		{
			if(application_name != NULL)
			{
				pfree(application_name);
				application_name = NULL;
			}

			int len = (int)strlen(value) + 1;

			Assert(len > 1);

			MemoryContext oldcxt = MemoryContextSwitchTo(ProcessTopMemoryContext);

			application_name = (char *) palloc0(len);

			MemoryContextSwitchTo(oldcxt);

			strcpy(application_name, value);

			return;
		}
	}
}

static 
void start_bgwriter(int i)
{
	if (CheckPostmasterSignal(PMSIGNAL_RECOVERY_STARTED))
	{
		/*
		* Crank up the background writer.	It doesn't matter if this fails,
		* we'll just try again later.
		*/
		fxdb_bgwriter_forkexec();
	} else {
		xlogStarterFinished = true;
	}
}

static void xlogstarterFinished(int i)
{
	xlogStarterFinished = true;
}

void setloglevel(int level)
{
	log_min_messages=level;
}

static void 
set_pgstat_parameters()
{
	extern bool pgstat_track_counts;
	extern pgsocket pgStatSock;

	pgstat_track_counts = true;
	pgStatSock = ~PGINVALID_SOCKET;
}

static void
set_bgwriter_parameters(storage_params *param)
{
	if(param == NULL)
		return;

	swap_assign(param->bgwriter_params.delay > 200, BgWriterDelay, param->bgwriter_params.delay);

	swap_assign(param->bgwriter_params.check_point_warning > 0, CheckPointWarning, param->bgwriter_params.check_point_warning);

	swap_assign(param->bgwriter_params.check_point_timeout > 300, CheckPointTimeout, param->bgwriter_params.check_point_timeout);

	swap_assign(
		param->bgwriter_params.check_point_completion_target > 0.0 && param->bgwriter_params.check_point_completion_target <= 1.0,
		CheckPointCompletionTarget, 
		param->bgwriter_params.check_point_completion_target
		);
}

static void
set_autovacuum_parameters(storage_params *param)
{
	extern double autovacuum_vac_scale;
	extern int autovacuum_vac_thresh;
	extern double autovacuum_anl_scale;
	extern int autovacuum_anl_thresh;
	extern int autovacuum_freeze_max_age;
	extern bool pgstat_track_counts;
	extern int autovacuum_max_workers;
	extern int autovacuum_naptime;

	if (param == NULL)
		return;

	if(param->vac_params.max_workers > DEFAULT_AUTOVAC_WORKER_NUM)
	{
		autovacuum_max_workers = param->vac_params.max_workers;
	} else {
		autovacuum_max_workers = DEFAULT_AUTOVAC_WORKER_NUM;
		param->vac_params.max_workers = DEFAULT_AUTOVAC_WORKER_NUM;
	}

	if(param->vac_params.naptime > DEFAULT_AUTOVAC_NAPTIME)
	{
		autovacuum_naptime = param->vac_params.naptime;
	} else {
		autovacuum_naptime = DEFAULT_AUTOVAC_NAPTIME;
		param->vac_params.naptime = DEFAULT_AUTOVAC_NAPTIME;
	}

	if(param->vac_params.vac_scale > DEFAULT_AUTOVAC_SCALE)
	{
		autovacuum_vac_scale = param->vac_params.vac_scale;
	} else {
		/* use default */
		autovacuum_vac_scale = DEFAULT_AUTOVAC_SCALE;
		param->vac_params.vac_scale = DEFAULT_AUTOVAC_SCALE;
	}

	if(param->vac_params.vac_thresh > DEFAULT_AUTOVAC_THRESH)
	{
		autovacuum_vac_thresh = param->vac_params.vac_thresh;
	} else {
		autovacuum_vac_thresh = DEFAULT_AUTOVAC_THRESH;
		param->vac_params.vac_thresh = DEFAULT_AUTOVAC_THRESH;
	}

	if(param->vac_params.freeze_max_age > DEFAULT_AUTOVAC_FREEZE_MAX_AGE)
	{
		autovacuum_freeze_max_age = param->vac_params.freeze_max_age;
	}	else {
		autovacuum_freeze_max_age = DEFAULT_AUTOVAC_FREEZE_MAX_AGE;
		param->vac_params.freeze_max_age = DEFAULT_AUTOVAC_FREEZE_MAX_AGE;
	}

	autovacuum_start_daemon = param->startVac;
}

static void setParams(storage_params * params)
{
	extern void setInArchiveRecovery(bool isInRecovery);
	extern void InitTableSpaceLocationList(char *);

	isSingleMode = params->isSingleMode;

	swap_assign(params->bufferPoolSize > 0, bufferPoolSize, params->bufferPoolSize);

	NBuffers = (bufferPoolSize)/(BLCKSZ/1024);
	CheckPointSegments = (bufferPoolSize/3)/(XLogSegSize/1024);
	if(CheckPointSegments < 3)
		CheckPointSegments = 3;

	swap_assign(params->workMem > 0, work_mem, params->workMem);

	swap_assign(params->bitmapMem > 0, bitmap_mem, params->bitmapMem);

	swap_assign(params->mainTenanceWorkMem > 0, maintenance_work_mem, params->mainTenanceWorkMem);
	
	swap_assign(params->max_wal_senders > 0, max_wal_senders, params->max_wal_senders);

	swap_assign(params->WalSndDelay > 0, WalSndDelay, params->WalSndDelay);

	swap_assign(params->replication_timeout >= (3* 60000), replication_timeout, params->replication_timeout);

	swap_assign(params->PostPortNumber > 0, PostPortNumber, params->PostPortNumber);

	swap_assign(params->fileIncrement > 0, fileIncrement, params->fileIncrement);

	swap_copy(params->XLogArchivePath, copyString, params->XLogArchivePath, XLogArchiveCommand);

	swap_copy(params->application_name, copyString, params->application_name, app_name);
	
	swap_assign(params->wal_keep_segments > 0, wal_keep_segments, params->wal_keep_segments);
	
	if(params->recoveryRestorePath)
	{
		copyString(params->recoveryRestorePath, &recoveryRestoreCommand);
		if (0 != params->recoveryTargetXid)
		{
			recoveryTarget = RECOVERY_TARGET_XID;
			recoveryTargetXid = params->recoveryTargetXid;
		}
		else if (NULL != params->recoveryTargetName)
		{            
			copyString(params->recoveryTargetName,&recoveryTargetName);
			if(recoveryTargetName)
				recoveryTarget = RECOVERY_TARGET_NAME;
		}
		else if (NULL != params->recoveryTargetTime&&strlen(params->recoveryTargetTime)>=13)
		{
			recoveryTarget = RECOVERY_TARGET_TIME;
			struct tm t;
			char tz[32];
			memset(&t,0,sizeof(t));
			sscanf(params->recoveryTargetTime,"%4d-%2d-%2d %2d:%2d:%2d %s"
				,&t.tm_year,&t.tm_mon,&t.tm_mday
				,&t.tm_hour,&t.tm_min,&t.tm_sec,tz);
			t.tm_year -= 1900;
			t.tm_mon -= 1;
			t.tm_isdst = -1;
			pg_time_t time = (pg_time_t)mktime(&t);
			recoveryTargetTime = time_t_to_timestamptz(time);

			if(params->StandbyMode != 0)
			{
				TimestampTz cur_time = GetCurrentTimestamp();
				if(recoveryTargetTime < cur_time)
					ereport(ERROR,
					(errmsg("recoveryTargetTime earlier than now")));
			}
		}
	}	else {
		copyString(recoveryRestoreCommand, &params->recoveryRestorePath);
	}

	swap_copy(params->archiveCleanupCommand, copyString, params->archiveCleanupCommand, archiveCleanupCommand);

	swap_copy(params->recoveryEndCommand, copyString, params->recoveryEndCommand, recoveryEndCommand);

	swap_copy(params->SyncRepStandbyNames, copyString, params->SyncRepStandbyNames, SyncRepStandbyNames);

	//swap_copy(params->PrimaryConnInfo, copyString, params->PrimaryConnInfo, PrimaryConnInfo);

	swap_copy(params->TriggerFile, copyString, params->TriggerFile, TriggerFile);

	XLogArchiveMode = params->XLogArchiveMode;
	StandbyMode = (params->StandbyMode != 0);

	if(XLogArchiveMode)
	{
		if(XLogArchiveCommand == NULL)
		{
			ereport(FATAL,
				(errcode(ERRCODE_NO_DATA),
				errmsg("archive mode must specify archive_command")));
		}
		wal_level = WAL_LEVEL_ARCHIVE;
	}

	if (params->masterEnableStandby > 0)
		wal_force_send_before_removed = params->wal_force_send_before_removed;
	
	if(params->masterEnableStandby == 1)
	{
		wal_level = WAL_LEVEL_ARCHIVE;
	}
	else if(params->masterEnableStandby == 2)
	{
		wal_level = WAL_LEVEL_HOT_STANDBY;
	}

	/*
	* Check for compulsory parameters
	*/
	if (StandbyMode)
	{
		if (params->StandbyMode == 1 && max_wal_senders > 0)
			wal_level = WAL_LEVEL_ARCHIVE;
		else if (params->StandbyMode == 2)
			wal_level = WAL_LEVEL_HOT_STANDBY;
		
		/* Enable fetching from archive recovery area */
		setInArchiveRecovery(true);
		EnableHotStandby = params->slaveEnableHotStandby;
	}
	else if(params->doRecovery)
	{
		if (recoveryRestoreCommand == NULL)
			ereport(FATAL,
			(errcode(ERRCODE_NO_DATA),
			errmsg("must specify restore_command when standby mode is not enabled")));
		
		setInArchiveRecovery(true);
	}

	swap_assign(params->elevel != 0, log_min_messages, params->elevel);
	
	set_autovacuum_parameters(params);

	set_bgwriter_parameters(params);

	InitTableSpaceLocationList(params->TableSpaceLocationList);
}

static void copyString(char * src, char **dest)
{
	if(src == NULL)
	{
		*dest = NULL;
		return;
	}

	int len = (int)strlen(src);
	if(len < 1)
	{
		*dest = 0;
		return;
	}
	*dest = (char*)malloc(len+1);
	memcpy(*dest, src, len+1);
}
#ifndef WIN32
void sigalrm_handler( int )
{
	ThreadInfo* threadInfo = GetThreadInfo(pthread_self());
	if (NULL != threadInfo  && NULL != threadInfo->handler[SIGALRM])
	{
		threadInfo->handler[SIGALRM](SIGALRM);
		pthread_mutex_lock(&threadInfo->mutex);
		uint32 uMask = 1 << SIGALRM;
		threadInfo->requests &= ~uMask;
		pthread_mutex_unlock(&threadInfo->mutex);
	}
}
#endif
void parse_backtrace_memcontext(storage_params * params)
{
#ifdef ALLOC_INFO
	extern uint32 nbacktraceMemcontext;
	extern char**backtraceMemcontext;
	if((params != NULL) && (params->backtraceMemcontext != NULL))
	{
		int nCount = 0;
		int aCount = 10;
		char **namelists = (char **)malloc(aCount * sizeof(char *));
		char *pName = params->backtraceMemcontext;
		char *p = params->backtraceMemcontext;
		while(1)
		{
			if((*p == ';') || (*p == '\0'))
			{
				int namelen = (int)(p - pName);
				if(namelen != 0)
				{
					char *name = (char *)malloc((namelen + 1) * sizeof(char));
					memcpy(name, pName, namelen);
					name[namelen] = '\0';

					if(nCount >= aCount)
					{
						aCount *= 2;
						namelists = (char **)realloc(namelists, aCount * sizeof(char *));
					}
					namelists[nCount] = name;
					nCount++;
				}
			}

			if(*p == '\0')
			{
				break;
			}
			else if(*p == ';')
			{
				p++;
				pName = p;
			}
			else
			{
				p++;
			}
		}

		if(nCount != 0)
		{
			nbacktraceMemcontext = (uint32)nCount;
			backtraceMemcontext = namelists;
		}
		else
		{
			free(namelists);
		}
	}
#endif
}

void start_engine(const char *datadir, const uint32 thread_num, 
				  bool initDatadir, storage_params * params)
{	
    ereport(WARNING, (errmsg("Starting storage engine.")));
	SetProcessingMode(NormalProcessing);		
	IsUnderPostmaster=false;
#ifdef ALLOC_INFO
	parse_backtrace_memcontext(params);
#endif
	MemoryContextInit(false);
	InitLOMemCxt();
	SetDataDir(datadir);
	PostmasterContext = AllocSetContextCreate(TopMemoryContext, "Postmaster",
		ALLOCSET_DEFAULT_MINSIZE, ALLOCSET_DEFAULT_INITSIZE,
		ALLOCSET_DEFAULT_MAXSIZE);
	MemoryContextSwitchTo(PostmasterContext);

	set_max_safe_fds();
	set_thread_max_safe_fds(thread_num);

	if(params)
	{
		setParams(params);
	}

    ereport(WARNING, (errmsg("Checking parameters.")));
	CheckForSyncParam();

	fdxdb_CreateSingeAttTupleDesc();
	fdxdb_CreateClusterIndexAttTupleDesc();
	fdxb_CreateToastAttTupleDesc(); 
	InitThreadInfoCache();
	PostmasterPid = postmasterTid = pthread_self();
#ifndef WIN32
	signal(SIGALRM,sigalrm_handler);
#endif
	pg_signal(SIGUSR1, start_bgwriter);
	pg_signal(SIGUSR2, xlogstarterFinished);
	//initXact();
	if(!initDatadir)
	{
		checkDataDir();
		ChangeToDataDir();	
	}	
	pg_timezone_initialize();
	if(initDatadir)
		SetProcessingMode(BootstrapProcessing);

	if(initDatadir)
		InitDataDir(datadir);

	CreateSharedMemoryAndSemaphores(false, 54321);

	if(initDatadir)
	{
		BootStrapXLOG();
		SetProcessingMode(NormalProcessing);
	}

	MyProcPid = pthread_self();
    ereport(WARNING, (errmsg("Initing transactions.")));
	RelationCacheInitialize();
	//DebugFileOpen();
	/* Do local initialization of file, storage and buffer managers */  
	InitFileAccess();
	smgrinit();
	InitProcess();
	InitTransactionArray();
	InitBufferPoolAccess();

	//start replication ha
	if(params != NULL){
		ereport(LOG, (errmsg("===========masterEnableStandby: %d, StandbyMode: %d",
			params->masterEnableStandby, params->StandbyMode)));
		if((params->masterEnableStandby != 0) || (params->StandbyMode != 0)){
			ereport(LOG, (errmsg("start ha............")));

			extern void fxdb_start_ha(rep_ha_t *, bool);
			fxdb_start_ha(&params->rep_ha_params, (params->masterEnableStandby != 0));

			ereport(LOG, (errmsg("start ha successed.")));
		}
	}else{
		ereport(LOG, (errmsg("==============HA parameters are not set.==============")));
	}
    ereport(WARNING, (errmsg("start log threads.")));
	if(initDatadir || isSingleMode)
	{
		StartupXLOG();	
		on_shmem_exit(ShutdownXLOG, 0);
	}
	else
	{		
		fxdb_xlogstarter_forkexec();
		waitForXlogstarter();
		fxdb_walwriter_forkexec();
	}
    ereport(WARNING, (errmsg("Trying recoering.")));
	RecoveryInProgress();
	MyBackendId = InvalidBackendId;

    ereport(WARNING, (errmsg("Loading back threads.")));
	SharedInvalBackendInit(false);
	GetFirstSnapShot();
	MyDatabaseId = 11967;
	InitProcessPhase2();
	MyDatabaseTableSpace = DEFAULTTABLESPACE_OID;

	if(initDatadir)
	{
		fxdb_create_catalog();
	}else
	{
		init_meta_colinfo();
	}

	if(!initDatadir)
	{
		//run bgwriter
		fxdb_bgwriter_forkexec();
		if(autovacuum_start_daemon)
		{
			initWorkerCache();
			//run vacuum launcher
			fxdb_vacuum_launcher_forkexec();
		}
		//run pgstat
		fxdb_pgstat_forkexec();
		
		// only enable standby mode need start wal sender process
		// so in this mode ,we start the WalreceiverListener
		// After slave premote to new master, it will start WalreceiverListener
		// at startup xlog process
		if (params != NULL && params->masterEnableStandby != 0 && max_wal_senders > 0)
			startWalreceiverListener(false);

		if(XLogArchiveMode)
		{
			pgarch_start();
		}
	}
    ereport(WARNING, (errmsg("Loading ids.")));
	RebuildDBInfo(false);
	InitTempObjectId();

	InitLORelList();
	GetMaxTablespaceId();

	extern void LsmInit(storage_params *params);
	LsmInit(params);
	ereport(WARNING, (errmsg("Started storage engine.")));
}

void interrupt_start_engine(int code)
{
	interruptCode = code;
	bInterrupt = true;
}

static void*
bgwriter_thread_func(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	extern void BackgroundWriterMain(void);

	fxdb_SubPostmaster_Main(params);
	free(params);
	//main of bgwriter here
	BackgroundWriterMain();
	return NULL;
}
static void*
xlogstarter_thread_func(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	fxdb_SubPostmaster_Main(params);
	free(params);
	StartupProcessMain();

	return NULL;
}

static void*
vacuum_launcher_thread_func(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	extern void AutoVacLauncherMain(int, char *[]);

	fxdb_SubPostmaster_Main(params);
	free(params);
	//main of vacuum launcher here
	AutoVacLauncherMain(0, NULL);
	return NULL;
}

static void* 
vacuum_worker_thread_func(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	extern void AutoVacWorkerMain(int, char *[]);
	extern void fxdb_close_handle(fxdb_thread_t *);
	PG_TRY(); {
		fxdb_SubPostmaster_Main(params);
		//main of vacuum worker here
		AutoVacWorkerMain(0, NULL);
	} PG_CATCH(); {
		/* ... */
	} PG_END_TRY();

	free(params);
	fxdb_thread_t *hworker = 
		unregiste_worker(pthread_self());
	fxdb_close_handle(hworker);

	return NULL;
}

static void*
pgstat_thread_func(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	extern void PgstatCollectorMain(int, char *[]);

	fxdb_SubPostmaster_Main(params);
	free(params);
	set_pgstat_parameters();
	//main of pgstat here
	PgstatCollectorMain(0, NULL);
	return NULL;
}

/*
*  fxdb_bgwriter_forkexec --
*		 here we start a thread to run bgwriter
*/
static void 
fxdb_bgwriter_forkexec()
{
	if (NULL == bgwriter_handle)
	{
		extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);

		BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
		param->MyThreadType = bgwriter;
		bgwriter_handle = fxdb_Thread_Create(bgwriter_thread_func, param);
		
		Assert(bgwriter_handle != NULL);
	}
}

static void 
fxdb_xlogstarter_forkexec()
{
	if (NULL == xlog_handle)
	{
		extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);

		BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
		param->MyThreadType = xlogstarter;
		xlog_handle = fxdb_Thread_Create(xlogstarter_thread_func, param);

		Assert(xlog_handle);
	}
}

/*
*  fxdb_pgstat_forkexec --
*    here we start a thread to run pgstat
*/
static void
fxdb_pgstat_forkexec()
{
	if (NULL == pgstat_handle)
	{
		extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);

		BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
		param->MyThreadType = pgstat;
		pgstat_handle = fxdb_Thread_Create(pgstat_thread_func, param);

		Assert(pgstat_handle != NULL);
	}
}

void*
walsender_thread_func(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	extern int WalSenderMain(void);
	extern void report_to_receiver();

	fxdb_SubPostmaster_Main(params);
	free(params);
	PG_TRY();
	{
		//main of walsender here
		WalSenderMain();
	} PG_CATCH();
	{
		int no = 0;
		
		report_to_receiver();
		char *msg = get_errno_errmsg(no);
		ereport(WARNING, (errmsg("%s", msg)));
		close_port(0, 0);
		proc_exit(0);
	} PG_END_TRY();

	pthread_t t_id = pthread_self();

	/* mark handle invalid */
	int i = max_wal_senders;

	SpinLockAcquire(&walsender_handle.sender_lock);

	while(i--)
	{
		if(walsender_handle.handle[i] && 
			 walsender_handle.handle[i]->thid == t_id)
		{
			walsender_handle.handle[i] = NULL;
			--walsender_handle.senders;
			break;
		}
	}

	Assert(walsender_handle.senders >= 0);

	SpinLockRelease(&walsender_handle.sender_lock);

	return NULL;
}


void 
fxdb_walsender_forkexec(Port * port)
{
	extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);
	extern void fxdb_save_thread_variables(BackendParameters *, BackendId, ThreadType);

	Assert(walsender_handle.handle != NULL && walsender_handle.senders >= 0);

	BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param,3,walsender);
	param->port = port;
	int i = max_wal_senders;

	MemoryContext oldcxt = MemoryContextSwitchTo(ProcessTopMemoryContext);
	SpinLockAcquire(&walsender_handle.sender_lock);

	while(--i >= 0)
	{
		if(!walsender_handle.handle[i])
		{
			walsender_handle.handle[i] = fxdb_Thread_Create(walsender_thread_func, param);
			++walsender_handle.senders;
			break;
		}
	}

	SpinLockRelease(&walsender_handle.sender_lock);
	MemoryContextSwitchTo(oldcxt);

}

static void shut_down_wal_senders()
{
	int i = max_wal_senders;
	
	if (walsender_handle.handle == NULL)
		return ;
	
	while(--i >= 0)
	{
		thid_t t_htid;
		
		SpinLockAcquire(&walsender_handle.sender_lock);
		if (walsender_handle.handle[i] == NULL)
		{
			SpinLockRelease(&walsender_handle.sender_lock);
			continue;
		}

		t_htid = walsender_handle.handle[i]->thid;
		SpinLockRelease(&walsender_handle.sender_lock);
		
#ifdef _DEBUG
		ereport(LOG, (errmsg("%s :%lu,%d","sender", t_htid, SIGUSR2)));
#endif
		// Send SIGUSR2, wal sender will do the last cycle .
		pgkill(t_htid, SIGUSR2);
	}
}

static void wait_wal_senders_down()
{
	int i = max_wal_senders;
	
	if (walsender_handle.handle == NULL)
		return ;
	
	while(--i >= 0)
	{
		thid_t t_htid;
		
		SpinLockAcquire(&walsender_handle.sender_lock);
		if (walsender_handle.handle[i] == NULL)
		{
			SpinLockRelease(&walsender_handle.sender_lock);
			continue;
		}

		t_htid = walsender_handle.handle[i]->thid;
		SpinLockRelease(&walsender_handle.sender_lock);
		
		pthread_join(t_htid,NULL);
	}
}


void*
walreceiver_thread_func(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	extern void WalReceiverMain(void);
	fxdb_SubPostmaster_Main(params);
	free(params);
	//main of bgwriter here
	PG_TRY();
	{
		WalReceiverMain();
	} PG_CATCH(); {
		/* Receiver reach an error(may be lost connection). */
		int msg_no;
		char *msg = get_errno_errmsg(msg_no);

		ereport(WARNING, (errmsg("%s", msg)));

		if(msg_no == ERRCODE_RECOVERY_TARGET_TIMELINE_DISMATCH ||
			msg_no == ERRCODE_RECOVERY_TARGET_XLOG_HAD_REMOVED ||
			msg_no == ERRCODE_SYSID_DIFF_BETWEEN_PRIMARY_AND_STANDBY ||
			msg_no == ERRCODE_RECOVERY_REQUEST_POINT_AHEAD_START)
		{			
			KillStorageEngine(msg_no);
		}

		proc_exit(0);

	} PG_END_TRY();
	return NULL;
}


void 
fxdb_walreceiver_forkexec()
{
	extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);
	extern void fxdb_save_thread_variables(BackendParameters *, BackendId, ThreadType);
	BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param,3,walreceiver);
	walreceiver_handle = fxdb_Thread_Create(walreceiver_thread_func, param);
}

void* walwriter_thread_func(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	extern void WalReceiverMain(void);
	fxdb_SubPostmaster_Main(params);
	free(params);
	WalWriterMain();
	return NULL;
}

void 
fxdb_walwriter_forkexec( void )
{
	if (NULL == PgWalWriterHandler)
	{
		extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);
		extern void fxdb_save_thread_variables(BackendParameters *, BackendId, ThreadType);
		BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
		param->MyThreadType = walwriter;
		PgWalWriterHandler = fxdb_Thread_Create(walwriter_thread_func, param); 

		Assert(PgWalWriterHandler);
	}
}

void 
fxdb_vacuum_launcher_forkexec()

{
	extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);

	if(!vacuum_launcher_handle)
	{
		BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
		param->MyThreadType = vacuum_launcher;
		vacuum_launcher_handle = fxdb_Thread_Create(vacuum_launcher_thread_func, param);

		Assert(vacuum_launcher_handle != NULL);
	}
}

static
void registe_worker(fxdb_thread_t *hworker)
{
	Assert(hworker != NULL && worker_arr.h_cache != NULL);
	thid_t tid = hworker->thid;

	pthread_mutex_lock(&worker_arr.vacuum_worker_mutex);
	fxdb_thread_t *tmp = (fxdb_thread_t *)hash_search(worker_arr.h_cache, 
		&tid,
		HASH_ENTER,
		NULL);
	tmp->os_handle = hworker->os_handle;
	tmp->thid = hworker->thid;
	pthread_mutex_unlock(&worker_arr.vacuum_worker_mutex);

#ifndef WIN32
	pthread_detach(hworker->thid);
#endif //WIN32
}

static
fxdb_thread_t *unregiste_worker(thid_t key)
{
	Assert(worker_arr.h_cache != NULL);

	fxdb_thread_t *hworker = NULL;
	pthread_mutex_lock(&worker_arr.vacuum_worker_mutex);
	hworker = (fxdb_thread_t*)hash_search(worker_arr.h_cache, 
		&key,
		HASH_REMOVE,
		NULL);
	pthread_mutex_unlock(&worker_arr.vacuum_worker_mutex);
	return hworker;
}

void fxdb_vacuum_worker_forkexec()
{
	extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);
	extern void AutoVacWorkerFailed();

	BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
	param->MyThreadType = vacuum_worker;
	fxdb_thread_t *hworker = fxdb_Thread_Create(vacuum_worker_thread_func, param);
	
	/* When fork failed.. */
	if(hworker == NULL &&
		vacuum_launcher_handle != NULL &&
		vacuum_launcher_handle->thid > 0)
	{
		AutoVacWorkerFailed();
		pgkill(vacuum_launcher_handle->thid, SIGUSR2);
	} else 
	{
		registe_worker(hworker);
		pfree(hworker);
	}
}

static void
shut_down_thread(fxdb_thread_t *th_handle, int sig,const char* msg)
{
	if(isSingleMode)
	{
		return;
	}
	if(!th_handle)
	{
		return;
	}
#ifdef _DEBUG
	if(msg == NULL)
		msg = "NULL";

	ereport(LOG, (errmsg("%s :%lu,%d", msg, th_handle->thid, sig)));
#endif
	pgkill(th_handle->thid, sig);
	pthread_join(th_handle->thid,NULL);
}

static void
shut_down_thread_noblock(fxdb_thread_t *th_handle, int sig, const char* msg)
{
	if(isSingleMode)
	{
		return;
	}
	if(!th_handle)
	{
		return;
	}
#ifdef _DEBUG
	if(msg == NULL)
		msg = "NULL";
	ereport(LOG, (errmsg("%s :%lu,%d",msg,th_handle->thid,sig)));
#endif
	pgkill(th_handle->thid, sig);
}

static void
wait_thread_shutdown(fxdb_thread_t *th_handle, const char* msg)
{
	if(isSingleMode)
	{
		return;
	}
	if(!th_handle)
	{
		return;
	}
#ifdef _DEBUG
	if(msg == NULL)
		msg = "NULL";
	ereport(LOG, (errmsg("%s :%lu", msg, th_handle->thid)));
#endif
	pthread_join(th_handle->thid, NULL);
}

static void shut_down_vac_workers()
{
	HASH_SEQ_STATUS seq;
	fxdb_thread_t *worker = NULL;

	if (worker_arr.h_cache == NULL)
		return ;
	
	pthread_mutex_lock(&worker_arr.vacuum_worker_mutex);
	hash_seq_init(&seq, worker_arr.h_cache);
	while((worker = (fxdb_thread_t *)hash_seq_search(&seq)) != NULL)
	{
		shut_down_thread_noblock(worker, SIGTERM);
	}
	pthread_mutex_unlock(&worker_arr.vacuum_worker_mutex);
}

static void wait_vac_workers_down()
{
	HASH_SEQ_STATUS seq;
	fxdb_thread_t *worker = NULL;
	
	if (worker_arr.h_cache == NULL)
			return ;

	pthread_mutex_lock(&worker_arr.vacuum_worker_mutex);
	hash_seq_init(&seq, worker_arr.h_cache);
	while((worker = (fxdb_thread_t *)hash_seq_search(&seq)) != NULL)
	{
		wait_thread_shutdown(worker);
	}
	pthread_mutex_unlock(&worker_arr.vacuum_worker_mutex);
}
	
extern void lock_buf_flush();
extern void lwlock_buf_flush();

#ifdef _DEBUG
#define InitTimer() \
	timeval timerStart, timerEnd;\
	double  timerResult = 0.0, totalTime = 0.0

#define StartTimer() \
		(\
			timerStart.tv_usec = timerEnd.tv_usec = 0,\
			timerStart.tv_sec = timerEnd.tv_sec = 0,\
			gettimeofday(&timerStart, NULL)\
		)
#define EndTimer() \
		(\
			gettimeofday(&timerEnd, NULL),\
			(timerResult = (timerEnd.tv_usec - timerStart.tv_usec)/1000 + \
						(timerEnd.tv_sec - timerStart.tv_sec)*1000),\
			totalTime += timerResult,\
			timerResult\
		)

#define LogTime(msg) \
		ereport(WARNING, (errmsg("%s: time is %.02f (ms)", msg, timerResult)))
#define LogTotalTime(msg) \
		(\
			ereport(WARNING, (errmsg("%s: total time is %.02f (ms)", msg, totalTime))),\
			totalTime = 0, true\
		)
#else
#define InitTimer()
#define StartTimer()
#define EndTimer()
#define LogTime(msg)
#define LogTotalTime(msg)
#endif

void stop_engine(int status)
{
	if (!isSingleMode)
	{
#ifdef _DEBUG
		bShutDownStarted  = true;
#endif

		InitTimer();
		StartTimer();

		// stop some backend of storage,  they can stop concurrently
		shut_down_thread_noblock(lsm_merge_handle, SIGUSR2, "shutdown lsm merger");
		shut_down_thread_noblock(lsm_freeze_handle, SIGUSR2, "shutdown lsm freezer");
		shut_down_thread_noblock(walreceiverLsnr_handle, SIGTERM, "shutdown receiver listener");
		shut_down_thread_noblock(xlog_handle, SIGTERM, "shutdown recovery");
		shut_down_vac_workers();
		shut_down_thread_noblock(vacuum_launcher_handle, SIGTERM,"shutdown vacuum_launcher");
		shut_down_thread_noblock(PgWalWriterHandler,SIGTERM,"shutdown WalWriter");
		EndTimer();
		LogTime("shutdown storage phase1 signal");

		StartTimer();
		wait_thread_shutdown(lsm_merge_handle, "wait lsm merger shutdown");
		wait_thread_shutdown(lsm_freeze_handle, "wait lsm freezer shutdown");
		wait_thread_shutdown(walreceiverLsnr_handle, "wait receiver listener shutdown");
		wait_thread_shutdown(xlog_handle, "wait recovery");
		wait_vac_workers_down();
		wait_thread_shutdown(vacuum_launcher_handle, "wait vacuum_launcher shutdown");
		wait_thread_shutdown(PgWalWriterHandler, "wait WalWriter shutdown");
		
		EndTimer();
		LogTime("shutdown storage phase1 wait");

		// aboving threads had stopped, bgwriter thread can be stopped 
		StartTimer();
		shut_down_thread(bgwriter_handle, SIGUSR2, "shutdown bgwriter");
		EndTimer();
		LogTime("shutdown storage phase2 shutdown");

		// all of wal sender thread, pgarch thread and pgstat thread must be stopped
		// after stoping bgwriter.
		// wal sender need send the last checkpoint xlog record that created by
		// shutdown bgwriter to slave.
		// pgarch need archive the last xlog segment after bgwriter
		// above threads may use pgstat thread to record some statistics information
		StartTimer();
		shut_down_wal_senders();
		shut_down_thread_noblock(PgArchHandler,SIGUSR2, "shutdown ArchHandler");
		shut_down_thread_noblock(pgstat_handle, SIGQUIT, "shutdown pgstat");
		EndTimer();
		LogTime("shutdown storage phase3 signal");

		// wait all backend stop
		StartTimer();
		wait_wal_senders_down();
		EndTimer();
		LogTime("shutdown storage phase3 wait(wal senders)");

		StartTimer();
		wait_thread_shutdown(PgArchHandler, "wait ArchHandler shutdown");
		EndTimer();
		LogTime("shutdown storage phase3 wait(pg_arch)");

		StartTimer();
		wait_thread_shutdown(pgstat_handle, "wait pgstat shutdown");
		EndTimer();
		LogTime("shutdown storage phase3 wait(pg_stat)");

		/*
		 * repha thread should shutdown after walsender, because repha maintain the
		 * heartbeat. If close repha before walsender, slave maybe loss the final checkpoint
		 * record(shutdown checkpoint) on master.
		 */
		StartTimer();
		shut_down_thread(repha_handle, SIGUSR2, "shutdown replication ha");
		EndTimer();
		LogTime("shutdown storage phase4 wait(repha)");
		LogTotalTime("shutdown storage");
	}

#ifdef LOCK_WAIT_INFO_LOG
	lwlock_buf_flush();
	lock_buf_flush();
#endif

	proc_exit(status);
    ereport(WARNING, (errmsg("Stopped storage engine.")));
}

/*
* these values are passed in by makefile defines
*/
static void *
pg_malloc(size_t size)
{
	void	   *result;

	result = malloc(size);
	if (!result)
	{
		fprintf(stderr, _("out of memory\n"));
		exit(1);
	}
	return result;
}
static bool
mkdatadir(const char *subdir, const char *pg_data)
{
	char	   *path;

	path = (char *)pg_malloc(strlen(pg_data) + 2 +
		(subdir == NULL ? 0 : strlen(subdir)));

	if (subdir != NULL)
		sprintf(path, "%s/%s", pg_data, subdir);
	else
		strcpy(path, pg_data);

#ifndef FOUNDER_XDB_SE
	if (pg_mkdir_p(path, S_IRWXU) == 0)
		return true;
#else
	if (pg_mkdir_p(path, S_IRWXU) == 0)
	{
		free(path);
		return true;
	}
#endif

	fprintf(stderr, _("could not create directory:%s,%s\n"),
		path, strerror(errno));

	return false;
}
/*
* Validate the proposed data directory
*/
static void
checkDataDir(void)
{
	char		path[MAXPGPATH];
	FILE	   *fp;
	struct stat stat_buf;

	Assert(DataDir);

	if (stat(DataDir, &stat_buf) != 0)
	{
		if (errno == ENOENT)
			ereport(FATAL,
			(errcode_for_file_access(),
			errmsg("data directory \"%s\" does not exist",
			DataDir)));
		else
			ereport(FATAL,
			(errcode_for_file_access(),
			errmsg("could not read permissions of directory \"%s\": %m",
			DataDir)));
	}

	/* eventual chdir would fail anyway, but let's test ... */
	if (!S_ISDIR(stat_buf.st_mode))
		ereport(FATAL,
		(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
		errmsg("specified data directory \"%s\" is not a directory",
		DataDir)));

	/*
	* Check that the directory belongs to my userid; if not, reject.
	*
	* This check is an essential part of the interlock that prevents two
	* postmasters from starting in the same directory (see CreateLockFile()).
	* Do not remove or weaken it.
	*
	* XXX can we safely enable this check on Windows?
	*/
	//    #ifndef FOUNDER_XDB_SE
	//
	//#if !defined(WIN32) && !defined(__CYGWIN__)
	//    if (stat_buf.st_uid != geteuid())
	//        ereport(FATAL,
	//        (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
	//        errmsg("data directory \"%s\" has wrong ownership",
	//        DataDir),
	//        errhint("The server must be started by the user that owns the data directory.")));
	//#endif
	//#endif //FOUNDER_XB_SE
	//    /*
	//    * Check if the directory has group or world access.  If so, reject.
	//    *
	//    * It would be possible to allow weaker constraints (for example, allow
	//    * group access) but we cannot make a general assumption that that is
	//    * okay; for example there are platforms where nearly all users
	//    * customarily belong to the same group.  Perhaps this test should be
	//    * configurable.
	//    *
	//    * XXX temporarily suppress check when on Windows, because there may not
	//    * be proper support for Unix-y file permissions.  Need to think of a
	//    * reasonable check to apply on Windows.
	//    */
	//#if !defined(WIN32) && !defined(__CYGWIN__)
	//    if (stat_buf.st_mode & (S_IRWXG | S_IRWXO))
	//        ereport(FATAL,
	//        (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
	//        errmsg("data directory \"%s\" has group or world access",
	//        DataDir),
	//        errdetail("Permissions should be u=rwx (0700).")));
	//#endif
#ifndef FOUNDER_XDB_SE
	/* Look for PG_VERSION before looking for pg_control */
	ValidatePgVersion(DataDir);
#endif
	snprintf(path, sizeof(path), "%s/global/fxdb_control", DataDir);

	fp = AllocateFile(path, PG_BINARY_R);
	if (fp == NULL)
	{
		write_stderr("%s: could not find the database system\n"
			"Expected to find it in the directory \"%s\",\n"
			"but could not open file: %s\n",
			DataDir, path, strerror(errno));
		proc_exit(2);
	}
	FreeFile(fp);
}

static void InitDataDir(const char *datadir)
{
	static const char *subdirs[] = {
		"global",
		"fxdb_xlog",
		"fxdb_xlog/archive_status",
		"fxdb_clog",
		"fxdb_notify",
		"fxdb_serial",
		"fxdb_subtrans",
		"fxdb_twophase",
		"fxdb_multixact/members",
		"fxdb_multixact/offsets",
		"base",
		"base/1",
		"fxdb_tblspc",
		"fxdb_stat_tmp",
		"dictionary",
		"cert"
	};
	for (unsigned int i = 0; i < (sizeof(subdirs) / sizeof(char *)); i++)
	{
		if (!mkdatadir(subdirs[i], datadir)) {
			fprintf(stderr, _("create %s is failed\n"), subdirs[i]);
			exit(1);
		}
	} 
	ChangeToDataDir();
}

static void* 
walreceiver_listener_thread_func(void *params)
{
	extern void * waitfor_walreceiver(void * param);

	PG_TRY();
	{
		waitfor_walreceiver(params);
	} PG_CATCH();
	{
		int no = 0;
		char *msg = get_errno_errmsg(no);

		FlushErrorState();
		ereport(WARNING, (errmsg(msg)));
		pfree(msg);
		
		KillStorageEngine(no);
	} PG_END_TRY();

	return NULL;
}

void startWalreceiverListener(bool force)
{
	extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);

	if(walreceiverLsnr_handle != NULL)
		return;

	Assert(walsender_handle.handle == NULL && max_wal_senders > 0);
	if ((wal_level < WAL_LEVEL_ARCHIVE || StandbyMode) && (!force))
	{
		return;
	}
	
	MemoryContext oldcxt = MemoryContextSwitchTo(ProcessTopMemoryContext);

	walsender_handle.handle =
		(fxdb_thread_t **)palloc0(sizeof(fxdb_thread_t*)*max_wal_senders);
	Assert(walsender_handle.handle != NULL);

	SpinLockInit(&walsender_handle.sender_lock);
	walreceiverLsnr_handle = fxdb_Thread_Create(walreceiver_listener_thread_func, NULL);

	MemoryContextSwitchTo(oldcxt);
	Assert(walreceiverLsnr_handle != NULL);
}

void close_port(int code, Datum arg)
{
	extern void ConnFree(Port *conn);

	if(MyProcPort)
	{
		StreamClose(MyProcPort->sock);
		ConnFree(MyProcPort);
		MyProcPort = NULL;
	}
}

void MarkStartWalreceiverListener(bool need)
{
	needListener = need;
}

bool NeedStartWalreceiverListener()
{
	return needListener;
}

bool HasSlave()
{
	SpinLockAcquire(&walsender_handle.sender_lock);
	int num = walsender_handle.senders;
	SpinLockRelease(&walsender_handle.sender_lock);

	return (num > 0);
}

void ShutDownWalRecvListener()
{
	if (walreceiverLsnr_handle != NULL)
	{
		shut_down_thread(walreceiverLsnr_handle, SIGTERM, "shutdown receiver listener");
		walreceiverLsnr_handle = NULL;
	}
}

