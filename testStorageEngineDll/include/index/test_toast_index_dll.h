/**
* @file test_toast_index_dll.h
* @brief 测试在toast上建立索引的测试用例
* @author 李书淦
* @date 2011-9-23 14:58:06
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TOAST_INDEX_DLL_H
#define _TEST_TOAST_INDEX_DLL_H

/************************************************************************** 
* @brief test_toast_index_insert 
* 在存有大数据的表上建立索引，然后插入另外一些数据，检查index是否同步更新
* Detailed description.
* @return bool  测试成功返回true,否则返回false
**************************************************************************/
bool test_toast_index_insert(void);


/************************************************************************** 
* @brief test_toast_index_update 
* 在存有大数据的表上建立索引，然后更新其中的一些数据，检查index是否同步更新
* Detailed description.
* @return bool  测试成功返回true,否则返回false
**************************************************************************/
bool test_toast_index_update(void);

/************************************************************************** 
* @brief test_toast_index_delete 
* 在存有大数据的表上建立索引，然后删除其中的一些数据，检查index是否同步更新
* Detailed description.
* @return bool  测试成功返回true,否则返回false
**************************************************************************/
bool test_toast_index_delete(void);

/************************************************************************** 
* @brief test_toast_index_order 
* 在存有大数据的表的两列上建立组合索引，构造查询，检测如果一关键字相等，结果是否按第二关键字排序
* Detailed description.
* @return bool  测试成功返回true,否则返回false
**************************************************************************/
bool test_toast_index_order(void);
#endif 
