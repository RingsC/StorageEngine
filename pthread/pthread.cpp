// pthread.cpp : 定义 DLL 应用程序的导出函数。
//
#include "windows.h"
#include "pthread/pthread_init.h"
#include "errno.h"
#include <process.h> 
#include <cstdio>
static const int MaxOnceMutexCount = 32;
static pthread_mutex_t OnceMutexArray[MaxOnceMutexCount];
static int LastOnceMutexIndex = -1;
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	int ret = 0;

	if (SignalObjectAndWait (*mutex, *cond, INFINITE, FALSE) != WAIT_OBJECT_0)
		ret = EINVAL;
	else
		pthread_mutex_lock(mutex);

	return ret;
}

struct timeval_t {
	long    tv_sec;         /* seconds */
	long    tv_usec;        /* and microseconds */
};
typedef unsigned __int64 uint64;
#define UINT64CONST(x) ((uint64) x##ULL)
static const unsigned __int64 epoch = UINT64CONST(116444736000000000);
static int
gettimeofday1(struct timeval_t * tp, struct timezone * tzp)
{
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

int pthread_cond_timedwait(pthread_cond_t *cond, 
						   pthread_mutex_t *mutex, const struct timespec *abstime)
{
	int ret = 0;
	DWORD wret;
	long timeout;

	struct timeval_t now;
	gettimeofday1(&now,NULL);
	timespec time4wait;
	time4wait.tv_sec = now.tv_sec;
	time4wait.tv_nsec= now.tv_usec * 1000L;

	timeout = (long)((abstime->tv_sec - time4wait.tv_sec) * 1000L + (abstime->tv_nsec - time4wait.tv_nsec) / 1000000L);
	if (timeout <= 0)
		timeout = 1;

	/* 
	* mutex is signaled(released) and then cond is acquired in SignalObjectAndWait. 
	* And SignalObjectAndWait needs a milliseconds parameter, rather than an absolute time. 
	*/
	wret = SignalObjectAndWait(*mutex, *cond, timeout, FALSE);

	switch (wret) {
	case WAIT_OBJECT_0: 
		pthread_mutex_lock(mutex);
		break;
	case WAIT_TIMEOUT: 
		pthread_mutex_lock(mutex);
		//elog(DEBUG1, "\nWake up from sleep in pthread_cond_timedwait due to timeout.");
		ret = ETIMEDOUT; /* is the standard return in this case but it is not defined in Windows. */
		break;
	default:
		ret = EINVAL; break;
	}

	return ret;
}
int pthread_cond_signal(pthread_cond_t *cond)
{
	SetEvent(*cond);
	return 0;
}
int pthread_cond_init (pthread_cond_t *cond, const pthread_condattr_t *)
{
	*cond = CreateEvent(NULL, false, false, NULL);/* unnamed, default security setting, auto reset, initially unsignaled. */
	return 0;
}
int pthread_cond_destroy(pthread_cond_t *cond)
{
	CloseHandle(*cond);
	return 0;
}

int pthread_mutex_init(pthread_mutex_t *mutex, 
					   const pthread_mutexattr_t *attr)
{
	*mutex = CreateMutex(NULL, FALSE, NULL);
	return 0;
}
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	CloseHandle(*mutex);
	return 0;
}
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	int ret;
	DWORD wret;

	wret = WaitForSingleObject(*mutex, INFINITE);
	switch (wret) {
	case WAIT_OBJECT_0: ret = 0;break;
	default: ret = EINVAL;break;
	}
	return ret;
}
int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	int ret;
	DWORD wret;

	wret = WaitForSingleObject(*mutex, 0);
	switch (wret) {
	case WAIT_OBJECT_0: ret = 0;break;
	case WAIT_TIMEOUT: ret = EBUSY;break;
	default: ret = EINVAL;
	}
	return ret;
}
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	ReleaseMutex(*mutex);
	return 0;
}

int pthread_kill(pthread_t thread, int sig)
{
	int bRet = ESRCH;
	DWORD ExitCode = 0;

	HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION,FALSE,thread);
	if(hThread != NULL)
	{
		if(GetExitCodeThread(hThread,&ExitCode))
		{
			if( ExitCode == STILL_ACTIVE)
				bRet = 0;
		}

		CloseHandle(hThread);
	}

	return bRet;
}

int pthread_once(pthread_once_t *once_control, void (*init_routine) (void))
{
	if (once_control == NULL || init_routine == NULL)
	{
		return EINVAL;
	}
	else if (once_control->bInitialized)
	{
		return 0;
	}

	if (!InterlockedExchangeAdd((LPLONG)&once_control->bInitialized, 0)) /* MBR fence */
	{
		pthread_mutex_lock(&once_control->mutexCall);

		if (!once_control->bInitialized)
		{
			(*init_routine)();
			once_control->bInitialized = true;
		}

		pthread_mutex_unlock(&once_control->mutexCall);
	}

	return 0;
}

static void _destory_once_mutex()
{
	for (;LastOnceMutexIndex >= 0;--LastOnceMutexIndex)
	{
		pthread_mutex_destroy(&OnceMutexArray[LastOnceMutexIndex]);
	}

}

pthread_mutex_t _init_once_mutex()
{
	if (0 == LastOnceMutexIndex)
	{
		atexit(_destory_once_mutex);
	}

	pthread_mutex_init(&OnceMutexArray[++LastOnceMutexIndex],NULL);
	return OnceMutexArray[LastOnceMutexIndex];
}

pthread_t pthread_self( void )
{
	return GetCurrentThreadId();
}


int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
	*key = TlsAlloc();
	if (TLS_OUT_OF_INDEXES == *key)
	{
		return EAGAIN;
	}

	AddSpecDesctor(*key,destructor);
	return 0;
}

void *pthread_getspecific(pthread_key_t key)
{
	void *pValue = TlsGetValue(key);

	return pValue;
}

int pthread_setspecific(pthread_key_t key, const void *value)
{
	BOOL success = TlsSetValue(key,(LPVOID)value);
	if (success)
	{
		return 0;
	}
	else
	{
		DWORD err = GetLastError();
		if (ERROR_NOT_ENOUGH_MEMORY == err)
		{
			return ENOMEM;
		}
		else
		{
			return EINVAL;
		}
	}
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return t1 == t2;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
				   void *(*start_routine)(void*), void *arg)
{
	unsigned 	size = 1024 * 1024;
	unsigned int tid = 0;
	uintptr_t hThread = _beginthreadex(NULL,	/* no security info */
		size,				/* statck size */
		(unsigned(__stdcall*)(void*)) start_routine,/* start routine */
		arg,			/* arg list */
		0,				/* run */
		&tid);		/* thread id */
    *thread = tid;
	/* error for returned value 0 */
	//printf("%d,%p",tid,hThread);
	if (NULL == hThread)
	{
		return EAGAIN;
	}
	else
	{
		return 0;
	}
}

void pthread_exit(void *retval)
{
    _endthreadex(*(int*)retval);
}

int pthread_join(pthread_t thread, void **retval)
{
	HANDLE hThrd = OpenThread(SYNCHRONIZE,true,thread);
	if (NULL == hThrd)
	{
		printf("can not get the thread handle,errcode:%d\n",GetLastError());
		return ESRCH;
	}
	DWORD err = WaitForSingleObject(hThrd,INFINITE);

	int ret = 0;
	switch(err)
	{
	case WAIT_OBJECT_0 :
		ret = 0;
		break;
	case WAIT_FAILED:
		{
			DWORD nErr = GetLastError();
			if (ERROR_POSSIBLE_DEADLOCK == nErr)
			{
				ret = EDEADLK;
				break;
			}
		}
		ret = -1;
		break;
	}

	CloseHandle(hThrd);
	return ret;
}