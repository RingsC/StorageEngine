/**
* @file test_subtrans.h
* @brief 
* @author 李书淦
* @date 2011-11-28 13:52:08
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_SUBTRANS_H
#define _TEST_SUBTRANS_H

/************************************************************************** 
* @brief TestSubTransaction 
* 
* Detailed description:
*  1. 启动一个事务,创建一个表，并插入一些数据
*  2. 启动一个子事务，向表中插入另外一些数据;
*  3. 提交子事务后，检测是否插入成功.
*  
* @param[in] void 
* @return bool  
**************************************************************************/
bool TestSubTransaction( void );

/************************************************************************** 
* @brief TestSubTransactionAbort 
* 
* Detailed description.
*  1. 启动一个事务,创建一个表，并插入一些数据;
*  2. 启动一个子事务，向表中插入另外一些数据;
*  3. 回滚.
*  3. 检测子事务的原子性.
* @param[in] void 
* @return bool  
**************************************************************************/
bool TestSubTransactionAbort( void );


/************************************************************************** 
* @brief TestMultiSubTransaction 
* 
* Detailed description.
* 1. 启动一个事务,创建一个表，并插入一些数据;"
* 2. 启动一个子事务，向表中插入另外一些数据后提交"
* 3. 启动另一个子事务，再插入一些数据"
* 4. 提交子事务后，检测是否插入成功.
* @param[in] void 
* @return bool  
**************************************************************************/
bool TestMultiSubTransaction( void );


/************************************************************************** 
* @brief TestMultiSubTransactionAbort 
* 
* Detailed description.
* 1. 启动一个事务,创建一个表，并插入一些数据;
* 2. 启动一个子事务，向表中插入另外一些数据后提交
* 3. 启动另一个子事务，再插入一些数据后回滚
* 4. 提交子事务后，检测是否插入成功.
* @param[in] void 
* @return bool  
**************************************************************************/
bool TestMultiSubTransactionAbort( void );

/************************************************************************** 
* @brief TestMultiLevelTransaction 
* 
* Detailed description.
* 1. 启动一个事务,创建一个表，并插入一些数据;"
* 2. 启动一个子事务1，向表中插入另外一些数据"
* 3. 启动另一个子事务2，再插入一些数据"
* 4. 提交子事务2
* 5. 提交子事务1
* 6. 检测是否插入成功.
* @param[in] void 
* @return bool  
**************************************************************************/
bool TestMultiLevelTransaction( void );

/************************************************************************** 
* @brief TestMultiLevelTransactionAbort 
* 
* Detailed description.
* 1. 启动一个事务,创建一个表，并插入一些数据;\n"
* 2. 启动一个子事务1，向表中插入另外一些数据\n"
* 3. 启动另一个子事务2，再插入一些数据后回滚\n"
* 4. 提交子事务1\n"
* 5. 检测是否插入成功\n");
* @param[in] void 
* @return bool  
**************************************************************************/
bool TestMultiLevelTransactionAbort( void );
#endif 
