#ifndef _TEST_HEAP_INSERT_H
#define _TEST_HEAP_INSERT_H
#include "test_utils/test_utils.h"
#include "heap/test_heap_create.h"

class HeapInsert : public HeapBase
{
public:
	std::vector<std::string> vInsertData;
	std::vector<std::string> vQueryData;
};
bool test_heap_insert_one_record();
bool test_heap_insert_multi_records();
bool test_heap_insert_large_data();
bool test_heap_insert_with_null_chars();
bool test_heap_insert_many_records();
#endif//_TEST_HEAP_INSERT_H
