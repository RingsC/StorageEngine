#ifndef __HA_NET_H__
#define __HA_NET_H__










/* *****************************************************************************
* 							addr functions start
***************************************************************************** */
typedef struct ha_myaddr{
	struct ha_myaddr *p_next;
	size_t addrlen;
	struct sockaddr *p_addr;
}ha_myaddr_t;

typedef struct ha_site_conf{
	//struct ha_site_conf *p_next;
	uint16 eid;
	uint16 port;
	char *pc_ip;
	struct addrinfo *p_ai;
}ha_site_conf_t;

extern ha_myaddr_t *ha_GetAllMyAddrs(void);
extern void ha_FreeAllMyAddrs(ha_myaddr_t *p_myaddrs);
extern bool ha_IsSiteMine(const ha_site_conf_t *p_site,
	const ha_myaddr_t *p_myaddrs, uint16 my_port);

extern bool ha_IpEqual(const struct sockaddr *p_sa1, size_t sa1_len,
	const struct sockaddr *p_sa2, size_t sa2_len);

extern struct addrinfo *ha_BuildAddrinfo(const char *pc_ip, uint16 port);
extern void ha_FreeAddrinfo(struct addrinfo *p_ai);
/* *****************************************************************************
* 							addr functions end
***************************************************************************** */













/* *****************************************************************************
* 							msg struct & functions start
***************************************************************************** */
/*
 * Description:
 * 	Msg type
 * 
 */
enum ha_msg_type{
	HA_MSG_ID,					/* tell server my identity, when first conn */
	HA_MSG_ID_RSP,				/* rsp for HA_MSG_ID */
	HA_MSG_LEI,					/* bcast my local election info */
	HA_MSG_VOTE,				/* vote */
	HA_MSG_NEW_MASTER,			/* master bcast */
	HA_MSG_TWO_MASTERS,			/* two masters found */
};

/*
 * Description:
 * 	Msg header(is common, for all msg type)
 * 
 * first 4 bytes is total length(include header)
 * second 4bytes is msg type
 */
#define HA_MSGH_OFFSET_TOTALLEN 0
#define HA_MSGH_SIZE_TOTALLEN 4

#define HA_MSGH_OFFSET_TYPE (HA_MSGH_OFFSET_TOTALLEN + HA_MSGH_SIZE_TOTALLEN)
#define HA_MSGH_SIZE_TYPE 4

#define HA_MSGH_SIZE (HA_MSGH_SIZE_TOTALLEN + HA_MSGH_SIZE_TYPE)

extern uint32 ha_MsgTotalLen(const char *p_msg);
extern ha_msg_type ha_MsgType(const char *p_msg);

extern void ha_DumpData(const unsigned char *p_data, unsigned int length,
    const unsigned char *pName, bool dump);

/*
 * Description:
 * 	All Msg pack & unpack
 * 
 */
extern char *ha_PackMsgId(const char *pc_ip, uint16 port);
extern bool ha_UnpackMsgId(const char *p_msg, char **pp_ip, uint16 *p_port);

extern char *ha_PackMsgIdRsp(bool rsp_ok);
extern bool ha_UnpackMsgIdRsp(const char *p_msg, bool *p_rsp_ok);

extern char *ha_PackMsgLei(uint32 priority, const ha_lsn_t *p_lsn,
			const char *pc_tiebreaker);
extern bool ha_UnpackMsgLei(const char *p_msg, uint32 *p_priority,
	ha_lsn_t *p_lsn, char *pp_tiebreaker);

extern char *ha_PackMsgVote(void);
extern bool ha_UnpackMsgVote(const char *p_msg);

extern char *ha_PackMsgNewMaster(const char *pc_ip, uint16 port);
extern bool ha_UnpackMsgNewMaster(const char *p_msg, char **pp_ip,
	uint16 *p_port);

extern char *ha_PackMsgTwoMasters(void);
extern bool ha_UnpackMsgTwoMasters(const char *p_msg);
/* *****************************************************************************
* 							msg struct & functions start
***************************************************************************** */











/* *****************************************************************************
* 						basic socket functions start
***************************************************************************** */
enum ha_tcp_conn_state{
	HA_TCONN_STATE_OK,
	HA_TCONN_STATE_CONNING,
	HA_TCONN_STATE_ESTABLISHED,
	HA_TCONN_STATE_FAILED,
	HA_TCONN_STATE_MAX,
};

typedef struct ha_tcp_conn{
	//struct ha_tcp_conn *p_next;
	ha_tcp_conn_state state;
	//uint32 other_eid;
	pgsocket socket;
}ha_tcp_conn_t;

#ifdef WIN32
#define initsocketenv() \
do{ \
	WSADATA	__wsaData; \
	if(WSAStartup(MAKEWORD(1, 1), &__wsaData)){ \
		fprintf(stderr, "WSAStartup failed\n"); \
		return -1; \
	} \
	WSASetLastError(0); \
}while(0)
#else
#define initsocketenv()
#endif

#define ha_Socket2Int(sock) ((int)(sock))
#define ha_SocketIsInvalid(sock) (ha_Socket2Int(sock) == PGINVALID_SOCKET)
#define ha_SetSocketInvalid(sock) sock = (pgsocket)PGINVALID_SOCKET

#define ha_SetBlock(sock) pg_set_block(sock)
#define ha_SetNoBlock(sock) pg_set_noblock(sock)

extern bool ha_SetKeepAlive(pgsocket sock, int idle_time_s,
	int interval_time_s, int count);
extern bool ha_SetSendTimeout(pgsocket sock, int time_s);
extern bool ha_SetRecvTimeout(pgsocket sock, int time_s);
extern bool ha_SetNoDelay(pgsocket sock);

#define ha_CloseSocket(sock) closesocket(sock)

extern int ha_Listen(ha_site_conf_t *p_mysite, uint32 max_listen,
	pgsocket *p_listen_socket);
extern ha_tcp_conn_t *ha_Connect(const ha_site_conf_t *p_mysite,
	const ha_site_conf_t *p_site, uint32 send_timeout_s, uint32 recv_timeout_s);
extern ha_tcp_conn_t *ha_ConnectNoBlock(const ha_site_conf_t *p_mysite,
	const ha_site_conf_t *p_site);
extern bool ha_IsConnectionEstablished(pgsocket sock);
extern ha_tcp_conn_t *ha_Accept(pgsocket listen_socket,
			const ha_site_conf_t *p_my_site);
extern int ha_SendMsg(pgsocket socket, const char *p_msg);
extern int ha_RecvMsg(pgsocket socket, char **pp_msg);
/* *****************************************************************************
* 						basic socket functions end
***************************************************************************** */

#endif

