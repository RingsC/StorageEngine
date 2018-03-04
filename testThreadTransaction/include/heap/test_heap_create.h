#ifndef _TEST_HEAP_CREATE_H
#define _TEST_HEAP_CREATE_H
#include "test_utils/test_utils.h"
class HeapMulti : public HeapBase
{
public:
	int nCount;
	std::vector<EntrySetID> vEntrySetID;
	std::vector<std::string> vWillDeleData;
	std::set<std::string> vAfterDeleData;
	std::map<std::string,std::string> mapWillUpdateData;
	std::set<std::string> vAfterUpdateData;

};
bool test_heap_create();
bool test_heap_create_drop_task(ParamBase* arg);
bool test_heap_create_task(ParamBase* arg);
bool test_heap_drop();
bool test_heap_create_1();
bool test_heap_create_2();
bool test_heap_create_3();
bool test_heap_create_4();
bool test_heap_create_5();
bool test_heap_create_6();
bool test_heap_create_7();
bool test_heap_create_8();
bool test_heap_create_9();
bool test_heap_create_10();
bool test_create_multi_heap_one_transaction();
#endif //_TEST_HEAP_CREATE_H
