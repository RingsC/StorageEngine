#include <string>
#include <vector>
#include <list>

#include "postgres.h"

#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "access/xdb_common.h"
#include "utils/relcache.h"
#include "access/xact.h"
#include "access/skey.h"
#include "utils/snapmgr.h"
#include "access/tuptoaster.h"

#include "interface/PGEntrySet.h"
#include "interface/ErrNo.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "Transaction.h"
#include "interface/FDPGAdapter.h"
#include "interface/utils.h"
#include "access/relscan.h"
#include "interface/PGDump.h"
#include "StorageEngine.h"
#include "interface/PGTransaction.h"


extern TupleDesc single_attibute_tupDesc;

using std::string;

namespace FS = FounderXDB::StorageEngineNS;

static
bool cmp_pair_eids(const std::pair<FS::EntryID, FS::DataItem>  &arg1,
						 const std::pair<FS::EntryID, FS::DataItem>  &arg2)
{
	return arg1.first < arg2.first;
}

static
bool cmp_eids(const FS::EntryID &arg1,
							const FS::EntryID &arg2)
{
	return arg1 < arg2;
}

namespace FounderXDB{
namespace StorageEngineNS {
	//如果一个宏有多个语句，那么必须用do while把这个宏括起来，否则使用的时候会发生错误，在条件分支中只有宏的第一条语句被执行。
#define THROW_CALL(FUNC,...)   do {\
		bool flag = false;\
		PG_TRY(); {\
		FUNC(__VA_ARGS__);\
	} PG_CATCH(); { \
		flag=true;	 \
	} PG_END_TRY(); \
		if (flag)\
	{\
		ThrowException(); \
	}   \
} while (0)

ColumnInfo::~ColumnInfo(){
	if(flag) {
		if (col_number) {
		    FDPG_Memory::fd_pfree(col_number);
			col_number =NULL;
		}
		if (rd_comfunction) {
			FDPG_Memory::fd_pfree(rd_comfunction);
			rd_comfunction = NULL;
		}
	} //if(flag)
}

PGIndinfoData::~PGIndinfoData(){
	for (unsigned int i = 0 ; i< this->index_num; i ++) {
		FDPG_Memory::fd_pfree(index_info[i].col_number);
		FDPG_Memory::fd_pfree(index_info[i].rd_comfunction);
	}
	if (index_array != NULL) {
		FDPG_Memory::fd_pfree(index_array);
		index_array = NULL;
	}
	if (index_info != NULL) {
		FDPG_Memory::fd_pfree(index_info);
		index_info =NULL;
	}	
}

PGEntrySet::PGEntrySet(EntrySetID entry_set_id, Relation heapRelation)
    : EntrySet(entry_set_id, HEAP_ROW_ENTRY_SET_TYPE), m_heapRelation(heapRelation)
{

}

se_uint64 PGEntrySet::getNumberOfPages()
{
	se_uint64 num = 0;
	if (m_heapRelation && m_heapRelation->rd_smgr){

		num = FDPG_StorageEngine::getNumberOfPages(m_heapRelation);
	} else {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please");
	}
	return num;
}


PGEntrySet::~PGEntrySet()
{

}

int PGEntrySet::truncate()
{
	if (!m_heapRelation){
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please");
	}

	try{
		FDPG_Heap::fd_heap_truncate(m_heapRelation);
	}catch (StorageEngineExceptionUniversal &){
		throw;
	}

	return 0;
}

int PGEntrySet::dump(const char *dir,Transaction *pTrans)
{
	//TransactionId xid = pTrans->getTransactionId();
	//TransactionId xid = 0;
	//Transaction *pTrans =  StorageEngine::getStorageEngine()->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	std::vector<ScanCondition> vConditions;
	EntrySetScan *pScan = startEntrySetScan(pTrans,SnapshotNOW,vConditions, 0, 0);//这里应该使用SnapshotNow，否则可能会有commit的事务的改动不能dump。

	DataItem outData;
	EntryID outId;
	DumpSaver dumpSaver(dir,getId(),pTrans->getTransactionId());
	while (0 == pScan->getNext(EntrySetScan::NEXT_FLAG,outId,outData))
	{
		dumpSaver.WriteTuple(outData.getData(),(int)outData.getSize());
	}

	endEntrySetScan(pScan);

    return 0;
}
int PGEntrySet::load(const char *dumpFilePath,Transaction *pTrans)
{
	//TransactionId xid = pTrans->getTransactionId();
	//TransactionId xid = 0;
	//Transaction *pTrans = StorageEngine::getStorageEngine()->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	//HeapTuple tup = NULL;
	BulkInsertState bis = FDPG_Heap::fd_GetBulkInsertState();
	try 
	{
		DumpLoader dumpFile(dumpFilePath,getId());
		for (DumpLoader::Iterator dataItem = dumpFile.Begin(); dataItem != dumpFile.End(); dataItem = dumpFile.Next())
		{
			const DataItem& pdata_item = *dataItem;
			char* pdata = (char *)pdata_item.getData();
			unsigned int len = (unsigned int)pdata_item.getSize();
			uint32 nEids;
			EntrySetID *pEids = pdata_item.getEids(nEids);
			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(pdata, len, nEids, pEids);
			FDPG_Heap::fd_heap_insert_bulk(m_heapRelation, tup, bis);
		}
	
	} catch (StorageEngineExceptionUniversal &) 
	{
		FDPG_Heap::fd_FreeBulkInsertState(bis);
		throw;
	}
	FDPG_Heap::fd_FreeBulkInsertState(bis);   

    return 0;
}

//bulk_insert will be replaced by multi_insert
/*
int PGEntrySet::insertEntries(Transaction* txn, const std::vector<DataItem>& rset)
{
	if (!m_heapRelation) {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please"); 
	}
	/// open relation
	
    char *pdata = NULL;
    unsigned int len = 0;
    HeapTuple tup = NULL;
    BulkInsertState bis = FDPG_Heap::fd_GetBulkInsertState();
    try {
        for (int i = 0; i < rset.size(); i++) {
            const DataItem& pdata_item = rset[i];
            pdata = (char *)pdata_item.getData();
            len = pdata_item.getSize();
            HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(pdata, len);			
            FDPG_Heap::fd_heap_insert_bulk(m_heapRelation, tup, bis);
			FDPG_Memory::fd_pfree(tup); 
        } //for
    } catch (StorageEngineExceptionUniversal &) {
        FDPG_Heap::fd_FreeBulkInsertState(bis);  
        throw;
    }
    FDPG_Heap::fd_FreeBulkInsertState(bis);  
    return 0;
}

int PGEntrySet::insertEntries(Transaction* txn, const EntrySet& rset)
{

    EntrySetID relid = rset.getId();

	PGEntrySet *tmp = (PGEntrySet *)(&rset);
	Relation testRelation = tmp->getRelation();

	if (!m_heapRelation || !testRelation) {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please"); 
	}

    HeapScanDesc scan = NULL;
    BulkInsertState bis = FDPG_Heap::fd_GetBulkInsertState();
    try {
        scan = FDPG_Heap::fd_heap_beginscan(testRelation, FDPG_Transaction::fd_GetTransactionSnapshot(), 0, NULL);

        HeapTuple tuple;
        while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
        {   
            FDPG_Heap::fd_heap_insert_bulk(m_heapRelation, tuple, bis);
        }
    } catch (StorageEngineExceptionUniversal &) {
            FDPG_Heap::fd_FreeBulkInsertState(bis);
            FDPG_Heap::fd_heap_endscan(scan);
            throw;
    }
    FDPG_Heap::fd_FreeBulkInsertState(bis);
    FDPG_Heap::fd_heap_endscan(scan);

    return 0;
}
*/
int PGEntrySet::insertEntries(Transaction* txn, const EntrySet& rset)
{
	PGEntrySet *tmp = (PGEntrySet *)(&rset);
	Relation testRelation = tmp->getRelation();

	if (!m_heapRelation || !testRelation) {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please"); 
	}

	HeapScanDesc scan = NULL;
	BulkInsertState bis = NULL;
	HeapTuple* tup = NULL;

	try 
	{
		//fill tuple vector
		std::vector<HeapTupleData> vTup;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, ((PGTransaction*)txn)->getSnapshot(), 0, NULL);
		HeapTuple tuple;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			vTup.push_back(*tuple);
		}
		tup = (HeapTuple*)FDPG_Memory::fd_palloc0(vTup.size() * sizeof(HeapTuple));
		for (unsigned int i = 0; i < vTup.size(); i ++)
			tup[i] = &vTup[i];

		//multi insert tuples
		bis=FDPG_Heap::fd_GetBulkInsertState();
		FDPG_Heap::fd_heap_multi_insert(m_heapRelation, tup, (int)vTup.size(),bis);
		FDPG_Heap::fd_FreeBulkInsertState(bis);
	}
	catch (StorageEngineExceptionUniversal &)
	{
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Memory::fd_pfree(tup);
		throw;
	}
	FDPG_Heap::fd_heap_endscan(scan);
	FDPG_Memory::fd_pfree(tup);

	return 0;
}

int PGEntrySet::insertEntries(Transaction* txn, const std::vector<DataItem>& rset)
{
	if (!m_heapRelation) {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please"); 
	}

	BulkInsertState bis = NULL;
	HeapTuple* tup = NULL;
	try
	{
		char *pdata = NULL;
		unsigned int len = 0;
		const unsigned int count = (const unsigned int)rset.size();
		tup = (HeapTuple*)FDPG_Memory::fd_palloc0(count * sizeof(HeapTuple));

		//fill tup array by items
		for (unsigned int i = 0; i < count; i++)
		{
			const DataItem& pdata_item = rset[i];
			pdata = (char *)pdata_item.getData();
			len = (unsigned int)pdata_item.getSize();
			uint32 nEids;
			EntrySetID *pEids = pdata_item.getEids(nEids);
			tup[i] = FDPG_Heap::fd_heap_form_tuple(pdata, len, nEids, pEids);
		}

		bis=FDPG_Heap::fd_GetBulkInsertState();
		FDPG_Heap::fd_heap_multi_insert(m_heapRelation, tup, count, bis);
		FDPG_Heap::fd_FreeBulkInsertState(bis);
		for (unsigned int i = 0; i < count; i ++)
			FDPG_Memory::fd_pfree(tup[i]);
	}
	catch (StorageEngineException &)
	{
		FDPG_Memory::fd_pfree(tup);

		throw;
	}

	FDPG_Memory::fd_pfree(tup);

	return 0;	
}

    /// Insert an entry into the entry set.
    /// @param[in]  txn     transaction
    /// @param[out] eid		entry ID of inserted entry
    /// @param[in]  entry   entry value
    /// @param[in]  bIndex  insert index tuple or not
    /// @return    value yet to be defined
int PGEntrySet::insertEntry(Transaction* txn, EntryID &eid, const DataItem& entry)
{

	if (!m_heapRelation){
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please");
	}
    ///form heaptuple
    char *pdata = (char *)entry.getData();
    unsigned int len = (unsigned int)entry.getSize();
	uint32 nEids;
	EntrySetID *pEids = entry.getEids(nEids);
    HeapTuple tup = NULL;
    try {
        tup = FDPG_Heap::fd_heap_form_tuple(pdata, len, nEids, pEids);
        FDPG_Heap::fd_simple_heap_insert(m_heapRelation, tup);
    } catch (StorageEngineExceptionUniversal &) {
		if(tup)
			FDPG_Memory::fd_pfree(tup);

        throw;
    }

       TidToEntryId(tup->t_self, eid);
       FDPG_Memory::fd_pfree(tup);

       return 0;
}

bool PGEntrySet::getIndexInfo(Transaction* txn,PGIndinfoData& indinfo)
{

	if (NULL == m_heapRelation || 0 == (MtInfo_GetIndexCount(m_heapRelation->mt_info)) ) {
		return false;
	}

	indinfo.index_num = MtInfo_GetIndexCount(m_heapRelation->mt_info);

    MemoryContext *cxt = txn->getAssociatedMemoryContext();
    unsigned int *pindex_array = (unsigned int *)cxt->alloc(indinfo.index_num * sizeof(unsigned int));
	memcpy(pindex_array, MtInfo_GetIndexOid(m_heapRelation->mt_info),indinfo.index_num * sizeof(*pindex_array));
	indinfo.index_array = pindex_array;

    ColumnInfo *pindex_info = (ColumnInfo *)cxt->alloc(indinfo.index_num * sizeof(ColumnInfo));
	ColumnInfo *tmp = NULL;
	for (unsigned int i = 0 ; i< indinfo.index_num; i ++) {
		tmp = (ColumnInfo *)(*((MtInfo_GetIndexColinfo(m_heapRelation->mt_info) ) + i));
		pindex_info[i].col_number = (size_t*)cxt->alloc(tmp->keys * sizeof(size_t));
		memcpy((pindex_info[i].col_number), tmp->col_number, tmp->keys * sizeof(size_t));
		pindex_info[i].rd_comfunction = (CompareCallbacki*)cxt->alloc(tmp->keys * sizeof(CompareCallbacki));
		memcpy((pindex_info[i].rd_comfunction), tmp->rd_comfunction, tmp->keys * sizeof(CompareCallbacki));
		pindex_info[i].split_function = tmp->split_function;
		pindex_info[i].keys = tmp->keys;
	}
	indinfo.index_info = pindex_info;
	return true;
}


int PGEntrySet::deleteEntry(Transaction* txn, const EntryID &eid)
{
	if (!m_heapRelation){
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please");
	}
	ItemPointerData tid = {{0,0},0};
    entryIdToTid(eid, tid);
 
    FDPG_Heap::fd_simple_heap_delete(m_heapRelation, &tid);

    return 0;
}

int PGEntrySet::deleteEntries(Transaction*, std::list<EntryID> &eids)
{
	if (!m_heapRelation){
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please");
	}

	eids.sort(cmp_eids);

	ItemPointerData tid = {{0, 0}, 0};

	std::list<EntryID>::iterator it = eids.begin();

	for(; it != eids.end(); ++it)
	{
		entryIdToTid(*it, tid);
		FDPG_Heap::fd_simple_heap_delete(m_heapRelation, &tid);
	}

	return SUCCESS;
}

int PGEntrySet::updateEntry(Transaction* txn, const EntryID& eid, const DataItem& entry, bool isInplaceUpdate)
{
	if (!m_heapRelation){
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please");
	}

    // TODO: do inplace update if isInplaceUpdate is true, see InplaceUpdateEntry member function to do so.
    
    char *pdata = (char *)entry.getData();
    unsigned int len = (unsigned int)entry.getSize();
	uint32 nEids;
	EntrySetID *pEids = entry.getEids(nEids);

    HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(pdata, len, nEids, pEids);
	//uint32 newLen = 0;
	//HeapTuple tup = FDPG_Heap::heap_tuple_copy_bytearray_straight(pdata, len, &newLen);
    ItemPointerData tid;
    entryIdToTid(eid, tid);
    if(isInplaceUpdate)
	{
		tup->t_self = tid;
		FDPG_Heap::fd_heap_inplace_update(m_heapRelation, tup);
    } else
        FDPG_Heap::fd_simple_heap_update(m_heapRelation, &tid, tup);
        
    //insert_index(m_heapRelation, tup);
    FDPG_Memory::fd_pfree(tup);

    return 0;
}

int PGEntrySet::updateEntries(Transaction* txn, 
															std::list<std::pair<EntryID, DataItem> >& eids,
															bool isInplaceUpdate)
{
	if (!m_heapRelation){
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please");
	}

	eids.sort(cmp_pair_eids);

	char *data = NULL;
	unsigned int len = 0;
	HeapTuple tuple = NULL;
	ItemPointerData tid = {{0, 0}, 0};
	std::list<std::pair<EntryID, DataItem> >::iterator it = eids.begin();

	for(; it != eids.end(); ++it)
	{
		data = (char*)it->second.getData();
		len = (unsigned int)it->second.getSize();
		uint32 nEids;
		EntrySetID *pEids = it->second.getEids(nEids);
		tuple = FDPG_Heap::fd_heap_form_tuple(data, len, nEids, pEids);
		entryIdToTid(it->first, tid);

		if(isInplaceUpdate)
		{
			tuple->t_self = tid;
			FDPG_Heap::fd_heap_inplace_update(m_heapRelation, tuple);
		} else {
			FDPG_Heap::fd_simple_heap_update(m_heapRelation, &tid, tuple);
		}
		FDPG_Memory::fd_pfree (tuple);
	}

	return SUCCESS;
}

int PGEntrySet::InplaceUpdateEntry(const EntryID& eid, const DataItem& entry)
{
	if (!m_heapRelation){
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please");
	}
	char *pdata = (char *)entry.getData();
    unsigned int len = (unsigned int)entry.getSize();
 
    HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(pdata, len);
	//uint32 newLen = 0;
	//HeapTuple tup = FDPG_Heap::heap_tuple_copy_bytearray_straight(pdata, len, &newLen);
    ItemPointerData tid;
    entryIdToTid(eid, tid);
	tup->t_self = tid;
	FDPG_Heap::fd_heap_inplace_update(m_heapRelation, tup);
        
    //insert_index(m_heapRelation, tup);
    FDPG_Memory::fd_pfree(tup);

	return 0;
}


EntrySetScan* PGEntrySet::startEntrySetScan
        (Transaction* txn,
         SnapshotTypes opt,  
         const std::vector<ScanCondition>&scankeys, filter_func_t filterFunc, void*userData,const EntryID* eid, const EntryID* endid)
{
	if (!m_heapRelation) {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "open relation first,please"); 
	}
    Assert(! (filterFunc == 0 && userData));// this case is forbidden

    int nkeys = (int)scankeys.size();
    int len = 0;
    Datum values = 0;

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
	if(nkeys)
    build_key = (ScanKey)FDPG_Memory::fd_palloc0(sizeof(ScanKeyData) * nkeys);
    
	std::vector<ScanCondition>::const_iterator iter;
	size_t pos;
	char *p = 0;
    for (iter = scankeys.begin(), pos = 0; pos < (size_t)nkeys; ++pos, ++iter) {
        len = iter->arg_length;
        p = DatumGetCString(iter->argument);

        //values[0] = fdxdb_string_formdatum(p, len);
		values = FDPG_Common::fd_string_formdatum(p, len);

        FDPG_ScanKey::fd_ScanKeyInitWithCallbackInfo(&build_key[pos], (AttrNumber)iter->fieldno, (StrategyNumber)iter->compop, (CompareCallback)iter->compare_func, values);

    }
	HeapScanDesc tuple_scan =0;
	if(eid&&endid)
	{
		ItemPointerData tid;
		ItemPointerData endtid;
	    pentryIdToTid(eid, tid);
		pentryIdToTid(endid, endtid);		
		tuple_scan = FDPG_Heap::fd_heap_beginscan(m_heapRelation, snapshot, nkeys, build_key,0,0,&tid, &endtid);
	}
	else 
		tuple_scan = FDPG_Heap::fd_heap_beginscan(m_heapRelation, snapshot, nkeys, build_key);
	if(build_key)
    FDPG_Memory::fd_pfree(build_key);
	MemoryContext *cxt = txn->getAssociatedMemoryContext();
    HeapEntrySetScan *heap_entry_set_scan = new(*cxt) HeapEntrySetScan(scankeys, tuple_scan);

    return heap_entry_set_scan;
}

void PGEntrySet::endEntrySetScan(EntrySetScan* scan)
{
    if (NULL == scan) return ;

    HeapEntrySetScan *heap_scan = (HeapEntrySetScan*)scan;
    
    //Relation relation = heap_scan->getRaletion();
    FDPG_Heap::fd_heap_endscan(heap_scan->getHeapScanDesc());
    delete heap_scan;
    heap_scan = NULL;
}

//heap scan
HeapEntrySetScan::HeapEntrySetScan(const std::vector<ScanCondition>& scankeys, HeapScanDesc heap_scan_desc) : m_heap_scan_desc(heap_scan_desc)
{
    m_scankeys = scankeys;
}


HeapEntrySetScan::~HeapEntrySetScan()
{
    m_heap_scan_desc = NULL;
}

int HeapEntrySetScan::getNext(CursorMovementFlags opt, EntryID &eid, DataItem& entry)
{
    ScanDirection direction = BackwardScanDirection;

    switch (opt) {
        case EntrySetScan::NEXT_FLAG:
            direction = ForwardScanDirection;
            break;
        case EntrySetScan::PREV_FLAG:
            direction = BackwardScanDirection;
            break;
        case EntrySetScan::CURRENT_FLAG:
            direction = NoMovementScanDirection;
            break;
        default:
            throw StorageEngineExceptionUniversal(INVALIDE_ARG, "Invalid CursorMovementFlags");
    }


    HeapTuple tuple = NULL;
    tuple = FDPG_Heap::fd_heap_getnext(m_heap_scan_desc, direction);
    if (tuple != NULL) {
        TidToEntryId(tuple->t_self, eid);

        int len = 0;
        char *pdata = NULL;
        pdata = FDPG_Common::fd_tuple_to_chars_with_len(tuple, len);
		//char *pmemFree = NULL;
		//GetHeapTupleDataSize(tuple, pdata, len, pmemFree);

        entry.setSize(len);
        entry.setData((void *)pdata);

        return 0;
    } //while
    return NO_DATA_FOUND;
}


//get batch data of one page
int HeapEntrySetScan::getNextBatch(CursorMovementFlags opt, std::vector<std::pair<EntryID, DataItem> > &eid_entries)
{

    ScanDirection direction = BackwardScanDirection;

    switch (opt) {
        case EntrySetScan::NEXT_FLAG:
            direction = ForwardScanDirection;
            break;
        case EntrySetScan::PREV_FLAG:
            direction = BackwardScanDirection;
            break;
        case EntrySetScan::CURRENT_FLAG:
            direction = NoMovementScanDirection;
            break;
        default:
            direction = NoMovementScanDirection;
    }
    
    HeapTuple tuple  = NULL;
    EntryID eid = {0};
    DataItem entry;
    int cblock = InvalidBlockNumber;
    int get_cblock = InvalidBlockNumber;
    
    while ((tuple = FDPG_Heap::fd_heap_getnext(m_heap_scan_desc, direction)) != NULL)
    {   
        get_cblock = m_heap_scan_desc->rs_cblock;

        if ((cblock == get_cblock) || (cblock == (int)InvalidBlockNumber)) {
            cblock = get_cblock;
            TidToEntryId(tuple->t_self, eid);

             int len = 0;
             char *pdata = FDPG_Common::fd_tuple_to_chars_with_len(tuple, len);
			 //char *pdata = NULL;
			 //char *pmemFree = NULL;
			 //GetHeapTupleDataSize(tuple, pdata, len, pmemFree);

			entry.setData(pdata);
			entry.setSize(len);
            eid_entries.push_back(std::pair<EntryID, DataItem>(eid,entry));
        } else {
            /// get the data come from another page
            break;
        }
    }
    return 0;
}
int HeapEntrySetScan::markPosition()
{
    FDPG_Heap::fd_heap_markpos(m_heap_scan_desc);
    return 0;
}

int HeapEntrySetScan::restorePosition()
{
    FDPG_Heap::fd_heap_restrpos(m_heap_scan_desc);
    return 0;
}

} //StorageEngineNS
} //FounerXDB