/**
* @file thread_commu.h
* @brief 
* @author ¿Ó È‰∆ ª∆Í…
* @date 2011-10-24 15:45:55
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com huang.cheng@founder.com.cn
* All rights reserved.
*/
#ifndef _THREAD_COMMU_H
#define _THREAD_COMMU_H
#include "postgres.h"
#include <pthread.h>
static const int MAX_COUNT_OF_SINGALHANDLER = 32;
typedef void (*SigHandler)( int );
typedef SigHandler pqsigfunc;
typedef void (*ThreadExitHandler)( void );
/**************************************************************************
* @struct ThreadInfo
* @brief used for communicate between threads
* 
* Detailed description.
* To enable the thread communicate of your program, you should invoke InitThreadInfoCache before
* all commutations and invoke DestoryThreadInfoCache after all threads finish their communicate
* for each thread, invoke RegisteThreadInfo  first, and then call pg_signal to register hander.
* from now no,Other thread will invoke pg_signal to send request to you and your thread can 
* invoke pg_sleep to wait these requests. when your threads receive some request,they will wakeup
* and do their handler. before your thread finish,you should invoke UnregisteThreadInfo.
*
* The following is a sample:
* main thread:
*  InitThreadInfoCache             -- main thread.
*  start thread 1
*  start thread 2
*  DestoryThreadInfoCache
*
* thread 1
*  RegisteThreadInfo           
*  pg_signal(SIGINT,hand)      
*  pg_signal(SIGHUP,hand) 
*  handle_loop()
*  UnregisteThreadInfo         

* thread 2
*  RegisteThreadInfo           
*  pg_wakeup(thread 1,SIGINT)
*  UnregisteThreadInfo 
**************************************************************************/
struct ThreadInfo
{
	pthread_t tid;        //the id of this thread
    pthread_cond_t cond;  //used to wait requests
	pthread_mutex_t mutex; 

	SigHandler handler[MAX_COUNT_OF_SINGALHANDLER];

	uint32  requests;    //request queue.
};

/** 
* @brief InitThreadInfoCache
* Initialize the hash table and apply space for ThreadInfoCache. 
* Hash style is Oid £¨256 threads allowed.
* Detailed description.
*/
 void InitThreadInfoCache( void );


/** 
* @brief DestoryThreadInfoCache
* Destroy the hash table and ThreadInfoCache Assigned to Null. 
* Detailed description.
*/
 void DestoryThreadInfoCache( void );


/** 
* @brief RegisteThreadInfo
* create a ThreadInfo for every thread , write ThreadInfo into 
* ThreadInfoCache. Initialize conditon and mutex.
* Detailed description.
*/
//ThreadInfo * RegisteThreadInfo( pthread_t tid  );


/** 
* @brief UnregisteThreadInfo
* Out the ThreadInfo from ThreadInfoCache and remove it.
* Detailed description.
*/
 //void UnregisteThreadInfo( void );

/************************************************************************** 
* @brief GetThreadInfo 
* 
* Detailed description.
* @param[in] tid thread id
* @return ThreadInfo*  thread info of the thread marked by tid
**************************************************************************/
ThreadInfo* GetThreadInfo(pthread_t tid,bool reg = true);

/************************************************************************** 
* @brief pg_signal 
* 
* Detailed description.
* @param[in] signo sig number
* @param[in] func  handler
**************************************************************************/
 SigHandler pg_signal(int signo,SigHandler func);

/************************************************************************** 
* @brief pg_sleep  :sleep microsec to wait requests.
*
* Detailed description.
* @param[in] microsec 
**************************************************************************/
 void pg_sleep(long microsec);

/************************************************************************** 
* @brief pg_wakeup 
* 
* Detailed description.
* @param[in] tid the id of target thread
* @param[in] signo 
**************************************************************************/
int pg_wakeup(pthread_t tid,int signo);


/************************************************************************** 
* @brief pg_process_signals  used to process pending signals
* 
* Detailed description.
* @param[in] void 
**************************************************************************/
void pg_process_signals( void );

/************************************************************************** 
* @brief pg_getthread_condition  get the thread condition variable
* 
* Detailed description.
* @param[in] void 
* @return pthread_cond_t  
**************************************************************************/
pthread_cond_t pg_getthread_condition( void );
void CheckThreadInfo( void );

/************************************************************************** 
* @brief ThreadAtExit  regesiter a func to call when thread exit
* 
* Detailed description.
* @param[in] ExitFunc function that will be called 
* @return void  
* @Note: functions will not be called if process terminated by the call of
*        exit/abort or exit normally from main thread.
**************************************************************************/
void ThreadAtExit(ThreadExitHandler exitFunc);

#ifdef _DEBUG
extern bool bShutDownStarted;
#endif

#endif 
