#include <set>
#include <map>

#include "postgres.h"
#include "interface/ErrNo.h"
#include "interface/PGTransaction.h"
#include "interface/PgXdbSequence.h"
#include "interface/PGIndexEntrySet.h"
#include "interface/PGEntrySet.h"
#include "LargeObject.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "access/xlog.h"
#include "access/xact.h"
#include "interface/FDPGAdapter.h"
#include "interface/PGStorageEngine.h"
#include "interface/PGLargeObject.h"
#include "interface/utils.h"
#include "PgMemcontext.h"
#include "nodes/memnodes.h"
#include "utils/memutils.h"

using std::set;
using std::exception;

namespace FounderXDB{
namespace StorageEngineNS {
// how to deal with transaction's error
PGTransaction::PGTransaction(FXTransactionId transaction_id, IsolationLevel isolation_level,beginFunc beg)
{
	m_isPrepared = false;
    userData_ = 0;
    m_transaction_id = transaction_id;
    m_isolation_level = isolation_level;
    m_CurrentTransactionState = NULL;
    m_TopTransactionState = NULL;
	currentSnapshot = 0;
    //seqFactory_ = 0;
    if (NULL != beg)
    {
		beg();
    }
    
    //PgMemcontext *pContext = static_cast<PgMemcontext*>(MemoryContext::getMemoryContext(MemoryContext::Transaction));
    m_TransactionMemoryContext = new PgMemcontext(::CurTransactionContext, true);
	
	if (READ_UNCOMMITTED_ISOLATION_LEVEL == isolation_level)
	{
		isolation_level = READ_COMMITTED_ISOLATION_LEVEL;
	}
	
    FDPG_Transaction::fd_setXactIsoLevel(isolation_level);
}


PGTransaction::~PGTransaction()
{
    m_CurrentTransactionState = NULL;
    m_TopTransactionState = NULL;
    if(m_TransactionMemoryContext)
    {
        m_TransactionMemoryContext->destroy();
        m_TransactionMemoryContext = 0;
    }
}
    
void PGTransaction::begin( void )
{
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_BeginTransactionBlock();
	FDPG_Transaction::fd_CommitTransactionCommand();
}

void PGTransaction::abort()
{
    closeCachedSequences();
    closeCachedEntrySets();
	closeCacheLargeObject();

	if(m_global_trans_id != InvalidTransactionID)
	{
		char gxid[254];
		memset(gxid, 0, 254);
		sprintf(gxid, "%u", m_global_trans_id);

		if(FDPG_Transaction::fd_GlobalTransactionIdIsPrepared(gxid))
		{
			FDPG_Transaction::fd_StartTransactionCommand();
			FDPG_Transaction::fd_FinishPreparedTransaction(gxid, false);
			FDPG_Transaction::fd_CommitTransactionCommand(); 
			return;
		}
	}

	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_UserAbortTransactionBlock();
	FDPG_Transaction::fd_CommitTransactionCommand();
}

void PGTransaction::abortCurrent()
{
	closeCachedSequences();
	closeCachedEntrySets();
	closeCacheLargeObject();

	FDPG_Transaction::fd_AbortOutOfAnyTransaction();
}

void PGTransaction::commit()
{
    closeCachedSequences();
    closeCachedEntrySets();
	closeCacheLargeObject();

	if(m_global_trans_id != InvalidTransactionID)
	{
		char gxid[254];
		memset(gxid, 0, 254);
		sprintf(gxid, "%u", m_global_trans_id);

		if(FDPG_Transaction::fd_GlobalTransactionIdIsPrepared(gxid))
		{
			FDPG_Transaction::fd_StartTransactionCommand();
			FDPG_Transaction::fd_FinishPreparedTransaction(gxid, true);
			FDPG_Transaction::fd_CommitTransactionCommand(); 
			return;
		}
	}

	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_EndTransactionBlock();
	FDPG_Transaction::fd_CommitTransactionCommand();
}

void PGTransaction::prepare()
{
	if(m_global_trans_id == InvalidTransactionID)
	{
		assert(false);
		return;
	}

	if(m_TransactionMemoryContext == NULL)
	{
		// TODO:
		return;
	}

	closeCachedSequences();
	closeCachedEntrySets();
	closeCacheLargeObject();

	char gxid[254];
	memset(gxid, 0, 254);
	sprintf(gxid, "%u", m_global_trans_id);
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_PrepareTransactionBlock(gxid);
	FDPG_Transaction::fd_CommitTransactionCommand();

	// TODO: set m_CurrentTransactionState, m_TopTransactionState, m_TransactionMemoryContext
	set_current_transaction_state(NULL);
	//set_current_transaction_state(FDPG_Transaction::fd_get_current_transaction_state());
	//set_top_transaction_state(FDPG_Transaction::fd_get_top_transaction_state());
	m_TransactionMemoryContext = NULL;

	PGStorageEngine *pSE = (PGStorageEngine *)StorageEngine::getStorageEngine();
	pSE->SetCurrentTransaction(NULL);

	m_isPrepared = true;
}

Transaction::IsolationLevel PGTransaction::getIsolationLevel() const
{
    return m_isolation_level;
}

void PGTransaction::setIsolationLevel(IsolationLevel level)
{
    if (level < NO_ISOLATION_LEVEL_SPECIFIED) {
        m_isolation_level = level;
        FDPG_Transaction::fd_setXactIsoLevel(level);
        return ;
    }
    m_isolation_level = READ_COMMITTED_ISOLATION_LEVEL;
    FDPG_Transaction::fd_setXactIsoLevel(level);
}

FXTransactionId PGTransaction::getTransactionId() const
{
    return m_transaction_id;
}

Transaction* PGTransaction::startSubTransaction(IsolationLevel isolevel)
{
	isolevel = (IsolationLevel)FDPG_Transaction::fd_getXactIsoLevel();
	return ((PGStorageEngine*)StorageEngine::getStorageEngine())->createTransaction(isolevel,this);
}

TransactionState PGTransaction::get_top_transaction_state()
{
    return m_TopTransactionState;
}

TransactionState PGTransaction::get_current_transaction_state()
{
    return m_CurrentTransactionState;
}

void PGTransaction::set_top_transaction_state(TransactionState top_stransaction_state)
{
    m_TopTransactionState = top_stransaction_state;
}

void PGTransaction::set_current_transaction_state(TransactionState current_stransaction_state)
{
    m_CurrentTransactionState = current_stransaction_state;
}

void PGTransaction::closeCacheLargeObject()
{
	/*MemoryContext *cxt = */this->getAssociatedMemoryContext();

	std::map<std::pair<DatabaseID, uint32>, LargeObject *>::iterator idIt = this->m_myWriteIDOpenLargeObject_.begin();
	std::map<std::pair<DatabaseID, uint32>, LargeObject *>::iterator idIt2 = this->m_myReadIDOpenLargeObject_.begin();

	while(idIt != this->m_myWriteIDOpenLargeObject_.end())
	{
		/*PGLargeObject *tmpLo =*/ static_cast<PGLargeObject *>(idIt->second);

		delete idIt->second;
		++idIt;
	}

	while(idIt2 != this->m_myReadIDOpenLargeObject_.end())
	{
		/*PGLargeObject *tmpLo =*/ static_cast<PGLargeObject *>(idIt2->second);

		delete idIt2->second;
		++idIt2;
	}

	this->m_myNameOpenLargeObject_.clear();
	this->m_myWriteIDOpenLargeObject_.clear();
	this->m_myReadIDOpenLargeObject_.clear();
	this->m_recentOpenLO.clear();
}

void PGTransaction::endStatementLargeObject()
{
	FDPG_LargeObject::fd_at_endstatement_large_object();
}

void PGTransaction::setGXID(FXGlobalTransactionId gxid)
{
	this->m_global_trans_id = gxid;
}

void PGTransaction::closeCachedEntrySets(bool delObj)
{
    std::map<EntrySetID, BaseEntrySet *>::iterator itr;
    
    //delete the index entrysets first to remove the dependency on the entrysets
    /*for (itr = this->myOpenEntrySets_.begin(); itr != this->myOpenEntrySets_.end();) {
        bool flag = true;
        if (itr->second->getType() != HEAP_ROW_ENTRY_SET_TYPE)
        {
            ((PGStorageEngine *)StorageEngine::getStorageEngine())->closeEntrySetInternal(this, (IndexEntrySet *)itr->second);
            this->myOpenEntrySets_.erase(itr++);
			if(delObj)
			{
				PGIndexEntrySet * temp = ((PGIndexEntrySet *)itr->second);
				delete temp;
			}
            flag = false;
        }
        if(flag)
            itr++;
    }*/
    
    for (itr = this->myOpenEntrySets_.begin(); itr != this->myOpenEntrySets_.end(); ++itr) {
       if (itr->second->getType() == HEAP_ROW_ENTRY_SET_TYPE)
		{	PGEntrySet * temp = (PGEntrySet *)itr->second;
            ((PGStorageEngine *)StorageEngine::getStorageEngine())->closeEntrySetInternal(this, temp);
			if(delObj)
				delete temp;
		}
        else
		{
			PGIndexEntrySet * temp = (PGIndexEntrySet *)itr->second;
            ((PGStorageEngine *)StorageEngine::getStorageEngine())->closeEntrySetInternal(this, (IndexEntrySet *)itr->second);
			if(delObj)
				delete temp;
		}
    }
    this->myOpenEntrySets_.clear();
}

void PGTransaction::closeEntrySet(EntrySetID esid, bool unlock, bool del)
{
    std::map<EntrySetID, BaseEntrySet *>::iterator itr, end;
    itr = this->myOpenEntrySets_.find(esid);
    end = this->myOpenEntrySets_.end();
    if (itr != end)//found
    {
        if (itr->second->getType() == HEAP_ROW_ENTRY_SET_TYPE)
		{	PGEntrySet * temp = (PGEntrySet *)itr->second;
            ((PGStorageEngine *)StorageEngine::getStorageEngine())->closeEntrySetInternal(this, temp, unlock);
			this->myOpenEntrySets_.erase(itr);
			if(del)
				delete temp;
		}
        else
		{
			PGIndexEntrySet * temp = (PGIndexEntrySet *)itr->second;
            ((PGStorageEngine *)StorageEngine::getStorageEngine())->closeEntrySetInternal(this, (PGIndexEntrySet *)itr->second, unlock);
			this->myOpenEntrySets_.erase(itr);
			if(del)
				delete temp;
		}
    }
}


XdbSequence *PGTransaction::getSequence(DatabaseID dbid, SeqID seqId) const
{
	std::pair<DatabaseID, SeqID> seqIdPair = std::make_pair(dbid, seqId);
	std::map<std::pair<DatabaseID, SeqID>, XdbSequence *>::const_iterator it = m_myIDOpenSeqs.find(seqIdPair);
    if (it != m_myIDOpenSeqs.end())
        return it->second;
    return NULL;
}

XdbSequence *PGTransaction::getSequence(DatabaseID dbid, const char *seqName) const
{
	std::pair<DatabaseID, std::string> seqNamePair = std::make_pair(dbid, std::string(seqName));
	std::map<std::pair<DatabaseID, std::string>, XdbSequence *>::const_iterator it = m_myNameOpenSeqs.find(seqNamePair);
    if (it != m_myNameOpenSeqs.end())
        return it->second;
    return NULL;
}

void PGTransaction::cacheSequence(DatabaseID dbid, SeqID seqId,
	const char *seqName, XdbSequence *pSeq)
{
	std::pair<DatabaseID, SeqID> seqIdPair = std::make_pair(dbid, seqId);
	std::pair<DatabaseID, std::string> seqNamePair = std::make_pair(dbid, std::string(seqName));
	m_myIDOpenSeqs.insert(std::make_pair(seqIdPair, pSeq));
	m_myNameOpenSeqs.insert(std::make_pair(seqNamePair, pSeq));
}

void PGTransaction::clearCachedSequence(DatabaseID dbid, SeqID seqId, const char *seqName)
{
	std::pair<DatabaseID, SeqID> seqIdPair = std::make_pair(dbid, seqId);
	std::map<std::pair<DatabaseID, SeqID>, XdbSequence *>::iterator it1 = m_myIDOpenSeqs.find(seqIdPair);
    if (it1 != m_myIDOpenSeqs.end())
    {
        delete it1->second;
        it1->second = NULL;
        m_myIDOpenSeqs.erase(it1);
    }

	std::pair<DatabaseID, std::string> seqNamePair = std::make_pair(dbid, std::string(seqName));
	std::map<std::pair<DatabaseID, std::string>, XdbSequence *>::iterator it2 = m_myNameOpenSeqs.find(seqNamePair);
    if (it2 != m_myNameOpenSeqs.end())
    {
        delete it2->second;
        it2->second = NULL;
        m_myNameOpenSeqs.erase(it2);
    }
}

void PGTransaction::clearCachedSequence(DatabaseID dbid, SeqID seqId)
{
	XdbSequence *sequence = getSequence(dbid, seqId);
	if(sequence != NULL)
	{
		clearCachedSequence(dbid, seqId, sequence->getName());
	}
}

void PGTransaction::clearCachedSequence(DatabaseID dbid, const char *seqName)
{
	XdbSequence *sequence = getSequence(dbid, seqName);
	if(sequence != NULL)
	{
		clearCachedSequence(dbid, sequence->getId(), seqName);
	}
}

//void PGTransaction::closeSequence(DatabaseID dbid, SeqID seqId)
//{
//	// TODO:
//}

//void PGTransaction::closeSequence(DatabaseID dbid, std::string seqName)
//{
//	// TODO:
//}

void PGTransaction::closeCachedSequences()
{
	m_myNameOpenSeqs.clear();

	for(std::map<std::pair<DatabaseID, SeqID>, XdbSequence *>::iterator it = m_myIDOpenSeqs.begin();
		it != m_myIDOpenSeqs.end();)
    {
        delete it->second;
        it->second = NULL;
        m_myIDOpenSeqs.erase(it++);
    }
    m_myIDOpenSeqs.clear();
}

PGSubTransaction::PGSubTransaction(FXTransactionId transaction_id
				 , IsolationLevel isolation_level)
				 :PGTransaction(transaction_id,isolation_level,PGSubTransaction::begin)				 
{

}

void PGSubTransaction::begin(void)
{
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_BeginSubTransaction((char*)"");
	FDPG_Transaction::fd_CommandCounterIncrement();
	//FDPG_Transaction::fd_CommitTransactionCommand(); 
}
void PGSubTransaction::abort()
{
	closeCachedSequences();
	closeCachedEntrySets();
	closeCacheLargeObject();

	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_AbortSubTransaction();
	FDPG_Transaction::fd_CommandCounterIncrement();
	//FDPG_Transaction::fd_CommitTransactionCommand();
}

/// Commit the transaction.
void PGSubTransaction::commit()
{
	closeCachedSequences();
	closeCachedEntrySets();
	closeCacheLargeObject();

	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_CommitSubTransaction();
	FDPG_Transaction::fd_CommandCounterIncrement();
	//FDPG_Transaction::fd_CommitTransactionCommand();
}

/// Prepare the transaction.
void PGSubTransaction::prepare()
{
	//FDPG_Transaction::fd_StartTransactionCommand();
	//PrepareTransactionBlock(NULL);
	//FDPG_Transaction::fd_CommitTransactionCommand();
}

BaseEntrySet *PGTransaction::getEntrySet(EntrySetID esid, DatabaseID dbid) const 
{
    std::map<EntrySetID, BaseEntrySet *>::const_iterator itr = this->myOpenEntrySets_.find(esid);
    if (itr != this->myOpenEntrySets_.end())//found
        return itr->second;
    else
        return NULL;
}

void PGTransaction::cacheLargeObject(LargeObject *lo, bool isWrite)
{
	this->m_myNameOpenLargeObject_.insert(std::make_pair(
		std::pair<DatabaseID, std::string>(MyDatabaseId, lo->getName()), lo->getId()));

	if(isWrite)
		this->m_myWriteIDOpenLargeObject_.insert(std::make_pair(
		std::pair<DatabaseID, uint32>(MyDatabaseId, lo->getId()), lo));
	else
		this->m_myReadIDOpenLargeObject_.insert(std::make_pair(
		std::pair<DatabaseID, uint32>(MyDatabaseId, lo->getId()), lo));
}

LargeObject *PGTransaction::getRecentOpenLO(std::string name)
{
	std::map<std::pair<DatabaseID, std::string>, LargeObject*>::const_iterator it = 
		m_recentOpenLO.find(std::pair<DatabaseID, std::string>(MyDatabaseId, name));

	if(it != m_recentOpenLO.end())
	{
		return it->second;
	}

	return NULL;
}

std::map<std::pair<DatabaseID, std::string>, LargeObject*> *PGTransaction::getRecentOpenLOHash()
{
	return &m_recentOpenLO;
}

void PGTransaction::unCacheLargeObject(const char *name)
{
	std::map<std::pair<DatabaseID, std::string>, uint32>::iterator it = 
		this->m_myNameOpenLargeObject_.find(std::pair<DatabaseID, std::string>(MyDatabaseId, name));

	if(it != this->m_myNameOpenLargeObject_.end())
	{
		unCacheLargeObject(it->second);
	}
}

void PGTransaction::unCacheLargeObject(const uint32 loid)
{
	std::map<std::pair<DatabaseID, uint32>, LargeObject *>::iterator it = 
		this->m_myWriteIDOpenLargeObject_.find(std::pair<DatabaseID, uint32>(MyDatabaseId, loid));

	std::map<std::pair<DatabaseID, uint32>, LargeObject *>::iterator it3 = 
		this->m_myReadIDOpenLargeObject_.find(std::pair<DatabaseID, uint32>(MyDatabaseId, loid));

	if(it != this->m_myWriteIDOpenLargeObject_.end() ||
		 it3 != this->m_myReadIDOpenLargeObject_.end())
	{
		std::map<std::pair<DatabaseID, std::string>, uint32>::iterator it2;

		if(it != this->m_myWriteIDOpenLargeObject_.end())
		{
			it2 = 
				this->m_myNameOpenLargeObject_.find(std::pair<DatabaseID, std::string>(MyDatabaseId, it->second->getName()));

			if(it2 != this->m_myNameOpenLargeObject_.end())
			{
				this->m_myNameOpenLargeObject_.erase(it2);
			}

			delete it->second;
			this->m_myWriteIDOpenLargeObject_.erase(it);
		}

		if(it3 != this->m_myReadIDOpenLargeObject_.end())
		{
			it2 = 
				this->m_myNameOpenLargeObject_.find(std::pair<DatabaseID, std::string>(MyDatabaseId, it3->second->getName()));

			if(it2 != this->m_myNameOpenLargeObject_.end())
			{
				this->m_myNameOpenLargeObject_.erase(it2);
			}

			delete it3->second;
			this->m_myReadIDOpenLargeObject_.erase(it3);
		}
	}
}

LargeObject *PGTransaction::getLargeObject(const char *name, bool isWrite) const
{
	std::map<std::pair<DatabaseID, std::string>, uint32>::const_iterator it = 
		this->m_myNameOpenLargeObject_.find(std::pair<DatabaseID, std::string>(MyDatabaseId, name));
	if(it != this->m_myNameOpenLargeObject_.end())
	{
		uint32 loid = it->second;
		return getLargeObject(loid, isWrite);
	}

	return NULL;
}

LargeObject *PGTransaction::getLargeObject(const uint32 loid, bool isWrite) const
{
	if(isWrite)
	{
		std::map<std::pair<DatabaseID, uint32>, LargeObject *>::const_iterator it = 
			this->m_myWriteIDOpenLargeObject_.find(std::pair<DatabaseID, uint32>(MyDatabaseId, loid));
		if(it != this->m_myWriteIDOpenLargeObject_.end())
		{
			return it->second;
		}
	} else {
		std::map<std::pair<DatabaseID, uint32>, LargeObject *>::const_iterator it = 
			this->m_myReadIDOpenLargeObject_.find(std::pair<DatabaseID, uint32>(MyDatabaseId, loid));
		if(it != this->m_myReadIDOpenLargeObject_.end())
		{
			return it->second;
		}
	}
	return NULL;
}

void PGTransaction::cacheEntrySet(EntrySetID esid, DatabaseID dbid, BaseEntrySet *bes)
{
    this->myOpenEntrySets_.insert(std::make_pair(esid, bes));
}

MemoryContext* PGTransaction::getAssociatedMemoryContext() const
{
	return m_TransactionMemoryContext;
}
Snapshot PGTransaction::getSnapshot(bool isNew)
{
	if (isNew)
	{			
		currentSnapshot = FDPG_Transaction::fd_GetTransactionSnapshot();
	}
	else
	{
		if(!currentSnapshot)			
			currentSnapshot = FDPG_Transaction::fd_GetTransactionSnapshot();
	}
	return currentSnapshot;
}

} //StorageEngineNS
}//FounerXDB
