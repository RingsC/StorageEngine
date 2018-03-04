#include "perform_bulk_insert.h"
#include "perform_utils.h"
#include "StorageEngine.h"
#include <boost/timer.hpp>

bool my_bulk_insert(EntrySetID entrySetId, const std::vector<DataItem>& vData)
{
	StorageEngine* pStorageEngine = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		int nSize = pEntrySet->insertEntries(pTrans,vData);

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}
bool my_normal_insert(EntrySetID entrySetId, const std::vector<DataItem>& vData)
{
	StorageEngine* pStorageEngine = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		EntryID eid;
		for (uint32 index = 0; index < vData.size(); index ++)
		{
			pEntrySet->insertEntry(pTrans,eid,vData.at(index));
		}

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}
bool perform_bulk_insert(bool bUseBulk, bool bIndex)
{
	bool bRet = true;
	//create heap
	CPerformBase create;
	uint32 colid = 0;
	EntrySetID entrySetId = create.Create_EntrySet(false,bIndex,colid);
	if (InvalidEntrySetID == entrySetId)
		return false;

	//prepare data
	uint32 nTupleCount = GetDataCount();
	uint32 nTupleLen = GetTupleLen();
	
	const uint32 nCount = 128*1024*1024/nTupleLen;
	uint32 nFactor = nTupleCount/nCount;
	uint32 nRemainder = nTupleCount%nCount;

	double dTotalTime = 0.0;

	TestData data;
	data.tuple_count = nCount;
	data.tuple_len = nTupleLen;
	GetTestDataPtr(data);

	//nFactor
	for (uint32 i = 0; i < nFactor; i ++)
	{
		char* pData = data.pData;
		std::vector<DataItem> vData;
		DataItem item;
		for (uint32 j = 0; j < data.tuple_count; j ++)
		{
			assert(vData.max_size() >= data.tuple_count);

			item.setData(pData);
			item.setSize(data.tuple_len);
			vData.push_back(item);

			pData += data.tuple_len;
		}
		pData = data.pData;

		boost::timer t;
		if (bUseBulk)
		{
			bRet = my_bulk_insert(entrySetId,vData);
		}
		else
		{
			bRet = my_normal_insert(entrySetId,vData);
		}
		dTotalTime += t.elapsed();
	}

	//认为nRemainder等于零，插入数据都是128M的倍数
	assert(nRemainder == 0);

	//bulk insert data
	std::string strFlag = "HEAP_NORMAL_INSERT";
	if(bUseBulk)
		strFlag = "HEAP_MULTI_INSERT";

	if (bIndex)
	{
		strFlag += "_INDEX";
	}
	create.WriteStatInfo2File_Ex(strFlag,dTotalTime);

	//clear task
	bool bClear = create.ClearTask();

	return bRet&&bClear;
}

bool test_perform_bulk_insert()
{
	return perform_bulk_insert(true,false);
}

bool test_perform_bulk_insert_index()
{
	return perform_bulk_insert(true,true);
}

bool test_perform_normal_insert()
{
	return perform_bulk_insert(false,false);
}
bool test_perform_normal_insert_index()
{
	return perform_bulk_insert(false,true);
}