/// @file PGTempEntrySet.cpp 
/// @brief Implements the member functions for the PGTempEntrySet class.
/// 
/// This file implements the member functions for the classes defined in PGTempEntrySet.h.
///
#include <string>
#include <vector>
#include <cassert>

#include "postgres.h"
#include "miscadmin.h"

#include "interface/PGTempEntrySet.h"
#include "interface/PGTransaction.h"
#include "interface/ErrNo.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "Transaction.h"
#include "interface/FDPGAdapter.h"

#define ASSERT_SE(condition) assert((condition))
#define NEW(m_size) new
#define DELETE_OBJ(object) delete (object)
#define FD_PFREE(object) FDPG_Memory::fd_pfree(object)

namespace FounderXDB
{
namespace StorageEngineNS 
{
// Constructor
PGTempEntrySet::PGTempEntrySet(StoreSortType sst,
															 bool randomAccess, 
															 bool interXact, 
															 int maxKBytes,
															 unsigned int spaceId)
	: EntrySet(InvalidOid, TEMPORARY_ENTRY_SET_TYPE)
	, m_TableSpaceId(spaceId)
	, m_iWorkingMemorySize(maxKBytes)
	, m_pTupleStoreState(NULL)
	, m_sst(sst)
{
	try
	{
		m_pTupleStoreState = FDPG_TupleStore::fd_tuplestore_begin_heap
			(
			randomAccess,
			interXact,
			maxKBytes
			);
	}catch(StorageEngineExceptionUniversal &)
	{
		m_TableSpaceId = InvalidOid;
		m_iWorkingMemorySize = -1;
		m_pTupleStoreState = NULL;
		throw;
	}
	ASSERT_SE(m_pTupleStoreState);
}

// Deconstructor
PGTempEntrySet::~PGTempEntrySet()
{
	ASSERT_SE(m_pTupleStoreState);
	try
	{
		FDPG_TupleStore::fd_tuplestore_end(m_pTupleStoreState);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}
}

// Insert a set of entry into this entry set.
int PGTempEntrySet::insertEntries(Transaction* txn, const std::vector<DataItem>& rset)
{
	if(!m_pTupleStoreState) 
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Not init temp entry set yet!");
	}

	char *data = NULL;
	size_t data_len = 0;
	HeapTuple tuple = NULL;
	try
	{
		for(Index i = 0; i < rset.size(); ++i)
		{
			data = static_cast<char*>(rset[i].getData());
			data_len = rset[i].getSize();
			tuple = FDPG_Heap::fd_heap_form_tuple(data, data_len);
			FDPG_TupleStore::fd_tuplestore_puttuple_common(m_pTupleStoreState, static_cast<void*>(tuple));
			FDPG_Memory::fd_pfree(tuple);
		}
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return 0;
}

/// Insert an entry into the entry set.
int PGTempEntrySet::insertEntries(Transaction* txn, const EntrySet& rset)
{
	if(!m_pTupleStoreState) 
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Not init temp entry set yet!");
	}

	EntrySet *entrySet = const_cast<EntrySet*>(&rset);
	PGEntrySet *pEntrySet = static_cast<PGEntrySet*>(entrySet);

	Relation relation = pEntrySet->getRelation();
	if(NULL == relation)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "EntrySet is empty. Open it first please.");
	}

	HeapScanDesc scan = NULL;
	try
	{
		scan = FDPG_Heap::fd_heap_beginscan(relation, ((PGTransaction*)txn)->getSnapshot(), 0, NULL);
		HeapTuple tuple = NULL;
		while(NULL != (tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)))
		{
			FDPG_TupleStore::fd_tuplestore_puttuple_common(m_pTupleStoreState, static_cast<void*>(tuple));
		}
		FDPG_Heap::fd_heap_endscan(scan);
	}catch(StorageEngineExceptionUniversal &)
	{
		if(NULL != scan)
		{
			FDPG_Heap::fd_heap_endscan(scan);
		}
		throw;
	}

	return 0;
}

/// Insert an entry into the entry set.
int PGTempEntrySet::insertEntry(Transaction* txn, EntryID &eid, const DataItem& entry)
{
	/* EID was useless here. */

	if(!m_pTupleStoreState) 
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Not init temp entry set yet!");
	}

	HeapTuple tuple = NULL;
	try
	{
		char *tmp_data = static_cast<char*>(entry.getData());
		tuple = FDPG_Heap::fd_heap_form_tuple(tmp_data, entry.getSize());
		FDPG_TupleStore::fd_tuplestore_puttuple_common(m_pTupleStoreState, static_cast<void*>(tuple));
		FDPG_Memory::fd_pfree(tuple);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return 0;
}

int PGTempEntrySet::truncate()
{
	///Not support yet;
	return 0;
}

EntrySetScan* PGTempEntrySet::startEntrySetScan(Transaction* txn,
																								SnapshotTypes opt,  
																								const std::vector<ScanCondition>&scankeys, filter_func_t filterFunc, void*userData,const EntryID* eid, const EntryID* endid )
{
    Assert(! (filterFunc || userData));// the two parameters are only used in index scan.
	MemoryContext *cxt = txn->getAssociatedMemoryContext();
	return new(*cxt) TempEntrySetScan(this);
}

void PGTempEntrySet::endEntrySetScan(EntrySetScan* scan)
{
	delete scan;
}

TempEntrySetScan::TempEntrySetScan(PGTempEntrySet *tmpEntry)
: m_iCurrentReader(0)
, m_iMarkedReader(0)
{
	m_pTupleStoreState = tmpEntry->m_pTupleStoreState;
	m_iMarkedReader = FDPG_TupleStore::fd_tuplestore_alloc_read_pointer(m_pTupleStoreState, m_iCurrentReader);
	FDPG_TupleStore::fd_tuplestore_rescan(m_pTupleStoreState);
}

TempEntrySetScan::~TempEntrySetScan()
{
	m_pTupleStoreState = NULL;
}

int TempEntrySetScan::getNext(CursorMovementFlags opt, EntryID &eid, DataItem& entry)
{
	/* EID was useless here. */

	if(!m_pTupleStoreState)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Temp entry set scan not init correct.");
	}

	void *mTuple = NULL;	
	HeapTuple tup = NULL;
	bool shouldFree = false;
	int len = 0;
	void *pData = NULL;
	bool forward = true;
	forward = (opt == NEXT_FLAG ? true : false);
	try
	{
		mTuple = FDPG_TupleStore::fd_tuplestore_gettuple(m_pTupleStoreState, forward, &shouldFree);
		if(!mTuple)
		{
			return NO_DATA_FOUND;
		}
		pData = FDPG_TupleStore::fd_minimal_tuple_getdata((MinimalTuple)mTuple,len);
		if(shouldFree)
		{
			FDPG_Memory::fd_pfree(mTuple);
		}
		entry.setData(pData);
		entry.setSize(len);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return SUCCESS;
}

int TempEntrySetScan::markPosition()
{
	if(!m_pTupleStoreState)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Temp entry set scan not init correct.");
	}

	try
	{
		FDPG_TupleStore::fd_tuplestore_copy_read_pointer(m_pTupleStoreState, m_iCurrentReader, m_iMarkedReader);
		//FDPG_TupleStore::fd_tuplestore_trim(m_pTupleStoreState);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return SUCCESS;
}

int TempEntrySetScan::restorePosition()
{
	if(!m_pTupleStoreState)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Temp entry set scan not init correct.");
	}

	try
	{
		FDPG_TupleStore::fd_tuplestore_copy_read_pointer(m_pTupleStoreState, m_iMarkedReader, m_iCurrentReader);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return SUCCESS;
}

int TempEntrySetScan::getNextBatch(CursorMovementFlags opt, std::vector<std::pair<EntryID, DataItem> > &eid_entries)
{
	/// Not support yet.
	return NO_DATA_FOUND;
}
}
}