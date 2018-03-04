/*
 * ha_env.cpp
 */

#include "ha_head.h"


ha_env_t g_ha_env;
bool select_thread_need_stop = false;
static char s_init_err_str[512];
FILE *g_ha_log_file = NULL;





/* *****************************************************************************
* 						ha env site ops start
***************************************************************************** */
void ha_AllocSitesMem(ha_env_t *p_env, uint32 asites)
{
	ha_site_t *p_sites = p_env->p_sites;
	uint32 alloc_size = sizeof(ha_site_t) * asites;

	ha_DebugEnv("current asites is %d, want to alloc: %d",
			p_env->asites, asites);

	if(p_env->asites > asites){
		return;
	}

	if(p_env->asites != 0){
		p_sites = (ha_site_t *)realloc(p_sites, alloc_size);
	}else{
		p_sites = (ha_site_t *)malloc(alloc_size);
	}

	if(p_sites == NULL){
		ha_EreportOOM();
	}

	memset((char *)p_sites + p_env->asites, 0, alloc_size);

	p_env->p_sites = p_sites;
	p_env->asites = asites;

	return;
}

void ha_InvalidOneSite(ha_site_t *p_site)
{
	if((p_site == NULL) || (!(p_site->valid))){
		ha_Assert(false);
		return;
	}

	ha_Log("Invalid site[%s:%d], eid %d",
		p_site->conf.pc_ip, p_site->conf.port, p_site->conf.eid);

	/* clear site conn */
	if(!ha_SocketIsInvalid(p_site->conn.tcp_conn.socket)){
		ha_CloseSocket(p_site->conn.tcp_conn.socket);
	}

	if(p_site->conf.pc_ip != NULL){
		free(p_site->conf.pc_ip);
	}
	if(p_site->conf.p_ai != NULL){
		ha_FreeAddrinfo(p_site->conf.p_ai);
	}

	p_site->valid = false;
	p_site->conf.pc_ip = NULL;
	p_site->conf.p_ai = NULL;

	return;
}
/* *****************************************************************************
* 						ha env site ops end
***************************************************************************** */













/* *****************************************************************************
* 						ha parse params start
***************************************************************************** */
bool ha_ParseIpPort(const char *pc_site, char **pp_ip, uint16 *p_port)
{
	char *pc_ip = NULL;
	const char *pc_port = NULL;
	int i = 0;
	bool ret = false;

	/* find the last ':' */
	for(i = (int)strlen(pc_site) - 1; i > 0 ; i--){
		if(pc_site[i] == ':'){
			ret = true;
			break;
		}
	}

	if(ret){
		pc_ip = (char *)malloc((i + 1) * sizeof(char));
		memcpy(pc_ip, pc_site, i);
		pc_ip[i] = '\0';

		pc_port = &pc_site[i + 1];

		*p_port = (uint16)atoi(pc_port);
		*pp_ip = pc_ip;
	}

	return ret;
}

bool ha_NsitesByGroupList(const char *pc_group, char c_decoll,
	uint32 *p_nsites)
{
	uint32 nsemis = 0;
	const char *p_c = pc_group;

	if(pc_group == NULL){
		ha_Error("group is NULL");
		return false;
	}

	while(*p_c != '\0'){
		if(*p_c == c_decoll){
			nsemis++;
		}
		p_c++;
	}

	if(nsemis == 0){
		ha_Error("Wrong group list[%s]: it must at least have 2 sites",
			pc_group);
		return false;
	}

	*p_nsites = nsemis + 1;
	return true;
}

char *ha_SiteByGroupList(const char *p_group_list, char c_decoll,
	uint32 index)
{
	uint32 nsites = 0;
	const char *p_c = p_group_list;
	const char *p_site_s = NULL;
	char *p_site;
	uint32 site_len = 0;

	while(*p_c != '\0'){
		if(nsites == index){
			p_site_s = p_c;
			break;
		}

		if(*p_c == c_decoll){
			nsites++;
		}

		p_c++;
	}

	if(p_site_s == NULL){
		ha_Error("Not have %d sites in group list[%s].",
			index + 1, p_group_list);
		return NULL;
	}

	while((*p_c != '\0') && (*p_c != c_decoll)){
		site_len++;
		p_c++;
	}

	if(site_len == 0){
		ha_Error("Wrong group list[%s].", p_group_list);
	}

	p_site = (char *)malloc(site_len + 1);
	if(p_site == NULL){
		ha_EreportOOM();
		return NULL;
	}

	memcpy(p_site, p_site_s, site_len);
	p_site[site_len] = '\0';

	return p_site;
}

bool ha_InitOneSite(ha_env_t *p_env, uint32 eid, const char *pc_site)
{
	ha_site_t *p_site = &p_env->p_sites[eid];
	ha_site_conf_t *p_site_conf = &p_site->conf;
	bool init_ok = false;

	ha_DebugEnv("Start to init site: eid[%d]", eid);

	memset((char *)p_site, 0, sizeof(ha_site_t));

	p_site_conf->eid = eid;

	if(ha_ParseIpPort(pc_site, &p_site_conf->pc_ip, &p_site_conf->port)){
		p_site_conf->p_ai = ha_BuildAddrinfo(p_site_conf->pc_ip, p_site_conf->port);
		if(p_site_conf->p_ai != NULL){
			init_ok = true;
		}
	}

	if(init_ok){
		ha_DebugEnv("init site[%s] eid[%d] successed.", pc_site, eid);

		p_site->valid = true;
	}else{
		ha_Log("init site[%s] eid[%d] failed.", pc_site, eid);

		if(p_site_conf->pc_ip != NULL){
			free(p_site_conf->pc_ip);
		}

		if(p_site_conf->p_ai != NULL){
			ha_FreeAddrinfo(p_site->conf.p_ai);
		}

		p_site->valid = false;
		p_site->conf.pc_ip = NULL;
		p_site->conf.p_ai = NULL;
	}

	return init_ok;
}

bool ha_InitAllSites(ha_env_t *p_env, const char *pc_group, char c_decoll)
{
	uint32 nsites = 0;
	//uint32 my_eid = HA_EID_INVALID;

	if(!ha_NsitesByGroupList(pc_group, c_decoll, &nsites)){
		return false;
	}

	ha_DebugEnv("Nsites: %d", nsites);

	ha_AllocSitesMem(p_env, nsites);

	if(p_env->asites < nsites){
		ha_Error("Alloc sites mem failed.");
		return false;
	}

	for(uint32 i = 0; i < nsites; i++){
		char *pc_site = ha_SiteByGroupList(pc_group, c_decoll, i);

		if(pc_site == NULL){
			return false;
		}

		if(!ha_InitOneSite(p_env, i, pc_site)){
			ha_Error("wrong arg[%d]: %s", i, pc_site);
			return false;
		}
	}
#if 0
	if(my_eid == HA_EID_INVALID){
		ha_Error("Not have my site.");
		return false;
	}
#endif
	p_env->nsites = nsites;
	p_env->max_eid = nsites - 1;

	ha_DebugEnv("parse option end.........");

	return true;
}

bool ha_SetMyEid(ha_env_t *p_env, const char *pc_ip, uint16 port)
{
	ha_myaddr_t *p_myaddrs = NULL;
	bool ret = false;

	if(pc_ip == NULL){
		p_myaddrs = ha_GetAllMyAddrs();
		if(p_myaddrs == NULL){
			ha_Error("Get my addr failed");
			return false;
		}
	}

	for(uint32 eid = 0; eid <= p_env->max_eid; eid++){
		if(port == p_env->p_sites[eid].conf.port){
			if(pc_ip != NULL){
				if(strcmp(pc_ip, p_env->p_sites[eid].conf.pc_ip) == 0){
					p_env->my_eid = eid;
					ret = true;
					break;
				}
			}else{
				if(ha_IsSiteMine(&p_env->p_sites[eid].conf, p_myaddrs, port)){
					p_env->my_eid = eid;
					ret = true;
					break;
				}
			}
		}
	}

	if(p_myaddrs != NULL){
		ha_FreeAllMyAddrs(p_myaddrs);
	}
	
	return ret;
}

bool ha_UconfInit(ha_env_t * p_env, const char *pc_ip, rep_ha_t *p_ha_conf)
{
	char c_decoll = ';';
	const uint32 us_per_s = 1000000;

	/* sites conf */
	if(!ha_InitAllSites(p_env, p_ha_conf->gourp_list, c_decoll)){
		if (p_ha_conf->gourp_list == NULL)
			snprintf(s_init_err_str, sizeof(s_init_err_str),"Grouplist is NULL");
		else
			snprintf(s_init_err_str, sizeof(s_init_err_str),
					"Grouplist[%s] is wrong", p_ha_conf->gourp_list);
		ha_Error("init sites failed.");
		return false;
	}

	/* set my eid */
	if((!ha_SetMyEid(p_env, pc_ip, p_ha_conf->listen_port))
	|| (p_env->my_eid == HA_EID_INVALID)){
		snprintf(s_init_err_str, sizeof(s_init_err_str),
				"can't find my site[listen_port: %d] in group list[%s]",
				p_ha_conf->listen_port, p_ha_conf->gourp_list);
		ha_Error("set my eid failed.");
		return false;
	}

	/* election conf */
	p_env->priority = p_ha_conf->priority;

	p_env->timeout1_us = p_ha_conf->phrase1_timeout * us_per_s;
	p_env->timeout2_us = p_ha_conf->phrase2_timeout * us_per_s;
	p_env->timeout4_us = p_ha_conf->election_timeout * us_per_s;

	p_env->min_leis = p_ha_conf->min_leis;
	p_env->min_votes = p_ha_conf->min_votes;

	p_env->timeout_hb_us = p_ha_conf->heartbeat_timeout * us_per_s;
	p_env->hb_interval_us = p_ha_conf->heartbeat_interval * us_per_s;

	/* communication conf */
	p_env->first_reconn_time_us = p_ha_conf->first_reconn_time * us_per_s;
	p_env->tmp_conn_life_time_us = p_ha_conf->tmp_conn_life_time * us_per_s;
	p_env->election_delay_s = p_ha_conf->election_delay;
	p_env->manual_start_election = ((p_ha_conf->manual_start_election == 0) ? false : true);

	p_env->send_timeout_s = p_ha_conf->send_timeout;
	p_env->recv_timeout_s = p_ha_conf->recv_timeout;

	p_env->ka_idle_s = p_ha_conf->tcp_ka_idle;
	p_env->ka_interval_s = p_ha_conf->tcp_ka_interval;
	p_env->ka_count = p_ha_conf->tcp_ka_count;

	p_env->max_etimes_2sites = p_ha_conf->election_count_2sites;

	return true;
}

void ha_DumpUconf(const ha_env_t *p_env)
{
//#define HA_DEBUG_ENV 0
#ifdef HA_DEBUG_ENV
	uint32 eid = 0;
	uint32 my_eid = ha_MyEid(p_env);

	ha_Printf("===================================================\n");

	ha_Printf("nsites: %d\n\n", p_env->nsites);
	ha_Printf("Sites:\n");
	for(uint32 eid = 0; eid <= ha_MaxEid(p_env); eid++){
		ha_site_conf_t *p_site_conf = ha_SiteConfByEid(p_env, eid);
		
		if(ha_IsMyEid(p_env, eid)){
			ha_Printf("My site: eid[%d], ip[%s], port[%d] priority[%d]\n",
				p_site_conf->eid, p_site_conf->pc_ip, p_site_conf->port,
				ha_MyPriority(p_env));
		}

		if(ha_IsOtherEid(p_env, eid)){
			ha_Printf("Other site: eid[%d], ip[%s], port[%d]\n",
				p_site_conf->eid, p_site_conf->pc_ip, p_site_conf->port);
		}
	}

	ha_Printf("timeout1: %dus, timeout2: %dus, timeout2: %dus\n",
		ha_Timeout1(p_env), ha_Timeout2(p_env), ha_Timeout4(p_env));
	ha_Printf("heartbeat timeout: %dus, heartbeat interval: %dus\n",
		ha_TimeoutHb(p_env), ha_HbTimeInterval(p_env));
	ha_Printf("first reconn time: %d, tmp conn life time: %d\n",
		ha_FirstReconnTime(p_env), ha_TmpConnLifeTime(p_env));

	ha_Printf("===================================================\n\n");
#endif
	return;
}
/* *****************************************************************************
*						ha parse params stop
***************************************************************************** */















/* *****************************************************************************
*						ha init start
***************************************************************************** */
bool ha_CommInit(ha_env_t *p_env, bool assign_master)
{
	ha_site_conf_t *p_my_site_conf = ha_MySiteConf(p_env);
	bool ret;

	ha_DebugEnv("Communication init start.");

	if(!ha_IsTestSm(p_env)){
		/* connect to all the other sites */
		ha_DebugEnv("Conn init start: to connect all other sites");
		for(uint32 eid = 0; eid <= ha_MaxEid(p_env); eid++){
			if(ha_IsOtherEid(p_env, eid)){
				ret = ha_FirstConnSite(p_env, eid);
				if(assign_master && ret){
					ha_site_conf_t *p_site_conf = ha_SiteConfByEid(p_env, eid);

					snprintf(s_init_err_str, sizeof(s_init_err_str),
						"Assign me as group creator, but [%s:%d] already exist.",
						p_site_conf->pc_ip, p_site_conf->port);

					ha_ThrowError((errcode(ERRCODE_REPHA_GROUP_CREATOR_EXIST),
						errmsg("Init rep ha env failed, details: %s",
						s_init_err_str)));
					return false;
				}
				continue;
			}
		}
		ha_DebugEnv("Conn init successed.");

		/* listen must be the last */
		if(ha_Listen(ha_MySiteConf(p_env), HA_MAX_LISTEN, p_env->listen_socket)
		!= STATUS_OK){
			snprintf(s_init_err_str, sizeof(s_init_err_str),
				"listen on [%s:%d] failed.",
				p_my_site_conf->pc_ip, p_my_site_conf->port);
			ha_Error("Listen failed.");
			return false;
		}
	}

	ha_DebugEnv("Communication init successed.");

	return true;
}

bool ha_ElectionInit(ha_env_t *p_env, bool assign_master)
{
	struct timeval now;

	gettimeofday(&now, NULL);

	ha_EstateStringsInit();
	ha_EventStringsInit();

	ha_ElectionLogInit(p_env, assign_master);

	if(assign_master){
		p_env->election.estate = HA_ESTATE_SUCCESSED;

		p_env->event.recv_heartbeat = true;
		p_env->event.master_eid = ha_MyEid(p_env);
		ha_ComputeDeadline(p_env->event.next_bcast_timev, now, ha_HbTimeInterval(p_env));
		ha_ComputeDeadline(p_env->event.timev_hb_d, now, ha_TimeoutHb(p_env));
	}else{
		p_env->election.estate = HA_ESTATE_MAX;

		p_env->event.recv_heartbeat = false;
		p_env->event.master_eid = HA_EID_INVALID;
		ha_ComputeDeadline(p_env->event.timev_hb_d, now, ha_TimeoutHb(p_env));
	}

	p_env->event.need_manual_start = p_env->manual_start_election;
	p_env->event.got_start_cmd = false;

	p_env->event.election_delay = false;

	p_env->event.two_masters_found = false;

	return true;
}

bool ha_EnvInit(ha_env_t *p_env, const char *pc_ip,
	rep_ha_t *p_ha_conf, bool assign_master)
{
	ha_InitLock(p_env);

	if((ha_UconfInit(p_env, pc_ip, p_ha_conf))
	&& (ha_ElectionInit(p_env, assign_master))
	&& (ha_CommInit(p_env, assign_master))){
		ha_DumpUconf(p_env);
		return true;
	}

	return false;
}
/* *****************************************************************************
*						ha init stop
***************************************************************************** */












/* *****************************************************************************
* 						ha env thread function start
***************************************************************************** */
bool ha_ElectionThreadExist(void)
{
	ha_election_log_t *p_election_log = &g_ha_election_log;
	bool exist = false;

	ha_Lock(&p_election_log->mlock);
	exist = (p_election_log->p_election_thread != NULL);
	ha_Unlock(&p_election_log->mlock);

	return exist;
}

void ha_RecordElectionExit(ha_env_t *p_env)
{
	ha_election_log_t *p_election_log = &g_ha_election_log;

	ha_Lock(&p_election_log->mlock);
	ha_free(p_election_log->p_election_thread);
	p_election_log->p_election_thread = NULL;
	ha_Unlock(&p_election_log->mlock);
	return;
}

void ha_RecordElectionStart(void *p_arg)
{
	ha_election_log_t *p_election_log = &g_ha_election_log;

	ha_Lock(&p_election_log->mlock);
	p_election_log->p_election_thread = p_arg;
	ha_Unlock(&p_election_log->mlock);

	return;
}

void ha_ElectionMain(void)
{
	// TODO:
	ha_StartElection((void *)&g_ha_env);

	return;
}

void *ha_election_thread_func(void *params)
{
#ifndef HA_SIMULATE_ENV
	extern void* fxdb_SubPostmaster_Main(void *);
	//extern int ha_ElectionMain(void *);

	fxdb_SubPostmaster_Main(params);
	free(params);

	ha_ElectionMain();
	return NULL;
#else
	return NULL;
#endif
}

void fxdb_election_forkexec(void *p_arg)
{
	ha_env_t *p_env = (ha_env_t *)p_arg;

#ifndef HA_SIMULATE_ENV
	fxdb_thread_t *p_thread = NULL;

	extern fxdb_thread_t *fxdb_Thread_Create(void *(*)(void*), void *);
	extern void fxdb_save_thread_variables(BackendParameters *, BackendId, ThreadType);


	ha_Log("start election thread...");
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));

	param->MyThreadType = replha;
	p_thread = fxdb_Thread_Create(ha_election_thread_func, param);

	ha_RecordElectionStart((void *)p_thread);
#else
	pthread_t *p_thread = (pthread_t *)malloc(sizeof(pthread_t));

	memset((char *)p_thread, 0, sizeof(pthread_t));

	if(pthread_create(p_thread, NULL, ha_StartElection, (void *)p_env) != 0){
		ha_Error("Create election thread failed.");
		return;
	}

	ha_RecordElectionStart((void *)p_thread);
#endif

	return;
}




#ifndef HA_SIMULATE_ENV
extern fxdb_thread_t *repha_handle;

static void ha_StopSelectThread(SIGNAL_ARGS)
{
	ha_Log("Select thread will stop later.");
	select_thread_need_stop = true;
	return;
}

static void ha_SelectRegisterSignal(void)
{
	/* Set up signal handlers */
	pg_signal(SIGHUP, SIG_IGN);		
	pg_signal(SIGINT, SIG_IGN);
	pg_signal(SIGTERM, SIG_IGN);
	pg_signal(SIGQUIT, SIG_IGN);
	pg_signal(SIGALRM, SIG_IGN);
	pg_signal(SIGPIPE, SIG_IGN);
	pg_signal(SIGUSR1, SIG_IGN);
	pg_signal(SIGUSR2, ha_StopSelectThread); /* shutdown */
	pg_signal(SIGCHLD, SIG_DFL);
	pg_signal(SIGTTIN, SIG_DFL);
	pg_signal(SIGTTOU, SIG_DFL);
	pg_signal(SIGCONT, SIG_DFL);
	pg_signal(SIGWINCH, SIG_DFL);

	return;
}
#endif


void ha_SelectMain(void)
{
	//ha_EnvInit(&g_ha_env, argc, argv);
	ha_Log("ha_SelectMain start");

#ifndef HA_SIMULATE_ENV
	ha_SelectRegisterSignal();
#endif

	ha_SelectLoop((void *)&g_ha_env);

	return;
}

void *ha_select_thread_func(void *params)
{
#ifndef HA_SIMULATE_ENV

	extern void* fxdb_SubPostmaster_Main(void *);

	ha_Log("ha_select_thread_func start.");

	fxdb_SubPostmaster_Main(params);
	free(params);

	ha_SelectMain();
	return NULL;
#else
	return NULL;
#endif
}

void fxdb_select_forkexec(void)
{
#ifndef HA_SIMULATE_ENV
	extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);
	extern void fxdb_save_thread_variables(BackendParameters *, BackendId, ThreadType);


	ha_Log("start select thread...");
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));

	param->MyThreadType = replha;
	repha_handle = fxdb_Thread_Create(ha_select_thread_func, param);
	return;
#else
	return;
#endif
}

static void ha_CleanupMain()
{
#ifndef HA_SIMULATE_ENV
	char filepath[1024];
	char backup_path[1024];
	FILE* pFile;

	const char* path = getenv("XMLDB_PATH");
	if (path == NULL)
	{
		return;
	}

	snprintf(filepath, 1024, "%s/backup_directory", path);	
	pFile = fopen(filepath, "r");
	if (pFile == NULL)
	{
		return;
	}

	while (!WalRcvHadCatchupPrimary())
	{
		pg_sleep(1000000);
	}

	while ((fgets(backup_path, 1024, pFile)) != NULL) 
	{
		size_t len = strlen(backup_path);
		if (backup_path[len - 1] == '\n')
		{
			backup_path[len - 1] = '\0';
		}		
		ha_Info2("Cleanup backup datafile: %s", backup_path);
		if (!rmtree(backup_path, true))
		{
			ha_Info2("some useless files may be left behind in old data directory \"%s\"", backup_path);
		}
	}

	fclose(pFile);

	ha_Info2("Remove backup directory file: %s", filepath);
	unlink(filepath);
	return;
#else
	return;
#endif
}

void *ha_cleanup_thread_func(void *params)
{
#ifndef HA_SIMULATE_ENV

	extern void* fxdb_SubPostmaster_Main(void *);

	ha_Log("ha_cleanup_thread_func start.");

	fxdb_SubPostmaster_Main(params);
	free(params);
	
	ha_CleanupMain();		

	ha_Log("ha_cleanup_thread_func exit.");

	return NULL;
#else
	return NULL;
#endif
}

void fxdb_cleanup_forkexec(void)
{
#ifndef HA_SIMULATE_ENV
	fxdb_thread_t *p_thread = NULL;
	
	extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);
	extern void fxdb_save_thread_variables(BackendParameters *, BackendId, ThreadType);

	ha_Log("start ha cleanup thread...");
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));

	param->MyThreadType = replha;
	p_thread = fxdb_Thread_Create(ha_cleanup_thread_func, param);
	return;
#else
	return;
#endif
}

void fxdb_start_ha(rep_ha_t *p_ha_conf, bool assign_master)
{
//#ifndef HA_SIMULATE_ENV
#if 0
	g_ha_log_file = fopen("ha.log", "w");
	if(g_ha_log_file == NULL){
		ha_Error("open rep ha log failed.");
		ha_ThrowError((errcode(ERRCODE_REPHA_ERROR),
                    errmsg("open rep ha log file failed")));
		return;
	}
#endif
	ha_Info2("HA is starting.");

	if(!ha_EnvInit(&g_ha_env, NULL, p_ha_conf, assign_master)){
		ha_Error("Init rep ha env failed, details: %s", s_init_err_str);
		ha_ThrowError((errcode(ERRCODE_REPHA_ERROR),
                    errmsg("Init rep ha env failed, details: %s",
                    		s_init_err_str)));
		return;
	}

	fxdb_select_forkexec();

	// basebackup完成后以备机方式启动时需要清理备份文件
	if (!assign_master)
	{
		fxdb_cleanup_forkexec();
	}	

	return;
}
/* *****************************************************************************
*						ha env thread function end
***************************************************************************** */

