#include "perform_bulk_insert.h"
#include "perform_utils.h"
#include "StorageEngine.h"
#include "EntryIDBitmap.h"
#include <boost/timer.hpp>
#include <fstream>

bool sort_index_my_bulk_insert(EntrySetID entrySetId, const std::vector<DataItem>& vData)
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

const std::string g_strPrepare = "../DataSrcId";
bool test_perform_sort_index_prepare()
{
	bool bRet = true;

	//create heap without index

	CPerformBase create;
	uint32 colid = 0;
	EntrySetID entrySetId = create.Create_EntrySet(false,false,colid);;

	//insert data into heap

	uint32 nTupleCount = GetDataCount();
	uint32 nTupleLen = GetTupleLen();

	uint32 nCount = nTupleCount;
	uint32 nFactor = 1;
	const uint32 MAX_LIMIT = 1000000;
	if (nTupleCount > MAX_LIMIT)
	{
		nCount = MAX_LIMIT;
		nFactor = nTupleCount/MAX_LIMIT;
	}

	//nFactor
	for (uint32 i = 0; i < nFactor; i ++)
	{
		TestData data;
		data.tuple_count = nCount;
		data.tuple_len = nTupleLen;
		GetTestDataPtr(data);

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

		bRet = sort_index_my_bulk_insert(entrySetId,vData);
	}

	//save the entrySetid into file	

	std::string strFile = g_strPrepare;
	std::fstream f(strFile.c_str(),std::ios_base::out | std::ios_base::trunc);
	if (f.is_open())
	{
		f<<entrySetId<<std::endl;

		f.close();
	}
	else
	{
		bRet = false;
		std::cerr<<"the file to save entrySetId is not to be created !"<<std::endl;
	}

	return bRet;
}

const int g_nTupleLen = 32;
EntrySetID read_data_entrysetId()
{
	EntrySetID entrySetId = InvalidEntrySetID;
	std::string strFile = g_strPrepare;
	std::fstream f(strFile.c_str(),std::ios_base::in);
	if (f.is_open())
	{
		std::string strLine;

		//first line is entrysetId
		std::getline(f,strLine);
		entrySetId = atoi(strLine.c_str());

		f.close();		
	}

	return entrySetId;
}
bool test_perform_multi_insert_index()
{
	bool bRet = true;

	//create heap
	CPerformBase create;
	uint32 colid = 0;
	EntrySetID entrySetId = create.Create_EntrySet(false,true,colid);
	if (InvalidEntrySetID == entrySetId)
		return false;

	{
		std::vector<std::string> vData;
		EntrySetID readId = read_data_entrysetId();
		const int MAX_COUNT = GetDataCount();

		const int nTuplen = g_nTupleLen;
		char* pData = new char[MAX_COUNT * nTuplen];
		memset(pData,0,MAX_COUNT);

		StorageEngine* pSE = NULL;
		Transaction* pTrans = NULL;
		try
		{
			pSE = StorageEngine::getStorageEngine();
			TransactionId xid = InvalidTransactionID;
			pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

			EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,readId);
			std::vector<ScanCondition> vCons;
			EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCons);

			char* pCur = pData;
			int flag = 0;
			DataItem item;
			EntryID eid;
			while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
			{
				if (flag >= MAX_COUNT)
					break;

				++flag;

				//std::string str((char*)item.getData());
				//vData.push_back(str);

				memcpy(pCur,item.getData(),item.getSize());

				pCur += nTuplen;

				MemoryContext::deAlloc(item.getData());
			}

			pEntrySet->endEntrySetScan(pScan);

			pTrans->commit();

			assert(flag == MAX_COUNT);
		}
		catch(StorageEngineException& ex)
		{
			std::cerr<<ex.getErrorMsg()<<std::endl;
			pTrans->abort();

			return false;
		}

		double dTotalTime = 0.0;

		char* pIter = pData;
		std::vector<DataItem> vItems;
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			DataItem item;
			item.setData(pIter);
			item.setSize(nTuplen);
			vItems.push_back(item);

			pIter += nTuplen;
		}
		//for (std::vector<std::string>::iterator it = vData.begin(); it != vData.end(); ++it)
		//{
		//	DataItem item;
		//	item.setData((void*)it->c_str());
		//	item.setSize(it->size());
		//	vItems.push_back(item);
		//}

		boost::timer t;
		bRet = sort_index_my_bulk_insert(entrySetId,vItems);
		dTotalTime = t.elapsed();

		if (pData)
			free(pData);

		//bulk insert data
		std::string strFlag = "PERFORM_HEAP_MULTI_INSERT_INDEX_SORT";
		create.WriteStatInfo2File_Ex(strFlag,dTotalTime);
	}

	bool bClear = create.ClearTask();
	return bRet && bClear;
}

std::string g_strUnSortFile = "../SortIndex";
bool test_perform_multi_insert_index_1()
{
	bool bRet = true;
	CPerformBase create;

	EntrySetID entrySetId = InvalidEntrySetID;
	uint32 colid_heap = 0;
	uint32 colid_index = 0;
	int times = 0;

	std::fstream f(g_strUnSortFile.c_str(),std::ios_base::in);
	if (f.is_open())
	{
		std::string strLine;

		//first line is entrysetId
		std::getline(f,strLine);
		entrySetId = atoi(strLine.c_str());

		//second line is heap colid
		std::getline(f,strLine);
		colid_heap = atoi(strLine.c_str());

		//third line is index colid
		std::getline(f,strLine);
		colid_index = atoi(strLine.c_str());

		//last line is execute times
		std::getline(f,strLine);
		times = atoi(strLine.c_str());

		f.close();		
	}

	if (InvalidEntrySetID == entrySetId)
	{
		entrySetId = create.Create_EntrySet(false,true,colid_index);
		colid_heap = colid_index - 1;
	}
	else
	{
		MySetColInfo_Heap(colid_heap);
		MySetColInfo_Index(colid_index);
	}

	{
		EntrySetID readId = read_data_entrysetId();
		const int MAX_COUNT = GetDataCount();
		const int TIMES = GetTupleLen();

		const int nTuplen = g_nTupleLen;
		char* pData = new char[MAX_COUNT * nTuplen];
		memset(pData,0,MAX_COUNT);

		StorageEngine* pSE = NULL;
		Transaction* pTrans = NULL;
		try
		{
			pSE = StorageEngine::getStorageEngine();
			TransactionId xid = InvalidTransactionID;
			pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

			EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,readId);
			std::vector<ScanCondition> vCons;
			EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCons);

			char* pCur = pData;
			int flag = 0;
			int start = times * MAX_COUNT;
			int end = start + MAX_COUNT;
			DataItem item;
			EntryID eid;
			while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
			{
				if (start > flag)
				{
					MemoryContext::deAlloc(item.getData());
					++flag;
					continue;
				}

				if (flag >= end)
					break;

				++flag;

				memcpy(pCur,item.getData(),item.getSize());

				pCur += nTuplen;

				MemoryContext::deAlloc(item.getData());
			}

			pEntrySet->endEntrySetScan(pScan);

			pTrans->commit();
		}
		catch(StorageEngineException& ex)
		{
			std::cerr<<ex.getErrorMsg()<<std::endl;
			pTrans->abort();

			return false;
		}

		double dTotalTime = 0.0;

		char* pIter = pData;
		std::vector<DataItem> vItems;
		for (int i = 0; i < MAX_COUNT; ++i)
		{
			DataItem item;
			item.setData(pIter);
			item.setSize(nTuplen);
			vItems.push_back(item);

			pIter += nTuplen;
		}

		boost::timer t;
		bRet = sort_index_my_bulk_insert(entrySetId,vItems);
		dTotalTime = t.elapsed();

		if (pData)
			free(pData);

		//bulk insert data
		std::string strFlag = "PERFORM_HEAP_MULTI_INSERT_INDEX_SORT";
		create.WriteStatInfo2File_Ex(strFlag,dTotalTime);
	}

	//write info about this run

	bool bClear = true;
	if (GetTupleLen() == times)
	{
		//bClear = create.ClearTask();
		//remove(g_strFile.c_str());
	}
	else
	{
		++times;
		std::fstream fileIn(g_strUnSortFile.c_str(), std::ios_base::out | std::ios_base::trunc);
		if (fileIn.is_open())
		{
			fileIn<<entrySetId<<std::endl;
			fileIn<<colid_heap<<std::endl;
			fileIn<<colid_index<<std::endl;
			fileIn<<times<<std::endl;

			//std::string strInLine = boost::str(boost::format("%d") % entrySetId);
			//strInLine += "\n";
			//fileIn.write(strInLine.c_str(),strInLine.size());

			//strInLine = boost::str(boost::format("%d") % colid_heap);
			//strInLine += "\n";
			//fileIn.write(strInLine.c_str(),strInLine.size());

			//strInLine = boost::str(boost::format("%d") % colid_index);
			//strInLine += "\n";
			//fileIn.write(strInLine.c_str(),strInLine.size());

			//strInLine = boost::str(boost::format("%d") % times);
			//strInLine += "\n";
			//fileIn.write(strInLine.c_str(),strInLine.size());

			fileIn.close();
		}
	}

	return bRet && bClear;
}

//find the string that will be used in a query 
std::string find_index_query_string(EntrySetID entrySetId, float percent)
{
	const int MAX_COUNT = 10000000;
	std::string strTarget;
	std::string strMin = "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
	int nStop = MAX_COUNT * percent;

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
		ScanCondition con(1,ScanCondition::GreaterEqual,(se_uint64)strMin.c_str(),strMin.size(),str_compare);
		vCon.push_back(con);

		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		EntryID eid;
		DataItem item;
		uint32 nFlag = 0;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			++nFlag;

			if (nFlag == nStop)
			{
				strTarget = std::string((char*)item.getData(),item.getSize());
				break;
			}
		}

		assert(0 < nFlag);

		pIndex->endEntrySetScan(pScan);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}

	return strTarget;

}

std::string find_index_query_string(EntrySetID entrySetId, const std::string strIndex)
{
	std::string strTarget;
	std::string strMin = strIndex;

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
		ScanCondition con(1,ScanCondition::GreaterEqual,(se_uint64)strMin.c_str(),strMin.size(),str_compare);
		vCon.push_back(con);

		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		EntryID eid;
		DataItem item;
		uint32 nFlag = 0;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			++nFlag;
		}

		assert(0 < nFlag);

		pIndex->endEntrySetScan(pScan);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}

	return strTarget;

}


bool sort_index_scan(const EntrySetID& entrySetId,const std::string& strIndex)
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
		ScanCondition con(1,ScanCondition::GreaterEqual,(se_uint64)strIndex.c_str(),strIndex.size(),str_compare);
		vCon.push_back(con);

		boost::timer t;
		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		EntryID eid;
		DataItem item;
		uint32 nFlag = 0;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{		
			MemoryContext::deAlloc(item.getData());
			nFlag ++;
		}

		assert(0 < nFlag);

		pIndex->endEntrySetScan(pScan);

		double d_time = t.elapsed();

		std::string strFlag = "INDEX_SAN_SORT";
		CPerformBase perform;
		perform.WriteStatInfo2File_Ex(strFlag,d_time);

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

//elapse time test
bool test_perform_sort_index_query()
{
	EntrySetID entrySetId = InvalidEntrySetID;
	uint32 colid_heap = 0;
	uint32 colid_index = 0;


	std::fstream f(g_strUnSortFile.c_str(),std::ios_base::in);
	if (f.is_open())
	{
		std::string strLine;

		//first line is entrysetId
		std::getline(f,strLine);
		entrySetId = atoi(strLine.c_str());

		//second line is heap colid
		std::getline(f,strLine);
		colid_heap = atoi(strLine.c_str());

		//third line is index colid
		std::getline(f,strLine);
		colid_index = atoi(strLine.c_str());

		f.close();
	}

	if (InvalidEntrySetID == entrySetId)
		return false;

	MySetColInfo_Heap(colid_heap);
	MySetColInfo_Index(colid_index);

	std::string strIndex = "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
	//find_index_query_string(entrySetId,strIndex);
	//strIndex = find_index_query_string(entrySetId,0.01);


	bool bRet = sort_index_scan(entrySetId,strIndex);

	return bRet;
}

bool index_scan_by_bitmap(const EntrySetID& entrySetId,const std::string& strIndex)
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
		ScanCondition con(1,ScanCondition::GreaterEqual,(se_uint64)strIndex.c_str(),strIndex.size(),str_compare);
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
			nFlag ++;
			MemoryContext::deAlloc(item.getData());
		}

		assert(0 < nFlag);

		pBitmap->endIterate(tbmIterator);

		pScan->deleteBitmap(pBitmap);
		pIndex->endEntrySetScan(pScan);

		double d_time = t.elapsed();
		std::string strFlag = "BitmapScanSort";
		CPerformBase perform;
		perform.WriteStatInfo2File_Ex(strFlag,d_time);

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

bool test_perform_bitmap_scan()
{
	EntrySetID entrySetId = InvalidEntrySetID;
	uint32 colid_heap = 0;
	uint32 colid_index = 0;


	std::fstream f(g_strUnSortFile.c_str(),std::ios_base::in);
	if (f.is_open())
	{
		std::string strLine;

		//first line is entrysetId
		std::getline(f,strLine);
		entrySetId = atoi(strLine.c_str());

		//second line is heap colid
		std::getline(f,strLine);
		colid_heap = atoi(strLine.c_str());

		//third line is index colid
		std::getline(f,strLine);
		colid_index = atoi(strLine.c_str());

		f.close();
	}

	if (InvalidEntrySetID == entrySetId)
		return false;

	MySetColInfo_Heap(colid_heap);
	MySetColInfo_Index(colid_index);

	std::string strIndex = "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
	//find_index_query_string(entrySetId,strIndex);
	//strIndex = find_index_query_string(entrySetId,0.01);


	bool bRet = index_scan_by_bitmap(entrySetId,strIndex);

	return bRet;
}