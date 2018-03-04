/**
* @file TestThreadCommon.cpp
* @brief 
* @author ¿Ó È‰∆
* @date 2011-11-1 16:58:31
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <iostream>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include "port/thread_commu.h"
#include "thread_commu/TestThreadCommon.h"
void RecverEx::operator()()
{
	printf("Receiver: %d",pthread_self());
	SAVE_THREADINFO
	if (NULL == m_pConfig || m_pConfig->size() == 0)
	{
		return ;
	}

	BOOST_FOREACH(BOOST_TYPEOF(*(m_pConfig->begin())) sig,*m_pConfig)
	{
		pg_signal(sig,MakeSigHandle<RecverEx,&RecverEx::SigHandler>(this));
	}
	*m_tid = pthread_self();

	while (true)
	{
		if (randflag==Delay)
		{
			pg_sleep(Random(10000));
		}
		else
		{
			pg_sleep(10000L);
		}

		if (m_sRecvedSigs->size() == m_pConfig->size())
		{
			break;
		}
	}

}

void SenderEx::operator()( void )
{
	if (NULL == m_pConfig || m_pConfig->size() == 0)
	{
		return ;
	}
    printf("Sender: %d",pthread_self());
	SAVE_THREADINFO

	BOOST_FOREACH(BOOST_TYPEOF(*(m_pConfig->begin())) recv,*m_pConfig)
	{
		if (NULL != recv.first)
		{
			while (true)
			{
				if (0 == recv.first->GetTID())
				{
					if (randflag==Delay)
					{
						pg_sleep(Random(10000));
					}
					else
					{
						pg_sleep(1000L);
					}
				}
				else
				{
					break;
				}
			}
			pg_wakeup(recv.first->GetTID(),recv.second);
		}
	}
}

ThreadInfoSaver::ThreadInfoSaver()
{
	//RegisteThreadInfo(pthread_self());
}

ThreadInfoSaver::~ThreadInfoSaver()
{
	//UnregisteThreadInfo();
}
