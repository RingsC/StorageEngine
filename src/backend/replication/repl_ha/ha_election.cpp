/*
 * ha_election.cpp
 */
#include "ha_head.h"









/* *****************************************************************************
* 						ha election debug funcs start
***************************************************************************** */
typedef struct ha_election_debug{
	const char *p_estate_strings[HA_ESTATE_MAX];
	const char *p_event_strings[HA_EVENT_MAX];
}ha_election_debug_t;

static ha_election_debug_t s_ha_election_debug;
ha_election_log_t g_ha_election_log;


/*
 * Description:
 * 	translate estate to string
 * 
 */
const char *ha_EstateString(ha_estate estate)
{
	return s_ha_election_debug.p_estate_strings[estate];
}



void ha_EstateStringsInit(void)
{
	for(unsigned estate = 0; estate < HA_ESTATE_MAX; estate++){
		const char **pp_estate_strings = &s_ha_election_debug.p_estate_strings[estate];

		switch(estate){
			case HA_ESTATE_SUCCESSED:
				*pp_estate_strings = "SUCCESSED";
				break;
			case HA_ESTATE_P1_START:
				*pp_estate_strings = "P1_START";
				break;
			case HA_ESTATE_P1_TIMEOUT:
				*pp_estate_strings = "P1_TIMEOUT";
				break;
			case HA_ESTATE_P2_START:
				*pp_estate_strings = "P2_START";
				break;
			case HA_ESTATE_P2_TIMEOUT:
				*pp_estate_strings = "P2_TIMEOUT";
				break;
			case HA_EVENT_MAX:
				*pp_estate_strings = "Unknown(max)";
				break;
			default:
				ha_Assert(false);
				break;
		}
	}

	return;
}

/*
 * Description:
 * 	translate event to string
 * 
 */
const char *ha_EventString(ha_event_type_e event)
{
	return s_ha_election_debug.p_event_strings[event];
}

void ha_EventStringsInit(void)
{
	for(unsigned event = 0; event < HA_EVENT_MAX; event++){
		const char **pp_event_strings = &s_ha_election_debug.p_event_strings[event];

		switch(event){
			case HA_EVENT_NONE:
				*pp_event_strings = "NONE";
				break;
			case HA_EVENT_GOT_ALL_LEIS:
				*pp_event_strings = "GotAllLeis";
				break;
			case HA_EVENT_GOT_MIN_VOTES:
				*pp_event_strings = "GotMinVotes";
				break;
			case HA_EVENT_GOT_HEARTBEAT:
				*pp_event_strings = "Heartbeat";
				break;
			case HA_EVENT_TIMER1_TIMEOUT:
				*pp_event_strings = "Timer1Timeout";
				break;
			case HA_EVENT_TIMER2_TIMEOUT:
				*pp_event_strings = "Timer2Timeout";
				break;
			case HA_EVENT_TIMER4_TIMEOUT:
				*pp_event_strings = "Timer4Timeout";
				break;
			case HA_EVENT_MAX:
				*pp_event_strings = "Unknown(max)";
				break;
			default:
				ha_Assert(false);
				break;
		}
	}

	return;
}

void ha_RestartLog4SimulateEnv(ha_env_t *p_env)
{
#ifdef HA_SIMULATE_ENV
	#ifdef WIN32
	if((_chsize( _fileno(stdout), 0 ) != 0/* new file length must be 0 */)
	|| (_lseek(_fileno(stdout), 0, SEEK_SET) != 0/* new offset must be 0 */)){
		ha_Error("Restart log for SimulateEnv failed.");
		return;
	}
	#else
	if((ftruncate(fileno(stdout), 0) != 0)
	|| (lseek(fileno(stdout), 0, SEEK_SET) != 0/* new offset must be 0 */)){
		ha_Error("Restart log for SimulateEnv failed.");
		return;
	}
	#endif
#endif

	return;
}

void ha_Log4SimulateEnv(ha_env_t *p_env, const char *p_string)
{
#ifdef HA_SIMULATE_ENV
	char __one_line[64];

	snprintf(__one_line, 64, "#####%s\n", p_string);
	ha_Write(stdout, __one_line, strlen(__one_line));
#endif

	return;
}

void ha_SaveElectionResult(ha_env_t *p_env, uint32 master_eid,
	const char *pc_ip, uint16 port)
{
	ha_election_log_t *p_result = &g_ha_election_log;

	ha_Lock(&p_result->mlock);
	p_result->master_port = port;
	p_result->i_am_master = (master_eid == ha_MyEid(p_env));
	p_result->first_election_done = true;
	snprintf(p_result->c_master_ip, HA_MAX_IP_LEN, "%s", pc_ip);
	ha_Unlock(&p_result->mlock);

	return;
}

void ha_ClearElectionResult(ha_env_t *p_env)
{
	ha_election_log_t *p_result = &g_ha_election_log;

	ha_Lock(&p_result->mlock);
	p_result->master_port = 0;
	p_result->i_am_master = false;
	p_result->first_election_done = false;
	memset(p_result->c_master_ip, 0, HA_MAX_IP_LEN);
	ha_Unlock(&p_result->mlock);

	return;
}

bool ha_FirstElectionDone(void)
{
	bool done;

	ha_Lock(&g_ha_election_log.mlock);
	done = g_ha_election_log.first_election_done;
	ha_Unlock(&g_ha_election_log.mlock);
	
	return done;
}

/*
 * Description:
 * 	Get election result.
 *	If return true, then current election successed, and all the output params are valid.
 * 	If return false, then current not successfully electied.
 *
 * output params:
 * 	p_i_am_master -- return true, if i am master;
 *				retur false, if i am not master.
 * 	pc_master_ip -- return master ip(will malloc(*not palloc) memory,
 				so caller should free it by free(*not pfree)).
 *	p_master_port -- return master port
 *
 */
bool ha_GetElectionResult(bool *p_i_am_master, char **pc_master_ip,
	uint16 *p_master_port)
{
	ha_election_log_t *p_result = &g_ha_election_log;

	ha_Lock(&p_result->mlock);
	if(!p_result->first_election_done){
		ha_Unlock(&p_result->mlock);
		return false;
	}else{
		uint32 ip_len = (uint32)strlen(p_result->c_master_ip) + 1;
		char *pc_ip = (char *)malloc(ip_len);
		if(pc_ip == NULL){
			ha_Unlock(&p_result->mlock);
			ha_EreportOOM();
			return false;
		}
		memcpy(pc_ip, p_result->c_master_ip, ip_len);
		*pc_master_ip = pc_ip;
		*p_master_port = p_result->master_port;
		*p_i_am_master = p_result->i_am_master;
		ha_Unlock(&p_result->mlock);
	}

	ha_DebugElection("i_am_master: %s, master port: %d, master ip: %s",
		*p_i_am_master ? "true" : "false", *p_master_port, *pc_master_ip);

	return true;
}

bool ha_ElectionLogInit(ha_env_t *p_env, bool assign_master)
{
	ha_election_log_t *p_election_log = &g_ha_election_log;

	pthread_mutex_init(&(p_election_log->mlock), NULL);

	p_election_log->p_election_thread = NULL;

	if(assign_master){
		ha_site_conf_t *p_my_site_conf = ha_MySiteConf(p_env);

		ha_SaveElectionResult(p_env, ha_MyEid(p_env),
			p_my_site_conf->pc_ip, p_my_site_conf->port);
	}else{
		p_election_log->first_election_done = false;
	}

	return true;
}
/* *****************************************************************************
* 						ha election debug funcs end
***************************************************************************** */















/* *****************************************************************************
* 						ha leis ops start
***************************************************************************** */
/*
 * Description:
 *	Lei compare function
 * 
 */
int ha_LsnCompare(const ha_lsn_t *p_lsn1, const ha_lsn_t *p_lsn2)
{
	//ha_DebugElection("lsn1: [id:%d, offset:%d]", p_lsn1->xlogid, p_lsn1->xrecoff);
	//ha_DebugElection("lsn2: [id:%d, offset:%d]", p_lsn2->xlogid, p_lsn2->xrecoff);

	if(XLByteEQ(*p_lsn1, *p_lsn2)){
		return 0;
	}else if(XLByteLT(*p_lsn1, *p_lsn2)){
		return -1;
	}

	return 1;
}

int ha_TiebreakerCompare(const char *pc_tb1, const char *pc_tb2)
{
	//ha_DebugElection("tiebreaker1: [%s]", pc_tb1);
	//ha_DebugElection("tiebreaker2: [%s]", pc_tb2);

	return strcmp(pc_tb1, pc_tb2);
}

static bool ha_LeiEqual(const ha_lei_t *p_lei1, const ha_lei_t *p_lei2)
{
	if((p_lei1->eid == p_lei2->eid) && (p_lei1->priority == p_lei2->priority)
	&& (ha_LsnCompare(&p_lei1->my_lsn, &p_lei2->my_lsn) == 0)
	&& (ha_TiebreakerCompare(p_lei1->c_tiebreaker, p_lei2->c_tiebreaker) == 0)){
		return true;
	}
	return false;
}

/* p_lei1 win, return true; else return false. */
static bool ha_LeiWin(const ha_lei_t *p_lei1, const ha_lei_t *p_lei2)
{
	int ret = 0;

	//ha_DebugElection("lei1: priority[%d]", p_lei1->priority);
	//ha_DebugElection("lei2: priority[%d]", p_lei2->priority);

	if(((p_lei1->priority == 0) && (p_lei2->priority != 0))
	|| ((p_lei1->priority != 0) && (p_lei2->priority == 0))){
		ha_DebugElection("We don't want to let a 0-priority site win");
		return (p_lei1->priority > p_lei2->priority);
	}

	/* first compare lsn */
	ret = ha_LsnCompare(&p_lei1->my_lsn, &p_lei2->my_lsn);
	if(ret < 0){
		return false;
	}else if(ret > 0){
		return true;
	}

	/* lsn equal, now we compare priority */
	if(p_lei1->priority > p_lei2->priority){
		return true;
	}else if(p_lei1->priority < p_lei2->priority){
		return false;
	}

	/* lsn & priority all equal, we compare tiebreaker */
	ret = ha_TiebreakerCompare(p_lei1->c_tiebreaker, p_lei2->c_tiebreaker);
	if(ret < 0){
		return false;
	}else if(ret > 0){
		return true;
	}

	return false;
}

/*
 * Description:
 *	process with recved lei
 * 
 * ha_SaveNewLei, ha_FreeSavedLeis, ha_UpdateWinLei
 * The above three fuctions are this file scope functions.
 * And the caller must be in the protection of event lock.
 *
 */
static void ha_FreeSavedLeis(ha_env_event_t *p_event)
{
	ha_lei_t *p_lei = p_event->p_leis;

	while(p_lei != NULL){
		ha_lei_t *p_next_lei = p_lei->p_next;

		ha_free(p_lei);

		p_lei = p_next_lei;
	}

	p_event->p_leis = NULL;

	return;
}

static ha_lei_t *ha_FindSavedLeiByEid(ha_lei_t *p_leis, uint32 eid)
{
	ha_lei_t *p_found_lei = NULL;
	ha_lei_t *p_tmp_lei = p_leis;

	while(p_tmp_lei != NULL){
		if(eid == p_tmp_lei->eid){
			p_found_lei = p_tmp_lei;
			break;
		}

		p_tmp_lei = p_tmp_lei->p_next;
	}

	return p_found_lei;
}

static void ha_EnqueueLei(ha_env_event_t *p_event, ha_lei_t *p_new_lei)
{
	p_new_lei->p_next = p_event->p_leis;
	p_event->p_leis = p_new_lei;

	return;
}

static bool ha_DequeueLei(ha_env_event_t *p_event, const ha_lei_t *p_given_lei)
{
	if(p_event->p_leis == NULL){
		ha_Assert(false);
		return false;
	}

	if(p_given_lei == p_event->p_leis){
		p_event->p_leis = p_given_lei->p_next;
		return true;
	}else{
		ha_lei_t *p_lei = p_event->p_leis;

		while(p_lei->p_next != NULL){
			if(p_lei->p_next == p_given_lei){
				p_lei->p_next = p_given_lei->p_next;
				return true;
			}
			p_lei = p_lei->p_next;
		}
	}

	return false;
}

static void ha_SaveNewLei(ha_env_event_t *p_event, const ha_lei_t *p_new_lei)
{
	ha_lei_t *p_found_lei = ha_FindSavedLeiByEid(p_event->p_leis, p_new_lei->eid);
	bool save_lei = false;

	if(p_found_lei == NULL){
		save_lei = true;

		/* new leis, so nleis++ */
		p_event->nleis++;
	}else{
		if(!ha_LeiEqual(p_new_lei, p_found_lei)){
			ha_Warning("One election round, but got different "
				"leis from eid:%d", p_new_lei->eid);

			if(ha_LeiWin(p_new_lei, p_found_lei)){
				if(!ha_DequeueLei(p_event, p_found_lei)){
					ha_Assert(false);
					ha_Error("Dequeue found lei failed");
				}

				ha_free(p_found_lei);
				save_lei = true;
			}else{
				ha_Assert(false);
				ha_Error("Got a lei from eid:%d again, and"
					" old lei win, it's weird", p_new_lei->eid);
			}
		}
	}

	if(save_lei){
		ha_lei_t *p_lei_copy = (ha_lei_t *)ha_malloc(sizeof(ha_lei_t));

		if(p_lei_copy == NULL){
			ha_EreportOOM();
			return;
		}

		memcpy((char *)p_lei_copy, (const char *)p_new_lei, sizeof(ha_lei_t));

		ha_EnqueueLei(p_event, p_lei_copy);
	}

	return;
}

/*
 * Description:
 *	call these functions when recved msg
 * 
 */
static void ha_UpdateWinLei(ha_env_event_t *p_event,
	const ha_lei_t *p_new_lei)
{
	memcpy((char *)&p_event->win_lei, (const char *)p_new_lei, sizeof(ha_lei_t));
	return;
}
/* *****************************************************************************
*						ha leis ops end
***************************************************************************** */














/* *****************************************************************************
* 						ha votes ops start
***************************************************************************** */
/* *****************************************************************************
*						ha votes ops start
***************************************************************************** */

















/* *****************************************************************************
* 						ha recv msg do funcs start
***************************************************************************** */
void ha_BcastTwoMasters(ha_env_t *p_env);

void ha_GotMsgLeiDo(ha_env_t *p_env, ha_lei_t *p_new_lei)
{
	uint32 win_eid_before;
	uint32 win_eid_after;
	uint32 nleis_before;
	uint32 nleis_after;

	ha_Lock(ha_Lock4Event(p_env));

	/* save before info */
	nleis_before = p_env->event.nleis;
	win_eid_before = ((p_env->event.nleis > 0)
		? (p_env->event.win_lei.eid) : HA_EID_INVALID);

	/* first save new lei */
	ha_SaveNewLei(&p_env->event, p_new_lei);

	/* update nleis */
	//p_env->event.nleis++;

	/* update winner lei */
	if((p_env->event.nleis == 1)
	|| (ha_LeiWin(p_new_lei, &p_env->event.win_lei))){
		ha_UpdateWinLei(&p_env->event, p_new_lei);
	}

	/* save after info */
	nleis_after = p_env->event.nleis;
	win_eid_after = p_env->event.win_lei.eid;

	ha_Unlock(ha_Lock4Event(p_env));

	ha_DebugElection("*****Got lei: [eid=%d|prio=%d|tb=%s|lid=%u|loff=%u], "
		"B[N:%d|W:%d], A[N:%d|W:%d]",
		p_new_lei->eid, p_new_lei->priority, p_new_lei->c_tiebreaker,
		p_new_lei->my_lsn.xlogid, p_new_lei->my_lsn.xrecoff,
		nleis_before, win_eid_before, nleis_after, win_eid_after);

	return;
}

void ha_GotMsgVoteDo(ha_env_t *p_env, uint32 from_eid)
{
	uint32 nvotes_before;
	uint32 nvotes_after;

	ha_Lock(ha_Lock4Event(p_env));

	/* save before info */
	nvotes_before = p_env->event.nvotes;

	/* update nvotes */
	p_env->event.nvotes++;

	/* save after info */
	nvotes_after = p_env->event.nvotes;

	ha_Unlock(ha_Lock4Event(p_env));

	ha_DebugElection("*****Got vote: [eid:%d], B[N:%d], A[N:%d]",
		from_eid, nvotes_before, nvotes_after);

	return;
}

void ha_DelayElection(ha_env_t *p_env, struct timeval now)
{
	uint32 delay_s = ha_ElectionDelaySecond(p_env);

	if(delay_s == 0){
		ha_Log("Election delayed until manual start!!!!!!!!");

		p_env->event.need_manual_start = true;
		p_env->event.got_start_cmd = false;
	}else{
		p_env->event.election_delay = true;
		if(now.tv_usec < 500000){
			p_env->event.delay_to_s = now.tv_sec + delay_s;
		}else{
			p_env->event.delay_to_s = now.tv_sec + delay_s + 1;
		}

		ha_Log("Election delayed %ds until [%lds]!!!!!!!!",
			delay_s, p_env->event.delay_to_s);
	}

	return;
}

void ha_ClearDelayInfo(ha_env_t *p_env)
{
	p_env->event.need_manual_start = false;
	p_env->event.election_delay = false;

	return;
}

void ha_GotMsgBcastTwoMastersDo(ha_env_t *p_env)
{
	ha_Lock(ha_Lock4Event(p_env));

	if(p_env->event.two_masters_found){
		ha_Unlock(ha_Lock4Event(p_env));
	}else{
		ha_Info2("*****Got two masters");		

		p_env->event.two_masters_found = true;

		if(p_env->event.master_eid == ha_MyEid(p_env)){
			ha_Unlock(ha_Lock4Event(p_env));

			ha_BcastTwoMasters(p_env);
			ha_KillTwoMasters();
		}else{
			struct timeval now;
			gettimeofday(&now, NULL);

			ha_DelayElection(p_env, now);
			ha_Unlock(ha_Lock4Event(p_env));

			ha_ClearElectionResult(p_env);
			ha_BcastTwoMasters(p_env);
		}	
	}

	return;
}


void ha_GotMsgBcastNewMasterDo(ha_env_t *p_env, uint32 master_eid,
	char *pc_ip, uint32 port)
{
	uint32 master_before;
	uint32 master_after;

	struct timeval now;

	gettimeofday(&now, NULL);

	ha_Lock(ha_Lock4Event(p_env));

	if(p_env->event.two_masters_found){
		ha_Unlock(ha_Lock4Event(p_env));

		ha_Error("Two masters found, ignore all new master msg.");
		return;
	}

	/* save before info */
	master_before = ((ha_GotHeartBeat(&p_env->event))
		? (p_env->event.master_eid) : (HA_EID_INVALID));

	/* if already got heartbeat */
	if(ha_GotHeartBeat(&p_env->event) && (master_eid != master_before)){
		if(master_eid == ha_MyEid(p_env)){
			ha_Warning("Two masters, weird case[master_after is mine]");
			p_env->event.master_eid = ha_MyEid(p_env);
		}
		ha_Unlock(ha_Lock4Event(p_env));

		ha_Error("Two masters[eid:%d & eid:%d]!!!!!", master_eid, master_before);

		/* do two masters */
		ha_GotMsgBcastTwoMastersDo(p_env);
		return;
	}

	p_env->event.recv_heartbeat = true;
	ha_ComputeDeadline(p_env->event.timev_hb_d, now, ha_TimeoutHb(p_env));
	p_env->event.master_eid = master_eid;
	if(master_eid == ha_MyEid(p_env)){
		ha_ComputeDeadline(p_env->event.next_bcast_timev, now,
			ha_HbTimeInterval(p_env));
	}

	/* clear invalid all leis & votes */
	ha_ClearAllLeis(&p_env->event);
	ha_ClearAllVotes(&p_env->event);

	/* save after info */
	master_after = p_env->event.master_eid;

	ha_Unlock(ha_Lock4Event(p_env));

	if(master_after != master_before){
		ha_Log("Got new master message from [%s:%d]: [eid:%d], B[M:%d], A[M:%d] [my:%d]",
				pc_ip, port, master_eid, master_before, master_after, ha_MyEid(p_env));
	}else{
		ha_DebugElection("Got new master message from [%s:%d]: [eid:%d], B[M:%d], A[M:%d] [my:%d]",
				pc_ip, port, master_eid, master_before, master_after, ha_MyEid(p_env));
	}

	if((master_before == HA_EID_INVALID) && (!ha_ElectionThreadExist())){
		ha_Log4SimulateEnv(p_env, "HEARTBEAT");
	}

	if(master_before != master_after){
		ha_SaveElectionResult(p_env, master_eid, pc_ip, port);
	}

	return;
}
/* *****************************************************************************
* 						ha recv msg do funcs end
***************************************************************************** */















/* *****************************************************************************
* 						ha send msg funcs start
***************************************************************************** */
/*
 * Description:
 *	Lei generation function
 * 
 */
static void ha_GenTiebreaker(ha_site_conf_t *p_my_site, char *p_tiebreaker,
	uint32 tb_buf_len)
{
	/* now use ip+port as tiebreaker */
	snprintf(p_tiebreaker, tb_buf_len, "%s:%d", p_my_site->pc_ip,
		p_my_site->port);

	return;
}

static void ha_GetCurrentLsn(ha_lsn_t *p_lsn)
{
	ha_lsn_t lsn = GetWalRcvWriteRecPtr(NULL, NULL);

	p_lsn->xlogid = lsn.xlogid;
	p_lsn->xrecoff = lsn.xrecoff;

	return;
}

static void ha_GetMyLei(ha_env_t *p_env, ha_lei_t *p_my_lei)
{
	p_my_lei->eid = ha_MyEid(p_env);
	p_my_lei->p_next = NULL;
	p_my_lei->priority = ha_MyPriority(p_env);
	ha_GetCurrentLsn(&p_my_lei->my_lsn);
	ha_GenTiebreaker(ha_MySiteConf(p_env), p_my_lei->c_tiebreaker,
		HA_TIEBREAKER_MAX_LEN);

	return;
}

/*
 * Description:
 *	send msg function
 * 
 * The caller must not in any lock with in ha_env_t.
 */
void ha_BcastMyLei(ha_env_t *p_env)
{
	if(!ha_IsTestSm(p_env)){
		ha_lei_t my_lei;
		char *p_msg = NULL;

		ha_GetMyLei(p_env, &my_lei);

		p_msg = ha_PackMsgLei(my_lei.priority, &my_lei.my_lsn, my_lei.c_tiebreaker);

		/* first set me got lei */
		ha_GotMsgLeiDo(p_env, &my_lei);

		/* bcast msg */
		ha_BcastMsg(p_env, p_msg);

		ha_free(p_msg);
	}

	return;
}

void ha_CastVote(ha_env_t *p_env)
{
	if(!ha_IsTestSm(p_env)){
		uint32 win_eid = 0;

		ha_Lock(ha_Lock4Event(p_env));
		win_eid = p_env->event.win_lei.eid;
		ha_Unlock(ha_Lock4Event(p_env));

		if(win_eid != ha_MyEid(p_env)){
			char *p_msg = ha_PackMsgVote();

			ha_SendMsgByEid(p_env, win_eid, p_msg);

			ha_free(p_msg);
		}else{
			ha_GotMsgVoteDo(p_env, ha_MyEid(p_env));
		}
	}

	return;
}

void ha_BcastNewMaster(ha_env_t *p_env)
{
	if(!ha_IsTestSm(p_env)){
		ha_site_conf_t *p_my_site_conf = ha_MySiteConf(p_env);
		char *pc_ip = p_my_site_conf->pc_ip;
		uint16 port = GetPostPort();
		char *p_msg = ha_PackMsgNewMaster(pc_ip, port);

		/* first set me recved bcast msg new mater*/
		ha_GotMsgBcastNewMasterDo(p_env, ha_MyEid(p_env), pc_ip, port);

		/* bcast msg */
		ha_BcastMsg(p_env, p_msg);

		ha_free(p_msg);
	}

	return;
}

void ha_BcastTwoMasters(ha_env_t *p_env)
{
	if(!ha_IsTestSm(p_env)){
		char *p_msg = ha_PackMsgTwoMasters();

		/* just bcast msg to others */
		ha_BcastMsg(p_env, p_msg);

		ha_free(p_msg);
	}

	return;
}
/* *****************************************************************************
*						ha send msg funcs end
***************************************************************************** */
















/* *****************************************************************************
*						ha election main start
***************************************************************************** */
static bool s_master_down_2sites = false;
extern void ha_ElectionSuccessedMaster(ha_env_t *p_env);

void ha_ElectionP1Start(ha_env_t *p_env, bool first_in_thread)
{
	static uint32 p1s_times = 1;
	const uint32 clear_times = ((p_env->nsites > 9) ? (p_env->nsites + 1) : 10);

	if(first_in_thread){
		p1s_times = 1;
		ha_RestartLog4SimulateEnv(p_env);
		ha_Log4SimulateEnv(p_env, "START");
	}

	/* just for 2 sites */
	if((s_master_down_2sites) && (p1s_times > p_env->max_etimes_2sites)){
		if (WalRcvHadCatchupPrimary())
		{
			ha_Assert(p_env->nsites == 2);

			ha_Info2("====After %d times, election failed(Nsites==2, "
				"master down). We just set slave as master.", (p1s_times - 1));

			s_master_down_2sites = false;
			ha_ElectionSuccessedMaster(p_env);
			return;
		}
		else
		{
			ha_Info2("Slave server does not catch up with the master server, can not be switched to master now.");
		}
	}

	ha_DebugElection("======P1 start times:%d=======", p1s_times++);

	ha_SetEstate(&p_env->election, HA_ESTATE_P1_START);

	/* start timer1 & timer4 */
	ha_SetTimer1And4(&p_env->election, ha_Timeout1(p_env),
		ha_Timeout4(p_env));

	ha_Lock(ha_Lock4Event(p_env));

	/* check heartbeat & master_eid */
	if((p_env->event.recv_heartbeat)
	|| (p_env->event.master_eid != HA_EID_INVALID)){
		ha_Warning("Wrong info when Set p1 start, heartbeat:%s, eid:%d",
				(p_env->event.recv_heartbeat) ? "true" : "false",
				p_env->event.master_eid);
	}

	/* if p1s_times > clear_times, clear all leis */
	if(p1s_times % clear_times == 0){
		ha_ClearAllLeis(&p_env->event);
	}

	ha_Unlock(ha_Lock4Event(p_env));

	/* send my local election info */
	ha_BcastMyLei(p_env);

	return;
}

void ha_ElectionP1Timeout(ha_env_t *p_env)
{
	ha_SetEstate(&p_env->election, HA_ESTATE_P1_TIMEOUT);
	ha_SetTimer2(&p_env->election, ha_Timeout2(p_env));

	return;
}

void ha_ElectionP2Start(ha_env_t *p_env)
{
	ha_SetEstate(&p_env->election, HA_ESTATE_P2_START);
	ha_SetTimer2(&p_env->election, ha_Timeout2(p_env));
	ha_CastVote(p_env);

	return;
}

void ha_ElectionP2Timeout(ha_env_t *p_env)
{
	ha_SetEstate(&p_env->election, HA_ESTATE_P2_TIMEOUT);

	ha_Lock(ha_Lock4Event(p_env));

	//ha_ClearAllLeis(&p_env->event);
	ha_ClearAllVotes(&p_env->event);

	ha_Unlock(ha_Lock4Event(p_env));

	return;
}

void ha_ElectionSuccessed(ha_env_t *p_env)
{
	ha_SetEstate(&p_env->election, HA_ESTATE_SUCCESSED);

	ha_Log4SimulateEnv(p_env, "SUCCESSED");

	ha_ClearAllTimer(&p_env->election);

	ha_Lock(ha_Lock4Event(p_env));

	ha_ClearAllLeis(&p_env->event);
	ha_ClearAllVotes(&p_env->event);
	//ha_ClearHeartbeat(&p_env->event);

	ha_Unlock(ha_Lock4Event(p_env));

	return;
}

void ha_ElectionSuccessedMaster(ha_env_t *p_env)
{
	ha_Log4SimulateEnv(p_env, "MASTER");

	ha_BcastNewMaster(p_env);
	ha_ElectionSuccessed(p_env);

	return;
}

bool ha_GuardIWin(ha_env_t *p_env)
{
	bool i_win;

	ha_Lock(ha_Lock4Event(p_env));

	ha_DebugElection("My eid: %d, winner eid: %d",
		ha_MyEid(p_env), p_env->event.win_lei.eid);

	i_win = (p_env->event.win_lei.eid == ha_MyEid(p_env));

	ha_Unlock(ha_Lock4Event(p_env));

	return i_win;
}

bool ha_GuardGotMinLeis(ha_env_t *p_env)
{
	bool got_min_leis;

	ha_Lock(ha_Lock4Event(p_env));
	
	got_min_leis = (p_env->event.nleis >= ha_MinLeis(p_env));

	ha_Unlock(ha_Lock4Event(p_env));

	return got_min_leis;
}

int ha_ProcessP1StartEvent(ha_env_t *p_env, ha_event_type_e event)
{
	switch(event){
		case HA_EVENT_GOT_ALL_LEIS:
			/* clear timer1 */
			ha_ClearTimer1(&p_env->election);

			/* check guard */
			if(ha_GuardIWin(p_env)){ /* i win */
				ha_ElectionSuccessedMaster(p_env);
			}else{
				ha_ElectionP2Start(p_env);
			}
			break;
		case HA_EVENT_TIMER1_TIMEOUT:
			/* check guard */
			if(ha_GuardGotMinLeis(p_env)){ /* got >= min_leis */
				ha_ElectionP2Start(p_env);
			}else{
				ha_ElectionP1Timeout(p_env);
			}
			break;

		/* election successed event */
		case HA_EVENT_GOT_HEARTBEAT:
			ha_ElectionSuccessed(p_env);
			break;
		case HA_EVENT_GOT_MIN_VOTES:
			ha_ElectionSuccessedMaster(p_env);
			break;
		case HA_EVENT_TIMER4_TIMEOUT:
			ha_Warning("Timer is not correct while debugging program, restarting election.");
			ha_ClearAllTimer(&p_env->election);
			ha_Lock(ha_Lock4Event(p_env));
			ha_ClearAllVotes(&p_env->event);
			ha_Unlock(ha_Lock4Event(p_env));
			ha_ElectionP1Start(p_env, false);
			break;
		default:
			ha_Assert(false);
			break;
	}

	return 0;
}

int ha_ProcessP1TimeoutEvent(ha_env_t *p_env, ha_event_type_e event)
{
	switch(event){
		case HA_EVENT_TIMER2_TIMEOUT:
			ha_ElectionP2Timeout(p_env);
			break;

		/* election successed event */
		case HA_EVENT_GOT_HEARTBEAT:
			ha_ElectionSuccessed(p_env);
			break;
		case HA_EVENT_GOT_ALL_LEIS:
			if(ha_GuardIWin(p_env)){ /* i win */
				ha_ElectionSuccessedMaster(p_env);
			}
			break;
		case HA_EVENT_GOT_MIN_VOTES:
			ha_ElectionSuccessedMaster(p_env);
			break;
		case HA_EVENT_TIMER4_TIMEOUT:
			ha_Warning("Timer is not correct while debugging program, restarting election.");
			ha_ClearAllTimer(&p_env->election);
			ha_Lock(ha_Lock4Event(p_env));
			ha_ClearAllVotes(&p_env->event);
			ha_Unlock(ha_Lock4Event(p_env));
			ha_ElectionP1Start(p_env, false);
			break;
		default:
			ha_Assert(false);
			break;
	}

	return 0;
}

int ha_ProcessP2StartEvent(ha_env_t *p_env, ha_event_type_e event)
{
	switch(event){
		case HA_EVENT_GOT_MIN_VOTES:
			ha_ElectionSuccessedMaster(p_env);
			break;
		case HA_EVENT_TIMER2_TIMEOUT:
			ha_ElectionP2Timeout(p_env);
			break;

		/* election successed event */
		case HA_EVENT_GOT_HEARTBEAT:
			ha_ElectionSuccessed(p_env);
			break;
		case HA_EVENT_GOT_ALL_LEIS:
			if(ha_GuardIWin(p_env)){ /* i win */
				ha_ElectionSuccessedMaster(p_env);
			}
			break;
		case HA_EVENT_TIMER4_TIMEOUT:
			ha_Warning("Timer is not correct while debugging program, restarting election.");
			ha_ClearAllTimer(&p_env->election);
			ha_Lock(ha_Lock4Event(p_env));
			ha_ClearAllVotes(&p_env->event);
			ha_Unlock(ha_Lock4Event(p_env));
			ha_ElectionP1Start(p_env, false);
			break;
		default:
			ha_Assert(false);
			break;
	}

	return 0;
}

int ha_ProcessP2TimeoutEvent(ha_env_t *p_env, ha_event_type_e event)
{
	switch(event){
		case HA_EVENT_TIMER4_TIMEOUT:
			ha_ElectionP1Start(p_env, false);
			break;

		/* election successed event */
		case HA_EVENT_GOT_HEARTBEAT:
			ha_ElectionSuccessed(p_env);
			break;
		case HA_EVENT_GOT_ALL_LEIS:
			if(ha_GuardIWin(p_env)){ /* i win */
				ha_ElectionSuccessedMaster(p_env);
			}
			break;
		case HA_EVENT_GOT_MIN_VOTES:
			ha_ElectionSuccessedMaster(p_env);
			break;
		default:
			ha_Assert(false);
			break;
	}

	return 0;
}

int ha_ProcessEvent(ha_env_t *p_env, ha_event_type_e event)
{
	int ret = STATUS_OK;

	switch(ha_Estate(&p_env->election)){
		case HA_ESTATE_P1_START:
			ret = ha_ProcessP1StartEvent(p_env, event);
			break;
		case HA_ESTATE_P1_TIMEOUT:
			ret = ha_ProcessP1TimeoutEvent(p_env, event);
			break;
		case HA_ESTATE_P2_START:
			ret = ha_ProcessP2StartEvent(p_env, event);
			break;
		case HA_ESTATE_P2_TIMEOUT:
			ret = ha_ProcessP2TimeoutEvent(p_env, event);
			break;
		default:
			ha_Assert(false);
			break;
	}

	return ret;
}

int ha_ComputeSleepTime(ha_env_t *p_env)
{
	uint32 sleep_time_us = 0;
	uint32 deadline_time_us = 0;
	struct timeval now;
	ha_env_election_t *p_election = &p_env->election;

	gettimeofday(&now, NULL);

	if(ha_IsSetTimer1(p_election)){
		sleep_time_us = ha_Timeout1(p_env) / 100;
		deadline_time_us = ha_TimevalDiff(p_election->timer1_d, now);

		goto compute;
	}else if(ha_IsSetTimer2(p_election)){
		sleep_time_us = ha_Timeout2(p_env) / 100;
		deadline_time_us = ha_TimevalDiff(p_election->timer2_d, now);

		goto compute;
	}else if(ha_IsSetTimer4(p_election)){
		sleep_time_us = ha_Timeout4(p_env) / 100;
		deadline_time_us = ha_TimevalDiff(p_election->timer4_d, now);

		goto compute;
	}else{
		ha_Assert(false);
		ha_Error("wrong timer_flags: %d", p_election->timer_flags);
		sleep_time_us = 0;
	}

compute:
	if(deadline_time_us < 2 * sleep_time_us){
		sleep_time_us = deadline_time_us;
	}

	if(sleep_time_us < 10000){
		sleep_time_us = 10000;
	}

	return sleep_time_us;
}

/*
 * Description:
 *	Get happened event
 * 
 * We see timer timeout as event too.
 */
static ha_event_type_e ha_GetEvents(ha_env_t *p_env)
{
	uint32 all_leis = ha_AllLeis(p_env);
	uint32 min_votes = ha_MinVotes(p_env);

	/* check events */
	ha_Lock(ha_Lock4Event(p_env));
	if(ha_GotAllLeis(&p_env->event, all_leis)){
		ha_ClearAllLeis(&p_env->event);

		ha_Unlock(ha_Lock4Event(p_env));
		return HA_EVENT_GOT_ALL_LEIS;
	}
	if(ha_GotMinVotes(&p_env->event, min_votes)){
		ha_ClearAllVotes(&p_env->event);

		ha_Unlock(ha_Lock4Event(p_env));
		return HA_EVENT_GOT_MIN_VOTES;
	}
	if(ha_GotHeartBeat(&p_env->event)){
		//ha_ClearHeartbeat(p_env->event);

		ha_Unlock(ha_Lock4Event(p_env));
		return HA_EVENT_GOT_HEARTBEAT;
	}
	ha_Unlock(ha_Lock4Event(p_env));

	/* check timers */
	if(ha_Timer1Timeout(&p_env->election)){
		ha_ClearTimer1(&p_env->election);
		return HA_EVENT_TIMER1_TIMEOUT;
	}
	if(ha_Timer2Timeout(&p_env->election)){
		ha_ClearTimer2(&p_env->election);
		return HA_EVENT_TIMER2_TIMEOUT;
	}
	if(ha_Timer4Timeout(&p_env->election)){
		ha_ClearTimer4(&p_env->election);
		return HA_EVENT_TIMER4_TIMEOUT;
	}

	return HA_EVENT_NONE;
}

void ha_ElectionWaitForEvent(ha_env_t *p_env)
{
	uint32 sleep_time_us = 0;
	ha_event_type_e event = HA_EVENT_NONE;

	/* first check event ocur */
	while(1){
		event = ha_GetEvents(p_env);
		if(event != HA_EVENT_NONE){
			ha_DebugElection("State %s: recieved event %s",
					ha_EstateString(ha_Estate(&p_env->election)),
					ha_EventString(event));
			ha_ProcessEvent(p_env, event);
			break;
		}else{
			sleep_time_us = ha_ComputeSleepTime(p_env);

			//ha_printf(HA_D_ELECTION, "Sleep time: %d", sleep_time_us);
			ha_Sleep(sleep_time_us);
		}
	}

	return;
}

void *ha_StartElection(void *p_arg)
{
	ha_env_t *p_env = (ha_env_t *)p_arg;

	ha_Assert(p_env != NULL);

	ha_Log("*******************ELECTION START*******************");
	ha_Info2("Election start.........");
	ha_DebugElection("timeout1: %dus, timeout2: %dus, timeout4: %dus",
		ha_Timeout1(p_env), ha_Timeout2(p_env), ha_Timeout4(p_env));

	/* set master eid invalid */
	ha_Lock(ha_Lock4Event(p_env));
	/* we note this: nsites == 2, master down, and this node is sync standby */
	if((p_env->nsites == 2) && (ha_IsOtherEid(p_env, p_env->event.master_eid))
	&& (CurrentNodeSync())){
		s_master_down_2sites = true;
	}
	p_env->event.two_masters_found = false;
	ha_ClearDelayInfo(p_env);
	ha_ClearHeartbeat(&p_env->event);
	ha_Unlock(ha_Lock4Event(p_env));

	/* set election state "Phrase1 start" */
	ha_ElectionP1Start(p_env, true);

	/* wait for event */
wait_for_event:
	ha_ElectionWaitForEvent(p_env);

	if(ha_Estate(&p_env->election) == HA_ESTATE_SUCCESSED){
		goto successed;
	}else{
		goto wait_for_event;
	}

successed:
	ha_Info2("Election successed and exit.");
	ha_Log("*******************ELECTION END*******************");

	ha_RecordElectionExit(p_env);
	
#ifndef HA_SIMULATE_ENV
		proc_exit(0);
#endif

	return NULL;
}
/* *****************************************************************************
*						ha election main end
***************************************************************************** */

