#include <iostream>
#include <fstream>
#include "perform_bitmap_scan.h"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "Transaction.h"
#include "perform_utils.h"
#include "EntryIDBitmap.h"
#include "boost/timer.hpp"
#include "boost/format.hpp"


using namespace FounderXDB::StorageEngineNS;
void write_stat_info(const std::string& strFlag,const double& dTime)
{
	boost::format fmt("%s : %f");
	fmt % strFlag.c_str();
	fmt % dTime;
	std::string strFmt = fmt.str();
	std::string strFile = "../bitmap_stat";
	std::ofstream out(strFile.c_str(),std::ios_base::out | std::ios_base::app);
	if (out.is_open())
	{
		out<<strFmt.c_str()<<std::endl;

		out.close();
	}
}

EntrySetID heap_create_bitmap()
{
	EntrySetID entrySetId = InvalidEntrySetID;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		const uint32 colid_heap = 11111;
		MySetColInfo_Heap(colid_heap);
		entrySetId = pSE->createEntrySet(pTrans,colid_heap);

		//EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);
		//const uint32 colid_index = 11112;
		//MySetColInfo_Index(colid_index);
		//pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,colid_index);

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	return entrySetId;
}
bool index_create_bitmap(const EntrySetID& entrySetId)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);
		const uint32 colid_index = 11112;
		MySetColInfo_Index(colid_index);
		pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,colid_index);

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
bool heap_insert_data_multi(const EntrySetID& entrySetId, const TestData& data)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		char* pData = data.pData;
		EntryID eid;
		DataItem item;
		std::vector<DataItem> vItems;
		for (uint32 tup_count = 0; tup_count < data.tuple_count; tup_count ++)
		{
			item.setData(pData);
			item.setSize(data.tuple_len);
			//pEntrySet->insertEntry(pTrans,eid,item);
			vItems.push_back(item);

			pData += data.tuple_len;
		}
		pEntrySet->insertEntries(pTrans,vItems);

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
enum Write_Type
{
	BITMAP_NONE,
	BITMAP_PRE,
	BITMAP_NOW
};
void print_entryId(EntryID eid,DataItem& item,Write_Type type,bool bForce)
{
	int len = 30;
	int nMemLen = 1024*1024;
	int nRecords = nMemLen/len;
	static int nUsed = 0;
	static char* pData = NULL;
	if (NULL == pData)
	{
		pData = (char*)malloc(nMemLen);
		memset(pData,0,nMemLen);
	}

	static char* pIter = pData;
	memcpy(pIter,item.getData(),item.getSize());
	pIter += item.getSize();
	nUsed += item.getSize();

	sprintf(pIter," %d %d %d\n",eid.bi_hi,eid.bi_lo,eid.ip_posid);
	int fmtLen = strlen(pIter);
	pIter += fmtLen;

	nUsed += fmtLen;

	if (nMemLen - nUsed > 30 && !bForce)
		return ;


	std::string strFileName;

	switch(type)
	{
	case BITMAP_NONE:
		strFileName = "../bitmap_none";
		break;
	case BITMAP_PRE:
		strFileName = "../bitmap_pre";
		break;
	case BITMAP_NOW:
		strFileName = "../bitmap_now";
		break;
	default :
		break;
	}

	std::ofstream out(strFileName.c_str(),std::ios_base::app);
	if (out.is_open())
	{
		//out<<pData<<std::endl;
		out.write(pData,nUsed);
	}
	out.close();
	memset(pData,0,nMemLen);
	pIter = pData;
	nUsed = 0;
}

bool index_scan_normal(const EntrySetID& entrySetId,const std::string& strIndex)
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

		EntrySetID index_id = InvalidEntrySetID;

		{
			PGIndinfoData index_info;
			pEntrySet->getIndexInfo(pTrans,index_info);
			if (index_info.index_num > 0)
				index_id = index_info.index_array[0];
		}
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,index_id);

		std::vector<ScanCondition> vCon;
		ScanCondition con(1,ScanCondition::LessThan,(se_uint64)strIndex.c_str(),strIndex.size(),str_compare);
		vCon.push_back(con);

		boost::timer t;
		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		EntryID eid;
		DataItem item;
		uint32 nFlag = 0;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			//do something

			char* pGet = (char*)item.getData();
			if (-1 != str_compare(pGet,10,strIndex.c_str(),10))
			{
				assert(false);
			}
			nFlag ++;

			//if (nFlag == 4500000)
			//	print_entryId(eid,item,BITMAP_NONE,true);
			//else
			//	print_entryId(eid,item,BITMAP_NONE,false);
		}

		assert(0 < nFlag);

		pIndex->endEntrySetScan(pScan);

		double d_time = t.elapsed();
		write_stat_info("INDEX_SCAN_NORMAL",d_time);
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
bool index_scan_bitmap(const EntrySetID& entrySetId,const std::string& strIndex)
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

		EntrySetID index_id = InvalidEntrySetID;
		{
			PGIndinfoData index_info;
			pEntrySet->getIndexInfo(pTrans,index_info);
			if (index_info.index_num > 0)
				index_id = index_info.index_array[0];
		}
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,index_id);

		std::vector<ScanCondition> vCon;
		ScanCondition con(1,ScanCondition::LessThan,(se_uint64)strIndex.c_str(),strIndex.size(),str_compare);
		vCon.push_back(con);

		boost::timer t;
		IndexEntrySetScan* pScan = (IndexEntrySetScan*)pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		EntryIDBitmap* pBitmap = pScan->getBitmap(pTrans);

		EntryIDBitmapIterator* tbmIterator = pBitmap->beginIterate();

		EntryID eid;
		DataItem item;
		uint32 nFlag = 0;
		while (NO_DATA_FOUND != tbmIterator->getNext(eid,item))
		{

			char* pTmp = (char*)item.getData();
			if (-1 != str_compare(pTmp,10,strIndex.c_str(),10))
			{
				assert(false);
				//break;
				//int i = 0;
			}
			nFlag ++;
			//if (nFlag == 4500000)
			//	print_entryId(eid,item,BITMAP_NOW,true);
			//else
			//	print_entryId(eid,item,BITMAP_NOW,false);
		}

		assert(0 < nFlag);

		pBitmap->endIterate(tbmIterator);

		pScan->deleteBitmap(pBitmap);
		pIndex->endEntrySetScan(pScan);

		double d_time = t.elapsed();
		write_stat_info("INDEX_SCAN_BITMAP",d_time);

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
bool heap_scan_normal(const EntrySetID& entrySetId,const std::string& strIndex)
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

		//EntrySetID index_id = InvalidEntrySetID;

		//{
		//	PGIndinfoData index_info;
		//	pEntrySet->getIndexInfo(pTrans,index_info);
		//	if (index_info.index_num > 0)
		//		index_id = index_info.index_array[0];
		//}
		//IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,index_id);

		std::vector<ScanCondition> vCon;
		ScanCondition con(1,ScanCondition::LessThan,(se_uint64)strIndex.c_str(),strIndex.size(),str_compare);
		vCon.push_back(con);

		boost::timer t;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		EntryID eid;
		DataItem item;
		uint32 nFlag = 0;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			//do something

			char* pGet = (char*)item.getData();
			if (-1 != str_compare(pGet,10,strIndex.c_str(),10))
			{
				assert(false);
			}
			nFlag ++;

			//if (nFlag == 4500000)
			//	print_entryId(eid,item,BITMAP_NONE,true);
			//else
			//	print_entryId(eid,item,BITMAP_NONE,false);
		}

		assert(0 < nFlag);

		pEntrySet->endEntrySetScan(pScan);

		double d_time = t.elapsed();
		write_stat_info("HEAP_SCAN_NORMAL",d_time);
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

bool clear_heap_buffer(const EntrySetID& entrySetId)
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
		uint32 nFlag = 0;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			//do something
			nFlag ++;
			MemoryContext::deAlloc(item.getData());
		}

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

std::string g_strFile = "BitmapScanIds.conf";
bool test_bitmap_scan_prepare()
{
	bool bRet = true;

	//create heap and index
	EntrySetID entrySetId_normal = InvalidEntrySetID;
	EntrySetID entrySetId_bitmap = InvalidEntrySetID;
	EntrySetID entrySetId_middle = InvalidEntrySetID;

	entrySetId_bitmap = heap_create_bitmap();
	entrySetId_normal = heap_create_bitmap();
	entrySetId_middle = heap_create_bitmap();

	if (InvalidEntrySetID == entrySetId_normal || InvalidEntrySetID == entrySetId_bitmap || InvalidEntrySetID == entrySetId_middle)
		return false;

	//insert data,1,000,000 tuples
	TestData data;
	data.tuple_count = 1000;//count = 100
	data.tuple_len = 10;
	data.pData = (char*)malloc(data.tuple_count*data.tuple_len);
	memset(data.pData,0,data.tuple_count*data.tuple_len);

	char* pIter = data.pData;
	uint32 data_base = 1000000000;
	char chFmt[11];
	for (uint32 i = data_base; i < data_base + data.tuple_count; i ++)
	{
		sprintf(chFmt,"%d",i);
		memcpy(pIter,chFmt,data.tuple_len);
		pIter += data.tuple_len;
	}

	uint32 nTotalCount = 10000000;
	uint32 insert_times = nTotalCount/data.tuple_count;

	if (bRet)
	{
		for (uint32 times = 0; times < insert_times; times ++)
		{
			bRet = heap_insert_data_multi(entrySetId_bitmap,data);
			bRet = heap_insert_data_multi(entrySetId_normal,data);
			bRet = heap_insert_data_multi(entrySetId_middle,data);
		}
	}

	bRet = index_create_bitmap(entrySetId_bitmap);
	bRet = index_create_bitmap(entrySetId_normal);
	bRet = index_create_bitmap(entrySetId_middle);



	std::string strFileName = "BitmapScanId.conf";
	std::ofstream out(g_strFile.c_str(),std::ios_base::out);
	if (out.is_open())
	{
		out<<entrySetId_middle<<std::endl;
		out<<entrySetId_normal<<std::endl;
		out<<entrySetId_bitmap<<std::endl;
	}
	out.close();

	return bRet;
}
bool test_bitmap_index_scan_ex(const std::string& strQueryData,bool bDropHeap)
{
	bool bRet = true;

	const uint32 colid_heap = 11111;
	MySetColInfo_Heap(colid_heap);

	const uint32 colid_index = 11112;
	MySetColInfo_Index(colid_index);

	//create heap and index
	EntrySetID entrySetId_normal = InvalidEntrySetID;
	EntrySetID entrySetId_bitmap = InvalidEntrySetID;
	EntrySetID entrySetId_middle = InvalidEntrySetID;

	std::string strTmp;
	std::ifstream infile(g_strFile.c_str(),std::ios_base::in);
	if (infile.is_open())
	{
		std::getline(infile,strTmp);
		entrySetId_middle = atoi(strTmp.c_str());

		std::getline(infile,strTmp);
		entrySetId_normal = atoi(strTmp.c_str());

		std::getline(infile,strTmp);
		entrySetId_bitmap = atoi(strTmp.c_str());
	}
	infile.close();


	//heap scan
	if (bRet)
	{
		bRet = heap_scan_normal(entrySetId_bitmap,strQueryData);
	}
	//bitmap index scan
	if (bRet)
	{
		//clear_heap_buffer(entrySetId_middle);
		bRet = index_scan_bitmap(entrySetId_bitmap,strQueryData);
		//bRet = index_scan_bitmap(entrySetId_bitmap,strQueryData);
	}

	//index scan 
	if (bRet)
	{
		//clear_heap_buffer(entrySetId_middle);
		bRet = index_scan_normal(entrySetId_normal,strQueryData);
	}

	//clear task
	if (bDropHeap)
	{
		bRet = perform_drop_heap(entrySetId_normal);
		bRet = perform_drop_heap(entrySetId_bitmap);
		bRet = perform_drop_heap(entrySetId_middle);
	}

	return bRet;
}

bool test_bitmap_index_scan()
{
	bool bRet = true;

	bRet = test_bitmap_index_scan_ex("1000000001",false);

	bRet = test_bitmap_index_scan_ex("1000000010",false);

	bRet = test_bitmap_index_scan_ex("1000000050",false);

	bRet = test_bitmap_index_scan_ex("1000000100",false);

	bRet = test_bitmap_index_scan_ex("1000000150",false);

	bRet = test_bitmap_index_scan_ex("1000000200",false);

	bRet = test_bitmap_index_scan_ex("1000000250",false);

	bRet = test_bitmap_index_scan_ex("1000000300",false);

	bRet = test_bitmap_index_scan_ex("1000000400",false);

	bRet = test_bitmap_index_scan_ex("1000000450",false);

	return bRet;
}

//bool test_bitmap_index_scan_multi_conditions()
//{
//	index_scan_bitmap()
//
//
//	return true;
//}
