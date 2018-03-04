/**
* @file test_transaction_toast_dll.h
* @brief 测试Toast的表在回滚时的原子性
* @author 李书淦
* @date 2011-9-26 14:18:03
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANSACTION_TOAST_DLL_H
#define _TEST_TRANSACTION_TOAST_DLL_H
/************************************************************************** 
* @brief testToastTrans_InsertRollback_DLL 
* 1. 向表中插入一些数据（包含有大数据）;
* 2. 提交事务
* 3. 再插入其他一些数据(包含大数据)
* 4. 回滚
* 5  表中的数据
* @param[in] void 
* @return bool  如果回滚成功，返回true,否则返回false
**************************************************************************/
bool testToastTrans_InsertRollback_DLL( void );

/************************************************************************** 
* @brief testToastTrans_UpdateRollback_DLL 
* 1. 向表中插入一些数据（包含有大数据）;
* 2. 提交事务
* 3. Update
* 4. 回滚
* 5  检查表中的数据
* @return bool  如果回滚成功，返回true,否则返回false
**************************************************************************/
bool testToastTrans_UpdateRollback_DLL(void);

/************************************************************************** 
* @brief testToastTrans_DeleteRollback_DLL 
* 1. 向表中插入一些数据（包含有大数据）;
* 2. 提交事务
* 3. delete一些数据
* 4. 回滚
* 5  表中的数据
* Detailed description.
* @return bool  如果回滚成功，返回true,否则返回false
**************************************************************************/
bool testToastTrans_DeleteRollback_DLL(void);
#endif 
