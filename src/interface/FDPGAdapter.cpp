#include "interface/StorageEngineExceptionUniversal.h"
#include "postgres.h"
#include "utils/relcache.h"
#include "access/genam.h"
#include "access/heapam.h"
#include "access/relscan.h"
#include "utils/tqual.h"
#include "storage/bufmgr.h"
#include "storage/proc.h"
#include "utils/snapmgr.h"
#include "storage/lock.h"
#include "storage/itemptr.h"
#include "utils/relcache.h"
#include "access/nbtree.h"
#include "access/tupdesc.h"
#include "access/relscan.h"
#include "access/sdir.h"
#include "access/tuptoaster.h"
#include "utils/tqual.h"
#include "catalog/heap.h"
#include "access/xact.h"
#include "access/twophase.h"
#include "storage/proc.h"
#include "access/xact.h"
#include "catalog/heap.h"
#include "storage/lmgr.h"
#include "storage/smgr.h"
#include "storage/ipc.h"
#include "storage/fd.h"
#include "storage/proc.h"
#include "catalog/index.h"
#include "catalog/pg_database.h"
#include "access/nbtree.h"
#include "access/heapam.h"
#include "access/xlog_internal.h"
#include "commands/tablespace.h"
#include "commands/tablecmds.h"
#include "utils/rel.h"
#include "utils/snapmgr.h"
#include "utils/memutils.h"
#include "miscadmin.h"
#include "catalog/pg_largeobject.h"
#include "postmaster/walwriter.h"
#include "postmaster/bgwriter.h"
#include "commands/vacuum.h"
#include "postmaster/pgarch.h"
#include "utils/tuplestore.h"
#include "utils/tuplesort.h"
#include "replication/walsender.h"
#include "storage/large_object.h"
#include "storage/buf_internals.h"
#include "interface/FDPGAdapter.h"
#include "interface/ErrNo.h"

#include "catalog/xdb_catalog.h"
#include "postmaster/postmaster.h"
#include "Macros.h"
#include "commands/cluster.h"
using namespace FounderXDB::StorageEngineNS;

extern  int work_mem;
extern  int maintenance_work_mem;

//void FDPG_Heap::fd_MyGetHeapTupleDataSize(HeapTupleData* htup, void **rdata, 
//	size_t *rsize, char **mem2free)
//{
//	PG_TRY(); {
//		GetHeapTupleDataSize(htup, (*rdata), (*rsize), (*mem2free));
//	} PG_CATCH(); { 
//		throw StorageEngineExceptionUniversal(); 
//	} PG_END_TRY(); 
//}

void FDPG_StorageEngine::fd_start_engine(const char *path, const uint32 thread_num, 
																				 bool initDatadir, storage_params * params)
{
    THROW_CALL(start_engine,path,thread_num,initDatadir,params);
}


unsigned int FDPG_StorageEngine::getNumberOfPages(Relation relation)
{
    bool flag = false;
	unsigned int num = 0;
    PG_TRY(); {
		num = getNumberOfPagesForFile(relation->rd_smgr, MAIN_FORKNUM);
    } PG_CATCH(); { 
    flag=true;          
    } PG_END_TRY(); 
    if (flag)
    {
      ThrowException(); 
    }
	return num;
}

uint32 FDPG_StorageEngine::fd_getCurrentTimeLine( void )
{
	uint32 thisTimeLine = 0;
	THROW_CALL(thisTimeLine = GetCurrentTimeLine);
    return thisTimeLine;
}

bool FDPG_StorageEngine::fd_am_master()
{
	return !RecoveryInProgress();
}

void FDPG_StorageEngine::fd_reg_trigger_func(TriggerFunc func)
{
	RegTriggerFunc(func);
}

void FDPG_StorageEngine::fd_list_all_tablespc(std::vector<std::pair<std::string, std::string> > &v_tablespc)
{
	bool flag = false;

	PG_TRY(); {
		List *l = ListTableSpace();

		ListCell *lc = NULL;

		FormTablespaceData formClass = NULL;

		foreach(lc, l)
		{
			formClass = (FormTablespaceData)lfirst(lc);

			size_t len = VARSIZE_ANY(&formClass->spclocation);
			char *data = VARDATA(&formClass->spclocation);
			v_tablespc.push_back(std::make_pair(formClass->spcname.data, std::string(data, len)));
		}

		list_free_deep(l);
		//int num = 3;
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_StorageEngine::fd_end_engine(void)
{
    bool flag = false;
    PG_TRY(); {
        stop_engine(0);
    } PG_CATCH(); { 
        flag=true;    
    } PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

bool FDPG_StorageEngine::fd_start_archive( void )
{
	int ret = 0;
    THROW_CALL(ret = pgarch_start);
	return ret != 0;
}

bool FDPG_StorageEngine::fd_end_archive( void )
{
	int ret = 0;
    THROW_CALL(ret = pgarch_end);
	return ret != 0;
}

void FDPG_StorageEngine::fd_start_backup(const char* strLabel, bool fast )
{
	XLogRecPtr	startpoint;
    THROW_CALL(startpoint = do_pg_start_backup,strLabel,fast,NULL); 
}

void FDPG_StorageEngine::fd_stop_backup( void )
{
	XLogRecPtr	result;
    THROW_CALL(result = do_pg_stop_backup,NULL,true);
}

void FDPG_StorageEngine::fd_interrupt_start_engine(int code)
{
	volatile int errorCode = 0;
	switch(code)
	{
	case 0:
		errorCode = 0;
		break;
	case 1:
		errorCode = ERRCODE_REPHA_TWO_MASTERS;
		break;
	default:
		errorCode = 0;
		break;
	}

	bool flag = false;
	PG_TRY(); {
		interrupt_start_engine(errorCode);
	} PG_CATCH(); { 
		flag=true;    
	} PG_END_TRY(); 
	if (flag)
	{
		ThrowException(); 
	}	
}

void FDPG_Heap::fd_heap_drop(Oid relationId, Oid dbid)
{
    bool flag = false;
	PG_TRY(); {
		fdxdb_heap_drop(relationId,dbid);
	} PG_CATCH(); { 
		flag=true;    
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

Oid FDPG_Heap::fd_heap_create(Oid reltablespace, Oid relid, uint32 dbid,uint32 colid)
{
    bool flag = false;
	//Relation rel = NULL;
	PG_TRY(); {
		return fxdb_heap_create(relid,colid,reltablespace, dbid);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return InvalidOid;
}

Oid FDPG_Heap::fd_temp_heap_create(Oid reltablespace, uint32 dbid /* = 0 */, 
																	 uint32 colid /* = 0 */, OnCommitAction action /* = ONCOMMIT_DROP */)
{
	bool flag = false;
	//Relation rel = NULL;
	PG_TRY(); {
		return fxdb_heap_create(InvalidOid, colid, reltablespace, dbid, 
			RELKIND_RELATION, RELPERSISTENCE_TEMP, action);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
	if (flag)
	{
		ThrowException(); 
	}
	return InvalidOid;
}

void FDPG_Heap::fd_fxdb_SubPostmaster_Main(void *param)
{
    bool flag = false;
	PG_TRY(); {
		fxdb_SubPostmaster_Main(param);
	} PG_CATCH(); { 
		flag=true;    
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    } 
}

void FDPG_Heap::fd_proc_exit(int code)
{
    bool flag = false;
	PG_TRY(); {
		proc_exit(code);
	} PG_CATCH(); { 
		flag=true;    
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }    
}

void FDPG_Heap::fd_heap_inplace_update(Relation relation, HeapTuple tup)
{
    bool flag = false;
	PG_TRY(); {
		heap_inplace_update(relation, tup);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }    
}

Relation FDPG_Heap::fd_heap_open(Oid relationId, LOCKMODE lockmode, Oid dbid)
{
    bool flag = false;
	Relation rel = NULL;
	PG_TRY(); {
		rel = heap_open(relationId, lockmode,dbid);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return rel;
}

void FDPG_Heap::fd_heap_close(Relation relation, LOCKMODE lockmode)
{
    bool flag = false;
	PG_TRY(); {
		heap_close(relation, lockmode);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

HeapTuple FDPG_Heap::fd_heap_form_tuple(const char *p, size_t len, uint32 nEids, Oid *pEids)
{
    bool flag = false;
	HeapTuple htup = NULL;
	PG_TRY(); {
		htup = fdxdb_heap_formtuple(p, len, nEids, pEids);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return htup;
}

HeapTuple FDPG_Heap::heap_tuple_copy_bytearray_straight(char *row, uint32 rowlength, uint32 *newlen)
{
	// TODO:
	HeapTuple tup = (HeapTuple)row;
	HeapTupleHeader td = NULL;

	memset(row, 0, HEAPTUPTWOHDRSZ);	
	td = (HeapTupleHeader) (row + HEAPTUPLESIZE);
	tup->t_data = td;

	HeapTupleHeaderSetDatumLength(td, rowlength);
	HeapTupleHeaderSetTypeId(td, (Oid)-1);
	HeapTupleHeaderSetTypMod(td, (Oid)-1);

	HeapTupleHeaderSetNatts(td, 1);
	td->t_hoff = HEAPTUPHDRSZ;

	td->t_infomask &= ~(HEAP_HASNULL | HEAP_HASEXTERNAL);//we always have HEAP_HASVARWIDTH bit.
	td->t_infomask |= HEAP_HASVARWIDTH;

	SET_VARSIZE(row + HEAPTUPTWOHDRSZ - VARHDRSZ, rowlength + VARHDRSZ);// the first HEAPTUPHDRSZ bytes are for the HeapTupleHeaderData.
	if (VARDATA(row + HEAPTUPTWOHDRSZ - VARHDRSZ) != row + HEAPTUPTWOHDRSZ) { // by default the data begins right after the 4 byte VARHDR, otherwise copy.
		memmove(VARDATA(row + HEAPTUPTWOHDRSZ - VARHDRSZ), row + HEAPTUPTWOHDRSZ, rowlength);
		*newlen = HEAPTUPHDRSZ + VARHDRSZ + rowlength;
		tup->t_len = *newlen;
	} else {
		*newlen = 0;// no change, set newlen to 0 to indicate.
		tup->t_len = HEAPTUPHDRSZ + VARHDRSZ + rowlength;
	}

	return tup;
}

Oid FDPG_Heap::fd_simple_heap_insert(Relation relation, HeapTuple tup)
{
    bool flag = false;
	Oid ret = InvalidOid;
	PG_TRY(); {
		ret = simple_heap_insert(relation, tup);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return ret;
}

struct ReIndexStat 
{
	Relation index;
	IndexInfo* info;
	BTBuildState buildstate;
	ReIndexStat() 
	{
		index = NULL;
		info = NULL;
		buildstate.isUnique = false;
		buildstate.haveDead = false;
		buildstate.heapRel = NULL;
		buildstate.spool = NULL;
		buildstate.spool2 = NULL;
		buildstate.indtuples = 0;
	}
	void init(Relation heap, Relation index_relation, int kbSize1, int kbSize2)
	{
		index = index_relation;
		info = BuildIndexInfo(index);
		buildstate.isUnique = info->ii_Unique;
		buildstate.haveDead = false;
		buildstate.heapRel = heap;
		buildstate.spool = NULL;
		buildstate.spool2 = NULL;
		buildstate.indtuples = 0;
		buildstate.spool = _bt_spoolinit_kb(index, info->ii_Unique, kbSize1);

		/*
		 * If building a unique index, put dead tuples in a second spool to keep
		 * them out of the uniqueness check.
		 */
		if (info->ii_Unique) {
			buildstate.spool2 = _bt_spoolinit_kb(index, false, kbSize2);
		}
	}
	void destroy() 
	{
		index->user_data = NULL;
		_bt_spooldestroy(buildstate.spool);
		if (buildstate.spool2)
			_bt_spooldestroy(buildstate.spool2);
		if (info) {
			pfree(info);
		} 
	}
};

struct ReIndexesStat 
{
	Relation relation;
	std::vector<ReIndexStat> indexes;
} ;

void FDPG_Index::fd_index_insert_indexes(ReIndexesStat& IndexesStat)
{ 
	HeapScanDesc scan = NULL;
	PG_TRY(); {
		Datum		values[INDEX_MAX_KEYS];
		bool		isnull[INDEX_MAX_KEYS];
		OffsetNumber root_offsets[MaxHeapTuplesPerPage];
		BlockNumber root_blkno = InvalidBlockNumber;
		scan = FDPG_Heap::fd_heap_beginscan(IndexesStat.relation, SnapshotNow, 0, NULL);
		HeapTuple tup = NULL;
		while ((tup = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL) 
		{
			if (scan->rs_cblock != root_blkno)
			{
				Page		page = BufferGetPage(scan->rs_cbuf);

				LockBuffer(scan->rs_cbuf, BUFFER_LOCK_SHARE);
				heap_get_root_tuples(page, root_offsets);
				LockBuffer(scan->rs_cbuf, BUFFER_LOCK_UNLOCK);

				root_blkno = scan->rs_cblock;
			}

			for (size_t i = 0; i < IndexesStat.indexes.size(); ++i) {
				ReIndexStat& indexStat = IndexesStat.indexes[i];

				indexStat.index->user_data = MtInfo_GetUserData(indexStat.index->mt_info);
				FormIndexDatum(IndexesStat.relation,
					indexStat.index,
					indexStat.info,
					tup,
					values,
					isnull);
				indexStat.index->user_data = NULL;
				/*
				* You'd think we should go ahead and build the index tuple here, but
				* some index AMs want to do further processing on the data first.	So
				* pass the values[] and isnull[] arrays, instead.
				*/

				if (HeapTupleIsHeapOnly(tup))
				{
					/*
					* For a heap-only tuple, pretend its TID is that of the root. See
					* src/backend/access/heap/README.HOT for discussion.
				 */
					HeapTupleData rootTuple;
					OffsetNumber offnum;

					rootTuple = *tup;
					offnum = ItemPointerGetOffsetNumber(&tup->t_self);

					Assert(OffsetNumberIsValid(root_offsets[offnum - 1]));

					ItemPointerSetOffsetNumber(&rootTuple.t_self,
						root_offsets[offnum - 1]);

					/* Call the AM's callback routine to process the tuple */
					btbuildCallback(indexStat.index, &rootTuple, values, isnull, true, &indexStat.buildstate);
				}
				else
				{
					btbuildCallback(indexStat.index, tup, values, isnull, true, &indexStat.buildstate);
				}
			}
		}
		FDPG_Heap::fd_heap_endscan(scan);
	} PG_CATCH(); { 
		if (scan) 
		{
			FDPG_Heap::fd_heap_endscan(scan);	
		}
		ThrowException();
	} PG_END_TRY(); 
}

ReIndexesStat* FDPG_Index::fd_index_init_reIndexes(Relation heap, const std::vector<Relation>& indexes)
{
	PG_TRY(); {
		ReIndexesStat* stat = new ReIndexesStat();
		stat->indexes.clear();
		stat->relation = heap;
		ReIndexStat indexStat;
		int kbSize1 = maintenance_work_mem/(int)indexes.size();
		int kbSize2 = work_mem/(int)indexes.size();
		stat->indexes.resize(indexes.size(), indexStat);
		for (size_t i = 0; i < indexes.size(); ++i) {
			stat->indexes[i].init(heap, indexes[i], kbSize1, kbSize2);
		}
		return stat;
	} PG_CATCH(); { 
		ThrowException();
	} PG_END_TRY(); 
	return NULL;
}

void FDPG_Index::fd_index_do_build_indexes(ReIndexesStat& stat)
{
	PG_TRY(); {
		for (size_t i = 0; i < stat.indexes.size(); ++i) {
			/* okay, all heap tuples are indexed */
			if (stat.indexes[i].buildstate.spool2 && !stat.indexes[i].buildstate.haveDead)
			{
				/* spool2 turns out to be unnecessary */
				_bt_spooldestroy(stat.indexes[i].buildstate.spool2);
				stat.indexes[i].buildstate.spool2 = NULL;
			}
			/*
			* Finish the build by (1) completing the sort of the spool file, (2)
			* inserting the sorted tuples into btree pages and (3) building the upper
			* levels.
			*/
			_bt_leafbuild(stat.indexes[i].buildstate.spool, stat.indexes[i].buildstate.spool2);
		}
	} PG_CATCH(); { 
		ThrowException();
	} PG_END_TRY(); 
}

void FDPG_Index::fd_index_destory_reIndexes(ReIndexesStat* stat)
{
	PG_TRY(); {
		if (stat == NULL) {
			return;
		}
		for (size_t i = 0; i < stat->indexes.size(); ++i) {
			stat->indexes[i].destroy();
		}
		delete stat;
	} PG_CATCH(); { 
		if (stat) {
			delete stat;
		}
		ThrowException();
	} PG_END_TRY(); 
}

void FDPG_Heap::fd_heap_multi_insert(Relation relation, HeapTuple* tup, int tup_count, BulkInsertState bis)
{
	bool flag = false;

	PG_TRY(); {
		heap_multi_insert(relation, tup, tup_count, bis);
		index_multi_insert(relation,tup,tup_count);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
	if (flag)
	{
		ThrowException(); 
	}
}
Oid FDPG_Heap::fd_heap_insert_bulk(Relation relation, HeapTuple tup, BulkInsertState bis)
{
    bool flag = false;
		int options = HEAP_INSERT_SKIP_FSM;
    Oid ret = InvalidOid;
    PG_TRY(); {
        ret = heap_insert(relation, tup, GetCurrentCommandId(true), options, bis);
				insert_tuple_to_indexes(relation,tup);
    } PG_CATCH(); { 
        flag=true;
    } PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
    return ret;
}

Oid FDPG_Heap::fd_heap_insert_bulk(Relation relation, HeapTuple *tup, uint32 tupnum, BulkInsertState bis)
{
	extern TupleDesc single_attibute_tupDesc;

	bool flag = false;
	Oid ret = InvalidOid;
	PG_TRY(); {
		bool should_free = false;
		void *compare_func = (void *)relation->mt_info.tableColInfo->rd_comfunction[0];
		Tuplesortstate *state = tuplesort_begin_heap(single_attibute_tupDesc, tupnum * sizeof(HeapTuple), true, compare_func);
		for(uint32 i = 0; i < tupnum; ++i)
		{
			ret = heap_insert(relation, tup[i], GetCurrentCommandId(true), 0, bis);
			tuplesort_putheaptuple(state, tup[i]);
		}

		tuplesort_performsort(state);
		HeapTuple tuple = NULL;
		while(NULL != (tuple = tuplesort_getheaptuple(state, true, &should_free)))
		{
			insert_tuple_to_indexes(relation, tuple);
			if(should_free)
			{
				pfree(tuple);
			}
		}
		tuplesort_end(state);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
	if (flag)
	{
		ThrowException(); 
	}
	return ret;
}

void FDPG_Heap::fd_simple_heap_delete(Relation relation, ItemPointer tid)
{
    bool flag = false;
	PG_TRY(); {
		simple_heap_delete(relation, tid);
	} PG_CATCH(); { 
    	flag=true;		
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

void FDPG_Heap::fd_simple_heap_update(Relation relation, ItemPointer otid, HeapTuple tup)
{
    bool flag = false;
	PG_TRY(); {
		simple_heap_update(relation, otid, tup);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

//void FDPG_Heap::fd_heap_freetuple(HeapTuple htup)
//{
//	PG_TRY(); {
//		heap_freetuple(htup);
//	} PG_CATCH(); { 
//		throw TDStorageEngineException(); 
//	} PG_END_TRY(); 
//}
//
bool FDPG_Heap::fd_heap_fetch(Relation relation,Snapshot snapshot,HeapTuple tuple,
	Buffer *userbuf,bool keep_buf,Relation stats_relation)
{
	bool flag = false;
	bool ret = false;
	PG_TRY(); {
		ret = heap_fetch(relation, snapshot,tuple,userbuf,keep_buf,stats_relation);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if (flag)
	{
		ThrowException(); 
	}
	return ret;
}

void FDPG_Heap::fd_heap_truncate(Relation relation)
{
	bool flag = false;

	PG_TRY(); {
		heap_truncate_one_rel(relation);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if (flag)
	{
		ThrowException(); 
	}

	return;
}

struct varlena *
toast_uncompress_datum(struct varlena * attr);
struct varlena *FDPG_Heap::fd_toast_uncompress_datum(struct varlena * attr)
{
	bool flag = false;
    struct varlena *res = 0;
    
	PG_TRY(); {
		res = toast_uncompress_datum(attr);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if (flag)
	{
		ThrowException(); 
	}

	return res;

}

//int FDPG_Heap::fd_heap_bulkload(Relation rel, BulkLoader *blr, BulkLoadRow *blrows,int norows)
//{
//	int ret = 0;
//	PG_TRY(); {
//		ret = heap_bulkload(rel, blr, blrows, norows);
//	} PG_CATCH(); { 
//		throw TDStorageEngineException(); 
//	} PG_END_TRY(); 
//	return ret;
//}
// scan API
HeapScanDesc FDPG_Heap::fd_heap_beginscan(Relation relation, Snapshot snapshot,int nkeys, ScanKey key, filter_func_t filterFunc, void*userData,ItemPointer otid,ItemPointer endid)
{
    bool flag = false;
	HeapScanDesc hscandesc = NULL;
	PG_TRY(); {
		hscandesc = heap_beginscan(relation, snapshot, nkeys, key, filterFunc, userData, otid?ItemPointerGetBlockNumber(otid):0, endid?ItemPointerGetBlockNumber(endid):0);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return hscandesc;
}
//
//void FDPG_Heap::fd_heap_rescan(HeapScanDesc scan,ScanKey key)
//{
//	PG_TRY(); {
//		heap_rescan(scan, key);
//	} PG_CATCH(); { 
//		throw TDStorageEngineException(); 
//	} PG_END_TRY(); 
//}

HeapTuple FDPG_Heap::fd_heap_getnext(HeapScanDesc scan, ScanDirection direction)
{
    bool flag = false;
	HeapTuple htup = NULL;
	PG_TRY(); {
		htup = heap_getnext(scan, direction);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }

	if(htup != NULL)
	{
		htup->t_nIndexes = 0;
		htup->t_indexOids = NULL;
	}
	return htup; 
}

void FDPG_Heap::fd_heap_endscan(HeapScanDesc scan)
{
    bool flag = false;
	PG_TRY(); {
		heap_endscan(scan);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

void FDPG_Heap::fd_heap_markpos(HeapScanDesc scan)
{
    bool flag = false;
	PG_TRY(); {
		heap_markpos(scan);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

void FDPG_Heap::fd_heap_restrpos(HeapScanDesc scan)
{
    bool flag = false;
	PG_TRY(); {
		heap_restrpos(scan);
	} PG_CATCH(); { 
		flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

unsigned int FDPG_Heap::fd_metatbl_get_max_id()
{
	bool flag = false;
	unsigned int max_id = 0;
	PG_TRY(); {
		max_id = fxdb_get_max_id();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if (flag)
	{
		ThrowException(); 
	}
	return max_id;
}

//unsigned int FDPG_Heap::fd_metatbl_get_tblspace(const Oid object_id)
//{
//	bool flag = false;
//	unsigned int tblspace = 0;
//	PG_TRY(); {
//		tblspace = fxdb_get_tblspace(object_id);
//	} PG_CATCH(); { 
//		flag = true;
//	} PG_END_TRY(); 
//
//	if (flag)
//	{
//		ThrowException(); 
//	}
//
//	if(tblspace == InvalidOid)
//	{
//		char msg[256];
//		memset(msg, 0, 256);
//		sprintf(msg, "Invalid table space:%u. ", tblspace);
//		ThrowException(INVALID_TABLE_SAPCE, msg); 
//	}
//	return tblspace;
//}

//unsigned int FDPG_Heap::fd_metatbl_get_type(Oid object_id)
//{
//	bool flag = false;
//	unsigned int tbl_type = 0;
//	PG_TRY(); {
//		tbl_type = fxdb_get_type(object_id);
//	} PG_CATCH(); { 
//		flag = true;
//	} PG_END_TRY(); 
//
//	if (flag)
//	{
//		ThrowException(); 
//	}
//
//	if(tbl_type == InvalidOid)
//	{
//		char msg[256];
//		memset(msg, 0, 256);
//		sprintf(msg, "Invalid type:%u. ", tbl_type);
//		ThrowException(INVALID_TABLE_TYPE, msg); 
//	}
//	return tbl_type;
//}

unsigned int FDPG_Heap::fd_metatbl_get_heapid(Oid index_id)
{
	bool flag = false;
	unsigned int heap_id = 0;
	PG_TRY(); {
		heap_id = fxdb_get_heapid(index_id);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if (flag)
	{
		ThrowException(); 
	}

	if(heap_id == InvalidOid)
	{
		char msg[256];
		memset(msg, 0, 256);
		sprintf(msg, "Index %u has no heap.", index_id);
		throw StorageEngineExceptionUniversal(INDEX_HAS_NO_HEAP, msg); 
	}
	return heap_id;
}

bool FDPG_Heap::fd_metatbl_insert_meta(Oid object_id, int type, Oid parent_table_id, pg_time_t time, 
														int owner, Oid database_id, Oid tablespace_id, unsigned int colinfo_id)
{
	bool flag = false;
	bool insert_sta = false;
	PG_TRY(); {
		insert_sta = fxdb_insert_meta(object_id, type, parent_table_id, time, owner, database_id, tablespace_id, colinfo_id);
		CommandCounterIncrement();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if (flag)
	{
		ThrowException(); 
	}
	return insert_sta;
}

bool FDPG_Heap::fd_metatbl_delete_meta(Oid object_id)
{
	bool flag = false;
	bool delete_sta = false;
	PG_TRY(); {
		delete_sta = fxdb_delete_meta(object_id);
		CommandCounterIncrement();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if (flag)
	{
		ThrowException(); 
	}

	if(delete_sta == false)
	{
		char msg[256];
		memset(msg, 0, 256);
		sprintf(msg, "Meta table has no relation %u..", object_id);
		throw StorageEngineExceptionUniversal(RELATION_NOT_IN_METATABLE, msg); 
	}
	return delete_sta;
}

bool FDPG_Heap::fd_metatbl_get_heap_info(Oid  m_index_entry_id, ColinfoData& colinfo)
{
	bool flag = false;
	bool get_sta = false;
	PG_TRY(); {
		get_sta = fxdb_get_heap_info(m_index_entry_id, colinfo);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if (flag)
	{
		ThrowException(); 
	}

	if(get_sta == false)
	{
		char msg[256];
		memset(msg, 0, 256);
		sprintf(msg, "Heap colinfo has no found.");
		throw StorageEngineExceptionUniversal(RELATION_NOT_IN_METATABLE, msg); 
	}
	return get_sta;
}

BulkInsertState FDPG_Heap::fd_GetBulkInsertState(void)
{
	bool flag = false;
	PG_TRY();{
		return GetBulkInsertState();
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return NULL;
}

void FDPG_Heap::fd_FreeBulkInsertState(BulkInsertState bistate)
{
	bool flag = false;
	PG_TRY();{
		FreeBulkInsertState(bistate);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
}

void FDPG_Heap::fd_ClusterRel(Oid relid, Oid idxid, bool use_sort)
{
	bool flag = false;
	PG_TRY();{
		set_cluster_use_sort(use_sort);
		cluster_rel(relid, idxid, true, true, -1, -1);
		set_cluster_use_sort(false);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
}

void FDPG_Heap::fd_VacuumRelations(int nrels, Oid *relids)
{
	bool flag = false;
	PG_TRY();{
		vacuum_relations(nrels, relids);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
}

//bool FDPG_Heap::fd_metatbl_get_index_info(Oid table_id, IndinfoData& indinfo)
//{
//	bool flag = false;
//	bool get_sta = false;
//	PG_TRY(); {
//		get_sta = fxdb_get_index_info(table_id, indinfo);
//	} PG_CATCH(); { 
//		flag = true;
//	} PG_END_TRY(); 
//
//	if (flag)
//	{
//		ThrowException(); 
//	}
//	return get_sta;
//}

//void FDPG_Index::fd_index_build(Relation heapRelation,Relation indexRelation,IndexInfo *indexInfo)
//{
//	PG_TRY(); {
//		index_build(heapRelation, indexRelation, indexInfo);
//	} PG_CATCH(); { 
//        ThrowException(); 
//	} PG_END_TRY(); 
//}

void FDPG_Index::fd_index_drop(Oid relid, Oid tblspcid, Oid indexId, Oid dbid)
{
    bool flag = false;
	PG_TRY(); {
		index_drop(indexId, relid,dbid);
	} PG_CATCH(); { 
        flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

void FDPG_Index::fd_index_close(Relation relation, LOCKMODE lockmode)
{
    bool flag = false;
	PG_TRY(); {
		index_close(relation, lockmode);
	} PG_CATCH(); { 
         flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

Oid FDPG_Index::fd_index_create(Relation heapRelation, IndexType idxtype,Oid reloid,uint32 colid,
	void* userData,filter_func_t filterfunc, uint32 userdataLength, bool skipInsert)
{
    bool flag = false;
	//Relation rel = NULL;
	Oid id = 0;
	PG_TRY(); {
		id = fxdb_index_create(reloid, heapRelation, idxtype, colid, userData, filterfunc, userdataLength, skipInsert);
		return id;
	} PG_CATCH(); { 
        flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return InvalidOid;
}
Relation FDPG_Index::fd_index_open(Oid relationId, LOCKMODE lockmode, Oid dbid)
{
    bool flag = false;
	Relation rel = NULL;
	PG_TRY(); {
		rel = index_open(relationId, lockmode,dbid);
	} PG_CATCH(); { 
        flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }
	return rel;
}

bool FDPG_Index::fd_index_insert(Relation indexRelation,Datum *values,
	bool *isnull,ItemPointer heap_t_ctid,Relation heapRelation,bool check_uniqueness)
{
    bool flag = false;
	bool ret = false;
    IndexUniqueCheck check_unique = UNIQUE_CHECK_NO;
    if (check_uniqueness) {
        check_unique = UNIQUE_CHECK_YES;
    }
	PG_TRY(); {
		ret = index_insert(indexRelation,values,isnull,heap_t_ctid,heapRelation,check_unique);
	} PG_CATCH(); { 
       flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return ret;
}

void FDPG_Index::fd_index_endscan(IndexScanDesc scan)
{
    bool flag = false;
	PG_TRY(); {
		index_endscan(scan);
	} PG_CATCH(); { 
        flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

IndexScanDesc FDPG_Index::fd_index_beginscan(Relation heapRelation,
	Relation indexRelation,Snapshot snapshot,int nkeys, ScanKey key, filter_func_t filterFunc, void*userData)
{
    bool flag = false;
	IndexScanDesc iscan = NULL;
	PG_TRY(); {
		iscan = index_beginscan(heapRelation,indexRelation,snapshot,nkeys, 0, filterFunc, userData);
	} PG_CATCH(); { 
       flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return iscan;
}

void FDPG_Index::fd_index_rescan(IndexScanDesc scan, ScanKey keys, int nkeys, ScanKey orderbys, int norderbys)
{
    bool flag = false;
	PG_TRY(); {
		index_rescan(scan, keys, nkeys, orderbys, norderbys);
	} PG_CATCH(); { 
        flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

HeapTuple FDPG_Index::fd_index_getnext(IndexScanDesc scan, ScanDirection direction)
{
    bool flag = false;
	HeapTuple htup = NULL;
	PG_TRY(); {
		htup = index_getnext(scan, direction);
	} PG_CATCH(); { 
         flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }
	if(htup != NULL)
	{
		htup->t_nIndexes = 0;
		htup->t_indexOids = NULL;
	}
	return htup; 
}

void FDPG_Index::fd_index_markpos(IndexScanDesc scan)
{
    bool flag = false;
	PG_TRY(); {
		index_markpos(scan);
	} PG_CATCH(); { 
        flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

void FDPG_Index::fd_index_restrpos(IndexScanDesc scan)
{
    bool flag = false;
	PG_TRY(); {
		index_restrpos(scan);
	} PG_CATCH(); { 
         flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

void FDPG_Index::fd_reindex_index(Oid heapId, Oid indexId, Oid dbid)
{
	bool flag = false;
	PG_TRY(); {
		reindex_index(heapId, indexId, dbid);
	} PG_CATCH(); { 
         flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

bool FDPG_Index::fd_reindex_relation(Oid relid, Oid dbid, int flags)
{
	bool flag = false;
	bool ret = false;
	PG_TRY(); {
		ret = reindex_relation(relid, dbid, flags);
	} PG_CATCH(); { 
         flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return ret;
}

void FDPG_Index::fd_reindex_database(Oid dbid)
{
	int flag = false;
	PG_TRY(); {
		reindex_database(dbid);
	} PG_CATCH(); { 
         flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}

void FDPG_Index::fd_index_getrange(Relation heapRelation, Relation indexRelation,
		std::string &min, std::string &max, Snapshot snapshot)
{
	int flag = false;
	PG_TRY(); {
		min.clear();
		max.clear();

		Datum dmin = index_get_min(heapRelation, indexRelation, snapshot);
		Datum dmax = index_get_max(heapRelation, indexRelation, snapshot);
		if(dmin != (Datum)0)
		{
			min.append(VARDATA_ANY(DatumGetPointer(dmin)), VARSIZE_ANY_EXHDR(DatumGetPointer(dmin)));
		}
		if(dmax != (Datum)0)
		{
			max.append(VARDATA_ANY(DatumGetPointer(dmax)), VARSIZE_ANY_EXHDR(DatumGetPointer(dmax)));
		}
	} PG_CATCH(); { 
         flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
}




//
//CommandId FDPG_Transaction::fd_GetCurrentCommandId(bool b)
//{
//	CommandId cmdid = FirstCommandId;
//	PG_TRY(); {
//		cmdid = GetCurrentCommandId(b);
//	} PG_CATCH(); { 
//		throw TDStorageEngineException(); 
//	} PG_END_TRY(); 
//	return cmdid;
//}

Snapshot FDPG_Transaction::fd_GetTransactionSnapshot()
{
    bool flag = false;
	Snapshot snapshot = NULL;
	PG_TRY(); {
		snapshot = GetTransactionSnapshot();
	} PG_CATCH(); { 
		 flag=true;
	} PG_END_TRY(); 
    if (flag)
    {
        ThrowException(); 
    }
	return snapshot;
}
//

void FDPG_Transaction::fd_StartTransactionCommand()
{
    bool flag = false;
	PG_TRY(); {
		StartTransactionCommand();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }
}

//void FDPG_Transaction::fd_RollbackToSavepoint(const char* name)
//{
//	PG_TRY(); {
//		RollbackToSavepoint(name);
//	} PG_CATCH(); {
//		throw TDStorageEngineException();
//	} PG_END_TRY();
//}
//
bool FDPG_Transaction::fd_EndTransactionBlock()
{
    bool flag = false;
	bool ret = false;
	PG_TRY(); {
		ret = EndTransactionBlock();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }
	return ret;
}
    
void FDPG_Transaction::fd_CommitTransactionCommand()
{
    bool flag = false;
	PG_TRY(); {
		CommitTransactionCommand();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }
}

//void FDPG_Transaction::fd_DefineSavepoint(const char *name)
//{
//	PG_TRY(); {
//		DefineSavepoint(name);
//	} PG_CATCH(); {
//		throw TDStorageEngineException();
//	} PG_END_TRY();
//}

void	FDPG_Transaction::fd_UserAbortTransactionBlock()
{
    bool flag = false;
	PG_TRY(); {
		UserAbortTransactionBlock();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }
}

void	FDPG_Transaction::fd_AbortCurrentTransaction()
{
    bool flag = false;
	PG_TRY(); {
		AbortCurrentTransaction();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException();
    }
}
void	FDPG_Transaction::fd_PreCommit_on_commit_actions()
{
    bool flag = false;
	PG_TRY(); {
		PreCommit_on_commit_actions();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException();
    }
}

void FDPG_Transaction::fd_AbortOutOfAnyTransaction(void)
{
    bool flag = false;
    PG_TRY(); {
        AbortOutOfAnyTransaction();
    } PG_CATCH(); {
        flag=true;
    }PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }
}


void FDPG_Transaction::fd_CommandCounterIncrement(void)
{
    bool flag = false;
	PG_TRY(); {
		CommandCounterIncrement();
	} PG_CATCH(); {
		 flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }
}

void FDPG_Transaction::fd_BeginTransactionBlock(void)
{
    bool flag = false;
	PG_TRY(); {
		BeginTransactionBlock();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
    if (flag)
    {
        ThrowException(); 
    }
}

void FDPG_Transaction::fd_BeginSubTransaction( char *name )
{
	bool flag = false;
	PG_TRY(); {
		BeginInternalSubTransaction(name);
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}
}

void FDPG_Transaction::fd_CommitSubTransaction( void )
{
	bool flag = false;
	PG_TRY(); {
		ReleaseCurrentSubTransaction();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}
}

void FDPG_Transaction::fd_AbortSubTransaction( void )
{
	bool flag = false;
	PG_TRY(); {
		RollbackAndReleaseCurrentSubTransaction();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}
}

#ifdef FOUNDER_XDB_SE
void FDPG_Transaction::fd_setXactIsoLevel(int level)
{
	bool flag = false;
	PG_TRY(); {
		setXactIsoLevel(level);
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}
}

int FDPG_Transaction::fd_getXactIsoLevel(void)
{
	return getXactIsoLevel();
}

TransactionState FDPG_Transaction::fd_get_current_transaction_state(void)
{
	return get_current_transaction_state();
}

TransactionState FDPG_Transaction::fd_get_top_transaction_state(void)
{
	return get_top_transaction_state();
}

void FDPG_Transaction::fd_set_top_transaction_state(TransactionState top_stransaction_state)
{
	set_top_transaction_state(top_stransaction_state);
}

void FDPG_Transaction::fd_set_current_transaction_state(TransactionState current_stransaction_state)
{
	bool flag = false;
	PG_TRY(); {
		set_current_transaction_state(current_stransaction_state);
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}
}

bool FDPG_Transaction::fd_IsTransactionBlock()
{
	bool flag = false;
	PG_TRY(); {
		return IsTransactionBlock();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}
	return flag;
}

#endif

bool FDPG_Transaction::fd_PrepareTransactionBlock(char *gid)
{
	bool flag = false;
	bool ret = false;
	
	PG_TRY(); {
		ret = PrepareTransactionBlock(gid);
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}
	return ret;
}

void FDPG_Transaction::fd_FinishPreparedTransaction(const char *gid, const bool isCommit)
{
	bool flag = false;

	PG_TRY(); {
		FinishPreparedTransaction(gid, isCommit);
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}
}

bool FDPG_Transaction::fd_GlobalTransactionIdIsPrepared(char *gxid)
{
	bool flag = false;
	bool ret = false;

	PG_TRY(); {
		ret = GlobalTransactionIdIsPrepared(gxid);
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}

	return ret;
}

uint32 FDPG_Transaction::fd_GetNextMaxPreparedGlobalTxnId()
{
	bool flag = false;
	uint32 gxid = 0;
	PG_TRY(); {
		gxid = GetNextMaxPreparedGlobalTxnId();
	} PG_CATCH(); {
		flag=true;
	} PG_END_TRY();
	if (flag)
	{
		ThrowException(); 
	}

	return gxid;
}

Tuplestorestate *
FDPG_TupleStore::fd_tuplestore_begin_heap(bool randomAccess, bool interXact, int maxKBytes)
{
	Tuplestorestate *ret = NULL;
	bool flag = false;

	PG_TRY(); {
		ret = tuplestore_begin_heap(true, interXact, maxKBytes);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return ret;
}

void
FDPG_TupleStore::fd_tuplestore_end(Tuplestorestate *state)
{
	bool flag = false;

	PG_TRY(); {
		tuplestore_end(state);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}
void
FDPG_TupleStore::fd_tuplestore_puttuple_common(Tuplestorestate *state, void *data)
{
	bool flag = false;

	PG_TRY(); {
		tuplestore_puttuple(state, (HeapTuple)data);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

int
FDPG_TupleStore::fd_tuplestore_alloc_read_pointer(Tuplestorestate *state, int eflags)
{
	int ret = 0;
	bool flag = false;

	PG_TRY(); {
		ret = tuplestore_alloc_read_pointer(state, eflags);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return ret;
}

void
FDPG_TupleStore::fd_tuplestore_rescan(Tuplestorestate *state)
{
	bool flag = false;

	PG_TRY(); {
		tuplestore_rescan(state);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

bool
FDPG_TupleStore::fd_tuplestore_ateof(Tuplestorestate *state)
{
	bool ret = false;
	bool flag = false;

	PG_TRY(); {
		ret = tuplestore_ateof(state);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return ret;
}

void *
FDPG_TupleStore::fd_tuplestore_gettuple(Tuplestorestate *state, bool forward,
																				bool *should_free)
{
	void *ret = NULL;
	bool flag = false;

	PG_TRY(); {
		ret = tuplestore_gettuple(state, forward, should_free);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return ret;
}
void
FDPG_TupleStore::fd_tuplestore_copy_read_pointer(Tuplestorestate *state,
																								 int srcptr, int destptr)
{
	bool flag = false;
	PG_TRY(); {
		tuplestore_copy_read_pointer(state, srcptr, destptr);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void
FDPG_TupleStore::fd_tuplestore_trim(Tuplestorestate *state)
{
	bool flag = false;

	PG_TRY(); {
		tuplestore_trim(state);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

HeapTuple FDPG_TupleStore::fd_heap_tuple_from_minimal_tuple(MinimalTuple mtup)
{
	bool flag = false;
    HeapTuple tup = NULL;
	PG_TRY();
	{
		tup = heap_tuple_from_minimal_tuple(mtup);
	} 
	PG_CATCH();
	{
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	if(tup != NULL)
	{
		tup->t_nIndexes = 0;
		tup->t_indexOids = NULL;
	}
	return tup;
}

void * FDPG_TupleStore::fd_minimal_tuple_getdata(MinimalTuple mtup,int &len)
{
	bool flag = false;
	void* ret = 0;
	PG_TRY();
	{
		ret = minimal_tuple_getdata(mtup, len);
	} 
	PG_CATCH();
	{
		flag = true;
	} PG_END_TRY();
	if(flag)
	{
		ThrowException();
	}	
	return ret;
}
Tuplesortstate *FDPG_TupleSort::fd_tuplesort_begin_data(int workMem,bool randomAccess,
																												//btree_compare_func_type func,
																												void* compare_func
																												/*,Oid tableSpaceId*/)
{
	Tuplesortstate *tupstate = NULL;
	bool flag = false;

	PG_TRY(); {
		tupstate = tuplesort_begin_data(workMem,true,compare_func);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if(flag)
	{
		ThrowException();
	}
	return tupstate;
}

Tuplesortstate *FDPG_TupleSort::fd_tuplesort_begin_heap(int workMem, bool randomAccess, void *compare_func)
{
	extern TupleDesc single_attibute_tupDesc;

	Tuplesortstate *tupstate = NULL;
	bool flag = false;

	PG_TRY(); {
		tupstate = tuplesort_begin_heap(single_attibute_tupDesc, workMem,true,compare_func);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if(flag)
	{
		ThrowException();
	}
	return tupstate;
}

void FDPG_TupleSort::fd_tuplesort_putdata(Tuplesortstate *state, void* data, size_t len)
{
	bool flag = false;

	PG_TRY(); {
		tuplesort_putdata(state, data, len);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_TupleSort::fd_tuplesort_put_heaptuple(Tuplesortstate *state, HeapTuple tup)
{
	bool flag = false;

	PG_TRY(); {
		tuplesort_putheaptuple(state, tup);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_TupleSort::fd_tuplesort_rescan(Tuplesortstate *state)
{
	bool flag = false;

	PG_TRY(); {
		tuplesort_rescan(state);
	} PG_CATCH(); { 
		flag = true; 
	} PG_END_TRY(); 

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_TupleSort::fd_tuplesort_performsort(Tuplesortstate *state, void *compare_func)
{
	bool flag = false;

	PG_TRY(); {
		tuplesort_performsort(state, compare_func);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_TupleSort::fd_tuplesort_end(Tuplesortstate *state)
{
	bool flag = false;

	PG_TRY(); {
		tuplesort_end(state);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

bool FDPG_TupleSort::fd_tuplesort_support_random_access(Tuplesortstate *state)
{
	bool ret = false;
	bool flag = false;

	PG_TRY(); {
		ret = tuplesort_support_random_access(state);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY(); 

	if(flag)
	{
		ThrowException();
	}
	return ret;
}

bool FDPG_TupleSort::fd_tuplesort_getdata(Tuplesortstate *state, bool forward, void** pData, int *len)
{
	bool ret = false;
	bool flag = false;

	PG_TRY(); {
		ret = tuplesort_getdata(state, forward, pData, len);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return ret;
}

bool FDPG_TupleSort::fd_tuplesort_getheaptuple(Tuplesortstate *state, bool forward, HeapTuple *tuple, bool* should_free)
{
	bool flag = false;

	PG_TRY(); {
		*tuple = tuplesort_getheaptuple(state, forward, should_free);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return (*tuple) != 0;
}

void FDPG_TupleSort::fd_tuplesort_markpos(Tuplesortstate *state)
{
	bool flag = false;

	PG_TRY(); {
		tuplesort_markpos(state);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}
void FDPG_TupleSort::fd_tuplesort_restorepos(Tuplesortstate *state)
{
	bool flag = false;

	PG_TRY(); {
		tuplesort_restorepos(state);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_TupleSort::fd_tuplesort_setsorted(Tuplesortstate *state)
{
    bool flag = false;

	PG_TRY(); {
		tuplesort_set_sorted(state);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_create(const char *name, 
																		 const uint32 extra_data_len,
																		 const void *extra_data,
																		 uint32 &lo_id)
{
	bool flag = false;

	PG_TRY(); {
		lo_id = inv_create(const_cast<char*>(name), 
			extra_data_len, 
			const_cast<void*>(extra_data));
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_open(const uint32 lobjId, const int flags, 
								   MemoryContext mcxt, LargeObjectDesc **lod)
{
	bool flag = false;

	PushActiveSnapshot(SnapshotNow);
	PG_TRY(); {
		*lod = inv_open(lobjId, flags, mcxt);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();
	PopActiveSnapshot();
	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_open(const char *name, const int flags, 
																	 MemoryContext mcxt, LargeObjectDesc **lod)
{
	bool flag = false;
		
	PushActiveSnapshot(SnapshotNow);
	PG_TRY(); {
		*lod = fxdb_inv_name_open(const_cast<char*>(name), flags, mcxt);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();
	PopActiveSnapshot();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_close(LargeObjectDesc *obj_desc)
{
	bool flag = false;

	PG_TRY(); {
		inv_close(obj_desc);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_write(LargeObjectDesc *obj_desc, const char *buf, 
									const int nbytes, int &nwritten)
{
	bool flag = false;

	PG_TRY(); {
		nwritten = inv_write(obj_desc, buf, nbytes);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_read(LargeObjectDesc *obj_desc, const int nbytes, 
								   char *buf, int &nread)
{
	bool flag = false;

	PG_TRY(); {
		nread = inv_read(obj_desc, buf, nbytes);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_truncate(LargeObjectDesc *obj_desc, const int64 len)
{
	bool flag = false;

	PG_TRY(); {
		inv_truncate(obj_desc, len);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_drop(const uint32 lo_id, bool &retval)
{
	bool flag = false;

	PG_TRY(); {
		retval = inv_drop(lo_id) != 0;
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_drop(const char *name, bool &retval)
{
	bool flag = false;

	PG_TRY(); {
		retval = fxdb_inv_name_drop(const_cast<char*>(name)) != 0;
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_tell(LargeObjectDesc *obj_desc, uint64 &offset)
{
	bool flag = false;

	PG_TRY(); {
		offset = inv_tell(obj_desc);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_seek(LargeObjectDesc *obj_desc, const int64 offset, 
								   const int whence, uint64 &roffset)
{
	bool flag = false;

	PG_TRY(); {
		roffset = inv_seek(obj_desc, offset, whence);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_getmeta(LargeObjectDesc *obj_desc, Form_meta_large_data **formClass)
{
	bool flag = false;

	PG_TRY(); {
		lo_exit(obj_desc->id, formClass);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_inv_metaget_size(Form_meta_large_data *formClass, size_t &len)
{
	Assert(formClass != NULL);

	len = formClass->extra_data_size;
}

void FDPG_LargeObject::fd_inv_metaget_extradata(Form_meta_large_data *formClass, void **data)
{
	Assert(formClass != NULL);

	*data = &((char*)formClass)[sizeof(Form_meta_large_data)];
}

void FDPG_LargeObject::fd_inv_metaget_name(Form_meta_large_data *formClass, char **name)
{
	Assert(formClass != NULL);

	*name = formClass->lo_name.data;
}

void FDPG_LargeObject::fd_inv_metaget_id(Form_meta_large_data *formClass, Oid &id)
{
	Assert(formClass != NULL);

	id = formClass->lo_id;
}

bool FDPG_LargeObject::fd_inv_metaupdate_extradata(LargeObjectDesc *obj_desc, const void *extraData, 
																 size_t len, Form_meta_large_data **formClass)
{
	Assert(obj_desc != NULL && obj_desc->id > InvalidOid);

	bool flag = false;
	
	Form_meta_large tmpClass = NULL;

	PG_TRY(); {
		if(lo_exit(obj_desc->id, &tmpClass))
		{
			*formClass = fxdb_inv_update_lo(tmpClass, const_cast<void*>(extraData), len);
			pfree(tmpClass);
			CommandCounterIncrement();
		} else {
			return false;
		}
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}

	return true;
}

bool FDPG_LargeObject::fd_inv_get_lo_list(const Oid tblspace, const uint32 scan_num, 
																					uint32 *idx_loid, std::vector<lo_info> &v_lo_list)
{
	Assert(idx_loid != NULL);

	bool flag = false;

	bool ret_flag;

	PG_TRY(); {
		
		List *l = ListLo(tblspace, scan_num, idx_loid, &ret_flag);
		
		ListCell *lc = NULL;
		Form_meta_large formClass = NULL;
		lo_info lo;

		foreach(lc, l)
		{
			formClass = (Form_meta_large)lfirst(lc);
			lo.lo_id = formClass->lo_id;
			lo.lo_name.assign(formClass->lo_name.data);
			lo.tblspc = formClass->lo_tblspace;
			v_lo_list.push_back(lo);
		}

		list_free_deep(l);

	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}

	return ret_flag;
}

uint32 FDPG_LargeObject::fd_inv_name_get_id(char *name)
{
	bool flag = false;
	uint32 loid = 0;

	PG_TRY();{
		loid = name_get_loid(name);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}

	return loid;
}

std::string FDPG_LargeObject::fd_inv_id_get_name(const Oid lo_id)
{
	bool flag = false;
	std::string name;
        char* cname = NULL;
	PG_TRY();{
		cname = loid_get_name(lo_id);
                name = cname;
                free(cname);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}

	return name;
}

void FDPG_LargeObject::fd_at_endstatement_large_object()
{
	bool flag = false;

	PG_TRY();{
		Fxdb_AtEndStatement_LargeObject();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_tablespace_create_lorel(const Oid tbcid)
{
	bool flag = false;

	PG_TRY();{
		TablespaceCreateLORel(tbcid);
		DestoryLORelList();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_LargeObject::fd_tablespace_drop_lorel(const char *tbcname, const bool missing_ok)
{
	bool flag = false;

	PG_TRY();{
		Oid tbcid = get_tablespace_oid(tbcname, missing_ok);
		DropLORel(tbcid);
		DestoryLORelList();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_Lock::fd_spinlock_init(volatile slock_t &lock)
{
	bool flag = false;

	PG_TRY();{
		SpinLockInit(&lock);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_Lock::fd_LWLockAcquire(LWLockId lockid, LWLockMode mode)
{
	bool flag = false;

	PG_TRY();{
		LWLockAcquire(lockid,mode);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_Lock::fd_LWLockRelease(LWLockId lockid)
{
	bool flag = false;

	PG_TRY();{
		LWLockRelease(lockid);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_Lock::fd_LWLockReleaseAll(void)
{
	bool flag = false;

	PG_TRY();{
		LWLockReleaseAll();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

bool FDPG_Lock::fd_LWLockHeldAnyByMe(void)
{
	bool flag = false;
	bool ret = false;
	
	PG_TRY();{
		ret = LWLockHeldAnyByMe();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}

	return ret;
}

bool FDPG_Lock::fd_LWLockHeldByMe(LWLockId lockid)
{
	bool flag = false;
	bool ret = false;
	PG_TRY();{
		ret = LWLockHeldByMe(lockid);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return ret;
}

LWLockId FDPG_Lock::fd_LWLockAssign(void)
{
	bool flag = false;
	LWLockId ret  = MaxDynamicLWLock;

	PG_TRY();{
		ret = LWLockAssign();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return ret;
}

void FDPG_Lock::fd_DestroyLock(LWLockId lockid)
{
	bool flag = false;

	PG_TRY();{
		DestroyLock(lockid);
	} PG_CATCH(); {
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}

}

void FDPG_Lock::fd_spinlock_acquire(volatile slock_t &lock)
{
	bool flag = false;
	PG_TRY();{
		SpinLockAcquire(&lock);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

void FDPG_Lock::fd_spinlock_release(volatile slock_t &lock)
{
	bool flag = false;

	PG_TRY();{
		SpinLockRelease(&lock);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}
void FDPG_Lock::fd_InitLocks(void)
{
	bool flag = false;

	PG_TRY();{
		InitLocks();
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
}

LockAcquireResult FDPG_Lock::fd_lock_acqurie(const LOCKTAG *locktag,
								LOCKMODE lockmode,
								bool sessionLock,
								bool dontWait)
{
	bool flag = false;
	LockAcquireResult ret = LOCKACQUIRE_NOT_AVAIL;
	PG_TRY();{
		ret = LockAcquire(locktag,lockmode,sessionLock,dontWait);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}

	return ret;
}

bool FDPG_Lock::fd_lock_release(const LOCKTAG *locktag, 
								LOCKMODE lockmode, 
								bool sessionLock)
{
	bool flag = false;
	bool ret =false;
	
	PG_TRY();{
		ret = LockRelease(locktag,lockmode,sessionLock);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return ret;
}

bool FDPG_Lock::fd_LWLockConditionalAcquire(LWLockId lockid, LWLockMode mode)
{
	bool flag = false;
	bool ret =false;
	
	PG_TRY();{
		ret = LWLockConditionalAcquire(lockid,mode);
	} PG_CATCH(); { 
		flag = true;
	} PG_END_TRY();

	if(flag)
	{
		ThrowException();
	}
	return ret;
}

void FDPG_Memory::fd_pfree(void * pointer)
{	
	pfree(pointer);	
}

void * FDPG_Memory::fd_palloc0(Size sz)
{
	bool flag = false;
	void* ret = NULL;
	
	PG_TRY();{
		ret = palloc0(sz);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return ret;
}

void* FDPG_Memory::fd_palloc(Size sz)
{
	void* ret = NULL;	
	ret = palloc(sz);
	if(!ret)
		throw OutOfMemoryException();
	return ret;
}

void *FDPG_Memory::fd_repalloc(void *pointer, Size size)
{
	bool flag = false;
	void* ret = NULL;
	
	PG_TRY();{
		ret = repalloc(pointer, size);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return ret;
}

int FDPG_Memory::fd_printf(const char *fmt,...)
{
	bool flag = false;
	int ret = 0;
	
	PG_TRY();{
		ret = printf(fmt);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return ret;
}
int FDPG_Memory::fd_sprintf(char *str, const char *fmt,...)
{
		bool flag = false;
		int ret = 0;
		
		PG_TRY();{
			ret = sprintf(str,fmt);
		}PG_CATCH();{
			flag = true;
		}PG_END_TRY();
	
		if (flag)
		{
			ThrowException();
		}
		return ret;
}


#ifdef FOUNDER_XDB_SE
char* FDPG_Common::fd_tuple_to_chars(HeapTuple tuple)
{
	bool flag = false;
	char* ret = NULL;

	PG_TRY();{
		ret = fxdb_tuple_to_chars(tuple);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return ret;
	
}

Datum FDPG_Common::fd_string_formdatum(const char *p, size_t len)
{
	bool flag = false;

	PG_TRY();{
		return fdxdb_string_formdatum(p,len);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return (Datum)NULL;
}

char* FDPG_Common::fd_tuple_to_chars_with_len(HeapTuple tuple, int &len)
{
	bool flag = false;
	char* ret = NULL;

	PG_TRY();{
		ret = fdxdb_tuple_to_chars_with_len(tuple,len);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return ret;
}
#endif

#ifdef FOUNDER_XDB_SE
void FDPG_ScanKey::fd_ScanKeyInitWithCallbackInfo(ScanKey entry,
			AttrNumber attributeNumber,
			StrategyNumber strategy,
			CompareCallback compareFunction,
			Datum argument)
{
	bool flag = false;

	PG_TRY();{
		Fdxdb_ScanKeyInitWithCallbackInfo(entry,attributeNumber,strategy,compareFunction,argument);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
}
#endif

FILE* FDPG_File::fd_fopen(const char *fileName, const char *mode)
{
	bool flag = false;

	PG_TRY();{
		return fopen(fileName,mode);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return NULL;
}



void FDPG_Database::fd_GetSeqTableByDbid(Oid dbid, Oid& seqRelId, Oid& seqIdIdxId, Oid& seqNameIdxId)
{
	bool flag = false;

	PG_TRY();{
		GetSeqTableByDbid(dbid, seqRelId, seqIdIdxId, seqNameIdxId);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_GetCurrSeqTable(Oid& seqRelId, Oid& seqIdIdxId, Oid& seqNameIdxId)
{
	bool flag = false;

	PG_TRY();{
		GetCurrSeqTable(seqRelId, seqIdIdxId, seqNameIdxId);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_InsertSeqInfo(Oid seqRelId, Oid seqId, const char *seqName, int64 value,
					bool hasRange, int64 minValue, int64 maxValue, uint32 flags)
{
	bool flag = false;

	PG_TRY();{
		InsertSeqInfo(seqRelId, seqId, seqName, value, hasRange, minValue, maxValue, flags);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_DeleteSeqInfoById(Oid seqRelId, Oid seqIdIdxId, Oid seqId)
{
	bool flag = false;

	PG_TRY();{
		DeleteSeqInfoById(seqRelId, seqIdIdxId, seqId);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_DeleteSeqInfo(Oid seqRelId, Oid seqNameIdxId,
	const char *seqName, Oid& seqId)
{
	bool flag = false;

	PG_TRY();{
		DeleteSeqInfo(seqRelId, seqNameIdxId, seqName, seqId);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_GetMaxSeqId(Oid seqRelId, Oid seqIdIdxId, Oid& maxSeqId)
{
	bool flag = false;
	
	PG_TRY();{
		GetMaxSeqId(seqRelId, seqIdIdxId, maxSeqId);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_GetSeqInfoById(Oid seqRelId, Oid seqIdIdxId, Oid seqId,
					std::string& seqName, int64& value, bool& hasRange, int64& minValue,
					int64& maxValue, uint32& flags)
{
	bool flag = false;

	PG_TRY();{
		char *cSeqName = NULL;
		GetSeqInfoById(seqRelId, seqIdIdxId, seqId, &cSeqName, value, hasRange, minValue, maxValue, flags);
		seqName.clear();
		seqName.append(cSeqName);
		free(cSeqName);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_GetSeqInfo(Oid seqRelId, Oid seqNameIdxId, const char *seqName,
					Oid& seqId, int64& value, bool& hasRange, int64& minValue,
					int64& maxValue, uint32& flags)
{
	bool flag = false;

	PG_TRY();{
		GetSeqInfo(seqRelId, seqNameIdxId, seqName, seqId, value, hasRange, minValue, maxValue, flags);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_GetSeqValueById(Oid seqRelId, Oid seqIdIdxId, Oid seqId, int64& value)
{
	bool flag = false;

	PG_TRY();{
		GetSeqValueById(seqRelId, seqIdIdxId, seqId, value);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_UpdateSeqValueById(Oid seqRelId, Oid seqIdIdxId,
	Oid seqId, bool isDeltaValue, int64 value, int64& oldValue)
{
	bool flag = false;

	PG_TRY();{
		UpdateSeqValueById(seqRelId, seqIdIdxId, seqId, isDeltaValue, value, oldValue);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_VacuumDatabase(void)
{
	bool flag = false;

	PG_TRY();{
		vacuum_database();
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

void FDPG_Database::fd_VacuumAll(void)
{
	bool flag = false;

	PG_TRY();{
		vacuum_all();
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	return;
}

bool FDPG_XLog::fd_XLogArchiveIsBusy(const char *xlog)
{
	bool flag = false;
	bool result = false;

	PG_TRY();{
		result = XLogArchiveIsBusy(xlog);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	
	return result;
}

bool FDPG_XLog::fd_FileCanArchive(const char *filename)
{
	bool flag = false;
	bool result = false;

	PG_TRY();{
		result = pgarch_fileCanArchive(filename);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
	
	return result;
}

void FDPG_XLog::fd_SetControlFileNodeId(uint32 nodeId)
{
	bool flag = false;

	PG_TRY();{
		SetControlFileNodeId(nodeId);
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}
}	

uint32 FDPG_XLog::fd_GetControlFileNodeId()
{
	bool flag = false;
	uint32 res = 0;
	
	PG_TRY();{
		res = GetControlFileNodeId();
	}PG_CATCH();{
		flag = true;
	}PG_END_TRY();

	if (flag)
	{
		ThrowException();
	}

	return res;
}

void FDPG_XLog::fd_SetWalSenderFunc(WalSenderFunc copyfunc, WalSenderFunc removefunc)
{
	SetWalSenderFunc(copyfunc, removefunc);
}

void FDPG_XLog::fd_SyncRepSetCancelWait(void)
{
	SyncRepSetCancelWait();
}