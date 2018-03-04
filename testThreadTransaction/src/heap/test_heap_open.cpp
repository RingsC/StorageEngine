#include "heap/test_heap_create.h"
#include "test_utils/test_utils.h"

bool test_heap_open_repeat_t(ParamBase* arg)
{
	HeapBase *pArgs = (HeapBase*)arg;
	try
	{
		const int MAX = 1;
		for (int index =0; index < MAX; index ++)
		{
			EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

			StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_open_repeat()
{
	//连续用openEntrySet打开同一个表，经测试发现，打开多少次表必须关闭多少次表，不然会报错
	//这个现象是正常的，因为openEntrySet有引用计数的功能，打开表就是为了使用它

	INITRANID()
	HeapBase* arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_open_repeat_t, arg);
	REGTASK(test_heap_create_drop_task, arg);
	return true;
}

static EntrySetID g_entrysetId = InvalidEntrySetID;
EntrySetID heap_create_in_transaction()
{
	EntrySetID entrySetId = InvalidEntrySetID;
	TransactionId xid = InvalidTransactionID;
	Transaction* pTrans = StorageEngine::getStorageEngine()->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		entrySetId = StorageEngine::getStorageEngine()->createEntrySet(pTrans,GetSingleColInfo());
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	g_entrysetId = entrySetId;
	return entrySetId;
}

bool test_heap_open_task(ParamBase* arg)
{
	HeapBase *pArgs = (HeapBase*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

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
bool test_heap_open_insert_task(ParamBase* arg)
{
	HeapBase *pArgs = (HeapBase*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		std::vector<std::string> vData;
		vData.push_back("abcde");
		vData.push_back("ddedd");
		vData.push_back("d855dd");

		InsertData(pArgs->GetTransaction(), pEntrySet,vData);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

		return true;
}

bool test_heap_open()
{
	INITRANID()
	HeapBase* arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_open_task, arg);
	REGTASK(test_heap_open_insert_task,arg);
	REGTASK(test_heap_create_drop_task, arg);
	return true;
}
bool test_heap_open_01()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = heap_create_in_transaction();
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
		REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_02()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
			REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_03()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
			REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_04()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
		REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_05()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
			REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_06()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
			REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_07()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
			REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_08()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
			REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_09()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
			REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_10()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
			REGTASK(test_heap_open_task,arg);
	}

	return true;
}
bool test_heap_open_11()
{
	HeapBase* arg = new HeapBase;
	arg->entrysetId = arg->entrysetId = g_entrysetId;
	if (InvalidEntrySetID != arg->entrysetId)
	{
		INITRANID()
			REGTASK(test_heap_open_task,arg);
	}

	return true;
}