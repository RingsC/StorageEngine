#include <string>
#include <vector>

/*#ifdef DEBUG
#include <iostream>
using std::cout;
using std::endl;
#endif*/ 


#include "postgres.h"

#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "access/xdb_common.h"
#include "utils/relcache.h"
#include "access/xact.h"
#include "access/skey.h"
#include "access/heapam.h"
#include "utils/snapmgr.h"
#include "access/relscan.h"
#include "access/tuptoaster.h"

#include "interface/PGEntrySet.h"
#include "interface/PGTransaction.h"
#include "interface/ErrNo.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "Transaction.h"
#include "interface/FDPGAdapter.h"
#include "interface/utils.h"


#include "interface/PGIndexEntrySet.h"

#include "interface/PGEntryIDBitmap.h"
#include "nodes/tidbitmap.h"


extern TupleDesc single_attibute_tupDesc;
extern int work_mem;
extern int bitmap_mem;

using std::vector;
using std::string;
namespace FounderXDB{
namespace StorageEngineNS {

#define THROW_CALL(FUNC,...)   \
		bool flag = false;\
		PG_TRY(); {\
		FUNC(__VA_ARGS__);\
	} PG_CATCH(); { \
		flag=true;	 \
	} PG_END_TRY(); \
		if (flag)\
	{\
		ThrowException(); \
	}
		
PGIndexEntrySet::PGIndexEntrySet(EntrySetID entry_set_id, EntrySetID index_entry_id, 
								 Relation heapRelation, Relation indexRelation)
                        : IndexEntrySet(index_entry_id),
						m_heap_entry_id(entry_set_id), 
						m_heapRelation(heapRelation), m_indexRelation(indexRelation)
{
    unsigned int type = MtInfo_GetType(m_indexRelation->mt_info);
	switch (type) {
	case BTREE_UNIQUE_TYPE:
		_entryset_type_ = UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE; break;
	case BTREE_TYPE:
		_entryset_type_ = BTREE_INDEX_ENTRY_SET_TYPE; break;
	case BTREE_UNIQUE_CLUSTER_TYPE:
	    _entryset_type_ = CLUSTER_UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE; break;
	case BTREE_CLUSTER_TYPE:
	    _entryset_type_ = CLUSTER_BTREE_INDEX_ENTRY_SET_TYPE; break;
	default:
		Assert(false); break;
	}
}

PGIndexEntrySet::~PGIndexEntrySet()
{
    
}

se_uint64 PGIndexEntrySet::getNumberOfPages()
{
	se_uint64 num = 0;
	if (NULL == m_indexRelation || NULL == m_heapRelation) {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open index first please");
	}

	num = FDPG_StorageEngine::getNumberOfPages(m_indexRelation);

	return num;
}

void PGIndexEntrySet::getNumsOfKey(Transaction* txn,std::vector<ScanCondition> &vscans, const ScanCondition::CompareOperation &comp, se_uint64 &nums)
{
	///init scan ?
	if (NULL == m_indexRelation || NULL == m_heapRelation) {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open index first please");
	}


	for (size_t i = 0; i < vscans.size(); i++) {
		ScanCondition cond = vscans[i];
		cond.compop = comp;
		vscans[i] = cond;
	}

	EntrySetScan *pscan = NULL;
    pscan = startEntrySetScan(txn, BaseEntrySet::SnapshotMVCC, vscans);

	
    EntryID eid_find = {0};
    DataItem entry;
    int ret = NO_DATA_FOUND;

	try {
		do {
			ret = pscan->getNext(EntrySetScan::NEXT_FLAG, eid_find, entry);
			
			if (ret != NO_DATA_FOUND) {
				++nums;
			}
		} while (ret != NO_DATA_FOUND);

	} catch (StorageEngineException &) {
		endEntrySetScan(pscan);
		throw;
	}

    endEntrySetScan(pscan);
	return;
}

void PGIndexEntrySet::getKeyRange(Transaction* txn,std::vector<ScanCondition> &vscans, KeyRange &range)
{	
	getNumsOfKey(txn,vscans, ScanCondition::LessThan, range.nLess);
	getNumsOfKey(txn,vscans, ScanCondition::Equal, range.nEqual);
	getNumsOfKey(txn,vscans, ScanCondition::GreaterThan, range.nGreater);
}

void PGIndexEntrySet::getEntrySetKeyRange(Transaction* txn, std::string& minKey, std::string& maxKey)
{
	FDPG_Index::fd_index_getrange(m_heapRelation, m_indexRelation, minKey, maxKey, ((PGTransaction*)txn)->getSnapshot());
}

EntrySetScan* PGIndexEntrySet::startEntrySetScan(Transaction* txn,
         SnapshotTypes opt,  
         const std::vector<ScanCondition> &scankeys, filter_func_t filterFunc, void*userData,const EntryID* eid, const EntryID* endid)
{

	if (NULL == m_indexRelation  || (_entryset_type_ != CLUSTER_BTREE_INDEX_ENTRY_SET_TYPE && _entryset_type_ != CLUSTER_UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE && NULL == m_heapRelation)) {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open heap and index first please");
	}
	Assert (!(filterFunc == 0 && userData != 0));// this case is not allowed.
	
    int nkeys = (int)scankeys.size();
    int len = 0;
	Datum  values = 0;
    Snapshot snapshot = NULL;
    
    switch (opt) {
    case BaseEntrySet::SnapshotMVCC: snapshot = ((PGTransaction*)txn)->getSnapshot(); break;
    case BaseEntrySet::SnapshotDIRTY: snapshot = SnapshotDirtyPtr; break;
    case BaseEntrySet::SnapshotNOW: snapshot = SnapshotNow; break;
    default:
        assert(false);
        break;
    }

    ScanKey build_key = NULL;
	if (nkeys > 0)
	{
		build_key = (ScanKey)FDPG_Memory::fd_palloc0(sizeof(ScanKeyData) * nkeys);
	}    
    std::vector<ScanCondition>::const_iterator iter;
    IndexScanDesc tuple_scan = NULL;
    size_t pos;
    char *p = 0;
    try {
        for (iter = scankeys.begin(), pos = 0; pos < (size_t)nkeys; ++pos, ++iter) {
            len = iter->arg_length;
            p = DatumGetCString(iter->argument);
			values = FDPG_Common::fd_string_formdatum(p, len);
            FDPG_ScanKey::fd_ScanKeyInitWithCallbackInfo(&build_key[pos], (AttrNumber)iter->fieldno, (StrategyNumber)iter->compop, (CompareCallback)iter->compare_func, values);
        }

        tuple_scan = FDPG_Index::fd_index_beginscan(m_heapRelation, m_indexRelation, snapshot, nkeys, build_key, filterFunc, userData);
        FDPG_Index::fd_index_rescan(tuple_scan, build_key, nkeys, NULL, 0);

    } catch (StorageEngineExceptionUniversal&) {
		if (build_key != NULL)
		{
			FDPG_Memory::fd_pfree(build_key);
		}        
        throw;
    }

	if (build_key != NULL)
	{
		FDPG_Memory::fd_pfree(build_key);
	}    
	MemoryContext *cxt = txn->getAssociatedMemoryContext();
    PGIndexEntrySetScan *index_entry_set_scan = new(*cxt) PGIndexEntrySetScan(scankeys, tuple_scan);  //throw std::bad_alloc

    return index_entry_set_scan;
}




void PGIndexEntrySet::endEntrySetScan(EntrySetScan* scan)
{
    if (NULL == scan) return ;

    PGIndexEntrySetScan *index_scan = (PGIndexEntrySetScan*)scan;

	FDPG_Index::fd_index_endscan(index_scan->getIndexScanDesc());

    delete index_scan;
    index_scan = NULL;
}

int PGIndexEntrySet::insertEntry(Transaction* txn, const DataItem& index_key, const DataItem &index_data, const EntryID* eid )
{
    // TODO: when needed, we can do toasting to the index key and/or index data, by toasting them into a temp heap table,
    // then use the toast pointers as 'index_key' and/or 'index_data' to insert into this index.
    
	if (NULL == m_indexRelation) {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open heap and index first please");
	}

    IndexUniqueCheck unique = UNIQUE_CHECK_NO;
    if (getType() == UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE || getType() == CLUSTER_UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE) {
        unique = UNIQUE_CHECK_YES;
    }

	bool		isnull[2];
	isnull[0] = isnull[1] = false;
	Datum   values[2];
	void *keyarr, *dataarr = NULL;
	ItemPointerData tid;
	
	keyarr = FDPG_Memory::fd_palloc0(index_key.getSize() + VARHDRSZ);
	if(eid == 0)
	dataarr = FDPG_Memory::fd_palloc0(index_data.getSize() + VARHDRSZ);
	memcpy(VARDATA(keyarr), index_key.getData(), index_key.getSize());
	SET_VARSIZE(keyarr, index_key.getSize() + VARHDRSZ);
	if(eid == 0)
	{
		memcpy(VARDATA(dataarr), index_data.getData(), index_data.getSize());
		SET_VARSIZE(dataarr, index_data.getSize() + VARHDRSZ);
	}
	else
	{
		pentryIdToTid(eid, tid);
	}
	values[0] = PointerGetDatum(keyarr);
	if(eid == 0)
	values[1] = PointerGetDatum(dataarr);

    FDPG_Index::fd_index_insert(m_indexRelation, /* index relation */
        values,	/* array of index Datums */
        isnull,	/* null flags */
		(eid == 0)? 0:&tid,		/* tid of heap tuple */
		(eid == 0)? 0:m_heapRelation,	/* heap relation */
        unique!=UNIQUE_CHECK_NO);	/* type of uniqueness check to do */

    FDPG_Memory::fd_pfree(keyarr);
    if(eid == 0)
    FDPG_Memory::fd_pfree(dataarr);

    return 0;

}

//index scan
PGIndexEntrySetScan::PGIndexEntrySetScan(const std::vector<ScanCondition>& scankeys, IndexScanDesc index_scan) : m_index_scan_desc(index_scan)
{
    m_scankeys = scankeys;
    m_curentry = 0;
    entrysz_ = 0;
}

PGIndexEntrySetScan::~PGIndexEntrySetScan()
{
    m_index_scan_desc = NULL;
    if (m_curentry)
        FDPG_Memory::fd_pfree(m_curentry);
}

int PGIndexEntrySetScan::getNext(CursorMovementFlags opt, EntryID &eid)
{
    throw StorageEngineExceptionUniversal(FORBID_USE_ERR, "forbid use now");
}

int PGIndexEntrySetScan::getNext(CursorMovementFlags opt, EntryID &eid, DataItem& entry)
{

	ScanDirection direction = BackwardScanDirection;

    switch (opt) {
        case NEXT_FLAG:
            direction = ForwardScanDirection;
            break;
        case PREV_FLAG:
            direction = BackwardScanDirection;
            break;
        case CURRENT_FLAG:
            direction = NoMovementScanDirection;
            break;
        default:
            throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid CursorMovementFlags");
    }

    HeapTuple tuple = NULL;
    int len = 0;
    tuple = FDPG_Index::fd_index_getnext(m_index_scan_desc, direction);
    if (tuple != NULL) {
        TidToEntryId(tuple->t_self, eid);
        char *pdata = NULL;
        pdata = FDPG_Common::fd_tuple_to_chars_with_len(tuple, len);

        entry.setSize(len);
        entry.setData((void *)pdata);
        return 0;
    } else if (IS_CLUSTER_INDEX(this->m_index_scan_desc->indexRelation->mt_info.type)) {
        int totalLen = 0;
        if (m_curentry == 0) {
            entrysz_ = 1024;
            m_curentry = (char *)FDPG_Memory::fd_palloc0(entrysz_);// for now this is big enough to store cluster index key-value entry.
        }
        if (entrysz_ < m_index_scan_desc->curdi_len) {
            entrysz_ += m_index_scan_desc->curdi_len;
            m_curentry = (char *)FDPG_Memory::fd_repalloc(m_curentry, entrysz_);
        }
        Assert(entrysz_ >= m_index_scan_desc->curdi_len);
        if (m_index_scan_desc->current_dataitem == 0)
            return NO_DATA_FOUND;
        const char *p = m_index_scan_desc->current_dataitem;
        size_t keylen = len = VARSIZE_ANY_EXHDR(p);
        memcpy(m_curentry, VARDATA_ANY(p), len);
        totalLen = len;
        
        p = m_index_scan_desc->current_dataitem + VARSIZE_ANY(m_index_scan_desc->current_dataitem);
        char *q = m_curentry + len;
        bool freep = false;
        if (VARATT_IS_COMPRESSED(p)) {
            p = (const char *)FDPG_Heap::fd_toast_uncompress_datum((varlena *)p);// uncompress it
            freep = true;
        }
        len = VARSIZE_ANY_EXHDR(p);
        totalLen += len;
        if (totalLen > (int)entrysz_) {// uncompressed data may need more memory.
            entrysz_ += totalLen;
            m_curentry = (char *)FDPG_Memory::fd_repalloc(m_curentry, entrysz_);
            q = m_curentry + keylen;
        }
        memcpy(q, VARDATA_ANY(p), len);
        
        entry.setData(m_curentry);
        entry.setSize(totalLen);
        if (freep)
            FDPG_Memory::fd_pfree((void *)p);
        return 0;
    }
    return NO_DATA_FOUND;
}

int PGIndexEntrySetScan::markPosition()
{
    FDPG_Index::fd_index_markpos(m_index_scan_desc);
    return 0;
}

int PGIndexEntrySetScan::restorePosition()
{
    FDPG_Index::fd_index_restrpos(m_index_scan_desc);
    return 0;
}
#if !defined(_MSC_VER)
#pragma GCC diagnostic ignored "-Wclobbered"
#endif

EntryIDBitmap*PGIndexEntrySetScan::getBitmap(Transaction* txn)
{
	TIDBitmap* pBitmap = NULL;
	int64 num;
	THROW_CALL(pBitmap = tbm_create, bitmap_mem * 1024L)
	try
	{
		if (NULL != m_index_scan_desc)
		{
			THROW_CALL(num = index_getbitmap,m_index_scan_desc, pBitmap);
		}
	}
	catch (StorageEngineExceptionUniversal &) 
	{
        //FDPG_Index::fd_index_endscan(m_heap_scan_desc);
        throw;
    }
	//FDPG_Index::fd_index_endscan(m_heap_scan_desc);
	MemoryContext* cxt = txn->getAssociatedMemoryContext();
	PGEntryIDBitmap* pEnBitmap = new(*cxt) PGEntryIDBitmap(pBitmap,txn,m_index_scan_desc);////∑≈µΩtxn memcxt÷–
	return pEnBitmap;
}
#if !defined(_MSC_VER)
#pragma GCC diagnostic warning "-Wclobbered"
#endif

void PGIndexEntrySetScan::deleteBitmap(EntryIDBitmap * pbitmap)
{
	if (NULL == pbitmap)
		return;
	delete pbitmap;
	pbitmap = NULL;
}

int PGIndexEntrySetScan::getNextBatch(CursorMovementFlags opt, std::vector<EntryID>& eids)
{
    throw StorageEngineExceptionUniversal(FORBID_USE_ERR, "forbid use now");
}

int PGIndexEntrySetScan::getNextBatch(CursorMovementFlags opt, std::vector<std::pair<EntryID, DataItem> > &eid_entries)
{
	//if (NULL == m_indexRelation  || NULL == m_heapRelation) {
	//	ThrowException(LOGIC_ERR, "open heap and index first please");
	//}

    //todo wangfh@founder.com  100 records
    ScanDirection direction = BackwardScanDirection;

    switch (opt) {
        case NEXT_FLAG:
            direction = ForwardScanDirection;
            break;
        case PREV_FLAG:
            direction = BackwardScanDirection;
            break;
        default:
            direction = NoMovementScanDirection;
    }
    
    HeapTuple tuple  = NULL;
    EntryID eid = {0};
    //int cblock = InvalidBlockNumber;
    //int get_cblock = InvalidBlockNumber;
    //DataItem entry;

    int num = 100;
	int ret = NO_DATA_FOUND;
    while ((tuple = FDPG_Index::fd_index_getnext(m_index_scan_desc, direction)) != NULL)
    {   
		ret = 0;
        //get_cblock = m_heap_scan_desc->rs_cblock;
        if (0 == num) break;

        TidToEntryId(tuple->t_self, eid);

        int len = 0;
        //char *pdata = fdxdb_tuple_to_chars_with_len(tuple, len);
		char *pdata = NULL;
        pdata = FDPG_Common::fd_tuple_to_chars_with_len(tuple, len);
		//char *pmemFree = NULL;
		//GetHeapTupleDataSize(tuple, pdata, len, pmemFree);

        DataItem entry((void *)pdata, len);

        eid_entries.push_back(std::pair<EntryID, DataItem>(eid,entry));

        --num;
    }
    return ret;

}

} // namespace StorageEngineNS
} //namespace FounderXDB
