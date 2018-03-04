/**
* @file thread_commu.h
* @brief 
* @author 李书淦
* @date 2011-10-26 13:34:36
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_THREAD_COMMU_H
#define _TEST_THREAD_COMMU_H

/************************************************************************** 
* @brief test_thread_communicate 
* 在主线程中启动两个字线程，并调用pg_sleep和pg_wakeup进行通信
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_communicate( void );
#endif 
