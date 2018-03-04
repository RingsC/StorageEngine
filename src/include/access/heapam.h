/*-------------------------------------------------------------------------
 *
 * heapam.h
 *	  POSTGRES heap access method definitions.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/heapam.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef HEAPAM_H
#define HEAPAM_H

#include "access/htup.h"
#include "access/sdir.h"
#include "access/skey.h"
#include "access/xlog.h"
//#include "nodes/primnodes.h"
#include "storage/bufpage.h"
#include "storage/lock.h"
#include "utils/relcache.h"
#include "utils/snapshot.h"
#include "postmaster/xdb_main.h"
#include "nodes/tidbitmap.h"
#ifdef FOUNDER_XDB_SE
#include "access/xdb_common.h"
#endif //FOUNDER_XDB_SE

/* "options" flag bits for heap_insert */
#define HEAP_INSERT_SKIP_WAL	0x0001
#define HEAP_INSERT_SKIP_FSM	0x0002

typedef struct BulkInsertStateData *BulkInsertState;
typedef struct BitmapHeapScanState BitmapHeapScanState;

typedef enum
{
	LockTupleShared,
	LockTupleExclusive
} LockTupleMode;


/* ----------------
 *		function prototypes for heap access method
 *
 * heap_create, heap_create_with_catalog, and heap_drop_with_catalog
 * are declared in catalog/heap.h
 * ----------------
 */

/* in heap/heapam.c */
#ifndef FOUNDER_XDB_SE
extern Relation relation_open(Oid relationId, LOCKMODE lockmode);
#else
extern void CreateCacheMemoryContext(void);
extern THREAD_LOCAL Oid MyDatabaseId;
extern Relation relation_open(Oid relationId, LOCKMODE lockmode,Oid dbid = MyDatabaseId);
#endif //FOUNDER_XDB_SE
extern Relation try_relation_open(Oid relationId, LOCKMODE lockmode);
//extern Relation relation_openrv(const RangeVar *relation, LOCKMODE lockmode);
//extern Relation try_relation_openrv(const RangeVar *relation, LOCKMODE lockmode);
extern void relation_close(Relation relation, LOCKMODE lockmode);
#ifndef FOUNDER_XDB_SE
extern Relation heap_open(Oid relationId, LOCKMODE lockmode);
#else
extern Relation heap_open(Oid relationId, LOCKMODE lockmode,Oid dbid = MyDatabaseId);
extern Oid fdxdb_heap_insert(Relation heapRelation, Relation indexRelation, HeapTuple tup);
extern Datum fdxdb_form_index_datum(Relation heapRelation, Relation indexRelation, HeapTuple tup);
#endif //FOUNDER_XDB_SE
//extern Relation heap_openrv(const RangeVar *relation, LOCKMODE lockmode);
//extern Relation try_heap_openrv(const RangeVar *relation, LOCKMODE lockmode);

#define heap_close(r,l)  relation_close(r,l)

/* struct definition appears in relscan.h */
typedef struct HeapScanDescData *HeapScanDesc;

/*
 * HeapScanIsValid
 *		True iff the heap scan is valid.
 */
#define HeapScanIsValid(scan) PointerIsValid(scan)
typedef bool (*filter_func_t)(void*userdata, const void*idxKeyBytes, size_t keylen);
extern HeapScanDesc heap_beginscan(Relation relation, Snapshot snapshot,
			   int nkeys, ScanKey key, filter_func_t filterfunc, void* userdata,BlockNumber start = 0, BlockNumber end = 0);
extern HeapScanDesc heap_beginscan_strat(Relation relation, Snapshot snapshot,
					 int nkeys, ScanKey key,
					 bool allow_strat, bool allow_sync, filter_func_t filterfunc, void* userdata);
extern HeapScanDesc heap_beginscan_bm(Relation relation, Snapshot snapshot,
				  int nkeys, ScanKey key, filter_func_t filterfunc, void* userdata);
extern void heap_rescan(HeapScanDesc scan, ScanKey key);
extern void heap_endscan(HeapScanDesc scan);
extern HeapTuple heap_getnext(HeapScanDesc scan, ScanDirection direction);

extern bool heap_fetch(Relation relation, Snapshot snapshot,
		   HeapTuple tuple, Buffer *userbuf, bool keep_buf,
		   Relation stats_relation);
extern bool heap_hot_search_buffer(ItemPointer tid, Relation relation,
					   Buffer buffer, Snapshot snapshot, bool *all_dead);
extern bool heap_hot_search(ItemPointer tid, Relation relation,
				Snapshot snapshot, bool *all_dead);

extern void heap_get_latest_tid(Relation relation, Snapshot snapshot,
					ItemPointer tid);
extern void setLastTid(const ItemPointer tid);

extern BulkInsertState GetBulkInsertState(void);
extern void FreeBulkInsertState(BulkInsertState);

extern Oid heap_insert(Relation relation, HeapTuple tup, CommandId cid,
			int options, BulkInsertState bistate);
extern HTSU_Result heap_delete(Relation relation, ItemPointer tid,
			ItemPointer ctid, TransactionId *update_xmax,
			CommandId cid, Snapshot crosscheck, bool wait);
extern HTSU_Result heap_update(Relation relation, ItemPointer otid,
			HeapTuple newtup,
			ItemPointer ctid, TransactionId *update_xmax,
			CommandId cid, Snapshot crosscheck, bool wait);
extern HTSU_Result heap_lock_tuple(Relation relation, HeapTuple tuple,
				Buffer *buffer, ItemPointer ctid,
				TransactionId *update_xmax, CommandId cid,
				LockTupleMode mode, bool nowait);
extern void heap_inplace_update(Relation relation, HeapTuple tuple);
extern bool heap_freeze_tuple(HeapTupleHeader tuple, TransactionId cutoff_xid,
				  Buffer buf);
#ifdef FOUNDER_XDB_SE
void insert_tuple_to_indexes(Relation relation,HeapTuple tup);
#endif //FOUNDER_XDB_SE
extern Oid	simple_heap_insert(Relation relation, HeapTuple tup);
#ifdef FOUNDER_XDB_SE
extern void simple_heap_delete(Relation relation, ItemPointer tid);
extern void simple_heap_update(Relation relation, ItemPointer otid,
				   HeapTuple tup);
#else
extern void simple_heap_delete(Relation relation, ItemPointer tid);
extern void simple_heap_update(Relation relation, ItemPointer otid,
				   HeapTuple tup);

#endif

extern void heap_markpos(HeapScanDesc scan);
extern void heap_restrpos(HeapScanDesc scan);

extern void heap_sync(Relation relation);

extern void heap_redo(XLogRecPtr lsn, XLogRecord *rptr);
extern void heap_desc(StringInfo buf, uint8 xl_info, char *rec);
extern void heap2_redo(XLogRecPtr lsn, XLogRecord *rptr);
extern void heap2_desc(StringInfo buf, uint8 xl_info, char *rec);

extern XLogRecPtr log_heap_cleanup_info(RelFileNode rnode,
					  TransactionId latestRemovedXid);
extern XLogRecPtr log_heap_clean(Relation reln, Buffer buffer,
			   OffsetNumber *redirected, int nredirected,
			   OffsetNumber *nowdead, int ndead,
			   OffsetNumber *nowunused, int nunused,
			   TransactionId latestRemovedXid);
extern XLogRecPtr log_heap_freeze(Relation reln, Buffer buffer,
				TransactionId cutoff_xid,
				OffsetNumber *offsets, int offcnt);
extern XLogRecPtr log_newpage(RelFileNode *rnode, ForkNumber forkNum,
			BlockNumber blk, Page page);

/* in heap/pruneheap.c */
extern void heap_page_prune_opt(Relation relation, Buffer buffer,
					TransactionId OldestXmin);
extern int heap_page_prune(Relation relation, Buffer buffer,
				TransactionId OldestXmin,
				bool report_stats, TransactionId *latestRemovedXid);
extern void heap_page_prune_execute(Buffer buffer,
						OffsetNumber *redirected, int nredirected,
						OffsetNumber *nowdead, int ndead,
						OffsetNumber *nowunused, int nunused);
extern void heap_get_root_tuples(Page page, OffsetNumber *root_offsets);

/* in heap/syncscan.c */
extern void ss_report_location(Relation rel, BlockNumber location);
extern BlockNumber ss_get_location(Relation rel, BlockNumber relnblocks);
extern void SyncScanShmemInit(void);
extern Size SyncScanShmemSize(void);
extern void heap_multi_insert(Relation relation, HeapTuple *tuples, int ntuples,BulkInsertState bistate);
extern void index_multi_insert(Relation relation,HeapTuple* tup,int tup_count);
extern BitmapHeapScanState* ExecInitBitmapHeapScan(Relation currentRelation, TIDBitmap *tbm, Snapshot snapshot,int nKeys, ScanKey key);
extern BitmapHeapScanState *ExecInitBitmapHeapScanNew(void *pIndexScan, TIDBitmap *tbm);
extern void ExecEndBitmapHeapScan(BitmapHeapScanState *node);
extern HeapTuple BitmapHeapNext(BitmapHeapScanState *node);
extern void insert_tuple_to_index(Relation heapRel, Relation indexRel, HeapTuple tup);
#endif   /* HEAPAM_H */