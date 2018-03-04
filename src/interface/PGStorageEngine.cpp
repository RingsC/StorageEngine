#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <time.h>
#include "postgres.h"
#include "utils/rel.h"
#include <pthread.h>
#include <sys/stat.h>

#include "interface/PGStorageEngine.h"
#include "access/transam.h"
#include "postmaster/xdb_main.h"
#include "interface/ErrNo.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/PGTransaction.h"
#include "interface/PGTempEntrySet.h"
#include "interface/PGSortEntrySet.h"
#include "utils/relcache.h"
#include "utils/GetTime.h"
#include "catalog/xdb_catalog.h"
#include "access/xact.h"
#include "access/twophase.h"
#include "miscadmin.h"
#include "postmaster/xdb_main.h"


#include "interface/FDPGAdapter.h"
#include "interface/PGIndexEntrySet.h"
#include "interface/utils.h"
#include "catalog/metaData.h"
#include "catalog/pg_database.h"
#include "commands/tablespace.h"
#include "nodes/parsenodes.h"
#include "Transaction.h"
#include "PGSETypes.h"
#include "Macros.h"
#include "postmaster/postmaster.h"
#include <iostream>



using std::map;
using std::string;
using std::exception;

#if !defined(_MSC_VER)
#pragma GCC diagnostic ignored "-Wclobbered"
#endif

namespace FounderXDB{
namespace StorageEngineNS {


/// static initialization
PGStorageEngine* PGStorageEngine::m_single_pgstorage_engine = NULL;
//pthread_mutex_t PGStorageEngine::m_single_pgstorage_engine_mutex = NULL;
//pthread_mutex_t m_single_pgstorage_engine_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_once_t PGStorageEngine::once_mutex = PTHREAD_ONCE_INIT;
//slock_t transactions_map_mutex;
slock_t setColinfo_mutex;



void DBMetaInfoCopy(std::vector<DBMetaInfo>&pIds, Form_meta_database pInfos,size_t nSize)
{
	for (size_t idx = 0; idx < nSize; ++idx)
	{
	    DBMetaInfo dbmi;
		Form_meta_database pInfo = Form_meta_database((char*)pInfos + idx * MAX_FORM_DBMETA_SIZE);
		dbmi.dbid = pInfo->db_id;
		dbmi.tbid = pInfo->defTablespace;

        std::string strName(pInfo->db_name.data,pInfo->db_name.data + strlen(pInfo->db_name.data));
        dbmi.dbname = strName;
        dbmi.metaDataLen = VARSIZE_ANY(&pInfo->extraData);
		dbmi.setMetaData(MemoryContext::getMemoryContext(MemoryContext::Top)->alloc(dbmi.metaDataLen), dbmi.metaDataLen);
		std::memcpy(dbmi.getMetaData(),VARDATA_ANY(&pInfo->extraData),dbmi.metaDataLen);
		pIds.push_back(dbmi);
	}
}

THREAD_LOCAL Transaction* m_current_transaction;//current executing transaction in current thread
THREAD_LOCAL bool bThreadInit = false; // true if the current thread has initialized, or false

PGStorageEngine::PGStorageEngine()
{
    m_single_pgstorage_engine = NULL;
    m_current_max_oid = 0;
    m_global_transation_id = 3;
	m_min_trans_id = m_global_transation_id;
	m_max_trans_id = m_global_transation_id;
	
	FDPG_Lock::fd_spinlock_init(transactions_id_mutex);
	FDPG_Lock::fd_spinlock_init(current_max_oid_mutex);


    //pthread_mutex_init(&transactions_id_mutex, NULL);
    //pthread_mutex_init(&current_max_oid_mutex, NULL);

}

PGStorageEngine::~PGStorageEngine()
{  
	FounderXDB::StorageEngineNS::ScopedXdbLock ssl(transactions_map_mutex, FounderXDB::StorageEngineNS::XdbLock::Exclusive);
    try {
        map<FXTransactionId, PGTransaction*>::iterator iter;
        for (iter = m_global_transactions.begin(); iter != m_global_transactions.end();) {
            if (iter->second) {
                iter->second->abort();
                delete iter->second;
                iter->second = NULL;
            }
            m_global_transactions.erase(iter++);
        }
    } catch (exception &) {       
        throw;
    }   
}

FounderXDB::StorageEngineNS::TableSpaceID PGStorageEngine::createTableSpace( const char *tblspcName, const char *path, bool isTempOk)
{
	TransactionId xid = 0;
	Transaction *pTrans = getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	TableSpaceID retSpace = 0;

	try
	{
		CreateTableSpaceStmt st = {T_Invalid ,(char*)tblspcName,NULL,(char*)path};
		THROW_CALL(retSpace = CreateTableSpace,&st,isTempOk);
		FDPG_Transaction::fd_CommandCounterIncrement();

		pTrans->commit();
	}
	catch(StorageEngineException& )
	{
        pTrans->abort();
		throw;
	}
	
	xid = InvalidTransactionID;

	pTrans = getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	try
	{
		FDPG_LargeObject::fd_tablespace_create_lorel(retSpace);

		pTrans->commit();
	}
	catch(StorageEngineException& se)
	{
		ereport(LOG, (errmsg(se.getErrorMsg())));
		pTrans->abort();
	}
	
	return retSpace;
}

void PGStorageEngine::cluster(EntrySetID entryid, EntrySetID indexid, bool use_sort)
{
	Assert(entryid > InvalidVariableId);

	if (FDPG_Transaction::fd_IsTransactionBlock())
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Cluster cannot run inside a transaction block.\n");

	TransactionId xid = 0;
	Transaction *pTrans = getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	try
	{
		FDPG_Heap::fd_ClusterRel(entryid, indexid, use_sort);

		pTrans->commit();
	}
	catch(StorageEngineException& )
	{
		pTrans->abort();
		throw;
	}
}

void PGStorageEngine::rebuildIndex(EntrySetID entryid, int flags)
{
	THROW_CALL(FDPG_Index::fd_reindex_relation,entryid,InvalidDatabaseID,flags);
}

/// Get a tablespace's ID by name.
TableSpaceID PGStorageEngine::getTableSpaceID(Transaction *txn, const char *tblspcName) const
{
    TableSpaceID tablespace = 0;
	THROW_CALL(tablespace = get_tablespace_oid,tblspcName,false);
	return tablespace;
}

void PGStorageEngine::createTablespaceDirs(const char *location,TableSpaceID tablespaceoid)
{
	if(NULL == TopMemoryContext)
	{
		THROW_CALL(MemoryContextInit,false);
	}
	THROW_CALL(create_tablespace_directories,const_cast<char *>(location),tablespaceoid);
}

void PGStorageEngine::destroyTablespaceDirs(TableSpaceID tablespaceoid)
{
	if(NULL == TopMemoryContext)
	{
		THROW_CALL(MemoryContextInit,false);
	}
	THROW_CALL(destroy_tablespace_directories, tablespaceoid, false);
}

int PGStorageEngine::dropTableSpace( const char *tblspcName )
{
	TransactionId xid = 0;
	DropTableSpaceStmt st = {T_Invalid,(char*)tblspcName,false};

	Transaction *pTrans = getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	try
	{
		FDPG_LargeObject::fd_tablespace_drop_lorel(tblspcName, st.missing_ok);
	} catch (StorageEngineException& )
	{
		pTrans->abort();
		throw;
	}

	pTrans->commit();

	xid = InvalidTransactionID;
	pTrans = getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	try
	{
		THROW_CALL(DropTableSpace,&st);
		FDPG_Transaction::fd_CommandCounterIncrement();
	}
	catch(StorageEngineException& )
	{
		pTrans->abort();
		throw;
	}
	pTrans->commit();
	return 0;
}

DatabaseID PGStorageEngine::createDatabase(Transaction *txn
										   , const char *dbname
										   , const char *defaultTblspcName
										   , const void*extraData
										   , size_t extraDataLen)
{
	DatabaseID dbId = 0;
	THROW_CALL(dbId = CreateDatabase,txn->getTransactionId(),(char*)dbname,(char*)defaultTblspcName,extraData,extraDataLen);
	FDPG_Transaction::fd_CommandCounterIncrement();
	return dbId;
}

int PGStorageEngine::dropDatabase(Transaction *txn, const char *dbname)
{
    THROW_CALL(DropDatabase,dbname);
    FDPG_Transaction::fd_CommandCounterIncrement();
	return 0;
}

void PGStorageEngine::listTableSpace(std::vector<TablespaceMetaData> &v_tblspc)
{
	Assert(v_tblspc.size() == 0);

	bool flag = false;

	PG_TRY(); 
	{
		List *l = ListTableSpace();

		ListCell *lc = NULL;

		FormTablespaceData formClass = NULL;

		foreach(lc, l)
		{
			formClass = (FormTablespaceData)lfirst(lc);

			size_t len = VARSIZE_ANY(&formClass->spclocation);
			char *data = VARDATA(&formClass->spclocation);
			v_tblspc.push_back(TablespaceMetaData(formClass->id
				,formClass->spcname.data
				,std::string(data, len)));
		}

		list_free_deep(l);
		//int num = 3;
	} PG_CATCH();
	{
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void PGStorageEngine::listTableSpace(std::vector<std::pair<std::string, std::string> > &v_tblspc)
{
	Assert(v_tblspc.size() == 0);

	FDPG_StorageEngine::fd_list_all_tablespc(v_tblspc);
}

/// Get database metadata (fill into dbmeta object) by db id, return true if dbmeta is filled with valid data, 
/// otherwise return false.
bool PGStorageEngine::getDatabaseInfoByID(DatabaseID dbId, DBMetaInfo&dbmeta) const
{
	Form_meta_database dbInfo = NULL; 
    THROW_CALL(dbInfo = GetDatabaseMetaDataByID,dbId);
	if (NULL == dbInfo)
	{
		return false;
	}
	std::vector<DBMetaInfo>dbmetas;
    DBMetaInfoCopy(dbmetas,dbInfo,1);
    dbmeta = dbmetas[0];
    FDPG_Memory::fd_pfree(dbInfo);
	return true;
}

/// Get database metadata (fill into dbmeta object) by name, return true if dbmeta is filled with valid data, 
/// otherwise return false.
bool PGStorageEngine::getDatabaseInfo(const char *dbname, DBMetaInfo&dbmeta) const
{
	Form_meta_database dbInfo = NULL; 
    THROW_CALL(dbInfo = GetDatabaseMetaData,dbname);
	if (NULL == dbInfo)
	{
		return false;
	}
	std::vector<DBMetaInfo>dbmetas;
    DBMetaInfoCopy(dbmetas,dbInfo,1);
    dbmeta = dbmetas[0];
    FDPG_Memory::fd_pfree(dbInfo);
	return true;
}
/// Update extra data. db id and name is permanent.
void PGStorageEngine::updateDatabaseExtraData(const char *dbname, const void *data, size_t len)
{
    THROW_CALL(UpdateExtraData,dbname,data,len);
}

DatabaseID PGStorageEngine::setCurrentDatabase(const char *dbname,Oid dbId,Oid tablespaceId)
{
    DatabaseID dbid = 0;
    THROW_CALL(dbid = SetCurrentDatabase,dbname,dbId,tablespaceId);
	return dbid;
}

DatabaseID PGStorageEngine::getCurrentDatabase(std::string& dbname) const
{
	DatabaseID dbid = 0;
	const char* szName;
    THROW_CALL(dbid = GetCurrentDatabase,szName);
	dbname = string(szName);
	return dbid;
}

/// Get database ID by name.
DatabaseID PGStorageEngine::getDatabaseID(const char *dbname) const
{
	DatabaseID dbid = 0;
    THROW_CALL(dbid = GetDatabaseOid,dbname);
	return dbid;
}
TableSpaceID PGStorageEngine::getCurrentDatabaseTablespace() const
{
	TableSpaceID id = 0;
    THROW_CALL(id = GetCurrentDatabaseTablespace);
	return id;
}

void PGStorageEngine::getAllDatabaseInfos(std::vector<DBMetaInfo>&dbs) const
{
	Form_meta_database pInfos = NULL;
	size_t nSize;
	THROW_CALL(pInfos = ::GetAllDatabaseInfos,nSize);
	//MemoryContext *cxt = MemoryContext::getMemoryContext(MemoryContext::Top);
	//pIds = new(*cxt)DBMetaInfo[nSize];
	DBMetaInfoCopy(dbs,pInfos, nSize);

	FDPG_Memory::fd_pfree(pInfos);
}

void PGStorageEngine::getPGStorageEngine()
{
    m_single_pgstorage_engine = new PGStorageEngine;
}


PGStorageEngine* PGStorageEngine::getInstance()
{
    pthread_once(&once_mutex,getPGStorageEngine);

    return m_single_pgstorage_engine;

}

void PGStorageEngine::releaseStorageEngine(PGStorageEngine* instance)
{
    if(instance != NULL) {
        delete instance;
        instance = NULL;
    }
}

PGTransaction *PGStorageEngine::createTransaction(Transaction::IsolationLevel   level,
                                                  Transaction* pParent,
                                                  FXGlobalTransactionId gxid)
{
	Assert(!(pParent != NULL && gxid > InvalidTransactionID));

    if(NULL == pParent)
    {
	    FDPG_Transaction::fd_set_current_transaction_state(GetDefaultXact());
    }
	PGTransaction *ptransaction = NULL;
	if (Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL == level)
	{
		level = Transaction::READ_COMMITTED_ISOLATION_LEVEL;
	}

	bool isPrepared = false;
	if (gxid > InvalidTransactionID)
		isPrepared = isGlobalTransactionPrepared(gxid);

	FDPG_Transaction::fd_setXactIsoLevel(level);
	{
		FounderXDB::StorageEngineNS::ScopedXdbLock ssl(transactions_map_mutex, FounderXDB::StorageEngineNS::XdbLock::Exclusive);
		try 
		{
			if(NULL != pParent)
			{
				MemoryContext *cxt = pParent->getAssociatedMemoryContext();
				ptransaction = new(*cxt) PGSubTransaction(++m_global_transation_id,level);
				FDPG_Transaction::fd_setXactIsoLevel(level);
			}
			else
			{
				MemoryContext *cxt = MemoryContext::getMemoryContext(MemoryContext::Top);
				if (isPrepared)
					ptransaction = new(*cxt) PGTransaction(++m_global_transation_id, level, NULL);
				else
					ptransaction = new(*cxt) PGTransaction(++m_global_transation_id, level);
				FDPG_Transaction::fd_setXactIsoLevel(level);
			}
		} 
		catch (std::bad_alloc &) 
		{
			m_global_transation_id--;
			throw;
		}

		ptransaction->setGXID(gxid);

		m_global_transactions.insert(std::pair<FXTransactionId, PGTransaction*>(m_global_transation_id, ptransaction) );
		m_max_trans_id = m_global_transation_id;
	}
	ptransaction->set_current_transaction_state(FDPG_Transaction::fd_get_current_transaction_state());
	ptransaction->set_top_transaction_state(FDPG_Transaction::fd_get_top_transaction_state());
	return ptransaction;
}

void PGStorageEngine::deleteTransaction(const FXTransactionId&  xid)
{
	try
	{
		if (InvalidTransactionId == xid)
			return;
		FounderXDB::StorageEngineNS::ScopedXdbLock ssl(transactions_map_mutex, FounderXDB::StorageEngineNS::XdbLock::Exclusive);
		{
			std::map<FXTransactionId, PGTransaction*>::iterator iter, end;
			end = m_global_transactions.end();
			iter = m_global_transactions.find(xid);
			if (iter != end)
			{
				if (NULL != iter->second)
					delete iter->second;
				m_global_transactions.erase(iter);
			}
		}
	}
	catch(exception &)
	{
		throw;
	}

	SetCurrentTransaction(NULL);
}
Transaction* PGStorageEngine::getTransaction(FXTransactionId&   xid,
																						 Transaction::IsolationLevel   level,
																						 FXGlobalTransactionId gxid)
{
	PGTransaction *ptransaction = NULL;

	if (xid == InvalidTransactionId) 
	{
		ptransaction = createTransaction(level, NULL, gxid);
		xid = ptransaction->getTransactionId();
	}
	else
	{
		FounderXDB::StorageEngineNS::ScopedXdbLock ssl(transactions_map_mutex, FounderXDB::StorageEngineNS::XdbLock::Shared);
		map<FXTransactionId, PGTransaction*>::iterator iter;
		iter = m_global_transactions.find(xid);
		if (iter == m_global_transactions.end())
		{
			throw StorageEngineExceptionUniversal(FIND_TRANSACTION_ERR, "cann't find transaction");
		}
		ptransaction = iter->second;
		FDPG_Transaction::fd_set_current_transaction_state(ptransaction->get_current_transaction_state());
		//FDPG_Transaction::fd_set_top_transaction_state(ptransaction->get_top_transaction_state());
	}

	SetCurrentTransaction(ptransaction);
	
	return ptransaction;
}
void PGStorageEngine::SetCurrentTransaction(Transaction* pTransaction)
{
	m_current_transaction = pTransaction;
}
Transaction* PGStorageEngine::GetCurrentTransaction()
{
	return m_current_transaction;
}

void PGStorageEngine::CurrentTransactionMustExist()
{
	if(m_current_transaction == NULL)
	{
		throw FounderXDB::StorageEngineNS::NotInTransactionException();
	}
}

bool PGStorageEngine::isGlobalTransactionPrepared(FXGlobalTransactionId gxid)
{
	Assert(gxid > InvalidTransactionID);

	char gcxid[254];
	memset(gcxid, 0, 254);
	sprintf(gcxid, "%u", gxid);
	return FDPG_Transaction::fd_GlobalTransactionIdIsPrepared(gcxid);
}

void PGStorageEngine::commitAllPreparedTransaction()
{
	std::string sDbid;
	DatabaseID old_dbid = getCurrentDatabase(sDbid);

	FXTransactionId txnid = InvalidTransactionID;
	Transaction* txn = NULL;
	int count = 0;

	try
	{
		GlobalPrepareTxnInfo* prepareList = NULL;
		count = GetPreparedGlobalTxnId(&prepareList);

		if (count > 0)
		{
			for (int i = 0; i < count; i++)
			{
				txnid = InvalidTransactionID;
				txn = getTransaction(txnid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
				setCurrentDatabase(NULL,prepareList[i].dbid);
				txn->commit();

				commitPreparedTransaction(prepareList[i].gxid);
			}

			txnid = InvalidTransactionID;
			txn = getTransaction(txnid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			setCurrentDatabase(sDbid.c_str(),old_dbid);
			txn->commit();
		}
	}
	catch(...)
	{
		if (count > 0)
		{
			txnid = InvalidTransactionID;
			txn = getTransaction(txnid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			setCurrentDatabase(sDbid.c_str(),old_dbid);
			txn->commit();
		}

		throw;
	}
}

void PGStorageEngine::commitPreparedTransaction(FXGlobalTransactionId gxid, Transaction::IsolationLevel level)
{
	FXTransactionId txid = InvalidTransactionID;
	Transaction* pTransaction = getTransaction(txid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
	pTransaction->commit();
}

void PGStorageEngine::rollbackPreparedTransaction(FXGlobalTransactionId gxid, Transaction::IsolationLevel level)
{
	FXTransactionId txid = InvalidTransactionID;
	Transaction* pTransaction = getTransaction(txid,Transaction::READ_COMMITTED_ISOLATION_LEVEL,gxid);
	pTransaction->abort();
}

FXGlobalTransactionId PGStorageEngine::GetNextMaxPreparedGlobalTxnId()
{
	return FDPG_Transaction::fd_GetNextMaxPreparedGlobalTxnId();
}

FXTransactionId PGStorageEngine::getMinTransactionId()
{
	map<FXTransactionId, PGTransaction*>::iterator iter;
	FounderXDB::StorageEngineNS::ScopedXdbLock ssl(transactions_map_mutex, FounderXDB::StorageEngineNS::XdbLock::Exclusive);
	if (m_global_transactions.empty())
		return 0;
	iter = m_global_transactions.find(m_min_trans_id);
	if (iter == m_global_transactions.end())
	{
		iter = m_global_transactions.begin();
		m_min_trans_id = iter->first;
		for (; iter!=m_global_transactions.end(); ++iter)
		{
			if (m_min_trans_id > iter->first)
				m_min_trans_id = iter->first;
		}
	}
	return m_min_trans_id;
}

FXTransactionId PGStorageEngine::getMaxTransactionId()
{
	map<FXTransactionId, PGTransaction*>::iterator iter;
	FounderXDB::StorageEngineNS::ScopedXdbLock ssl(transactions_map_mutex, FounderXDB::StorageEngineNS::XdbLock::Exclusive);
	if (m_global_transactions.empty())
		return 0;
	iter = m_global_transactions.find(m_max_trans_id);
	if (iter == m_global_transactions.end())
	{
		iter = m_global_transactions.begin();
		m_max_trans_id = iter->first;
		for (; iter!=m_global_transactions.end(); ++iter)
		{
			if (m_max_trans_id < iter->first)
				m_max_trans_id = iter->first;
		}
	}
	return m_max_trans_id;
}

XdbLock* PGStorageEngine::GetSequenceLock(EntrySetID seqTableId, SeqID seqId)
{
	std::pair<EntrySetID, SeqID> seqUniqueId = std::make_pair(seqTableId, seqId);
	std::map<std::pair<EntrySetID, SeqID>, XdbLock*>::iterator it;
	XdbLock *lock = NULL;

	FDPG_Lock::fd_LWLockAcquire(SeqLocksMapLock, LW_SHARED);
	it = m_seqlocks_map.find(seqUniqueId);
	if(it != m_seqlocks_map.end()){
		lock = it->second;
	}
	FDPG_Lock::fd_LWLockRelease(SeqLocksMapLock);
	if(lock != NULL){
		return lock;
	}

	FDPG_Lock::fd_LWLockAcquire(SeqLocksMapLock, LW_EXCLUSIVE);
	it = m_seqlocks_map.find(seqUniqueId);
	if(it == m_seqlocks_map.end()){
		/* new a lock, and insert to map */
		XdbLock* newlock = XdbLock::createLock(XdbLock::SpinLock,
					MemoryContext::getMemoryContext(MemoryContext::ProcessTop));
		m_seqlocks_map.insert(std::make_pair(seqUniqueId, newlock));
		lock = newlock;
	}else{
		lock = it->second;
	}
	FDPG_Lock::fd_LWLockRelease(SeqLocksMapLock);

	return lock;
}

void PGStorageEngine::RemoveSequenceLock(EntrySetID seqTableId, SeqID seqId)
{
	std::pair<EntrySetID, SeqID> seqUniqueId = std::make_pair(seqTableId, seqId);
	std::map<std::pair<EntrySetID, SeqID>, XdbLock*>::iterator it;

	FDPG_Lock::fd_LWLockAcquire(SeqLocksMapLock, LW_EXCLUSIVE);
	it = m_seqlocks_map.find(seqUniqueId);
	if(it != m_seqlocks_map.end()){
		delete it->second;
		it->second = NULL;
		m_seqlocks_map.erase(it);
	}
	FDPG_Lock::fd_LWLockRelease(SeqLocksMapLock);

	return;
}

FXTransactionId PGStorageEngine::getNextTransactionId()
{
	FounderXDB::StorageEngineNS::ScopedXdbLock ssl(transactions_map_mutex, FounderXDB::StorageEngineNS::XdbLock::Exclusive);
	return m_global_transation_id + 1;
}

void PGStorageEngine::removeIndexEntrySet(Transaction*       txn, 
                            EntrySetID parentID,
                            EntrySetID      name,
							DatabaseID dbid)
{
	CurrentTransactionMustExist();
    if (parentID == MetaTableId     		||
        parentID == MetaTableColFirstIndex  ||
        parentID == MetaTableColThirdIndex  ||
        name == MetaTableId         		||
        name == MetaTableColFirstIndex		||
        name == MetaTableColThirdIndex) {
        throw StorageEngineExceptionUniversal(MEAT_TABLE_DELETE_ERR, "deletion of meta table and it's index is forbidden");
    }
    
    (static_cast<PGTransaction*>(txn))->closeEntrySet(name, true);
    
	if (0 == dbid) {
		dbid = MyDatabaseId;
	}

	if(name > (EntrySetID)0 )
		FDPG_Index::fd_index_drop(parentID, 0, name, dbid);
}

void PGStorageEngine::removeEntrySet(Transaction*       txn,
                             EntrySetID      name,
							 DatabaseID dbid) 
{
	CurrentTransactionMustExist();
    if (name == MetaTableId  || name == MetaTableColFirstIndex || name == MetaTableColThirdIndex) {
        throw StorageEngineExceptionUniversal(MEAT_TABLE_DELETE_ERR, "deletion of meta table and it's index is forbidden");
    }
    PGTransaction* txn_ = static_cast<PGTransaction*>(txn);
    EntrySet* es = static_cast<EntrySet*>(txn_->getEntrySet(name, dbid));
    if(es)
    {
        PGIndinfoData indinfo = {0};
        es->getIndexInfo(txn_, indinfo);
        for(unsigned int i = 0; i < indinfo.index_num; i++)
        {
            txn_->closeEntrySet(indinfo.index_array[i],true);
        }
    }
    
    txn_->closeEntrySet(name, true);
    
	if (0 == dbid) {
		dbid = MyDatabaseId;
	}

	if(name > (EntrySetID)0 )
		FDPG_Heap::fd_heap_drop(name, dbid);

	return ;
}

void PGStorageEngine::initialize(const char *database_home, 
																 const uint32 thread_num, 
																 storage_params * params)
{
	
	FDPG_Lock::fd_spinlock_init(setColinfo_mutex);
    FDPG_StorageEngine::fd_start_engine(database_home, thread_num, false, params);
	//commitAllPreparedTransaction();
	transactions_map_mutex = XdbLock::createLock(XdbLock::Mutex,
					MemoryContext::getMemoryContext(MemoryContext::ProcessTop));
}

void PGStorageEngine::interruptInit(int code)
{
	FDPG_StorageEngine::fd_interrupt_start_engine(code);
}

void PGStorageEngine::initMemoryContext()
{
	MemoryContextInit(false);
}

bool PGStorageEngine::amMaster() const
{
	return FDPG_StorageEngine::fd_am_master();
}

void PGStorageEngine::regTriggerFunc(StandbyTriggerFunc func)
{
	FDPG_StorageEngine::fd_reg_trigger_func(func);
}

void PGStorageEngine::bootstrap(const char *datadir)
{
    FDPG_StorageEngine::fd_start_engine(datadir, 80, true, NULL);
	transactions_map_mutex = XdbLock::createLock(XdbLock::Mutex,
					MemoryContext::getMemoryContext(MemoryContext::ProcessTop));
}

void PGStorageEngine::shutdown(void)
{
    FDPG_StorageEngine::fd_end_engine();
}

bool PGStorageEngine::startArchive(bool isOnline)
{
	return FDPG_StorageEngine::fd_start_archive();
}

bool PGStorageEngine::endArchive(bool isOnline)
{
	return FDPG_StorageEngine::fd_end_archive();
}

void PGStorageEngine::startBackup(const char* strLabel,bool fast)
{
	FDPG_StorageEngine::fd_start_backup(strLabel,fast);
}

void PGStorageEngine::endBackup( void )
{
	FDPG_StorageEngine::fd_stop_backup();
}

bool PGStorageEngine::startVacuum()
{
    return false;
}

bool PGStorageEngine::endVacuum()
{
    return false;
}

bool PGStorageEngine::prepareAsyncAbortTransaction(FXTransactionId &xid)
{
    return false;
}

EntrySetID PGStorageEngine::createEntrySet(Transaction*     txn,
                             uint32 colid, const char *tblspcName, 
														 DatabaseID dbid, bool isTempEntrySe)
{
	CurrentTransactionMustExist();
    //Oid tmp = m_current_max_oid;
    //EntrySet *pentry_set = NULL;
    Oid tmOid = InvalidOid;
	TableSpaceID spcid = 0;
	if (0 != tblspcName)
	{
		spcid = StorageEngine::getStorageEngine()->getTableSpaceID(txn,tblspcName);
	}

	try{
		if(!isTempEntrySe)
		{
			tmOid =	FDPG_Heap::fd_heap_create(spcid, InvalidOid, dbid,colid);
		} else {
			tmOid = FDPG_Heap::fd_temp_heap_create(spcid, dbid, colid);
		}

    } catch (StorageEngineExceptionUniversal &) {
			throw;
    } catch (std::exception &) {  
      throw;
    }   

    return tmOid;

}

EntrySet* PGStorageEngine::createTempEntrySet(Transaction *txn, 
																							EntrySet::StoreSortType sst,
																							uint32 spaceId,
																							const uint32 maxKBytes, 
																							const bool randomAccess, 
																							const bool interXact)
{
	CurrentTransactionMustExist();
	EntrySet *pentry_set = NULL;
	try {

		if (0 == spaceId) {
			spaceId = DEFAULTTABLESPACE_OID;
		}
		if(interXact) 
		{
			MemoryContext *cxt = MemoryContext::getMemoryContext(MemoryContext::Top);
			pentry_set = new(*cxt) PGTempEntrySet(sst,randomAccess, interXact, maxKBytes, spaceId);
		}
		else 
		{
			MemoryContext *cxt = txn->getAssociatedMemoryContext();
			pentry_set = new(*cxt) PGTempEntrySet(sst,randomAccess, interXact, maxKBytes, spaceId);
		}

	} catch (StorageEngineExceptionUniversal &) {
		throw;
	} catch (std::exception &) {  // new throw this exception 
		throw;
	}   

	return pentry_set;
}

SortEntrySet* PGStorageEngine::createSortEntrySet(Transaction *txn, 
																									EntrySet::StoreSortType sst,
																									int workMem,
																									bool randomAccess,
																									CompareCallbacki     pCompare)
{
	CurrentTransactionMustExist();
	PGSortEntrySet *pentry_set = NULL;
	try {

		MemoryContext *cxt = txn->getAssociatedMemoryContext();
		pentry_set = new(*cxt) PGSortEntrySet(sst);
		pentry_set->initialize(workMem, randomAccess, pCompare);

	} catch (StorageEngineExceptionUniversal &) {
		throw;
	} catch (std::exception &) {  // new throw this exception 
		throw;
	}   

	return pentry_set;
}

EntrySetID PGStorageEngine::createIndexEntrySet(Transaction*     txn,EntrySet *parentSet,
                             EntrySetType    type,
                             uint32 colid, const char *tblspcName, DatabaseID dbid,
                             void* userData,filter_func_t filterFunc, uint32 userdataLength, bool skipInsert) 
{
	CurrentTransactionMustExist();
    bool isNotCluster = (type != CLUSTER_UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE && type != CLUSTER_BTREE_INDEX_ENTRY_SET_TYPE);
    if (NULL == parentSet && isNotCluster) {
        throw StorageEngineExceptionUniversal(HEAP_OPEN_ERR, "open heap before related index created");
    }

    Oid relid = parentSet ? parentSet->getId() : InvalidEntrySetID;
    
    if (relid == MetaTableId || relid == MetaTableColFirstIndex || relid == MetaTableColThirdIndex) {
        throw StorageEngineExceptionUniversal(MEAT_TABLE_ERR, "can not create meta table");
    }

    
   // Relation relation = NULL;


    IndexType index_type = UNKNOWN_TYPE;

    if (type == UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE) {
        index_type = BTREE_UNIQUE_TYPE;
    }
	if (type == BTREE_INDEX_ENTRY_SET_TYPE) {
		index_type = BTREE_TYPE;
	}
    if (type == CLUSTER_BTREE_INDEX_ENTRY_SET_TYPE)
        index_type = BTREE_CLUSTER_TYPE;
    if (type == CLUSTER_UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE)
        index_type = BTREE_UNIQUE_CLUSTER_TYPE;
	
	if (type == HASH_INDEX_ENTRY_SET_TYPE)
		index_type = HASH_TYPE;
	if (type == LSM_INDEX_ENTRY_SET_TYPE) {
		index_type = LSM_TYPE;
	}
        
	Relation heapRelation = isNotCluster ? (static_cast<PGEntrySet *>(parentSet))->getRelation() : 0;
	
	if (0 == dbid) {
		dbid = MyDatabaseId;
	}
	bool flag = false;
	if (NULL == heapRelation && isNotCluster) {
		heapRelation = FDPG_Heap::fd_heap_open(relid, RowExclusiveLock, dbid);
		flag = true;
	}


    //Oid tmp = m_current_max_oid;
    Oid tmpOid = InvalidOid;
    //PGIndexEntrySet *pindex_entry = NULL;
   
	try {		
    	tmpOid = FDPG_Index::fd_index_create(heapRelation,index_type,InvalidOid,colid,userData,filterFunc,userdataLength, skipInsert);
	} catch (StorageEngineExceptionUniversal &) {
   
		if (flag) {
			FDPG_Heap::fd_heap_close(heapRelation, NoLock);
		}
		throw;
	} catch (std::exception &) {
 			if (flag) {
			FDPG_Heap::fd_heap_close(heapRelation, NoLock);
		}
		throw;
	}

	if (flag) {
		FDPG_Heap::fd_heap_close(heapRelation, NoLock);
	}

    return tmpOid;

}
IndexEntrySet* PGStorageEngine::openIndexEntrySet( Transaction*     txn,
        EntrySet *parentSet,
        EntrySet::EntrySetOpenFlags     opt,
        EntrySetID    entrySetID,
		DatabaseID dbid)
{
	Assert(txn != NULL);
	CurrentTransactionMustExist();
	 LOCKMODE lockmode = (opt == EntrySet::OPEN_SHARED) ? AccessShareLock : AccessExclusiveLock;
    // first find from txn's cache, if not found, do real open. a txn does real open only once.
    PGIndexEntrySet *pentry_set = (PGIndexEntrySet *)(((PGTransaction *)txn)->getEntrySet(entrySetID, dbid));
	if (pentry_set) {
		if(pentry_set->getIndexRelation()->lockMode == lockmode )
		{
		    return pentry_set;
		}
		else
			  (static_cast<PGTransaction*>(txn))->closeEntrySet(entrySetID, true, true);
	}
    Relation testRelation = NULL;
   
	if (0 == dbid) {
		dbid = MyDatabaseId;
	}
    
    testRelation = FDPG_Index::fd_index_open(entrySetID, lockmode, dbid);
    if(testRelation == NULL)
        return NULL;        

    EntrySetType type;
    IndexType rawtype = (IndexType)testRelation->mt_info.type;	
    switch(rawtype) {
    case BTREE_TYPE		:	type = BTREE_INDEX_ENTRY_SET_TYPE; break;
    case BTREE_UNIQUE_TYPE	:	type = UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE; break;
    case BTREE_CLUSTER_TYPE	:	type = CLUSTER_BTREE_INDEX_ENTRY_SET_TYPE; break;
    case BTREE_UNIQUE_CLUSTER_TYPE:	type = CLUSTER_UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE; break;
	case HASH_TYPE			:	type = HASH_INDEX_ENTRY_SET_TYPE; break;
	case LSM_TYPE			:	type = LSM_INDEX_ENTRY_SET_TYPE; break;
    default: Assert(false);type = BTREE_INDEX_ENTRY_SET_TYPE; break;
    }

    if (NULL == parentSet && type != CLUSTER_UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE && type != CLUSTER_BTREE_INDEX_ENTRY_SET_TYPE) {
        throw StorageEngineExceptionUniversal(HEAP_OPEN_ERR, "open heap before related index opened");
    }
    EntrySetID parentID = parentSet ? parentSet->getId() : InvalidEntrySetID;

    if (parentID == MetaTableId || 
        parentID == MetaTableColFirstIndex || 
        parentID == MetaTableColThirdIndex|| 
        entrySetID == MetaTableId || 
        entrySetID == MetaTableColFirstIndex || 
        entrySetID == MetaTableColThirdIndex) {
        throw StorageEngineExceptionUniversal(MEAT_TABLE_DELETE_ERR, "deletion of meta table and it's index is forbidden");
    }

	PGEntrySet *tmp = static_cast<PGEntrySet *>(parentSet);
	Relation heap = tmp ? tmp->getRelation() : 0;
	MemoryContext *cxt = txn->getAssociatedMemoryContext();
	pentry_set = new(*cxt) PGIndexEntrySet(parentID, entrySetID, heap, testRelation);
	(static_cast<PGTransaction*>(txn))->cacheEntrySet(entrySetID, dbid, pentry_set);

    return pentry_set;

}

EntrySet* PGStorageEngine::openEntrySet(Transaction*     txn,
        EntrySet::EntrySetOpenFlags     opt,
        EntrySetID    entrySetID,
		DatabaseID dbid) 
{
    Assert(txn != NULL);
	CurrentTransactionMustExist();
	 LOCKMODE lockmode = (opt == EntrySet::OPEN_SHARED) ? AccessShareLock : AccessExclusiveLock;
    if (entrySetID == MetaTableId || entrySetID == MetaTableColFirstIndex || entrySetID == MetaTableColThirdIndex) {
        throw StorageEngineExceptionUniversal(MEAT_TABLE_ERR, "can not open meta table");
    }
    // first find from txn's cache, if not found, do real open. a txn does real open only once.
    
    PGEntrySet *pentry_set = (PGEntrySet *)(((PGTransaction *)txn)->getEntrySet(entrySetID, dbid));
	if (pentry_set) 
	{
		if(pentry_set->getRelation()->lockMode == lockmode )
		{
			 return pentry_set;
		}
		else
			(static_cast<PGTransaction*>(txn))->closeEntrySet(entrySetID, true, true);
	}
   

    Relation testRelation = NULL;

	if (0 == dbid) {
		dbid = MyDatabaseId;
	}
	
	testRelation = FDPG_Heap::fd_heap_open(entrySetID,lockmode, dbid);
	MemoryContext *cxt = txn->getAssociatedMemoryContext();
	pentry_set = new(*cxt) PGEntrySet(entrySetID, testRelation);

    //put into txn cache
    (static_cast<PGTransaction*>(txn))->cacheEntrySet(entrySetID, dbid, pentry_set);
    return pentry_set;
}

void PGStorageEngine::closeEntrySet(Transaction*  txn, BaseEntrySet *set)
{
	CurrentTransactionMustExist();
    if ((set && set->getType() == TEMPORARY_ENTRY_SET_TYPE) || (set && set->getType() == SORT_ENTRY_SET_TYPE))
        delete set;
    // Otherwise do nothing. entry sets opened in txn will be closed when txn ends, and no entry set object should survive txns.
}

uint32 PGStorageEngine::getCurrentTimeLine( void )
{
	return FDPG_StorageEngine::fd_getCurrentTimeLine();
}

void PGStorageEngine::closeEntrySetInternal(Transaction*  txn, IndexEntrySet *set, bool unlock)
{
	CurrentTransactionMustExist();

    // relcnt decrement
    if (NULL == set) {
        return ;
    }
    Oid id = set->getId();

    if (id == MetaTableId || id == MetaTableColFirstIndex || id == MetaTableColThirdIndex) {
        throw StorageEngineExceptionUniversal(MEAT_TABLE_ERR, "can not close meta table");
    }

    PGIndexEntrySet *pentry_set = static_cast<PGIndexEntrySet*>(set);
	Relation index = pentry_set->getIndexRelation();	
	if (NULL == index) {
		return ;
	}
	FDPG_Index::fd_index_close(index, unlock? index->lockMode : NoLock);	

	return ;

}

void PGStorageEngine::closeEntrySetInternal(Transaction*  txn, EntrySet *set, bool unlock)
{
    Assert(txn != NULL);
	CurrentTransactionMustExist();

    // relcnt decrement
    if (NULL == set) {
        return ;
    }

	PGEntrySet *pentry_set = static_cast<PGEntrySet *>(set);
	Relation testRelation = pentry_set->getRelation();
	FDPG_Heap::fd_heap_close(testRelation, unlock? testRelation->lockMode : NoLock);	
    return ;
}

void PGStorageEngine::vacuumDatabase(void)
{
	FDPG_Database::fd_VacuumDatabase();
}

void PGStorageEngine::vacuumAll(void)
{
	FDPG_Database::fd_VacuumAll();
}

void PGStorageEngine::vacuumRelations(std::vector<EntrySetID>& eids)
{
	int nrels = (int)eids.size();
	EntrySetID *relids = &(eids[0]);
	FDPG_Heap::fd_VacuumRelations(nrels, relids);
}

void PGStorageEngine::localDeadlockDetect(int* aborted)
{
}

void PGStorageEngine::getWaitForGraph(WaitForGraph *wfg)
{
}

void PGStorageEngine::beginThread()
{
    BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    param->MyThreadType = backend;
    FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		free(param);

	bThreadInit = true;
}

void PGStorageEngine::endThread()
{
    FDPG_Heap::fd_proc_exit(0);
}

void PGStorageEngine::detachThread()
{
    FDPG_Transaction::fd_set_current_transaction_state(GetDefaultXact());
}

bool PGStorageEngine::isThreadInit()
{
	return bThreadInit;
}
void PGStorageEngine::beginStatement(Transaction *transaction)
{
	if(transaction)
	{
		((PGTransaction*)transaction)->getSnapshot(true);
		THROW_CALL(CommandEndInvalidationMessages, false);
		THROW_CALL(AcceptInvalidationMessages);
	}
}
void PGStorageEngine::endStatement(Transaction *transaction )
{   
	if(transaction)
	{
		PGTransaction *trans = static_cast<PGTransaction *>(transaction);
		trans->closeCachedEntrySets(true);
		//trans->closeCachedSequences();
		trans->endStatementLargeObject();
		/*trans->closeCacheLargeObject();*/
		FDPG_Transaction::fd_PreCommit_on_commit_actions();
	}
	FDPG_Transaction::fd_CommandCounterIncrement();
}

void PGStorageEngine::beginPlanStep()
{

}

void PGStorageEngine::endPlanStep()
{

}
#include "storage/ipc.h"
void PGStorageEngine::resetOnExit( void )
{
    on_exit_reset();
}

bool PGStorageEngine::XLogArchiveIsBusy(const char *xlog)
{  
   return FDPG_XLog::fd_XLogArchiveIsBusy(xlog);
}

bool PGStorageEngine::FileCanArchive(const char *filename)
{  
   return FDPG_XLog::fd_FileCanArchive(filename);
}

void PGStorageEngine::SetControlFileNodeId(uint32 nodeId)
{
	FDPG_XLog::fd_SetControlFileNodeId(nodeId);
}

uint32 PGStorageEngine::GetControlFileNodeId()
{
	return FDPG_XLog::fd_GetControlFileNodeId();
}

void PGStorageEngine::SetWalSenderFunc(WalSenderFunc copyfunc, WalSenderFunc removefunc)
{
	return FDPG_XLog::fd_SetWalSenderFunc(copyfunc, removefunc);
}

void PGStorageEngine::SyncRepSetCancelWait(void)
{
	FDPG_XLog::fd_SyncRepSetCancelWait();
}

void PGStorageEngine::LWLockReleaseAll(void)
{
	FDPG_Lock::fd_LWLockReleaseAll();
}

bool PGStorageEngine::LWLockHeldAnyByMe(void)
{
	return FDPG_Lock::fd_LWLockHeldAnyByMe();
}

void PGStorageEngine::reIndexes(Transaction* txn, EntrySet *parentSet, 
								const std::vector<IndexEntrySet*>& indexes )
{
	std::vector<Relation> indexHeaps;
	ReIndexesStat* stat = NULL;
	size_t i = 0;
	HeapScanDesc scan = NULL;
	try {		
		for (i = 0; i < indexes.size(); ++i) 
		{
			indexHeaps.push_back(((PGIndexEntrySet*)indexes[i])->getIndexRelation());
		}
		Relation heap = ((PGEntrySet*)parentSet)->getRelation();
		stat = FDPG_Index::fd_index_init_reIndexes(heap, indexHeaps);
		FDPG_Index::fd_index_insert_indexes(*stat);
		FDPG_Index::fd_index_do_build_indexes(*stat);
		FDPG_Index::fd_index_destory_reIndexes(stat);
	} catch (exception &) {   
		FDPG_Index::fd_index_destory_reIndexes(stat);
		throw;
	}
}

}//StorageEngineNS
}//FounerXDB

#if !defined(_MSC_VER)
#pragma GCC diagnostic warning "-Wclobbered"
#endif
