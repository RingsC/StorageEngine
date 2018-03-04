#include <iostream>
#include "heap/test_heap_insert.h"

bool test_heap_insert_create_task(ParamBase* arg)
{
	HeapBase *pArgs = (HeapBase*)arg;
	std::cout<<">>>>>>>>>>>>>"<<pArgs->strCaseName<<"<<<<<<<<<<<<"<<std::endl;
	try
	{
		if (pArgs->isMultiCol)
		{
			pArgs->entrysetId = StorageEngine::getStorageEngine()->createEntrySet(pArgs->GetTransaction(),GetMultiCollInfo());
		}
		else
		{
			pArgs->entrysetId = StorageEngine::getStorageEngine()->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

		return true;
}

bool test_heap_insert_task(ParamBase* arg)
{
	HeapInsert *pArgs = (HeapInsert*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		InsertData(pArgs->GetTransaction(), pEntrySet,pArgs->vInsertData);
		
		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}

bool test_heap_insert_check_task(ParamBase* arg)
{
	HeapInsert *pArgs = (HeapInsert*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE, pArgs->entrysetId);

		GetDataFromEntrySet(pArgs->vQueryData,pArgs->GetTransaction(),pEntrySet);

		pArgs->SetSuccessFlag(pArgs->vInsertData == pArgs->vQueryData);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}

bool test_heap_insert_one_record()
{
	//测试插入一行数据是否正确
	INITRANID()
	HeapInsert *arg = new HeapInsert;
	arg->vInsertData.push_back("test_heap_insert_data");
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_insert_create_task, arg);
	REGTASK(test_heap_insert_task, arg);
	REGTASK(test_heap_insert_check_task, arg);
	REGTASK(test_heap_create_drop_task, arg);
	
	return true;
}

#define DATAROWS 10
bool test_heap_insert_multi_records()
{
	//测试插入多行数据是否正确
	INITRANID()
	HeapInsert *arg = new HeapInsert;
	arg->vInsertData.push_back("test_heap_insert_data_01");
	arg->vInsertData.push_back("test_heap_insert_data_02");
	arg->vInsertData.push_back("test_heap_insert_data_03");
	arg->vInsertData.push_back("test_heap_insert_data_11");
	arg->vInsertData.push_back("test_heap_insert_data_45");
	arg->vInsertData.push_back("test_heap_insert_data_47");
	arg->vInsertData.push_back("test_heap_insert_data_75");
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_insert_task, arg);
	REGTASK(test_heap_insert_check_task, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

#define test_insert_long_data_len_1020 "testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
estinserttestinserttestinserttestinserttestitestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
estinserttestinserttestinserttestinserttestitestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
estinserttestinserttestinserttestinserttestitestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
estinserttestinserttestinserttestinserttesti"
bool test_heap_insert_large_data()
{
	//测试插入数据长度较大时是否正确
	INITRANID()
	HeapInsert *arg = new HeapInsert;
	arg->vInsertData.push_back(test_insert_long_data_len_1020);
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_insert_task, arg);
	REGTASK(test_heap_insert_check_task, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_insert_with_null_chars()
{
	//测试插入数据中包含空串的情况
	INITRANID()
		HeapInsert *arg = new HeapInsert;
	arg->vInsertData.push_back("");
	arg->vInsertData.push_back("\0\0\0\0\0");
	arg->vInsertData.push_back("\0\0abcde\0\0");
	arg->vInsertData.push_back("abc\0\0de\0\0");
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_insert_task, arg);
	REGTASK(test_heap_insert_check_task, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_insert_many_records()
{
	//测试插入大量数据情况
	INITRANID()
	HeapInsert *arg = new HeapInsert;
	const int DATA_ROW = 10000;
	for (int index = 0; index < DATA_ROW; index ++)
	{
		arg->vInsertData.push_back("test_data_data");
	}
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_insert_task, arg);
	REGTASK(test_heap_insert_check_task, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;

}