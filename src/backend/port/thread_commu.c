/**
* @file thread_commu.c
* @brief 
* @author ������ ����
* @date 2011-10-24 15:53:19
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com huang.cheng@founder.com.cn
* All rights reserved.
*/
#include <ctime>
#include "port/thread_commu.h"
#include "utils/hsearch.h"
#include "postmaster/postmaster.h"
#include <sys/time.h>

struct HTAB *ThreadInfoCache = NULL;
pthread_mutex_t Mutex4ThreadInfoCache;
THREAD_LOCAL bool hasRegisterExitHandler = false;

#define output(fmt,...) \
	do\
	{\
		if (0)\
		{\
			ereport(DEBUG1, (errmsg(fmt, __VA_ARGS__)));\
		}\
	}while(0)
static void UnregisteThreadInfo( void );

void InitThreadInfoCache( void )
{
	if (ThreadInfoCache == NULL)
	{
		HASHCTL		ctl;

		/* Initialize the hash table. */
		MemSet(&ctl, 0, sizeof(ctl));
		ctl.keysize = sizeof(pthread_t);
		ctl.entrysize = sizeof(ThreadInfo);
		
		/* hash style Oid  */
		if (4 == sizeof(pthread_t))
		{
			ctl.hash = oid_hash;
		}
		else
		{
			ctl.hash = uint64_hash;
		}
		
		ThreadInfoCache =hash_create("Commu Cache", 256, &ctl, 
									HASH_ELEM | HASH_FUNCTION);
		pthread_mutex_init(&Mutex4ThreadInfoCache ,NULL);
	}
}

void DestoryThreadInfoCache( void )
{
	hash_destroy(ThreadInfoCache);
	ThreadInfoCache = NULL;
	pthread_mutex_destroy(&Mutex4ThreadInfoCache);
}

static ThreadInfo * RegisteThreadInfo( pthread_t tid )
{
	bool found = false;
	
	if (ThreadInfoCache == NULL)
	{
		ereport(ERROR,
                (errcode(ERRCODE_HASH_ERROR),
                errmsg("thread info hash table not created")));
		return NULL;
	}

	/* create thread information  */
	pthread_mutex_lock(&Mutex4ThreadInfoCache);
	ThreadInfo *ptrInfo = (ThreadInfo *)hash_search(ThreadInfoCache, &tid, HASH_ENTER, &found);
	pthread_mutex_unlock(&Mutex4ThreadInfoCache);

	if (!found)
	{
		pthread_cond_init( &ptrInfo->cond , NULL);
		pthread_mutex_init( &ptrInfo->mutex , NULL);
		ptrInfo->requests = 0;
		MemSet(ptrInfo->handler, 0 , sizeof(ptrInfo->handler));	

		if (pthread_equal(tid,pthread_self()) && !hasRegisterExitHandler)
		{
#ifdef _DEBUG
			ereport(DEBUG1, (errmsg("unregi %lu",tid)));
#endif
		    pthread_mutex_lock(&ptrInfo->mutex);
			if(!hasRegisterExitHandler)
			{
				hasRegisterExitHandler = true;
				ThreadAtExit(UnregisteThreadInfo);
			}
			pthread_mutex_unlock(&ptrInfo->mutex);
		}
	}

	return ptrInfo;
}

static void UnregisteThreadInfo( void )
{
	pthread_t tid = pthread_self();
	ThreadInfo *ptrInfo = NULL;
	
	if (ThreadInfoCache == NULL)
		ereport(ERROR,
                (errcode(ERRCODE_HASH_ERROR),
                errmsg("thread info hash table not created")));

	/** get ThreadInfo from ThreadInfoCache,and remove it  */
	ptrInfo = GetThreadInfo(tid,false);
	if (ptrInfo == NULL)
		return;

	/* hold the lock, avoid others use this thread info when we are removing */
	pthread_mutex_lock(&Mutex4ThreadInfoCache);
	
	pthread_mutex_destroy(&ptrInfo->mutex);
	pthread_cond_destroy(&ptrInfo->cond);
	hash_search(ThreadInfoCache, &tid , HASH_REMOVE, NULL);
	
    pthread_mutex_unlock(&Mutex4ThreadInfoCache);

}

ThreadInfo* GetThreadInfo(pthread_t tid,bool reg)
{
	ThreadInfo* pThreadInfo;
	
	if (NULL == ThreadInfoCache)
	{
		return NULL;
	}

	pthread_mutex_lock(&Mutex4ThreadInfoCache);
	pThreadInfo = (ThreadInfo*)hash_search(ThreadInfoCache, &tid,
											HASH_FIND, NULL);
	pthread_mutex_unlock(&Mutex4ThreadInfoCache);

	if(NULL != pThreadInfo)
	{
		Assert(pthread_equal(tid , pThreadInfo->tid));
		if (!hasRegisterExitHandler)
		{
			pthread_mutex_lock(&pThreadInfo->mutex);
			if(!hasRegisterExitHandler)
			{
				hasRegisterExitHandler = true;
				ThreadAtExit(UnregisteThreadInfo);
			}
			pthread_mutex_unlock(&pThreadInfo->mutex);
		}
		return pThreadInfo;
	}

	if(reg)
		return RegisteThreadInfo(tid);

	return NULL;
}

static void process_signal_internal(ThreadInfo* pThreadInfo)
{
	Assert(pThreadInfo != NULL);

	for (int iIndex = 0;iIndex < MAX_COUNT_OF_SINGALHANDLER;++iIndex)
	{
		uint32 uMask = 1<<iIndex;
		SigHandler hander = NULL;

		pthread_mutex_lock(&pThreadInfo->mutex);
#ifdef _DEBUG
		if ((pThreadInfo->requests & uMask))
		{
			output("thread:%lu should handle signo:%d ,h=%p",
					pThreadInfo->tid, iIndex, pThreadInfo->handler[iIndex]);
		}
#endif
		if ((pThreadInfo->requests & uMask)
			&& NULL != pThreadInfo->handler[iIndex])
		{
#ifdef _DEBUG
			output("invoke %lu's signal handle:%d", pThreadInfo->tid, iIndex);
#endif
			pThreadInfo->requests &= ~uMask;
			hander = pThreadInfo->handler[iIndex];
			pthread_mutex_unlock(&pThreadInfo->mutex);

			/*
			 * Signal hander func maybe invoke the pg_sleep/pg_process_signal .etc, for
			 * avoiding the nest lock, release the thread lock while call signal hander func.
			 */
		    hander(iIndex);
		}
		
		pthread_mutex_unlock(&pThreadInfo->mutex);
	}
}

void pg_sleep(long microsec)
{
	if (NULL == ThreadInfoCache)
	{
		return;
	}

	ThreadInfo* pThreadInfo = GetThreadInfo(pthread_self());
	struct timeval now;
	timespec time4wait;
	int nWaitResult;

	Assert(NULL != pThreadInfo);
	gettimeofday(&now,NULL);
	time4wait.tv_sec = now.tv_sec + (microsec + now.tv_usec) / 1000000L ;
	time4wait.tv_nsec= ((microsec + now.tv_usec) % 1000000L) * 1000L;

	pthread_mutex_lock(&pThreadInfo->mutex);	
	nWaitResult = pthread_cond_timedwait(&pThreadInfo->cond,&pThreadInfo->mutex,&time4wait);
	if (nWaitResult != EINVAL)
	{
#ifdef _DEBUG
		output("%lu thread received requests :%d", pThreadInfo->tid, pThreadInfo->requests);
#endif

		pthread_mutex_unlock(&pThreadInfo->mutex);
		process_signal_internal(pThreadInfo);
	}
}

void pg_process_signals( void )
{
	ThreadInfo* pThreadInfo = GetThreadInfo(pthread_self());

	if (pThreadInfo)
		process_signal_internal(pThreadInfo);	
}

pthread_cond_t pg_getthread_condition( void )
{
	ThreadInfo* pThreadInfo = GetThreadInfo(pthread_self());

	if (NULL != pThreadInfo)
	{
		return pThreadInfo->cond;
	}
	else
	{
		//ereport(LOG, (errmsg("�Ҳ��� %d ��ThreadInfo",tid)));
		elog(ERROR, "no thread info");
	} 
#if defined(_MSC_VER)
#define PTHREAD_COND_INITIALIZER {0}
#endif
	pthread_cond_t t = PTHREAD_COND_INITIALIZER;
	return t;
}
int pg_wakeup(pthread_t tid,int signo)
{
	if (NULL == ThreadInfoCache)
	{
		return -1;
	}

	ThreadInfo* pThreadInfo = GetThreadInfo(tid);
#ifdef _DEBUG
	output("invoke %p pg_wakeup(%lu,%d)",pThreadInfo,tid,signo);
#endif
	if (NULL != pThreadInfo && signo < MAX_COUNT_OF_SINGALHANDLER)
	{
		uint32 uMask = 1 << signo;
		//send request
		pthread_mutex_lock(&pThreadInfo->mutex);
		pThreadInfo->requests |= uMask;
		pthread_mutex_unlock(&pThreadInfo->mutex);

		pthread_cond_signal(&pThreadInfo->cond);
		return 0;
	}

	return -1;
}

SigHandler pg_signal(int signo,SigHandler func)
{
	if (NULL == ThreadInfoCache)
	{
		return NULL;
	}

	pthread_t tid = pthread_self();
	ThreadInfo* pThreadInfo = GetThreadInfo(tid);
#ifdef _DEBUG
	ereport(DEBUG1, (errmsg("thread %lu signal(%d,%p) :%p",tid,signo,func,pThreadInfo)));
#endif
	if (NULL != pThreadInfo && SigHandler(1) != func)
	{
		SigHandler oldHandler = NULL;
		
		pthread_mutex_lock(&pThreadInfo->mutex);
		oldHandler = pThreadInfo->handler[signo];
		pThreadInfo->handler[signo] = func;
		pthread_mutex_unlock(&pThreadInfo->mutex);
		return oldHandler;
	}

	return NULL;
}

void CheckThreadInfo( void )
{
	Assert(NULL != ThreadInfoCache);
	pthread_t tid = pthread_self();
	ThreadInfo* pThreadInfo = GetThreadInfo(tid);
	Assert(NULL != pThreadInfo);
}

pthread_key_t ExitHandleKey;
pthread_once_t OnceInitExitArray = PTHREAD_ONCE_INIT;
static const int MAX_THREAD_EXIT_HANDLER = 64;
struct ExitHandlerArrayT
{
	ThreadExitHandler base[MAX_THREAD_EXIT_HANDLER] ;
	int size;
};

THREAD_LOCAL ExitHandlerArrayT  ExitHandlerArray = {{0},0};

void CallExitHandler( void *)
{
    for (int i = 0; i < ExitHandlerArray.size; ++i)
    {
		ExitHandlerArray.base[i]();
		ExitHandlerArray.base[i] = 0;
    }
}

void ExitInfoInit( void )
{
	int status;

	//ereport(LOG, (errmsg("initializing key")));
	status = pthread_key_create(&ExitHandleKey,CallExitHandler);

	if(0 != status)
	{
		ereport(ERROR,
			(errcode(ERRCODE_HASH_ERROR),
			errmsg("hash table not created")));
	}
}

void ThreadAtExit(ThreadExitHandler exitFunc)
{
	int status;

	status = pthread_once(&OnceInitExitArray,ExitInfoInit);
	if (0 == ExitHandlerArray.size)
	{
		pthread_setspecific(ExitHandleKey,&ExitHandlerArray);
	}
	ExitHandlerArray.base[ExitHandlerArray.size++] = exitFunc;
}
#ifdef _DEBUG
bool bShutDownStarted = false;
#endif
