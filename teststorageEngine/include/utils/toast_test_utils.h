/**
* @file toast_test_utils.h
* @brief ����һЩ������ͺ���
* @author ������
* @date 2011-9-15 13:47:10
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TOAST_TEST_UTILS_H
#define _TOAST_TEST_UTILS_H
#include <stdexcept>
#include <string>
#include <vector>
#include <set>
#include "interface/StorageEngineExceptionUniversal.h"
#include "utils/util.h"
using namespace FounderXDB::StorageEngineNS;
//����Toast���Ե�һЩ��������
#define DECLARE_TOAST_TEST(T)\
	T(const T&);\
public:\
	static T* g_pInstance;\
	static T* g_pTempInstance;\
static class T& instance()\
{\
	if(NULL == g_pInstance)\
	{\
		try\
		{\
			begin_first_transaction();\
			{\
				g_pInstance = new T(false);\
				assert(NULL != g_pInstance);\
				g_pInstance->setUp();\
				g_pTempInstance = new T(true);\
				assert(NULL != g_pTempInstance);\
				g_pTempInstance->setUp();\
			}\
		}CATCHNORETURN\
	}else{\
		Assert((NULL != g_pInstance) && (NULL != g_pTempInstance));\
	}\
	return *g_pInstance;\
}\
static class T& temp_instance()\
{\
	if((NULL == g_pInstance) && (NULL == g_pTempInstance))\
	{\
		try\
		{\
			begin_first_transaction();\
			{\
				g_pInstance = new T(false);\
				assert(NULL != g_pInstance);\
				g_pInstance->setUp();\
				g_pTempInstance = new T(true);\
				assert(NULL != g_pTempInstance);\
				g_pTempInstance->setUp();\
			}\
		}CATCHNORETURN\
	}else{\
		Assert((NULL != g_pInstance) && (NULL != g_pTempInstance));\
	}\
	return *g_pTempInstance;\
}

//DECLARE_TOAST_TEST_CASE���Զ�����Բ��Կ�ܵĲ�������
#define DECLARE_TOAST_TEST_CASE(T,name)\
inline	bool T##_##name(void)\
{\
	return T::instance().test##name();\
}

#define DECLARE_TEMP_TOAST_TEST_CASE(T,name)\
inline	bool T##_temp_##name(void)\
{\
	return T::temp_instance().test##name();\
}

//DEFINE_TOAST_END����������toast���Խ�����ִ�е�������
#define DEFINE_TOAST_END(T)\
inline	void T##_end()\
{\
try\
{\
	{\
		delete T::g_pInstance;\
		T::g_pInstance = NULL;\
		delete T::g_pTempInstance;\
		T::g_pTempInstance = NULL;\
	}\
	commit_transaction();\
}CATCHNORETURN\
}

/************************************************************************** 
* @brief RandomGenString 
*  ������������������ָ�����ȵĴ�����
* 
* @param[in/out] ���Է������ݣ����ɵ����ݻ���׷�ӵķ�ʽ����s����
* @param[in] nLen Ҫ���ɵ����ݳ���
**************************************************************************/
void RandomGenString(std::string& s,size_t nLen);
extern Oid test_toast_get_heap_id(bool temp);
extern Oid test_toast_get_index_id(bool temp);

#endif 
