#include "test_utils/test_utils.h"
#include "heap/test_heap_create.h"
#include "heap/createheap.h"

EntrySetID g_eid = InvalidEntrySetID;

bool test_the_same_heap_operate_create_t(ParamBase* arg)
{
	HeapOperate* pArgs = (HeapOperate*)arg;

	EntrySetID eid = InvalidEntrySetID;
	try
	{
		eid = StorageEngine::getStorageEngine()->createEntrySet(arg->GetTransaction(),GetSingleColInfo());
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
	}

	if (InvalidEntrySetID == eid)
		arg->SetSuccessFlag(false);

	g_eid = eid;
	return true;
}

void test_heap_generate_data(std::vector<std::string>& vData)
{
	const int DATA_ROW_NUM = 10;
	const int DATA_ROW_LEN = 10;
	DataGenerater dg(DATA_ROW_NUM,DATA_ROW_LEN);
	dg.dataGenerate();
	for (int index = 0; index < DATA_ROW_NUM; index ++)
	{
		std::string str(dg[index],DATA_ROW_LEN);
		vData.push_back(str);
	}
}

bool test_the_same_heap_operate_insert_t(ParamBase* arg)
{
	HeapOperate *pArgs = (HeapOperate*)arg;
	try
	{
		if (InvalidEntrySetID == g_eid)
		{
			pArgs->SetSuccessFlag(false);
			return true;
		}

		test_heap_generate_data(pArgs->vInsert);

		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,g_eid);

		InsertData(pArgs->GetTransaction(), pEntrySet,pArgs->vInsert);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
const int TRANS_WAIT_SAME_HEAP = GetTransWaitId();
bool test_the_same_heap_operate_create()
{
	INITRANID()
	HeapOperate* arg = new HeapOperate;
	arg->strCaseName = __FUNCTION__;
	REGTASKSEQ(test_the_same_heap_operate_create_t, arg, 1,TRANS_WAIT_SAME_HEAP);

	return true;
}

bool test_the_same_heap_operate_insert()
{
	INITRANID()
	HeapOperate* arg = new HeapOperate;
	REGTASKSEQ(test_the_same_heap_operate_insert_t, arg, 2,TRANS_WAIT_SAME_HEAP);

	return true;
}

bool test_the_same_heap_operate_select_t(ParamBase* arg)
{
	HeapOperate *pArgs = (HeapOperate*)arg;
	try
	{
		if (InvalidEntrySetID == g_eid)
		{
			pArgs->SetSuccessFlag(false);
			return true;
		}

		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,g_eid);

		EntrySetScan *pHeapScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(),BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);

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
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_the_same_heap_operate_select()
{
	INITRANID()
	HeapOperate* arg = new HeapOperate;
	REGTASKSEQ(test_the_same_heap_operate_select_t, arg,2,TRANS_WAIT_SAME_HEAP);

	return true;
}

bool test_the_same_heap_operate_update_t(ParamBase* arg)
{
	HeapOperate *pArgs = (HeapOperate*)arg;
	try
	{
		if (InvalidEntrySetID == g_eid)
		{
			pArgs->SetSuccessFlag(false);
			return true;
		}

        test_heap_generate_data(pArgs->vUpdate);

		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,g_eid);
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(),BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);
		DataItem item;
		EntryID eid;
		std::map<EntryID,std::string> mapData;
		while (SUCCESS == pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string str((char*)item.getData(),item.getSize());
			mapData.insert(make_pair(eid,str));
		}


		UpdateData(pArgs->GetTransaction(), pEntrySet,mapData);

		pEntrySet->endEntrySetScan(pScan);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_the_same_heap_operate_update()
{
	INITRANID()
	HeapOperate* arg = new HeapOperate;
	REGTASKSEQ(test_the_same_heap_operate_update_t, arg,2,TRANS_WAIT_SAME_HEAP);

	return true;
}

bool test_the_same_heap_operate_delete_t(ParamBase* arg)
{
	HeapOperate *pArgs = (HeapOperate*)arg;
	try
	{
		if (InvalidEntrySetID == g_eid)
		{
			pArgs->SetSuccessFlag(false);
			return true;
		}

		test_heap_generate_data(pArgs->vUpdate);

		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,g_eid);

		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(),BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);
		DataItem item;
		EntryID eid;
		std::vector<EntryID> vDi;
		while (SUCCESS == pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			vDi.push_back(eid);
		}

		DeleteData(pArgs->GetTransaction(), pEntrySet,vDi);

		pEntrySet->endEntrySetScan(pScan);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)
	return true;
}

bool test_the_same_heap_operate_delete()
{
	INITRANID()
	HeapOperate* arg = new HeapOperate;
	REGTASKSEQ(test_the_same_heap_operate_delete_t, arg, 2,TRANS_WAIT_SAME_HEAP);

	return true;
}

bool test_the_same_heap_operate_drop_t(ParamBase* arg)
{
	HeapOperate *pArgs = (HeapOperate*)arg;
	try
	{
		if (InvalidEntrySetID == g_eid)
		{
			pArgs->SetSuccessFlag(false);
			return true;
		}
		StorageEngine::getStorageEngine()->removeEntrySet(pArgs->GetTransaction(),g_eid);
		g_eid = InvalidEntrySetID;
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)
	return true;
}
bool test_the_same_heap_operate_drop()
{
	INITRANID()
	HeapOperate* arg = new HeapOperate;
	REGTASKSEQ(test_the_same_heap_operate_drop_t, arg,3,TRANS_WAIT_SAME_HEAP);

	return true;
}