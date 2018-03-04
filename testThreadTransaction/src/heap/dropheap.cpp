#include "heap/test_heap_create.h"
#include "heap/createheap.h"

bool test_heap_create_wait(ParamBase* arg)
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
bool test_pg_stat_heap_drop(ParamBase* arg)
{
	//MySleep(100000);
	HeapBase *pArgs = (HeapBase*)arg;
	//std::cout<<">>>>>>>>>>>>"<<pArgs->strCaseName<<"<<<<<<<<<<<<<"<<std::endl;
	try
	{
		StorageEngine::getStorageEngine()->removeEntrySet(pArgs->GetTransaction(),pArgs->entrysetId);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;

		//MemLog.Print(">>>>>>>>>>>>>>>>>>Drop failure heap id : %d \n",pArgs->entrysetId);

		//MemLog.Print("Print End !\n");
		//MemLog.Save("E:\\log.txt");

		arg->SetSuccessFlag(false);
	}

	return true;
}
bool test_pgstat_trans1()
{
	INITRANID()
	HeapBase* arg = new HeapBase;
	REGTASK(test_heap_create_wait, arg);
	REGTASK(test_pg_stat_heap_drop,arg);

	return true;
}

bool test_heap_create_wait_long(ParamBase* arg)
{
	//MySleep(30000);
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
bool test_heap_pgstat_drop(ParamBase* arg)
{
	//MySleep(30000);
	HeapBase *pArgs = (HeapBase*)arg;
	try
	{
		StorageEngine::getStorageEngine()->removeEntrySet(pArgs->GetTransaction(),pArgs->entrysetId);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;

		arg->SetSuccessFlag(false);
	}

	return true;
}
bool test_pgstat_trans2()
{
	INITRANID()
	HeapBase* arg = new HeapBase;
	REGTASK(test_heap_create_wait_long, arg);
	REGTASK(test_heap_pgstat_drop,arg);

	return true;
}