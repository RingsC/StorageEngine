#include <iostream>
#include "heap/test_heap_delete.h"
#include "heap/test_heap_create.h"

using namespace FounderXDB::StorageEngineNS;

bool test_heap_delete_insert(ParamBase* arg)
{
	HeapDelete* pArgs = (HeapDelete*)arg;
	pArgs->vInsertData.push_back("abcdef");
	pArgs->vInsertData.push_back("123456");
	pArgs->vInsertData.push_back("!!@#$");
	pArgs->vInsertData.push_back("desssdf");
	pArgs->vInsertData.push_back("fgeehhdd4");
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		InsertData(pArgs->GetTransaction(),pEntrySet,pArgs->vInsertData);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);

		pArgs->SetSuccessFlag(vResult == pArgs->vInsertData);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs);
	return true;
}
bool test_heap_delete_tuple_task(ParamBase* arg)
{
	HeapDelete* pArgs = (HeapDelete*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		int nIndex = 0;
		EntryID eid;
		DataItem item;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(), BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);
		while(0 == pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			if (nIndex == pArgs->nDelNum)
			{
				pEntrySet->deleteEntry(pArgs->GetTransaction(),eid);

				//erase the corresponding element in arg vector
				for (int i = 0; i < pArgs->vInsertData.size(); i ++)
				{
					if (i == nIndex)
					{
						std::string str = pArgs->vInsertData.at(i);
						std::vector<std::string>::iterator pos;
						pos = std::find(pArgs->vInsertData.begin(),pArgs->vInsertData.end(),str);
						pArgs->vInsertData.erase(pos);
					}
				}
			}

			nIndex ++;			
		}
		
		command_counter_increment();

		pEntrySet->endEntrySetScan(pScan);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs);
	return true;
}

bool test_heap_delete_check(ParamBase* arg)
{
	HeapDelete* pArgs = (HeapDelete*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);
		

		pArgs->SetSuccessFlag(vResult == pArgs->vInsertData);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs);
	return true;
}
bool test_heap_delete_first_tuple()
{
	//总共五条记录，delete第一个tuple
	INITRANID()
	HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 0;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_delete_first_tuple_ex()
{
	//总共五条记录，delete第一个tuple
	INITRANID()
		HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 0;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_delete_second_tuple()
{
	//总共五条记录，delete第一个tuple
	INITRANID()
	HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 1;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_delete_second_tuple_ex()
{
	//总共五条记录，delete第一个tuple
	INITRANID()
		HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 1;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_delete_middle_tuple()
{
	//总共五条记录，删除第三个记录
	INITRANID()
	HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 2;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_delete_middle_tuple_ex()
{
	//总共五条记录，删除第三个记录
	INITRANID()
		HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 2;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_delete_forth_tuple()
{
	//总共五条记录，delete第一个tuple
	INITRANID()
	HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 3;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_delete_forth_tuple_ex()
{
	//总共五条记录，delete第一个tuple
	INITRANID()
		HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 3;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_delete_the_last_tuple()
{
	//总共五条记录，删除最后一条
	INITRANID()
	HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 4;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_delete_the_last_tuple_ex()
{
	//总共五条记录，删除最后一条
	INITRANID()
		HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 4;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_delete_the_last_tuple_ex_1()
{
	//总共五条记录，删除最后一条
	INITRANID()
		HeapDelete *arg = new HeapDelete;
	arg->nDelNum = 4;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_delete_insert, arg);
	REGTASK(test_heap_delete_tuple_task, arg);
	REGTASK(test_heap_delete_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}