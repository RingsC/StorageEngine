#include "boost/archive/text_iarchive.hpp"
#include "boost/archive/text_oarchive.hpp"
#include "boost/thread/thread.hpp"
#include <iostream>
#include <fstream>
#include "test_twophase_heap_operations.h"
#include "test_twophase_utils.h"
#include "boost/serialization/serialization.hpp"
#include "boost/serialization/set.hpp"
#include "StorageEngine.h"
#include "MemoryContext.h"

using namespace FounderXDB::StorageEngineNS;

extern std::map<std::string,std::string> ProgramOpts;
bool need_run_twophase()
{
	if (ProgramOpts.find("-twophase") != ProgramOpts.end())
		return true;
	return false;
}

bool check_heap_ops_success(EntrySetID entrySetId, const std::set<std::string>& setSrc)
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

		pTrans->commit();

		if (setSrc != setScan)
			bRet = false;
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}

	return bRet;
}

bool twophase_drop_relation(EntrySetID entrySetId)
{
	bool bRet = true;
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
		bRet = false;
	}

	return bRet;
}

enum HEAP_OPS_FLAGS
{
	HEAP_OPS_DELETE,
	HEAP_OPS_UPDATE,
	HEAP_OPS_INSERT,
	HEAP_OPS_CREATE_INDEX,
	HEAP_OPS_OTHERS
};
class TwophaseParas
{
public:
	FXGlobalTransactionId g_xid;
	TransactionId xid;
	Transaction::IsolationLevel iso_level;
	EntrySetID heapId;
	uint32 heapColId;
	EntrySetID indexid;
	uint32 indexColId;
	bool bIsIndex;
	bool bCommit;
	std::string strDelete;
	std::string strUpdateSrc;
	std::string strUpdateDes;
	std::string scanKey;
	std::string strInsert;
	std::set<std::string> setData;//all heap data
	HEAP_OPS_FLAGS flag;

	TwophaseParas()
	{
		g_xid = InvalidTransactionID;
		xid = InvalidTransactionID;
		iso_level = Transaction::READ_COMMITTED_ISOLATION_LEVEL;
		heapId = InvalidEntrySetID;
		heapColId = InvalidEntrySetID;
		indexid = InvalidEntrySetID;
		indexColId = InvalidEntrySetID;
		bIsIndex = false;
		bCommit = true;
	}

protected:
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & g_xid & xid & iso_level & heapId & heapColId & indexid & indexColId & bIsIndex\
			& bCommit & strDelete & strUpdateSrc & strUpdateDes & scanKey \
			& strInsert & setData & flag;
	}
};

std::string strSerialize = "object-serialize";

typedef bool (*Twophase_Func)(TwophaseParas&, Transaction*);

//************************************************************************/
//* utils functions                                                      */
//************************************************************************/
void twophase_index_split(RangeDatai& range,const char* src,int col,size_t len)
{
	range.start = 0;
	range.len = 2;
}
uint32 twophase_setColIndex()
{
	ColumnInfo* col = new ColumnInfo;
	col->keys = 1;
	col->col_number = new size_t[1];
	col->col_number[0] = 2;
	col->rd_comfunction = new CompareCallbacki[col->keys];
	col->rd_comfunction[0] = str_compare;
	col->split_function = twophase_index_split;

	const uint32 INDEX_COLID = 10002;
	setColumnInfo(INDEX_COLID,col);

	return INDEX_COLID;
}
void twophase_heap_split(RangeDatai& range,const char* src,int col,size_t len)
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
uint32 twophase_setColHeap()
{
	ColumnInfo* col = new ColumnInfo;
	col->keys = 0;
	col->col_number = NULL;
	col->split_function = twophase_heap_split;
	col->rd_comfunction = NULL;

	const uint32 HEAP_COLID = 10001;
	setColumnInfo(HEAP_COLID,col);

	return HEAP_COLID;
}

bool twophase_commit_heap_create_insert(TwophaseParas& para)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		
		para.heapColId = twophase_setColHeap();
		para.heapId = pSE->createEntrySet(pTrans,para.heapColId);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);
		if (para.bIsIndex)
		{
			para.indexColId = twophase_setColIndex();
			para.indexid = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,para.indexColId);
		}

		para.setData.insert("000111");
		para.setData.insert("XJdkek");
		para.setData.insert("-=1258");

		DataItem item;
		EntryID eid;
		std::set<std::string>::iterator setItor;
		for (setItor = para.setData.begin(); setItor != para.setData.end(); setItor ++)
		{
			item.setData((char*)setItor->c_str());
			item.setSize(setItor->size());
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

bool twophase_prepare_task(const Twophase_Func& func,TwophaseParas& para, bool bIsCrash)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		if (HEAP_OPS_OTHERS != para.flag)
			twophase_commit_heap_create_insert(para);//prepare work

		pSE = StorageEngine::getStorageEngine();
		
		para.iso_level = Transaction::READ_COMMITTED_ISOLATION_LEVEL;

		para.xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(para.xid, para.iso_level, para.g_xid);
		//do real trans work
		func(para,pTrans);
		pTrans->prepare();

		if (bIsCrash)
		{
			std::ofstream out_file(strSerialize.c_str());
			if (out_file.is_open())
			{
				boost::archive::text_oarchive ar_out(out_file);
				ar_out & para;
				out_file.close();
			}

			pSE->resetOnExit();
			exit(0);
		}
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		return false;
	}

	return true;
}

bool twophase_commit_task(const Twophase_Func& func,TwophaseParas& para, bool bIsCrash,bool bIsCommit = true)
{
	bool bCheck = false;
	bool bClear = false;

	try
	{
		if (bIsCrash)
		{
			std::ifstream in_file(strSerialize.c_str());
			if (in_file.is_open())
			{
				boost::archive::text_iarchive ar_in(in_file);
				ar_in & para;
				in_file.close();
			}
		}

		StorageEngine* pSE = StorageEngine::getStorageEngine();
		Transaction* pTrans = NULL;
		TransactionId xid = para.xid;
		if (bIsCrash)
			xid = InvalidTransactionID;
		
		pTrans = pSE->getTransaction(xid,para.iso_level,para.g_xid);

		if (bIsCommit)
		{
			pTrans->commit();
			para.setData.insert(para.strInsert);
			para.setData.insert(para.strUpdateDes);
			para.setData.erase(para.strUpdateSrc);
			para.setData.erase(para.strDelete);
		}
		else
		{
			pTrans->abort();
		}

		para.bCommit = bIsCommit;
		bCheck = func(para,NULL);
		bClear = twophase_drop_relation(para.heapId);
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;

		return false;
	}

	return bCheck&&bClear;
}
bool twophase_commit_clear_task(TwophaseParas& para)
{
	return twophase_drop_relation(para.heapId);
}

/************************************************************************/
/* heap create                                                          */
/************************************************************************/
bool twophase_heap_create_prepare(TwophaseParas& para,Transaction* trans)
{
	para.heapColId = twophase_setColHeap();
	StorageEngine* pSE = StorageEngine::getStorageEngine();
	Transaction* pTrans = pSE->getTransaction(para.xid,para.iso_level);

	para.heapId = pSE->createEntrySet(pTrans,para.heapColId);
	EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);

	if (para.bIsIndex)
	{
		para.indexColId = twophase_setColIndex();
		para.indexid = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,para.indexColId);
	}

	return true;
}
bool twophase_heap_create_commit(TwophaseParas& para,Transaction* trans)
{
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);
		pTrans->commit();
	}
	catch (StorageEngineException& ex)
	{
		pTrans->abort();
		if (para.bCommit)
			return false;
		return true;
	}
	if (!para.bCommit)
		return false;
	return true;
}
bool test_twophase_heap_create_prepare()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.bIsIndex = false;
	para.flag = HEAP_OPS_OTHERS;
	para.g_xid = GetGlobalTxid();
	return twophase_prepare_task(twophase_heap_create_prepare,para,true);
}
bool test_twophase_heap_create_rollback()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	return twophase_commit_task(twophase_heap_create_commit,para,true,false);
}

bool test_twophase_heap_create_rollback_ex()
{
	TwophaseParas para;
	para.g_xid = GetGlobalTxid();
	bool bPrepare = twophase_prepare_task(twophase_heap_create_prepare,para,false);
	bool bCommit = twophase_commit_task(twophase_heap_create_commit,para,false,false);
	return bPrepare && bCommit;
}

/************************************************************************/
/* heap insert                                                          */
/************************************************************************/
bool twophase_heap_insert_prepare(TwophaseParas& para,Transaction* pTrans)
{
	EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);

	std::string strInsert = "adfIop";
	para.scanKey = strInsert.substr(1,2);
	para.strInsert = strInsert;

	DataItem item;
	item.setData((void*)strInsert.c_str());
	item.setSize(strInsert.size());
	EntryID eid;
	pEntrySet->insertEntry(pTrans,eid,item);
	
	return true;
}

bool twophase_heap_insert_commit(TwophaseParas& para,Transaction* pTrans)
{
	return check_heap_ops_success(para.heapId,para.setData);
}

bool twophase_heap_insert_commit_index(TwophaseParas& para,Transaction* pTrans)
{
	bool bRet = check_heap_ops_success(para.heapId,para.setData);

	if (!bRet)
		return false;

	bRet = true;

	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		Transaction* pTrans = pSE->getTransaction(xid,para.iso_level);
		twophase_setColIndex();

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,para.indexid);

		std::vector<ScanCondition> vCon;
		ScanCondition con(1,ScanCondition::Equal,se_uint64(para.scanKey.c_str()),2,str_compare);
		vCon.push_back(con);

		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		uint32 count = 0;
		DataItem item;
		EntryID eid;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strItem = (char*)item.getData();
			if (strItem == para.strInsert)
				++ count;
		}
		pIndex->endEntrySetScan(pScan);
		pTrans->commit();

		if (1 != count)
			bRet = false;
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}

	return bRet;
}

bool test_twophase_heap_insert()
{
	TwophaseParas para;
	para.flag = HEAP_OPS_INSERT;
	para.g_xid = GetGlobalTxid();
	bool bPrepare = twophase_prepare_task(twophase_heap_insert_prepare,para,false);
	bool bCommit = twophase_commit_task(twophase_heap_insert_commit,para,false);
	return bPrepare&&bCommit;
}
bool test_twophase_heap_insert_rollback_ex()
{
	TwophaseParas para;
	para.g_xid = GetGlobalTxid();
	para.flag = HEAP_OPS_INSERT;
	bool bPrepare = twophase_prepare_task(twophase_heap_insert_prepare,para,false);
	bool bCommit = twophase_commit_task(twophase_heap_insert_commit,para,false,false);
	return bPrepare&&bCommit;
}

bool test_twophase_heap_insert_prepare()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.flag = HEAP_OPS_INSERT;
	para.g_xid = GetGlobalTxid();
	bool bPrepare = twophase_prepare_task(twophase_heap_insert_prepare,para,true);
	return bPrepare;
}

bool test_twophase_heap_insert_commit()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	bool bCommit = twophase_commit_task(twophase_heap_insert_commit,para,true);
	return bCommit;
}

bool test_twophase_heap_insert_rollback()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	bool bPrepare = twophase_commit_task(twophase_heap_insert_commit,para,true,false);
	return bPrepare;
}

bool test_twophase_heap_insert_prepare_index()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.bIsIndex = true;
	para.flag = HEAP_OPS_INSERT;
	para.g_xid = GetGlobalTxid();
	bool bRet = twophase_prepare_task(twophase_heap_insert_prepare,para,true);
	return bRet;
}
bool test_twophase_heap_insert_commit_index()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.bIsIndex = true;
	bool bRet1 = twophase_commit_task(twophase_heap_insert_commit_index,para,true);
	return bRet1;
}
bool test_twophase_heap_insert_index()
{
	TwophaseParas para;
	para.bIsIndex = true;
	para.g_xid = GetGlobalTxid();
	para.flag = HEAP_OPS_INSERT;
	bool bRet1 = twophase_prepare_task(twophase_heap_insert_prepare,para,false);
	bool bRet2 = twophase_commit_task(twophase_heap_insert_commit_index,para,false);
	return bRet1&&bRet2;

}

/************************************************************************/
/*       heap delete                                                    */
/************************************************************************/
bool twophase_heap_delete_prepare(TwophaseParas& para,Transaction* pTrans)
{
	EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);
	std::vector<ScanCondition> vCon;
	EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

	std::set<std::string>::iterator it = para.setData.begin();
	std::string subStr = *it;
	para.scanKey = subStr.substr(1,2);
	para.strDelete = *it;

	DataItem item;
	EntryID eid;

	while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
	{
		std::string strTuple = (char*)item.getData();
		if (strTuple == *it)
		{
			item.setData((void*)para.strDelete.c_str());
			pEntrySet->deleteEntry(pTrans,eid);
			break;
		}
	}
	pEntrySet->endEntrySetScan(pScan);

	return true;
}

bool twophase_heap_delete_commit(TwophaseParas& para,Transaction* pTrans)
{
	if (!para.bCommit)
	{
		para.setData.insert(para.strDelete);
	}
	bool bRet = check_heap_ops_success(para.heapId,para.setData);
	return bRet;
}

bool twophase_heap_delete_commit_index(TwophaseParas& para,Transaction* trans)
{
	bool bRet = check_heap_ops_success(para.heapId,para.setData);

	if (!bRet)
		return false;

	bRet = false;

	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		twophase_setColIndex();
        pSE = StorageEngine::getStorageEngine();
        TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,para.iso_level);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,para.indexid);

		std::vector<ScanCondition> vCon;
		ScanCondition con(1,ScanCondition::Equal,se_uint64(para.scanKey.c_str()),2,str_compare);

		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		uint32 count = 0;
		DataItem item;
		EntryID eid;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strItem = (char*)item.getData();
			if (strItem == para.strDelete)
				++ count;
		}
		pIndex->endEntrySetScan(pScan);
		pTrans->commit();

		if (para.bCommit)
		{
			bRet = (0 == count);
		}
		else
		{
			bRet = (1 == count);
		}
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		bRet = false;
	}

	return bRet;
}

bool test_twophase_heap_delete_prepare()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.flag = HEAP_OPS_DELETE;
	para.g_xid = GetGlobalTxid();
	return twophase_prepare_task(twophase_heap_delete_prepare,para,true);
}

bool test_twophase_heap_delete_commit()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	return twophase_commit_task(twophase_heap_delete_commit,para,true);
}
bool test_twophase_heap_delete_rollback()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	return twophase_commit_task(twophase_heap_delete_commit,para,true,false);
}
bool test_twophase_heap_delete()
{
	TwophaseParas para;
	para.flag = HEAP_OPS_DELETE;
	para.g_xid = GetGlobalTxid();
	bool bPrepare = twophase_prepare_task(twophase_heap_delete_prepare,para,false);
	bool bCommit = twophase_commit_task(twophase_heap_delete_commit,para,false);
	return bPrepare && bCommit;
}

bool test_twophase_heap_delete_rollback_ex()
{
	TwophaseParas para;
	para.flag = HEAP_OPS_DELETE;
	para.g_xid = GetGlobalTxid();
	bool bPrepare = twophase_prepare_task(twophase_heap_delete_prepare,para,false);
	bool bCommit = twophase_commit_task(twophase_heap_delete_commit,para,false,false);
	return bPrepare && bCommit;
}

bool test_twophase_heap_delete_prepare_index()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.bIsIndex = true;
	para.flag = HEAP_OPS_DELETE;
    para.g_xid = GetGlobalTxid();
	return twophase_prepare_task(twophase_heap_delete_prepare,para,true);
}

bool test_twophase_heap_delete_commit_index()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.bIsIndex = true;
	para.bCommit = true;
	return twophase_commit_task(twophase_heap_delete_commit_index,para,true);
}

bool test_twophase_heap_delete_index()
{
	TwophaseParas para;
	para.bIsIndex = true;
	para.bCommit = true;
	para.g_xid = GetGlobalTxid();
	bool bRet1 = twophase_prepare_task(twophase_heap_delete_prepare,para,false);
	bool bRet2 = twophase_commit_task(twophase_heap_delete_commit_index,para,false);
	return bRet1&&bRet2;
}
/************************************************************************/
/* heap update                                                          */
/************************************************************************/
bool twophase_heap_update_prepare(TwophaseParas& para,Transaction* pTrans)
{
	EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);
	std::vector<ScanCondition> vCon;
	EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

	std::set<std::string>::iterator it = para.setData.begin();
	std::string newData = "qwerty";
	para.strUpdateDes = newData;
	para.strUpdateSrc = *it;
	para.scanKey = "we";

	DataItem item;
	EntryID eid;

	while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
	{
		std::string strTuple = (char*)item.getData();
		if (strTuple == para.strUpdateSrc)
		{
			item.setData((void*)para.strUpdateDes.c_str());
			pEntrySet->updateEntry(pTrans,eid,item);
			break;
		}
	}
	pEntrySet->endEntrySetScan(pScan);

	return true;
}

bool twophase_heap_update_commit(TwophaseParas& para,Transaction* pTrans)
{
	if (!para.bCommit)
	{
		para.setData.erase(para.strUpdateDes);
		para.setData.insert(para.strUpdateSrc);
	}
	bool bRet = check_heap_ops_success(para.heapId,para.setData);
	return bRet;
}

bool twophase_heap_update_commit_index(TwophaseParas& para,Transaction* trans)
{
	bool bRet = check_heap_ops_success(para.heapId,para.setData);
	if (!bRet)
		return false;

	bRet = false;

	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		twophase_setColIndex();
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,para.iso_level);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,para.indexid);

		std::vector<ScanCondition> vCon;
		ScanCondition con(1,ScanCondition::Equal,se_uint64(para.scanKey.c_str()),2,str_compare);

		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		uint32 count = 0;
		DataItem item;
		EntryID eid;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			std::string strItem = (char*)item.getData();
			if (strItem == para.strUpdateDes)
				++ count;
		}
		pIndex->endEntrySetScan(pScan);
		pTrans->commit();

		if (para.bCommit)
			bRet = (1 == count);
		else
			bRet = (0 == count);
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		bRet = false;
	}

	return bRet;
}
bool test_twophase_heap_update_prepare()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.flag = HEAP_OPS_UPDATE;
	para.g_xid = GetGlobalTxid();
	return twophase_prepare_task(twophase_heap_update_prepare,para,true);
}

bool test_twophase_heap_update_commit()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	return twophase_commit_task(twophase_heap_update_commit,para,true);
}
bool test_twophase_heap_update_rollback()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	return twophase_commit_task(twophase_heap_update_commit,para,true,false);
}
bool test_twophase_heap_update()
{
	TwophaseParas para;
	para.flag = HEAP_OPS_UPDATE;
	para.g_xid = GetGlobalTxid();
	bool bPrepare = twophase_prepare_task(twophase_heap_update_prepare,para,false);
	bool bCommit = twophase_commit_task(twophase_heap_update_commit,para,false);
	return bPrepare && bCommit;
}

bool test_twophase_heap_update_rollback_ex()
{
	TwophaseParas para;
	para.flag = HEAP_OPS_UPDATE;
	para.g_xid = GetGlobalTxid();
	bool bPrepare = twophase_prepare_task(twophase_heap_update_prepare,para,false);
	bool bCommit = twophase_commit_task(twophase_heap_update_commit,para,false,false);
	return bPrepare && bCommit;
}
bool test_twophase_heap_update_prepare_index()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.bIsIndex = true;
	para.flag = HEAP_OPS_UPDATE;
	para.g_xid = GetGlobalTxid();
	return twophase_prepare_task(twophase_heap_update_prepare,para,true);
}

bool test_twophase_heap_update_commit_index()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.bIsIndex = true;
	return twophase_commit_task(twophase_heap_update_commit_index,para,true);
}

bool test_twophase_heap_update_index()
{
	TwophaseParas para;
	para.bIsIndex = true;
	para.flag = HEAP_OPS_UPDATE;
	para.g_xid = GetGlobalTxid();
	bool bRet1 = twophase_prepare_task(twophase_heap_update_prepare,para,false);
	bool bRet2 = twophase_commit_task(twophase_heap_update_commit_index,para,false);
	return bRet1&&bRet2;
}

/************************************************************************/
/* create index rollback                                                */
/************************************************************************/
bool twophase_create_index_prepare(TwophaseParas& para,Transaction* pTrans)
{
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);
		para.indexColId = twophase_setColIndex();
		para.indexid = StorageEngine::getStorageEngine()->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,para.indexColId);
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		return false;
	}

	return true;
}
bool twophase_create_index_commit(TwophaseParas& para,Transaction* trans)
{
	bool bRet = true;

	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		twophase_setColIndex();
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,para.iso_level);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,para.heapId);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,para.indexid);
		if (para.bCommit && !pIndex)
			bRet = false;

		if (!para.bCommit && pIndex)
			bRet = false;

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

bool test_twophase_create_index_prepare()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	para.flag = HEAP_OPS_CREATE_INDEX;
	para.g_xid = GetGlobalTxid();
	return twophase_prepare_task(twophase_create_index_prepare,para,false);
}
bool test_twophase_create_index_commit()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	return twophase_commit_task(twophase_create_index_commit,para,false);
}
bool test_twophase_create_index_rollback()
{
	if (!need_run_twophase())
		return true;

	TwophaseParas para;
	return twophase_commit_task(twophase_create_index_commit,para,false,false);
}
bool test_twophase_create_index()
{
	TwophaseParas para;
	para.flag = HEAP_OPS_CREATE_INDEX;
	para.g_xid = GetGlobalTxid();
	bool bPre = twophase_prepare_task(twophase_create_index_prepare,para,false);
	bool bCommit = twophase_commit_task(twophase_create_index_commit,para,false);

	return bPre&&bCommit;
}
bool test_twophase_create_index_rollback_ex()
{
	TwophaseParas para;
	para.flag = HEAP_OPS_CREATE_INDEX;
	para.g_xid = GetGlobalTxid();
	bool bPre = twophase_prepare_task(twophase_create_index_prepare,para,false);
	bool bCommit = twophase_commit_task(twophase_create_index_commit,para,false,false);

	return bPre&&bCommit;
}


//////////////////////////////////////////////////////////////////////////
//multi thread tests

TwophaseParas g_twophase_para;//used between multi thread 

bool g_Prepare = false;
bool g_Commit = false;

void EmptyGlobalPara()
{
	g_twophase_para.bCommit = true;
	g_twophase_para.bIsIndex = false;
	g_twophase_para.g_xid = InvalidTransactionID;
	g_twophase_para.scanKey = "";
	g_twophase_para.setData.clear();
	g_twophase_para.strDelete = "";
	g_twophase_para.strInsert = "";
	g_twophase_para.strUpdateDes = "";
	g_twophase_para.strUpdateSrc = "";
}
typedef void (*thread_func)(TwophaseParas* , bool*);

bool twophase_multi_thread_heap_ops(thread_func func_prepare, thread_func func_commit)
{
	TwophaseParas para;
	para.g_xid = GetGlobalTxid();

	boost::thread_group g;

	g_Prepare = false;
	g_Commit = false;

	bool bPre = false;
	g.create_thread(boost::bind(func_prepare,&para,&bPre));
	
	while (!g_Prepare){	}

	bool bCommit = false;
	g.create_thread(boost::bind(func_commit,&para,&bCommit));

	g.join_all();

	return bPre&&bCommit;
}
/************************************************************************/
/* heap insert tests                                                    */
/************************************************************************/
void twophase_heap_insert_thread_prepare(TwophaseParas* para,bool* bRet)
{
	//beginThread_mine();
	StorageEngine::getStorageEngine()->beginThread();
	para->flag = HEAP_OPS_INSERT;
	*bRet = twophase_prepare_task(twophase_heap_insert_prepare,*para,false);
	g_Prepare = true;
	StorageEngine::getStorageEngine()->detachThread();
	while (!g_Commit) { }
}
void twophase_heap_insert_thread_commit(TwophaseParas* para,bool* bRet)
{
	StorageEngine::getStorageEngine()->beginThread();
	bool bRet1 = twophase_commit_task(twophase_heap_insert_commit,*para,false);
	*bRet = bRet1;
	g_Commit = true;
	StorageEngine::getStorageEngine()->detachThread();
}
bool test_twophase_heap_insert_thread()
{
	return twophase_multi_thread_heap_ops(
		twophase_heap_insert_thread_prepare,
		twophase_heap_insert_thread_commit
		);
}
//index
void twophase_heap_insert_index_thread_prepare(TwophaseParas* para,bool* bRet)
{
	StorageEngine::getStorageEngine()->beginThread();
	para->flag = HEAP_OPS_INSERT;
	para->bIsIndex = true;
	para->g_xid = GetGlobalTxid();
	*bRet = twophase_prepare_task(twophase_heap_insert_prepare,*para,false);
	StorageEngine::getStorageEngine()->detachThread();
	g_Prepare = true;
	while (!g_Commit) { }
}
void twophase_heap_insert_index_thread_commit(TwophaseParas* para,bool* bRet)
{
	StorageEngine::getStorageEngine()->beginThread();
	bool bRet1 = twophase_commit_task(twophase_heap_insert_commit_index,*para,false);
	*bRet = bRet1;
	g_Commit = true;
	StorageEngine::getStorageEngine()->detachThread();
}
bool test_twophase_heap_insert_index_thread()
{
	return twophase_multi_thread_heap_ops(
		twophase_heap_insert_index_thread_prepare,
		twophase_heap_insert_index_thread_commit
		);
}
/************************************************************************/
/*heap delete                                                           */
/************************************************************************/
void twophase_heap_delete_thread_prepare(TwophaseParas* para,bool* bRet)
{
	//beginThread_mine();
	StorageEngine::getStorageEngine()->beginThread();
	para->flag = HEAP_OPS_DELETE;
	*bRet = twophase_prepare_task(twophase_heap_delete_prepare,*para,false);
	StorageEngine::getStorageEngine()->detachThread();
	g_Prepare = true;
	while (!g_Commit) { }
}
void twophase_heap_delete_thread_commit(TwophaseParas* para,bool* bRet)
{
	//beginThread_mine();
	StorageEngine::getStorageEngine()->beginThread();
	bool bRet1 = twophase_commit_task(twophase_heap_delete_commit,*para,false);
	*bRet = bRet1;
	g_Commit = true;
	StorageEngine::getStorageEngine()->detachThread();
}
bool test_twophase_heap_delete_thread()
{
	return twophase_multi_thread_heap_ops(
		twophase_heap_delete_thread_prepare,
		twophase_heap_delete_thread_commit
		);
}
//index
void twophase_heap_delete_index_thread_prepare(TwophaseParas* para,bool* bRet)
{
	StorageEngine::getStorageEngine()->beginThread();
	para->flag = HEAP_OPS_DELETE;
	para->bIsIndex = true;
	para->g_xid = GetGlobalTxid();
	*bRet = twophase_prepare_task(twophase_heap_delete_prepare,*para,false);
	StorageEngine::getStorageEngine()->detachThread();
    g_Prepare = true;
	while (!g_Commit) { }
}
void twophase_heap_delete_index_thread_commit(TwophaseParas* para,bool* bRet)
{
	StorageEngine::getStorageEngine()->beginThread();
	bool bRet1 = twophase_commit_task(twophase_heap_delete_commit_index,*para,false);
	*bRet = bRet1;
	g_Commit = true;
	StorageEngine::getStorageEngine()->detachThread();
}
bool test_twophase_heap_delete_index_thread()
{
	return twophase_multi_thread_heap_ops(
		twophase_heap_delete_index_thread_prepare,
		twophase_heap_delete_index_thread_commit
		);
}

/************************************************************************/
/*heap update                                                           */
/************************************************************************/
void twophase_heap_update_thread_prepare(TwophaseParas* para,bool* bRet)
{
	StorageEngine::getStorageEngine()->beginThread();
	para->flag = HEAP_OPS_UPDATE;
	*bRet = twophase_prepare_task(twophase_heap_update_prepare,*para,false);
	StorageEngine::getStorageEngine()->detachThread();
	g_Prepare = true;
	while (!g_Commit) { }
}
void twophase_heap_update_thread_commit(TwophaseParas* para,bool* bRet)
{
	StorageEngine::getStorageEngine()->beginThread();
	bool bRet1 = twophase_commit_task(twophase_heap_update_commit,*para,false);
	*bRet = bRet1;
	g_Commit = true;
	StorageEngine::getStorageEngine()->detachThread();
}
bool test_twophase_heap_update_thread()
{
	return twophase_multi_thread_heap_ops(
		twophase_heap_update_thread_prepare,
		twophase_heap_update_thread_commit
		);
}
//index
void twophase_heap_update_index_thread_prepare(TwophaseParas* para,bool* bRet)
{
	StorageEngine::getStorageEngine()->beginThread();
	para->flag = HEAP_OPS_UPDATE;
	para->bIsIndex = true;
	para->g_xid = GetGlobalTxid();
	*bRet = twophase_prepare_task(twophase_heap_update_prepare,*para,false);
	g_Prepare = true;
	while (!g_Commit) { }
	StorageEngine::getStorageEngine()->detachThread();
}
void twophase_heap_update_index_thread_commit(TwophaseParas* para,bool* bRet)
{
	StorageEngine::getStorageEngine()->beginThread();
	bool bRet1 = twophase_commit_task(twophase_heap_update_commit_index,*para,false);
	*bRet = bRet1;
	g_Commit = true;
	StorageEngine::getStorageEngine()->detachThread();
}
bool test_twophase_heap_update_index_thread()
{
	return twophase_multi_thread_heap_ops(
		twophase_heap_update_index_thread_prepare,
		twophase_heap_update_index_thread_commit
		);
}