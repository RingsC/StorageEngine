#ifndef TEST_HEAP_DELETE_H
#define TEST_HEAP_DELETE_H
#include "test_utils/test_utils.h"

class HeapDelete : public HeapBase
{
public:
	std::vector<std::string> vInsertData;
	std::string strNextData;
	std::string strPreData;
	int nDelNum;
};
bool test_heap_delete_first_tuple();
bool test_heap_delete_middle_tuple();
bool test_heap_delete_the_last_tuple();
bool test_heap_delete_second_tuple();
bool test_heap_delete_forth_tuple();
bool test_heap_delete_the_last_tuple_ex_1();
bool test_heap_delete_the_last_tuple_ex();
bool test_heap_delete_forth_tuple_ex();
bool test_heap_delete_middle_tuple_ex();
bool test_heap_delete_second_tuple_ex();
bool test_heap_delete_first_tuple_ex();
#endif//TEST_HEAP_DELETE_H
