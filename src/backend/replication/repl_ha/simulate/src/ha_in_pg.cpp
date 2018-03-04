#include "ha_head.h"

bool pg_set_noblock(pgsocket sock)
{
#ifndef WIN32
	return (fcntl(sock, F_SETFL, O_NONBLOCK) != -1);
#else
	unsigned long ioctlsocket_ret = 1;

	/* Returns non-0 on failure, while fcntl() returns -1 on failure */
	return (ioctlsocket(sock, FIONBIO, &ioctlsocket_ret) == 0);
#endif
}

bool pg_set_block(pgsocket sock)
{
#ifndef WIN32
	int			flags;

	flags = fcntl(sock, F_GETFL);
	if (flags < 0 || fcntl(sock, F_SETFL, (long) (flags & ~O_NONBLOCK)))
		return false;
	return true;
#else
	unsigned long ioctlsocket_ret = 0;

	/* Returns non-0 on failure, while fcntl() returns -1 on failure */
	return (ioctlsocket(sock, FIONBIO, &ioctlsocket_ret) == 0);
#endif
}

int pg_getaddrinfo_all(const char *hostname, const char *servname,
	const struct addrinfo * hintp, struct addrinfo ** result)
{
	int			rc;

	/* not all versions of getaddrinfo() zero *result on failure */
	*result = NULL;

	/* NULL has special meaning to getaddrinfo(). */
	rc = getaddrinfo((!hostname || hostname[0] == '\0') ? NULL : hostname,
					 servname, hintp, result);

	return rc;
}

void pg_freeaddrinfo_all(int hint_ai_family, struct addrinfo * ai)
{
	if(ai != NULL){
		freeaddrinfo(ai);
	}

	return;
}

bool set_sock_reused(pgsocket sock)
{
#ifndef WIN32
	int one = 1;

	if((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
	(char *)&one, sizeof(one))) != 0){
		//ha_WarningWithErrno("set_sock_reused failed");
		return false;
	}
#endif

	return true;
}

int StreamServerPort(int family, char *hostName, unsigned short portNumber,
	char *unixSocketName, pgsocket ListenSocket[], unsigned int MaxListen)
{
	struct addrinfo hint;
	struct addrinfo *p_addrs = NULL;
	struct addrinfo *p_addr = NULL;
	int ret = STATUS_OK;
	unsigned int listen_index = 0;
	pgsocket listen_sock;
	int	maxconn = 64;
	char c_port[8];

	/* Initialize hint structure */
	memset(&hint, 0, sizeof(hint));
	hint.ai_family = family;
	hint.ai_flags = AI_PASSIVE;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;

	snprintf(c_port, 8, "%d", portNumber);
	ret = pg_getaddrinfo_all(hostName, c_port, &hint, &p_addrs);
	if((ret != 0) || (p_addrs == NULL)){
		//ha_WarningWithErrno("pg_getaddrinfo_all failed");
		ret = STATUS_ERROR;
		goto out;
	}

	p_addr = p_addrs;
	while((p_addr != NULL) && (listen_index < MaxListen)){
		listen_sock = socket(p_addr->ai_family, SOCK_STREAM, 0);
		if(ha_Socket2Int(listen_sock) < 0){
			//ha_WarningWithErrno("create socket failed");
			ret = STATUS_ERROR;
			goto out;
		}

		if(!set_sock_reused(listen_sock)){
			ret = STATUS_ERROR;
			goto out;
		}

		if(bind(listen_sock, p_addr->ai_addr, p_addr->ai_addrlen) != 0){
			//ha_WarningWithErrno("bind failed");
			ret = STATUS_ERROR;
			goto out;
		}

		if(listen(listen_sock, maxconn) != 0){
			//ha_WarningWithErrno("listen failed");
			ret = STATUS_ERROR;
			goto out;
		}

		ListenSocket[listen_index] = listen_sock;
		p_addr = p_addrs->ai_next;
	}

out:
	if(p_addrs != NULL){
		pg_freeaddrinfo_all(hint.ai_family, p_addrs);
	}

	if(ret != STATUS_OK){
		for(uint32 i = 0; i < MaxListen; i++){
			if(!ha_SocketIsInvalid(ListenSocket[i])){
				ha_CloseSocket(ListenSocket[i]);
			}
		}
	}

	return ret;
}

/* FILETIME of Jan 1 1970 00:00:00. */
#ifdef WIN32
int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	static const uint64 epoch = uint64(116444736000000000);
	FILETIME	file_time;
	SYSTEMTIME	system_time;
	ULARGE_INTEGER ularge;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	ularge.LowPart = file_time.dwLowDateTime;
	ularge.HighPart = file_time.dwHighDateTime;

	tp->tv_sec = (long) ((ularge.QuadPart - epoch) / 10000000L);
	tp->tv_usec = (long) (system_time.wMilliseconds * 1000);

	return 0;
}
#endif

XLogRecPtr GetWalRcvWriteRecPtr(XLogRecPtr *latestChunkStart, TimeLineID *receiveTLI)
{
	XLogRecPtr lsn = {1, 1};
	return lsn;
}

int GetPostPort(void)
{
	return 8888;
}

bool CurrentNodeSync(void)
{
	return true;
}

void RequestShutDownWalRcv()
{

}

bool
WalRcvHadCatchupPrimary(void)
{
	return true;
}


