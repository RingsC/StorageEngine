/**
* @file test_transactionWithToast.h
* @brief 测试大数据的插入，删除，更新的回滚
* @author 李书淦
* @date 2011-9-20 14:06:33
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANSACTIONWITHTOAST_H
#define _TEST_TRANSACTIONWITHTOAST_H

/************************************************************************** 
* @brief testToastTrans_InsertRollback 
* 1. 向表中插入一些数据（包含有大数据）;
* 2. 提交事务
* 3. 再插入其他一些数据(包含大数据)
* 4. 回滚
* 5  表中的数据
* @param[in] void 
* @return bool  如果回滚成功，返回true,否则返回false
**************************************************************************/
bool testToastTrans_InsertRollback( void );

/************************************************************************** 
* @brief testToastTrans_UpdateRollback 
* 1. 向表中插入一些数据（包含有大数据）;
* 2. 提交事务
* 3. Update
* 4. 回滚
* 5  检查表中的数据
* @return bool  如果回滚成功，返回true,否则返回false
**************************************************************************/
bool testToastTrans_UpdateRollback(void);

/************************************************************************** 
* @brief testToastTrans_DeleteRollback 
* 1. 向表中插入一些数据（包含有大数据）;
* 2. 提交事务
* 3. delete一些数据
* 4. 回滚
* 5  表中的数据
* Detailed description.
* @return bool  如果回滚成功，返回true,否则返回false
**************************************************************************/
bool testToastTrans_DeleteRollback(void);
#endif 
