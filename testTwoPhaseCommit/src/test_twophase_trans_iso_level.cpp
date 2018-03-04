#include "test_twophase_trans_iso_level.h"
#include "test_twophase_utils.h"
#include "StorageEngine.h"
#include <vector>
#include <set>
#include <cstring>
#include <algorithm>
#include "boost/thread/thread.hpp"

using namespace FounderXDB::StorageEngineNS;

void twophase_index_split_trans(RangeDatai& range,const char* src,int col,size_t len)
{
	range.start = 0;
	range.len = 2;
}
uint32 twophase_setColIndex_trans()
{
	ColumnInfo* col = new ColumnInfo;
	col->keys = 1;
	col->col_number = new size_t[1];
	col->col_number[0] = 2;
	col->rd_comfunction = new CompareCallbacki[col->keys];
	col->rd_comfunction[0] = str_compare;
	col->split_function = twophase_index_split_trans;

	const uint32 INDEX_COLID = 10002;
	setColumnInfo(INDEX_COLID,col);

	return INDEX_COLID;
}
void twophase_heap_split_trnas(RangeDatai& range,const char* src,int col,size_t len)
{
	range.start = 0;
	range.len = 0;
	if (!src)
		return ;
	if (1 == col)
	{
		range.len = 1;
	}
	if (2 == col)
	{
		range.start = 1;
		range.len = 2;
	}
	if (3 == col)
	{
		range.start = 3;
		range.len = 3;
	}
}
uint32 twophase_setColHeap_trans()
{
	ColumnInfo* col = new ColumnInfo;
	col->keys = 0;
	col->col_number = NULL;
	col->split_function = twophase_heap_split_trnas;
	col->rd_comfunction = NULL;

	const uint32 HEAP_COLID = 10001;
	setColumnInfo(HEAP_COLID,col);

	return HEAP_COLID;
}

bool twophase_trans_create_heap(EntrySetID& heapid, EntrySetID& indexid,bool bIndex = false)
{
	heapid = InvalidEntrySetID;
	indexid = InvalidEntrySetID;

	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		uint32 heap_colid = twophase_setColHeap_trans();
		heapid = pSE->createEntrySet(pTrans,heap_colid);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		if (bIndex)
		{
			uint32 index_colid = twophase_setColIndex_trans();
			indexid = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,index_colid);
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

bool twophase_trans_heap_insert(const EntrySetID& heapid, const std::vector<std::string>& vData)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		
		DataItem item;
		EntryID eid;
		std::vector<std::string>::const_iterator it = vData.begin();
		for (;it != vData.end(); ++ it)
		{
			item.setData((void*)it->c_str());
			item.setSize(it->size());
			pEntrySet->insertEntry(pTrans,eid,item);
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

/************************************************************************/
/* Read Commited --- No Dirty Read                                      */
/************************************************************************/
//事务T1修改tuple1，不提交；事务T2读取tuple1，不应该读取到T1修改后的tuple1的值

bool g_bPrepare = false;
bool g_bCommit = false;
bool g_bEnd = false;

void thread_read_commit_read(const EntrySetID& heapid, const std::set<std::string>& setPrepare, const std::set<std::string>& setCommit,bool* bRet)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	bool bReadPrepare = false;
	bool bReadCommit = false;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);

		while (!g_bPrepare){	}

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		std::set<std::string> setScan;
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			setScan.insert(strTuple);
		}
		pEntrySet->endEntrySetScan(pScan);
		bReadPrepare = (setPrepare == setScan);

		g_bEnd = true;
		while (!g_bCommit){	}

		setScan.clear();
		pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			setScan.insert(strTuple);
		}
		pEntrySet->endEntrySetScan(pScan);
		bReadCommit = (setCommit == setScan);

		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();
		pSE->detachThread();

		*bRet = bReadPrepare && bReadCommit;
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}
}
void thread_read_commit_modify(const EntrySetID& heapid, const std::string& strSrc, const std::string& strUpdate, bool* bRet)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();
		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);

		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);

		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			if (strTuple == strSrc)
			{
				item.setData((void*)strUpdate.c_str());
				item.setSize(strUpdate.size());
				pEntrySet->updateEntry(pTrans,eid,item);
				*bRet = true;
				break;
			}
		}
		pEntrySet->endEntrySetScan(pScan);

		pTrans->prepare();
		g_bPrepare = true;

		while (!g_bEnd){	}

		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();
		g_bCommit = true;

		pSE->detachThread();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}
}
bool test_twophase_trans_read_commit()
{
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::string strData = "123456";
	std::string strUpdate = "654321";

	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back("taobao");
	vData.push_back(strData);
	vData.push_back("jingdo");
	twophase_trans_heap_insert(heapid,vData);

	g_bPrepare = false;
	g_bEnd = false;
	g_bCommit = false;

	boost::thread_group g;
	bool bTrans1 = false;
	g.create_thread(boost::bind(thread_read_commit_modify,heapid,strData,strUpdate,&bTrans1));

	std::set<std::string> setPrepare(vData.begin(),vData.end());
	std::set<std::string> setCommit(vData.begin(),vData.end());
	setCommit.erase(strData);
	setCommit.insert(strUpdate);
	bool bTrans2 = false;
	g.create_thread(boost::bind(thread_read_commit_read,heapid,setPrepare,setCommit,&bTrans2));

	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bTrans1&&bTrans2&&bClear;
}

/************************************************************************/
/* Repeatable Read                                                      */
/************************************************************************/
bool bReadFirst = false;
bool bReadSecond = false;
bool bModify = false;
void twophase_trans_repeat_modify(const EntrySetID& heapid, const std::string& strSrc, const std::string& strUpdate, bool* bRet)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();
		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);

	    //wait bReadFirst
		while (!bReadFirst){	}

		//modify
        EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			if (strTuple == strSrc)
			{
				item.setData((void*)strUpdate.c_str());
				item.setSize(strUpdate.size());
				pEntrySet->updateEntry(pTrans,eid,item);
				*bRet = true;
				break;
			}
		}
		pEntrySet->endEntrySetScan(pScan);
		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();

		bModify = true;

		pSE->detachThread();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}
}
void thread_twophase_trans_repeatable_read(const EntrySetID& heapid, Transaction::IsolationLevel iso_level, const std::set<std::string>& setData, bool* bRet)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();
		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,iso_level,gxid);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		std::vector<ScanCondition> keys;
		//read first
		std::set<std::string> setScan1;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			setScan1.insert(strTuple);
		}
		pEntrySet->endEntrySetScan(pScan);

		bReadFirst = true;

		//wait modify
		while (!bModify){	}

		//read second
		std::set<std::string> setScan2;
		EntrySetScan* pScan1 = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid1;
		DataItem item1;
		while (NO_DATA_FOUND != pScan1->getNext(EntrySetScan::NEXT_FLAG,eid1,item1))
		{
			std::string strTuple((char*)item1.getData());
			MemoryContext::deAlloc(item1.getData());
			setScan2.insert(strTuple);
		}
		pEntrySet->endEntrySetScan(pScan1);

		//commit
		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();

		pSE->detachThread();

		*bRet = (setData == setScan1 && setData == setScan2);
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}
}

bool test_twophase_trans_repeatable_update()
{
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::string strData = "123456";
	std::string strUpdate = "654321";

	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back("taobao");
	vData.push_back(strData);
	vData.push_back("jingdo");
	twophase_trans_heap_insert(heapid,vData);

	boost::thread_group g;
	bool bModify = false;
	g.create_thread(boost::bind(twophase_trans_repeat_modify,heapid,strData,strUpdate,&bModify));

	bool bRead = false;
	std::set<std::string> setScan(vData.begin(),vData.end());
	g.create_thread(boost::bind(thread_twophase_trans_repeatable_read,heapid,Transaction::REPEATABLE_READ_ISOLATION_LEVEL , setScan,&bRead));
	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bModify && bRead && bClear;
}

enum REPEAT_TYPE
{
	REPEAT_INSERT,
	REPEAT_DELETE
};

void twophase_trans_repeat_del_insert(REPEAT_TYPE type,const EntrySetID& heapid, const std::set<std::string>& setData, bool* bRet)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();
		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);

		//wait bReadFirst
		while (!bReadFirst){	}

		//modify
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		EntryID eid;
		DataItem item;
		if (REPEAT_DELETE == type)
		{
			std::vector<ScanCondition> keys;
			EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
			
			while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
			{
				std::string strTuple((char*)item.getData());
				MemoryContext::deAlloc(item.getData());
				if (setData.end() != setData.find(strTuple))
				{
					pEntrySet->deleteEntry(pTrans,eid);
				}
			}
			pEntrySet->endEntrySetScan(pScan);
		}
		if (REPEAT_INSERT == type)
		{
			std::set<std::string>::const_iterator it = setData.begin();
			for (; it != setData.end(); ++ it)
			{
				item.setData((void*)it->c_str());
				item.setSize(it->size());
				pEntrySet->insertEntry(pTrans,eid,item);
			}
		}
		
		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();

		*bRet = true;
		bModify = true;

		pSE->detachThread();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		*bRet = false;
	}
}
bool test_twophase_trans_repeatable_delete()
{
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back("taobao");
	vData.push_back("123456");
	vData.push_back("jingdo");
	vData.push_back("1haodi");
	vData.push_back("tmalll");
	twophase_trans_heap_insert(heapid,vData);

	std::set<std::string> setDel(vData.begin(),vData.begin() + 3);

	bReadFirst = false;
	bReadSecond = false;
	bModify = false;

	boost::thread_group g;
	bool bUpdate = false;
	g.create_thread(boost::bind(twophase_trans_repeat_del_insert,REPEAT_DELETE,heapid,setDel,&bUpdate));

	std::set<std::string> setData(vData.begin(),vData.end());
	bool bRead = false;
	g.create_thread(boost::bind(thread_twophase_trans_repeatable_read, heapid,Transaction::REPEATABLE_READ_ISOLATION_LEVEL, setData,&bRead));
	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bUpdate && bRead && bClear;
}

bool test_twophase_trans_repeatable_insert()
{
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back("taobao");
	vData.push_back("123456");
	vData.push_back("jingdo");
	vData.push_back("1haodi");
	vData.push_back("tmalll");
	twophase_trans_heap_insert(heapid,vData);

	std::set<std::string> setInsert;
	setInsert.insert("abcdef");
	setInsert.insert("dadihu");

	bReadFirst = false;
	bReadSecond = false;
	bModify = false;

	boost::thread_group g;
	bool bUpdate = false;
	g.create_thread(boost::bind(twophase_trans_repeat_del_insert,REPEAT_INSERT,heapid,setInsert,&bUpdate));

	std::set<std::string> setData(vData.begin(),vData.end());
	bool bRead = false;
	g.create_thread(boost::bind(thread_twophase_trans_repeatable_read,heapid,Transaction::REPEATABLE_READ_ISOLATION_LEVEL,setData,&bRead));
	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bUpdate && bRead && bClear;
}
/************************************************************************/
/* Serializable                                                         */
/************************************************************************/
bool g_bSerialTransPrepare = false;
bool g_bSerialTransEnd = false;

enum SERIALIZABLE_TYPE
{
	SERIALIZABLE_TYPE_UPDATE,
	SERIALIZABLE_TYPE_DELETE,
	SERIALIZABLE_TYPE_INSERT
};
void thread_serializable_trans_01(SERIALIZABLE_TYPE type,const EntrySetID& heapid, const std::string& strSrc, const std::string& strUpdate, bool* bRet)
{
	*bRet = false;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::SERIALIZABLE_ISOLATION_LEVEL,gxid);

		//modify
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			if (strTuple == strSrc)
			{
				item.setData((void*)strUpdate.c_str());
				item.setSize(strUpdate.size());
				if (SERIALIZABLE_TYPE_UPDATE == type)
					pEntrySet->updateEntry(pTrans,eid,item);
				if (SERIALIZABLE_TYPE_DELETE == type)
					pEntrySet->deleteEntry(pTrans,eid);
			}
		}

		if (SERIALIZABLE_TYPE_INSERT == type)
		{
			item.setData((void*)strSrc.c_str());
			item.setSize(strSrc.size());
			pEntrySet->insertEntry(pTrans,eid,item);
		}

		
		pEntrySet->endEntrySetScan(pScan);

		pTrans->prepare();
		g_bSerialTransPrepare = true;

		while (!g_bSerialTransEnd){	}

		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();
		*bRet = true;

		pSE->detachThread();
	}
	catch(SerializationFailure& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}
}

void thread_serializable_trans_02(SERIALIZABLE_TYPE type, const EntrySetID& heapid, const std::string& strSrc, const std::string& strUpdate, bool* bRet)
{
	*bRet = false;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();
		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::SERIALIZABLE_ISOLATION_LEVEL,gxid);

		while (!g_bSerialTransPrepare){	}

		//modify
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			if (strTuple == strSrc)
			{
				item.setData((void*)strUpdate.c_str());
				item.setSize(strUpdate.size());
				if (SERIALIZABLE_TYPE_UPDATE == type)
					pEntrySet->updateEntry(pTrans,eid,item);
				if (SERIALIZABLE_TYPE_DELETE == type)
					pEntrySet->deleteEntry(pTrans,eid);
			}
		}

		if (SERIALIZABLE_TYPE_INSERT == type)
		{
			item.setData((void*)strSrc.c_str());
			item.setSize(strSrc.size());
			pEntrySet->insertEntry(pTrans,eid,item);
		}

		pEntrySet->endEntrySetScan(pScan);
		
		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::SERIALIZABLE_ISOLATION_LEVEL,gxid);
		pTrans->commit();

		*bRet = false;
		g_bSerialTransEnd = true;

		pSE->detachThread();
	}
	catch(SerializationFailure& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		*bRet = true;
		g_bSerialTransEnd = true;
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		*bRet = true;
		g_bSerialTransEnd = true;
	}
}

bool test_twophase_trans_serializable_update()
{
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::string strData1 = "123456";
	std::string strUpdate1 = "654321";
	std::string strData2 = "taobao";
	std::string strUpdate2 = "tianma";

	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back(strData2);
	vData.push_back(strData1);
	vData.push_back("jingdo");
	twophase_trans_heap_insert(heapid,vData);

	g_bSerialTransPrepare = false;
	g_bSerialTransEnd = false;

	boost::thread_group g;
	bool bUpdate = false;
	g.create_thread(boost::bind(thread_serializable_trans_01,SERIALIZABLE_TYPE_UPDATE,heapid,strData1, strUpdate1, &bUpdate));

	bool bRead = false;
	g.create_thread(boost::bind(thread_serializable_trans_02,SERIALIZABLE_TYPE_UPDATE,heapid,strData2,strUpdate2,&bRead));
	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bUpdate && bRead && bClear;
}
bool test_twophase_trans_serializable_delete()
{
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back("world!");
	vData.push_back("taobao");
	vData.push_back("jingdo");
	twophase_trans_heap_insert(heapid,vData);

	g_bSerialTransPrepare = false;
	g_bSerialTransEnd = false;

	boost::thread_group g;
	bool bUpdate = false;
	g.create_thread(boost::bind(thread_serializable_trans_01,SERIALIZABLE_TYPE_DELETE,heapid,vData.at(1), "", &bUpdate));

	bool bRead = false;
	g.create_thread(boost::bind(thread_serializable_trans_02,SERIALIZABLE_TYPE_DELETE,heapid,vData.at(2),"",&bRead));
	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bUpdate && bRead && bClear;

}
bool test_twophase_trans_serializable_insert()
{

	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back("world!");
	vData.push_back("taobao");
	vData.push_back("jingdo");
	twophase_trans_heap_insert(heapid,vData);

	g_bSerialTransPrepare = false;
	g_bSerialTransEnd = false;

	boost::thread_group g;
	bool bUpdate = false;
	g.create_thread(boost::bind(thread_serializable_trans_01,SERIALIZABLE_TYPE_INSERT,heapid,"abcdef", "", &bUpdate));

	bool bRead = false;
	g.create_thread(boost::bind(thread_serializable_trans_02,SERIALIZABLE_TYPE_INSERT,heapid,"123456","",&bRead));
	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bUpdate && bRead && bClear;
}

/************************************************************************/
/* 两个事务同时操作同一个记录                                           */
/************************************************************************/
bool g_bConcurrent1 = false;
bool g_bConcurrent2 = false;
enum CONCURRENT_TYPE
{
	CONCURRENT_TYPE_UPDATE,
	CONCURRENT_TYPE_DELETE
};
void thread_concurrent_operation_01(Transaction::IsolationLevel iso_level,
									CONCURRENT_TYPE type,
									const EntrySetID& heapid, 
									const std::string& strSrc, 
									const std::string& strUpdate, 
									bool* bRet)
{
	*bRet = false;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,iso_level,gxid);

		//modify
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			if (strTuple == strSrc)
			{
				item.setData((void*)strUpdate.c_str());
				item.setSize(strUpdate.size());
				if (CONCURRENT_TYPE_UPDATE == type)
					pEntrySet->updateEntry(pTrans,eid,item);
				if (CONCURRENT_TYPE_DELETE == type)
					pEntrySet->deleteEntry(pTrans,eid);
			}
		}

		pEntrySet->endEntrySetScan(pScan);

		pTrans->prepare();
		g_bConcurrent1 = true;

		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(2000));

		pTrans = pSE->getTransaction(xid,iso_level,gxid);
		pTrans->commit();
		*bRet = true;
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		*bRet = false;
	}
    pSE->detachThread();
}
void thread_concurrent_operation_02(Transaction::IsolationLevel iso_level,
									CONCURRENT_TYPE type,
									const EntrySetID& heapid, 
									const std::string& strSrc, 
									const std::string& strUpdate, 
									bool* bRet)
{
	*bRet = false;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,iso_level,gxid);

		//modify
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			if (strTuple == strSrc)
			{
				item.setData((void*)strUpdate.c_str());
				item.setSize(strUpdate.size());

				while (!g_bConcurrent1){	}

				if (CONCURRENT_TYPE_UPDATE == type)
					pEntrySet->updateEntry(pTrans,eid,item);
				if (CONCURRENT_TYPE_DELETE == type)
					pEntrySet->deleteEntry(pTrans,eid);
			}
		}

		pEntrySet->endEntrySetScan(pScan);

		pTrans->prepare();
		g_bConcurrent2 = true;
		pTrans = pSE->getTransaction(xid,iso_level,gxid);
		pTrans->commit();
		*bRet = false;
	}
	catch(StorageEngineException& ex)
	{
		g_bConcurrent2 = true;
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		*bRet = true;
	}
	pSE->detachThread();
}

bool test_twophase_trans_concurrent_update()
{
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::string strData = "123456";
	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back("world!");
	vData.push_back("taobao");
	vData.push_back("jingdo");
	vData.push_back(strData);
	twophase_trans_heap_insert(heapid,vData);

	g_bConcurrent1 = false;
	g_bConcurrent2 = false;

	boost::thread_group g;
	bool bUpdate1 = false;
	g.create_thread(boost::bind(thread_concurrent_operation_01,Transaction::SERIALIZABLE_ISOLATION_LEVEL,CONCURRENT_TYPE_UPDATE,heapid,strData, "abcdef", &bUpdate1));

	bool bUpdate2 = false;
	g.create_thread(boost::bind(thread_concurrent_operation_02,Transaction::SERIALIZABLE_ISOLATION_LEVEL,CONCURRENT_TYPE_UPDATE,heapid,strData,"654321",&bUpdate2));
	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bUpdate1 && bUpdate2 && bClear;

}
bool test_twophase_trans_concurrent_delete()
{
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::string strData = "123456";
	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back("world!");
	vData.push_back("taobao");
	vData.push_back("jingdo");
	vData.push_back(strData);
	twophase_trans_heap_insert(heapid,vData);

	g_bConcurrent1 = false;
	g_bConcurrent2 = false;

	boost::thread_group g;
	bool bDelete1 = false;
	g.create_thread(boost::bind(thread_concurrent_operation_01,Transaction::READ_COMMITTED_ISOLATION_LEVEL,CONCURRENT_TYPE_DELETE,heapid,strData, "", &bDelete1));

	bool bDelete2 = false;
	g.create_thread(boost::bind(thread_concurrent_operation_02,Transaction::READ_COMMITTED_ISOLATION_LEVEL,CONCURRENT_TYPE_DELETE,heapid,strData,"",&bDelete2));
	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bDelete1 && bDelete2 && bClear;
}

//multi tasks
bool g_bMulTaskInsert = false;
bool g_bMulTaskDelete = false;
bool g_bMulTaskUpdate = false;
bool g_bMulTaskCommit = false;

void twophase_insert_task(EntrySetID setId,TransactionId xid,FXGlobalTransactionId gxid,const std::string& strInsert)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,setId);
		EntryID eid;
		DataItem item;
		item.setData((void*)strInsert.c_str());
		item.setSize(strInsert.size());
		pEntrySet->insertEntry(pTrans,eid,item);

		pSE->detachThread();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	g_bMulTaskInsert = true;
	while (!g_bMulTaskCommit){	}

	pSE->endThread();
}
void twophase_delete_task(EntrySetID setId,TransactionId xid,FXGlobalTransactionId gxid,const std::string& strDelete)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();


		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,setId);
		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			if (strTuple == strDelete)
			{
				pEntrySet->deleteEntry(pTrans,eid);
			}
		}
		pEntrySet->endEntrySetScan(pScan);

		pSE->detachThread();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	g_bMulTaskDelete = true;
	while (!g_bMulTaskCommit){	}

	pSE->endThread();
}
void twophase_update_task(EntrySetID setId,TransactionId xid,FXGlobalTransactionId gxid,const std::string& strSrc,const std::string& strUpdate)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();


		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,setId);
		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			if (strTuple == strSrc)
			{
				item.setData((void*)strUpdate.c_str());
				item.setSize(strUpdate.size());
				pEntrySet->updateEntry(pTrans,eid,item);
			}
		}
		pEntrySet->endEntrySetScan(pScan);

		pSE->detachThread();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	g_bMulTaskUpdate = true;
	while (!g_bMulTaskCommit){	}
	pSE->endThread();
}
void twophase_prepare_commit_task(FXGlobalTransactionId gxid,TransactionId xid)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();


		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		
		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();

		pSE->detachThread();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	g_bMulTaskCommit = true;
	pSE->endThread();
}

bool twophase_check_result(EntrySetID setId,const std::set<std::string>& setData)
{
	bool bRet = false;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,setId);
		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		std::set<std::string> setScan;
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strTuple((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
			setScan.insert(strTuple);
		}
		pEntrySet->endEntrySetScan(pScan);
		bRet = (setData == setScan);

		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
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

bool test_twophase_multi_task()
{
	//create entryset
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	std::string strDelete = "123456";
	std::string strSrc = "taobao";
	std::vector<std::string> vData;
	vData.push_back("hello!");
	vData.push_back("world!");
	vData.push_back(strSrc);
	vData.push_back("jingdo");
	vData.push_back(strDelete);
	twophase_trans_heap_insert(heapid,vData);

	StorageEngine* pSE = StorageEngine::getStorageEngine();
	TransactionId xid = InvalidTransactionID;
	FXGlobalTransactionId gxid = GetGlobalTxid();
	Transaction* pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
	pSE->detachThread();

	boost::thread_group g;

	//insert data
	std::string strInsert = "abcdef";
	vData.push_back(strInsert);
	g.create_thread(boost::bind(twophase_insert_task, heapid,xid,gxid,strInsert));

	while (!g_bMulTaskInsert){	}

	//delete data
	g.create_thread(boost::bind(twophase_delete_task,heapid,xid,gxid,strDelete));

	while (!g_bMulTaskDelete){	}

	//update data
	std::string strUpdate = "guomei";
	g.create_thread(boost::bind(twophase_update_task,heapid,xid,gxid,strSrc,strUpdate));

	while (!g_bMulTaskUpdate){	}

	//prepare and commit
	g.create_thread(boost::bind(twophase_prepare_commit_task,gxid,xid));
	g.join_all();

	//check result
	std::set<std::string> setData(vData.begin(),vData.end());
	setData.erase(strDelete);
	setData.erase(strSrc);
	setData.insert(strUpdate);
	bool bCheck = twophase_check_result(heapid,setData);

	//drop entryset
	bool bClear = TwophaseDropEntrySet(heapid);

	return bCheck && bClear;
}

bool test_twophase_ops_after_prepare()
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,GetGlobalTxid());

		uint32 heap_colid = twophase_setColHeap_trans();
		EntrySetID heapid = pSE->createEntrySet(pTrans,heap_colid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		uint32 index_colid = twophase_setColIndex_trans();
		EntrySetID indexid = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,index_colid);

		pSE->removeEntrySet(pTrans,heapid);

		pTrans->prepare();

		pTrans->commit();

		//pSE->deleteTransaction(xid);

		//pTrans = pSE->GetCurrentTransaction();

		//pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}


/************************************************************************/
/*                                                                      */
/************************************************************************/

bool g_bLock = false;

void thread_twophase_lock_conflict_01(EntrySetID setId, EntrySet::EntrySetOpenFlags flag, bool* bRet)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,flag,setId);

		pTrans->prepare();
		g_bLock = true;

		boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(2000));

		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();

		*bRet = true;
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		*bRet = false;
	}
	pSE->endThread();
}
void thread_twophase_lock_conflict_02(EntrySetID setId,EntrySet::EntrySetOpenFlags flag,bool* bRet)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);

		while (!g_bLock){	}

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,flag,setId);

		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();

		*bRet = true;
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		*bRet = false;
	}
	pSE->endThread();
}
bool test_twophase_lock_conflict()
{
	//create entryset
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid,indexid,false);

	if (heapid == InvalidTransactionID)
		return false;

	boost::thread_group g;

	//第一个事务为共享锁，第二个事务为独占锁
	bool bLock1 = false;
	bool bLock2 = false;
	g.create_thread(boost::bind(thread_twophase_lock_conflict_01,heapid,EntrySet::OPEN_SHARED,&bLock1));
	g.create_thread(boost::bind(thread_twophase_lock_conflict_02,heapid,EntrySet::OPEN_EXCLUSIVE,&bLock2));
	g.join_all();

	//第一个事务为独占锁，第二个事务为共享锁
	g_bLock = false;
	bool bLock3 = false;
	bool bLock4 = false;
	g.create_thread(boost::bind(thread_twophase_lock_conflict_01,heapid,EntrySet::OPEN_EXCLUSIVE,&bLock3));
	g.create_thread(boost::bind(thread_twophase_lock_conflict_02,heapid,EntrySet::OPEN_SHARED,&bLock4));
	g.join_all();

	//两个事务都为独占锁
	g_bLock = false;
	bool bLock5 = false;
	bool bLock6 = false;
	g.create_thread(boost::bind(thread_twophase_lock_conflict_01,heapid,EntrySet::OPEN_EXCLUSIVE,&bLock5));
	g.create_thread(boost::bind(thread_twophase_lock_conflict_02,heapid,EntrySet::OPEN_EXCLUSIVE,&bLock6));
	g.join_all();

	bool bClear = TwophaseDropEntrySet(heapid);

	return bLock1 && bLock2 && bLock3 && bLock4 && bLock5 && bLock6 && bClear;
}

//多并发操作
void thread_twophase_concurrent_insert(EntrySetID heapid,std::vector<uint32> vData,uint32 tup_size)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);

		DataItem item;
		EntryID eid;
		for (std::vector<uint32>::iterator it = vData.begin(); it != vData.end(); ++it)
		{
			uint32 unValue = *it;
			item.setData((void*)(&unValue));
			item.setSize(tup_size);
			pEntrySet->insertEntry(pTrans,eid,item);
		}

		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}
	pSE->endThread();
}
void thread_twophase_concurrent_delete(EntrySetID heapid, std::set<uint32> setData,uint32 tup_size)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	uint32 flag = 0;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);

		//for (std::set<uint32>::iterator it = setData.begin(); it != setData.end(); ++it)
		//{
		//	uint32 unKey = *it;
		//	std::vector<ScanCondition> keys;
		//	ScanCondition con(1,ScanCondition::Equal,se_uint64(&unKey),tup_size,str_compare);
		//	keys.push_back(con);
		//	EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		//	EntryID eid;
		//	DataItem item;
		//	while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		//	{
		//		pEntrySet->deleteEntry(pTrans,eid);
		//		MemoryContext::deAlloc(item.getData());
		//		++flag;
		//	}
		//	pEntrySet->endEntrySetScan(pScan);
		//}

		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			uint32 unValue = *((uint32*)item.getData());
			if (std::find(setData.begin(),setData.end(),unValue) != setData.end())
				pEntrySet->deleteEntry(pTrans,eid);
			MemoryContext::deAlloc(item.getData());
			++flag;
		}
		pEntrySet->endEntrySetScan(pScan);
		

		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}
	pSE->endThread();
}
void thread_twophase_concurrent_update(EntrySetID heapid,std::map<uint32,uint32> mapData,uint32 tup_size)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	uint32 flag = 0;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);

		for (std::map<uint32,uint32>::iterator it = mapData.begin(); it != mapData.end(); ++it)
		{
			std::vector<ScanCondition> keys;
			EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
			EntryID eid;
			DataItem item;
			while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
			{
				uint32 unTuple = *(uint32*)item.getData();
				if (unTuple == it->first)
				{
					MemoryContext::deAlloc(item.getData());
					item.setData((void*)(&it->second));
					item.setSize(tup_size);
					pEntrySet->updateEntry(pTrans,eid,item);
					++flag;
				}
			}
			pEntrySet->endEntrySetScan(pScan);
		}

		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}
	pSE->endThread();
}
void thread_twophase_concurrent_query(EntrySetID heapid,std::set<uint32> setQuery,uint32 tup_size)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);

		for (std::set<uint32>::iterator it = setQuery.begin(); it != setQuery.end(); ++it)
		{
			uint32 unKey = *it;

			std::vector<ScanCondition> keys;
			ScanCondition con(1,ScanCondition::Equal,se_uint64(&unKey),tup_size,str_compare);
			keys.push_back(con);
			EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
			EntryID eid;
			DataItem item;
			while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
			{
				uint32 unTuple;
				unTuple = *((uint32*)item.getData());
				MemoryContext::deAlloc(item.getData());
			}
			pEntrySet->endEntrySetScan(pScan);
		}

		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}
	pSE->endThread();
}
void thread_twophase_concurrent_traverse(EntrySetID heapid)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		pSE->beginThread();

		TransactionId xid = InvalidTransactionID;
		FXGlobalTransactionId gxid = GetGlobalTxid();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);

		std::vector<ScanCondition> keys;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,keys);
		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string uint32((char*)item.getData());
			MemoryContext::deAlloc(item.getData());
		}
		pEntrySet->endEntrySetScan(pScan);

		pTrans->prepare();
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}
	pSE->endThread();
}

void twophase_index_split_concurrent(RangeDatai& range,const char* src,int col,size_t len)
{
	range.start = 0;
	range.len = 6;
}
uint32 twophase_setColIndex_concurrent()
{
	ColumnInfo* col = new ColumnInfo;
	col->keys = 1;
	col->col_number = new size_t[1];
	col->col_number[0] = 1;
	col->rd_comfunction = new CompareCallbacki[col->keys];
	col->rd_comfunction[0] = str_compare;
	col->split_function = twophase_index_split_concurrent;

	const uint32 INDEX_COLID = 10003;
	setColumnInfo(INDEX_COLID,col);

	return INDEX_COLID;
}
void twophase_heap_split_concurrent(RangeDatai& range,const char* src,int col,size_t len)
{
	range.start = 0;
	range.len = 0;
	if (!src)
		return ;
	range.len = 6;
}
uint32 twophase_setColHeap_concurrent()
{
	ColumnInfo* col = new ColumnInfo;
	col->keys = 0;
	col->col_number = NULL;
	col->split_function = twophase_heap_split_concurrent;
	col->rd_comfunction = NULL;

	const uint32 HEAP_COLID = 10004;
	setColumnInfo(HEAP_COLID,col);

	return HEAP_COLID;
}
void twophase_multi_concurrent_create_heap(EntrySetID& heapid, EntrySetID& indexid,bool bIndex = false)
{
	heapid = InvalidEntrySetID;
	indexid = InvalidEntrySetID;

	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		uint32 heap_colid = twophase_setColHeap_concurrent();
		heapid = pSE->createEntrySet(pTrans,heap_colid);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapid);
		if (bIndex)
		{
			uint32 index_colid = twophase_setColIndex_concurrent();
			indexid = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,index_colid);
		}

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}
}
bool test_twophase_high_concurrent()
{
	EntrySetID heapid = InvalidTransactionID;
	EntrySetID indexid = InvalidTransactionID;
	twophase_trans_create_heap(heapid, indexid, true);

	std::vector<uint32> vData;
	for (uint32 i = 0; i < 100000; ++i)
	{
		vData.push_back(i);
	}

	boost::thread_group g;
	g.create_thread(boost::bind(thread_twophase_concurrent_insert,heapid,vData,6));
	g.join_all();

	std::vector<uint32> vInsert;
	for (uint32 i = 0; i < 10000; ++i)
	{
		vInsert.push_back(i);
	}

	std::set<uint32> setDelete(vData.begin(),vData.begin() + 10000);

	std::map<uint32,uint32> mapUpdate;
	for (uint32 i = 5000; i < 15000; ++i)
	{
		mapUpdate.insert(std::make_pair(i,i+1));
	}

	std::set<uint32> setQuery(vData.begin(),vData.begin() + 20000);

	g.create_thread(boost::bind(thread_twophase_concurrent_insert,heapid,vInsert,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_delete,heapid,setDelete,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_update,heapid,mapUpdate,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_query,heapid,setQuery,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_traverse,heapid));

	g.create_thread(boost::bind(thread_twophase_concurrent_insert,heapid,vInsert,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_delete,heapid,setDelete,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_update,heapid,mapUpdate,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_query,heapid,setQuery,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_traverse,heapid));

	g.create_thread(boost::bind(thread_twophase_concurrent_insert,heapid,vInsert,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_delete,heapid,setDelete,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_update,heapid,mapUpdate,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_query,heapid,setQuery,6));
	g.create_thread(boost::bind(thread_twophase_concurrent_traverse,heapid));

	g.join_all();

	TwophaseDropEntrySet(heapid);
	return true;
}