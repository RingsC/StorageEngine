/**
* @file BRTest.h
* @brief 
* @author 李书淦
* @date 2012-2-14 9:23:47
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _BRTEST_H
#define _BRTEST_H
#include <string>
/*=====================================================================
Note: 1.这三个测试用例必有单独运行。
      2.运行时需要设置-c backup。 否则不会运行;
	  3.运行test_backup时还需要指定-d选项，用以指定base backup存储的目录
	  4.最好是直接运行BRTest.cpp所在目录中的python脚本BackupRecoveryTest.py
//=====================================================================*/

/************************************************************************/
/*                                                                      */
/************************************************************************/
/************************************************************************** 
* @brief prepareBackup 
* 
* Detailed description. 用来准备一些数据库
* @param[in] void 
* @return bool  
**************************************************************************/
bool prepareBackup( void );


/************************************************************************** 
* @brief test_backup 
* 
* Detailed description. 做一个base backup并且持续备份一会儿,然后挂掉
* @param[in] void 
* @return bool  
**************************************************************************/
bool test_backup( void );


/************************************************************************** 
* @brief test_Recovery 
* 
* Detailed description. 测试恢复结果是否正确
* @param[in] void   
* @return bool  
**************************************************************************/
bool test_Recovery( void );

/************************************************************************** 
* @brief BackUpThread 
* 
* Detailed description. 这个函数是线程函数，用来做一个基础备份
* @param[in] baseBackDir 通过这个参数指定base backup要存入的目录
**************************************************************************/
bool BackUpThread(const std::string& baseBackDir);
#endif 
