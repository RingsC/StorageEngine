#ifndef TEST_THE_HEAP_OPRATE_H
#define TEST_THE_HEAP_OPRATE_H
#include "test_utils/test_utils.h"
class HeapOperate : public HeapBase
{
public:
	std::vector<std::string> vInsert;
	std::vector<std::string> vUpdate;
	std::vector<std::string> vDelete;
};
bool test_the_same_heap_operate_drop();
bool test_the_same_heap_operate_delete();
bool test_the_same_heap_operate_update();
bool test_the_same_heap_operate_select();
bool test_the_same_heap_operate_insert();
bool test_the_same_heap_operate_create();
#endif//TEST_THE_HEAP_OPRATE_H