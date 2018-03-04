/**
* @file BRTest.h
* @brief 
* @author ������
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
Note: 1.�����������������е������С�
      2.����ʱ��Ҫ����-c backup�� ���򲻻�����;
	  3.����test_backupʱ����Ҫָ��-dѡ�����ָ��base backup�洢��Ŀ¼
	  4.�����ֱ������BRTest.cpp����Ŀ¼�е�python�ű�BackupRecoveryTest.py
//=====================================================================*/

/************************************************************************/
/*                                                                      */
/************************************************************************/
/************************************************************************** 
* @brief prepareBackup 
* 
* Detailed description. ����׼��һЩ���ݿ�
* @param[in] void 
* @return bool  
**************************************************************************/
bool prepareBackup( void );


/************************************************************************** 
* @brief test_backup 
* 
* Detailed description. ��һ��base backup���ҳ�������һ���,Ȼ��ҵ�
* @param[in] void 
* @return bool  
**************************************************************************/
bool test_backup( void );


/************************************************************************** 
* @brief test_Recovery 
* 
* Detailed description. ���Իָ�����Ƿ���ȷ
* @param[in] void   
* @return bool  
**************************************************************************/
bool test_Recovery( void );

/************************************************************************** 
* @brief BackUpThread 
* 
* Detailed description. ����������̺߳�����������һ����������
* @param[in] baseBackDir ͨ���������ָ��base backupҪ�����Ŀ¼
**************************************************************************/
bool BackUpThread(const std::string& baseBackDir);
#endif 
