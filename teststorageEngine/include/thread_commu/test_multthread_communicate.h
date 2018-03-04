/**
* @file test_multthread_communicate.h
* @brief 
* @author 李书淦
* @date 2011-11-3 9:42:24
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_MULTTHREAD_COMMUNICATE_H
#define _TEST_MULTTHREAD_COMMUNICATE_H

/************************************************************************** 
* @brief test_multthread_communicate 
* 
* Detailed description.
* 创建三个发送线程，四个接收线程，交叉发送信号，检查接收到的信号是否正确
* @param[in] void 
* @return bool  
**************************************************************************/
bool test_multthread_communicate( void );
#endif 
