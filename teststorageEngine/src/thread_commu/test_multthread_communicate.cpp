/**
* @file test_multthread_communicate.cpp
* @brief 
* @author ¿Ó È‰∆
* @date 2011-11-3 9:45:40
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
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
bool test_multthread_communicate( void )
{
	using namespace boost;
	using namespace std;
	{

		RecverEx  recv1,recv2,recv3,recv4;		recv1 += SIGINT,SIGUSR1;		recv2 += SIGINT;		recv3 += SIGHUP,SIGUSR2;		recv4 += SIGILL;		SenderEx  send1,send2,send3;		send1(&recv1,SIGINT)(&recv3,SIGHUP);		send2(&recv1,SIGUSR1);		send3(&recv2,SIGINT)(&recv3,SIGUSR2)(&recv4,SIGILL);		thread_group trdGrp;		trdGrp.create_thread(send1);		trdGrp.create_thread(send2);		trdGrp.create_thread(send3);		trdGrp.create_thread(recv1);		trdGrp.create_thread(recv2);		trdGrp.create_thread(recv3);		trdGrp.create_thread(recv4);        trdGrp.join_all();
		CHECK_BOOL(recv1.IsAllRecevied());
		CHECK_BOOL(recv2.IsAllRecevied());
		CHECK_BOOL(recv3.IsAllRecevied());
		CHECK_BOOL(recv4.IsAllRecevied());
	}
	return true;

}