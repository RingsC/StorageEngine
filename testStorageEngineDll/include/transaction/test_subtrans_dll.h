/**
* @file test_subtrans_dll.h
* @brief 
* @author 李书淦
* @date 2011-11-29 16:12:20
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_SUBTRANS_DLL_H
#define _TEST_SUBTRANS_DLL_H
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
bool TestSubTransactionDLL( void );

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
bool TestSubTransactionAbortDLL( void );


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
bool TestMultiSubTransactionDLL( void );


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
bool TestMultiSubTransactionAbortDLL( void );

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
bool TestMultiLevelTransactionDLL( void );

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
bool TestMultiLevelTransactionAbortDLL( void );

/************************************************************************** 
* @brief TestMultiSubTransCreateIndex 
* 
* Detailed description.
* 1. 启动一个事务,创建表并插入若干数据;\n"
* 2. 启动子事务A先创建索引A后插入若干数据并提交子事务A\n"
* 3. 启动子事务B插入若干数据后创建索引B\n"
* 4. 启动子事务C使用索引A扫描数据\n"
* 5. 启动子事务D使用索引B扫描数据\n");
* @param[in] void 
* @return bool  
**************************************************************************/
bool TestMultiSubTransCreateIndex( void );
#endif 
