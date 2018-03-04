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
	unsigned int	priority;				/*本地站点的选举优先级*/
	unsigned short	listen_port;			/*本地站点监听其它站点连接的端口*/
	char*			gourp_list;				/*所有站点的列表（ip+port）*/
	unsigned int	phrase1_timeout;		/*选举的第一阶段超时时间（微秒）*/
	unsigned int	phrase2_timeout;		/*选举的第二阶段超时时间（微秒）*/
	unsigned int	election_timeout;		/*一轮选举的超时时间（微秒）*/
	unsigned int	min_leis;				/*最少收到min_leis，才能进入投票阶段*/
	unsigned int	min_votes;				/*最少收到min_votes，才能成为master*/
	unsigned int	heartbeat_interval;		/*master广播心跳包的间隔（微秒）*/
	unsigned int	heartbeat_timeout;		/*master广播心跳包的间隔（微秒）*/
	unsigned int	election_delay;			/*多于一个master时，推迟选举的时间（秒） */
	unsigned int	manual_start_election;	/* 手动输入开始选举的命令，再开始选举 */
	unsigned int	tmp_conn_life_time;		/*connection（至少有一方不知道对端是哪个站点）的生命期（微秒）*/
	unsigned int	first_reconn_time;		/*到某个站点的connection断开后，第一次重连的时间（微秒）*/
	unsigned int	send_timeout;			/*发送的超时时间（微秒）*/
	unsigned int	recv_timeout;			/*接收的超时时间（微秒）*/
	int				tcp_ka_idle;			/*链路空闲多长时间会发送keep alive的探测包（秒）*/
	int				tcp_ka_interval;		/*探测包发送时间间隔（秒）*/
	int				tcp_ka_count;			/*最多发送多少探测包*/
	int             election_count_2sites;  /*2个节点的情况下的选举次数*/  
}rep_ha_t;

extern bool pg_set_noblock(pgsocket sock);
extern bool pg_set_block(pgsocket sock);
extern int pg_getaddrinfo_all(const char *hostname, const char *servname,
	const struct addrinfo * hintp, struct addrinfo ** result);
extern void pg_freeaddrinfo_all(int hint_ai_family, struct addrinfo * ai);
extern int StreamServerPort(int family, char *hostName, unsigned short portNumber,
	char *unixSocketName, pgsocket ListenSocket[], unsigned int MaxListen);

#endif
