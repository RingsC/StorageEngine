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
		pEntrySet->insertEntry(pArgs->GetTransaction(), tid, data);//��������
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
			std::cout<<"����������������!"<<std::endl;
			pArgs->SetSuccessFlag(false);	
		}
		char *str=(char*)getdata.getData();//getdata�е����ݸ�str
		int length=getdata.getSize();//��С
		flag = memcmp(testdata,str,sizeof(testdata));
		if (flag != 0)
		{
			std::cout<<"�Ƚϵ����ݲ�һ��!"<<std::endl;
			pArgs->SetSuccessFlag(false);;
		}
		pEntrySet->endEntrySetScan(pEntrySetScan);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_close_then_insert()
{
	//����closeEntrySet֮���Ƿ��������в�������,���Է��ֻ��ܲ�������
	//�����ܹ������,����ʱ��Ҳ����ҪcloseEntrySet

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
	//����û��openEntrySet�������ֱ��close,penry_setΪNULL,����ͨ��

	INITRANID()
	ParamBase *arg = new ParamBase;
	REGTASK(test_heap_close_null_entryset_t ,arg);
	return true;
}


