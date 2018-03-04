/**
* @file test_multthread_communicate.cpp
* @brief 
* @author ������
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

		RecverEx  recv1,recv2,recv3,recv4;
		CHECK_BOOL(recv1.IsAllRecevied());
		CHECK_BOOL(recv2.IsAllRecevied());
		CHECK_BOOL(recv3.IsAllRecevied());
		CHECK_BOOL(recv4.IsAllRecevied());
	}
	return true;

}