#include "ha_head.h"








static bool ha_SendMsgId(pgsocket conn_socket, ha_site_conf_t *p_my_site_conf);
static bool ha_SendMsgIdRsp(pgsocket conn_socket, bool rsp_ok);
static bool ha_SendMsgIdAndWaitRsp(pgsocket conn_socket,
	ha_site_conf_t *p_my_site_conf);








/* *****************************************************************************
* 						ha env tmp conn functions start
***************************************************************************** */
#define ha_TmpConnExpire(p_tmp_conn) \
	(!ha_TimevalAfterNow(p_tmp_conn->conn_invalid_timev))
#define ha_TmpConnExpireByTimev(p_tmp_conn, timev) \
	(!ha_TimevalAfter(p_tmp_conn->conn_invalid_timev, timev))



#define ha_TmpConnEstablished(p_tmp_conn) \
	((p_tmp_conn->p_tcp_conn->state == HA_TCONN_STATE_ESTABLISHED) \
	&& (!ha_SocketIsInvalid(p_tmp_conn->p_tcp_conn->socket)))
#define ha_TmpConnConnecting(p_tmp_conn) \
	((p_tmp_conn->p_tcp_conn->state == HA_TCONN_STATE_CONNING) \
	&& (!ha_SocketIsInvalid(p_tmp_conn->p_tcp_conn->socket)))

#define ha_TmpConnAbnormal(p_tmp_conn) \
	((p_tmp_conn->p_tcp_conn->state == HA_TCONN_STATE_FAILED) \
	|| (ha_SocketIsInvalid(p_tmp_conn->p_tcp_conn->socket)))

#define ha_SetTmpConnFailed(p_tmp_conn) \
	do{ \
		ha_Log("Tmp conn[eid: %d] failed", p_tmp_conn->eid); \
		(p_tmp_conn)->p_tcp_conn->state = HA_TCONN_STATE_FAILED; \
	}while(0)
#define ha_SetTmpConnEstablished(p_tmp_conn) \
	do{ \
		ha_Log("Tmp conn[eid: %d] established", p_tmp_conn->eid); \
		(p_tmp_conn)->p_tcp_conn->state = HA_TCONN_STATE_ESTABLISHED; \
	}while(0)








static void ha_QueueGivenTmpConn(ha_env_t *p_env,
	ha_conn_list_t *p_given_tmp_conn)
{
	p_given_tmp_conn->p_next = p_env->p_tmp_conns;
	p_env->p_tmp_conns = p_given_tmp_conn;

	return;
}

static bool ha_DeQueueGivenTmpConn(ha_env_t *p_env,
	ha_conn_list_t *p_given_tmp_conn)
{
	if(p_env->p_tmp_conns == NULL){
		ha_Assert(false);
		return false;
	}

	if(p_given_tmp_conn == p_env->p_tmp_conns){
		/* this tmp conn is header */
		p_env->p_tmp_conns = p_given_tmp_conn->p_next;
		return true;
	}else{
		ha_conn_list_t *p_tmp_conn = p_env->p_tmp_conns;

		while(p_tmp_conn->p_next != NULL){
			if(p_tmp_conn->p_next == p_given_tmp_conn){
				p_tmp_conn->p_next = p_given_tmp_conn->p_next;
				return true;
			}
			p_tmp_conn = p_tmp_conn->p_next;
		}
	}

	ha_Warning("Not found the tmp conn");
	return false;
}

static void ha_FreeGivenTmpConn(ha_conn_list_t *p_given_tmp_conn)
{
	p_given_tmp_conn->p_tcp_conn = NULL;
	free(p_given_tmp_conn->p_tcp_conn);
	free(p_given_tmp_conn);
	return;
}

static bool ha_SetTmpConnRSTimeout(ha_env_t *p_env,
	ha_conn_list_t *p_tmp_conn)
{
	ha_Assert(ha_TmpConnEstablished(p_tmp_conn));
	if(!ha_SetSendTimeout(p_tmp_conn->p_tcp_conn->socket, ha_SendTimeout(p_env))){
		return false;
	}

	if(!ha_SetRecvTimeout(p_tmp_conn->p_tcp_conn->socket, ha_RecvTimeout(p_env))){
		return false;
	}

	return true;
}

static void ha_TmpConnEstablishedDo(ha_env_t *p_env,
	ha_conn_list_t *p_tmp_conn)
{
	ha_Assert(p_tmp_conn->p_tcp_conn->state == HA_TCONN_STATE_ESTABLISHED);

	/* set sock send & recv timeout */
	if(!ha_SetTmpConnRSTimeout(p_env, p_tmp_conn)){
		ha_Warning("Can't set tmp conn timeout.");
		ha_SetTmpConnFailed(p_tmp_conn);
		return;
	}

	if (!ha_SetNoDelay(p_tmp_conn->p_tcp_conn->socket))
	{
		ha_Warning("Can't set tmp conn TCP_NODELAY.");
		ha_SetTmpConnFailed(p_tmp_conn);
		return;
	}

	/* if known eid(my site connect to p_tmp_conn->eid), so should send my site conf */
	if(p_tmp_conn->eid != HA_EID_INVALID){
		if(!ha_SendMsgId(p_tmp_conn->p_tcp_conn->socket, ha_MySiteConf(p_env))){
			ha_SetTmpConnFailed(p_tmp_conn);
			return;
		}
	}

	return;
}

static void ha_AcceptTmpConn(ha_env_t *p_env, pgsocket listen_socket)
{
	ha_tcp_conn_t *p_new_tcp_conn = ha_Accept(listen_socket, ha_MySiteConf(p_env));

	if(p_new_tcp_conn != NULL){
		ha_conn_list_t *p_tmp_conn = (ha_conn_list_t *)malloc(sizeof(ha_conn_list_t));

		ha_Assert(p_new_tcp_conn->state == HA_TCONN_STATE_ESTABLISHED);

		if(p_tmp_conn == NULL){
			ha_EreportOOM();
		}

		p_tmp_conn->p_next = NULL;
		p_tmp_conn->eid = HA_EID_INVALID;
		p_tmp_conn->p_tcp_conn = p_new_tcp_conn;
		ha_ComputeDeadlineByNow(p_tmp_conn->conn_invalid_timev,
			ha_TmpConnLifeTime(p_env));

		ha_TmpConnEstablishedDo(p_env, p_tmp_conn);

		ha_QueueGivenTmpConn(p_env, p_tmp_conn);

		ha_DebugSelect("Accept a tmp connection, life time is %d",
				ha_TmpConnLifeTime(p_env));
	}

	return;
}

void ha_CreateTmpConn(ha_env_t *p_env, uint32 eid)
{
	ha_site_conf_t *p_site_conf = ha_SiteConfByEid(p_env, eid);
	ha_conn_list_t *p_tmp_conn = (ha_conn_list_t *)malloc(sizeof(ha_conn_list_t));
	ha_tcp_conn_t *p_tcp_conn = NULL;

	ha_DebugSelect("Start to connect site: eid[%d]", eid);

	if(p_site_conf == NULL){
		ha_Error("wrong eid: %d", eid);
		return;
	}

	if(p_tmp_conn == NULL){
		ha_EreportOOM();
	}

	p_tcp_conn = ha_ConnectNoBlock(ha_MySiteConf(p_env),p_site_conf);
	if(p_tcp_conn == NULL){
		ha_DebugSelect("Connect site: eid[%d] failed", eid);
		return;
	}

	if((p_tcp_conn->state != HA_TCONN_STATE_ESTABLISHED)
	&& (p_tcp_conn->state != HA_TCONN_STATE_CONNING)){
		ha_Assert(false);
	}

	ha_DebugSelect("Connecting site: eid[%d]", eid);

	p_tmp_conn->p_next = NULL;
	p_tmp_conn->eid = eid;
	p_tmp_conn->p_tcp_conn = p_tcp_conn;
	ha_ComputeDeadlineByNow(p_tmp_conn->conn_invalid_timev,
		ha_TmpConnLifeTime(p_env));

	if(ha_TmpConnEstablished(p_tmp_conn)){
		ha_TmpConnEstablishedDo(p_env, p_tmp_conn);
	}

	ha_QueueGivenTmpConn(p_env, p_tmp_conn);

	ha_DebugSelect("Create a tmp connection to eid[%d], life time is %dus",
			eid, ha_TmpConnLifeTime(p_env));

	return;
}



static void ha_ProcAbnormalOrExpireTmpConn(ha_env_t *p_env,
	ha_conn_list_t *p_given_tmp_conn)
{
	/* close socket */
	if(!ha_SocketIsInvalid(p_given_tmp_conn->p_tcp_conn->socket)){
		ha_CloseSocket(p_given_tmp_conn->p_tcp_conn->socket);
	}

	/* dequeue */
	if(!ha_DeQueueGivenTmpConn(p_env, p_given_tmp_conn)){
		ha_Assert(false);
		return;
	}

	/* free given tmp conn */
	ha_FreeGivenTmpConn(p_given_tmp_conn);

	return;
}
/* *****************************************************************************
*						ha env tmp conn functions end
***************************************************************************** */















/* *****************************************************************************
* 						ha env conn functions start
***************************************************************************** */
#define ha_SiteConnAbnormal(p_site) \
	((p_site->conn.tcp_conn.state != HA_TCONN_STATE_OK) \
	|| (ha_SocketIsInvalid(p_site->conn.tcp_conn.socket)))
#define ha_SetSiteConnNextReconnTimev(p_site, timev, timeout_us) \
	ha_ComputeDeadline((p_site)->conn.next_reconn_timev, timev, timeout_us);\

#define ha_SetSiteConnFailedByTimevAndReconnTime(p_site, timev, reconn_time) \
	do{ \
		if(p_site->conn.tcp_conn.state != HA_TCONN_STATE_FAILED){ \
			ha_Log("SiteConn to site[%d] failed, reconn time %uus.", \
				p_site->conf.eid, (uint32)reconn_time); \
			p_site->conn.tcp_conn.state = HA_TCONN_STATE_FAILED; \
			ha_SetSiteConnNextReconnTimev(p_site, timev, reconn_time);\
		} \
	}while(0)
#define ha_SetSiteConnFailedByTimev(p_env, p_site, timev) \
	ha_SetSiteConnFailedByTimevAndReconnTime(p_site, timev, \
		ha_ReconnTime(p_env, ha_NextReconn(p_site)))
#define ha_SetSiteConnFailed(p_env, p_site) \
	do{ \
		struct timeval __now; \
		gettimeofday(&__now, NULL); \
		ha_SetSiteConnFailedByTimev(p_env, p_site, __now); \
	}while(0)

static void ha_SetSiteConnFailed4Conflict(ha_env_t *p_env,
	ha_site_t *p_site)
{
	struct timeval __now;
	uint32 reconn_time;
	static uint32 call_times = 0;
	const int max_seeds = 64;
	static uint32 seeds[max_seeds];

	gettimeofday(&__now, NULL);

	// TODO:
	if(call_times == 0){
		ha_site_conf_t *p_my_site_conf = ha_MySiteConf(p_env);
		uint32 offset = 0;

		while((offset + 1) < sizeof(seeds)){
			snprintf(((char *)seeds + offset),
				(sizeof(seeds) - offset), "%p%p%p%ld%s%d",
				&__now, &reconn_time, &offset, __now.tv_usec, 
				p_my_site_conf->pc_ip, p_my_site_conf->port);
			offset = (uint32)strlen((char *)seeds);
		}
	}

	srand(seeds[call_times%max_seeds]);
	call_times++;

	reconn_time = rand()%(ha_Timeout1(p_env)) + HA_MIN_TIME_US;

	ha_SetSiteConnFailedByTimevAndReconnTime(p_site, __now, reconn_time);
	return;
}

static bool ha_SiteSaveNewTcpConn(ha_env_t *p_env, uint32 eid,
	const ha_tcp_conn_t *p_tcp_conn)
{
	ha_site_t *p_site = ha_SiteByEid(p_env, eid);
	pgsocket old_sock;
	bool old_conn_abnormal;
	bool ret;

	ha_Assert(p_tcp_conn->state == HA_TCONN_STATE_OK);
	ha_Assert(!ha_SocketIsInvalid(p_tcp_conn->socket));

	if(!ha_IsOtherEid(p_env, eid)){
		ha_Assert(false);
		ha_Error("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%d is not other eid", eid);
		return false;
	}

	/* set site conn keep alive */
	ret = ha_SetKeepAlive(p_tcp_conn->socket, ha_KaIdle(p_env),
			ha_KaInterval(p_env), ha_KaCount(p_env));
	if(!ret){
		ha_Warning("Set keep alive failed.");
	}

	ha_Lock(ha_Lock4Env(p_env));
	old_sock = p_site->conn.tcp_conn.socket;
	old_conn_abnormal = ha_SiteConnAbnormal(p_site);
	if(old_conn_abnormal){
		if(!ha_SocketIsInvalid(old_sock)){
			ha_CloseSocket(old_sock);
		}

		p_site->conn.nreconns = 0;
		p_site->conn.tcp_conn.state = p_tcp_conn->state;
		p_site->conn.tcp_conn.socket = p_tcp_conn->socket;
		ha_Unlock(ha_Lock4Env(p_env));

		ha_Log("SiteConn to site[%d] OK, socket[%d]. close old socket[%d]",
			eid, p_tcp_conn->socket, old_sock);
		ret = true;
	}else{
		/* should not update siteconn */
		ha_SetSiteConnFailed4Conflict(p_env, p_site);

		/* close socket */
		ha_CloseSocket(old_sock);
		p_site->conn.tcp_conn.socket = PGINVALID_SOCKET;

		ha_Unlock(ha_Lock4Env(p_env));

		ha_Warning("SiteConn to site[%d] not update, socket[%d]."
			" because old conn is OK, socket: %d",
			eid, p_tcp_conn->socket, old_sock);
		ret =  false;
	}

	return ret;
}

static void ha_MoveTmpConnToSite(ha_env_t *p_env,
	ha_conn_list_t *p_given_tmp_conn)
{
	if(!ha_DeQueueGivenTmpConn(p_env, p_given_tmp_conn)){
		ha_Error("Dequeue tmp conn failed");
		ha_Assert(false);
		return;
	}

	/* set tmp conn OK */
	p_given_tmp_conn->p_tcp_conn->state = HA_TCONN_STATE_OK;

	/* save new tcp conn */
	if(!ha_SiteSaveNewTcpConn(p_env, p_given_tmp_conn->eid,
	p_given_tmp_conn->p_tcp_conn)){
		ha_CloseSocket(p_given_tmp_conn->p_tcp_conn->socket);
	}

	/* free given tmp conn */
	ha_FreeGivenTmpConn(p_given_tmp_conn);

	return;
}

bool ha_FirstConnSite(ha_env_t *p_env, uint32 eid)
{
	ha_site_conf_t *p_my_site_conf = ha_MySiteConf(p_env);
	ha_site_t *p_site = ha_SiteByEid(p_env, eid);
	ha_tcp_conn_t *p_tcp_conn = NULL;
	bool conn_ok = false;

	ha_DebugSelect("First connect site: eid[%d]", eid);

	ha_Assert(p_site != NULL);

	p_tcp_conn = ha_Connect(p_my_site_conf, &p_site->conf,
					ha_SendTimeout(p_env), ha_RecvTimeout(p_env));
	if(p_tcp_conn != NULL){
		ha_Assert(p_tcp_conn->state == HA_TCONN_STATE_ESTABLISHED);

		if(ha_SendMsgIdAndWaitRsp(p_tcp_conn->socket, p_my_site_conf)){
			conn_ok = true;
		}
	}

	/* update conn info */
	if(conn_ok){
		/* save conn */
		p_tcp_conn->state = HA_TCONN_STATE_OK;
		p_site->conn.tcp_conn.socket = PGINVALID_SOCKET;
		ha_SiteSaveNewTcpConn(p_env, eid, p_tcp_conn);
	}else{
		if(p_tcp_conn != NULL){
			ha_CloseSocket(p_tcp_conn->socket);
		}
		p_site->conn.nreconns = 0;
		p_site->conn.tcp_conn.socket = PGINVALID_SOCKET;
		//p_site->conn.tcp_conn.state = HA_TCONN_STATE_FAILED;
		ha_SetSiteConnFailed(p_env, p_site);
	}

	/* free tcp conn */
	if(p_tcp_conn != NULL){
		free(p_tcp_conn);
		return true;
	}

	return false;
}

void ha_FirstConnSiteNoBlock(ha_env_t *p_env, uint32 eid)
{
	ha_site_t *p_site = ha_SiteByEid(p_env, eid);

	ha_DebugSelect("First connect site no block: eid[%d]", eid);

	ha_Assert(p_site != NULL);

	/* init conn  struct */
	p_site->conn.nreconns = 0;
	p_site->conn.tcp_conn.socket = PGINVALID_SOCKET;
	p_site->conn.tcp_conn.state = HA_TCONN_STATE_FAILED;
	ha_SetSiteConnFailed(p_env, p_site);

	ha_CreateTmpConn(p_env, eid);

	return;
}

static void ha_ReconnSite(ha_env_t *p_env, uint32 eid, uint32 nreconns)
{
	ha_Log("Reconnect[N:%d] site[%d]", nreconns, eid);
	ha_CreateTmpConn(p_env, eid);
	return;
}

uint32 ha_ProcAbnormalSiteConn(ha_env_t *p_env, ha_site_t *p_site,
	struct timeval now)
{
	uint32 timeout_us;
	uint32 next_reconn;

	ha_Lock(ha_Lock4Env(p_env));

	if(p_site->conn.tcp_conn.state != HA_TCONN_STATE_FAILED){
		ha_SetSiteConnFailedByTimev(p_env, p_site, now);
	}

	if(!ha_SocketIsInvalid(p_site->conn.tcp_conn.socket)){
		ha_CloseSocket(p_site->conn.tcp_conn.socket);
		ha_SetSocketInvalid(p_site->conn.tcp_conn.socket);
	}

	/* reconn */
	next_reconn = ha_NextReconn(p_site);
	timeout_us = ha_TimevalDiff(p_site->conn.next_reconn_timev, now);
	if(timeout_us != 0){ /* next_reconn_timev is after now */
		ha_Unlock(ha_Lock4Env(p_env));

		ha_DebugSelect("%uus before next connect site: eid:%d nextreconn:%d",
			timeout_us, p_site->conf.eid, next_reconn);
	}else{ /* next_reconn_timev is equal or after now */
		timeout_us = ha_TimevalDiff(now, p_site->conn.next_reconn_timev);
		ha_Unlock(ha_Lock4Env(p_env));

		ha_DebugSelect("Reconnect[%d] site[eid:%d] after timeout %uus",
			next_reconn, p_site->conf.eid, timeout_us);

		/* reconn */
		ha_ReconnSite(p_env, p_site->conf.eid, next_reconn);

		ha_Lock(ha_Lock4Env(p_env));

		/* check conn state */
		if(!ha_SiteConnAbnormal(p_site)){
			timeout_us = 0;
		}else{
			/* increase nreconns */
			ha_IncNReconns(p_site);
			next_reconn = ha_NextReconn(p_site);
			timeout_us = ha_ReconnTime(p_env, next_reconn);
			ha_SetSiteConnNextReconnTimev(p_site, now, timeout_us);
		}

		ha_Unlock(ha_Lock4Env(p_env));
	}
	
	return timeout_us;
}
/* *****************************************************************************
*						ha env conn functions end
***************************************************************************** */
















/* *****************************************************************************
* 						ha send election msg functions end
***************************************************************************** */
static bool ha_SendMsgId(pgsocket conn_socket, ha_site_conf_t *p_my_site_conf)
{
	char *p_msg = ha_PackMsgId(p_my_site_conf->pc_ip, p_my_site_conf->port);
	bool ret = true;

	if(p_msg == NULL){
		ha_Error("Pack msg ID failed");
		ret = false;
		goto out;
	}

	if(ha_SendMsg(conn_socket, p_msg) != STATUS_OK){
		ha_Error("Send msg ID failed");
		ret = false;
		goto out;
	}

out:
	ha_free(p_msg);

	return ret;
}

static bool ha_SendMsgIdRsp(pgsocket conn_socket, bool rsp_ok)
{
	char *p_msg = ha_PackMsgIdRsp(rsp_ok);
	bool ret = true;

	if(p_msg == NULL){
		ha_Error("Pack msg ID failed");
		ret = false;
		goto out;
	}

	if(ha_SendMsg(conn_socket, p_msg) != STATUS_OK){
		ha_Error("Send msg ID failed");
		ret = false;
		goto out;
	}

out:
	ha_free(p_msg);

	return ret;
}

static bool ha_SendMsgIdAndWaitRsp(pgsocket conn_socket,
	ha_site_conf_t *p_my_site_conf)
{
	char *p_rsp_msg = NULL;
	bool rsp_ok = false;
	bool ret = true;

	if(!ha_SendMsgId(conn_socket, p_my_site_conf)){
		ret = false;
		goto out;
	}

	if(ha_RecvMsg(conn_socket, &p_rsp_msg) != STATUS_OK){
		ha_Error("Recv msg ID RSP failed");
		ret = false;
		goto out;
	}else{
		if (p_rsp_msg != NULL)
		{
			ret = ha_UnpackMsgIdRsp(p_rsp_msg, &rsp_ok);
			if((!ret) || (!rsp_ok)){
				ha_Error("msg ID RSP: failed");
				ret = false;
				goto out;
			}
		}		
	}

out:
	if(p_rsp_msg){
		ha_free(p_rsp_msg);
	}

	return ret;
}

void ha_BcastMsg(ha_env_t *p_env, const char * p_msg)
{
	uint32 eid = 0;
	ha_site_t *p_site = NULL;
	pgsocket conn_sock;
	int ret;

	for(eid = 0; eid <= ha_MaxEid(p_env); eid++){
		p_site = ha_SiteByEid(p_env, eid);

		if((eid == ha_MyEid(p_env)) || (p_site == NULL)){
			continue;
		}

		ha_Lock(ha_Lock4Env(p_env));

		conn_sock = p_site->conn.tcp_conn.socket;
		if(ha_SiteConnAbnormal(p_site)){
			ha_Unlock(ha_Lock4Env(p_env));
		}else{
			ha_Unlock(ha_Lock4Env(p_env));

			ret = ha_SendMsg(conn_sock, p_msg);
			if(ret == STATUS_OK){
				ha_DebugSelect("Send msg to site[%d] successed", eid);
			}else{
				ha_Warning("Send msg to site[%d] failed", eid);

				ha_Lock(ha_Lock4Env(p_env));

				ha_SetSiteConnFailed(p_env, p_site);

				ha_Unlock(ha_Lock4Env(p_env));
			}
		}
	}

	return;
}

void ha_SendMsgByEid(ha_env_t *p_env, uint32 eid, const char * p_msg)
{
	ha_site_t *p_site = ha_SiteByEid(p_env, eid);
	pgsocket conn_sock;
	int ret;

	if((eid == ha_MyEid(p_env)) || p_site == NULL){
		ha_Error("Can't find site for eid[%d] or not send to my site", eid);
		return;
	}

	ha_Lock(ha_Lock4Env(p_env));
	conn_sock = p_site->conn.tcp_conn.socket;
	if(ha_SiteConnAbnormal(p_site)){
		ha_Unlock(ha_Lock4Env(p_env));
	}else{
		ha_Unlock(ha_Lock4Env(p_env));

		ret = ha_SendMsg(conn_sock, p_msg);

		if(ret == STATUS_OK){
			ha_DebugSelect("Send msg to site[%d] successed", eid);
		}else{
			ha_Warning("Send msg to site[%d] failed", eid);

			ha_Lock(ha_Lock4Env(p_env));

			ha_SetSiteConnFailed(p_env, p_site);

			ha_Unlock(ha_Lock4Env(p_env));
		}
	}

	return;
}
/* *****************************************************************************
* 						ha send election msg functions end
***************************************************************************** */














/* *****************************************************************************
* 						ha env process msg functions start
***************************************************************************** */
void ha_ProcMsgLei(ha_env_t *p_env, char *p_msg, uint32 eid)
{
	ha_lei_t new_lei;

	memset((char *)&new_lei, 0, sizeof(ha_lei_t));

	if(!ha_UnpackMsgLei(p_msg, &new_lei.priority, &new_lei.my_lsn,
	new_lei.c_tiebreaker)){
		ha_Warning("Got wrong msg");
		return;
	}

	/* set new_lei eid */
	new_lei.p_next = NULL;
	new_lei.eid = eid;

	/* update nleis & win_lei */
	ha_GotMsgLeiDo(p_env, &new_lei);
	return;
}

void ha_ProcMsgVote(ha_env_t *p_env, char *p_msg, uint32 eid)
{
	if(!ha_UnpackMsgVote(p_msg)){
		ha_Warning("Got wrong msg");
		return;
	}

	ha_GotMsgVoteDo(p_env, eid);
	return;
}

void ha_ProcMsgBcastNewMaster(ha_env_t *p_env, char *p_msg, uint32 eid)
{
	char *pc_ip = NULL;
	uint16 port;

	if(!ha_UnpackMsgNewMaster(p_msg, &pc_ip, &port)){
		ha_Warning("Got wrong msg");
		return;
	}

	ha_GotMsgBcastNewMasterDo(p_env, eid, pc_ip, port);

	if(pc_ip != NULL){
		ha_free(pc_ip);
	}

	return;
}

void ha_ProcMsgBcastTwoMasters(ha_env_t *p_env, char *p_msg, uint32 eid)
{
	char *pc_ip = NULL;
	//uint16 port;

	if(!ha_UnpackMsgTwoMasters(p_msg)){
		ha_Warning("Got wrong msg");
		return;
	}

	ha_GotMsgBcastTwoMastersDo(p_env);

	if(pc_ip != NULL){
		ha_free(pc_ip);
	}

	return;
}

void ha_SiteConnProcMsg(ha_env_t *p_env, char *p_msg, uint32 eid)
{
	ha_msg_type msg_type = ha_MsgType(p_msg);

	ha_DebugSelect("process msg from eid:%d", eid);

	ha_Assert(eid <= ha_MaxEid(p_env));

	switch(msg_type){
		case HA_MSG_LEI:
			ha_ProcMsgLei(p_env, p_msg, eid);
			break;
		case HA_MSG_VOTE:
			ha_ProcMsgVote(p_env, p_msg, eid);
			break;
		case HA_MSG_NEW_MASTER:
			ha_ProcMsgBcastNewMaster(p_env, p_msg, eid);
			break;
		case HA_MSG_TWO_MASTERS:
			ha_ProcMsgBcastTwoMasters(p_env, p_msg, eid);
			break;
		default:
			ha_Error("Unknow msg[type: %d] from site[%d]",
				msg_type, eid);
			ha_DumpData((const unsigned char *)p_msg, ha_MsgTotalLen(p_msg),
				(const unsigned char *)"Msg", true);
			break;
	}

	return;
}

uint32 ha_FindEidByIpPort(const ha_env_t *p_env,
	const char *pc_ip, uint16 port)
{
	struct addrinfo *p_ai = ha_BuildAddrinfo(pc_ip, port);

	ha_DebugSelect("Find %s:%d", pc_ip, port);

	if(p_ai == NULL){
		return HA_EID_INVALID;
	}

	for(uint32 eid = 0; eid <= ha_MaxEid(p_env); eid++){
		ha_site_conf_t *p_site_conf = ha_SiteConfByEid(p_env, eid);

		if(p_site_conf != NULL){
			if(p_site_conf->port == port){
				if(ha_IpEqual(p_ai->ai_addr, p_ai->ai_addrlen,
				p_site_conf->p_ai->ai_addr, p_site_conf->p_ai->ai_addrlen)){
					ha_DebugSelect("Find eid: %d", p_site_conf->eid);
					ha_FreeAddrinfo(p_ai);
					return p_site_conf->eid;
				}
			}
		}
	}

	ha_FreeAddrinfo(p_ai);
	return HA_EID_INVALID;
}

void ha_ProcMsgId(ha_env_t *p_env, ha_conn_list_t *p_tmp_conn, char *p_msg)
{
	char *pc_ip = NULL;
	uint16 port = 0;
	uint32 eid = 0;
	bool rsp_ok = false;

	if(!ha_UnpackMsgId(p_msg, &pc_ip, &port)){
		ha_Warning("wrong msg");
		rsp_ok = false;
		goto send_rsp;
	}

	eid = ha_FindEidByIpPort(p_env, pc_ip, port);
	if(eid != HA_EID_INVALID){
		ha_DebugSelect("found Eid %d for tmp site", eid);
		rsp_ok = true;
	}else{
		ha_Error("unknown site: %s:%d", pc_ip, port);
		rsp_ok = false;
	}

send_rsp:
	if(ha_SendMsgIdRsp(p_tmp_conn->p_tcp_conn->socket, rsp_ok)){
		/* move tmp conn to site */
		p_tmp_conn->eid = eid;
		ha_MoveTmpConnToSite(p_env, p_tmp_conn);
	}else{
		ha_Warning("Send msg failed");

		ha_SetTmpConnFailed(p_tmp_conn);
	}

	if(pc_ip != NULL){
		ha_free(pc_ip);
	}

	return;
}

void ha_ProcMsgIdRsp(ha_env_t *p_env, ha_conn_list_t *p_tmp_conn,
	char *p_msg)
{
	bool rsp_ok;
	bool ret;

	ret = ha_UnpackMsgIdRsp(p_msg, &rsp_ok);
	if((!ret) || (!rsp_ok)){
		ha_Warning("Got msg id rsp[failed].");
		ha_SetTmpConnFailed(p_tmp_conn);
		return;
	}

	ha_MoveTmpConnToSite(p_env, p_tmp_conn);

	return;
}

void ha_TmpConnProcMsg(ha_env_t *p_env, char *p_msg,
	ha_conn_list_t *p_tmp_conn)
{
	switch(ha_MsgType(p_msg)){
		case HA_MSG_ID:
			ha_ProcMsgId(p_env, p_tmp_conn, p_msg);
			break;
		case HA_MSG_ID_RSP:
			ha_ProcMsgIdRsp(p_env, p_tmp_conn, p_msg);
			break;
		default:
			ha_Warning("tmp conn got unkown Msg[type: %d]", ha_MsgType(p_msg));

			ha_DumpData((const unsigned char *)p_msg, ha_MsgTotalLen(p_msg),
				(const unsigned char *)"Msg", true);
			break;
	}

	return;
}
/* *****************************************************************************
* 						ha env process msg functions end
***************************************************************************** */

















/* *****************************************************************************
* 						ha select functions start
***************************************************************************** */
static void ha_SelectInitMasks(ha_env_t *p_env, int *p_max_fd,
	fd_set *p_reads, fd_set *p_writes)
{
	FD_ZERO(p_reads);
	FD_ZERO(p_writes);

	/* set listen fd */
	for(uint32 i = 0; i < HA_MAX_LISTEN; i++){
		pgsocket listen_sock = p_env->listen_socket[i];
		int listen_sock_int = ha_Socket2Int(listen_sock);

		ha_Assert(!ha_SocketIsInvalid(listen_sock));

		ha_SetNoBlock(listen_sock);

		FD_SET(listen_sock_int, p_reads);
		if(listen_sock_int > *p_max_fd){
			*p_max_fd = listen_sock_int;
		}
	}

	/* set tmp conns fd */
	ha_conn_list_t *p_tmp_conn = p_env->p_tmp_conns;
	while(p_tmp_conn != NULL){
		pgsocket tmp_conn_sock = p_tmp_conn->p_tcp_conn->socket;
		int tmp_conn_sock_int = ha_Socket2Int(tmp_conn_sock);

		if(ha_TmpConnEstablished(p_tmp_conn)){
			ha_SetNoBlock(tmp_conn_sock);

			FD_SET(tmp_conn_sock_int, p_reads);
			if(tmp_conn_sock_int > *p_max_fd){
				*p_max_fd = tmp_conn_sock_int;
			}
		}else if(ha_TmpConnConnecting(p_tmp_conn)){
			ha_SetNoBlock(tmp_conn_sock);

			FD_SET(tmp_conn_sock_int, p_writes);
			if(tmp_conn_sock_int > *p_max_fd){
				*p_max_fd = tmp_conn_sock_int;
			}
		}

		p_tmp_conn = p_tmp_conn->p_next;
	}

	/* set all site conns fd */
	ha_Lock(ha_Lock4Env(p_env));
	for(uint32 eid = 0; eid <= ha_MaxEid(p_env); eid++){
		ha_site_t *p_site;

		/* skip my site & not used site */
		if(!ha_IsOtherEid(p_env, eid)){
			ha_DebugSelect("skip site: eid[%d]", eid);
			continue;
		}

		p_site = ha_SiteByEid(p_env, eid);
		ha_Assert(p_site != NULL);

		if(!ha_SiteConnAbnormal(p_site)){
			pgsocket conn_sock = p_site->conn.tcp_conn.socket;
			int conn_sock_int = ha_Socket2Int(conn_sock);

			ha_SetNoBlock(conn_sock);

			FD_SET(conn_sock_int, p_reads);
			if(conn_sock_int > *p_max_fd){
				*p_max_fd = conn_sock_int;
			}
		}
	}
	ha_Unlock(ha_Lock4Env(p_env));

	return;
}

static void ha_SelectProcEvent(ha_env_t *p_env, fd_set *p_reads,
	fd_set *p_writes)
{
	/* check sites conn */
	for(uint32 eid = 0; eid <= ha_MaxEid(p_env); eid++){
		ha_site_t *p_site;

		/* skip site */
		if(!ha_IsOtherEid(p_env, eid)){
			ha_DebugSelect("Skip site: eid[%d]", eid);
			continue;
		}

		p_site = ha_SiteByEid(p_env, eid);
		ha_Assert(p_site != NULL);

		ha_Lock(ha_Lock4Env(p_env));
		if(ha_SiteConnAbnormal(p_site)){
			ha_Unlock(ha_Lock4Env(p_env));
		}else{
			pgsocket conn_sock = p_site->conn.tcp_conn.socket;
			int conn_sock_int = ha_Socket2Int(conn_sock);

			ha_Unlock(ha_Lock4Env(p_env));

			if(FD_ISSET(conn_sock_int, p_reads)){
				char *p_msg;
				int ret = ha_RecvMsg(conn_sock, &p_msg);

				if(ret == STATUS_OK){
					if (p_msg != NULL)
					{
						ha_SiteConnProcMsg(p_env, p_msg, eid);
						ha_free(p_msg);
					}
				}else{
					ha_Lock(ha_Lock4Env(p_env));
					ha_SetSiteConnFailed(p_env, p_site);
					ha_Unlock(ha_Lock4Env(p_env));
				}
			}
		}
	}

	/* process event tmp conn */
	ha_conn_list_t *p_tmp_conn = p_env->p_tmp_conns;
	while(p_tmp_conn != NULL){
		ha_conn_list_t *p_tmp_conn_next = p_tmp_conn->p_next;
		pgsocket tmp_conn_sock = p_tmp_conn->p_tcp_conn->socket;
		int tmp_conn_sock_int = ha_Socket2Int(tmp_conn_sock);
		char *p_msg;
		int ret;

		if(ha_TmpConnEstablished(p_tmp_conn)){
			if(FD_ISSET(tmp_conn_sock_int, p_reads)){
				ret = ha_RecvMsg(tmp_conn_sock, &p_msg);

				if(ret != STATUS_OK){
					ha_Warning("Recv failed");
					ha_SetTmpConnFailed(p_tmp_conn);
				}else{
					if (p_msg != NULL)
					{
						ha_TmpConnProcMsg(p_env, p_msg, p_tmp_conn);
						ha_free(p_msg);
					}
				}
			}
		}else if(ha_TmpConnConnecting(p_tmp_conn)){
			if(FD_ISSET(tmp_conn_sock_int, p_writes)){
				if(ha_IsConnectionEstablished(tmp_conn_sock)){
					ha_SetTmpConnEstablished(p_tmp_conn);
					ha_TmpConnEstablishedDo(p_env, p_tmp_conn);
				}else{
					ha_SetTmpConnFailed(p_tmp_conn);
				}
			}
		}

		p_tmp_conn = p_tmp_conn_next;
	}

	/* process event on listen fd */
	for(uint32 i = 0; i < HA_MAX_LISTEN; i++){
		pgsocket listen_sock = p_env->listen_socket[i];
		int listen_sock_int = ha_Socket2Int(listen_sock);

		ha_Assert(!ha_SocketIsInvalid(listen_sock));

		if(FD_ISSET(listen_sock_int, p_reads)){
			ha_AcceptTmpConn(p_env, listen_sock);
		}
	}

	return;
}

uint32 ha_ProcAbnormalConns(ha_env_t *p_env, struct timeval now,
	uint32 min_timeout)
{
	uint32 timeout = 0;

	/* process abnormal tmp conns */
	ha_conn_list_t *p_tmp_conn = p_env->p_tmp_conns;
	while(p_tmp_conn != NULL){
		ha_conn_list_t *p_tmp_conn_next = p_tmp_conn->p_next;

		if(ha_TmpConnAbnormal(p_tmp_conn)
		|| ha_TmpConnExpireByTimev(p_tmp_conn, now)){
			/* tmp conn should be closed */
			ha_DebugSelect("tmp connection abnormal, should be closed");
			ha_ProcAbnormalOrExpireTmpConn(p_env, p_tmp_conn);
		}else{
			timeout = ha_TimevalDiff(p_tmp_conn->conn_invalid_timev, now);
			ha_DebugSelect("tmp connection left time %d.", timeout);
			if(min_timeout == 0){
				min_timeout = timeout;
			}else if((timeout != 0) && (timeout < min_timeout)){
				min_timeout = timeout;
			}
		}

		p_tmp_conn = p_tmp_conn_next;
	}

	/* process abnormal site conns */
	for(uint32 eid = 0; eid <= ha_MaxEid(p_env); eid++){
		ha_site_t *p_site;
		bool abnormal;

		if(!ha_IsOtherEid(p_env, eid)){
			ha_DebugSelect("skip site: eid[%d]", eid);
			continue;
		}

		p_site = ha_SiteByEid(p_env, eid);
		ha_Assert(p_site != NULL);

		ha_Lock(ha_Lock4Env(p_env));
		abnormal = ha_SiteConnAbnormal(p_site);
		ha_Unlock(ha_Lock4Env(p_env));

		if(abnormal){
			ha_DebugSelect("connection to site: eid[%d] abnormal", eid);

			timeout = ha_ProcAbnormalSiteConn(p_env, p_site, now);
			if(min_timeout == 0){
				min_timeout = timeout;
			}else if((timeout != 0) && (timeout < min_timeout)){
				min_timeout = timeout;
			}
		}	
	}

	return min_timeout;
}

uint32 ha_CheckBcastNewMaster(ha_env_t *p_env, struct timeval now,
	uint32 min_timeout)
{
	uint32 timeout = 0;
	bool i_am_master = false;

	ha_Lock(ha_Lock4Event(p_env));
	if(p_env->event.master_eid == ha_MyEid(p_env)){
		i_am_master = true;
		timeout = ha_TimevalDiff(p_env->event.next_bcast_timev, now);
	}
	ha_Unlock(ha_Lock4Event(p_env));

	if(i_am_master){
		if(timeout == 0){
			ha_DebugSelect("Need bcast master.");
			ha_BcastNewMaster(p_env);

			timeout = ha_HbTimeInterval(p_env);
		}

		ha_DebugSelect("%uus before bcast master", timeout);
	}else{
		ha_DebugSelect("I'm not master, do nothing.");
	}

	if(min_timeout == 0){
		min_timeout = timeout;
	}else if((timeout != 0) && (timeout < min_timeout)){
		min_timeout = timeout;
	}

	return min_timeout;
}

bool ha_Ready4Election(ha_env_t *p_env, uint32 *p_timeout)
{
	static bool lsn_ready = false;
	bool ready = true;
	uint32 timeout = 0;

	/* lsn ready */
	if(!lsn_ready){
		ha_lsn_t lsn = GetWalRcvWriteRecPtr(NULL, NULL);

		if((lsn.xlogid != 0) || (lsn.xrecoff != 0)){
			ha_Log("***Ready for election.");
			lsn_ready = true;
		}
	}

	if(!lsn_ready){
		ready = false;
		goto out;
	}

	ha_Lock(ha_Lock4Event(p_env));
	if(p_env->event.need_manual_start){
		if(!p_env->event.got_start_cmd){
			ready = false;
		}
	}
	if(p_env->event.election_delay){
		struct timeval now;

		gettimeofday(&now, NULL);
		if(now.tv_sec < p_env->event.delay_to_s){
			time_t dvalue = (p_env->event.delay_to_s - now.tv_sec);

			ready = false;

			if(dvalue <= (HA_MAX_TIME_US / 1000000)){
				uint32 dvalue_us = (uint32)(dvalue * 1000000);

				if((timeout == 0) || (timeout > dvalue_us)){
					timeout = dvalue_us;
				}
			}
		}
	}
	ha_Unlock(ha_Lock4Event(p_env));

out:
	*p_timeout = timeout;
	return ready;
}

uint32 ha_CheckElection(ha_env_t *p_env, struct timeval now,
	uint32 min_timeout)
{
	uint32 timeout = 0;

	ha_Lock(ha_Lock4Event(p_env));
	timeout = ha_TimevalDiff(p_env->event.timev_hb_d, now);
	ha_Unlock(ha_Lock4Event(p_env));

	// TODO:
	if(timeout == 0){
		uint32 timeout1 = 0;

		if((!ha_ElectionThreadExist()) && (ha_Ready4Election(p_env, &timeout1))){
			ha_Log("Need start election");
			fxdb_election_forkexec((void *)p_env);
			RequestShutDownWalRcv();
		}else{
			ha_DebugSelect("Election thread already exist");
		}

		/* to avoid master elected, but select timeout too long, not bcastcast new master in time */
		timeout = ha_HbTimeInterval(p_env);

		/* set timeout again */
		if((timeout1 != 0) && (timeout1 < timeout)){
			timeout = timeout1;
		}
	}else{
		ha_DebugSelect("%uus before start election", timeout);
	}

	if(min_timeout == 0){
		min_timeout = timeout;
	}else if((timeout != 0) && (timeout < min_timeout)){
		min_timeout = timeout;
	}

	return min_timeout;
}

void *ha_SelectLoop(void *p_arg)
{
	ha_env_t *p_env = (ha_env_t *)p_arg;

#define HA_SELECT_DEFAULT_TIMEOUT_US 20000000
	int max_fd = 0;
	fd_set reads;
	fd_set writes;
	struct timeval timev;
	struct timeval *p_timev = NULL;
	int ret = 0;
	uint32 nloop = 0; // just for debug
	struct timeval now;
	uint32 min_timeout;

	ha_Log("Select start......");

	while(!ha_SelectThreadNeedStop()){
		ha_DebugSelect("Loop[%d] start...", nloop++);

		min_timeout = 0;
		gettimeofday(&now, NULL);

		//ha_Lock(ha_Lock4Env(p_env));

		/* process all abnormal site conns, and all abnormal or expire tmp conns */
		min_timeout = ha_ProcAbnormalConns(p_env, now, min_timeout);

		/* check need bcast new master */
		min_timeout = ha_CheckBcastNewMaster(p_env, now, min_timeout);

		/* check need election */
		min_timeout = ha_CheckElection(p_env, now, min_timeout);

		/* init select mask */
		ha_SelectInitMasks(p_env, &max_fd, &reads, &writes);

		//ha_Unlock(ha_Lock4Env(p_env));

		/* set timeout */
		if(min_timeout == 0){
			min_timeout = HA_SELECT_DEFAULT_TIMEOUT_US;
		}else if(min_timeout > HA_SELECT_DEFAULT_TIMEOUT_US){
			min_timeout = HA_SELECT_DEFAULT_TIMEOUT_US;
		}else if(min_timeout < HA_MIN_TIME_US){
			min_timeout = HA_MIN_TIME_US;
		}
		if(min_timeout > 0){
			timev.tv_sec = min_timeout / 1000000;
			timev.tv_usec = min_timeout % 1000000;

			p_timev = &timev;
		}else{
			p_timev = NULL;
		}

		ha_DebugSelect("Before select, timeout is %u", min_timeout);
		ret = select(max_fd + 1, &reads, &writes, NULL, p_timev);
		if(ret < 0){
#ifndef HA_SIMULATE_ENV
			ha_DebugSelect("select ret %d, errno is %d[%s]",
				ret, errno, (errno == EBADFD) ? "bad fd" : "uknown");
#endif
			if(errno == EINTR && errno == EWOULDBLOCK){
				continue;
			}else{
				ha_Error("select() failed in ha: %m");
			}
		}else if(ret > 0){
			ha_DebugSelect("Select got %d events", ret);

			ha_SelectProcEvent(p_env, &reads, &writes);
		}else{
			ha_DebugSelect("select timeout, now do nothing,"
					" because we do these before the last loop start");
		}

		ha_DebugSelect("Loop[%d] end...", nloop - 1);

#ifndef HA_SIMULATE_ENV
		pg_process_signals();
#endif
	}

	ha_Log("HA select thread exit.");

#ifndef HA_SIMULATE_ENV
	proc_exit(0);
#endif

	return NULL;
}
/* *****************************************************************************
* 						ha select functions end
***************************************************************************** */

