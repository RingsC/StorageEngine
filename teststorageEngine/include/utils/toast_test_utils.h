/**
* @file toast_test_utils.h
* @brief 定义一些帮助宏和函数
* @author 李书淦
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
//申明Toast测试的一些帮助数据
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

//DECLARE_TOAST_TEST_CASE用以定义针对测试框架的测试用例
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

//DEFINE_TOAST_END用来定义在toast测试结束后执行的清理函数
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
*  这个函数用以随机生成指定长度的大数据
* 
* @param[in/out] 用以返回数据，生成的数据会以追加的方式加在s后面
* @param[in] nLen 要生成的数据长度
**************************************************************************/
void RandomGenString(std::string& s,size_t nLen);
extern Oid test_toast_get_heap_id(bool temp);
extern Oid test_toast_get_index_id(bool temp);

#endif 
