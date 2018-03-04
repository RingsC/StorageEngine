/**
* @file thread_commu.cpp
* @brief 
* @author ¿Ó È‰∆
* @date 2011-10-26 13:34:47
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <boost/thread/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include "port/thread_commu.h"
#include "thread_commu/thread_commu.h"
#ifndef FOUNDER_XDB_SE
#include "libpq/pg_signal.h"
#else
#include "port/thread_commu.h"
#endif
#include "utils/util.h"
#include "thread_commu/TestThreadCommon.h"

#include "test_fram.h"

class Recevier
{
public:
	Recevier():
	  m_tid(0)
      ,m_bIntReceived(false)
	  ,m_bHupReceived(false)
	  ,m_bUser2Received(false){}


	void run( void )
	{
		SAVE_THREADINFO
		m_tid = pthread_self();


		pg_signal(SIGINT,MakeSigHandle<Recevier,&Recevier::sigIntHandler>(this));
		pg_signal(SIGHUP,MakeSigHandle<Recevier,&Recevier::sigHupHandler>(this));
		pg_signal(SIGUSR2,MakeSigHandle<Recevier,&Recevier::sigUsr2Handler>(this));
		while(true)
		{
			pg_sleep(1000L);

			if (m_bHupReceived && m_bIntReceived && m_bUser2Received)
			{
				break;
			}
		}

	}

private:
	Recevier(const Recevier&);
	void sigIntHandler(int)
	{
		m_bIntReceived = true;
	}

	void sigHupHandler(int)
	{
		m_bHupReceived = true;
	}

	void sigUsr2Handler(int)
	{
		m_bUser2Received = true;
	}

public:
	pthread_t m_tid;
	bool  m_bIntReceived ;
	bool  m_bHupReceived ;
	bool  m_bUser2Received ;
};

class Sender
{
public:
	Sender(Recevier& recv):
	  m_recv(recv)
	{

	}
	void run( void )
	{
         SAVE_THREADINFO
         while(true)
		 {
			 if (0 == m_recv.m_tid)
			 {
				 pg_sleep(1000L);
			 }
			 else
			 {
                 break;
			 }
		 }

		 pg_wakeup(m_recv.m_tid,SIGINT);
		 pg_wakeup(m_recv.m_tid,SIGHUP);
		 pg_wakeup(m_recv.m_tid,SIGUSR2);
	}

private:
	Recevier &m_recv;
};

bool test_thread_communicate( void )
{
    using namespace boost;
	//InitThreadInfoCache();
	{

		Recevier recver;
		Sender  sender(recver);


		thread threadRecv(&Recevier::run,&recver);
		thread threadSend(&Sender::run,&sender);

		threadRecv.join();
		threadSend.join();


		CHECK_BOOL(true == recver.m_bIntReceived);
		CHECK_BOOL(true == recver.m_bHupReceived);
		CHECK_BOOL(true == recver.m_bUser2Received);
	}
	//DestoryThreadInfoCache();

	return true;
}


bool test_thread_communicate1(void)
{

    return true;
};
