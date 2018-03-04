#include "heap/test_heap_create.h"
#include "test_utils/test_utils.h"
#include "test_fram.h"


using namespace FounderXDB::StorageEngineNS;


#define testdata "testdata_1"
bool test_heap_close_insert(ParamBase* arg)
{
	HeapBase *pArgs = (HeapBase*)arg;
	try
	{
		EntrySet *pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		DataItem data;
		int len = sizeof(testdata);
		char *pstr = (char *)malloc(len);
		memcpy(pstr, testdata, len);
		data.setData((void *)pstr);
		data.setSize(len);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);

		EntryID tid;
		pEntrySet->insertEntry(pArgs->GetTransaction(), tid, data);//插入数据
		command_counter_increment();		
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_close_check_result(ParamBase* arg)
{
	HeapBase *pArgs = (HeapBase*)arg;
	try
	{
		EntrySet *pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);
		EntrySetScan *pEntrySetScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(), BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);

		DataItem getdata;
		
		EntryID eid;
		int flag = pEntrySetScan->getNext(EntrySetScan::NEXT_FLAG,eid,getdata);
		if (flag == NO_DATA_FOUND)
		{
			std::cout<<"检索至最后的数据了!"<<std::endl;
			pArgs->SetSuccessFlag(false);	
		}
		char *str=(char*)getdata.getData();//getdata中的数据给str
		int length=getdata.getSize();//大小
		flag = memcmp(testdata,str,sizeof(testdata));
		if (flag != 0)
		{
			std::cout<<"比较的数据不一致!"<<std::endl;
			pArgs->SetSuccessFlag(false);;
		}
		pEntrySet->endEntrySetScan(pEntrySetScan);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_close_then_insert()
{
	//测试closeEntrySet之后是否还能往表中插入数据,测试发现还能插入数据
	//而且能够查出来,最后的时候也不需要closeEntrySet

	INITRANID()
	HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task ,arg);
	REGTASK(test_heap_close_insert, arg);
	REGTASK(test_heap_close_check_result, arg);
	REGTASK(test_heap_create_drop_task, arg);
	return true;
}

bool test_heap_close_null_entryset_t(ParamBase* arg)
{
	try
	{
		EntrySet *pEntrySet = NULL;
		StorageEngine::getStorageEngine()->closeEntrySet(arg->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(arg)

	return true;
}
bool test_heap_close_null_entryset()
{
	//测试没有openEntrySet的情况下直接close,penry_set为NULL,测试通过

	INITRANID()
	ParamBase *arg = new ParamBase;
	REGTASK(test_heap_close_null_entryset_t ,arg);
	return true;
}


