#ifndef TEST_HEAP_STORE_H
#define TEST_HEAP_STORE_H

#include "test_utils/test_utils.h"

class HeapStore : public HeapBase
{
public:
	enum
	{
		HEAP_SORT = 1,
		HEAP_TMP
	};
	int nflag;
	EntrySet::StoreSortType nSortFlag;
	EntrySet* pSpecialEntrySet;
	EntrySetScan* pScan;
	const static int row = 1;
	std::vector<DataItem> v_di;
	std::vector<std::string> vCmp1;
	std::vector<std::string> vCmp2;
	std::vector<std::string> vCmpPos1;
	std::vector<std::string> vCmpPos2;

};
bool test_heap_sort_1();
bool test_heap_sort();
bool test_heap_store();
#endif//TEST_HEAP_STORE_H