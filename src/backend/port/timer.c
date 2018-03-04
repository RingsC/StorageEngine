/*-------------------------------------------------------------------------
*
* timer.c
*	  Microsoft Windows Win32 Timer Implementation
*
*	  Limitations of this implementation:
*
*	  - Does not support interval timer (value->it_interval)
*	  - Only supports ITIMER_REAL
*
* Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
*
* IDENTIFICATION
*	  src/backend/port/win32/timer.c
*
*-------------------------------------------------------------------------
*/

#include "postgres.h"

#ifndef FOUNDER_XDB_SE
#include "libpq/pg_signal.h"
#else
#include "port/thread_commu.h"
#include "postmaster/postmaster.h"
#endif
extern  int	MaxBackends;

/* Communication area for inter-thread communication */
struct timerCA
{
	pthread_t  tid;
	timespec deadline;
} ;

static fxdb_thread_t* timerThreadHandle = NULL;
static const int MAX_COUNT_OF_TIMER = 256;
struct _TimerArray
{
    pthread_mutex_t  mut;
	pthread_cond_t cond;
#ifdef _DEBUG
	char __1;
	int curSize;
	char __2;
#else
	int curSize;
#endif
    timerCA hole[MAX_COUNT_OF_TIMER];
}TimerArray;

#ifdef _DEBUG
struct OutTimerCount
{
	OutTimerCount( const char* szFuncName )
	{
		Assert(TimerArray.curSize >= 0);
		strcpy(m_szFuncName,szFuncName);
		ereport(LOG, (errmsg("Enter %s : size -- %d",m_szFuncName,TimerArray.curSize)));
	}

	~OutTimerCount( void )
	{
		Assert(TimerArray.curSize >= 0);
		ereport(LOG, (errmsg("Exit %s : size -- %d",m_szFuncName,TimerArray.curSize)));
	}

	char m_szFuncName[256];
};

#define OUTPUT_TIMER_COUNT //OutTimerCount  __oou(__FUNCTION__)
#else
#define OUTPUT_TIMER_COUNT
#endif
int CompareTimeCA(const timerCA& t1,const timerCA& t2)
{
    if (t1.deadline.tv_sec > t2.deadline.tv_sec)
    {
		return 1;
    }
	else if (t1.deadline.tv_sec < t2.deadline.tv_sec)
	{
		return -1;
	}
	else
	{
		return t1.deadline.tv_nsec - t2.deadline.tv_nsec;
	}
}

void ClearDeadTimer( void )
{
	OUTPUT_TIMER_COUNT;
	int nLiveThread = TimerArray.curSize;
	for (int i = 0; i < TimerArray.curSize; ++i)
	{
		if (0 == TimerArray.hole[i].tid || ESRCH == pthread_kill(TimerArray.hole[i].tid,0))
		{
			--nLiveThread;
			TimerArray.hole[i].tid = 0;
		}
	}

	int i = 0;
	for (int j = 0; j < TimerArray.curSize; ++j)
	{
		if (0 != TimerArray.hole[j].tid)
		{
			if(i != j)
			{
				TimerArray.hole[i++] = TimerArray.hole[j];
			}
			else
			{
				++i;
			}
		}
	}	
	TimerArray.curSize = nLiveThread ;
}

void MoveTimer(int dest,int src)
{
	OUTPUT_TIMER_COUNT;
#ifdef _DEBUG
	ereport(LOG, (errmsg("move %d to %d",dest,src)));
#endif
	if (dest < src)
	{
		for (; src < TimerArray.curSize; ++src)
		{
			TimerArray.hole[dest] = TimerArray.hole[src];
			++dest;
		}
		
	}
	else if(dest > src)
	{
		int dstI = TimerArray.curSize + dest - src;
		int srcI = TimerArray.curSize;
		for (; dstI >= dest; --dstI)
		{
			TimerArray.hole[dstI] = TimerArray.hole[srcI];
			--srcI;
		}
	}
	else
	{

	}
}
void AddTimer(const timerCA& t)
{
	OUTPUT_TIMER_COUNT;
	pthread_mutex_lock(&TimerArray.mut);

	ClearDeadTimer();
	int low = 0,hight = TimerArray.curSize - 1;
    while(low <= hight)
	{
		int mid = (low + hight) / 2;
		int cmp = CompareTimeCA(t,TimerArray.hole[mid]);
		if (0 == cmp)
		{
            low = mid;
			break;
		}
		else if(cmp > 0)
		{
            low = mid + 1;
		}
		else
		{
			hight = mid - 1;
		}
	}

	if (low == TimerArray.curSize)
	{
		TimerArray.hole[TimerArray.curSize++] = t;
	}
	else
	{
		MoveTimer(low + 1,low);
		TimerArray.hole[low] = t;
		++TimerArray.curSize;
	}
	pthread_mutex_unlock(&TimerArray.mut);
}

void RemoveTimer(pthread_t tid)
{
	OUTPUT_TIMER_COUNT;
	pthread_mutex_lock(&TimerArray.mut);

	int i = 0;
	for (int j = 0; j < TimerArray.curSize; ++j)
	{
		if (tid != TimerArray.hole[j].tid)
		{
			if(i != j)
			{
				TimerArray.hole[i++] = TimerArray.hole[j];
			}
			else
			{
				++i;
			}
		}
	}	
	TimerArray.curSize = i ;
	pthread_mutex_unlock(&TimerArray.mut);
}
/* Timer management thread */
static void*
pg_timer_thread(void* param)
{
	Assert(param == NULL);

	timerCA sentinel = {pthread_self(),{60,0}};
	timerCA *waittime = NULL;
	timerCA tNow = {pthread_self(),{0,0}};
	for (;;)
	{
		pthread_mutex_lock(&TimerArray.mut);
		ClearDeadTimer();
	    OUTPUT_TIMER_COUNT;
		if (0 == TimerArray.curSize)
		{
			waittime = &sentinel;

			struct timeval now;
			gettimeofday(&now,NULL);
			sentinel.deadline.tv_sec = now.tv_sec + 60;
		}
		else
		{
			waittime = &TimerArray.hole[0];
		}

		Assert(waittime->deadline.tv_sec >= 0 && waittime->deadline.tv_nsec >= 0);

		int	r = pthread_cond_timedwait(&TimerArray.cond,&TimerArray.mut,&waittime->deadline);

		if (r == ETIMEDOUT)
		{
			/* Timeout expired, signal SIGALRM and turn it off */
			//			pg_queue_signal(SIGALRM);
			if (waittime != &sentinel)
			{
				struct timeval now;
				gettimeofday(&now,NULL);
				tNow.deadline.tv_sec = now.tv_sec;
				tNow.deadline.tv_nsec = now.tv_usec * 1000L;
				int i = 0,killed = 0;
				for(;i < TimerArray.curSize; ++i)
				{
					if (CompareTimeCA(tNow,TimerArray.hole[i]) >= 0)
					{
						++killed;
#ifdef WIN32
				        pgkill(TimerArray.hole[i].tid,SIGALRM);
#else
				        pthread_kill(TimerArray.hole[i].tid,SIGALRM);
#endif
					}
					else
				    {
						break;
				    }
				}
				if (killed > 0)
				{
					MoveTimer(0,killed);
				}
				TimerArray.curSize -= killed;
			}
		}
		else if(EINVAL == r)
		{
			/* Should never happen */
			Assert(false);
		}		
		pthread_mutex_unlock(&TimerArray.mut);
	}
	return NULL;
}

pthread_once_t  onceInitTimerThread = PTHREAD_ONCE_INIT;

void StartTimerThread( void )
{
	OUTPUT_TIMER_COUNT;
	if (NULL == timerThreadHandle)
	{
		TimerArray.curSize = 0;
		pthread_mutex_init(&TimerArray.mut,NULL);
		pthread_cond_init(&TimerArray.cond,NULL);
		timerThreadHandle = (fxdb_thread_t*)talloc(sizeof(fxdb_thread_t));
		extern int	fxdb_thread_create(fxdb_thread_t *,void *(*start)(void*),	void *);
		fxdb_thread_create(timerThreadHandle,pg_timer_thread,NULL)	;
	}
}
/*
* Win32 setitimer emulation by creating a persistent thread
* to handle the timer setting and notification upon timeout.
*/
int
pg_setitimer(int which, const struct itimerval * value, struct itimerval * ovalue)
{
	OUTPUT_TIMER_COUNT;
	Assert(value != NULL);
	Assert(value->it_interval.tv_sec == 0 && value->it_interval.tv_usec == 0);
	Assert(which == ITIMER_REAL);

    pthread_once(&onceInitTimerThread,StartTimerThread);

	while (NULL == timerThreadHandle);	

	if (0 == value->it_value.tv_sec && 0 == value->it_value.tv_usec)
	{ 
		//Remove the timer
		RemoveTimer(pthread_self());
		pthread_cond_signal(&TimerArray.cond);
	}
	else
	{
		struct timeval now;
		gettimeofday(&now,NULL);
		timespec time4wait;
		time4wait.tv_sec = now.tv_sec + value->it_value.tv_sec + (value->it_value.tv_usec + now.tv_usec) / 1000000L ;
		time4wait.tv_nsec= ((value->it_value.tv_usec + now.tv_usec) % 1000000L) * 1000L;  

		timerCA t;
		t.tid = pthread_self();
		t.deadline = time4wait;
		AddTimer(t);
		pthread_cond_signal(&TimerArray.cond);
	}
	return 0;
}
