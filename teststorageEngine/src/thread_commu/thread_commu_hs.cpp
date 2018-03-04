/**
* @file thread_commu.cpp
* @brief 
* @author 黄晟
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
	//INTENT("测试两个线程发信号，对应的两个线程接收信号");
	 using namespace boost;
	 using namespace std;

	 {
		 RecverEx  recv1,recv2;		 recv1 += SIGINT;		 recv2 += SIGHUP;		 SenderEx  send1,send2;		 send1(&recv1,SIGINT);		 send2(&recv2,SIGHUP);		 thread_group trdGrp;		 trdGrp.create_thread(send1);		 trdGrp.create_thread(send2); 		 trdGrp.create_thread(recv1);		 trdGrp.create_thread(recv2);		 trdGrp.join_all();
		 CHECK_BOOL(recv1.IsAllRecevied());
	     CHECK_BOOL(recv2.IsAllRecevied());

		 }
	return true;
 }

bool test_thread_communicate_Onesend_Multreceive( void )
{
	//INTENT("测试一个线程发不同的信号，三个线程接收其信号");
	using namespace boost;
	using namespace std;
	{
		RecverEx  recv1,recv2,recv3;		recv1 += SIGINT;		recv2 += SIGHUP;		recv3 += SIGUSR2;		SenderEx  send1;		send1(&recv1,SIGINT);		send1(&recv2,SIGHUP);		send1(&recv3,SIGUSR2);		thread_group trdGrp;		trdGrp.create_thread(send1); 		trdGrp.create_thread(recv1);		trdGrp.create_thread(recv2);		trdGrp.create_thread(recv3);		trdGrp.join_all();
		CHECK_BOOL(recv1.IsAllRecevied());
		CHECK_BOOL(recv2.IsAllRecevied());
		CHECK_BOOL(recv3.IsAllRecevied());
	}
	return true;
}

bool test_thread_communicate_Multsend_Onereceive( void )
{
	//INTENT("测试三个线程分别发不同的信号，一个线程接收这些信号");
	using namespace boost;
	using namespace std;
	{
		RecverEx  recv1;		recv1 += SIGINT,SIGHUP,SIGUSR2;		SenderEx  send1,send2,send3;		send1(&recv1,SIGINT);		send2(&recv1,SIGHUP);		send3(&recv1,SIGUSR2);		thread_group trdGrp;		trdGrp.create_thread(send1); 		trdGrp.create_thread(send2);		trdGrp.create_thread(send3);		trdGrp.create_thread(recv1);		trdGrp.join_all();
		CHECK_BOOL(recv1.IsAllRecevied());

	}
	return true;
}

bool test_thread_communicate_Largesend(void)
{
	//INTENT("一个线程对另一个线程发多个信号，检测是否这些信号都能收到")
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

		thread_group trdGrp;		trdGrp.create_thread(send1); 		trdGrp.create_thread(recv1);    	trdGrp.join_all();
		CHECK_BOOL(recv1.IsAllRecevied());	}
	return true;

}

bool test_thread_communicate_RandDely(void)
{
	//INTENT(" 测试两个线程与两个线程通信前，每个线程发生随机延迟，是否能够正确解释信号");
	using namespace boost;
	using namespace std;
	{
		RecverEx  recv1,recv2;		recv1 += SIGINT,SIGHUP;		recv2 += SIGINT,SIGHUP;		SenderEx  send1,send2;		send1(&recv1,SIGINT)(&recv2,SIGHUP);		send2(&recv1,SIGHUP)(&recv2,SIGINT);		thread_group trdGrp;				pg_sleep(Random(10000));		trdGrp.create_thread(send1);		pg_sleep(Random(10000));		trdGrp.create_thread(send2);		pg_sleep(Random(10000));		trdGrp.create_thread(recv1);		pg_sleep(Random(10000));		trdGrp.create_thread(recv2);		trdGrp.join_all();
		CHECK_BOOL(recv1.IsAllRecevied());
		CHECK_BOOL(recv2.IsAllRecevied());

	}
	return true;
}

bool test_thread_communicate_RandDely_thread(void)
{
	//INTENT("测试两个线程与两个线程通信中，线程发生随机延迟，是否能够正确解释信号");
	using namespace boost;
	using namespace std;
	{
		RecverEx  recv1(true),recv2(true);		recv1 += SIGINT,SIGHUP;		recv2 += SIGINT,SIGHUP;		SenderEx  send1(true),send2(true);		send1(&recv1,SIGINT)(&recv2,SIGHUP);		send2(&recv1,SIGHUP)(&recv2,SIGINT);		thread_group trdGrp;		trdGrp.create_thread(send1);		trdGrp.create_thread(send2);		trdGrp.create_thread(recv1);		trdGrp.create_thread(recv2);		trdGrp.join_all();
		CHECK_BOOL(recv1.IsAllRecevied());
		CHECK_BOOL(recv2.IsAllRecevied());

	}
	return true;
}