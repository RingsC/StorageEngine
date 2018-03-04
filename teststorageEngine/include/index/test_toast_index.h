/**
* @file test_toast_index.h
* @brief TestIndexWithToast�Ķ���
* @author ������
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
* @brief �����ڴ�����֮�Ͻ��������Ĳ�������
* 
* Detailed description.
**************************************************************************/
class TestIndexWithToast
{
public:

	/************************************************************************** 
	* @brief setUp 
	* 
	* ����׼��������Ҫʹ�õ�����
	**************************************************************************/
	void setUp(void);

	/************************************************************************** 
	* @brief testInsert 
	* �ڴ������Ͻ���������Ȼ��������ݣ���index������������Ƿ���ȷ
	* 
	* @return bool  �������ͨ������true,���򷵻�false.
	**************************************************************************/
	bool testInsert(void);



	/************************************************************************** 
	* @brief testMultiOrder 
	* ������������������Ƿ��������
	* Detailed description.
	* @return bool  
	**************************************************************************/
	bool testMultiOrder(void);


	/************************************************************************** 
	* @brief testUpdate 
	* 
	* �������ݿ��е����ݣ�Ȼ���������Ƿ�Ҳͬ���ظ�����
	* @return bool  �������ͨ������true,���򷵻�false.
	**************************************************************************/
	bool testUpdate(void);


	/************************************************************************** 
	* @brief testDelete 
	* 
	* ɾ��һЩ���ݣ�Ȼ��������
	* @return bool  �������ͨ������true,���򷵻�false.
	**************************************************************************/
	bool testDelete(void);

private:
	TestIndexWithToast(bool);

	SimpleHeap  m_heap;
	SimpleIndex m_index;

	std::string m_strInsertData1;    //����ĵ�һ������
	std::string m_strInsertData2;    //����ĵڶ�������
	std::string m_strInsertData3;    //����ĵ���������
	std::string m_strInsertData4;    //����ĵ���������
	std::string m_strInsertData5;    //����ĵ���������

	std::string m_strUpdateData1;    //���Ը���m_strInsertData1
	std::string m_strUpdateData2;    //���Ը���m_strInsertData2
	std::string m_strUpdateData3;    //���Ը���m_strInsertData4

	std::set<std::string> m_setInsert;  //���������ݵļ���
	std::stack<std::string> m_stackSort; //����Ĳ�ѯ���
	std::set<std::string> m_setAfterUpdate; //���º����ݿ�Ӧ���е�����
	std::set<std::string> m_setAfterDelete; //ɾ�������ݿ���Ӧ�е�����

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

//����������
DEFINE_TOAST_END(TestIndexWithToast)
#endif 
