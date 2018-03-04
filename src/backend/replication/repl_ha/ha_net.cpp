/* *****************************************************************************
* 							basic socket functions start
***************************************************************************** */
#ifdef WIN32
#include <Ws2tcpip.h>
#include <mstcpip.h>
#include <winsock.h> 
#include <Windows.h>

static inline SOCKET sock_socket(int af, int type, int protocol)
{
	return socket(af, type, protocol);
}

static inline SOCKET sock_accept(SOCKET s, struct sockaddr * addr, int *addrlen)
{
	return accept(s, addr, addrlen);
}

static inline int sock_connect(SOCKET s, const struct sockaddr * addr, int addrlen)
{
	return connect(s, addr, addrlen);
}

static inline int sock_recv(SOCKET s, char *buf, int len, int f)
{
	return recv(s, buf, len, f);
}

static inline int sock_send(SOCKET s, const void *buf, int len, int flags)
{
	return send(s, (const char *)buf, len, flags);
}
#else
#define sock_socket(af, type, protocol) socket(af, type, protocol)
#define sock_accept(s, addr, addrlen) accept(s, addr, addrlen)
#define sock_connect(s, name, namelen) connect(s, name, namelen)
#define sock_recv(s, buf, len, flags) recv(s, buf, len, flags)
#define sock_send(s, buf, len, flags) send(s, buf, len, flags)
#endif
/* *****************************************************************************
* 							basic socket functions end
***************************************************************************** */



#include "ha_head.h"









/* *****************************************************************************
* 							addr functions start
***************************************************************************** */
static void ha_DumpAddrinfo(struct addrinfo *p_addrs)
{
#ifdef _TODO_HA_DEBUG_NET
	struct addrinfo *p_addr = p_addrs;
	uint32 i = 0;

	ha_Printf("\n\n==============================================\n");
	while(p_addr != NULL){
		ha_Printf("ai[%d]\n", i);
		ha_Printf("flags: 0x%x   ", p_addr->ai_flags);
		ha_Printf("family: %d   ", p_addr->ai_family);
		ha_Printf("socketype: %d   ", p_addr->ai_socktype);
		ha_Printf("protocol: %d   ", p_addr->ai_protocol);
		ha_Printf("addr_len: %d   ", p_addr->ai_addrlen);
		//ereport(LOG, (errmsg("cannoname: %s", p_addr->ai_canonname)));
		ha_Printf("addr: family %d, data ", p_addr->ai_addr->sa_family);
		for(uint32 j = 0; j < p_addr->ai_addrlen - 2; j++){
			ha_Printf("0x%x ", (uint8)p_addr->ai_addr->sa_data[j]);
		}
		ha_Printf("next: %p", p_addr->ai_next);
		ha_Printf("\n\n");

		p_addr = p_addr->ai_next;
		i++;
	}
	ha_Printf("==============================================\n\n");
#endif
	return;
}

static void ha_DumpMyAddrs(ha_myaddr_t *p_addrs)
{
#ifdef _TODO_HA_DEBUG_NET
	ha_myaddr_t *p_addr = p_addrs;
	uint32 i = 0;

	ha_Printf("\n\n==============================================\n");
	while(p_addr != NULL){
		ha_Printf("myaddr[%d]\n", i);
		ha_Printf("family: %d   ", p_addr->p_addr->sa_family);
		ha_Printf("addr_len: %zd   ", p_addr->addrlen);
		for(uint32 j = 0; j < p_addr->addrlen - 2; j++){
			ha_Printf("0x%x ", (uint8)p_addr->p_addr->sa_data[j]);
		}
		ha_Printf("next: %p", p_addr->p_next);
		ha_Printf("\n\n");

		p_addr = p_addr->p_next;
		i++;
	}
	ha_Printf("==============================================\n\n");
#endif

	return;
}

void ha_FreeAllMyAddrs(ha_myaddr_t *p_myaddrs)
{
	ha_myaddr_t *p_myaddr = p_myaddrs;
	ha_myaddr_t *p_next_myaddr = NULL;

	while(p_myaddr != NULL){
		p_next_myaddr = p_myaddr->p_next;

		free(p_myaddr->p_addr);
		free(p_myaddr);

		p_myaddr = p_next_myaddr;
	}

	return;
}

static ha_myaddr_t *ha_GetAllMyAddrsByAddrinfo(void)
{
	int ret = 0;
	ha_myaddr_t *p_myaddrs = NULL;
	ha_myaddr_t *p_myaddr = NULL;
	char hostname[256];
	struct addrinfo *p_ais = NULL;
	struct addrinfo *p_ai = NULL;
	struct addrinfo hint;

	/* get host name */	
	memset(hostname, 0, 256);
	if(gethostname(hostname, 256) != 0){
		ha_DebugNet("Get hostname failed.");
		return p_myaddr;
	}

	ha_DebugNet("hostname: <%s>", hostname);

	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	/* libpq get addr info */
	ret = pg_getaddrinfo_all(hostname, NULL, &hint, &p_ais);
	if((ret != 0) || (p_ais == NULL)){
		ha_DebugNet("Get addr info failed.");
		if(p_ais){
			pg_freeaddrinfo_all(hint.ai_family, p_ais);
		}
		return p_myaddr;
	}

	p_ai = p_ais;
	while(p_ai != NULL){
		size_t addrlen = 0;
		ha_myaddr_t *p_new_myaddr = NULL;
		struct sockaddr *p_new_sockaddr = NULL;

		if((p_ai->ai_family != AF_INET) && (p_ai->ai_family != AF_INET6)){
			ha_DebugNet("ai family: %d", p_ai->ai_family);
			p_ai = p_ai->ai_next;
			continue;
		}

		addrlen = p_ai->ai_addrlen;

		p_new_myaddr = (ha_myaddr_t *)malloc(sizeof(ha_myaddr_t));
		p_new_sockaddr = (struct sockaddr *)malloc(addrlen);
		if((p_new_myaddr == NULL) || (p_new_sockaddr == NULL)){
			pg_freeaddrinfo_all(hint.ai_family, p_ais);
			ha_FreeAllMyAddrs(p_myaddrs);
			p_myaddrs = NULL;
			return p_myaddrs;
		}

		memcpy((char *)p_new_sockaddr, (char *)p_ai->ai_addr, addrlen);
		p_new_myaddr->p_addr = p_new_sockaddr;
		p_new_myaddr->addrlen = p_ai->ai_addrlen;
		p_new_myaddr->p_next = NULL;

		if(p_myaddr == NULL){
			p_myaddrs = p_new_myaddr;
			p_myaddr = p_new_myaddr;
		}else{
			p_myaddr->p_next = p_new_myaddr;
			p_myaddr = p_myaddr->p_next;
		}

		p_ai = p_ai->ai_next;
	}

	pg_freeaddrinfo_all(hint.ai_family, p_ais);

	/* just for debug */
	ha_DumpMyAddrs(p_myaddrs);

	return p_myaddrs;
}

static ha_myaddr_t *ha_GetAllMyAddrsByIfaddrs(void)
{
#ifndef WIN32
	int ret = 0;
	ha_myaddr_t *p_myaddrs = NULL;
	ha_myaddr_t *p_myaddr = NULL;
	struct ifaddrs *p_ifas = NULL;
	struct ifaddrs *p_ifa = NULL;

	ret = getifaddrs(&p_ifas);
	if((ret != 0) || (p_ifas == NULL)){
		ha_DebugNet("Get if addrs failed.");
		if(p_ifas != NULL){
			freeifaddrs(p_ifas);
		}
		return p_myaddrs;
	}

	p_ifa = p_ifas;
	while(p_ifa != NULL){
		size_t addrlen = 0;
		ha_myaddr_t *p_new_myaddr = NULL;
		struct sockaddr *p_new_sockaddr = NULL;

		if(p_ifa->ifa_addr->sa_family == AF_INET){
			addrlen = sizeof(struct sockaddr_in);
		}else if(p_ifa->ifa_addr->sa_family == AF_INET6){
			addrlen = sizeof(struct sockaddr_in6);
		}else{
			if(p_ifa->ifa_addr->sa_family != AF_PACKET){
				ha_DebugNet("Device %s, family: %d",
						p_ifa->ifa_name, p_ifa->ifa_addr->sa_family);
			}
			p_ifa = p_ifa->ifa_next;
			continue;
		}

		p_new_myaddr = (ha_myaddr_t *)malloc(sizeof(ha_myaddr_t));
		p_new_sockaddr = (struct sockaddr *)malloc(addrlen);
		if((p_new_myaddr == NULL) || (p_new_sockaddr == NULL)){
			freeifaddrs(p_ifas);
			ha_FreeAllMyAddrs(p_myaddrs);
			p_myaddrs = NULL;
			return NULL;
		}

		memcpy((char *)p_new_sockaddr, (char *)p_ifa->ifa_addr, addrlen);
		p_new_myaddr->p_addr = p_new_sockaddr;
		p_new_myaddr->addrlen = addrlen;
		p_new_myaddr->p_next = NULL;

		if(p_myaddr == NULL){
			p_myaddrs = p_new_myaddr;
			p_myaddr = p_new_myaddr;
		}else{
			p_myaddr->p_next = p_new_myaddr;
			p_myaddr = p_myaddr->p_next;
		}

		p_ifa = p_ifa->ifa_next;
	}

	freeifaddrs(p_ifas);

	/* just for debug */
	ha_DumpMyAddrs(p_myaddrs);

	return p_myaddrs;
#else
	return NULL;
#endif
}

ha_myaddr_t *ha_GetAllMyAddrs(void)
{
#ifdef WIN32
	return ha_GetAllMyAddrsByAddrinfo();
#else
	return ha_GetAllMyAddrsByIfaddrs();
#endif
}

struct addrinfo *ha_BuildAddrinfo(const char *pc_ip, uint16 port)
{
	struct addrinfo hint;
	struct addrinfo *p_ai = NULL;
	char c_port[8];
	char *pc_port = NULL;
	int ret = 0;

	ha_DebugNet("ip: <%s:%d>", pc_ip, port);

	memset(&hint, 0, sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	if(port != 0){
		snprintf(c_port, 8, "%d", port);
		pc_port = c_port;
	}else{
		pc_port = NULL;
	}

	ret = pg_getaddrinfo_all(pc_ip, pc_port, &hint, &p_ai);
	if(ret || !p_ai){
		ha_DebugNet("could not translate host \"%s\" to address: %s",
				pc_ip, gai_strerror(ret));
		if(p_ai){
			pg_freeaddrinfo_all(hint.ai_family, p_ai);
		}

		return NULL;
	}

	//ha_DumpAddrinfo(p_ai);

	return p_ai;
}

void ha_FreeAddrinfo(struct addrinfo *p_ai)
{
	pg_freeaddrinfo_all(AF_UNSPEC, p_ai);

	return;
}

bool ha_IpEqual(const struct sockaddr *p_sa1, size_t sa1_len,
	const struct sockaddr *p_sa2, size_t sa2_len)
{
	if(p_sa1->sa_family == p_sa2->sa_family){
		const char *p_comp1 = NULL;
		const char *p_comp2 = NULL;
		size_t len = 0;

		if(p_sa1->sa_family == AF_INET){
			const struct sockaddr_in *p_sa1_sin = (const struct sockaddr_in *)(p_sa1);
			const struct sockaddr_in *p_sa2_sin = (const struct sockaddr_in *)(p_sa2);

			if((sa1_len != sa2_len) && (sa1_len != sizeof(struct sockaddr_in))){
				ha_DebugNet("sa1_len: %zd, sa2_len: %zd, not match %lu",
						sa1_len, sa2_len, sizeof(struct sockaddr_in));
				ha_Assert(false);
				return false;
			}

			p_comp1 = (char *)&p_sa1_sin->sin_addr;
			p_comp2 = (char *)&p_sa2_sin->sin_addr;
			len = sizeof(struct in_addr);
		}else if(p_sa1->sa_family == AF_INET6){
			const struct sockaddr_in6 *p_sa1_sin6 = (const struct sockaddr_in6 *)(p_sa1);
			const struct sockaddr_in6 *p_sa2_sin6 = (const struct sockaddr_in6 *)(p_sa2);

			if((sa1_len != sa2_len) && (sa1_len != sizeof(struct sockaddr_in6))){
				ha_DebugNet("sa1_len: %zd, sa2_len: %zd, not match %lu",
						sa1_len, sa2_len, sizeof(struct sockaddr_in6));
				ha_Assert(false);
				return false;
			}

			p_comp1 = (char *)&p_sa1_sin6->sin6_addr;
			p_comp2 = (char *)&p_sa2_sin6->sin6_addr;
			len = sizeof(struct in6_addr);
		}else{
			ha_Assert(false);
		}

		if(memcmp(p_comp1, p_comp2, len) == 0){
			ha_DebugNet("ip equal");
			return true;
		}
	}

	return false;
}

bool ha_IsSiteMine(const ha_site_conf_t *p_site,
	const ha_myaddr_t *p_myaddrs, uint16 my_port)
{
	static const char *ignore_ip[] =
		{
			"127.0.0.1"
		};

	const ha_myaddr_t *p_myaddr = NULL;
	struct addrinfo *p_ai = p_site->p_ai;

	/* first check port */
	if(p_site->port != my_port){
		ha_DebugNet("Site port is %d, not match my port: %d",
					p_site->port, my_port);
		return false;
	}

	/* check ip */
	/* ignore */
	for(int i = 0; i < (int)(sizeof(ignore_ip)/sizeof(const char *)); i++)
	{
		if(strcmp(p_site->pc_ip, ignore_ip[i]) == 0)
		{
			return true;
		}
	}
	/* compare */
	p_myaddr = p_myaddrs;
	while(p_myaddr != NULL){
		if(ha_IpEqual(p_myaddr->p_addr, p_myaddr->addrlen,
		p_ai->ai_addr, p_ai->ai_addrlen)){
			ha_DebugNet("Got it");
			return true;
		}

		p_myaddr = p_myaddr->p_next;
	}

	return false;
}
/* *****************************************************************************
* 							addr functions end
***************************************************************************** */














/* *****************************************************************************
* 							msg functions start
***************************************************************************** */
void ha_DumpData(const unsigned char *p_data, unsigned int length,
	const unsigned char *pName, bool dump)
{
	char line[1024];
	line[0] = '\0';

#define CHARS_PER_LINE 16

#define PRINT(f, ...)                     \
do                                        \
{                                         \
	char buf[128];                        \
	snprintf(buf, 128, f, ##__VA_ARGS__); \
	strcat(line, buf);                    \
} while (0);

#define END_PRINT()                   \
do                                    \
{                                     \
    ha_Printf("%s", line);            \
	line[0] = '\0';                   \
} while (0);
	
#define NORMAL_CHAR(a) (\
	(((a) >= 'a') && ((a) <= 'z')) /* little char */ \
	|| (((a) >= 'A') && ((a) <= 'Z')) /* capital char */ \
	|| (((a) >= '0') && ((a) <= '9')) /* numeral */ \
	/* the last are all frequently-used chars */ \
	|| ((a) == ':') || ((a) == '.') || ((a) == '-') \
	|| ((a) == '_') || ((a) == '=') \
	)

#define PRINT_CHAR(a) (NORMAL_CHAR(a) ? (a) : '.')

#define BOUNDARY_BETWEEN_TWO_BINARY "|"
#define LINE_START "|"
#define LINE_END "|\n"

#define BOUNDARY_BETWEEN_BINARY_AND_CHAR "\t\t|"

	unsigned int i = 0;
	const unsigned char *p = NULL;
	const unsigned char *p_line_s = NULL;
	const unsigned char *p_line_e = NULL;

	if(!dump){
		return;
	}

	PRINT("******************* DUMP (%s) START *********************\n", pName);
    END_PRINT();

	/* print title */
	for(i = 0; i < CHARS_PER_LINE; i++){
		PRINT("|%02d", i);
	}
	PRINT("|\n");
	END_PRINT()

	for(i = 0; i < CHARS_PER_LINE; i++){
		PRINT("---");
	}
	PRINT("-\n");
	END_PRINT();

	/* print all per line */
	for(i = 0, p = p_data; i < length; i++, p++){
		if(i % 16 == 0){
			p_line_s = p;

			/* print line start */
			PRINT("%s", LINE_START);
		}

		/* first print binary */
		PRINT("%02x%s", (*p), BOUNDARY_BETWEEN_TWO_BINARY);
		
		if(i % 16 == 15){
			const unsigned char *q = p_line_s;

			p_line_e = p;

			/* printf boundary between binary and char */
			PRINT("%s", BOUNDARY_BETWEEN_BINARY_AND_CHAR);

			/* need print all line chars */
			while(q != p_line_e){
				PRINT("%c", PRINT_CHAR(*q));
				q++;
			}

			/* print line end */
			PRINT("%s", LINE_END);
			END_PRINT();
		}
	}

	/* the last line is not a whole line */
	if(p_line_e != p_data + length - 1){
		const unsigned char *q = p_line_s;
		unsigned int last_line_len = (unsigned int)(p_data + length - p_line_s);

		/* fill ' ' with the last line binary */
		for(i = 0; i < CHARS_PER_LINE - last_line_len; i++){
			PRINT("%s", "   ");
		}

		/* printf boundary between binary and char */
		PRINT("%s", BOUNDARY_BETWEEN_BINARY_AND_CHAR);

		/* need print all line chars */
		while(q != p_data + length){
			PRINT("%c", PRINT_CHAR(*q));
			q++;
		}

		/* print line end */
		PRINT("%s", LINE_END);
		END_PRINT();
	}

	PRINT("******************** DUMP (%s) END **********************\n", pName);
	END_PRINT();

	return;
}

#define HA_MAX_MSG_LENGTH 1024
#define HA_MSG_BUFFER_LENGTH 8192

#define HA_MSG_IP_PFX "ip="
#define HA_MSG_PORT_PFX "port="
#define HA_MSG_LSN_PFX "lsn="

#define HA_MSG_PRIO_PFX "priority="
#define HA_MSG_TB_PFX "tiebreaker="

uint32 ha_MsgTotalLen(const char *p_msg)
{
	const uint32 *p_msg_len = (const uint32 *)(p_msg + HA_MSGH_OFFSET_TOTALLEN);
	return htonl((*p_msg_len));
}

ha_msg_type ha_MsgType(const char *p_msg)
{
	const uint32 *p_msg_type = (const uint32 *)(p_msg + HA_MSGH_OFFSET_TYPE);
	return (ha_msg_type)htonl(*p_msg_type);
}

void ha_FillMsgHeader(char *p_msg, uint32 msg_len, ha_msg_type msg_type)
{
	msg_len = ntohl(msg_len);
	msg_type = (ha_msg_type)ntohl((uint32)msg_type);

	memcpy(p_msg, (char *)&msg_len, HA_MSGH_SIZE_TOTALLEN);
	memcpy(p_msg + HA_MSGH_SIZE_TOTALLEN, (char *)&msg_type, HA_MSGH_SIZE_TYPE);
	return;
}

/* msg data is be, but msg header is host endian */
char *ha_PackMsgId(const char *pc_ip, uint16 port)
{
	char *p_data = (char *)ha_malloc(HA_MAX_MSG_LENGTH);
	uint32 offset = HA_MSGH_SIZE;
	char *p_str = NULL;

	if(p_data == NULL){
		ha_EreportOOM();
		return NULL;
	}

	p_str = p_data + offset;
	snprintf(p_str, HA_MAX_MSG_LENGTH - offset, "%s%s", HA_MSG_IP_PFX, pc_ip);
	offset += ((uint32)strlen(p_str) + 1);

	p_str = p_data + offset;
	snprintf(p_str, HA_MAX_MSG_LENGTH - offset, "%s%d", HA_MSG_PORT_PFX, port);
	offset += ((uint32)strlen(p_str) + 1);

	ha_FillMsgHeader(p_data, offset, HA_MSG_ID);

	return p_data;
}

bool ha_UnpackMsgId(const char *p_msg, char **pp_ip, uint16 *p_port)
{
	uint32 msg_len = ha_MsgTotalLen(p_msg);
	uint32 offset = HA_MSGH_SIZE;
	const char *p_str = NULL;
	uint32 str_len = 0;
	const char *pc_ip = NULL;
	uint32 ip_len = 0;
	uint16 port = 0;

	do{
		p_str = p_msg + offset;
		str_len = (uint32)strlen(p_str) + 1;

		if(memcmp(p_str, HA_MSG_IP_PFX, strlen(HA_MSG_IP_PFX)) == 0){
			pc_ip = p_str + strlen(HA_MSG_IP_PFX);
			ip_len = (uint32)strlen(pc_ip) + 1;
		}else if(memcmp(p_str, HA_MSG_PORT_PFX, strlen(HA_MSG_PORT_PFX)) == 0){
			port = atoi(p_str + strlen(HA_MSG_PORT_PFX));
		}else{
			ha_DebugNet("wrong msg");
			goto err;
		}

		offset += str_len;
		p_str = p_msg + offset;
	}while(offset < msg_len);

	if((pc_ip == NULL) || (ip_len <= 1)){
		ha_DebugNet("wrong ip");
		goto err;
	}

	if(port == 0){
		ha_DebugNet("wrong port");
		goto err;
	}

	if(msg_len != offset){
		ha_DebugNet("wrong msg, length not match");
		goto err;
	}

	*pp_ip = (char *)ha_malloc(ip_len);
	if(*pp_ip == NULL){
		ha_EreportOOM();
		goto err;
	}

	memcpy(*pp_ip, pc_ip, ip_len);
	*p_port = port;
	return true;

err:
	*pp_ip = NULL;
	return false;
}

char *ha_PackMsgIdRsp(bool rsp_ok)
{
	char *p_data = (char *)ha_malloc(HA_MAX_MSG_LENGTH);
	uint32 offset = HA_MSGH_SIZE;
	char *p_str = NULL;

	if(p_data == NULL){
		ha_EreportOOM();
		return NULL;
	}

	p_str = p_data + offset;
	snprintf(p_str, HA_MAX_MSG_LENGTH - offset, "%s", rsp_ok ? "OK" : "Fail");
	offset += ((uint32)strlen(p_str) + 1);

	ha_FillMsgHeader(p_data, offset, HA_MSG_ID_RSP);

	return p_data;
}

bool ha_UnpackMsgIdRsp(const char *p_msg, bool *p_rsp_ok)
{
	uint32 msg_len = ha_MsgTotalLen(p_msg);
	uint32 offset = HA_MSGH_SIZE;
	const char *p_str = NULL;
	uint32 str_len = 0;

	p_str = p_msg + offset;
	if(memcmp(p_str, "OK", strlen("OK")) == 0){
		str_len = ((uint32)strlen("OK") + 1);
		*p_rsp_ok = true;
	}else if(memcmp(p_str, "Fail", strlen("Fail")) == 0){
		str_len = ((uint32)strlen("Fail") + 1);
		*p_rsp_ok = false;
	}else{
		ha_DebugNet("wrong msg, length not match");
		return false;
	}

	offset += str_len;
	if(msg_len != offset){
		ha_DebugNet("wrong msg");
		return false;
	}

	return true;
}

uint32 ha_PackLsn(char *p_msg_lsn_s, uint32 buf_len, const ha_lsn_t *p_lsn)
{
	snprintf(p_msg_lsn_s, buf_len, "%sid:%d;offset:%d",
		HA_MSG_LSN_PFX, p_lsn->xlogid, p_lsn->xrecoff);

	return ((uint32)strlen(p_msg_lsn_s) + 1);
}

uint32 ha_UnpackLsn(const char *p_msg_lsn_s, ha_lsn_t *p_lsn)
{
	uint32 lsn_len = (uint32)strlen(p_msg_lsn_s);
	const char *p_str;
	uint32 str_len;
	uint32 offset = 0;
	const char *pc_id = NULL;
	const char *pc_offset = NULL;

	/* lsn prefix */
	p_str = p_msg_lsn_s + offset;
	str_len = (uint32)strlen(HA_MSG_LSN_PFX);
	if(memcmp(p_str, HA_MSG_LSN_PFX, str_len) != 0){
		return 0;
	}
	offset += str_len;

	/* lsn id */
	p_str = p_msg_lsn_s + offset;
	str_len = (uint32)strlen("id:");
	if(memcmp(p_str, "id:", str_len) != 0){
		return 0;
	}
	offset += str_len;
	pc_id = p_msg_lsn_s + offset;

	/* lsn offset */
	p_str = p_msg_lsn_s + offset;
	for(; offset < lsn_len; offset++){
		if(*(p_msg_lsn_s + offset) == ';'){
			p_str = p_msg_lsn_s + offset;
			str_len = (uint32)strlen(";offset:");
			if(memcmp(p_str, ";offset:", str_len) != 0){
				return 0;
			}
			offset += str_len;
			pc_offset = p_msg_lsn_s + offset;
			break;
		}
	}

	if((pc_offset == NULL) || (offset >= lsn_len)){
		return 0;
	}

	p_lsn->xlogid = atoi(pc_id);
	p_lsn->xrecoff = atoi(pc_offset);

	return lsn_len + 1;
}

char *ha_PackMsgLei(uint32 priority, const ha_lsn_t *p_lsn,
	const char *pc_tiebreaker)
{
	char *p_data = (char *)ha_malloc(HA_MAX_MSG_LENGTH);
	uint32 offset = HA_MSGH_SIZE;
	char *p_str = NULL;
	uint32 tmp_len = 0;

	if(p_data == NULL){
		ha_EreportOOM();
		return NULL;
	}

	/* lsn */
	p_str = p_data + offset;
	tmp_len = ha_PackLsn(p_str, HA_MAX_MSG_LENGTH - offset, p_lsn);
	offset += tmp_len;

	/* priority */
	p_str = p_data + offset;
	snprintf(p_str, HA_MAX_MSG_LENGTH - offset, "%s%d", HA_MSG_PRIO_PFX, priority);
	offset += ((uint32)strlen(p_str) + 1);

	/* tiebreaker */
	p_str = p_data + offset;
	snprintf(p_str, HA_MAX_MSG_LENGTH - offset, "%s%s", HA_MSG_TB_PFX, pc_tiebreaker);
	offset += ((uint32)strlen(p_str) + 1);

	ha_FillMsgHeader(p_data, offset, HA_MSG_LEI);

	return p_data;
}

bool ha_UnpackMsgLei(const char *p_msg, uint32 *p_priority,
	ha_lsn_t *p_lsn, char *p_tiebreaker)
{
	uint32 msg_len = ha_MsgTotalLen(p_msg);
	uint32 offset = HA_MSGH_SIZE;
	const char *p_str = NULL;
	uint32 str_len = 0;
	const char *pc_tiebreaker = NULL;
	uint32 tiebreaker_len = 0;

	/* get lsn */
	p_str = p_msg + offset;
	str_len = ha_UnpackLsn(p_str, p_lsn);
	if(str_len == 0){
		ha_DebugNet("wrong msg");
		goto err;
	}
	offset += str_len;

	do{
		p_str = p_msg + offset;
		str_len = (uint32)strlen(p_str) + 1;

		if(memcmp(p_str, HA_MSG_PRIO_PFX, strlen(HA_MSG_PRIO_PFX)) == 0){
			*p_priority = atoi(p_str + strlen(HA_MSG_PRIO_PFX));
		}else if(memcmp(p_str, HA_MSG_TB_PFX, strlen(HA_MSG_TB_PFX)) == 0){
			pc_tiebreaker = p_str + strlen(HA_MSG_TB_PFX);
			tiebreaker_len = (uint32)strlen(pc_tiebreaker) + 1;
		}else{
			ha_DebugNet("wrong msg");
			goto err;
		}

		offset += str_len;
		p_str = p_msg + offset;
	}while(offset < msg_len);

	if(msg_len != offset){
		ha_DebugNet("wrong msg, length not match");
		goto err;
	}

	memcpy(p_tiebreaker, pc_tiebreaker, tiebreaker_len);

	return true;
err:
	return false;
}

char *ha_PackMsgVote(void)
{
	char *p_data = (char *)ha_malloc(HA_MAX_MSG_LENGTH);
	uint32 offset = HA_MSGH_SIZE;

	if(p_data == NULL){
		ha_EreportOOM();
		return NULL;
	}

	ha_FillMsgHeader(p_data, offset, HA_MSG_VOTE);

	return p_data;
}

bool ha_UnpackMsgVote(const char *p_msg)
{
	uint32 msg_len = ha_MsgTotalLen(p_msg);
	uint32 offset = HA_MSGH_SIZE;

	if(msg_len != offset){
		ha_DebugNet("wrong msg");
		return false;
	}

	return true;
}

char *ha_PackMsgNewMaster(const char *pc_ip, uint16 port)
{
	char *p_data = (char *)ha_malloc(HA_MAX_MSG_LENGTH);
	uint32 offset = HA_MSGH_SIZE;
	char *p_str = NULL;

	if(p_data == NULL){
		ha_EreportOOM();
		return NULL;
	}

	p_str = p_data + offset;
	snprintf(p_str, HA_MAX_MSG_LENGTH - offset, "%s%s", HA_MSG_IP_PFX, pc_ip);
	offset += ((uint32)strlen(p_str) + 1);

	p_str = p_data + offset;
	snprintf(p_str, HA_MAX_MSG_LENGTH - offset, "%s%d", HA_MSG_PORT_PFX, port);
	offset += ((uint32)strlen(p_str) + 1);

	ha_FillMsgHeader(p_data, offset, HA_MSG_NEW_MASTER);

	return p_data;
}

bool ha_UnpackMsgNewMaster(const char *p_msg, char **pp_ip,
	uint16 *p_port)
{
	uint32 msg_len = ha_MsgTotalLen(p_msg);
	uint32 offset = HA_MSGH_SIZE;
	const char *p_str = NULL;
	uint32 str_len = 0;
	const char *pc_ip = NULL;
	uint32 ip_len = 0;
	uint16 port = 0;

	do{
		p_str = p_msg + offset;
		str_len = (uint32)strlen(p_str) + 1;

		if(memcmp(p_str, HA_MSG_IP_PFX, strlen(HA_MSG_IP_PFX)) == 0){
			pc_ip = p_str + strlen(HA_MSG_IP_PFX);
			ip_len = (uint32)strlen(pc_ip) + 1;
		}else if(memcmp(p_str, HA_MSG_PORT_PFX, strlen(HA_MSG_PORT_PFX)) == 0){
			port = atoi(p_str + strlen(HA_MSG_PORT_PFX));
		}else{
			ha_DebugNet("wrong msg");
			goto err;
		}

		offset += str_len;
		p_str = p_msg + offset;
	}while(offset < msg_len);

	if((pc_ip == NULL) || (ip_len <= 1)){
		ha_DebugNet("wrong ip");
		goto err;
	}

	if(port == 0){
		ha_DebugNet("wrong port");
		goto err;
	}

	if(msg_len != offset){
		ha_DebugNet("wrong msg, length not match");
		goto err;
	}

	*pp_ip = (char *)ha_malloc(ip_len);
	if(*pp_ip == NULL){
		ha_EreportOOM();
		goto err;
	}

	memcpy(*pp_ip, pc_ip, ip_len);
	*p_port = port;
	return true;

err:
	*pp_ip = NULL;
	return false;
}

char *ha_PackMsgTwoMasters(void)
{
	char *p_data = (char *)ha_malloc(HA_MAX_MSG_LENGTH);
	uint32 offset = HA_MSGH_SIZE;

	if(p_data == NULL){
		ha_EreportOOM();
		return NULL;
	}

	ha_FillMsgHeader(p_data, offset, HA_MSG_TWO_MASTERS);

	return p_data;
}

bool ha_UnpackMsgTwoMasters(const char *p_msg)
{
	uint32 msg_len = ha_MsgTotalLen(p_msg);
	uint32 offset = HA_MSGH_SIZE;

	if(msg_len != offset){
		ha_DebugNet("wrong msg");
		return false;
	}

	return true;
}
/* *****************************************************************************
* 							msg functions end
***************************************************************************** */















/* *****************************************************************************
* 							basic socket functions start
***************************************************************************** */
/* 
 * Description:
 * 	set keep alive
 *
 * idle		idle time(second) >= keep_idle, should probe
 * interval 	probe packet interval time(second)
 * count		count of probe packets, * if windows, this value not take effect
 */
bool ha_SetKeepAlive(pgsocket sock, int idle_time_s,
	int interval_time_s, int count)
{
#ifndef WIN32
	int keep_alive = 1;

	/* set keep alive */
	if(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
	(char *)(&keep_alive), sizeof(keep_alive)) != 0){
		ha_DebugNetWithErrno("Set sock keep alive failed.");
		return false;
	}

	/* set keep idle */
	if((idle_time_s > 0)
	&& (setsockopt(sock, SOL_TCP, TCP_KEEPIDLE, (char *)&idle_time_s,
	sizeof(idle_time_s)) != 0)){
		ha_DebugNetWithErrno("Set sock keep idle %ds failed.", idle_time_s);
		return false;
	}

	/* set keep interval */
	if((interval_time_s > 0)
	&& (setsockopt(sock, SOL_TCP, TCP_KEEPINTVL, (char *)&interval_time_s,
	sizeof(interval_time_s)) != 0)){
		ha_DebugNetWithErrno("Set sock keep interval %ds failed.", interval_time_s);
		return false;
	}

	/* set keep count */
	if((count > 0)
	&& (setsockopt(sock, SOL_TCP, TCP_KEEPCNT, (char *)&count,
	sizeof(count)) != 0)){
		ha_DebugNetWithErrno("Set sock keep count %ds failed.", count);
		return false;
	}
#else
	struct tcp_keepalive ka;
	DWORD retsize;

	#define DEFAULT_IDLE_TIME_WIN32 2 * 60 * 60 /* 2 hours */
	#define DEFAULT_INTERVAL_TIME_WIN32 1 /* 1 second */

	if (idle_time_s <= 0)
		idle_time_s = DEFAULT_IDLE_TIME_WIN32;

	if (interval_time_s <= 0)
		interval_time_s = DEFAULT_INTERVAL_TIME_WIN32;

	ka.onoff = 1;
	ka.keepalivetime = idle_time_s * 1000;
	ka.keepaliveinterval = interval_time_s * 1000;

	if (WSAIoctl(sock, SIO_KEEPALIVE_VALS, (LPVOID)&ka, sizeof(ka),
	NULL, 0, &retsize, NULL, NULL) != 0){
		ha_DebugNet("Windows set sock keep alive failed: %ui\n",
			WSAGetLastError());
		return false;
	}
#endif

	return true;
}

bool ha_SetSockTimeout(int opt, pgsocket sock, int time_s)
{
	void *p_timeout;
	int timeout_size;
	const char *p_str_opt;

	if(opt == SO_SNDTIMEO){
		p_str_opt = "send";
	}else if(opt == SO_RCVTIMEO){
		p_str_opt = "recv";
	}else{
		ha_Assert(false);
		ha_DebugNet("Unknown option: %d", opt);
		return false;
	}
	
#ifndef WIN32
	struct timeval timev;

	timev.tv_sec = time_s;
	timev.tv_usec = 0;

	p_timeout = (void *)&timev;
	timeout_size = sizeof(timev);
#else
	int time_ms = time_s * 1000;

	p_timeout = (void *)&time_ms;
	timeout_size = sizeof(time_ms);
#endif

	if(setsockopt(sock, SOL_SOCKET, opt, (char *)p_timeout, timeout_size) != 0){
		ha_DebugNet("Set socket %s timeout[%ds] failed.",
			p_str_opt, time_s);
		return false;
	}

	ha_DebugNet("Set socket %s timeout[%ds] successed.", p_str_opt, time_s);
	return true;
}

bool ha_SetSendTimeout(pgsocket sock, int time_s)
{
	return ha_SetSockTimeout(SO_SNDTIMEO, sock, time_s);
}

bool ha_SetRecvTimeout(pgsocket sock, int time_s)
{
	return ha_SetSockTimeout(SO_RCVTIMEO, sock, time_s);
}

bool ha_SetNoDelay(pgsocket sock)
{
	int			on = 1;

	if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
				   (char *) &on,
				   sizeof(on)) < 0)
	{
		return false;
	}

	ha_DebugNet("Set TCP_NODELAY.");
	return true;
}

int ha_Listen(ha_site_conf_t *p_mysite, uint32 max_listen,
	pgsocket *p_listen_socket)
{
	char c_listen_sock_name[] = "ha";
	char host[] = "0.0.0.0";
	int status = 0;

	for(uint32 i = 0; i < max_listen; i++){
		ha_SetSocketInvalid(p_listen_socket[i]);
	}

	status = StreamServerPort(AF_UNSPEC, host, p_mysite->port,
				c_listen_sock_name, p_listen_socket, max_listen);
	if(status != STATUS_OK){
		ha_Warning("Listen failed");
		return status;
	}

	for(uint32 i = 0; i < max_listen; i++){
		ha_DebugNet("listen_socket[%d] = %d\n", i, p_listen_socket[i]);
	}

	return STATUS_OK;
}

ha_tcp_conn_t *ha_Connect(const ha_site_conf_t *p_mysite,
	const ha_site_conf_t *p_site, uint32 send_timeout_s, uint32 recv_timeout_s)
{
	ha_tcp_conn_t *p_conn = (ha_tcp_conn_t *)malloc(sizeof(ha_tcp_conn_t));
	pgsocket conn_socket = 0;
	struct addrinfo *p_ai = p_site->p_ai;
	int ret = 0;

	ha_DebugNet("%s:%d start to connect: ip %s, port %d",
			p_mysite->pc_ip, p_mysite->port, p_site->pc_ip, p_site->port);

	if(p_conn == NULL){
		ha_EreportOOM();
		goto err;
	}

	conn_socket = sock_socket(p_ai->ai_family, p_ai->ai_socktype, p_ai->ai_protocol);
	if(conn_socket <= 0){
		ha_DebugNet("Create conn socket failed, ret is %d",
				ha_Socket2Int(conn_socket));
		goto err;
	}

	if(!ha_SetBlock(conn_socket)){
		ha_DebugNet("Set sock block failed");
		goto err;
	}

	if((!ha_SetSendTimeout(conn_socket, send_timeout_s))
	|| (!ha_SetRecvTimeout(conn_socket, recv_timeout_s))){
		ha_DebugNet("Set sock send&recv timeout failed");
		goto err;
	}

	if (!ha_SetNoDelay(conn_socket))
	{
		ha_DebugNet("Set TCP_NODELAY failed");
		goto err;
	}

	ret = sock_connect(conn_socket, p_ai->ai_addr, (int)p_ai->ai_addrlen);
	if(ret != 0){
		ha_DebugNet("Connect failed");
		goto err;
	}

	ha_DebugNet("connection established");

	p_conn->state = HA_TCONN_STATE_ESTABLISHED;
	p_conn->socket = conn_socket;

	return p_conn;

err:
	if(conn_socket > 0){
		ha_CloseSocket(conn_socket);
	}

	if(p_conn != NULL){
		free(p_conn);
	}

	return NULL;
}

ha_tcp_conn_t *ha_ConnectNoBlock(const ha_site_conf_t *p_mysite,
	const ha_site_conf_t *p_site)
{
	ha_tcp_conn_t *p_conn = (ha_tcp_conn_t *)malloc(sizeof(ha_tcp_conn_t));
	pgsocket conn_socket = 0;
	struct addrinfo *p_ai = p_site->p_ai;
	int ret;

	ha_DebugNet("%s:%d start to connect: ip %s, port %d",
			p_mysite->pc_ip, p_mysite->port, p_site->pc_ip, p_site->port);

	if(p_conn == NULL){
		ha_EreportOOM();
		goto err;
	}

	conn_socket = sock_socket(p_ai->ai_family, p_ai->ai_socktype, p_ai->ai_protocol);
	if(conn_socket <= 0){
		ha_DebugNet("Create conn socket failed, ret is %d",
				ha_Socket2Int(conn_socket));
		goto err;
	}

	ha_SetNoBlock(conn_socket);

	ret = sock_connect(conn_socket, p_ai->ai_addr, (int)p_ai->ai_addrlen);
	if(ret == 0){
		ha_DebugNet("Connect established.");

		p_conn->state = HA_TCONN_STATE_ESTABLISHED;
		p_conn->socket = conn_socket;
	}else{
		#ifdef WIN32
			if(WSAGetLastError() == EWOULDBLOCK){
				ha_DebugNet("Connect in progress.\n");
			}else{
				ha_Warning("Connect failed, ret is %d, errno is %d[%s]\n",
					ret, WSAGetLastError(), strerror(WSAGetLastError()));
				goto err;
			}
		#else
			if(errno == EINPROGRESS){
				ha_DebugNet("Connect in progress.\n");
			}else{
				ha_Warning("Connect failed, ret is %d, errno is %d[%s]\n",
					ret, errno, strerror(errno));
				goto err;
			}
		#endif

		p_conn->state = HA_TCONN_STATE_CONNING;
		p_conn->socket = conn_socket;
	}

	return p_conn;

err:
	if(conn_socket > 0){
		ha_CloseSocket(conn_socket);
	}

	if(p_conn != NULL){
		free(p_conn);
	}

	return NULL;
}

bool ha_IsConnectionEstablished(pgsocket sock)
{
	int error = 0;
	socklen_t len = sizeof(error);
	int ret;

	ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *)&error, &len);
	if((ret != 0) || (error != 0)){
		ha_DebugNet("Connection failed, error %d[%s]", error, strerror(error));
		return false;
	}

	ha_DebugNet("Connection established.");
	return true;
}

ha_tcp_conn_t *ha_Accept(pgsocket listen_socket, const ha_site_conf_t *p_my_site)
{
	ha_tcp_conn_t *p_conn = NULL;
	struct addrinfo *p_my_ai = p_my_site->p_ai;
	struct addrinfo *p_remote_ai;
	pgsocket new_socket;

	ha_DebugNet("Start to accept.");

	p_remote_ai = (struct addrinfo *)malloc(sizeof(struct addrinfo));
	if(p_remote_ai == NULL){
		ha_EreportOOM();
		goto free_res;
	}

	memset((char *)p_remote_ai, 0, sizeof(struct addrinfo));
	p_remote_ai->ai_addrlen = p_my_ai->ai_addrlen;
	p_remote_ai->ai_addr = (struct sockaddr *)malloc(p_remote_ai->ai_addrlen);
	if(p_remote_ai->ai_addr == NULL){
		ha_EreportOOM();
		goto free_res;
	}

	new_socket = sock_accept(listen_socket, p_remote_ai->ai_addr,
		(ACCEPT_TYPE_ARG3 *)&p_remote_ai->ai_addrlen);
	if(new_socket <= 0){
		ha_DebugNet("Accept failed, ret %d", new_socket);
		goto free_res;
	}

	ha_DebugNet("Accept OK.");
	p_conn = (ha_tcp_conn_t *)malloc(sizeof(ha_tcp_conn_t));
	if(p_conn == NULL){
		ha_EreportOOM();
		goto free_res;
	}
	
	//p_conn->state = HA_TCONN_STATE_OK;
	p_conn->state = HA_TCONN_STATE_ESTABLISHED;
	p_conn->socket = new_socket;

free_res:
	if(p_remote_ai != NULL){
		if(p_remote_ai->ai_addr != NULL){
			free(p_remote_ai->ai_addr);
		}
		free(p_remote_ai);
	}

	return p_conn;
}

int ha_SendMsg(pgsocket socket, const char *p_msg)
{
	int msg_len = ha_MsgTotalLen(p_msg);
	int send_len = 0;

	ha_SetBlock(socket);

	send_len = sock_send(socket, p_msg, msg_len, 0);
	if(send_len <= 0){
		ha_Error("Send failed, ret %d, errno %d", send_len, errno);
		return STATUS_ERROR;
	}

	//ha_SetNoBlock(socket);

	return STATUS_OK;
}

int ha_recvbuf(pgsocket socket, char* recvBuffer, int* recvPointer, int* recvLength)
{
	if (*recvPointer > 0)
	{
		if (*recvLength > *recvPointer)
		{
			/* still some unread data, left-justify it in the buffer */
			memmove(recvBuffer, recvBuffer + *recvPointer,
				*recvLength - *recvPointer);
			*recvLength -= *recvPointer;
			*recvPointer = 0;
		}
		else
			*recvLength = *recvPointer = 0;
	}

	ha_SetBlock(socket);

	/* Can fill buffer from recvLength and upwards */
	for (;;)
	{
		int			r;

		r = sock_recv(socket, recvBuffer + *recvLength,
						HA_MAX_MSG_LENGTH - *recvLength, 0);

		if (r < 0)
		{
			if (errno == EINTR)
				continue;		/* Ok if interrupted */

			ha_Error("Recv failed, ret %d, errno %d", r, errno);
			return EOF;
		}
		if (r == 0)
		{
			ha_Error("Recv failed, the peer has performed an orderly shutdown.");
			return EOF;
		}
		/* r contains number of bytes read, so just incr length */
		*recvLength += r;
		return 0;
	}
}

int ha_getbytes(char* s, int len, pgsocket socket, char* recvBuffer, int* recvPointer, int* recvLength)
{
	int		amount;

	while (len > 0)
	{
		while (*recvPointer >= *recvLength)
		{
			if (ha_recvbuf(socket, recvBuffer, recvPointer, recvLength))	/* If nothing in buffer, then recv some */
				return EOF;		/* Failed to recv data */
		}
		amount = *recvLength - *recvPointer;
		if (amount > len)
			amount = len;
		memcpy(s, recvBuffer + *recvPointer, amount);
		*recvPointer += amount;
		s += amount;
		len -= amount;
	}
	return 0;	
}

int ha_RecvMsg(pgsocket socket, char **pp_msg)
{
	char *recvBuffer = (char *)ha_malloc(HA_MSG_BUFFER_LENGTH);
	char *p_msg = (char *)ha_malloc(HA_MAX_MSG_LENGTH);
	char ch_len[HA_MSGH_SIZE_TOTALLEN];
	int recv_pointer = 0;
	int recv_len = 0;
	int r;
	int msg_len;
	int ret = STATUS_OK;
	bool bDiscardingMsg = false;

	ha_DebugNet("Receive msg");

	if(recvBuffer == NULL || p_msg == NULL){
		ha_EreportOOM();
		ret = STATUS_ERROR;
		goto failed;
	}

retry:
	r = ha_getbytes(ch_len, HA_MSGH_SIZE_TOTALLEN, socket, recvBuffer, &recv_pointer, &recv_len);
	if(r == EOF)
	{
		ha_Error("get message head failed.");
		ret = STATUS_EOF;
		goto failed;
	}
	
	memcpy(p_msg, ch_len, HA_MSGH_SIZE_TOTALLEN);

	/* first ntoh msg header */
	msg_len = ha_MsgTotalLen(ch_len);
	if (msg_len > HA_MAX_MSG_LENGTH || msg_len <= HA_MSGH_SIZE_TOTALLEN)
	{
		ha_Error("message length %d is not correct.", msg_len);
		ret = STATUS_EOF;
		goto failed;
	}

	r = ha_getbytes(p_msg + HA_MSGH_SIZE_TOTALLEN, msg_len - HA_MSGH_SIZE_TOTALLEN, socket, recvBuffer, &recv_pointer, &recv_len);
	if(r == EOF)
	{
		ha_Error("get message body failed.");
		ret = STATUS_EOF;
		goto failed;
	}

	if (recv_pointer == recv_len)
	{
		if (bDiscardingMsg)
		{
			ha_Warning("Multiple messages have been received at the same time, discarding messages.");
			ret = STATUS_OK;
			goto failed;
		}

		if(recvBuffer != NULL)
		{
			ha_free(recvBuffer);
		}

		*pp_msg = p_msg;
		return STATUS_OK;
	}

	if (recv_pointer < recv_len)
	{
		bDiscardingMsg = true;
		goto retry;
	}

failed:
	if (p_msg != NULL)
	{
		ha_free(p_msg);
	}

	if(recvBuffer != NULL)
	{
		ha_free(recvBuffer);
	}

	*pp_msg = NULL;
	return ret;
}
/* *****************************************************************************
* 							basic socket functions end
***************************************************************************** */

