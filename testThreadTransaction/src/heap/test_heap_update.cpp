#include <iostream>
#include <list>
#include <algorithm>

#include "heap/test_heap_create.h"
#include "heap/test_heap_update.h"

using namespace FounderXDB::StorageEngineNS;

bool test_heap_update_insert(ParamBase* arg)
{
	HeapUpdate *pArgs = (HeapUpdate*)arg;
	try
	{
		pArgs->vData.push_back("abcdef");
		pArgs->vData.push_back("12345");
		pArgs->vData.push_back("+-*/");

		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(), EntrySet::OPEN_EXCLUSIVE, pArgs->entrysetId);

		InsertData(pArgs->GetTransaction(),pEntrySet,pArgs->vData);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);

		pArgs->SetSuccessFlag(vResult == pArgs->vData);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_update_modify(ParamBase* arg)
{
	HeapUpdate *pArgs = (HeapUpdate*)arg;
	try
	{

		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(), EntrySet::OPEN_EXCLUSIVE, pArgs->entrysetId);

		EntrySetScan *pEntrySetScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(), BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);

		pArgs->vData.clear();

		std::map<EntryID,std::string> mUpData;
		std::string strBase = "asdf";
		EntryID eid;
		DataItem item;
		while (pEntrySetScan->getNext(EntrySetScan::NEXT_FLAG,eid,item) == 0)
		{
			strBase += "ee";
			mUpData.insert(std::make_pair(eid,strBase));
			pArgs->vData.push_back(strBase);
		}
		pEntrySet->endEntrySetScan(pEntrySetScan);

		UpdateData(pArgs->GetTransaction(),pEntrySet,mUpData);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);

		pArgs->SetSuccessFlag(vResult == pArgs->vData);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_update_check(ParamBase* arg)
{
	HeapUpdate *pArgs = (HeapUpdate*)arg;
	try
	{

		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(), EntrySet::OPEN_EXCLUSIVE, pArgs->entrysetId);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);

		pArgs->SetSuccessFlag(vResult == pArgs->vData);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_update_open_task(ParamBase* arg)
{
	HeapUpdate *pArgs = (HeapUpdate*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_SHARED,pArgs->entrysetId);

		//EntryID eid;
		//DataItem item;
		//std::string str = "abcd";
		//item.setData((char*)str.c_str());
		//item.setSize(str.size());
		//pEntrySet->insertEntry(pArgs->GetTransaction(),eid,item);
		//StorageEngine::getStorageEngine()->endStatement();

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)
	return true;
}

bool test_heap_update_normal()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
	HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	//REGTASK(test_heap_update_open_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_1()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_2()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_3()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_4()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_5()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_6()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_7()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_8()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_9()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_normal_10()
{
	//插入heap三条记录，然后全部更新，比较结果
	INITRANID()
		HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_modify, arg);
	REGTASK(test_heap_update_check, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool test_heap_update_many_times_t(ParamBase* arg)
{
	bool bException = false;
	HeapUpdate *pArgs = (HeapUpdate*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(), EntrySet::OPEN_EXCLUSIVE, pArgs->entrysetId);

		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(), BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);
		EntryID eid;
		DataItem item;
		int flag = pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item);
		pEntrySet->endEntrySetScan(pScan);

		if (NO_DATA_FOUND == flag)
		{
			std::cout<<"no data found in heap !"<<std::endl;
		}
		else
		{
			DataItem item;
			char* pData = (char*)malloc(sizeof("adddded")+1);
			memset(pData,0,sizeof("adddded")+1);
			item.setData(pData);
			item.setSize(sizeof("adddded"));
			const int count = 10;
			for (int index =0; index < count; index ++)
			{
				pEntrySet->updateEntry(pArgs->GetTransaction(),eid,item);
				if (pArgs->bIncrement)
				{
					command_counter_increment();
				}
			}
		}

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		bException = true;
	}

	if (!bException)
		pArgs->SetSuccessFlag(false);

	return true;
}
bool test_heap_update_too_many_times()
{
	//update一条记录过于频繁
	INITRANID()
	HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	arg->bIncrement = false;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_many_times_t, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_too_many_times_ex()
{
	//update一条记录过于频繁
	INITRANID()
	HeapUpdate *arg = new HeapUpdate;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_many_times_t, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}
bool test_heap_update_too_many_times_increment()
{
	//update一条记录过于频繁
	INITRANID()
	HeapUpdate *arg = new HeapUpdate;
	arg->bIncrement = true;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_update_insert, arg);
	REGTASK(test_heap_update_many_times_t, arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}