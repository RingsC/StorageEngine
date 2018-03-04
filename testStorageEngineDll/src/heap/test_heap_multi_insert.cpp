#include <iostream>
#include <time.h>
#include <vector>
#include "utils/utils_dll.h"
#include "heap/test_heap_multi_insert.h"
#include <fstream>
#include <list>
#include "boost/thread.hpp"

std::string g_strFilePath = "./multi_insert.conf";
std::string g_strDataFile = "./insert_data";
extern std::map<std::string,std::string> ProgramOpts;

bool need_run()
{
	if (ProgramOpts.find("-multi") != ProgramOpts.end())
		return true;
	return false;
}
std::string GenerateRandomString(uint32 len)
{
	std::string strRet;

	char chDict[] = {"123456789abcdefghijklmnopqrstuvwxyz!@#$%^&*()_+|.~=-/"};
	uint32 arr_len = sizeof(chDict);

	for (uint32 i = 0; i < len; i ++)
	{
		uint32 index = rand() / (RAND_MAX/(arr_len-1));
		if (index < arr_len)
		{
			char chTmp[2] = {0};
			memcpy(chTmp,chDict + index,1);
			std::string strTmp(chTmp);
			strRet += strTmp;
		}
	}

	return strRet;
}
void GetTestDataPtr(TestData& data)
{
	if (data.tuple_count == 0 || data.tuple_len == 0)
		return;

	char* pData = data.pData;
	if (!pData)
	{
		pData = (char*)malloc(data.tuple_count * data.tuple_len + 1);
		memset(pData,0,data.tuple_count * data.tuple_len + 1);
		data.pData = pData;
	}

	if (pData)
	{
		memset(pData,0,data.tuple_count * data.tuple_len);
		char* pTmp = pData;
		srand((int)time(0));
		for (uint32 i = 0; i < data.tuple_count; i ++)
		{
			std::string strRandom = GenerateRandomString(data.tuple_len);
			memcpy(pTmp,strRandom.c_str(),data.tuple_len);
			pTmp += data.tuple_len;
		}
	}

	assert(pData);
}
void heap_multi_insert_split(RangeDatai& rdi,const char* pSrc,int colNum,size_t len)
{
	rdi.start = 0;
	rdi.len = 0;

	if (1 == colNum)
	{
		rdi.len = 50;
	}
	if (2 == colNum)
	{
		rdi.start = 50;
		rdi.len = 100;
	}
	if (3 == colNum)
	{
		rdi.start = 150;
		rdi.len = len - 150;
	}
}
void heap_multi_insert_split_index(RangeDatai& rdi,const char* pSrc,int colNum,size_t len)
{
	rdi.start = 0;
	rdi.len = 0;

	if (1 == colNum)
	{
		rdi.len = 50;
	}
	if (2 == colNum)
	{
		rdi.start = 50;
		rdi.len = 106;
	}
}
void MySetColInfo_Index(const uint32& colId)
{
	ColumnInfo *pCol = new ColumnInfo;
	pCol->col_number = new size_t[2];
	pCol->col_number[0] = 1;
	pCol->col_number[1] = 3;
	pCol->keys = 2;
	pCol->rd_comfunction = new CompareCallbacki[2];
	pCol->rd_comfunction[0] = str_compare;
	pCol->rd_comfunction[1] = str_compare;
	pCol->split_function = heap_multi_insert_split_index;
	setColumnInfo(colId,pCol);
}
void MySetColInfo_Heap(const uint32& colId)
{
	ColumnInfo *pCol = new ColumnInfo;
	pCol->col_number = NULL;
	pCol->keys = 0;
	pCol->rd_comfunction = NULL;
	pCol->split_function = heap_multi_insert_split;
	setColumnInfo(colId,pCol);
}

bool heap_multi_insert_dataitem(EntrySet* pEntrySet, Transaction* pTrans, const TestData& data)
{
	std::vector<DataItem> vData;
	DataItem item;
	char* pTmpData = data.pData;
	for (uint32 i = 0; i < data.tuple_count; i ++)
	{
		item.setData((void*)pTmpData);
		item.setSize(data.tuple_len);
		vData.push_back(item);
		pTmpData += data.tuple_len;
	}

	pEntrySet->insertEntries(pTrans,vData);

	return true;
}
bool heap_multi_insert_entry(EntrySet* pEntrySet, Transaction* pTrans, const TestData& data)
{
	//create heap
	EntrySetID entrySetId = StorageEngine::getStorageEngine()->createEntrySet(pTrans,GetSingleColInfo());
	if (InvalidEntrySetID == entrySetId)
		return false;

	//insert data normal
	EntrySet* pEntryData = StorageEngine::getStorageEngine()->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);
	DataItem item;
	EntryID eid;
	const char* pTmpData = data.pData;
	for (uint32 i = 0; i < data.tuple_count; i ++)
	{
		item.setData((void*)pTmpData);
		item.setSize(data.tuple_len);
		pEntryData->insertEntry(pTrans, eid, item);
		pTmpData += data.tuple_len;
	}

	command_counter_increment();
	//insert data from heap
	pEntrySet->insertEntries(pTrans, *pEntryData);

	StorageEngine::getStorageEngine()->removeEntrySet(pTrans,entrySetId);

	return true;
}

bool heap_multi_insert_identify(const EntrySetID& entrySetId, const TestData& data)
{
	bool bRet = true;

	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		std::vector<ScanCondition> vCon;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);
		EntryID eid;
		DataItem item;
		char* pDataItem = NULL;
		char* pData = data.pData;
		uint32 nCount = 0;

		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			//if (nCount >= data.tuple_count)
			//	break;

			//pDataItem = (char*)item.getData();
			//if (0 != memcmp(pData,pDataItem,data.tuple_len))
			//	break;
			
			nCount ++;
            //pData += data.tuple_len;
			MemoryContext::deAlloc(item.getData());
		}
		if (nCount != data.tuple_count)
			bRet = false;

		pEntrySet->endEntrySetScan(pScan);

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();

		return false;
	}

	return bRet;
}
bool heap_multi_insert_identify_index(const EntrySetID& entrySetId, const TestData& data)
{
	bool bRet = true;

	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	    EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		//search by index for the first tuple
		char* pTmp = (char*)malloc(data.tuple_len + 1);
		memset(pTmp,0,data.tuple_len);
		memcpy(pTmp,data.pData,data.tuple_len);

		std::vector<ScanCondition> vCon;
		ScanCondition con1(1,ScanCondition::Equal,(se_uint64)pTmp,50,str_compare);
		char* pCon2 = pTmp + 150;
		ScanCondition con2(2,ScanCondition::Equal,(se_uint64)pCon2,106,str_compare);
		vCon.push_back(con1);
		vCon.push_back(con2);

		EntrySetID indexId = InvalidEntrySetID;
		{
			PGIndinfoData indinfo;
			indinfo.index_num = 0;
			pEntrySet->getIndexInfo(pTrans,indinfo);
			assert(indinfo.index_num != 0);
			indexId = indinfo.index_array[0];
		}
		
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,indexId);
		
		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);
		EntryID eid;
		DataItem item;
		char* pDataItem = NULL;
		uint32 nCount = 0;

		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			pDataItem = (char*)item.getData();
			if (0 != memcmp(pTmp,pDataItem,data.tuple_len))
				break;

			nCount ++;
			MemoryContext::deAlloc(item.getData());
		}
		if (nCount == 0)
			bRet = false;

		pIndex->endEntrySetScan(pScan);

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();

		return false;
	}

	return bRet;
}
bool heap_multi_insert_drop(const EntrySetID& entrySetId)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pSE->removeEntrySet(pTrans,entrySetId);
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
bool heap_multi_insert_adapter(bool bFrmEntrySet, bool bIndex, bool bShutDown)
{
	//create a heap, insert some tuples without index,then identify data

	bool bRet = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	EntrySetID entrySetId = InvalidEntrySetID;
	TestData data;

	try
	{
		//prepare for insert data, 10*8k
		data.tuple_len = 256;
		data.tuple_count = (10*8*1024)/256;
		GetTestDataPtr(data);

		//create entrySet
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		uint32 heap_colid = 11111;
		MySetColInfo_Heap(heap_colid);
		entrySetId = pSE->createEntrySet(pTrans,heap_colid);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);
		
		if (bIndex)
		{
			uint32 index_colid = 11112;
			MySetColInfo_Index(index_colid);
			pSE->createIndexEntrySet(pTrans, pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,index_colid);
		}
		
		if (bFrmEntrySet)
		{
			bRet = heap_multi_insert_entry(pEntrySet,pTrans,data);
		}
		else
		{
			bRet = heap_multi_insert_dataitem(pEntrySet,pTrans,data);
		}

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();

		return false;
	}

	//save entryId
	std::ofstream fout(g_strFilePath.c_str(),std::ios_base::out);
	if (fout.is_open())
	{
		std::cout<<"open entrySetId file success !\n"<<std::endl;
		fout<<entrySetId<<std::endl;
	}
	fout.close();

	//save data
	fout.open(g_strDataFile.c_str(),std::ios_base::out | std::ios_base::binary);
	if (fout.is_open())
	{
		std::cout<<"open insert data success!\n"<<std::endl;
		fout.write(data.pData,data.tuple_len*data.tuple_count);
	}
	fout.close();

	//shutdown
	if (bShutDown)
	{
		StorageEngine::getStorageEngine()->resetOnExit();
		exit(0);
	}

	//identify insert data with index or no
	bool bIdentify = heap_multi_insert_identify(entrySetId,data);
	if (bIndex)
		bIdentify = heap_multi_insert_identify_index(entrySetId,data);

	//clear task ,include drop heap,
	bool bClear = heap_multi_insert_drop(entrySetId);

	return bRet&&bIdentify&&bClear;	
}

bool test_heap_multi_insert_frm_items()
{
	return heap_multi_insert_adapter(false,false,false);
}

bool test_heap_multi_insert_frm_entrySet()
{
	return heap_multi_insert_adapter(true,false,false);
}
bool test_heap_multi_insert_frm_items_index()
{
	return heap_multi_insert_adapter(false,true,false);
}

bool test_heap_multi_insert_frm_entrySet_index()
{
	return heap_multi_insert_adapter(true,true,false);
}

//////////////////////////////////////////////////////////////////////////
bool test_heap_multi_insert_frm_entrySet_ShutDown()
{
	if (need_run())
		return heap_multi_insert_adapter(true,false,true);
	else
		return true;
}
bool test_heap_multi_insert_frm_items_ShutDown()
{
	if (need_run())
		return heap_multi_insert_adapter(false,false,true);
	else
		return true;
}
bool test_heap_multi_insert_frm_items_ShutDown_index()
{
	if (need_run())
		return heap_multi_insert_adapter(false,true,true);
	else
		return true;
}
bool test_heap_multi_insert_frm_entrySet_ShutDown_index()
{
	if (need_run())
		return heap_multi_insert_adapter(true,true,true);
	else
		return true;
}

//////////////////////////////////////////////////////////////////////////
bool heap_multi_insert_reboot_identify(bool bIndex)
{
	std::string strEntrySetId;
	EntrySetID entrySetId = InvalidEntrySetID;
	TestData data;
	data.tuple_len = 256;
	data.tuple_count = (10*8*1024)/256;
	data.pData = (char*)malloc(data.tuple_len*data.tuple_count + 1);
	memset(data.pData,0,data.tuple_len*data.tuple_count + 1);

	//get entrySetId frm file
	std::ifstream fin(g_strFilePath.c_str(),std::ios_base::in);
	if (fin.is_open())
	{
		std::getline(fin,strEntrySetId);
	}
	fin.close();
	entrySetId = atoi(strEntrySetId.c_str());

	if (InvalidEntrySetID == entrySetId)
	{
		return false;
	}

	//load insert data frm file
	fin.open(g_strDataFile.c_str(),std::ios_base::binary | std::ios_base::in);
	if (fin.is_open())
	{
		fin.read(data.pData,data.tuple_len*data.tuple_count);
	}
	fin.close();

	//identify
	if (need_run())
	{
		uint32 heap_colid = 11111;
		MySetColInfo_Heap(heap_colid);

		uint32 index_colid = 11112;
		MySetColInfo_Index(index_colid);
	}

	bool bIdentify = heap_multi_insert_identify(entrySetId,data);
	if (bIndex)
		bIdentify = heap_multi_insert_identify_index(entrySetId,data);

	//clear task
	bool bClear = heap_multi_insert_drop(entrySetId);

	return bIdentify&&bClear;
}

bool test_heap_multi_insert_identify()
{
	if (need_run())
		return heap_multi_insert_reboot_identify(false);
	else
		return true;
}
bool test_heap_multi_insert_identify_index()
{
	if (need_run())
		return heap_multi_insert_reboot_identify(true);
	else
		return true;
}

//一个事务内向多个表插入数据
bool test_multi_heap_multi_insert(bool bIndex, bool bFrmEntrySet)
{
	bool bRet = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	EntrySetID entrySetId1 = InvalidEntrySetID;
	EntrySetID entrySetId2 = InvalidEntrySetID;
	EntrySetID entrySetId3 = InvalidEntrySetID;
	TestData data1;
	TestData data2;
	TestData data3;

	try
	{
		//create entrySet
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		uint32 heap_colid1 = 11111;
		MySetColInfo_Heap(heap_colid1);
		entrySetId1 = pSE->createEntrySet(pTrans,heap_colid1);
		EntrySet* pEntrySet1 = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId1);

		uint32 heap_colid2 = 11112;
		MySetColInfo_Heap(heap_colid2);
		entrySetId2 = pSE->createEntrySet(pTrans,heap_colid2);
		EntrySet* pEntrySet2 = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId2);

		uint32 heap_colid3 = 11113;
		MySetColInfo_Heap(heap_colid3);
		entrySetId3 = pSE->createEntrySet(pTrans,heap_colid3);
		EntrySet* pEntrySet3 = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId3);

		if (bIndex)
		{
			uint32 index_colid1 = 11114;
			MySetColInfo_Index(index_colid1);
			pSE->createIndexEntrySet(pTrans, pEntrySet1,BTREE_INDEX_ENTRY_SET_TYPE,index_colid1);

			uint32 index_colid2 = 11115;
			MySetColInfo_Index(index_colid2);
			pSE->createIndexEntrySet(pTrans, pEntrySet2,BTREE_INDEX_ENTRY_SET_TYPE,index_colid2);

			uint32 index_colid3 = 11116;
			MySetColInfo_Index(index_colid3);
			pSE->createIndexEntrySet(pTrans, pEntrySet3,BTREE_INDEX_ENTRY_SET_TYPE,index_colid3);
		}

		//prepare for insert data, 10*8k
		data1.tuple_len = 256;
		data1.tuple_count = (10*8*1024)/256;
		GetTestDataPtr(data1);

		//prepare for insert data, 10*8k
		data2.tuple_len = 256;
		data2.tuple_count = (10*8*1024)/256;
		GetTestDataPtr(data2);

		//prepare for insert data, 10*8k
		data3.tuple_len = 256;
		data3.tuple_count = (10*8*1024)/256;
		GetTestDataPtr(data3);

		if (bFrmEntrySet)
		{
			bRet = heap_multi_insert_entry(pEntrySet1,pTrans,data1);
			bRet = heap_multi_insert_entry(pEntrySet2,pTrans,data2);
			bRet = heap_multi_insert_entry(pEntrySet3,pTrans,data3);
		}
		else
		{
			bRet = heap_multi_insert_dataitem(pEntrySet1,pTrans,data1);
			bRet = heap_multi_insert_dataitem(pEntrySet2,pTrans,data2);
			bRet = heap_multi_insert_dataitem(pEntrySet3,pTrans,data3);
		}

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();

		return false;
	}

	//identify insert data with index or no
	bool bIdentify = heap_multi_insert_identify(entrySetId1,data1);
	if (bIndex)
		bIdentify = heap_multi_insert_identify_index(entrySetId1,data1);

	bIdentify = heap_multi_insert_identify(entrySetId2,data2);
	if (bIndex)
		bIdentify = heap_multi_insert_identify_index(entrySetId2,data2);

	bIdentify = heap_multi_insert_identify(entrySetId3,data3);
	if (bIndex)
		bIdentify = heap_multi_insert_identify_index(entrySetId3,data3);


	//clear task ,include drop heap,
	bool bClear = heap_multi_insert_drop(entrySetId1);
	bClear = heap_multi_insert_drop(entrySetId2);
	bClear = heap_multi_insert_drop(entrySetId3);

	return bRet&&bIdentify&&bClear;	
}
bool test_heap_multi_insert_multi()
{
	return test_multi_heap_multi_insert(false,false);
}
bool test_heap_multi_insert_multi_index()
{
	return test_multi_heap_multi_insert(true,false);
}
bool test_heap_multi_insert_multi_frmEntrySet()
{
	return test_multi_heap_multi_insert(false,true);
}
bool test_heap_multi_insert_multi_frmEntrySet_index()
{
	return test_multi_heap_multi_insert(true,true);
}

EntrySetID heap_multi_insert_create()
{
	EntrySetID entrySetId = InvalidEntrySetID;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		entrySetId = pSE->createEntrySet(pTrans,GetSingleColInfo());

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	return entrySetId;
}

bool heap_multi_insert_data_toast(const EntrySetID& entrySetId, const TestData& data)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		DataItem item;
		std::vector<DataItem> vData;

		char* pData = data.pData;
		for (uint32 i = 0; i < data.tuple_count; i ++)
		{
			item.setData(pData);
			item.setSize(data.tuple_len);
			vData.push_back(item);
			pData += data.tuple_len;
		}
		pEntrySet->insertEntries(pTrans,vData);

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

bool heap_multi_insert_toast_identify(const EntrySetID& entrySetId,const TestData& data)
{
	bool bRet = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);
		std::vector<ScanCondition> vCon;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		uint32 index = 0;

		std::list<EntryID> list_entry;
		DataItem item;
		EntryID eid;
		char* pData = data.pData;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG, eid, item))
		{
			list_entry.push_back(eid);
			index ++;
			char* pItem = (char*)item.getData();
			if (0 != memcmp(pItem,pData,data.tuple_len))
			{
				bRet = false;
				break;
			}
			pData += data.tuple_len;
		}
		pEntrySet->endEntrySetScan(pScan);

		if (data.tuple_count != index)
			bRet = false;

		pEntrySet->deleteEntries(pTrans,list_entry);

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}

	return bRet;
}

void GenerateToastData(TestData& data)
{
	std::vector<std::string> g_vData;
	g_vData.push_back("1");
	g_vData.push_back("2");
	g_vData.push_back("3");
	g_vData.push_back("4");
	g_vData.push_back("5");
	g_vData.push_back("6");
	g_vData.push_back("7");
	g_vData.push_back("8");
	g_vData.push_back("9");
	g_vData.push_back("a");

	if (data.tuple_count == 0 || data.tuple_len == 0)
		return;

	char* pData = data.pData;
	pData = (char*)malloc(data.tuple_count * data.tuple_len + 1);
	data.pData = pData;

	memset(pData,0,data.tuple_count * data.tuple_len + 1);

	for (uint32 count = 0; count < data.tuple_count; count ++)
	{
		std::string strTuple = g_vData.at(count);
		for (uint32 i = 0; i < data.tuple_len; i ++)
		{
			memcpy(pData,strTuple.c_str(),1);
			pData += 1;
		}
	}
}
bool test_heap_multi_insert_toast()
{
	EntrySetID entrySetId = heap_multi_insert_create();
	if (InvalidEntrySetID == entrySetId)
		return false;

	bool bRet = true;
	//tup_len = 1024
	TestData data;
	data.tuple_len = 1024;
	data.tuple_count = 10;

	GenerateToastData(data);
	bRet = heap_multi_insert_data_toast(entrySetId,data);
	bRet = heap_multi_insert_toast_identify(entrySetId,data);

	if (!bRet)
	{
		std::cout<<"heap multi insert data failed , tup_count = "<<data.tuple_count<<" ,tup_len = "<<data.tuple_len<<" !\n"<<std::endl;
		heap_multi_insert_drop(entrySetId);
		return false;
	}

	//tup_len = 2048
	data.tuple_len = 2048;
	data.tuple_count = 10;

	GenerateToastData(data);
	bRet = heap_multi_insert_data_toast(entrySetId,data);
	bRet = heap_multi_insert_toast_identify(entrySetId,data);
	data.Clear_Mem();

	if (!bRet)
	{
		std::cout<<"heap multi insert data failed , tup_count = "<<data.tuple_count<<" ,tup_len = "<<data.tuple_len<<" !\n"<<std::endl;
		heap_multi_insert_drop(entrySetId);
		return false;
	}


	//tup_len = 2050
	data.tuple_len = 2050;
	data.tuple_count = 10;

	GenerateToastData(data);
	bRet = heap_multi_insert_data_toast(entrySetId,data);
	bRet = heap_multi_insert_toast_identify(entrySetId,data);
	data.Clear_Mem();

	if (!bRet)
	{
		std::cout<<"heap multi insert data failed , tup_count = "<<data.tuple_count<<" ,tup_len = "<<data.tuple_len<<" !\n"<<std::endl;
		heap_multi_insert_drop(entrySetId);
		return false;
	}

	//tup_len = 8192
	data.tuple_len = 8192;
	data.tuple_count = 10;

	GenerateToastData(data);
	bRet = heap_multi_insert_data_toast(entrySetId,data);
	bRet = heap_multi_insert_toast_identify(entrySetId,data);
	data.Clear_Mem();

	if (!bRet)
	{
		std::cout<<"heap multi insert data failed , tup_count = "<<data.tuple_count<<" ,tup_len = "<<data.tuple_len<<" !\n"<<std::endl;
		heap_multi_insert_drop(entrySetId);
		return false;
	}

	//tup_len = 9000
	data.tuple_len = 9000;
	data.tuple_count = 10;

	GenerateToastData(data);
	bRet = heap_multi_insert_data_toast(entrySetId,data);
	bRet = heap_multi_insert_toast_identify(entrySetId,data);
	data.Clear_Mem();

	if (!bRet)
	{
		std::cout<<"heap multi insert data failed , tup_count = "<<data.tuple_count<<" ,tup_len = "<<data.tuple_len<<" !\n"<<std::endl;
		heap_multi_insert_drop(entrySetId);
		return false;
	}

	//clear task
	bRet = heap_multi_insert_drop(entrySetId);

	return bRet;
}

void thread_multi_insert_transaction(const EntrySetID& entrySetId, const char* pData, const int& nSize,const int& nLen)
{
	bool bRet = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;

	try
	{
		//begin transaction
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		//open and insert data
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);
		std::vector<DataItem> vDataItem;
		char* pCur = (char*)pData;
		for (int i = 0; i < nSize; ++i)
		{
			DataItem item;
			item.setData(pCur);
			item.setSize(nLen);
			vDataItem.push_back(item);

			pCur += nLen;
		}
		pEntrySet->insertEntries(pTrans,vDataItem);

		pTrans->commit();

	}
	catch(StorageEngineException& ex)
	{
		bRet = false;
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	pSE->endThread();
}

//multi transaction insert tuples into the same entryset concurrently 
bool test_heap_multi_insert_thread_transaction()
{
	bool bRet = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;

	try
	{
		//begin transaction
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);		

		//create entryset
		uint32 colid_heap = 11115;
		MySetColInfo_Heap(colid_heap);
		uint32 colid_index = 11116;
		MySetColInfo_Index(colid_index);
		EntrySetID entrySetId = pSE->createEntrySet(pTrans,colid_heap);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);
		EntrySetID indexId = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,colid_index);

		pTrans->commit();

		//prepare data
		TestData data;
		data.tuple_count = 10000;
		data.tuple_len = 256;
		GetTestDataPtr(data);

		//multi thread insert data
		int step = 2000;
		char* pData = data.pData;

		boost::thread_group g;
		g.create_thread(boost::bind(thread_multi_insert_transaction,entrySetId,pData,step,data.tuple_len));
		g.create_thread(boost::bind(thread_multi_insert_transaction,entrySetId,pData + step,step,data.tuple_len));
		g.create_thread(boost::bind(thread_multi_insert_transaction,entrySetId,pData + step,step,data.tuple_len));
		g.create_thread(boost::bind(thread_multi_insert_transaction,entrySetId,pData + step,step,data.tuple_len));
		g.create_thread(boost::bind(thread_multi_insert_transaction,entrySetId,pData + step,step,data.tuple_len));
		g.join_all();

		//identify 
		bool bIdentify = heap_multi_insert_identify(entrySetId,data);
		bool bIdentifyIndex = heap_multi_insert_identify_index(entrySetId,data);

		//clear task
		int i=0;
		g.create_thread(boost::bind(heap_remove_poi,entrySetId,&i));
		g.join_all();

		data.Clear_Mem();
	}
	catch(StorageEngineException& ex)
	{
		bRet = false;
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	return bRet;
}
