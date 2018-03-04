#ifndef TEST_HEAP_SELECT_H
#define TEST_HEAP_SELECT_H
#include "test_utils/test_utils.h"

class HeapSelect : public HeapBase
{
public:
	std::vector<ScanCondition> vCondition;
	std::vector<std::string> vSelectData;
};
bool test_heap_select_01();
bool test_heap_select_02();
bool test_heap_select_03();
bool test_heap_select_04();


#endif//TEST_HEAP_SELECT_H 