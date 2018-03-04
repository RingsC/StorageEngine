#ifndef _FDTEST_TOAST_H
#define _FDTEST_TOAST_H
#include "utils/util.h"
#include "utils/toast_test_utils.h"
/**************************************************************************
* @file test_toast.h
* @brief ����toast�Ĳ��룬���£�ɾ������
* @author ������
* @date 2011-9-15 13:58:17
* @version 1.0
**************************************************************************/



/**************************************************************************
* @class TestToast
* @brief ���Բ���toast����ɾ�Ĳ����Ƿ�����
* 
**************************************************************************/
class TestToast
{
public:


	/************************************************************************** 
	* @brief setUp 
	* �����������룬���º�ɾ���Ĵ�����
	* Detailed description.
	* @param[in] void 
	**************************************************************************/
	void setUp( void );


	/************************************************************************** 
	* @brief testInsert 
	* ����һЩ�����ݣ�Ȼ�����Ƿ����ɹ�
	* Detailed description.
	* @return bool  
	**************************************************************************/
	bool testInsert(void);



	/************************************************************************** 
	* @brief testUpdate 
	* ����һЩ���ݣ�Ȼ������³ɹ����
	* Detailed description.
	* @return bool  
	**************************************************************************/
	bool testUpdate(void);


	/************************************************************************** 
	* @brief testDelete 
	* ɾ��һЩ���ݣ���������Ƿ�ɾ���ɹ�
	* Detailed description.
	* @return bool  
	**************************************************************************/
	bool testDelete(void);

private:
	TestToast(bool);


	SimpleHeap  m_heap;
	std::string m_strInsertData1;    //����ĵ�һ������
	std::string m_strInsertData2;    //����ĵڶ�������
	std::string m_strInsertData3;    //����ĵ���������
	std::string m_strInsertData4;    //����ĵ���������
 	std::string m_strInsertData5;    //����ĵ���������

	std::string m_strUpdateData1;    //���Ը���m_strInsertData1
	std::string m_strUpdateData2;    //���Ը���m_strInsertData2
	std::string m_strUpdateData3;    //���Ը���m_strInsertData4

	std::set<std::string> m_setInsert;  //���������ݵļ���
	std::set<std::string> m_setAfterUpdate; //���º����ݿ�Ӧ���е�����
	std::set<std::string> m_setAfterDelete; //ɾ�������ݿ���Ӧ�е�����

	DECLARE_TOAST_TEST(TestToast)
};

//������Բ��Կ�ܵ�����
DECLARE_TOAST_TEST_CASE(TestToast,Insert)
DECLARE_TOAST_TEST_CASE(TestToast,Update)
DECLARE_TOAST_TEST_CASE(TestToast,Delete)

DECLARE_TEMP_TOAST_TEST_CASE(TestToast,Insert)
DECLARE_TEMP_TOAST_TEST_CASE(TestToast,Update)
DECLARE_TEMP_TOAST_TEST_CASE(TestToast,Delete)

//����������
DEFINE_TOAST_END(TestToast)
#endif