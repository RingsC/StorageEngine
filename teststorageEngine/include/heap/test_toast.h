#ifndef _FDTEST_TOAST_H
#define _FDTEST_TOAST_H
#include "utils/util.h"
#include "utils/toast_test_utils.h"
/**************************************************************************
* @file test_toast.h
* @brief 测试toast的插入，更新，删除操作
* @author 李书淦
* @date 2011-9-15 13:58:17
* @version 1.0
**************************************************************************/



/**************************************************************************
* @class TestToast
* @brief 用以测试toast的增删改操作是否正常
* 
**************************************************************************/
class TestToast
{
public:


	/************************************************************************** 
	* @brief setUp 
	* 生成用来插入，更新和删除的大数据
	* Detailed description.
	* @param[in] void 
	**************************************************************************/
	void setUp( void );


	/************************************************************************** 
	* @brief testInsert 
	* 插入一些大数据，然后检查是否插入成功
	* Detailed description.
	* @return bool  
	**************************************************************************/
	bool testInsert(void);



	/************************************************************************** 
	* @brief testUpdate 
	* 更新一些数据，然后检查更新成功与否
	* Detailed description.
	* @return bool  
	**************************************************************************/
	bool testUpdate(void);


	/************************************************************************** 
	* @brief testDelete 
	* 删除一些数据，检查数据是否删除成功
	* Detailed description.
	* @return bool  
	**************************************************************************/
	bool testDelete(void);

private:
	TestToast(bool);


	SimpleHeap  m_heap;
	std::string m_strInsertData1;    //插入的第一条数据
	std::string m_strInsertData2;    //插入的第二条数据
	std::string m_strInsertData3;    //插入的第三条数据
	std::string m_strInsertData4;    //插入的第四条数据
 	std::string m_strInsertData5;    //插入的第五条数据

	std::string m_strUpdateData1;    //用以更新m_strInsertData1
	std::string m_strUpdateData2;    //用以更新m_strInsertData2
	std::string m_strUpdateData3;    //用以更新m_strInsertData4

	std::set<std::string> m_setInsert;  //待插入数据的集合
	std::set<std::string> m_setAfterUpdate; //更新后数据库应该有的数据
	std::set<std::string> m_setAfterDelete; //删除后数据库中应有的数据

	DECLARE_TOAST_TEST(TestToast)
};

//申明针对测试框架的用例
DECLARE_TOAST_TEST_CASE(TestToast,Insert)
DECLARE_TOAST_TEST_CASE(TestToast,Update)
DECLARE_TOAST_TEST_CASE(TestToast,Delete)

DECLARE_TEMP_TOAST_TEST_CASE(TestToast,Insert)
DECLARE_TEMP_TOAST_TEST_CASE(TestToast,Update)
DECLARE_TEMP_TOAST_TEST_CASE(TestToast,Delete)

//定义清理函数
DEFINE_TOAST_END(TestToast)
#endif