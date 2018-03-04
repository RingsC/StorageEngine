/**
* @file thread_commu.cpp
* @brief 
* @author ����
* @date 2011-10-28 14:23:55
* @version 1.0
* @copyright: founder.com
* @email: huang.cheng@founder.com 
* All rights reserved.
*/
#include <boost/thread/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include "port/thread_commu.h"
#ifndef FOUNDER_XDB_SE
#include "libpq/pg_signal.h"
#else
#include "port/thread_commu.h"
#endif
#include "utils/util.h"
#include "thread_commu/TestThreadCommon.h"
#include "test_fram.h"
#include "thread_commu/test_multthread_communicate.h"
#include "thread_commu/thread_commu_hs.h"

bool test_thread_communicate_twothread( void )
 {
	//INTENT("���������̷߳��źţ���Ӧ�������߳̽����ź�");
	 using namespace boost;
	 using namespace std;

	 {
		 RecverEx  recv1,recv2;
		 CHECK_BOOL(recv1.IsAllRecevied());
	     CHECK_BOOL(recv2.IsAllRecevied());

		 }
	return true;
 }

bool test_thread_communicate_Onesend_Multreceive( void )
{
	//INTENT("����һ���̷߳���ͬ���źţ������߳̽������ź�");
	using namespace boost;
	using namespace std;
	{
		RecverEx  recv1,recv2,recv3;
		CHECK_BOOL(recv1.IsAllRecevied());
		CHECK_BOOL(recv2.IsAllRecevied());
		CHECK_BOOL(recv3.IsAllRecevied());
	}
	return true;
}

bool test_thread_communicate_Multsend_Onereceive( void )
{
	//INTENT("���������̷ֱ߳𷢲�ͬ���źţ�һ���߳̽�����Щ�ź�");
	using namespace boost;
	using namespace std;
	{
		RecverEx  recv1;
		CHECK_BOOL(recv1.IsAllRecevied());

	}
	return true;
}

bool test_thread_communicate_Largesend(void)
{
	//INTENT("һ���̶߳���һ���̷߳�����źţ�����Ƿ���Щ�źŶ����յ�")
	using namespace boost;
	using namespace std;
	{
		RecverEx  recv1;
		recv1 +=SIGINT,SIGHUP,SIGUSR2,SIGQUIT,SIGTRAP,SIGABRT,SIGKILL,SIGPIPE,SIGALRM,SIGSTOP,
			SIGTSTP,SIGCONT,SIGINT,SIGCHLD,SIGTTIN,SIGTTOU,SIGWINCH,SIGUSR1,SIGUSR2;

		SenderEx  send1;
		send1(&recv1,SIGINT)(&recv1,SIGHUP)(&recv1,SIGUSR2)(&recv1,SIGQUIT)(&recv1,SIGTRAP)
			(&recv1,SIGABRT)(&recv1,SIGKILL)(&recv1,SIGPIPE)(&recv1,SIGALRM)(&recv1,SIGSTOP)
			(&recv1,SIGTSTP)(&recv1,SIGCONT)(&recv1,SIGCHLD)(&recv1,SIGTTIN)(&recv1,SIGTTOU)
			(&recv1,SIGWINCH)(&recv1,SIGUSR1)(&recv1,SIGUSR2);

		thread_group trdGrp;
		CHECK_BOOL(recv1.IsAllRecevied());
	return true;

}

bool test_thread_communicate_RandDely(void)
{
	//INTENT(" ���������߳��������߳�ͨ��ǰ��ÿ���̷߳�������ӳ٣��Ƿ��ܹ���ȷ�����ź�");
	using namespace boost;
	using namespace std;
	{
		RecverEx  recv1,recv2;
		CHECK_BOOL(recv1.IsAllRecevied());
		CHECK_BOOL(recv2.IsAllRecevied());

	}
	return true;
}

bool test_thread_communicate_RandDely_thread(void)
{
	//INTENT("���������߳��������߳�ͨ���У��̷߳�������ӳ٣��Ƿ��ܹ���ȷ�����ź�");
	using namespace boost;
	using namespace std;
	{
		RecverEx  recv1(true),recv2(true);
		CHECK_BOOL(recv1.IsAllRecevied());
		CHECK_BOOL(recv2.IsAllRecevied());

	}
	return true;
}