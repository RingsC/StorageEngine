#ifndef __HA_H__
#define __HA_H__




/* *****************************************************************************
* 					ha timeval operations start
***************************************************************************** */
/* operation for struct timeval */
#define ha_ComputeDeadline(timev_d, timev_s, timeout_us) \
	do{ \
		ha_Assert(timeout_us > 0); \
		(timev_d).tv_usec = ((timev_s).tv_usec + (timeout_us)%1000000) % 1000000; \
		(timev_d).tv_sec = (timev_s).tv_sec + (timeout_us)/1000000 \
						+ ((timev_s).tv_usec + (timeout_us)%1000000) / 1000000; \
	}while(0)

#define ha_ComputeDeadlineByNow(timev_d, timeout_us) \
	do{ \
		struct timeval __now; \
		gettimeofday(&__now, NULL); \
		ha_ComputeDeadline(timev_d, __now, timeout_us); \
	}while(0)

/* timev1 after or equal(not before) timev2 */
#define ha_TimevalAfterOrEqual(timev1, timev2) \
	(((timev1).tv_sec == (timev2).tv_sec) \
	? ((timev1).tv_usec >= (timev2).tv_usec) \
	: ((timev1).tv_sec > (timev2).tv_sec))

/* timev1 after timev2 */
#define ha_TimevalAfter(timev1, timev2) \
	(((timev1).tv_sec == (timev2).tv_sec) \
	? ((timev1).tv_usec > (timev2).tv_usec) \
	: ((timev1).tv_sec > (timev2).tv_sec))

/* timev1 - timev2, if timev1 < timev2, return 0 */
#define ha_TimevalDiff(timev1, timev2) \
	(ha_TimevalAfter(timev1, timev2) \
	? (((timev1).tv_usec > (timev2).tv_usec) \
		? (((timev1).tv_sec - (timev2).tv_sec) * 1000000 \
			+ ((timev1).tv_usec - (timev2).tv_usec)) \
		: (((timev1).tv_sec - (timev2).tv_sec - 1) * 1000000 \
			+ ((timev1).tv_usec + 1000000 - (timev2).tv_usec))) \
	: 0)

static inline bool ha_TimevalAfterNow(struct timeval timev)
{
	struct timeval now;

	gettimeofday(&now, NULL);

	return ha_TimevalAfter(timev, now);
}
/* *****************************************************************************
* 					ha timeval operations start
***************************************************************************** */
















/* *****************************************************************************
* 					ha env election structs & funcs start
***************************************************************************** */
/*
 * Description:
 * 	election state
 * 
 */
enum ha_estate{
	HA_ESTATE_SUCCESSED,	/* election successed */
	HA_ESTATE_P1_START,		/* phrase1 start */
	HA_ESTATE_P1_TIMEOUT,	/* phrase1 timeout */
	HA_ESTATE_P2_START,		/* phrase2 start */
	HA_ESTATE_P2_TIMEOUT,	/* phrase2 timeout */
	HA_ESTATE_MAX,
};

/*
 * Description:
 * 	election timer flags
 * 
 */
#define HA_ETIMER_FLAGS_TIMER1 0x01
#define HA_ETIMER_FLAGS_TIMER2 0x02
#define HA_ETIMER_FLAGS_TIMER4 0x08

/* 
 * Description:
 * 	all data releated election
 *
 * estate & timers & timer_flags only change in ha_StartElection,
 * so don't need lock for use all about ha_env_election_t
 */
typedef struct ha_env_election{
	ha_estate estate; 				/* current state */

	uint32 timer_flags;				/* current timers */
	struct timeval timer1_d;		/* timer1 deadline */
	struct timeval timer2_d;		/* timer2 deadline */
	struct timeval timer4_d;		/* timer4 deadline */
}ha_env_election_t;

/*
 * Description:
 * 	set election state
 *
 */
#define ha_SetEstate(p_election, __estate) \
	do{ \
		ha_DebugElection("set state %s to %s", \
					ha_EstateString((p_election)->estate), \
					ha_EstateString(__estate)); \
		(p_election)->estate = (__estate); \
	}while(0)

/*
 * Description:
 * 	current election state
 *
 */
#define ha_Estate(p_election) ((p_election)->estate)

/*
 * Description:
 * 	set timer
 *
 * Timer1 & Timer4 always be set together,
 * so just provide ha_SetTimer1And4 to set both two timers.
 */
#define ha_SetTimer1And4(p_election, timeout1_us, timeout4_us) \
	do{ \
		struct timeval __now; \
		ha_Assert(!((p_election)->timer_flags & HA_ETIMER_FLAGS_TIMER1)); \
		ha_Assert(!((p_election)->timer_flags & HA_ETIMER_FLAGS_TIMER4)); \
		gettimeofday(&__now, NULL); \
		ha_ComputeDeadline((p_election)->timer4_d, __now, timeout4_us); \
		ha_ComputeDeadline((p_election)->timer1_d, __now, timeout1_us); \
		(p_election)->timer_flags |= HA_ETIMER_FLAGS_TIMER1; \
		(p_election)->timer_flags |= HA_ETIMER_FLAGS_TIMER4; \
	}while(0)

#define ha_SetTimer2(p_election, timeout2_us) \
	do{ \
		ha_Assert(!((p_election)->timer_flags & HA_ETIMER_FLAGS_TIMER2)); \
		ha_ComputeDeadlineByNow((p_election)->timer2_d, timeout2_us); \
		(p_election)->timer_flags |= HA_ETIMER_FLAGS_TIMER2; \
	}while(0)

/*
 * Description:
 * 	clear timer
 *
 */
#define ha_ClearTimer1(p_election) \
	do{\
		ha_Assert((p_election)->timer_flags & HA_ETIMER_FLAGS_TIMER1); \
		(p_election)->timer_flags &= (~HA_ETIMER_FLAGS_TIMER1); \
		struct timeval __now; \
		gettimeofday(&__now, NULL);\
	}while(0)

#define ha_ClearTimer2(p_election) \
	do{\
		ha_Assert((p_election)->timer_flags & HA_ETIMER_FLAGS_TIMER2); \
		(p_election)->timer_flags &= (~HA_ETIMER_FLAGS_TIMER2); \
	}while(0)

#define ha_ClearTimer4(p_election) \
	do{\
		ha_Assert((p_election)->timer_flags & HA_ETIMER_FLAGS_TIMER4); \
		(p_election)->timer_flags &= (~HA_ETIMER_FLAGS_TIMER4); \
	}while(0)

#define ha_ClearAllTimer(p_election) (p_election)->timer_flags = 0

/*
 * Description:
 * 	check timer set or not
 *
 */
#define ha_IsSetTimer1(p_election) \
	((p_election)->timer_flags & HA_ETIMER_FLAGS_TIMER1)
#define ha_IsSetTimer2(p_election) \
	((p_election)->timer_flags & HA_ETIMER_FLAGS_TIMER2)
#define ha_IsSetTimer4(p_election) \
	((p_election)->timer_flags & HA_ETIMER_FLAGS_TIMER4)

/*
 * Description:
 * 	check timer timeout or not
 *
 */
#define ha_Timer1Timeout(p_election) \
	(ha_IsSetTimer1(p_election) \
	? (!ha_TimevalAfterNow((p_election)->timer1_d)) \
	: false)
#define ha_Timer2Timeout(p_election) \
	(ha_IsSetTimer2(p_election) \
	? (!ha_TimevalAfterNow((p_election)->timer2_d)) \
	: false)
#define ha_Timer4Timeout(p_election) \
	(ha_IsSetTimer4(p_election) \
	? (!ha_TimevalAfterNow((p_election)->timer4_d)) \
	: false)

extern const char *ha_EstateString(ha_estate estate);
extern void ha_EstateStringsInit(void);
/* *****************************************************************************
* 					ha env election structs & funcs end
***************************************************************************** */














/* *****************************************************************************
* 						ha event structs & funcs start
***************************************************************************** */
/*
 * Description:
 * 	event type
 *
 */
enum ha_event_type_e{
	HA_EVENT_NONE,
	HA_EVENT_GOT_ALL_LEIS,
	HA_EVENT_GOT_MIN_VOTES,
	HA_EVENT_GOT_HEARTBEAT,
	HA_EVENT_TIMER1_TIMEOUT,
	HA_EVENT_TIMER2_TIMEOUT,
	HA_EVENT_TIMER4_TIMEOUT,
	HA_EVENT_MAX,
};

/*
 * Description:
 * 	local election info(lei)
 *
 * p_next is always NULL, unless we ha_SetDebugLeis for debug leis.
 */
#define HA_TIEBREAKER_MAX_LEN 256
typedef struct ha_lei{
	struct ha_lei *p_next;
	uint32 eid;
	uint32 priority;
	ha_lsn_t my_lsn;
	char c_tiebreaker[HA_TIEBREAKER_MAX_LEN];
}ha_lei_t;

typedef struct ha_vote{
	struct ha_vote *p_next;
	uint32 from_eid;
}ha_vote_t;

/*
 * Description:
 * 	event which will trigger election state transition
 *
 * all events will be change in ha_SelectLoop,
 * and will be check and change(because when we got event,
 * we need clear, or we always got this event.) in ha_StartElection,
 * so we need a lock for use ha_env_event_t.
 *
 * p_leis is always NULL, unless we use ha_SetDebugLeis for debug leis.
 */
typedef struct ha_env_event{
	//pthread_mutex_t mlock;

	uint32 nleis; 						/* count of recved leis */
	ha_lei_t win_lei; 					/* winner lei */
	ha_lei_t *p_leis; 					/* all recved leis */

	uint32 nvotes; 						/* how many votes I got */
	ha_vote_t *p_votes; 				/* all recved votes */

	bool recv_heartbeat; 				/* recv heartbeat */
	uint32 master_eid; 					/* master eid */
	struct timeval timev_hb_d; 			/* last recved heartbeat */
	struct timeval next_bcast_timev; 	/* next bcast new master timev */

	bool election_delay; 				/* election delay */
	time_t delay_to_s; 					/* election delay to, accurate to one second */

	bool need_manual_start; 			/* need manual start */
	bool got_start_cmd; 				/* got election command */

	bool two_masters_found;				/* two masters found */
}ha_env_event_t;

/*
 * Description:
 *	clear event which already happened
 * 
 * all macro functions must used with the protection of event lock.
 */
#define ha_ClearAllLeis(p_event) \
	do{ \
		ha_FreeSavedLeis(p_event); \
		(p_event)->nleis = 0; \
	}while(0)
#define ha_ClearAllVotes(p_event) \
	do{ \
		(p_event)->nvotes = 0; \
	}while(0)
#define ha_ClearHeartbeat(p_event) \
	do{ \
		ha_DebugElection("current heartbeat: %s, master eid: %d", \
				((p_event)->recv_heartbeat ? "true" : "false"), \
				(p_event)->master_eid); \
		(p_event)->recv_heartbeat = false; \
		(p_event)->master_eid = HA_EID_INVALID; \
	}while(0)

/*
 * Description:
 *	check got event
 * 
 * all macro functions must used with the protection of event lock.
 */
#define ha_GotAllLeis(p_event, all_leis) ((p_event)->nleis >= all_leis)
#define ha_GotMinVotes(p_event, min_votes) ((p_event)->nvotes >= min_votes)
#define ha_GotHeartBeat(p_event) ((p_event)->recv_heartbeat)

extern const char *ha_EventString(ha_event_type_e event);
extern void ha_EventStringsInit(void);
/* *****************************************************************************
* 					 ha event structs & funcs end
***************************************************************************** */














/* *****************************************************************************
* 						ha env structs & funcs start
***************************************************************************** */
#define HA_MAX_TIME_US 1000000000
#define HA_MIN_TIME_US 10000

#define HA_EID_INVALID 0xffffffff

#define HA_MAX_LISTEN 1

typedef struct ha_conn{
	uint32 nreconns;					/* how many times reconnect after connection disconnect */
	struct timeval next_reconn_timev;	/* next reconn timev */
	ha_tcp_conn_t tcp_conn;				/* tcp connection info */
}ha_conn_t;

typedef struct ha_conn_list{
	struct ha_conn_list *p_next;
	struct timeval conn_invalid_timev;
	uint32 eid;
	ha_tcp_conn_t *p_tcp_conn;
}ha_conn_list_t;

typedef struct ha_site{
	bool valid;
	ha_site_conf_t conf;
	ha_conn_t conn;
}ha_site_t;

#define HA_ENV_FLAGS_TESTSM 0x01
#define HA_ENV_FLAGS_SIMULATE_ENV 0x02

/*
 * Description:
 * 	all info about ha envionment
 *
 * This struct include:
 * 1. user config information, eg. priority, timeout, and so on
 * 2. also save the data related with ha group communication
 * 3. election info
 * 4. events
 */
typedef struct ha_env{
	pthread_mutex_t mlock;

	ha_env_election_t election;		/* all data related with election */
	ha_env_event_t event;			/* all data related with event */

	uint32 flags;					/* flags */

	/* the below are conf about election */
	uint32 priority;				/* my priority */

	uint32 timeout1_us; 			/* phrase1 timeout */
	uint32 timeout2_us;				/* phrase2 timeout */
	uint32 timeout4_us;				/* a round of election timeout */

	uint32 min_leis;				/* at least got min_leis, we can vote */
	uint32 min_votes;				/* at least got min_votes, we win and be master */

	uint32 timeout_hb_us;			/* heartbeat timeout */
	uint32 hb_interval_us;			/* heartbeat bcast time interval */
	uint32 election_delay_s;		/* delay election, when more than 1 master */
	bool manual_start_election;		/* manual start election */

	/* the below are conf about network & communication info */
	uint32 tmp_conn_life_time_us;	/* tmp conn life cycle */
	uint32 first_reconn_time_us;	/* first reconn time */

	int send_timeout_s;				/* send timeout */
	int recv_timeout_s;				/* recv timeout */

	int ka_idle_s;					/* keep alive idle */
	int ka_interval_s;				/* keep alive interval */
	int ka_count;					/* keep alive count */

	uint32 max_etimes_2sites;		/* nsites == 2, if master down, max election times */

	uint32 my_eid;					/* my eid */
	uint32 max_eid;					/* max eid */
	uint32 nsites;					/* current nsites */
	uint32 asites;					/* alloced sites */
	ha_site_t *p_sites;				/* all sites, now is a array */
	ha_conn_list_t *p_tmp_conns;	/* the conns we don't know where come from */
	pgsocket listen_socket[HA_MAX_LISTEN];	/* ha listen socket */
}ha_env_t;

/*
 * Description:
 * 	macros flags
 *
 */
#define ha_SetTestSm(p_env) ((p_env)->flags |= HA_ENV_FLAGS_TESTSM)
#define ha_ClearTestSm(p_env) ((p_env)->flags &= (~HA_ENV_FLAGS_TESTSM))
#define ha_IsTestSm(p_env) ((p_env)->flags & HA_ENV_FLAGS_TESTSM)

/*
 * Description:
 * 	macros about eid and site
 *
 */
#define ha_MaxEid(p_env) ((p_env)->max_eid)
#define ha_EidValid(p_env, eid) \
	(((eid) <= (p_env)->max_eid) && ((p_env)->p_sites[(eid)].valid))
#define ha_MyEid(p_env) ((p_env)->my_eid)
#define ha_IsMyEid(p_env, eid) ((eid) == ha_MyEid(p_env))
#define ha_IsOtherEid(p_env, eid)\
	((!ha_IsMyEid(p_env, eid)) && (ha_EidValid(p_env, eid)))

#define ha_SiteByEid(p_env, eid)  \
	(ha_EidValid(p_env, eid) ? (&((p_env)->p_sites[(eid)])) : NULL)
#define ha_SiteConfByEid(p_env, eid) \
	(ha_EidValid(p_env, eid) ? (&((p_env)->p_sites[(eid)].conf)) : NULL)
#define ha_MySite(p_env) (ha_SiteByEid((p_env), ha_MyEid(p_env)))
#define ha_MySiteConf(p_env) (ha_SiteConfByEid((p_env), ha_MyEid(p_env)))
#define ha_MyPriority(p_env) ((p_env)->priority)

/*
 * Description:
 * 	macros about election & event
 *
 */
#define ha_DefaultMinLeis(p_env) ((p_env)->nsites / 2 + 1)
#define ha_DefaultMinVotes(p_env) ((p_env)->nsites / 2 + 1)
//#define ha_AllLeis(p_env) ((p_env)->nsites - 1)
#define ha_AllLeis(p_env) ((p_env)->nsites)
#define ha_MinLeis(p_env) (((p_env)->min_leis == 0) \
	? (ha_DefaultMinLeis((p_env))) : ((p_env)->min_leis))
#define ha_MinVotes(p_env) (((p_env)->min_votes == 0) \
	? (ha_DefaultMinVotes(p_env)) : ((p_env)->min_votes))

#define ha_Timeout1(p_env) ((p_env)->timeout1_us)
#define ha_Timeout2(p_env) ((p_env)->timeout2_us)
#define ha_Timeout4(p_env) ((p_env)->timeout4_us)

#define ha_TimeoutHb(p_env) ((p_env)->timeout_hb_us)
#define ha_HbTimeInterval(p_env) ((p_env)->hb_interval_us)
#define ha_ElectionDelaySecond(p_env) ((p_env)->election_delay_s)

#define ha_TmpConnLifeTime(p_env) ((p_env)->tmp_conn_life_time_us)
#define ha_FirstReconnTime(p_env) ((p_env)->first_reconn_time_us)
#define ha_ReconnTime(p_env, nreconns) \
	(((nreconns) < (HA_MAX_TIME_US / ha_FirstReconnTime(p_env))) \
	? (ha_FirstReconnTime(p_env)) \
	: (HA_MAX_TIME_US))

/*
 * Description:
 * 	macros about net conf
 *
 */
#define ha_SendTimeout(p_env) ((p_env)->send_timeout_s)
#define ha_RecvTimeout(p_env) ((p_env)->recv_timeout_s)

#define ha_KaIdle(p_env) ((p_env)->ka_idle_s)
#define ha_KaInterval(p_env) ((p_env)->ka_interval_s)
#define ha_KaCount(p_env) ((p_env)->ka_count)

#define ha_NReconns(p_site) ((p_site)->conn.nreconns)
#define ha_NextReconn(p_site) (ha_NReconns(p_site) + 1)
#define ha_IncNReconns(p_site) ((p_site)->conn.nreconns++)

#define ha_InitLock(p_env) pthread_mutex_init(&((p_env)->mlock), NULL)
#define ha_DestroyLock(p_env) pthread_mutex_destroy(&((p_env)->mlock))

#if 0
#define ha_Lock(p_lock) \
	do{ \
		ha_Log("Start to lock..."); \
		pthread_mutex_lock(p_lock); \
		ha_Log("In lock"); \
	}while(0)
#define ha_Unlock(p_lock) \
	do{ \
		ha_Log("Start to Unlock..."); \
		pthread_mutex_unlock(p_lock); \
		ha_Log("End lock"); \
	}while(0)
#else
#define ha_Lock(p_lock) pthread_mutex_lock(p_lock)
#define ha_Unlock(p_lock) pthread_mutex_unlock(p_lock)
#endif

#define ha_Lock4Env(p_env) (&((p_env)->mlock))
#define ha_Lock4Event(p_env) ha_Lock4Env(p_env)

extern ha_env_t g_ha_env;
/* *****************************************************************************
* 						ha env structs & funcs end
***************************************************************************** */















/* *****************************************************************************
* 						ha election result and others start
***************************************************************************** */
#define HA_MAX_IP_LEN 128

typedef struct ha_election_log{
	pthread_mutex_t mlock;

	void *p_election_thread;
	bool first_election_done;
	bool i_am_master;
	char c_master_ip[HA_MAX_IP_LEN];
	uint16 master_port;
}ha_election_log_t;

extern ha_election_log_t g_ha_election_log;

extern bool ha_FirstElectionDone(void);
extern bool ha_GetElectionResult(bool *p_i_am_master, char **pc_master_ip,
	uint16 *p_master_port);
/* *****************************************************************************
* 						ha election result and others end
***************************************************************************** */











extern void ha_SendMsgByEid(ha_env_t *p_ha_env, uint32 eid,
	const char * p_msg);
extern void ha_BcastMsg(ha_env_t *p_ha_env, const char * p_msg);

extern void ha_GotMsgLeiDo(ha_env_t *p_env, ha_lei_t *p_new_lei);
extern void ha_GotMsgVoteDo(ha_env_t *p_env, uint32 from_eid);
extern void ha_GotMsgBcastNewMasterDo(ha_env_t *p_env,
	uint32 master_eid, char *pc_ip, uint32 port);
extern void ha_GotMsgBcastTwoMastersDo(ha_env_t *p_env);

extern void ha_BcastNewMaster(ha_env_t *p_env);

extern void *ha_SelectLoop(void *p_arg);
extern void *ha_StartElection(void *p_arg);

extern void ha_CreateTmpConn(ha_env_t *p_env, uint32 eid);
extern bool ha_FirstConnSite(ha_env_t *p_env, uint32 eid);
extern void ha_FirstConnSiteNoBlock(ha_env_t *p_env, uint32 eid);

extern bool ha_ParseIpPort(const char *pc_site, char **pp_ip, uint16 *p_port);
extern bool ha_EnvInit(ha_env_t *p_env, const char *pc_ip,
	rep_ha_t *p_ha_conf, bool assign_master);

extern bool ha_ElectionLogInit(ha_env_t *p_env, bool assign_master);



extern bool ha_ElectionThreadExist(void);
extern void ha_RecordElectionExit(ha_env_t *p_env);
extern void ha_RecordElectionStart(void *p_arg);
extern void fxdb_election_forkexec(void *p_arg);

extern bool select_thread_need_stop;
#define ha_SelectThreadNeedStop() (select_thread_need_stop)

extern FILE *g_ha_log_file;

#endif

