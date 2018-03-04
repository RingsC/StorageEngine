#ifndef TEST_HEAP_UPDATE_H
#define TEST_HEAP_UPDATE_H
#include "test_utils/test_utils.h"
class HeapUpdate : public HeapBase
{
public:
	HeapUpdate():bIncrement(true){}
	std::vector<std::string> vData;
	bool bIncrement;
};
bool test_heap_update_too_many_times_increment();
bool test_heap_update_too_many_times();
bool test_heap_update_normal();
bool test_heap_update_normal_1();
bool test_heap_update_normal_2();
bool test_heap_update_normal_3();
bool test_heap_update_normal_4();
bool test_heap_update_normal_5();
bool test_heap_update_normal_6();
bool test_heap_update_normal_7();
bool test_heap_update_normal_8();
bool test_heap_update_normal_9();
bool test_heap_update_normal_10();
bool test_heap_update_too_many_times_ex();
#endif//TEST_HEAP_UPDATE_H