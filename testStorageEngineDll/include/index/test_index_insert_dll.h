/**
* @file test_index_insert_dll.h
* @brief 
* @author 李书淦
* @date 2011-9-21 15:48:34
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_INDEX_INSERT_DLL_H
#define _TEST_INDEX_INSERT_DLL_H

/************************************************************************** 
* @brief testIndexInsert_SingleColumn 
* 创建表和索引，向其中插入一些数据，并检查结果是否正确
* Detailed description.
* @param[in] void 
* @return bool  如果测试成功返回true,否则返回false
**************************************************************************/
bool testIndexInsert_SingleColumn( void );

/************************************************************************** 
* @brief testIndexInsert_SameScaKey 
* 创建表和索引，向其中插入一些数据，并检查结果是否正确(查询时在相同列使用两个相同的scakkey)
* Detailed description.
* @param[in] void 
* @return bool  如果测试成功返回true,否则返回false
**************************************************************************/
bool testIndexInsert_SameScaKey( void );

/************************************************************************** 
* @brief testIndex_InAnotherTrans 
* 在一个事务中建立表和索引，并插入一些数据，在另外一个事务中检查结果是否正确
* Detailed description.
* @return bool  如果测试成功返回true,否则返回false
**************************************************************************/
bool testIndex_InAnotherTrans(void);


/************************************************************************** 
* @brief testIndex_Sort 
* 创建一个组合索引，查找与第一个键值相等的值，检查返回结果是否按照第二关键字排序
* Detailed description.
* @return bool  如果测试成功返回true,否则返回false
**************************************************************************/
bool testIndex_Sort();

/************************************************************************** 
* @brief test_cluster_index
* tests cluster index functionalities, including insert key-value entries, 
* and then scan the index using various scan conditions; 
***************************************************************************/
bool test_cluster_index();
#endif 


