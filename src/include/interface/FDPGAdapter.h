
#ifndef FD_PG_ADAPTER_H
#define FD_PG_ADAPTER_H

#include "postgres.h"
#include "utils/rel.h"
#include "storage/lmgr.h"
#include "access/htup.h"
#include "access/heapam.h"
#include "access/genam.h"
#include "catalog/index.h"
#include "fmgr.h"
#include "utils/inval.h"
#include "storage/s_lock.h"
#include "nodes/parsenodes.h"
#include "access/xact.h"
#include "storage/lock.h"
#include "nodes/parsenodes.h"
#include "access/xlog.h"
#include "replication/walsender.h"

#include <vector>
#include <string>

/// @file TDPGAdapter.h
/// @brief Wraps all PGStorageEngine functions, convert C exceptions to C++ exceptions.
/// All other code inside TDStorageEngine must always use this header file to call PGStorageEngine
/// functions, never directly include PGStorageEngine header files or call its functions.
struct Tuplesortstate;
struct Tuplestorestate;
struct LargeObjectDesc;
struct Form_meta_large_data;

class FDPG_StorageEngine
{
public:
    static void fd_start_engine(const char *path, const uint32 thread_num, 
			bool initDatadir, storage_params * params);
    static void fd_end_engine();
	static unsigned int getNumberOfPages(Relation relation);
	static bool fd_start_archive( void );
	static bool fd_end_archive( void ); 
	static void fd_start_backup(const char* strLabel, bool fast );
	static void fd_stop_backup( void );
	static uint32 fd_getCurrentTimeLine( void );
	static void fd_list_all_tablespc(std::vector<std::pair<std::string, std::string> > &v_tablespc);
	static bool fd_am_master();
	static void fd_reg_trigger_func(TriggerFunc func);
	static void fd_interrupt_start_engine(int code);
};

class FDPG_Heap
{
public:
	static void fd_heap_drop(Oid relationId, Oid dbid = 0);

	static Oid fd_heap_create(Oid reltablespace, Oid relid,
		uint32 dbid = 0, uint32 colid = 0);
	static Oid fd_temp_heap_create(Oid reltablespace, uint32 dbid = 0, 
		uint32 colid = 0, OnCommitAction action = ONCOMMIT_DROP);
	static void fd_heap_close(Relation relation, LOCKMODE lockmode);
	static HeapTuple fd_heap_form_tuple(const char *p, size_t len, uint32 nEids = 0, Oid *pEids = NULL);
	static HeapTuple heap_tuple_copy_bytearray_straight(char *row, uint32 rowlength, uint32 *newlen);
	static Relation fd_heap_open(Oid relationId, LOCKMODE lockmode, Oid dbid = 0);
	static Oid fd_simple_heap_insert(Relation relation, HeapTuple tup);
	static void fd_heap_multi_insert(Relation relation, HeapTuple* tup, int tup_count, BulkInsertState bis);
	static Oid fd_heap_insert_bulk(Relation relation, HeapTuple tup, BulkInsertState bis);
	static Oid fd_heap_insert_bulk(Relation relation, HeapTuple *tup, uint32 tupnum, BulkInsertState bis);
	static void fd_simple_heap_delete(Relation relation, ItemPointer tid);
	static void fd_simple_heap_update(Relation relation, ItemPointer otid, HeapTuple tup);
    static void fd_heap_inplace_update(Relation relation, HeapTuple tup);
    static void fd_fxdb_SubPostmaster_Main(void *param);
    static void fd_proc_exit(int code);
	static bool fd_heap_fetch(Relation relation,Snapshot snapshot,HeapTuple tuple,
								Buffer *userbuf,bool keep_buf,Relation stats_relation);
	static void fd_heap_truncate(Relation relation);
	static struct varlena *fd_toast_uncompress_datum(struct varlena * attr);

	//// scan API
	static HeapScanDesc fd_heap_beginscan(Relation relation, Snapshot snapshot,
	           int nkeys, ScanKey key, filter_func_t filterFunc = 0, void*userData = 0,ItemPointer otid = 0,ItemPointer endid = 0);
	static HeapTuple fd_heap_getnext(HeapScanDesc scan, ScanDirection direction);
	static void fd_heap_endscan(HeapScanDesc scan);
	static void fd_heap_markpos(HeapScanDesc scan);
	static void fd_heap_restrpos(HeapScanDesc scan);
	
	static unsigned int fd_metatbl_get_max_id();
	static unsigned int fd_metatbl_get_heapid(Oid index_id);
	static bool fd_metatbl_insert_meta(Oid object_id, int type, Oid parent_table_id, pg_time_t time, 
																		 int owner, Oid database_id, Oid tablespace_id, unsigned int colinfo_id);
	static bool fd_metatbl_delete_meta(Oid object_id);
	static bool fd_metatbl_get_heap_info(Oid  m_index_entry_id, ColinfoData& colinfo);

	static BulkInsertState fd_GetBulkInsertState(void);
	static void fd_FreeBulkInsertState(BulkInsertState bistate);
	static void fd_ClusterRel(Oid relid, Oid idxid, bool use_sort);

	static void fd_VacuumRelations(int nrels, Oid *relids);
};

struct ReIndexesStat;
class FDPG_Index
{
public:
	static void fd_index_drop(Oid relid, Oid tblspcid, Oid indexId, Oid dbid = 0); //
	static void fd_index_close(Relation relation, LOCKMODE lockmode); //
	static Oid fd_index_create(Relation heapRelation, IndexType idxtype,Oid reloid,uint32 colid,
				void* userData = 0, filter_func_t filterfunc = 0, uint32 userdataLength = 0, bool skipInsert = false); 
	static Relation fd_index_open(Oid relationId, LOCKMODE lockmode, Oid dbid = 0); //
	static bool fd_index_insert(Relation indexRelation,
			 Datum *values,
			 bool *isnull,
			 ItemPointer heap_t_ctid,
			 Relation heapRelation,
			 bool check_uniqueness); //
	static void fd_index_endscan(IndexScanDesc scan);//
	static IndexScanDesc fd_index_beginscan(Relation heapRelation,
				Relation indexRelation,
				Snapshot snapshot,
				int nkeys, ScanKey key, filter_func_t filterFunc = 0, void*userData = 0);//
	static void fd_index_rescan(IndexScanDesc scan, ScanKey keys, int nkeys, ScanKey orderbys, int norderbys);
	static HeapTuple fd_index_getnext(IndexScanDesc scan, ScanDirection direction);
	static void fd_index_markpos(IndexScanDesc scan);
	static void fd_index_restrpos(IndexScanDesc scan);
	static void fd_reindex_index(Oid heapId, Oid indexId, Oid dbid);
	static bool fd_reindex_relation(Oid relid, Oid dbid, int flags);
	static void fd_reindex_database(Oid dbid);
	static void fd_index_getrange(Relation heapRelation, Relation indexRelation,
		std::string &min, std::string &max, Snapshot snapshot);
	static void fd_index_insert_indexes(ReIndexesStat& IndexesStat);
	static ReIndexesStat* fd_index_init_reIndexes(Relation heap, const std::vector<Relation>& indexes);
	static void fd_index_do_build_indexes(ReIndexesStat& stat);
	static void fd_index_destory_reIndexes(ReIndexesStat* stat);
};

class FDPG_Transaction
{
public:
//	static CommandId fd_GetCurrentCommandId(bool);
    static Snapshot fd_GetTransactionSnapshot();
    static void fd_StartTransactionCommand();
//	static void fd_RollbackToSavepoint(const char* name);
    static bool fd_EndTransactionBlock();    
    static void fd_CommitTransactionCommand();
//	static void fd_DefineSavepoint(const char *name);
	static void	fd_UserAbortTransactionBlock();
	static void fd_AbortCurrentTransaction();
	static void fd_CommandCounterIncrement(void);
	static void fd_BeginTransactionBlock(void);
    static void fd_AbortOutOfAnyTransaction(void);
	static void fd_PreCommit_on_commit_actions(void);
	//for subtransaction
	static void fd_BeginSubTransaction( char *name );
	static void fd_CommitSubTransaction( void );
	static void fd_AbortSubTransaction( void );
	#ifdef FOUNDER_XDB_SE
	static void fd_setXactIsoLevel(int level);
	static int fd_getXactIsoLevel(void);
	static TransactionState fd_get_current_transaction_state(void);
	static TransactionState fd_get_top_transaction_state(void);
	static void fd_set_top_transaction_state(TransactionState top_stransaction_state);
	static void fd_set_current_transaction_state(TransactionState current_stransaction_state);
	#endif
	static bool fd_PrepareTransactionBlock(char *gid);
	static bool fd_IsTransactionBlock();
	static void fd_FinishPreparedTransaction(const char *gid, const bool isCommit);
	static bool fd_GlobalTransactionIdIsPrepared(char *gxid);
	static uint32 fd_GetNextMaxPreparedGlobalTxnId();
};

// Placeholders in PGStorageEngine code files for PG_TRY, PG_CATCH, PG_END_TRY, in case we need to
// know this information.
//#define MY_PG_END_TRY_TAG 
//#define MY_PG_TRY_TAG 
//#define MY_PG_CATCH_TAG

// Must call PG_PFREE to free memory allocated by FDPG_StorageEngine::fd_palloc, etc.
//#define PG_PFREE(ptr) do { FDPG_StorageEngine::fd_pfree(ptr); ptr = NULL; } while (0)

class FDPG_TupleStore
{
public:
	static Tuplestorestate *
		fd_tuplestore_begin_heap(bool randomAccess, bool interXact, int maxKBytes/*,Oid tempTableSpaceOid*/);
	static void
		fd_tuplestore_end(Tuplestorestate *state);
	static void
		fd_tuplestore_puttuple_common(Tuplestorestate *state, void *data/*, unsigned int datalen*/);
	static int
		fd_tuplestore_alloc_read_pointer(Tuplestorestate *state, int eflags);
	static void
		fd_tuplestore_rescan(Tuplestorestate *state);
	static bool
		fd_tuplestore_ateof(Tuplestorestate *state);
	static void *
		fd_tuplestore_gettuple(Tuplestorestate *state, bool forward,
		bool *should_free);
	static void
		fd_tuplestore_copy_read_pointer(Tuplestorestate *state,
		int srcptr, int destptr);
	static void
		fd_tuplestore_trim(Tuplestorestate *state);

	static HeapTuple 
		fd_heap_tuple_from_minimal_tuple(MinimalTuple mtup);
	static void *
		fd_minimal_tuple_getdata(MinimalTuple mtup,int &len);

};

class FDPG_TupleSort
{
public:
	static Tuplesortstate *fd_tuplesort_begin_data(int workMem, 
		bool randomAccess, 
		void* compare_func);
	static Tuplesortstate *fd_tuplesort_begin_heap(int workMem, 
		bool randomAccess, 
		void *compare_func);
	static void fd_tuplesort_putdata(Tuplesortstate *state, void* data, size_t len);
	static void fd_tuplesort_put_heaptuple(Tuplesortstate *state, HeapTuple tup);

	static void fd_tuplesort_rescan(Tuplesortstate *state);
	static void fd_tuplesort_performsort(Tuplesortstate *state, void *compare_func);
	static void fd_tuplesort_end(Tuplesortstate *state);
	static bool fd_tuplesort_support_random_access(Tuplesortstate *state);
	static bool fd_tuplesort_getdata(Tuplesortstate *state, bool forward, void** pData, int *len);
	static bool fd_tuplesort_getheaptuple(Tuplesortstate *state, bool forward, HeapTuple *tuple, bool* should_free);
	static void fd_tuplesort_markpos(Tuplesortstate *state);
	static void fd_tuplesort_restorepos(Tuplesortstate *state);
	static void fd_tuplesort_setsorted(Tuplesortstate *state);
};

class FDPG_LargeObject
{
public:
	struct lo_info
	{
		std::string lo_name;
		uint32 lo_id;
		uint32 tblspc;
	};

public:
	static void fd_inv_create(const char *name,
		const uint32 extra_data_len,
		const void *extra_data,
		uint32 &lo_id);
	static void fd_inv_open(const uint32 lobjId, const int flags,
		MemoryContext mcxt, LargeObjectDesc **lod);
	static void fd_inv_open(const char *name, const int flags,
		MemoryContext mcxt, LargeObjectDesc **lod);
	static void fd_inv_close(LargeObjectDesc *obj_desc);
	static void fd_inv_write(LargeObjectDesc *obj_desc, const char *buf, 
		const int nbytes, int &nwritten);
	static void fd_inv_read(LargeObjectDesc *obj_desc, const int nbytes, 
		char *buf, int &nread);
	static void fd_inv_truncate(LargeObjectDesc *obj_desc, const int64 len);
	static void fd_inv_drop(const uint32 lo_id, bool &retval);
	static void fd_inv_drop(const char *name, bool &retval);
	static void fd_inv_tell(LargeObjectDesc *obj_desc, uint64 &offset);
	static void fd_inv_seek(LargeObjectDesc *obj_desc, const int64 offset, 
		const int whence, uint64 &roffset);
	static void fd_inv_getmeta(LargeObjectDesc *obj_desc, Form_meta_large_data **formClass);
	static void fd_inv_metaget_size(Form_meta_large_data *formClass, size_t &len);
	static void fd_inv_metaget_extradata(Form_meta_large_data *formClass, void **data);
	static void fd_inv_metaget_name(Form_meta_large_data *formClass, char **name);
	static void fd_inv_metaget_id(Form_meta_large_data *formClass, Oid &id);
	static bool fd_inv_metaupdate_extradata(LargeObjectDesc *obj_desc, const void *extraData, 
		size_t len, Form_meta_large_data **formClass);
	static bool fd_inv_get_lo_list(const Oid tblspace, const uint32 scan_num, 
		uint32 *idx_loid, std::vector<lo_info> &v_lo_list);
	static uint32 fd_inv_name_get_id(char *name);
	static std::string fd_inv_id_get_name(const Oid lo_id);
	static void fd_at_endstatement_large_object();
	static void fd_tablespace_create_lorel(const Oid tbcid);
	static void fd_tablespace_drop_lorel(const char *tbcname, const bool missing_ok);
};

class FDPG_Lock
{
public:
	static void fd_spinlock_init(volatile slock_t &lock);
	static void fd_spinlock_acquire(volatile slock_t &lock);
	static void fd_spinlock_release(volatile slock_t &lock);
	static void fd_InitLocks(void);
	static LockAcquireResult fd_lock_acqurie(const LOCKTAG *locktag,
									LOCKMODE lockmode,
									bool sessionLock,
									bool dontWait);
	static bool fd_lock_release(const LOCKTAG *locktag, 
									LOCKMODE lockmode, 
									bool sessionLock);
	static bool fd_LWLockConditionalAcquire(LWLockId lockid, LWLockMode mode);
	static void fd_LWLockAcquire(LWLockId lockid, LWLockMode mode);
	static void fd_LWLockRelease(LWLockId lockid);
	static void fd_LWLockReleaseAll(void);
	static bool fd_LWLockHeldAnyByMe(void);
	static bool fd_LWLockHeldByMe(LWLockId lockid);
	static LWLockId fd_LWLockAssign(void);
	static void fd_DestroyLock(LWLockId lockId);
};

class FDPG_Memory
{
public:
	static void fd_pfree(void *pointer);
	static void* fd_palloc0(Size sz);
	static void* fd_palloc(Size sz);
	static void *fd_repalloc(void *pointer, Size size);
	
	static int fd_printf(const char *fmt,...);
	static int fd_sprintf(char *str, const char *fmt,...);
};

#ifdef FOUNDER_XDB_SE
class FDPG_Common
{
public:
	static char* fd_tuple_to_chars(HeapTuple tuple);
	static Datum fd_string_formdatum(const char *p, size_t len);
	static char* fd_tuple_to_chars_with_len(HeapTuple tuple, int &len);
};
#endif

class FDPG_ScanKey
{
public:
	#ifdef FOUNDER_XDB_SE
	static void fd_ScanKeyInitWithCallbackInfo(ScanKey entry,
			AttrNumber attributeNumber,
			StrategyNumber strategy,
			CompareCallback compareFunction,
			Datum argument);
	#endif
};

class FDPG_File
{
public:
	static FILE* fd_fopen(const char *fileName, const char *mode);
};

class FDPG_Database
{
public:
	static void fd_GetSeqTableByDbid(Oid dbid, Oid& seqRelId, Oid& seqIdIdxId, Oid& seqNameIdxId);
	static void fd_GetCurrSeqTable(Oid& seqRelId, Oid& seqIdIdxId, Oid& seqNameIdxId);
	static void fd_InsertSeqInfo(Oid seqRelId, Oid seqId, const char *seqName, int64 value,
					bool hasRange, int64 minValue, int64 maxValue, uint32 flags);
	static void fd_DeleteSeqInfoById(Oid seqRelId, Oid seqIdIdxId, Oid seqId);
	static void fd_DeleteSeqInfo(Oid seqRelId, Oid seqNameIdxId, const char *seqName, Oid& seqId);
	static void fd_GetMaxSeqId(Oid seqRelId, Oid seqIdIdxId, Oid& maxSeqId);

	static void fd_GetSeqInfoById(Oid seqRelId, Oid seqIdIdxId, Oid seqId,
					std::string& seqName, int64& value, bool& hasRange, int64& minValue,
					int64& maxValue, uint32& flags);
	static void fd_GetSeqInfo(Oid seqRelId, Oid seqNameIdxId, const char *seqName,
					Oid& seqId, int64& value, bool& hasRange, int64& minValue,
					int64& maxValue, uint32& flags);
	static void fd_GetSeqValueById(Oid seqRelId, Oid seqIdIdxId, Oid seqId, int64& value);
	static void fd_UpdateSeqValueById(Oid seqRelId, Oid seqIdIdxId,
					Oid seqId, bool isDeltaValue, int64 delta, int64& oldValue);
	static void fd_VacuumDatabase(void);
	static void fd_VacuumAll(void);
};

class FDPG_XLog
{
public:
	static bool fd_XLogArchiveIsBusy(const char *xlog);
	static bool fd_FileCanArchive(const char *filename);
	static void fd_SetControlFileNodeId(uint32 nodeId);
	static uint32 fd_GetControlFileNodeId();
	static void fd_SetWalSenderFunc(WalSenderFunc copyfunc, WalSenderFunc removefunc);
	static void fd_SyncRepSetCancelWait(void);
};

#endif // !FD_PG_ADAPTER_H