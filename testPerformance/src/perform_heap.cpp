#include "perform_heap.h"
#include <iostream>
#include "boost/timer.hpp"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "Transaction.h"
#include <fstream>
#include "MemoryContext.h"
#include "boost/thread.hpp"
#include "boost/format.hpp"
#include <time.h>
using namespace FounderXDB::StorageEngineNS;

extern std::map<std::string,std::string> ProgramOpts;

const uint32 g_thread_count = 10;//并发线程数

bool IsHeapEmpty(EntrySetID entrySetId)
{
	StorageEngine* pStorageEngine = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId tid = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(tid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);
		std::vector<ScanCondition> con;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,con);

		uint32 flag = 0;
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			flag ++;
			MemoryContext::deAlloc(item.getData());
		}
		pEntrySet->endEntrySetScan(pScan);
		pTrans->commit();

		if (flag)
			return false;
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}
std::vector<std::string> g_vStrIndex;//用于索引检索时，查找目标值
std::vector<std::string> g_FixData;//store fix data for concurrent operations
bool MyInsertFixData(EntrySetID entrySetId, const uint32& count)
{
	StorageEngine* pStorageEngine = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId tid = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(tid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		g_FixData.clear();
		for (uint32 i = 0; i < g_thread_count; i ++)
		{
			std::string strData = GenerateRandomString(GetTupleLen());
			g_FixData.push_back(strData);
		}

		const uint32 data_len = GetTupleLen();
		assert(0 != g_FixData.size());
		uint32 quotient = count/g_FixData.size();
		uint32 remainder = count%g_FixData.size();
		assert(0 == remainder);

		uint32 flag = 0;

		for (uint32 i = 0; i < quotient; i ++)
		{
			for (uint32 j = 0; j < g_FixData.size(); j ++)
			{
				DataItem item;
				item.setData((void*)g_FixData[j].c_str());
				item.setSize(data_len);
				EntryID eid;
				pEntrySet->insertEntry(pTrans,eid,item);

				flag ++;
			}
		}

		pTrans->commit();
		assert(flag == count);
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}

bool InsertData_Ex(EntrySetID entrySetId)
{
	StorageEngine* pStorageEngine = NULL;
	Transaction* pTrans = NULL;
	try
	{
		//get data ptr
		TestData data;
		data.tuple_len = GetTupleLen();
		data.tuple_count = GetDataCount();
		GetTestDataPtr(data);
		if (!data.pData)
		{
			std::cout<<"get data ptr failed !\n"<<std::endl;
			pTrans->abort();
			return false;
		}

		//insert data to entryset
		pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId tid = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(tid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		uint32 flag = 0;
		std::string strTmp;
		g_vStrIndex.clear();
		char* pData = data.pData;
		char* pTupleData = (char*)malloc(data.tuple_len + 1);
		memset(pTupleData,0,data.tuple_len + 1);
		for (uint32 i = 0; i < data.tuple_count; i ++)
		{
			memcpy(pTupleData,pData,data.tuple_len);
			if (i < g_thread_count)
			{
				strTmp = (char*)pTupleData;
				g_vStrIndex.push_back(strTmp);//并发索引查找时，不同线程查找不同的数据
			}

			DataItem item;
			item.setData(pTupleData);
			item.setSize(data.tuple_len);
			EntryID eid;
			pEntrySet->insertEntry(pTrans,eid,item);

			pData += data.tuple_len;
			flag ++;
		}
		pTrans->commit();
		assert(flag == GetDataCount());
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}
std::string g_IdFilePath = "../EntrySetID";
bool Write_EntrySetId_2_file(EntrySetID entrySetId, uint32 colId)
{
	std::ofstream write(g_IdFilePath.c_str());
	if (!write.is_open())
	{
		std::cout<<"save entrysetId failed !\n"<<std::endl;
		return false;
	}

	write<<entrySetId<<std::endl;
	write<<colId<<std::endl;
	write.close();

	return true;
}
std::string g_IndexDataFile = "../index_data";
bool Write_Index_Data_2_File()
{
	assert(!g_vStrIndex.empty());

	std::ofstream write(g_IndexDataFile.c_str());
	if (write.is_open())
	{
		std::vector<std::string>::iterator it;
		for (it = g_vStrIndex.begin(); it != g_vStrIndex.end(); it ++)
		{
			write<<*it<<std::endl;
		}
		write.close();
	}
	else
	{
		std::cout<<"open index_data file failed !\n"<<std::endl;
		return false;
	}

	return true;
}
bool Get_Index_Data_Frm_File()
{
	g_vStrIndex.clear();

	std::ifstream read(g_IndexDataFile.c_str());
	std::string strData;
	if (read.is_open())
	{
		std::getline(read,strData);
		g_vStrIndex.push_back(strData);
		read.close();
	}
	else
	{
		std::cout<<"read index data failed !\n"<<std::endl;
		return false;
	}

	return true;
}
bool GetEntrySetIdFromFile(EntrySetID& entrySetId, uint32& colId)
{
	bool bRet = true;
	std::ifstream read(g_IdFilePath.c_str());
	if (read.is_open())
	{
		std::string strId;
		std::getline(read,strId);
		if (strId.empty())
			bRet = false;

		entrySetId = atoi(strId.c_str());

		std::getline(read,strId);
		colId = atoi(strId.c_str());
		
		read.close();
	}
	else
	{
		std::cout<<"read id file failed !\n"<<std::endl;
		bRet = false;
	}

	return bRet;
}
bool heap_create_and_insert(bool bIsIndex)
{
	//create entryset
	CPerformInsert perform;
	uint32 colId = 0;
	EntrySetID entrySetId = perform.Create_EntrySet(false,bIsIndex,colId);
	if(InvalidEntrySetID == entrySetId)
	{
		std::cout<<"create entryset failed !\n";
		return false;
	}
	//save entryset id to file 
	if(!Write_EntrySetId_2_file(entrySetId,colId))
		return false;

	//insert data
	bool bInsert = InsertData_Ex(entrySetId);
	bool bWrite = Write_Index_Data_2_File();

	return bInsert&&bWrite;
}
//////////////////////////////////////////////////////////////////////////
bool test_perform_heap_prepare_data()
{
	return heap_create_and_insert(false);
}
bool test_perform_heap_prepare_data_index()
{
	return heap_create_and_insert(true);
}
bool test_perform_heap_query()
{
	EntrySetID entrySetId = InvalidEntrySetID;
	uint32 colId = 0;
	if (!GetEntrySetIdFromFile(entrySetId, colId))
		return false;

	MySetColInfo_Index(colId);
	CPerformSearch search;
	bool bRet = search.Begin("QUERY",entrySetId,true);
	bool bClear = search.ClearTask();

	return bRet&&bClear;
}
bool test_perform_heap_query_index()
{
	//test_perform_heap_prepare_data_index();

	EntrySetID entrySetId = InvalidEntrySetID;
	uint32 colid = 0;
	if (!GetEntrySetIdFromFile(entrySetId, colid))
		return false;

	MySetColInfo_Index(colid);

	if (!Get_Index_Data_Frm_File())
		return false;

	CPerformIndexSearch search;
	bool bRet = search.Begin("QUERY_INDEX",entrySetId,true);
	bool bClear = search.ClearTask();

	return bRet&&bClear;
}
bool test_perform_heap_operate_adapter(CPerformFactory::PerformType type, const std::string& strFlag, bool bIndex = false, bool bMultiCol = false)
{
	CPerformFactory factory;
	CPerformBase* pPerform = factory.GetInstance(CPerformFactory::PERFORM_INSERT);
	if (!pPerform)
		return false;

	uint32 colid = 0;
	EntrySetID entrySetId = pPerform->Create_EntrySet(bMultiCol, bIndex, colid);
	if (InvalidEntrySetID == entrySetId)
		return false;

	//insert data
	if (CPerformFactory::PERFORM_INDEX_SEARCH == type)
	{
		TestData data;
		bool bRet = InsertData_Ex(entrySetId);
		if (!bRet)
			return false;
	}
	else if (CPerformFactory::PERFORM_INSERT != type)
	{
		std::string strTaskInfo = "INSERT : Records--%u, Platform--Windows 7, Version--Release, Transaction--Single, Index--No\n";
		bool bRetExe = pPerform->Begin(strTaskInfo, entrySetId,false);
		if (!bRetExe)
			return false;
	}

	//heap operation
	CPerformBase *pOperate = factory.GetInstance(type);
	std::string strTaskFlag = strFlag;
	bool bRetOperate = pOperate->Begin(strTaskFlag, entrySetId,true);

	//clear task
	bool bRetClear = pPerform->ClearTask();

	delete pPerform;
	delete pOperate;

	return bRetOperate&&bRetClear;
}
bool test_perform_heap_insert()
{
	return test_perform_heap_operate_adapter(CPerformFactory::PERFORM_INSERT,"INSERT");
}
bool test_perform_heap_insert_index()
{
	return test_perform_heap_operate_adapter(CPerformFactory::PERFORM_INSERT,"INSERT_INDEX",true);
}

bool test_perform_heap_traverse()
{
	return test_perform_heap_operate_adapter(CPerformFactory::PERFORM_SEARCH,"TRAVERSE");
}
bool test_perform_heap_traverse_index()
{
	return test_perform_heap_operate_adapter(CPerformFactory::PERFORM_INDEX_SEARCH,"TRAVERSE_INDEX",true);
}
bool test_perform_heap_delete()
{
	return test_perform_heap_operate_adapter(CPerformFactory::PERFORM_DELETE,"DELETE");
}
bool test_perform_heap_delete_index()
{
	return test_perform_heap_operate_adapter(CPerformFactory::PERFORM_DELETE,"DELETE_INDEX", true);
}
bool test_perform_heap_update()
{
	return test_perform_heap_operate_adapter(CPerformFactory::PERFORM_UPDATE,"UPDATE");
}
bool test_perform_heap_update_index()
{
	return test_perform_heap_operate_adapter(CPerformFactory::PERFORM_UPDATE,"UPDATE_INDEX");
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//concurrent operations

/************************************************************************/
/* vData---用于并发删除、更新时，每个线程对同一个表需要操作的数据       */
/************************************************************************/
void thread_concurrent_func_operate(CPerformFactory::PerformType type, EntrySetID entrySetId, uint32 trans_count,const std::vector<std::string>& vData)
{
	StorageEngine::getStorageEngine()->beginThread();

	CPerformFactory factory;
	CPerformBase* pOperate = factory.GetInstance(type);
	pOperate->SetDelData(vData);//并发删除、更新时用

	std::string strTaskInfo = "";
	for (uint32 i = 0; i < trans_count; i ++)
	{
		pOperate->Begin(strTaskInfo,entrySetId,false);		
	}

	StorageEngine::getStorageEngine()->endThread();

	delete pOperate;
}

//开启十个线程同时操作同一个表，每个线程开启trans_count个事务
bool test_perform_heap_concurrent_adapter(CPerformFactory::PerformType type,
										  bool bIsIndex, 
										  uint32 trans_count,
										  const std::string strFmt)
{
	bool bRet = true;

	CPerformFactory factory;
	CPerformBase* pPerform = factory.GetInstance(CPerformFactory::PERFORM_INSERT);
	if (!pPerform)
		return false;

	uint32 colid = 0;
	EntrySetID entrySetId = pPerform->Create_EntrySet(false,bIsIndex, colid);
	if (InvalidEntrySetID == entrySetId)
		return false;

	if (CPerformFactory::PERFORM_SEARCH == type)
	{
		CPerformFactory factory;
		CPerformBase* pInsert = factory.GetInstance(CPerformFactory::PERFORM_INSERT);
		bRet = pInsert->Begin("",entrySetId,false);
		delete pInsert;
	}
	if (CPerformFactory::PERFORM_INDEX_SEARCH == type)
	{
		bRet = InsertData_Ex(entrySetId);
		assert(g_vStrIndex.size() != 0);
	}
	if (CPerformFactory::PERFORM_DELETE == type || CPerformFactory::PERFORM_UPDATE == type)
	{
		bRet = MyInsertFixData(entrySetId,GetDataCount());
		assert(g_thread_count == g_FixData.size());//防止越界，下面需要通过下标访问表
	}

	if(!bRet)
		return false;

	boost::timer t;
	boost::thread_group tg;

	for (uint32 nIndex = 0; nIndex < g_thread_count; nIndex ++)
	{
		std::vector<std::string> vThreadData;
		if (CPerformFactory::PERFORM_INDEX_SEARCH == type)
		{
			if (nIndex < g_vStrIndex.size())
				vThreadData.push_back(g_vStrIndex.at(nIndex));
			else
				vThreadData.push_back(g_vStrIndex.at(g_vStrIndex.size() - 1));
		}
		if(CPerformFactory::PERFORM_DELETE == type || CPerformFactory::PERFORM_UPDATE == type)
		{
			vThreadData.push_back(g_FixData.at(nIndex));
		}
		tg.create_thread(boost::bind(thread_concurrent_func_operate, type, entrySetId,trans_count,vThreadData));
	}
	tg.join_all();
	double elapse = t.elapsed();

	if (CPerformFactory::PERFORM_DELETE == type)
		bRet = IsHeapEmpty(entrySetId);

	pPerform->ClearTask();
	delete pPerform;

	std::string strTaskInfo = strFmt;

	if (bRet)
		pPerform->WriteStatInfo2File_Ex(strTaskInfo,elapse);

	return bRet;
}
bool test_perform_heap_insert_concurrent()
{
	std::string strFmt = "INSERT_CONCURRENT";
	return test_perform_heap_concurrent_adapter(CPerformFactory::PERFORM_INSERT,false,1,strFmt);
}
bool test_perform_heap_insert_concurrent_index()
{
	std::string strFmt = "INSERT_INDEX_CONCURRENT";
	return test_perform_heap_concurrent_adapter(CPerformFactory::PERFORM_INSERT,true,1,strFmt);
}

bool test_perform_heap_traverse_concurrent()
{
	std::string strFmt = "TRAVERSE_CONCURRENT";
	return test_perform_heap_concurrent_adapter(CPerformFactory::PERFORM_SEARCH,false,1,strFmt);
}
bool test_perform_heap_traverse_concurrent_index()
{
	std::string strFmt = "TRAVERSE_INDEX_CONCURRENT";
	return test_perform_heap_concurrent_adapter(CPerformFactory::PERFORM_INDEX_SEARCH,true,1,strFmt);
}

bool test_perform_heap_update_concurrent()
{
	std::string strFmt = "UPDATE_CONCURRENT";
	return test_perform_heap_concurrent_adapter(CPerformFactory::PERFORM_UPDATE,false,1,strFmt);
}
bool test_perform_heap_update_concurrent_index()
{
	std::string strFmt = "UPDATE_INDEX_CONCURRENT";
	return test_perform_heap_concurrent_adapter(CPerformFactory::PERFORM_UPDATE,true,1,strFmt);
}
bool test_perform_heap_delete_concurrent()
{
	std::string strFmt = "DELETE_CONCURRENT";
	return test_perform_heap_concurrent_adapter(CPerformFactory::PERFORM_DELETE,false,1,strFmt);
}
bool test_perform_heap_delete_concurrent_index()
{
	std::string strFmt = "DELETE_INDEX_CONCURRENT";
	return test_perform_heap_concurrent_adapter(CPerformFactory::PERFORM_DELETE,true,1,strFmt);
}

bool perform_heap_insert_entries(const EntrySetID& entrySetId, const std::vector<std::string>& vData)
{
	if (InvalidEntrySetID == entrySetId)
		return false;

	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	Transaction* pTrans = NULL;

	try
	{
		TransactionId transId = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		std::vector<DataItem> vItem;
		std::vector<std::string>::const_iterator it;
		for (it = vData.begin(); it != vData.end(); it ++)
		{
			DataItem item;
			item.setData((void*)it->c_str());
			item.setSize(it->size());
			vItem.push_back(item);
		}
		pEntrySet->insertEntries(pTrans,vItem);

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}
	catch(std::exception &ex)
	{
		std::cout<<ex.what()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}
bool perform_heap_insert_trans(bool bIndex)
{
	//create heap
	CPerformFactory factory;
	CPerformBase *pTraverse = factory.GetInstance(CPerformFactory::PERFORM_SEARCH);

	uint32 colid = 0;
	EntrySetID entrySetId = pTraverse->Create_EntrySet(false,bIndex,colid);
	if (InvalidEntrySetID == entrySetId)
		return false;

	//分配内存和数据
	TestData data;
	data.tuple_count = GetDataCount();
	data.tuple_len = GetTupleLen();
	GetTestDataPtr(data);
	char* pData = data.pData;
	char* pTupleData = (char*)malloc(data.tuple_len + 1);
	memset(pTupleData,0,data.tuple_len + 1);

	//insert data
	bool bRet = true;
	std::vector<std::string> vData;
	std::string strData;
	boost::timer t;
	for (uint32 i = 0; i < data.tuple_count; i ++)
	{
		memcpy(pTupleData,pData,data.tuple_len);
		strData = pTupleData;
		vData.clear();
		vData.push_back(strData);

		if (!perform_heap_insert_entries(entrySetId, vData))
		{
			bRet = false;
			break;
		}

		pData += data.tuple_len;
	}
	double dTime = t.elapsed();

	bRet = pTraverse->Begin("",entrySetId,false);

	pTraverse->WriteStatInfo2File_Ex("INSERT_TRANS_ONE_TUPLE",dTime);

	bool bClear = pTraverse->ClearTask();
	delete pTraverse;

	return bRet&&bClear;
}

bool test_perform_heap_insert_trans()
{
	return perform_heap_insert_trans(false);
}
bool test_perform_heap_insert_trans_index()
{
    return perform_heap_insert_trans(true);
}

bool Perform_Insert_Data(const TestData& data,const EntrySetID& entrySetId)
{
	if (InvalidEntrySetID == entrySetId)
		return false;

	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	Transaction* pTrans = NULL;

	try
	{
		TransactionId transId = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		uint32 flag = 0;
		char* pData = data.pData;
		char* pTupleData = (char*)malloc(data.tuple_len + 1);
		memset(pTupleData,0,data.tuple_len + 1);
		for (uint32 i = 0; i < data.tuple_count; i ++)
		{
			memcpy(pTupleData,pData,data.tuple_len);
			DataItem item;
			item.setData(pTupleData);
			item.setSize(data.tuple_len);
			EntryID eid;
			pEntrySet->insertEntry(pTrans,eid,item);

			pData += data.tuple_len;
			flag ++;
		}
		pTrans->commit();
		assert(flag == GetDataCount());
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}
	catch(std::exception &ex)
	{
		std::cout<<ex.what()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}
bool perform_heap_trans_query(const EntrySetID& entrySetId, const std::vector<ScanCondition>& vCon)
{
	if (InvalidEntrySetID == entrySetId)
		return false;

	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	Transaction* pTrans = NULL;

	try
	{
		TransactionId transId = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		unsigned int indexId = InvalidEntrySetID;
		{
			PGIndinfoData info;
			if(pEntrySet->getIndexInfo(pTrans,info))
			{
				indexId = info.index_array[0];
			}
		}

		IndexEntrySet* pIndex = StorageEngine::getStorageEngine()->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,indexId);

		uint32 count = 0;

		EntryID eid;
		DataItem item;

		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			count ++;
			std::string strData = (char*)item.getData();
			MemoryContext::deAlloc(item.getData());
		}

		pIndex->endEntrySetScan(pScan);

		pTrans->commit();

		assert(count > 0);
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}
	catch(std::exception &ex)
	{
		std::cout<<ex.what()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}
bool test_perform_heap_query_trans_index()
{
	//create heap
	CPerformFactory factory;
	CPerformBase* pTraverse = factory.GetInstance(CPerformFactory::PERFORM_SEARCH);
	uint32 colid = 0;
	EntrySetID entrySetId = pTraverse->Create_EntrySet(false,true,colid);
	if(InvalidEntrySetID == entrySetId)
	{
		delete pTraverse;
		return false;
	}

	//insert data
	TestData data;
	data.tuple_len = GetTupleLen();
	data.tuple_count = GetDataCount();
	GetTestDataPtr(data);

	if(!Perform_Insert_Data(data,entrySetId))
	{
		delete pTraverse;
		return false;
	}

	//transaction query
	bool bRet = true;
	char* pData = data.pData;
	std::vector<ScanCondition> vCon;
	boost::timer t;
	for (uint32 i = 0; i < data.tuple_count; i ++)
	{
		vCon.clear();
		ScanCondition con(1,ScanCondition::Equal,(se_uint64)pData,data.tuple_len,str_compare);
		vCon.push_back(con);
		if (!perform_heap_trans_query(entrySetId,vCon))
		{
			bRet = false;
			break;
		}
	}
	double dTime = t.elapsed();
	pTraverse->WriteStatInfo2File_Ex("TRANS_QUERY",dTime);

	//clear task
	bool bClear = pTraverse->ClearTask();
	delete pTraverse;

	return bRet&&bClear;
}
//////////////////////////////////////////////////////////////////////////

/************************************************************************/
/* CFactoryPerform 获取一个执行实例                                     */
/************************************************************************/
CPerformBase* CPerformFactory::GetInstance(PerformType type)
{
	CPerformBase* pPerform = NULL;

	switch(type)
	{
	case PERFORM_INSERT:
		pPerform = new CPerformInsert();
		break;
	case PERFORM_DELETE:
		pPerform = new CPerformDelete();
		break;
	case PERFORM_UPDATE:
		pPerform = new CPerformUpdate();
		break;
	case PERFORM_SEARCH:
		pPerform = new CPerformSearch();
		break;
	case PERFORM_INDEX_SEARCH:
		pPerform = new CPerformIndexSearch();
		break;
	case PERFORM_INSERT_CONCURRENT:
		pPerform = new CPerformInsertConcurrent();
		break;
	default:
		break;
	}

	return pPerform;
}

/************************************************************************/
/* CPerformInsert 测试插入记录性能                                      */
/************************************************************************/
void CPerformInsert::Execute(Transaction* pTrans, EntrySet* pEntrySet)
{
	InsertData(pTrans,pEntrySet,GetDataCount());
}
CPerformInsert::~CPerformInsert()
{

}
/************************************************************************/
/* CPerformInsertConcurrent 测试并发插入性能                            */
/************************************************************************/
void CPerformInsertConcurrent::Execute(Transaction* pTrans, EntrySet* pEntrySet)
{
	uint32 count = GetDataCount()/g_thread_count;

	InsertData(pTrans, pEntrySet, count);
}

CPerformInsertConcurrent::~CPerformInsertConcurrent()
{

}
/************************************************************************/
/* CPerformSearch 测试遍历元组的性能                                    */
/************************************************************************/
void CPerformSearch::Execute(Transaction* pTrans, EntrySet* pEntrySet)
{
	std::vector<ScanCondition> con;
	EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC,con);

	uint32 count = 0;

	EntryID eid;
	DataItem item;

	while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
	{
		count ++;
		std::string strTmp = (char*)item.getData();
		MemoryContext::deAlloc((void*)item.getData());
	}

	pEntrySet->endEntrySetScan(pScan);

	assert(count == GetDataCount());
}
CPerformSearch::~CPerformSearch()
{

}
/************************************************************************/
/* CPerformIndexSearch 测试根据index查找元组的性能                      */
/************************************************************************/
void CPerformIndexSearch::Execute(Transaction* pTrans, EntrySet* pEntrySet)
{
	assert(g_vStrIndex.size() > 0);
	std::string strDes = g_vStrIndex.at(0);
	std::vector<ScanCondition> vCon;
	ScanCondition con(1,ScanCondition::LessEqual,(se_uint64)strDes.c_str(),strDes.size(),str_compare);
	vCon.push_back(con);

	EntrySetID indexId = InvalidEntrySetID;
	PGIndinfoData info;
	if(pEntrySet->getIndexInfo(pTrans,info))
	{
		indexId = info.index_array[0];
	}

	IndexEntrySet* pIndex = StorageEngine::getStorageEngine()->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,indexId);

	uint32 count = 0;

	EntryID eid;
	DataItem item;

	EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);
	while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
	{
		count ++;
		std::string strData = (char*)item.getData();
		MemoryContext::deAlloc(item.getData());
	}

	pIndex->endEntrySetScan(pScan);

	assert(count > 0);
}
CPerformIndexSearch::~CPerformIndexSearch()
{

}
/************************************************************************/
/* CPerformUpdate 测试更新元组的性能                                    */
/************************************************************************/
void CPerformUpdate::Execute(Transaction* pTrans, EntrySet* pEntrySet)
{
	std::vector<ScanCondition> vCon;
	EntrySetScan* pScn = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

	uint32 count = 0;

	DataItem item;
	EntryID eid;
	while (NO_DATA_FOUND != pScn->getNext(EntrySetScan::NEXT_FLAG,eid,item))
	{
		std::string strTmp = (char*)item.getData();
		if (IsNeedDelOrUpdate(strTmp))
		{
			DataItem update;
			std::string strData = GenerateRandomString(GetTupleLen());
			update.setData((void*)strData.c_str());
			update.setSize(strData.size());
			pEntrySet->updateEntry(pTrans,eid,update);

			count ++;
		}

		MemoryContext::deAlloc(item.getData());
	}

	pEntrySet->endEntrySetScan(pScn);

	//assert(count == GetDataCount());
}
CPerformUpdate::~CPerformUpdate()
{

}

/************************************************************************/
/* CPerformDelete 测试删除元组的性能                                    */
/************************************************************************/
void CPerformDelete::Execute(Transaction* pTrans, EntrySet* pEntrySet)
{
	std::vector<ScanCondition> vCon;
	EntrySetScan* pScn = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

	uint32 count = 0;

	DataItem item;
	EntryID eid;
	while (NO_DATA_FOUND != pScn->getNext(EntrySetScan::NEXT_FLAG,eid,item))
	{
		std::string strTmp = (char*)item.getData();
		if (IsNeedDelOrUpdate(strTmp))
		{
			pEntrySet->deleteEntry(pTrans,eid);
			count ++;
		}

		MemoryContext::deAlloc(item.getData());
	}
	pEntrySet->endEntrySetScan(pScn);

	//assert(count == GetDataCount()/g_thread_count);
}
CPerformDelete::~CPerformDelete()
{

}