/**
* @file test_transaction_index_dll.h
* @brief 测试index在有事务回滚情况的测试用例
* @author 李书淦
* @date 2011-9-23 14:36:16
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANSACTION_INDEX_DLL_H
#define _TEST_TRANSACTION_INDEX_DLL_H

/************************************************************************** 
* @brief test_trans_index_InsertRollback 
* 在表上创建索引，插入数据，然后回滚，检查表的原子性
* Detailed description.
* @return bool  
**************************************************************************/
bool test_trans_index_InsertRollback(void);


/************************************************************************** 
* @brief test_trans_index_UpdateRollback 
* 在表上创建索引，更新数据，然后回滚，检查表的原子性
* Detailed description.
* @return bool  
**************************************************************************/
bool test_trans_index_UpdateRollback(void);


/************************************************************************** 
* @brief test_trans_index_DeleteRollback 
* 在表上创建索引，删除数据，然后回滚，检查表的原子性
* Detailed description.
* @return bool  
**************************************************************************/
bool test_trans_index_DeleteRollback(void);

#endif 
