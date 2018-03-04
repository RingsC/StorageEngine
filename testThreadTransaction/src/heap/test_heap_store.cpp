#include "test_utils/test_utils.h"
#include "heap/test_heap_store.h"
#include "PGSETypes.h"
#include "StorageEngine.h"
#include "StorageEngineException.h"
//#include "SortEntrySet.h"
#include "vector"
#include <string>
#include <iostream>
#include <algorithm>
#include "heap/test_heap_create.h"

int sort_cmp_func(const char *data1, size_t len1, const char *data2, size_t len2)
{
	if(len1 != len2)
	{
		return (len1 > len2) ? 1 : -1;
	}

	return memcmp(data1, data2, len1);
}

bool test_heap_insert_data(ParamBase* arg)
{
	//const int DATA_INSERT_LEN = DATA_LEN;
	const int DATA_INSERT_LEN = 5;
	HeapStore* pArgs = (HeapStore*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		/* 构造测试数据 */
		/* 20+MB测试数据 */
		DataGenerater dg(pArgs->row, DATA_INSERT_LEN);
		dg.dataGenerate();
		DataItem item;
		for(uint32 i = 0; i < pArgs->row; ++i)
		{
			char* pData = (char*)malloc(DATA_INSERT_LEN);
			memcpy(pData,dg[i],DATA_INSERT_LEN);
			item.setData(pData);
			item.setSize(DATA_INSERT_LEN);
			pArgs->v_di.push_back(item);
		}
		item.setData(NULL);

		pEntrySet->insertEntries(pArgs->GetTransaction(), pArgs->v_di);
		StorageEngine::getStorageEngine()->endStatement();
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_store_data_2_special_EntrySet(ParamBase* arg)
{
	HeapStore* pArgs = (HeapStore*)arg;
	try
	{
		StorageEngine* pStorageEngine =  StorageEngine::getStorageEngine();
		/* 打开表，将查询出来的数据放入PGTempEntrySet中 */
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		EntrySet *pSpecialEntrySet = NULL;
		if (HeapStore::HEAP_TMP == pArgs->nflag)
			pSpecialEntrySet = pStorageEngine->createTempEntrySet(pArgs->GetTransaction(),
			EntrySet::SST_HeapTuples,
			0, 
			pArgs->row * DATA_LEN, 
			false, 
			true);

		if(HeapStore::HEAP_SORT == pArgs->nflag)
			pSpecialEntrySet = pStorageEngine->createSortEntrySet(pArgs->GetTransaction(), 
			pArgs->nSortFlag, 
			pArgs->row * DATA_LEN, 
			true, 
			sort_cmp_func);
		
		pArgs->pSpecialEntrySet = pSpecialEntrySet;
		/// 使用第一种方式，传递整个表作为参数
		pSpecialEntrySet->insertEntries(pArgs->GetTransaction(), *pEntrySet);

		std::vector<ScanCondition> v_sc;
		EntrySetScan *ess = pEntrySet->startEntrySetScan(pArgs->GetTransaction(),BaseEntrySet::SnapshotNOW,v_sc);
		EntryID ei;
		DataItem di;
		while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
		{
			/// 使用第二种方式，使用一个DataItem作为参数
			pSpecialEntrySet->insertEntry(pArgs->GetTransaction(), ei, di);
		}
		pEntrySet->endEntrySetScan(ess);

		/// 使用第三种方式， 使用一个vector作为参数
		pSpecialEntrySet->insertEntries(pArgs->GetTransaction(), pArgs->v_di);

		if (HeapStore::HEAP_SORT == pArgs->nflag)
		{
			SortEntrySet* pSortEntrySet = (SortEntrySet*)pSpecialEntrySet;
			pSortEntrySet->performSort();
		}
		
		pStorageEngine->closeEntrySet(pArgs->GetTransaction(), pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_store_mark_pos(ParamBase* arg)
{
	HeapStore* pArgs = (HeapStore*)arg;

	try
	{
		/* 此时pTempEntrySet中拥有了3份entrySet的数据 */
		EntrySetScan* pScan = pArgs->pSpecialEntrySet->startEntrySetScan(pArgs->GetTransaction(),BaseEntrySet::SnapshotNOW,(std::vector<ScanCondition>)NULL);
		int32 index_pos = (pArgs->row * 3 - 20);
		DataItem item;
		EntryID eid;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG, eid, item) == SUCCESS)
		{
			pArgs->vCmp1.push_back(static_cast<char*>(item.getData()));
			if(index_pos == 0)
			{
				pScan->markPosition();
			}
			if(index_pos < 0)
			{
				pArgs->vCmpPos1.push_back(static_cast<char*>(item.getData()));
			}
			--index_pos;
		}
		pArgs->pScan = pScan;
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_store_check(ParamBase* arg)
{
	HeapStore* pArgs = (HeapStore*)arg;

	try
	{
		pArgs->pScan->restorePosition();

		DataItem item;
		EntryID eid;
		while(pArgs->pScan->getNext(EntrySetScan::NEXT_FLAG, eid, item) == SUCCESS)
		{
			pArgs->vCmpPos2.push_back(static_cast<char*>(item.getData()));
		}
		pArgs->pSpecialEntrySet->endEntrySetScan(pArgs->pScan);

		for(int i = 0; i < 3; ++i)
		{
			for(int j = 0; j < pArgs->row; ++j)
			{
				std::string str((char*)pArgs->v_di[j].getData(),pArgs->v_di[j].getSize()-1);
				pArgs->vCmp2.push_back(str);
			}
		}
		sort(pArgs->vCmp1.begin(), pArgs->vCmp1.end());
		sort(pArgs->vCmp2.begin(), pArgs->vCmp2.end());
		sort(pArgs->vCmpPos1.begin(), pArgs->vCmpPos1.end());
		sort(pArgs->vCmpPos2.begin(), pArgs->vCmpPos2.end());
		bool bflag = (pArgs->vCmp1 == pArgs->vCmp2);
		bool bflag1 = (pArgs->vCmpPos1 == pArgs->vCmpPos2);
		pArgs->SetSuccessFlag(bflag1 && bflag1);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_store()
{
	INITRANID()
	HeapStore* arg = new HeapStore;
	arg->nflag = HeapStore::HEAP_TMP;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_insert_data, arg);
	REGTASK(test_heap_store_data_2_special_EntrySet, arg);
	REGTASK(test_heap_store_mark_pos, arg);
	REGTASK(test_heap_store_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_sort()
{
	INITRANID()
	HeapStore* arg = new HeapStore;
	arg->nflag = HeapStore::HEAP_SORT;
	arg->nSortFlag = EntrySet::SST_HeapTuples;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_insert_data, arg);
	REGTASK(test_heap_store_data_2_special_EntrySet, arg);
	REGTASK(test_heap_store_mark_pos, arg);
	REGTASK(test_heap_store_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_sort_1()
{
	INITRANID()
	HeapStore* arg = new HeapStore;
	arg->nflag = HeapStore::HEAP_SORT;
	arg->nSortFlag = EntrySet::SST_GenericDataTuples;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_insert_data, arg);
	REGTASK(test_heap_store_data_2_special_EntrySet, arg);
	REGTASK(test_heap_store_mark_pos, arg);
	REGTASK(test_heap_store_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}