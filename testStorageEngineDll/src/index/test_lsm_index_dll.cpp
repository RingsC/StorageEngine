#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/thread/thread.hpp>
#include <boost/timer.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "EntryIDBitmap.h"
#include "FDBitmapSet.h"


using namespace FounderXDB::StorageEngineNS;
using std::cout;
using std::endl;

class LsmTimer
{
public:
	LsmTimer()
	{
	#ifndef WIN32
		gettimeofday(&startTime,NULL);
	#endif
	}

	void restart(void)
	{
	#ifndef WIN32
		gettimeofday(&startTime,NULL);
	#else
		m_time.restart();
	#endif
	}

	double elapsed(void)
	{
	#ifndef WIN32
		gettimeofday(&endTime, NULL);
		double use_s = endTime.tv_sec - startTime.tv_sec;
		double use_us = endTime.tv_usec - startTime.tv_usec;
		return (use_s + use_us/1000000);
	#else
		return m_time.elapsed();
	#endif
	}
private:
#ifdef WIN32
	boost::timer m_time;
#else
	struct timeval startTime;
	struct timeval endTime;
#endif
};

static uint32 s_lsm_heap_colid = 0;
static uint32 s_lsm_idx_colid = 0;

static void testlsmdll_HeapSplit(RangeDatai& range, const char *str, int col, size_t len = 0)
{
	assert(len > sizeof(uint32));

	if(col == 1)
	{
		range.start = 0;
		range.len = sizeof(uint32);
	}

	if(col == 2)
	{
		range.start = sizeof(uint32);
		range.len = len - sizeof(uint32);
	}
}

static void testlsmdll_IndexSplit(RangeDatai& range, const char *str, int col, size_t len = 0)
{
	assert(len == sizeof(uint32));

	if (1 == col)
	{
		range.start = 0;
		range.len = len;
	}
}

static int testlsmdll_KeyCompare(const char* a, size_t len1, const char* b, size_t len2)
{
	uint32 cmp_1 = *(uint32*)a;
	uint32 cmp_2 = *(uint32*)b;

	return cmp_1 - cmp_2;
}

void testlsmdll_SetColid(void)
{
	if((s_lsm_heap_colid != 0) && (s_lsm_idx_colid != 0)){
		return;
	}

	s_lsm_heap_colid = 565656;
	s_lsm_idx_colid = 565657;

	ColumnInfo *pcol_info = new ColumnInfo;
	pcol_info->keys = 2;
	pcol_info->col_number = NULL;
	pcol_info->rd_comfunction = NULL;
	pcol_info->split_function = testlsmdll_HeapSplit;
	setColumnInfo(s_lsm_heap_colid, pcol_info);

	pcol_info = new ColumnInfo;
	pcol_info->keys = 1;
	pcol_info->col_number = (size_t*)malloc(pcol_info->keys * sizeof(size_t));
	pcol_info->col_number[0] = 1;
	pcol_info->rd_comfunction = (CompareCallbacki*)malloc(pcol_info->keys * sizeof(CompareCallbacki));
	pcol_info->rd_comfunction[0] = testlsmdll_KeyCompare;
	pcol_info->split_function = testlsmdll_IndexSplit;
	setColumnInfo(s_lsm_idx_colid, pcol_info);
}

static void testlsmdll_BuildData(uint32 key, const char *value, std::string& data)
{
	data.append((const char *)&key, sizeof(uint32));
	data.append(value);
}

static void testlsmdll_PrintData(const char *name, const char *data, uint32 length)
{
	uint32 data_key = *((uint32 *)data);
	const char *data_value = (data + sizeof(uint32));
	uint32 data_value_size = length - sizeof(uint32);

	cout << name << ": key[" << data_key << "], value[" << data_value
		<< "], Size[" << data_value_size << "]"<< endl;
}

bool testlsmdll_QueryOneData(Transaction *pTrans, IndexEntrySet *pLsmIdx,
	const std::string& data, EntryID& eid)
{
	uint32 data_key = *((uint32 *)data.c_str());

	ScanCondition condition(1, ScanCondition::Equal, (se_uint64)&data_key, sizeof(uint32), testlsmdll_KeyCompare);
	std::vector<ScanCondition> vConditions;
	vConditions.push_back(condition);
	EntrySetScan *pScan = pLsmIdx->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, vConditions);

	DataItem entry;
	pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry);
	pLsmIdx->endEntrySetScan(pScan);

	testlsmdll_PrintData("Data", data.c_str(), data.length());
	testlsmdll_PrintData("ResultData", (char *)entry.getData(), entry.getSize());

	bool ret = true;
	if((data.length() != entry.getSize())
	|| (memcmp(data.c_str(), entry.getData(), data.length()) != 0))
	{
		ret = false;
	}

	MemoryContext::deAlloc(entry.getData());
	return ret;
}

bool testlsmdll_QueryAllData(Transaction *pTrans, IndexEntrySet *pLsmIdx,
	const std::multiset<std::string>& expectedDataSet)
{
	std::vector<ScanCondition> vConditions;
	EntrySetScan *pScan = pLsmIdx->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, vConditions);

	std::multiset<std::string> resultDataSet;
	EntryID eid;
	DataItem entry;
	while(0 == pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry))
	{
		resultDataSet.insert(std::string((char*)entry.getData(), entry.getSize()));
		MemoryContext::deAlloc(entry.getData());
	}
	pLsmIdx->endEntrySetScan(pScan);

	return (resultDataSet == expectedDataSet);
}

bool testlsmdll_QueryAllDataByBitmap(Transaction *pTrans, IndexEntrySet *pLsmIdx,
	const std::multiset<std::string>& expectedDataSet)
{
	std::vector<ScanCondition> vConditions;
	IndexEntrySetScan *pScan = (IndexEntrySetScan*)pLsmIdx->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, vConditions);

	std::multiset<std::string> resultDataSet;

	/* use bitmap */
	EntryIDBitmap *pBitmap = pScan->getBitmap(pTrans);
	EntryIDBitmapIterator *bitmapIt = pBitmap->beginIterate();

	EntryID eid;
	DataItem entry;
	while(NO_DATA_FOUND != bitmapIt->getNext(eid, entry))
	{
		resultDataSet.insert(std::string((char*)entry.getData(), entry.getSize()));
	}

	pBitmap->endIterate(bitmapIt);
	pLsmIdx->endEntrySetScan(pScan);

	return (resultDataSet == expectedDataSet);
}

bool testlsmdll_Operation(void)
{
	testlsmdll_SetColid();

	std::string data_insert;
	std::string data_update_new;

	testlsmdll_BuildData(1, "data_insert", data_insert);
	testlsmdll_BuildData(1, "data_update", data_update_new);

	StorageEngine *pSE = StorageEngine::getStorageEngine();
	TransactionId xid = 0;
	Transaction *pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	EntrySetID heapId;
	EntrySetID lsmIdxId;

	try
	{
		/* create heap */
		heapId = pSE->createEntrySet(pTrans, s_lsm_heap_colid);

		/* create index */
		EntrySetType lsmIdxType = LSM_INDEX_ENTRY_SET_TYPE;
		//EntrySetType lsmIdxType = BTREE_INDEX_ENTRY_SET_TYPE;
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_EXCLUSIVE, heapId);
		lsmIdxId = pSE->createIndexEntrySet(pTrans, pEntrySet, lsmIdxType, s_lsm_idx_colid);
		pSE->closeEntrySet(pTrans, pEntrySet);
		IndexEntrySet *pLsmIdx = pSE->openIndexEntrySet(pTrans, pEntrySet, EntrySet::OPEN_EXCLUSIVE, lsmIdxId);

		/* insert */
		{
			DataItem data;
			data.setData((void*)data_insert.c_str());
			data.setSize(data_insert.length());
			EntryID eid;
			pEntrySet->insertEntry(pTrans, eid, data);
			command_counter_increment();
		}

		/* update */
		{
			EntryID eid;
			if(!testlsmdll_QueryOneData(pTrans, pLsmIdx, data_insert, eid)){
				throw false;
			}

			DataItem data;
			data.setData((void*)data_update_new.c_str());
			data.setSize(data_update_new.length());
			pEntrySet->updateEntry(pTrans, eid, data);
			command_counter_increment();
		}

		/* delete */
		{
			EntryID eid;
			if(!testlsmdll_QueryOneData(pTrans, pLsmIdx, data_update_new, eid)){
				throw false;
			}

			DataItem data;
			data.setData((void*)data_update_new.c_str());
			data.setSize(data_update_new.length());
			pEntrySet->deleteEntry(pTrans, eid);
			command_counter_increment();
		}

		/* query */
		{
			std::multiset<std::string> expectedDataSet;
			if(!testlsmdll_QueryAllData(pTrans, pLsmIdx, expectedDataSet))
			{
				throw false;
			}
		}

		/* query by bitmap */
		{
			std::multiset<std::string> expectedDataSet;
			if(!testlsmdll_QueryAllDataByBitmap(pTrans, pLsmIdx, expectedDataSet))
			{
				throw false;
			}
		}

		/* remove entry set */
		pSE->removeIndexEntrySet(pTrans, heapId, lsmIdxId);
		pSE->removeEntrySet(pTrans, heapId);
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	} 
	catch(bool& ret)
	{
		pTrans->abort();
		delete pTrans;
		return ret;
	}
	pTrans->commit();
	return true;
}

static const uint32 s_max_data = 10000;
static bool testlsmdll_InitInsertRandArray(uint32 *insertRandArray,
	bool expectedExist, uint32 *pTotalCount)
{
	memset(insertRandArray, 0, s_max_data*sizeof(uint32));

	std::ifstream infile("insertlog.txt");
	bool fileExist = infile.is_open();

	if(expectedExist && fileExist)
	{
		pTotalCount = 0;
		for(uint32 i = 0; i < s_max_data; i++)
		{
			infile >> insertRandArray[i];
			pTotalCount += insertRandArray[i];
		}

		return true;
	}

	if((!expectedExist) && (!fileExist))
	{
		return true;
	}

	cout << "Expected insertlog.txt " << (expectedExist ? "exist" : "not exist")
		<< ", but " << (fileExist ? "exist" : "not exist") << std::endl;
	return false;
}

static void testlsmdll_SaveInsertRandArray(uint32 insertRandArray[s_max_data])
{
	std::ofstream outfile("insertlog.txt");
	for(uint32 i = 0; i < s_max_data; i++)
	{
		outfile << insertRandArray[i] << endl;
	}
}

bool __testlsmdll_InsertBigDataFirst(uint32 *insertRandArray,
	uint32 insert_count, EntrySetID& heapId, EntrySetID& lsmIdxId)
{
	if((insertRandArray != NULL)
	&& (!testlsmdll_InitInsertRandArray(insertRandArray, false, NULL)))
	{
		return false;
	}

	StorageEngine *pSE = StorageEngine::getStorageEngine();
	TransactionId xid;
	Transaction *pTrans;

	/* create heap and index */
	xid = 0;
	pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try{
		/* create heap */
		heapId = pSE->createEntrySet(pTrans, s_lsm_heap_colid);

		/* create index */
		EntrySetType lsmIdxType = LSM_INDEX_ENTRY_SET_TYPE;
		//EntrySetType lsmIdxType = BTREE_INDEX_ENTRY_SET_TYPE;
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_EXCLUSIVE, heapId);
		lsmIdxId = pSE->createIndexEntrySet(pTrans, pEntrySet, lsmIdxType, s_lsm_idx_colid);
		pSE->closeEntrySet(pTrans, pEntrySet);
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	pTrans->commit();

	LsmTimer t;

	/* insert */
	xid = 0;
	pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_SHARED, heapId);
		IndexEntrySet *pLsmIdx = pSE->openIndexEntrySet(pTrans, pEntrySet, EntrySet::OPEN_SHARED, lsmIdxId);

		srand(1);
		char data_value[32];
		for(uint32 i = 0; i < insert_count; i++)
		{
			std::string data_insert;
			uint32 data_key = rand()%s_max_data; /* random */
			if(insertRandArray != NULL)
			{
				insertRandArray[data_key]++;
			}
			snprintf(data_value, 32, "data_insert%d", data_key);
			testlsmdll_BuildData(data_key, "data_insert", data_insert);

			DataItem data;
			data.setData((void*)data_insert.c_str());
			data.setSize(data_insert.length());

			EntryID eid;
			pEntrySet->insertEntry(pTrans, eid, data);
		}
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	pTrans->commit();

	double d_time = t.elapsed();
	cout << "Insert " << insert_count << " data: " << d_time << "s" << endl;

	return true;
}

bool testlsmdll_InsertBigDataFirst(void)
{
	testlsmdll_SetColid();

	uint32 insertRandArray[s_max_data];
	uint32 insert_count = 100000000;
	EntrySetID heapId;
	EntrySetID lsmIdxId;

	if(!__testlsmdll_InsertBigDataFirst(insertRandArray, insert_count,
	heapId, lsmIdxId))
	{
		return false;
	}

	testlsmdll_SaveInsertRandArray(insertRandArray);

	for(uint32 i = 0; i < 2; i++)
	{
		boost::thread::sleep(boost::get_system_time()
						+ boost::posix_time::milliseconds(3600000));
	}

	return true;
	
}

bool testlsmdll_ScanAll(void)
{
	testlsmdll_SetColid();

	/* get insert array */
	uint32 insertRandArray[s_max_data];
	if(!testlsmdll_InitInsertRandArray(insertRandArray, true, NULL))
	{
		return false;
	}

	StorageEngine *pSE = StorageEngine::getStorageEngine();
	TransactionId xid;
	Transaction *pTrans;
	EntrySetID heapId = 1025;
	EntrySetID lsmIdxId = 1026;

	/* scan */
	uint32 scanarray[s_max_data];
	memset(scanarray, 0, s_max_data*sizeof(uint32));
	xid = 0;
	pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_SHARED, heapId);
		IndexEntrySet *pLsmIdx = pSE->openIndexEntrySet(pTrans, pEntrySet, EntrySet::OPEN_SHARED, lsmIdxId);

		for(uint32 i = 0; i < 10000; i++)
		{
			ScanCondition condition(1, ScanCondition::Equal, (se_uint64)&i, sizeof(uint32), testlsmdll_KeyCompare);
			std::vector<ScanCondition> vConditions;
			vConditions.push_back(condition);
			EntrySetScan *pScan = pLsmIdx->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, vConditions);

			DataItem entry;
			EntryID eid;
			while(pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry) == 0)
			{
				MemoryContext::deAlloc(entry.getData());
				scanarray[i]++;
			}
			pLsmIdx->endEntrySetScan(pScan);
		}
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	pTrans->commit();

	/* check scan data */
	for(uint32 i = 0; i < s_max_data; i++)
	{
		if(scanarray[i] != insertRandArray[i])
		{
			cout << "scan data is not match insert data." << endl;
			return false;
		}
	}

	return true;
}

bool testlsmdll_InsertBigData(void)
{
	testlsmdll_SetColid();

	/* get insert array */
	uint32 insertRandArray[s_max_data];
	uint32 insert_count = 10000000;
	if(!testlsmdll_InitInsertRandArray(insertRandArray, true, NULL))
	{
		return false;
	}

	StorageEngine *pSE = StorageEngine::getStorageEngine();
	TransactionId xid;
	Transaction *pTrans;
	EntrySetID heapId = 1025;
	EntrySetID lsmIdxId = 1026;

	LsmTimer t;

	/* insert */
	xid = 0;
	pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_SHARED, heapId);
		IndexEntrySet *pLsmIdx = pSE->openIndexEntrySet(pTrans, pEntrySet, EntrySet::OPEN_SHARED, lsmIdxId);

		srand(1);
		char data_value[32];
		for(uint32 i = 0; i < insert_count; i++)
		{
			std::string data_insert;
			uint32 data_key = rand()%s_max_data; /* random */
			insertRandArray[data_key]++;
			snprintf(data_value, 32, "data_insert%d", data_key);
			testlsmdll_BuildData(data_key, "data_insert", data_insert);

			DataItem data;
			data.setData((void*)data_insert.c_str());
			data.setSize(data_insert.length());

			EntryID eid;
			pEntrySet->insertEntry(pTrans, eid, data);
		}
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	pTrans->commit();

	double d_time = t.elapsed();
	cout << "Insert " << insert_count << " data: " << d_time << "s" << endl;

	testlsmdll_SaveInsertRandArray(insertRandArray);

	return true;
}

static void testlsmdll_SetDataKey(uint32 insertRandArray[s_max_data],
	uint32& data_key, uint32& queryCount)
{
	uint32 maxQueryCount = 5000000;
	
	data_key = 0;
	queryCount = 0;

	for(uint32 i = s_max_data - 1; i >= 0; i--)
	{
		if((queryCount <= maxQueryCount)
		&& (queryCount + insertRandArray[i] >= maxQueryCount))
		{
			if(maxQueryCount - queryCount < insertRandArray[i])
			{
				data_key = i + 1;
				break;
			}
			else
			{
				data_key = i;
				queryCount += insertRandArray[i];
				break;
			}
		}

		queryCount += insertRandArray[i];
	}

	cout << "data_key: " << data_key << ", queryCount: " << queryCount << endl;
}

bool testlsmdll_QueryBigData(void)
{
	testlsmdll_SetColid();

	/* get insert array */
	uint32 insertRandArray[s_max_data];
	uint32 totalCount;
	if(!testlsmdll_InitInsertRandArray(insertRandArray, true, &totalCount))
	{
		return false;
	}

	uint32 data_key;
	uint32 queryCount;
	testlsmdll_SetDataKey(insertRandArray, data_key, queryCount);

	StorageEngine *pSE = StorageEngine::getStorageEngine();
	TransactionId xid;
	Transaction *pTrans;
	EntrySetID heapId = 1025;
	EntrySetID lsmIdxId = 1026;

	LsmTimer t;

	/* scan */
	uint32 count = 0;
	xid = 0;
	pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_SHARED, heapId);
		IndexEntrySet *pLsmIdx = pSE->openIndexEntrySet(pTrans, pEntrySet, EntrySet::OPEN_SHARED, lsmIdxId);

		ScanCondition condition(1, ScanCondition::GreaterEqual, (se_uint64)&data_key, sizeof(uint32), testlsmdll_KeyCompare);
		std::vector<ScanCondition> vConditions;
		vConditions.push_back(condition);
		EntrySetScan *pScan = pLsmIdx->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, vConditions);

		DataItem entry;
		EntryID eid;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry) == 0)
		{
			//testlsmdll_PrintData("ResultData", (char *)entry.getData(), entry.getSize());
			MemoryContext::deAlloc(entry.getData());
			count++;
		}
		pLsmIdx->endEntrySetScan(pScan);
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	pTrans->commit();

	double d_time = t.elapsed();
	cout << "Get " << count << "data from " << totalCount << " data: " << d_time << "s" << endl;

	if(count != queryCount)
	{
		cout << "expect count: " << queryCount << ", but get " << count << endl;
		return false;
	}

	return true;
}

bool testlsmdll_BitmapScanBigData(void)
{
	testlsmdll_SetColid();

	/* get insert array */
	uint32 insertRandArray[s_max_data];
	uint32 totalCount;
	if(!testlsmdll_InitInsertRandArray(insertRandArray, true, &totalCount))
	{
		return false;
	}

	uint32 data_key;
	uint32 queryCount;
	testlsmdll_SetDataKey(insertRandArray, data_key, queryCount);

	StorageEngine *pSE = StorageEngine::getStorageEngine();
	TransactionId xid;
	Transaction *pTrans;
	EntrySetID heapId = 1025;
	EntrySetID lsmIdxId = 1026;

	LsmTimer t;

	/* scan */
	uint32 count = 0;
	xid = 0;
	pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_SHARED, heapId);
		IndexEntrySet *pLsmIdx = pSE->openIndexEntrySet(pTrans, pEntrySet, EntrySet::OPEN_SHARED, lsmIdxId);

		ScanCondition condition(1, ScanCondition::GreaterEqual, (se_uint64)&data_key, sizeof(uint32), testlsmdll_KeyCompare);
		std::vector<ScanCondition> vConditions;
		vConditions.push_back(condition);
		IndexEntrySetScan *pScan = (IndexEntrySetScan*)pLsmIdx->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, vConditions);

		/* use bitmap */
		EntryIDBitmap *pBitmap = pScan->getBitmap(pTrans);
		EntryIDBitmapIterator *bitmapIt = pBitmap->beginIterate();

		EntryID eid;
		DataItem entry;
		while(NO_DATA_FOUND != bitmapIt->getNext(eid, entry))
		{
			MemoryContext::deAlloc(entry.getData());
			count++;
		}

		pBitmap->endIterate(bitmapIt);
		pLsmIdx->endEntrySetScan(pScan);
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	pTrans->commit();

	double d_time = t.elapsed();
	cout << "Bitmap scan " << count << "data from " << totalCount << " data: " << d_time << "s" << endl;

	if(count != queryCount)
	{
		cout << "expect count: " << queryCount << ", but get " << count << endl;
		return false;
	}

	return true;
}

bool testlsmdll_QueryByDataKey(Transaction *pTrans, IndexEntrySet *pLsmIdx,
	uint32 data_key, uint32 expected_count)
{
	uint32 data_count = 0;
	ScanCondition condition(1, ScanCondition::Equal, (se_uint64)&data_key,
				sizeof(uint32), testlsmdll_KeyCompare);
	std::vector<ScanCondition> vConditions;
	vConditions.push_back(condition);
	EntrySetScan *pScan = pLsmIdx->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, vConditions);

	DataItem entry;
	EntryID eid;
	while(0 == pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry))
	{
		data_count++;
		MemoryContext::deAlloc(entry.getData());
	}
	pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry);
	pLsmIdx->endEntrySetScan(pScan);

	return (data_count == expected_count);
}

bool testlsmdll_InternalMergeConcurrency(void)
{
	testlsmdll_SetColid();

	uint32 insertRandArray[s_max_data];
	uint32 insert_count = 10000000;
	EntrySetID heapId;
	EntrySetID lsmIdxId;

	if(!__testlsmdll_InsertBigDataFirst(insertRandArray, insert_count, heapId, lsmIdxId))
	{
		return false;
	}

	testlsmdll_SaveInsertRandArray(insertRandArray);

	for(uint32 i = 0; i < 2; i++)
	{
		boost::thread::sleep(boost::get_system_time()
						+ boost::posix_time::milliseconds(50000));
	}

	StorageEngine *pSE = StorageEngine::getStorageEngine();
	TransactionId xid;
	Transaction *pTrans;

	/* do operation concurrency with merge */
	xid = 0;
	pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_SHARED, heapId);
		IndexEntrySet *pLsmIdx = pSE->openIndexEntrySet(pTrans, pEntrySet, EntrySet::OPEN_SHARED, lsmIdxId);
#if 0
		/* query */
		uint32 update_data_key = 100;
		uint32 update_data_count = insertRandArray[update_data_key];
		uint32 update_data_new_key = s_max_data + update_data_key;
		{
			ScanCondition condition(1, ScanCondition::Equal, (se_uint64)&update_data_key, sizeof(uint32), testlsmdll_KeyCompare);
			std::vector<ScanCondition> vConditions;
			vConditions.push_back(condition);
			EntrySetScan *pScan = pLsmIdx->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, vConditions);

			DataItem entry;
			EntryID eid;
			uint32 count = 0;
			while(0 == pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry))
			{
				count++;
				std::string data_update;
				testlsmdll_BuildData(update_data_new_key, "data_update", data_update);

				DataItem data;
				data.setData((void*)data_update.c_str());
				data.setSize(data_update.length());
				pEntrySet->updateEntry(pTrans, eid, data);

				MemoryContext::deAlloc(entry.getData());
			}
			pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry);
			pLsmIdx->endEntrySetScan(pScan);

			if(count != update_data_count)
			{
				cout << "update " << count << ", expect " << update_data_count << endl;
				throw false;
			}
		}
		command_counter_increment();
		if(!testlsmdll_QueryByDataKey(pTrans, pLsmIdx, update_data_new_key, update_data_count))
		{
			throw false;
		}

		/* delete */
		uint32 delete_data_key = 200;
		uint32 delete_data_count = insertRandArray[delete_data_key];
		{
			ScanCondition condition(1, ScanCondition::Equal, (se_uint64)&delete_data_key,
				sizeof(uint32), testlsmdll_KeyCompare);
			std::vector<ScanCondition> vConditions;
			vConditions.push_back(condition);
			EntrySetScan *pScan = pLsmIdx->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, vConditions);

			DataItem entry;
			EntryID eid;
			uint32 count = 0;
			while(0 == pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry))
			{
				count++;
				pEntrySet->deleteEntry(pTrans, eid);

				MemoryContext::deAlloc(entry.getData());
			}
			pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry);
			pLsmIdx->endEntrySetScan(pScan);

			if(count != delete_data_count)
			{
				cout << "delete " << count << ", expect " << delete_data_count << endl;
				throw false;
			}
		}
		command_counter_increment();
		if(!testlsmdll_QueryByDataKey(pTrans, pLsmIdx, delete_data_key, 0))
		{
			throw false;
		}
#endif
		/* insert */
		uint32 insert_data_key = s_max_data + 300;
		uint32 insert_data_count = 100;
		for(uint32 i = 0; i < insert_data_count; i++)
		{
			std::string data_insert;
			testlsmdll_BuildData(insert_data_key, "data_insert_again", data_insert);

			DataItem data;
			data.setData((void*)data_insert.c_str());
			data.setSize(data_insert.length());

			EntryID eid;
			pEntrySet->insertEntry(pTrans, eid, data);
		}
		command_counter_increment();
		if(!testlsmdll_QueryByDataKey(pTrans, pLsmIdx, insert_data_key, insert_data_count))
		{
			throw false;
		}
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	catch(bool& ret)
	{
		pTrans->abort();
		delete pTrans;
		return ret;
	}
	pTrans->commit();

	return true;
}

void testlsmdll_InsertThread(EntrySetID heapId, EntrySetID lsmIdxId, bool *pRet)
{
	StorageEngine *pSE = StorageEngine::getStorageEngine();
	pSE->beginThread();

	uint32 insert_count = 10000000;

	*pRet = true;

	TransactionId xid = 0;
	Transaction *pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_SHARED, heapId);
		IndexEntrySet *pLsmIdx = pSE->openIndexEntrySet(pTrans, pEntrySet, EntrySet::OPEN_SHARED, lsmIdxId);
		/* insert */
		for(uint32 i = 0; i < insert_count; i++)
		{
			std::string data_insert;
			testlsmdll_BuildData(i, "data_insert_again", data_insert);

			DataItem data;
			data.setData((void*)data_insert.c_str());
			data.setSize(data_insert.length());

			EntryID eid;
			pEntrySet->insertEntry(pTrans, eid, data);
		}
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		*pRet = false;
	}
	pTrans->commit();

	pSE->endThread();
}

bool testlsmdll_InternalInsertConcurrency(void)
{
	testlsmdll_SetColid();

	uint32 insert_count = 10000000;
	EntrySetID heapId;
	EntrySetID lsmIdxId;

	if(!__testlsmdll_InsertBigDataFirst(NULL, insert_count, heapId, lsmIdxId))
	{
		return false;
	}

	static const uint32 threadCount = 10;
	bool threadRet[threadCount];
	boost::thread_group thgrp;
	for(uint32 i = 0; i < threadCount; i++)
	{
		thgrp.create_thread(boost::bind(&testlsmdll_InsertThread, heapId, lsmIdxId, &threadRet[i]));
	}
	thgrp.join_all();

	bool ret = true;
	for(uint32 i = 0; i < threadCount; i++)
	{
		cout << "threadRet[" << i << "]: " << threadRet[i] << endl;
		ret = (ret && threadRet[i]);
	}

	return ret;
}

void testlsmdll_CreateAndInsertThread(bool *pRet)
{
	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	pStorageEngine->beginThread();

	uint32 insert_count = 10000000;
	EntrySetID heapId;
	EntrySetID lsmIdxId;

	*pRet = __testlsmdll_InsertBigDataFirst(NULL, insert_count, heapId, lsmIdxId);

	pStorageEngine->endThread();
}

bool testlsmdll_Concurrency(void)
{
	testlsmdll_SetColid();

	static const uint32 threadCount = 10;
	bool threadRet[threadCount];
	boost::thread_group thgrp;
	for(uint32 i = 0; i < threadCount; i++)
	{
		thgrp.create_thread(boost::bind(&testlsmdll_CreateAndInsertThread, &threadRet[i]));
	}
	thgrp.join_all();

	bool ret = true;
	for(uint32 i = 0; i < threadCount; i++)
	{
		cout << "threadRet[" << i << "]: " << threadRet[i] << endl;
		ret = (ret && threadRet[i]);
	}

	return ret;
}

