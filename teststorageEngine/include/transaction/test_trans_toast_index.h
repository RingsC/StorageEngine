/**
* @file test_trans_toast_index.h
* @brief 申明测试在toast数据上建立index,然后rollback的测试用例
* @author 李书淦
* @date 2011-9-20 16:33:01
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANS_TOAST_INDEX_H
#define _TEST_TRANS_TOAST_INDEX_H


/************************************************************************** 
* @brief test_toastIndex_InsertRollabck 
* 在taost数据上创建索引，测试插入回滚的用例
* Detailed description.
* @return bool  如果测试成功返回true,否则返回false
**************************************************************************/
bool test_toastIndex_InsertRollabck(void);


/************************************************************************** 
* @brief test_toastIndex_UpdateRollback 
* 在taost数据上创建索引，测试更新回滚的用例
* Detailed description.
* @return bool  如果测试成功返回true,否则返回false
**************************************************************************/
bool test_toastIndex_UpdateRollback(void);


/************************************************************************** 
* @brief test_toastIndex_DeleteRollback 
* 在taost数据上创建索引，测试删除回滚的用例
* Detailed description.
* @return bool  如果测试成功返回true,否则返回false
**************************************************************************/
bool test_toastIndex_DeleteRollback(void);
#endif 
