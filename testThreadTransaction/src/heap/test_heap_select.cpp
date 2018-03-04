#include <iostream>
#include "test_utils/test_utils.h"
#include "heap/test_heap_select.h"
#include "heap/test_heap_create.h"

using namespace FounderXDB::StorageEngineNS;

bool test_heap_select_insert(ParamBase* arg)
{
	HeapSelect *pArgs = (HeapSelect*)arg;
	try
	{
		std::vector<std::string> vInsertData;
		vInsertData.push_back("123ab*");
		vInsertData.push_back("234bc+");
		vInsertData.push_back("345cd-");
		vInsertData.push_back("456de&");
		vInsertData.push_back("567ef#");

		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		InsertData(pArgs->GetTransaction(),pEntrySet,vInsertData);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_select_check(ParamBase* arg)
{
	HeapSelect* pArgs = (HeapSelect*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		EntrySetScan *pHeapScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(),BaseEntrySet::SnapshotMVCC,pArgs->vCondition);

		std::vector<std::string> vResult;
		DataItem item;
		EntryID eid;
		while (0 == pHeapScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strItem = (char*)item.getData();
			vResult.push_back(strItem);
		}
		
		pEntrySet->endEntrySetScan(pHeapScan);
		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);

		pArgs->SetSuccessFlag(vResult == pArgs->vSelectData);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_select_set_condition_01(ParamBase* arg)
{
	HeapSelect* pArgs = (HeapSelect*)arg;
	try
	{
		ScanCondition Con(1,ScanCondition::LessThan,(se_uint64)"345",2,str_compare);
		pArgs->vCondition.push_back(Con);
		pArgs->vSelectData.push_back("123ab*");
		pArgs->vSelectData.push_back("234bc+");
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_select_01()
{
	//LessThan单列单scankey，仅向前扫描
	INITRANID()
	HeapSelect *arg = new HeapSelect;
	arg->isMultiCol = true;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_select_insert, arg);
	REGTASK(test_heap_select_set_condition_01, arg);
	REGTASK(test_heap_select_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_select_set_condition_02(ParamBase* arg)
{
	HeapSelect* pArgs = (HeapSelect*)arg;
	try
	{
		ScanCondition Con1(1,ScanCondition::LessThan,(se_uint64)"456",3,str_compare);
		ScanCondition Con2(2,ScanCondition::GreaterThan,(se_uint64)"ab",2,str_compare);
		ScanCondition Con3(3,ScanCondition::Equal,(se_uint64)"+",1,str_compare);
		pArgs->vCondition.push_back(Con1);
		pArgs->vCondition.push_back(Con2);
		pArgs->vCondition.push_back(Con3);
		
		pArgs->vSelectData.push_back("234bc+");		
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_select_02()
{
	//多列多scankey，仅向前扫描，使用大于、小于和等于策略
	INITRANID()
	HeapSelect *arg = new HeapSelect;
	arg->isMultiCol = true;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_select_insert, arg);
	REGTASK(test_heap_select_set_condition_02, arg);
	REGTASK(test_heap_select_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_select_set_condition_03(ParamBase* arg)
{
	HeapSelect* pArgs = (HeapSelect*)arg;
	try
	{
		ScanCondition Con1(1,ScanCondition::LessThan,(se_uint64)"456",3,str_compare);
		ScanCondition Con2(1,ScanCondition::GreaterThan,(se_uint64)"123",3,str_compare);
		pArgs->vCondition.push_back(Con1);
		pArgs->vCondition.push_back(Con2);

		pArgs->vSelectData.push_back("234bc+");
		pArgs->vSelectData.push_back("345cd-");
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_select_03()
{
	//单列多scankey，仅向前扫描
	INITRANID()
	HeapSelect *arg = new HeapSelect;
	arg->isMultiCol = true;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_select_insert, arg);
	REGTASK(test_heap_select_set_condition_03, arg);
	REGTASK(test_heap_select_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_select_set_condition_04(ParamBase* arg)
{
	HeapSelect* pArgs = (HeapSelect*)arg;
	try
	{
		ScanCondition Con1(1,ScanCondition::LessThan,(se_uint64)"456",3,str_compare);
		ScanCondition Con2(2,ScanCondition::GreaterThan,(se_uint64)"ab",2,str_compare);
		pArgs->vCondition.push_back(Con1);
		pArgs->vCondition.push_back(Con2);

		pArgs->vSelectData.push_back("234bc+");
		pArgs->vSelectData.push_back("345cd-");
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

		return true;
}
bool test_heap_select_04()
{
	//多列多scankey，仅向前扫描，使用大于、小于和等于策略

	INITRANID()
	HeapSelect *arg = new HeapSelect;
	arg->isMultiCol = true;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_select_insert, arg);
	REGTASK(test_heap_select_set_condition_04, arg);
	REGTASK(test_heap_select_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}