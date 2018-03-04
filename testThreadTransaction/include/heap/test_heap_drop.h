//bool test_heap_drop();
#ifndef TEST_HEAP_DROP_H
#define TEST_HEAP_DROP_H
#include "test_utils/test_utils.h"

class DatabasePara : public ParamBase
{
public:
	std::string strDbName;
	static int nFlag;
};
bool test_database_01();
bool test_database_02();
#endif//TEST_HEAP_DROP_H