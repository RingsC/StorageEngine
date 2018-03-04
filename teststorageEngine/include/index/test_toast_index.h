/**
* @file test_toast_index.h
* @brief TestIndexWithToast的定义
* @author 李书淦
* @date 2011-9-15 13:17:39
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TOAST_INDEX_H
#define _TEST_TOAST_INDEX_H
#include <exception>
#include <stack>
#include "utils/util.h"
#include "utils/toast_test_utils.h"

/**************************************************************************
* @class TestIndexWithToast
* @brief 测试在大数据之上建立索引的测试用例
* 
* Detailed description.
**************************************************************************/
class TestIndexWithToast
{
public:

	/************************************************************************** 
	* @brief setUp 
	* 
	* 用以准备测试中要使用的数据
	**************************************************************************/
	void setUp(void);

	/************************************************************************** 
	* @brief testInsert 
	* 在大数据上建立索引，然后插入数据，用index检索，检查结果是否正确
	* 
	* @return bool  如果测试通过返回true,否则返回false.
	**************************************************************************/
	bool testInsert(void);



	/************************************************************************** 
	* @brief testMultiOrder 
	* 创建多列索引，检查是否是有序的
	* Detailed description.
	* @return bool  
	**************************************************************************/
	bool testMultiOrder(void);


	/************************************************************************** 
	* @brief testUpdate 
	* 
	* 更新数据库中的数据，然后检查索引是否也同步地更新了
	* @return bool  如果测试通过返回true,否则返回false.
	**************************************************************************/
	bool testUpdate(void);


	/************************************************************************** 
	* @brief testDelete 
	* 
	* 删除一些数据，然后检查索引
	* @return bool  如果测试通过返回true,否则返回false.
	**************************************************************************/
	bool testDelete(void);

private:
	TestIndexWithToast(bool);

	SimpleHeap  m_heap;
	SimpleIndex m_index;

	std::string m_strInsertData1;    //插入的第一条数据
	std::string m_strInsertData2;    //插入的第二条数据
	std::string m_strInsertData3;    //插入的第三条数据
	std::string m_strInsertData4;    //插入的第四条数据
	std::string m_strInsertData5;    //插入的第五条数据

	std::string m_strUpdateData1;    //用以更新m_strInsertData1
	std::string m_strUpdateData2;    //用以更新m_strInsertData2
	std::string m_strUpdateData3;    //用以更新m_strInsertData4

	std::set<std::string> m_setInsert;  //待插入数据的集合
	std::stack<std::string> m_stackSort; //按序的查询结果
	std::set<std::string> m_setAfterUpdate; //更新后数据库应该有的数据
	std::set<std::string> m_setAfterDelete; //删除后数据库中应有的数据

	DECLARE_TOAST_TEST(TestIndexWithToast)

};

DECLARE_TOAST_TEST_CASE(TestIndexWithToast,Insert)
DECLARE_TOAST_TEST_CASE(TestIndexWithToast,MultiOrder)
DECLARE_TOAST_TEST_CASE(TestIndexWithToast,Update)
DECLARE_TOAST_TEST_CASE(TestIndexWithToast,Delete)

DECLARE_TEMP_TOAST_TEST_CASE(TestIndexWithToast,Insert)
DECLARE_TEMP_TOAST_TEST_CASE(TestIndexWithToast,MultiOrder)
DECLARE_TEMP_TOAST_TEST_CASE(TestIndexWithToast,Update)
DECLARE_TEMP_TOAST_TEST_CASE(TestIndexWithToast,Delete)

//定义清理函数
DEFINE_TOAST_END(TestIndexWithToast)
#endif 
