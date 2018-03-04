#ifndef __HA_DEBUG_H__
#define __HA_DEBUG_H__

#define HA_ELEVEL_DEBUG    1
#define HA_ELEVEL_INFO_1   2  // 输出到ha.log
#define HA_ELEVEL_INFO_2   3  // 同时输出到ha.log和server.log
#define HA_ELEVEL_CRITICAL 4

#define PRINT_TO_FILE   true

extern void ha_message_to_log4cxx(const char* msg, int len, int elevel);

extern void SvrCfgSetServerRole(int master_enable_standby, int standby_mode);

static inline void ha_Write(FILE *p_file, const char *p_line, int len)
{
	if (p_file != NULL)
	{
#ifdef WIN32
		_write(_fileno(p_file), p_line, len);
#else
		write(fileno(p_file), p_line, len);
#endif
	}
}

static inline void ha_WriteLogFile(FILE *p_file, const char *p_line, int len, int elevel)
{
#ifdef HA_SIMULATE_ENV
	ha_Write(p_file, p_line, len);
#else
	char message[2048];
	strcpy(message, p_line);
	if (message[len - 1] == '\n')  
	{
		message[len - 1] = '\0';
	}	
	ha_message_to_log4cxx(message, len, elevel);
#endif
}

#define __ha_Printf(print2srceen, print2file, elevel, f, ...) \
	do{ \
		char __one_line[2048]; \
		int __total_len; \
		snprintf(__one_line, 2048, f, ##__VA_ARGS__); \
		__total_len = (int)strlen(__one_line); \
		if(print2srceen){\
			ha_Write(stderr, __one_line, __total_len); \
		}\
		if(print2file){ \
			ha_WriteLogFile(g_ha_log_file, __one_line, __total_len, elevel); \
		} \
	}while(0)

#ifdef HA_SIMULATE_ENV
#define PRINT_TO_SCREEN true
#define __ha_Debug(print2srceen, print2file, f, ...) \
	do{ \
struct timeval __now; \
	gettimeofday(&__now, NULL); \
	__ha_Printf(print2srceen, print2file, HA_ELEVEL_DEBUG, "[%lds%ldus][%s:%d]"f"\n", \
	__now.tv_sec, __now.tv_usec, \
	__FUNCTION__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#define ha_Printf(f, ...) __ha_Printf(true, true, HA_ELEVEL_INFO_1, f, ##__VA_ARGS__)
#define __ha_Info(print2srceen, print2file, f, ...) \
	do{ \
		struct timeval __now; \
		gettimeofday(&__now, NULL); \
		__ha_Printf(print2srceen, print2file, HA_ELEVEL_INFO_1, "[%lds%ldus][%s:%d]"f"\n", \
			__now.tv_sec, __now.tv_usec, \
			__FUNCTION__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#else
#define PRINT_TO_SCREEN false
#define __ha_Debug(print2srceen, print2file, f, ...) \
	do{ \
	__ha_Printf(print2srceen, print2file, HA_ELEVEL_DEBUG, "[%s:%d]"f"\n", \
	__FUNCTION__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#define ha_Printf(f, ...) __ha_Printf(PRINT_TO_SCREEN, PRINT_TO_FILE, HA_ELEVEL_INFO_1, f, ##__VA_ARGS__)
#define __ha_Info(print2srceen, print2file, f, ...) \
	do{ \
	__ha_Printf(print2srceen, print2file, HA_ELEVEL_INFO_1, "[%s:%d]"f"\n", \
	__FUNCTION__, __LINE__, ##__VA_ARGS__); \
	}while(0)
#endif

#define __ha_DebugWithErrno(print2srceen, print2file, f, ...) \
	__ha_Debug(print2srceen, print2file, f"errno %d[%s]", ##__VA_ARGS__, \
	errno, strerror(errno))

#define __ha_InfoWithErrno(print2srceen, print2file, f, ...) \
	__ha_Info(print2srceen, print2file, f"errno %d[%s]", ##__VA_ARGS__, \
			errno, strerror(errno))

#define __ha_Info2(print2srceen, print2file, f, ...) \
	do{ \
		__ha_Printf(print2srceen, print2file, HA_ELEVEL_INFO_2, f"\n", ##__VA_ARGS__); \
	}while(0)

#define ha_Info2(f, ...) __ha_Info2(PRINT_TO_SCREEN, PRINT_TO_FILE, f, ##__VA_ARGS__)
#define ha_Log(f, ...) __ha_Info(PRINT_TO_SCREEN, PRINT_TO_FILE, "[LOG]"f, ##__VA_ARGS__)
#define ha_Warning(f, ...) __ha_Info(PRINT_TO_SCREEN, PRINT_TO_FILE, "[WARNING]"f, ##__VA_ARGS__)
#define ha_Error(f, ...) __ha_Info(PRINT_TO_SCREEN, PRINT_TO_FILE, "[ERROR]"f, ##__VA_ARGS__)
#define ha_WarningWithErrno(f, ...) __ha_InfoWithErrno(PRINT_TO_SCREEN, PRINT_TO_FILE, "[WARNING]"f, ##__VA_ARGS__)
#define ha_ErrorWithErrno(f, ...) __ha_InfoWithErrno(PRINT_TO_SCREEN, PRINT_TO_FILE, "[ERROR]"f, ##__VA_ARGS__)

#define HA_DEBUG_NET
#ifdef HA_DEBUG_NET
#define ha_DebugNet(f, ...) __ha_Debug(false, true, "[NET]"f, ##__VA_ARGS__)
#define ha_DebugNetWithErrno(f, ...) __ha_DebugWithErrno(false, PRINT_TO_FILE, "[NET]"f, ##__VA_ARGS__)
#else
#define ha_DebugNet(f, ...)
#define ha_DebugNetWithErrno(f, ...)
#endif

#define HA_DEBUG_SELECT
#ifdef HA_DEBUG_SELECT
#define ha_DebugSelect(f, ...) __ha_Debug(false, PRINT_TO_FILE, "[SELECT]"f, ##__VA_ARGS__)
#else
#define ha_DebugSelect(f, ...)
#endif

#define HA_DEBUG_ELECTION
#ifdef HA_DEBUG_ELECTION
#define ha_DebugElection(f, ...) __ha_Info(false, PRINT_TO_FILE, "[ELECTION]"f, ##__VA_ARGS__)
#else
#define ha_DebugElection(f, ...)
#endif

//#define HA_DEBUG_ENV
#ifdef HA_DEBUG_ENV
#define ha_DebugEnv(f, ...) __ha_Info(false, PRINT_TO_FILE, "[ELECTION]"f, ##__VA_ARGS__)
#else
#define ha_DebugEnv(f, ...)
#endif

#define ha_EreportOOM() \
	do{ \
		ha_Error("Out of memory"); \
	}while(0)

#define ha_Assert(cond) \
	do{ \
		if(!(cond)){ \
			ha_Error("BUG!!!!!!!");\
		} \
	}while(0)

#ifndef HA_SIMULATE_ENV
#define ha_ThrowError(rest) ereport(ERROR, rest)
extern void ShutDownWalRecvListener();

static inline void ha_KillTwoMasters()
{
	ShutDownWalRecvListener();

#ifdef WIN32
	GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
#else
	kill(getpid(), SIGUSR1);
#endif
}

#else
#define ha_ThrowError(rest) exit(-5555)
#define ha_KillTwoMasters() exit(-2222)
#endif

#endif
