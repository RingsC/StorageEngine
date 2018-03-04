/// @file PGSortEntrySet.cpp
/// @brief Implements the member functions for the PGSortEntrySet class.
/// 
/// This file implements the member functions for the classes defined in PGSortEntrySet.h.
///
#include <string>
#include <vector>
#include <cassert>

#include "postgres.h"
#include "miscadmin.h"

#include "interface/PGSortEntrySet.h"
#include "interface/PGTransaction.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "Transaction.h"
#include "interface/FDPGAdapter.h"
#include "interface/ErrNo.h"
#include "interface/PGEntrySet.h"

#define ASSERT_SE(condition) assert((condition))
#define NEW(size) new
#define DELETE_DYNAMIC(object) delete (object)

namespace FounderXDB
{
namespace StorageEngineNS
{

// Constructor
PGSortEntrySet::PGSortEntrySet(StoreSortType sst)
	: SortEntrySet(InvalidOid, SORT_ENTRY_SET_TYPE)
	, m_state(NULL)
	, m_bSortDone(false)
	, m_pCompare(NULL)
	, m_sst(sst)
{

}

// Deconstructor
PGSortEntrySet::~PGSortEntrySet()
{
	FDPG_TupleSort::fd_tuplesort_end(m_state);
	m_state = NULL;
	if(m_pCompare != NULL)
	{
		//m_pCompare->destroy();
	}
}

void PGSortEntrySet::doPut(const DataItem &di)
{
	HeapTuple tuple = NULL;
	switch(m_sst)
	{
	case SST_GenericDataTuples:
		FDPG_TupleSort::fd_tuplesort_putdata(m_state, di.getData(), di.getSize());
		break;
	case SST_HeapTuples:
		tuple = FDPG_Heap::fd_heap_form_tuple(static_cast<char*>(di.getData()), di.getSize());
		FDPG_TupleSort::fd_tuplesort_put_heaptuple(m_state,tuple);
		FDPG_Memory::fd_pfree(tuple);
		break;
	default:
		throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invaild store sort type!");
	}
}

//initialize the sort entry set
void PGSortEntrySet::initialize(int workMem,
																bool randomAccess,
																CompareCallbacki pCompare)
{
	ASSERT_SE(m_state == NULL);
	m_pCompare = pCompare;

	switch(m_sst)
	{
		case SST_GenericDataTuples:
			m_state = FDPG_TupleSort::fd_tuplesort_begin_data(
				workMem, 
				randomAccess, 
				(void *)pCompare);
			break;
		case SST_HeapTuples:
			m_state = FDPG_TupleSort::fd_tuplesort_begin_heap(
				workMem, 
				randomAccess, 
				(void *)pCompare);
			break;
		default:
			throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invaild store sort type!");
	}
	

}

/// Insert a set of entries into this entry set.
int PGSortEntrySet::insertEntries(Transaction* txn, const std::vector<DataItem>& rset)
{
	if(!m_state)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Not init sort entry set yet!");
	}
	
	try
	{
		//void *put_func = NULL;
		for(Index i = 0; i < rset.size(); ++i)
		{
			doPut(rset[i]);
		}
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return SUCCESS;
}

/// Insert a set of entries into this entry set.
int PGSortEntrySet::insertEntries(Transaction* txn, const EntrySet& rset)
{
	if(!m_state)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Not init sort entry set yet!");
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
		scan = FDPG_Heap::fd_heap_beginscan(relation,((PGTransaction*)txn)->getSnapshot(), 0, NULL);
		HeapTuple tuple = NULL;
		char *data = NULL;
		int len = 0;
		DataItem tmp_di;
		while(NULL != (tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)))
		{
			data = FDPG_Common::fd_tuple_to_chars_with_len(tuple, len);
			tmp_di.setData(data);
			tmp_di.setSize(len);
			doPut(tmp_di);
			FDPG_Memory::fd_pfree(data);
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
	return SUCCESS;
}

/// Insert an entry into the entry set.
int PGSortEntrySet::insertEntry(Transaction* txn, EntryID &eid, const DataItem& entry)
{
	/* EID was useless here. */
	if(!m_state) 
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Not init sort entry set yet!");
	}

	//HeapTuple tuple = NULL;
	try
	{
		doPut(entry);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return SUCCESS;
}

void PGSortEntrySet::performSort(CompareCallbacki pCompare)
{
	if(!m_state)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Not init sort entry set yet!");
	}
	
	try
	{
	    if(pCompare)
	        m_pCompare = pCompare;
		FDPG_TupleSort::fd_tuplesort_performsort(m_state, (void *)pCompare);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}
}

void PGSortEntrySet::setSorted()
{
    FDPG_TupleSort::fd_tuplesort_setsorted(m_state);
}

EntrySetScan* PGSortEntrySet::startEntrySetScan
	(Transaction* txn,
	SnapshotTypes opt,  
	const std::vector<ScanCondition>&scankeys, filter_func_t filterFunc, void*userData,const EntryID* eid, const EntryID* endId)
{
    Assert(! (filterFunc || userData));// the two parameters are only used in index scan.
	MemoryContext *cxt = txn->getAssociatedMemoryContext();
	return new(*cxt) PGSortEntrySetScan(this);
}

EntrySetScan* PGSortEntrySet::startEntrySetScan(Transaction* txn)
{
	MemoryContext *cxt = txn->getAssociatedMemoryContext();
	return new(*cxt) PGSortEntrySetScan(this);
}

void PGSortEntrySet::endEntrySetScan(EntrySetScan* scan)
{
	delete scan;
}

PGSortEntrySetScan::PGSortEntrySetScan(PGSortEntrySet *tmpEntry)
{
	m_pTupleStoreState = tmpEntry->m_state;
	m_sst = tmpEntry->m_sst;
	FDPG_TupleSort::fd_tuplesort_rescan(m_pTupleStoreState);
}

PGSortEntrySetScan::~PGSortEntrySetScan()
{
	m_pTupleStoreState = NULL;
}

int PGSortEntrySetScan::getNext(CursorMovementFlags opt, EntryID &eid, DataItem& entry)
{
	/* EID was useless here. */
	if(!m_pTupleStoreState)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Sort entry set scan not init correct.");
	}

	try
	{
		bool forward = (opt == NEXT_FLAG ? true : false);
		void *pData = NULL;
		HeapTuple tuple = NULL;
		bool shouldFree = false;
		int len = 0;
		switch(m_sst)
		{
			case EntrySet::SST_GenericDataTuples:
				FDPG_TupleSort::fd_tuplesort_getdata(m_pTupleStoreState, forward, &pData, &len);
				break;
			case EntrySet::SST_HeapTuples:
				FDPG_TupleSort::fd_tuplesort_getheaptuple(m_pTupleStoreState, forward, &tuple, &shouldFree);
				if(tuple)
				{
					pData = FDPG_Common::fd_tuple_to_chars_with_len(tuple, len);
					if(shouldFree)
					    FDPG_Memory::fd_pfree(tuple);
				}else
				{
					pData = NULL;
				}
				break;
			default:
				throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invaild store sort type!");
		}
		if(NULL == pData)
		{
			return NO_DATA_FOUND;
		}
		entry.setData(pData);
		entry.setSize(len);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return SUCCESS;
}

int PGSortEntrySetScan::markPosition()
{
	if(!m_pTupleStoreState)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Sort entry set scan not init correct.");
	}

	try
	{
		FDPG_TupleSort::fd_tuplesort_markpos(m_pTupleStoreState);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return SUCCESS;
}

int PGSortEntrySetScan::restorePosition()
{
	if(!m_pTupleStoreState)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "Sort entry set scan not init correct.");
	}

	try
	{
		FDPG_TupleSort::fd_tuplesort_restorepos(m_pTupleStoreState);
	}catch(StorageEngineExceptionUniversal &)
	{
		throw;
	}

	return SUCCESS;
}

int PGSortEntrySetScan::getNextBatch(CursorMovementFlags opt, std::vector<std::pair<EntryID, DataItem> > &eid_entries)
{
	/// Not support yet.
	return NO_DATA_FOUND;
}

}
}