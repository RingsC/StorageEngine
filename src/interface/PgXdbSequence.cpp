#include <stdlib.h>
#include "StorageEngine.h"
#include "Transaction.h"
#include "DataItem.h"
#include "EntrySet.h"

#include "XdbSequence.h"
#include "interface/PgXdbSequence.h"
#include "interface/ErrNo.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/utils.h"
#include "interface/FDPGAdapter.h"
#include "catalog/xdb_catalog.h"
#include "interface/PGEntrySet.h"
#include "interface/PGIndexEntrySet.h"
#include "interface/PGTransaction.h"
#include "interface/PGStorageEngine.h"
#include "storage/s_lock.h"
#include <map>

namespace FounderXDB{
namespace StorageEngineNS{

SequenceFactory* SequenceFactory::getSequenceFactory()
{
    return PgXdbSequenceFactory::getInstance();
}

void SequenceFactory::releaseSequenceFactory(SequenceFactory* instance)
{
    PgXdbSequenceFactory* pgInstance = static_cast<PgXdbSequenceFactory*>(instance);
    return PgXdbSequenceFactory::releaseSequenceFactory(pgInstance);
}



PgXdbSequenceFactory* PgXdbSequenceFactory::m_singlePgSequenceFactory = NULL;
pthread_once_t PgXdbSequenceFactory::m_onceMutex = PTHREAD_ONCE_INIT;

PgXdbSequenceFactory* PgXdbSequenceFactory::getInstance()
{
	pthread_once(&m_onceMutex, getPGSequenceFactory);
    return m_singlePgSequenceFactory;
}

void PgXdbSequenceFactory::releaseSequenceFactory(PgXdbSequenceFactory* instance)
{
	if(instance != NULL) {
        delete instance;
        instance = NULL;
    }
}

SeqID PgXdbSequenceFactory::createSequence(xdb_seq_t initialValue,
	std::pair<xdb_seq_t, xdb_seq_t> range, const char* name, uint32 flags,
	DatabaseID dbid)
{
	EntrySetID seqTableId = 0;
	EntrySetID seqIdIdxId = 0;
	EntrySetID seqNameIdxId = 0;

	if(dbid == 0)
	{
		std::string dbname;
		dbid = StorageEngine::getStorageEngine()->getCurrentDatabase(dbname);
	}

	FDPG_Database::fd_GetSeqTableByDbid(dbid, seqTableId, seqIdIdxId, seqNameIdxId);

	/* use seqId=0 */
	XdbLock *lock = ((PGStorageEngine *)(StorageEngine::getStorageEngine()))->GetSequenceLock(seqTableId, 0);
	SeqID maxSeqId;
	{
		ScopedXdbLock scopedlock(lock, XdbLock::Exclusive);

		maxSeqId = getMaxSeqId(seqTableId, seqIdIdxId);

		const char *pName = name;
		char cName[32];
		if(pName == NULL)
		{
			snprintf(cName, 32, "%d", maxSeqId);
			pName = cName;
		}

		FDPG_Database::fd_InsertSeqInfo(seqTableId, maxSeqId, pName, initialValue,
			true, range.first, range.second, flags);
	}

	return maxSeqId;
}

SeqID PgXdbSequenceFactory::createSequence(xdb_seq_t initialValue,
	const char* name, uint32 flags, DatabaseID dbid)
{
	EntrySetID seqTableId = 0;
	EntrySetID seqIdIdxId = 0;
	EntrySetID seqNameIdxId = 0;
	if(dbid == 0)
	{
		std::string dbname;
		dbid = StorageEngine::getStorageEngine()->getCurrentDatabase(dbname);
	}
	FDPG_Database::fd_GetSeqTableByDbid(dbid, seqTableId, seqIdIdxId, seqNameIdxId);

	/* use seqId=0 */
	XdbLock *lock = ((PGStorageEngine *)StorageEngine::getStorageEngine())->GetSequenceLock(seqTableId, 0);
	SeqID maxSeqId;
	{
		ScopedXdbLock scopedlock(lock, XdbLock::Exclusive);

		maxSeqId = getMaxSeqId(seqTableId, seqIdIdxId);

		const char *pName = name;
		char cName[32];
		if(pName == NULL)
		{
			snprintf(cName, 32, "%d", maxSeqId);
			pName = cName;
		}

		FDPG_Database::fd_InsertSeqInfo(seqTableId, maxSeqId, pName, initialValue,
			false, 0, 0, flags);
	}

	return maxSeqId;
}

XdbSequence* PgXdbSequenceFactory::openSequence(SeqID seqID, DatabaseID dbid)
{
	Transaction *xact = StorageEngine::getStorageEngine()->GetCurrentTransaction();

	if(dbid == 0)
	{
        std::string dbname;
        dbid = StorageEngine::getStorageEngine()->getCurrentDatabase(dbname);
	}
    
    if(dbid == (DatabaseID)-1)
    {
        dbid = StorageEngine::getStorageEngine()->getDatabaseID("defaultdatabase");
    }
    
	Assert(xact != NULL);
	MemoryContext *mcxt = xact->getAssociatedMemoryContext();
	XdbSequence *sequence = NULL;
	sequence = ((PGTransaction *)xact)->getSequence(dbid, seqID);
	if(sequence == NULL)
	{
		sequence = new(*mcxt)PgXdbSequence(seqID, dbid);
		((PGTransaction *)xact)->cacheSequence(dbid, seqID, sequence->getName(), sequence);
	}

	return sequence;
}

XdbSequence* PgXdbSequenceFactory::openSequence(const char* seqName, DatabaseID dbid)
{
	Transaction *xact = StorageEngine::getStorageEngine()->GetCurrentTransaction();

	if(dbid == 0)
	{
		std::string dbname;
		dbid = StorageEngine::getStorageEngine()->getCurrentDatabase(dbname);
	}
    
    
	if(dbid == (DatabaseID)-1)
    {
        dbid = StorageEngine::getStorageEngine()->getDatabaseID("defaultdatabase");
    }
    
	Assert(xact != NULL);
	MemoryContext *mcxt = xact->getAssociatedMemoryContext();
	XdbSequence *sequence = NULL;
	sequence = ((PGTransaction *)xact)->getSequence(dbid, seqName);
	if(sequence == NULL)
	{
		sequence = new(*mcxt)PgXdbSequence(seqName, dbid);
		((PGTransaction *)xact)->cacheSequence(dbid, sequence->getId(), seqName, sequence);
	}

	return sequence;
}

void PgXdbSequenceFactory::deleteSequence(SeqID seqID, DatabaseID dbid)
{
    if(dbid == 0)
    {
        std::string dbname;
        dbid = StorageEngine::getStorageEngine()->getCurrentDatabase(dbname);
    }
	/* clear cached sequence */
	std::string dbname;
	Transaction *xact = StorageEngine::getStorageEngine()->GetCurrentTransaction();
	((PGTransaction *)xact)->clearCachedSequence(dbid, seqID);

	/* delete data */
	EntrySetID seqTableId = 0;
	EntrySetID seqIdIdxId = 0;
	EntrySetID seqNameIdxId = 0;

	FDPG_Database::fd_GetSeqTableByDbid(dbid, seqTableId, seqIdIdxId, seqNameIdxId);
	FDPG_Database::fd_DeleteSeqInfoById(seqTableId, seqIdIdxId, seqID);

	((PGStorageEngine *)StorageEngine::getStorageEngine())->RemoveSequenceLock(seqTableId, seqID);

	return;
}

void PgXdbSequenceFactory::deleteSequence(const char*name, DatabaseID dbid)
{
    if(dbid == 0)
    {
        std::string dbname;
        dbid = StorageEngine::getStorageEngine()->getCurrentDatabase(dbname);
    }
	/* clear cached sequence */
	Transaction *xact = StorageEngine::getStorageEngine()->GetCurrentTransaction();
	((PGTransaction *)xact)->clearCachedSequence(dbid, name);

	/* delete data */
	EntrySetID seqTableId = 0;
	EntrySetID seqIdIdxId = 0;
	EntrySetID seqNameIdxId = 0;

	FDPG_Database::fd_GetSeqTableByDbid(dbid, seqTableId, seqIdIdxId, seqNameIdxId);

	SeqID seqID;
	FDPG_Database::fd_DeleteSeqInfo(seqTableId, seqNameIdxId, name, seqID);

	((PGStorageEngine *)StorageEngine::getStorageEngine())->RemoveSequenceLock(seqTableId, seqID);

	return;
}

SeqID PgXdbSequenceFactory::getMaxSeqId(EntrySetID seqTableId, EntrySetID seqIdIdxId)
{
	SeqID maxSeqId;
	FDPG_Database::fd_GetMaxSeqId(seqTableId, seqIdIdxId, maxSeqId);

	return maxSeqId;
}

void PgXdbSequenceFactory::getPGSequenceFactory()
{
	m_singlePgSequenceFactory = new PgXdbSequenceFactory;
}

PgXdbSequence::PgXdbSequence(SeqID seqId, DatabaseID dbid) : m_dbid(dbid), m_seqId(seqId)
{
	FDPG_Database::fd_GetSeqTableByDbid(m_dbid, m_seqTableId, m_seqIdIdxId, m_seqnameIdxId);

	/* get seq info */
	int64 value;
	int64 minValue;
	int64 maxValue;
	FDPG_Database::fd_GetSeqInfoById(m_seqTableId, m_seqIdIdxId, m_seqId, m_seqName,
		value, m_hasRange, minValue, maxValue, m_flags);

	m_range = std::make_pair(minValue, maxValue);
}

PgXdbSequence::PgXdbSequence(const char *name, DatabaseID dbid) : m_dbid(dbid), m_seqName(name)
{
	FDPG_Database::fd_GetSeqTableByDbid(m_dbid, m_seqTableId, m_seqIdIdxId, m_seqnameIdxId);

	/* get seq info */
	int64 value;
	int64 minValue;
	int64 maxValue;
	FDPG_Database::fd_GetSeqInfo(m_seqTableId, m_seqnameIdxId, m_seqName.c_str(), m_seqId, 
		value, m_hasRange, minValue, maxValue, m_flags);

	if(m_hasRange)
	{
		m_range = std::make_pair(minValue, maxValue);
	}
	else
	{
		m_range = std::make_pair(0, 0);
	}
}

void PgXdbSequence::reset(xdb_seq_t newInitialValue)
{
	xdb_seq_t oldValue;

	XdbLock *lock = ((PGStorageEngine *)StorageEngine::getStorageEngine())->GetSequenceLock(m_seqTableId, m_seqId);
	{
		ScopedXdbLock scopedlock(lock, XdbLock::Exclusive);
		FDPG_Database::fd_UpdateSeqValueById(m_seqTableId, m_seqIdIdxId, m_seqId, false, newInitialValue, oldValue);
	}
	
	return;
}

xdb_seq_t PgXdbSequence::getValue(int32 delta) const
{
	xdb_seq_t oldValue;

	XdbLock *lock = ((PGStorageEngine *)StorageEngine::getStorageEngine())->GetSequenceLock(m_seqTableId, m_seqId);
	{
		ScopedXdbLock scopedlock(lock, XdbLock::Exclusive);
		FDPG_Database::fd_UpdateSeqValueById(m_seqTableId, m_seqIdIdxId, m_seqId, true, delta, oldValue);
	}

	return oldValue;
}

const char* PgXdbSequence::getName() const
{
	return m_seqName.c_str();
}

SeqID PgXdbSequence::getId() const
{
	return m_seqId;
}

std::pair<xdb_seq_t, xdb_seq_t> PgXdbSequence::getRange() const
{
	// has range
	return m_range;
}

uint32 PgXdbSequence::getFlags() const
{
	return m_flags;
}

sequenceInfo PgXdbSequence::getSequenceInfo() const
{
	sequenceInfo seqInfo;

	seqInfo.seqId = m_seqId;
	seqInfo.flags = m_flags;
	seqInfo.range = m_range;

	/* get value */
	FDPG_Database::fd_GetSeqValueById(m_seqTableId, m_seqIdIdxId, m_seqId, seqInfo.value);

	return seqInfo;
}

} //StorageEngineNS
}//FounerXDB
