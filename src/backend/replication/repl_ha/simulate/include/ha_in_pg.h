#ifndef __HA_IN_PG_H__
#define __HA_IN_PG_H__

#include <stdio.h>
#include <stdlib.h>

/* include */
#ifdef WIN32
#include <io.h>
#include <Ws2tcpip.h>
#include <mstcpip.h>
#include <pthread.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <pthread.h>
#endif





/* macros, structs, and functions */
#ifdef WIN32
/* windows */
#define pg_sleep(sleep_time_us) Sleep(sleep_time_us/1000)

#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINTR WSAEINTR

#define PGINVALID_SOCKET INVALID_SOCKET

typedef SOCKET pgsocket;

extern int gettimeofday(struct timeval * tp, struct timezone * tzp);

#define ACCEPT_TYPE_ARG3 int

#define snprintf(buf, nbytes, f, ...) _snprintf_s(buf, nbytes, nbytes - 1, f, ##__VA_ARGS__)
#else
/* linux */
#define pg_sleep(sleep_time_us) usleep(sleep_time_us/1000)
#define closesocket(sockfd) close(sockfd)

#define PGINVALID_SOCKET (-1)
#define ACCEPT_TYPE_ARG3 socklen_t

typedef int pgsocket;
#endif

#define palloc(size) malloc(size)
#define pfree(ptr) free(ptr)

#define STATUS_OK				(0)
#define STATUS_ERROR			(-1)
#define STATUS_EOF				(-2)

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef uint32 TimeLineID;

typedef struct XLogRecPtr
{
	uint32		xlogid;			/* log file #, 0 based */
	uint32		xrecoff;		/* byte offset of location in log file */
} XLogRecPtr;

#define XLByteLT(a, b)		\
			((a).xlogid < (b).xlogid || \
			 ((a).xlogid == (b).xlogid && (a).xrecoff < (b).xrecoff))

#define XLByteLE(a, b)		\
			((a).xlogid < (b).xlogid || \
			 ((a).xlogid == (b).xlogid && (a).xrecoff <= (b).xrecoff))

#define XLByteEQ(a, b)		\
			((a).xlogid == (b).xlogid && (a).xrecoff == (b).xrecoff)

extern XLogRecPtr GetWalRcvWriteRecPtr(XLogRecPtr *latestChunkStart, TimeLineID *receiveTLI);
extern int GetPostPort(void);
extern bool CurrentNodeSync(void);
extern void RequestShutDownWalRcv();
extern bool WalRcvHadCatchupPrimary(void);

typedef struct rep_ha
{
	unsigned int	priority;				/*����վ���ѡ�����ȼ�*/
	unsigned short	listen_port;			/*����վ���������վ�����ӵĶ˿�*/
	char*			gourp_list;				/*����վ����б�ip+port��*/
	unsigned int	phrase1_timeout;		/*ѡ�ٵĵ�һ�׶γ�ʱʱ�䣨΢�룩*/
	unsigned int	phrase2_timeout;		/*ѡ�ٵĵڶ��׶γ�ʱʱ�䣨΢�룩*/
	unsigned int	election_timeout;		/*һ��ѡ�ٵĳ�ʱʱ�䣨΢�룩*/
	unsigned int	min_leis;				/*�����յ�min_leis�����ܽ���ͶƱ�׶�*/
	unsigned int	min_votes;				/*�����յ�min_votes�����ܳ�Ϊmaster*/
	unsigned int	heartbeat_interval;		/*master�㲥�������ļ����΢�룩*/
	unsigned int	heartbeat_timeout;		/*master�㲥�������ļ����΢�룩*/
	unsigned int	election_delay;			/*����һ��masterʱ���Ƴ�ѡ�ٵ�ʱ�䣨�룩 */
	unsigned int	manual_start_election;	/* �ֶ����뿪ʼѡ�ٵ�����ٿ�ʼѡ�� */
	unsigned int	tmp_conn_life_time;		/*connection��������һ����֪���Զ����ĸ�վ�㣩�������ڣ�΢�룩*/
	unsigned int	first_reconn_time;		/*��ĳ��վ���connection�Ͽ��󣬵�һ��������ʱ�䣨΢�룩*/
	unsigned int	send_timeout;			/*���͵ĳ�ʱʱ�䣨΢�룩*/
	unsigned int	recv_timeout;			/*���յĳ�ʱʱ�䣨΢�룩*/
	int				tcp_ka_idle;			/*��·���ж೤ʱ��ᷢ��keep alive��̽������룩*/
	int				tcp_ka_interval;		/*̽�������ʱ�������룩*/
	int				tcp_ka_count;			/*��෢�Ͷ���̽���*/
	int             election_count_2sites;  /*2���ڵ������µ�ѡ�ٴ���*/  
}rep_ha_t;

extern bool pg_set_noblock(pgsocket sock);
extern bool pg_set_block(pgsocket sock);
extern int pg_getaddrinfo_all(const char *hostname, const char *servname,
	const struct addrinfo * hintp, struct addrinfo ** result);
extern void pg_freeaddrinfo_all(int hint_ai_family, struct addrinfo * ai);
extern int StreamServerPort(int family, char *hostName, unsigned short portNumber,
	char *unixSocketName, pgsocket ListenSocket[], unsigned int MaxListen);

#endif
