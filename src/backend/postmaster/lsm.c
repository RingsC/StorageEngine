#include "postgres.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "access/heapam.h"
#include "access/reloptions.h"
#include "access/transam.h"
#include "access/xact.h"
#ifndef FOUNDER_XDB_SE
#include "catalog/dependency.h"
#include "catalog/namespace.h"
#include "commands/dbcommands.h"
#else 
#include "utils/rel.h"
#include "commands/xdb_command.h"
#endif //FOUNDER_XDB_SE
#include "catalog/pg_database.h"
#include "commands/vacuum.h"
#ifndef FOUNDER_XDB_SE
#include "libpq/pg_signal.h"
#else
#include "port/thread_commu.h"
#include "catalog/xdb_catalog.h"
#endif
#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/autovacuum.h"
#include "postmaster/fork_process.h"
#include "postmaster/postmaster.h"
#include "storage/bufmgr.h"
#include "storage/ipc.h"
#include "storage/smgr.h"
#include "storage/pmsignal.h"
#include "storage/proc.h"
#include "storage/procsignal.h"
#include "storage/sinvaladt.h"
#include "tcop/tcopprot.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#ifndef FOUNDER_XDB_SE
#include "utils/ps_status.h"
#endif
#include "utils/snapmgr.h"
#include "utils/syscache.h"
#include "utils/tqual.h"
#include "utils/tuplesort.h"

extern fxdb_thread_t *lsm_merge_handle;
extern fxdb_thread_t *lsm_freeze_handle;
extern fxdb_thread_t * fxdb_Thread_Create(void *(*)(void*), void *);
extern void fxdb_save_thread_variables(BackendParameters *, BackendId, ThreadType);
extern Oid __fxdb_index_create(Oid relid, Relation heapRelation, IndexType indextype,
	Oid coliId, Relation parentRelation, void* userData, filter_func_t filterfunc, uint32 userDataLength, bool skipInsert = false);
extern void RelationTruncateIndex(Relation heapRelation, Oid indexId);
extern void insert_tuple_to_index(Relation heapRel, Relation indexRel, HeapTuple tup);
extern void CacheInvalidateRelcache(Relation relation);
extern TransactionId GetOldestXmin(bool allDbs, bool ignoreVacuum);
extern void index_build(Relation heapRelation, Relation indexRelation,
	IndexInfo *indexInfo, bool isprimary, bool isreindex,void* userData =0 , filter_func_t filterfunc = 0);
extern IndexInfo *BuildIndexInfo(Relation index);
extern void reindex_index(Oid heapId, Oid indexId, Oid dbid);

BlockNumber LsmSmgrNblocks(Oid relid, Oid dbid, Oid tablespaceid)
{
	BlockNumber nblocks = InvalidBlockNumber;
	RelFileNode rnode;
	SMgrRelation reln = NULL;

	rnode.dbNode = dbid;
	rnode.spcNode = tablespaceid;
	rnode.relNode = relid;
	reln = smgropen(rnode, InvalidBackendId);
	// TODO: MAIN_FORKNUM
	nblocks = smgrnblocks(reln, MAIN_FORKNUM);
	smgrclose(reln);

	Assert(nblocks != InvalidBlockNumber);
	return nblocks;
}

TransactionId LsmNextTransactionId(void)
{
	return ReadNewTransactionId();
}

TransactionId LsmGetOldestTransactionId(void)
{
	return GetOldestXmin(true, false);
}

//uint32 g_lsm_tree_threshold_lsm = 1024 * 8;
uint32 g_lsm_subtree_threshold_lsm = 1024 * 8;
MemoryContext LsmContext = NULL;















/*******************************************************************************
**                LsmData start
*******************************************************************************/
typedef struct LsmData
{
	Oid lsmIdxId;
	Oid lsmActiveIdxId;
	slock_t slock;
}LsmData;

static void LsmDataInit(LsmData *pLsmData, Oid lsmActiveIdxId)
{
	Assert(pLsmData != NULL);
	Assert(lsmActiveIdxId != InvalidOid);
	SpinLockInit(&pLsmData->slock);
	pLsmData->lsmActiveIdxId = lsmActiveIdxId;
}

static void LsmDataDestory(LsmData *pLsmData, Oid lsmActiveIdxId)
{
	Assert(pLsmData != NULL);
	SpinLockFree(&pLsmData->slock);
	pLsmData->lsmActiveIdxId = lsmActiveIdxId;
}

static void LsmDataSetActiveIdxId(LsmData *pLsmData, Oid lsmActiveIdxId)
{
	Assert(pLsmData != NULL);
	Assert(lsmActiveIdxId != InvalidOid);
	SpinLockAcquire(&pLsmData->slock);
	pLsmData->lsmActiveIdxId = lsmActiveIdxId;
	SpinLockRelease(&pLsmData->slock);
}

static Oid LsmDataGetActiveIdxId(LsmData *pLsmData)
{
	Assert(pLsmData != NULL);

	Oid lsmActiveIdxId;

	SpinLockAcquire(&pLsmData->slock);
	lsmActiveIdxId = pLsmData->lsmActiveIdxId;
	SpinLockRelease(&pLsmData->slock);

	Assert(lsmActiveIdxId != InvalidOid);
	return lsmActiveIdxId;
}

HTAB *lsmHash = NULL;

static void LsmInitHash(void)
{
	HASHCTL ctl;

	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(Oid);
	ctl.entrysize = sizeof(LsmData);

	/* hash style Oid  */
	ctl.hash = oid_hash;
	ctl.hcxt = LsmContext;
	lsmHash = hash_create("Lsm hash", 20000, &ctl, HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);
}

static void LsmIdxIdSetActiveIdxId(Oid idxId, Oid lsmActiveIdxId)
{
	LWLockAcquire(LsmLock, LW_SHARED);
	LsmData *pLsmData = (LsmData *)hash_search(lsmHash, &idxId, HASH_FIND, NULL);
	LWLockRelease(LsmLock);
	Assert(pLsmData != NULL);
	LsmDataSetActiveIdxId(pLsmData, lsmActiveIdxId);
}
/*******************************************************************************
**                LsmData end
*******************************************************************************/












/*******************************************************************************
**                LsmOverThresholdQueue  start
*******************************************************************************/
bool lsmFreezeThreadNeedStop = false;

typedef struct LsmOverThresholdNode
{
	Oid subIdxId;
	Oid idxId;
	Oid heapId;
	struct LsmOverThresholdNode *next;
	struct LsmOverThresholdNode *pre;
}LsmOverThresholdNode;

LsmOverThresholdNode *lsmOverThresholdQueue = NULL;

void LsmOverThresholdQueueAdd(Oid subIdxId, Oid idxId, Oid heapId)
{
	LsmOverThresholdNode *newNode = (LsmOverThresholdNode *)MemoryContextAlloc(
										LsmContext, sizeof(LsmOverThresholdNode));
	newNode->subIdxId = subIdxId;
	newNode->idxId = idxId;
	newNode->heapId = heapId;
	newNode->next = newNode;
	newNode->pre = newNode;

	LWLockAcquire(LsmOverThresholdLock, LW_EXCLUSIVE);

	if(lsmOverThresholdQueue == NULL)
	{
		lsmOverThresholdQueue = newNode;
	}else{
		LsmOverThresholdNode *head = lsmOverThresholdQueue;
		LsmOverThresholdNode *tail = head->pre;

		newNode->next = head;
		newNode->pre = tail;

		head->pre = newNode;
		tail->next = newNode;
	}

	LWLockRelease(LsmOverThresholdLock);
}

void LsmOverThresholdQueueRemove(LsmOverThresholdNode *pNode)
{
	LWLockAcquire(LsmOverThresholdLock, LW_EXCLUSIVE);

	if(pNode == lsmOverThresholdQueue)
	{
		if((pNode->next == pNode) && (pNode->pre == pNode))
		{
			lsmOverThresholdQueue = NULL;
		}
		else
		{
			Assert(pNode->pre != pNode);
			Assert(pNode->next != pNode);
			lsmOverThresholdQueue = pNode->next;
			pNode->pre->next = pNode->next;
			pNode->next->pre = pNode->pre;
		}
	}
	else
	{
		pNode->pre->next = pNode->next;
		pNode->next->pre = pNode->pre;
	}

	LWLockRelease(LsmOverThresholdLock);

	pfree(pNode);
}
/*******************************************************************************
**                LsmOverThresholdQueue end
*******************************************************************************/














/*******************************************************************************
**                LsmSubPool(just save subidx after frozen) start
*******************************************************************************/
enum LsmSubPriority
{
	LSM_SUBPRIO_IMMEDIATELY, /* must merge immediately */
	LSM_SUBPRIO_SOON, /* use this lsmsubidx as soon as prossible */
	LSM_SUBPRIO_NOMAL, /* normal */
	LSM_SUBPRIO_ERROR, /* error over maxtimes */
	LSM_SUBPRIO_MAX,
};

enum LsmSubState{
	LSM_SUBSTATE_FROZEN,
	LSM_SUBSTATE_MERGED,
};

typedef struct LsmSubNode
{
	Oid subIdxId;
	Oid idxId;
	Oid heapId;
	LsmSubPriority priority;
	LsmSubState state;
	uint32 nerrors;
	TransactionId frozenNextXid;
	struct LsmSubNode *next;
	struct LsmSubNode *pre;
}LsmSubNode;

typedef struct LsmSubPool
{
	LsmSubNode *prioQs[LSM_SUBPRIO_MAX];
	Oid subIdxIdInprogress;
	LWLockId lockid;
	uint32 ntries;
}LsmSubPool;

/* just save frozen lsm sub idx */
LsmSubPool lsmSubFrozenPool;

void LsmSubPoolEnqueue(LsmSubNode *newLsmSubNode)
{
	LsmSubPriority nodePrio = newLsmSubNode->priority;
	Assert((nodePrio >= 0) && (nodePrio < LSM_SUBPRIO_MAX));
	LsmSubPool *pFrozenPool = &lsmSubFrozenPool;

	if(pFrozenPool->prioQs[nodePrio] == NULL)
	{
		pFrozenPool->prioQs[nodePrio] = newLsmSubNode;
		newLsmSubNode->pre = newLsmSubNode;
		newLsmSubNode->next = newLsmSubNode;
	}else{
		LsmSubNode *prioHead = pFrozenPool->prioQs[nodePrio];
		LsmSubNode *prioTail = prioHead->pre;

		newLsmSubNode->next = prioHead;
		newLsmSubNode->pre = prioTail;

		prioHead->pre = newLsmSubNode;
		prioTail->next = newLsmSubNode;
	}
}

void LsmSubPoolDequeue(LsmSubNode *lsmSubNode)
{
	LsmSubPriority nodePrio = lsmSubNode->priority;
	Assert((nodePrio >= 0) && (nodePrio < LSM_SUBPRIO_MAX));
	LsmSubPool *pFrozenPool = &lsmSubFrozenPool;

	if(lsmSubNode != pFrozenPool->prioQs[nodePrio])
	{
		lsmSubNode->pre->next = lsmSubNode->next;
		lsmSubNode->next->pre = lsmSubNode->pre;
	}
	else
	{
		if((lsmSubNode->next != lsmSubNode) && (lsmSubNode->pre != lsmSubNode))
		{
			pFrozenPool->prioQs[nodePrio] = lsmSubNode->next;
			lsmSubNode->pre->next = lsmSubNode->next;
			lsmSubNode->next->pre = lsmSubNode->pre;
		}
		else
		{
			Assert(lsmSubNode->pre = lsmSubNode);
			Assert(lsmSubNode->next == lsmSubNode);
			pFrozenPool->prioQs[nodePrio] = NULL;
		}
	}
}

void LsmSubPoolAdd(Oid subIdxId, Oid idxId, Oid heapId, LsmSubState state,
	LsmSubPriority priority, uint32 nerrors, TransactionId frozenNextXid, bool inLock)
{
	Assert((priority >= 0) && (priority < LSM_SUBPRIO_MAX));
	LsmSubPool *pFrozenPool = &lsmSubFrozenPool;

	LsmSubNode *newLsmSubNode = (LsmSubNode *)MemoryContextAlloc(LsmContext, sizeof(LsmSubNode));
	newLsmSubNode->subIdxId = subIdxId;
	newLsmSubNode->idxId = idxId;
	newLsmSubNode->heapId = heapId;
	newLsmSubNode->priority = priority;
	newLsmSubNode->state = state;
	newLsmSubNode->nerrors = nerrors;
	newLsmSubNode->frozenNextXid = frozenNextXid;
	newLsmSubNode->next = newLsmSubNode;
	newLsmSubNode->pre = newLsmSubNode;

	if(!inLock)
	{
		LWLockAcquire(pFrozenPool->lockid, LW_EXCLUSIVE);
	}

	LsmSubPoolEnqueue(newLsmSubNode);

	if(!inLock)
	{
		LWLockRelease(pFrozenPool->lockid);
	}
}

void LsmSubPoolRemove(LsmSubNode *lsmSubNode, bool inLock)
{
	if(!inLock)
	{
		LWLockAcquire(lsmSubFrozenPool.lockid, LW_EXCLUSIVE);
	}

	LsmSubPoolDequeue(lsmSubNode);

	if(!inLock)
	{
		LWLockRelease(lsmSubFrozenPool.lockid);
	}

	pfree(lsmSubNode);
}

void LsmSubPoolUpdate(LsmSubNode *lsmSubNode, LsmSubState newState,
	LsmSubPriority newPriority, uint32 nerrors, bool inLock)
{
	Assert((newPriority >= 0) && (newPriority < LSM_SUBPRIO_MAX));

	if(!inLock)
	{
		LWLockAcquire(lsmSubFrozenPool.lockid, LW_EXCLUSIVE);
	}

	lsmSubNode->state = newState;
	lsmSubNode->nerrors = nerrors;

	if(lsmSubNode->priority != newPriority)
	{
		LsmSubPoolDequeue(lsmSubNode);
		lsmSubNode->priority = newPriority;
		LsmSubPoolEnqueue(lsmSubNode);
	}

	if(!inLock)
	{
		LWLockRelease(lsmSubFrozenPool.lockid);
	}
}

void LsmInitSubPool(void)
{
	LsmSubPool *pFrozenPool = &lsmSubFrozenPool;

	for(uint32 prio = 0; prio < LSM_SUBPRIO_MAX; prio++)
	{
		pFrozenPool->prioQs[prio] = NULL;
	}
	pFrozenPool->lockid = LsmSubPoolLock;
	pFrozenPool->ntries = 5;
	pFrozenPool->subIdxIdInprogress = InvalidOid;

	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();

	Relation rel = heap_open(MetaTableId, AccessShareLock);
	Relation idxrel = index_open(MetaTableColFirstIndex, AccessShareLock);
	HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);
	HeapTuple tuple = NULL;
	while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		int len = 0;
		Form_meta_table pMetaData = (Form_meta_table)fdxdb_tuple_to_chars_with_len(tuple, len);
		//Assert(len == sizeof(Form_meta_table_data));

		if(pMetaData->type != LSM_SUBTYPE){
			continue;
		}

		BlockNumber nblocks = LsmSmgrNblocks(pMetaData->rel_id,
									pMetaData->database_id,
									pMetaData->tablespace_id);
		if(nblocks >= BlockNumberThresholdByRelType(pMetaData->type))
		{
			Oid lsmSubIdxId = pMetaData->rel_id;
			Oid lsmIdxId = pMetaData->rel_pid;
			Relation lsmSubIdxRel = index_open(lsmSubIdxId, AccessShareLock);
			Relation lsmIdxRel = index_open(lsmIdxId, AccessShareLock);
			Oid heapId = lsmIdxRel->mt_info.parentId;
			index_close(lsmIdxRel, NoLock);
			index_close(lsmSubIdxRel, AccessShareLock);

			LsmSubPoolAdd(lsmSubIdxId, lsmIdxId, heapId, LSM_SUBSTATE_FROZEN,
				LSM_SUBPRIO_NOMAL, 0, InvalidTransactionId, false);
		}
		else if(nblocks == 0)
		{
			Oid lsmSubIdxId = pMetaData->rel_id;
			Oid lsmIdxId = pMetaData->rel_pid;
			Relation lsmSubIdxRel = index_open(lsmSubIdxId, AccessShareLock);
			Relation lsmIdxRel = index_open(lsmIdxId, AccessShareLock);
			Oid heapId = lsmIdxRel->mt_info.parentId;
			Relation heapRel = heap_open(heapId, AccessShareLock);
			IndexInfo *indexInfo = BuildIndexInfo(lsmSubIdxRel);
			index_build(heapRel, lsmSubIdxRel, indexInfo, false, true);
			
			index_close(lsmSubIdxRel, NoLock);
			index_close(lsmIdxRel, NoLock);
			heap_close(heapRel, NoLock);
		}

		pfree(pMetaData);
	}
	heap_endscan(scan);
	heap_close(rel, NoLock);
	index_close(idxrel, NoLock);

	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();
}
/*******************************************************************************
**                LsmSubPool(just save subidx after frozen) end
*******************************************************************************/











/*******************************************************************************
**                        merge&&truncate start
*******************************************************************************/
bool lsmMergeThreadNeedStop = false;

static void LsmBitmapScanAllTuples(Relation heapRel, Relation subIdxRel,
	bool deleteAll, Tuplesortstate *pState, bool saveTuple)
{
	SnapshotData SnapshotMVCC= {HeapTupleSatisfiesMVCC};
	IndexScanDesc index_scan = index_beginscan(heapRel, subIdxRel, &SnapshotMVCC,
	//IndexScanDesc index_scan = index_beginscan(heapRel, subIdxRel, SnapshotNow,
									0, 0, 0,0);
	index_rescan(index_scan, NULL, 0, NULL, 0);

	TIDBitmap * bitmap = tbm_create(work_mem*1024L);
	index_getbitmap(index_scan, bitmap);
	BitmapHeapScanState* bmpScanState = ExecInitBitmapHeapScan(heapRel, bitmap,
											index_scan->xs_snapshot,
											index_scan->numberOfKeys,
											index_scan->keyData);

	HeapTuple tuple;
	while(1)
	{
		tuple = BitmapHeapNext(bmpScanState);
		if(tuple == NULL)
		{
			break;
		}

		if((!deleteAll) && (pState == NULL))
		{
			continue;
		}

		ItemPointerData it = tuple->t_self;
		simple_heap_delete(heapRel, &it);

		if(pState != NULL)
		{
			void *privateData = NULL;
			if(saveTuple)
			{
				HeapTuple ctuple = heap_copytuple(tuple);
				privateData = (void *)ctuple;
			}
			else
			{
				ItemPointer tid = (ItemPointer)palloc(sizeof(ItemPointerData));
				*tid = tuple->t_self;
				privateData = (void *)tid;
			}

			Datum values = fdxdb_form_index_datum(heapRel, subIdxRel, tuple);
			if (values != 0 && values != (Datum)-1)
			{// skip if no index key needed for this tuple    
				tuplesort_putdatum(pState, values, (ItemPointer)privateData);
			}
			else if (values == (Datum)-1)
			{// multiple keys per heaptuple.
				size_t nkeys = 0;
				Datum *pvalues = combineIndexKeys(heapRel, subIdxRel, tuple, &nkeys);
				if(pvalues && nkeys != 0)
				{
					for (size_t i = 0; i < nkeys; i++)
					{
						tuplesort_putdatum(pState, pvalues[i], (ItemPointer)privateData);
					}
					pfree(pvalues);
				}
			}
		}
	}

	ExecEndBitmapHeapScan(bmpScanState);
	index_endscan(index_scan);
}

static void LsmMergeTuples(Relation heapRel, uint32 ntuples, HeapTuple *tuples)
{
	if(ntuples == 0)
	{
		return;
	}

	/* insert heap */
	BulkInsertState bistate = GetBulkInsertState();
	heap_multi_insert(heapRel, tuples, ntuples, bistate);
	FreeBulkInsertState(bistate);

	/* insert indexes */
	for(uint32 i = 0; i < MtInfo_GetIndexCount(heapRel->mt_info); ++i)
	{
		Oid idxId = MtInfo_GetIndexOid(heapRel->mt_info)[i];
		Relation idxRel = index_open(idxId, AccessShareLock);

		for(uint32 j = 0; j < ntuples; j++)
		{
			insert_tuple_to_index(heapRel, idxRel, tuples[j]);
		}

		index_close(idxRel, NoLock);
	}

	/* free tuples */
	for(uint32 i = 0; i < ntuples; i++)
	{
		pfree(tuples[i]);
	}
}

static void LsmMergeDeleteUseBitmap(Relation heapRel, Relation subIdxRel)
{
	/* delete all */
	LsmBitmapScanAllTuples(heapRel, subIdxRel, true, NULL, false);

	/* insert all */
	IndexScanDesc indexscan = index_beginscan(heapRel, subIdxRel, SnapshotNow, 0, 0, 0, 0);
	index_rescan(indexscan, NULL, 0, NULL, 0);
	static const int insertCountOnce = 100000;
	HeapTuple *tuples = (HeapTuple *)palloc(insertCountOnce * sizeof(HeapTuple));
	uint32 ntuples = 0;
	while(1)
	{
		HeapTuple tuple = index_getnext(indexscan, ForwardScanDirection);
		if(tuple == NULL)
		{
			LsmMergeTuples(heapRel, ntuples, tuples);
			break;
		}
		else
		{
			tuples[ntuples] = heap_copytuple(tuple);

			ntuples++;
			if(ntuples == (uint32)insertCountOnce)
			{
				LsmMergeTuples(heapRel, ntuples, tuples);
				ntuples = 0;
			}
		}
	}
	index_endscan(indexscan);
}
#if 0
static void LsmMergeAllUseBitmap(Relation heapRel, Relation subIdxRel)
{
	/* tuplesort begin */
	Tuplesortstate *pState;
	int workMem  = BlockNumberThresholdByRelType(LSM_SUBTYPE)*(BLCKSZ/1024)*2;
	pState = tuplesort_begin_index_datum(workMem, true, subIdxRel);

	LsmBitmapScanAllTuples(heapRel, subIdxRel, true, pState, true);

	tuplesort_performsort(pState);

	/* insert tuple */
	static const int insertCountOnce = 100000;
	HeapTuple tuples[insertCountOnce];
	uint32 ntuple = 0;
	while(1)
	{
		/* get tuple */
		Datum dat_value;
		HeapTuple newtuple = NULL;
		if(!tuplesort_getdatum1(pState, true, &dat_value, (void **)&newtuple))
		{
			newtuple = NULL;
		}
		else
		{
			if(DatumGetPointer(dat_value) != NULL)
			{
				pfree(DatumGetPointer(dat_value));
			}
		}

		if(newtuple == NULL)
		{
			LsmMergeTuples(heapRel, ntuple, tuples);
			break;
		}
		else
		{
			tuples[ntuple] = newtuple;

			ntuple++;
			if(ntuple == insertCountOnce)
			{
				LsmMergeTuples(heapRel, ntuple, tuples);
				ntuple = 0;
			}
		}
	}

	tuplesort_end(pState);
}
#endif
/* just merge index */
#if 0
static void LsmMergeNoDelete(Relation heapRel, Relation idxRel,
	Relation subIdxRel)
{
	LsmBitmapScanAllTuples(heapRel, subIdxRel, false, NULL, false);

	/* insert all */
	IndexScanDesc indexscan = index_beginscan(heapRel, subIdxRel, SnapshotNow, 0, 0, 0, 0);
	index_rescan(indexscan, NULL, 0, NULL, 0);
	HeapTuple tuple;
	while((tuple = index_getnext(indexscan, ForwardScanDirection)) != NULL)
	{
		insert_tuple_to_index(heapRel, idxRel, tuple);
	}
	index_endscan(indexscan);
}

static void LsmMergeNoDeleteAllUseBitmap(Relation heapRel, Relation idxRel,
	Relation subIdxRel)
{
	/* tuplesort begin */
	Tuplesortstate *pState;
	int workMem  = BlockNumberThresholdByRelType(LSM_SUBTYPE)*(BLCKSZ/1024)*2;
	pState = tuplesort_begin_index_datum(workMem, true, subIdxRel);

	LsmBitmapScanAllTuples(heapRel, subIdxRel, false, pState, false);

	tuplesort_performsort(pState);

	/* insert tuples to indexes */
	static const int insertCountOnce = 100000;
	HeapTuple tuples[insertCountOnce];
	uint32 ntuple = 0;
	while(1)
	{
		/* get tuple */
		Datum dat_value;
		ItemPointer tid;
		if(!tuplesort_getdatum1(pState, true, &dat_value, (void **)&tid))
		{
			break;
		}

		bool isnull = (DatumGetPointer(dat_value) == NULL);
		index_insert(idxRel, &dat_value, &isnull, tid, heapRel, UNIQUE_CHECK_NO);
		if(isnull)
		{
			pfree(DatumGetPointer(dat_value));
		}
		pfree(tid);
	}

	tuplesort_end(pState);

	/* insert all */
	IndexScanDesc indexscan = index_beginscan(heapRel, subIdxRel, SnapshotNow, 0, 0, 0, 0);
	index_rescan(indexscan, NULL, 0, NULL, 0);
	HeapTuple tuple;
	while((tuple = index_getnext(indexscan, ForwardScanDirection)) != NULL)
	{
		insert_tuple_to_index(heapRel, idxRel, tuple);
	}
	index_endscan(indexscan);
}
#endif
bool LsmMerge(Oid lsmSubIdxId, Oid lsmIdxId, Oid heapId)
{
	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();

	PG_TRY();
	{
		Relation lsmSubIdxRel = index_open(lsmSubIdxId, AccessShareLock);
		Assert(lsmSubIdxRel->mt_info.type == LSM_SUBTYPE);
		Relation lsmIdxRel = index_open(lsmIdxId, AccessShareLock);
		Relation heapRel = heap_open(heapId, AccessShareLock, 0);

		//LsmMergeNoDelete(lsmSubIdxId, lsmIdxId, heapId);
		LsmMergeDeleteUseBitmap(heapRel, lsmSubIdxRel);
		//LsmMergeAllUseBitmap(heapRel, lsmSubIdxRel);
	}
	PG_CATCH();
	{
		ereport(WARNING, (errmsg("merge failed.")));
		StartTransactionCommand();
		UserAbortTransactionBlock();
		CommitTransactionCommand();
		return false;
	}
	PG_END_TRY();

	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();
	return true;
}

bool LsmTruncate(Oid lsmSubIdxId, Oid lsmIdxId, Oid heapId)
{
	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();

	//ForceSyncCommit();

	/* truncate */
	PG_TRY();
	{
		Relation heapRel = heap_open(heapId, AccessShareLock);

		RelationTruncateIndex(heapRel, lsmSubIdxId);
		//reindex_index(heapId, lsmSubIdxId, heapRel->mt_info.database_id);
		heap_close(heapRel, NoLock);
	}
	PG_CATCH();
	{
		ereport(WARNING, (errmsg("truncate failed.")));
		StartTransactionCommand();
		UserAbortTransactionBlock();
		CommitTransactionCommand();
		return false;
	}
	PG_END_TRY();

	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();

	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();

	//ForceSyncCommit();
	return true;
}

static void LsmStopMergeThread(SIGNAL_ARGS)
{
	lsmMergeThreadNeedStop = true;
	return;
}

static void LsmMergeThreadRegisterSignal(void)
{
	/* Set up signal handlers */
	pg_signal(SIGHUP, SIG_IGN);		
	pg_signal(SIGINT, SIG_IGN);
	pg_signal(SIGTERM, SIG_IGN);
	pg_signal(SIGQUIT, SIG_IGN);
	pg_signal(SIGALRM, SIG_IGN);
	pg_signal(SIGPIPE, SIG_IGN);
	pg_signal(SIGUSR1, SIG_IGN);
	pg_signal(SIGUSR2, LsmStopMergeThread); /* shutdown */
	pg_signal(SIGCHLD, SIG_DFL);
	pg_signal(SIGTTIN, SIG_DFL);
	pg_signal(SIGTTOU, SIG_DFL);
	pg_signal(SIGCONT, SIG_DFL);
	pg_signal(SIGWINCH, SIG_DFL);

	return;
}

static LsmSubNode *LsmGetNextProcessSubNode(void)
{
	TransactionId xmin = LsmGetOldestTransactionId();

	for(uint32 prio = 0; prio < LSM_SUBPRIO_MAX; prio++)
	{
		if(lsmSubFrozenPool.prioQs[prio] == NULL)
		{
			continue;
		}

		LsmSubNode *head = lsmSubFrozenPool.prioQs[prio];
		LsmSubNode *tmplsmSubNode = head;
		do
		{
			if(tmplsmSubNode->state == LSM_SUBSTATE_MERGED)
			{
				return tmplsmSubNode;
			}

			if(tmplsmSubNode->state == LSM_SUBSTATE_FROZEN)
			{
				if((tmplsmSubNode->frozenNextXid == InvalidOid)
				|| (TransactionIdFollowsOrEquals(xmin, tmplsmSubNode->frozenNextXid)))
				{
					return tmplsmSubNode;
				}
			}

			tmplsmSubNode = tmplsmSubNode->next;
		}
		while(tmplsmSubNode->next != head);
	}

	return NULL;
}

void *LsmMergeThread(void *arg)
{
	LsmMergeThreadRegisterSignal();

	LsmSubPool *pFrozenPool = &lsmSubFrozenPool;
	long sleep_us = 1000000L; /* 1s */

	while(!lsmMergeThreadNeedStop)
	{
		LWLockAcquire(pFrozenPool->lockid, LW_EXCLUSIVE);
		LsmSubNode *lsmSubNode = LsmGetNextProcessSubNode();

		if(lsmSubNode == NULL){
			pFrozenPool->subIdxIdInprogress = InvalidOid;
			//ereport(LOG, (errmsg("merge thread start to sleep.")));
			LWLockRelease(pFrozenPool->lockid);
			pg_sleep(sleep_us);
		}else{
			pFrozenPool->subIdxIdInprogress = lsmSubNode->subIdxId;
			LWLockRelease(pFrozenPool->lockid);

			bool needTruncate = (lsmSubNode->state == LSM_SUBSTATE_MERGED);
			uint32 nerrors = lsmSubNode->nerrors;
			LsmSubPriority priority = lsmSubNode->priority;
#ifdef _DEBUG
			ereport(LOG, (errmsg("Start to merge %d, state %d", lsmSubNode->subIdxId, lsmSubNode->state)));

			/* merge */
			struct timeval start_time;
			struct timeval end_time;
			gettimeofday(&start_time, NULL);
#endif

			if(lsmSubNode->state == LSM_SUBSTATE_FROZEN){
				if(LsmMerge(lsmSubNode->subIdxId, lsmSubNode->idxId, lsmSubNode->heapId)){
					needTruncate = true;
					nerrors = 0; /* merge successfully, restart counting */
				}else{
					++nerrors;
					if(nerrors >= pFrozenPool->ntries){
						priority = LSM_SUBPRIO_ERROR;
					}
					LsmSubPoolUpdate(lsmSubNode, LSM_SUBSTATE_FROZEN, priority, nerrors, false);
				}
			}

			/* truncate */
			if(needTruncate)
			{
				if(LsmTruncate(lsmSubNode->subIdxId, lsmSubNode->idxId, lsmSubNode->heapId))
				{
					LsmSubPoolRemove(lsmSubNode, false);
				}else
				{
					++nerrors;
					if(nerrors >= pFrozenPool->ntries){
						priority = LSM_SUBPRIO_ERROR;
					}
					LsmSubPoolUpdate(lsmSubNode, LSM_SUBSTATE_MERGED, priority, nerrors, false);
				}
			}
#ifdef _DEBUG
			gettimeofday(&end_time, NULL);
			double use_s = end_time.tv_sec-start_time.tv_sec;
			double use_us = end_time.tv_usec-start_time.tv_usec;
			double use_time = use_us/1000000 + use_s;
			ereport(LOG, (errmsg("Merge %d successfully, use %fs(%lds %ldus)", pFrozenPool->subIdxIdInprogress, use_time,
				end_time.tv_sec-start_time.tv_sec, end_time.tv_usec-start_time.tv_usec)));
#endif
		}
	}

	return NULL;
}
/*******************************************************************************
**                        merge&&truncate end
*******************************************************************************/












/*******************************************************************************
**                freeze&&active start
*******************************************************************************/
bool LsmFreeze(Oid subIdxId, Oid& idxId, Oid& heapId, Oid& newSubIdxId)
{
	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();

	PG_TRY();
	{
		Relation subIdxRel = index_open(subIdxId, AccessShareLock);
		idxId = subIdxRel->mt_info.parentId;
		Relation idxRel = index_open(idxId, AccessShareLock);
		heapId = idxRel->mt_info.parentId;
		Relation heapRel = heap_open(heapId, AccessShareLock);

		/* create new subIdx */
		newSubIdxId = __fxdb_index_create(InvalidOid, heapRel, LSM_SUBTYPE,
							idxRel->mt_info.colid, idxRel,
							MtInfo_GetUserData(idxRel->mt_info), NULL,
							MtInfo_GetUserDataLength(idxRel->mt_info));
		Relation newSubIdxRel = index_open(newSubIdxId, AccessShareLock);
		CacheInvalidateRelcache(idxRel);
		CacheInvalidateRelcache(newSubIdxRel);
	}
	PG_CATCH();
	{
		StartTransactionCommand();
		UserAbortTransactionBlock();
		CommitTransactionCommand();
		return false;
	}
	PG_END_TRY();

	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();

	return true;
}

static void LsmStopFreezeThread(SIGNAL_ARGS)
{
	lsmFreezeThreadNeedStop = true;
	return;
}

static void LsmFreezeThreadRegisterSignal(void)
{
	/* Set up signal handlers */
	pg_signal(SIGHUP, SIG_IGN);		
	pg_signal(SIGINT, SIG_IGN);
	pg_signal(SIGTERM, SIG_IGN);
	pg_signal(SIGQUIT, SIG_IGN);
	pg_signal(SIGALRM, SIG_IGN);
	pg_signal(SIGPIPE, SIG_IGN);
	pg_signal(SIGUSR1, SIG_IGN);
	pg_signal(SIGUSR2, LsmStopFreezeThread); /* shutdown */
	pg_signal(SIGCHLD, SIG_DFL);
	pg_signal(SIGTTIN, SIG_DFL);
	pg_signal(SIGTTOU, SIG_DFL);
	pg_signal(SIGCONT, SIG_DFL);
	pg_signal(SIGWINCH, SIG_DFL);

	return;
}

void *LsmFreezeThread(void *arg)
{
	LsmFreezeThreadRegisterSignal();

	long sleep_us = 1000000L; /* 1s */

	while(!lsmFreezeThreadNeedStop)
	{
		LWLockAcquire(LsmOverThresholdLock, LW_EXCLUSIVE);
		if(lsmOverThresholdQueue == NULL)
		{
			//ereport(LOG, (errmsg("freeze thread start to sleep.")));
			LWLockRelease(LsmOverThresholdLock);
			pg_sleep(sleep_us);
		}
		else
		{
			LsmOverThresholdNode *pNode = lsmOverThresholdQueue;
			LWLockRelease(LsmOverThresholdLock);

			Oid subIdxId = pNode->subIdxId;
			Oid idxId;
			Oid heapId;
			Oid newSubIdxId;
			if(LsmFreeze(subIdxId, idxId, heapId, newSubIdxId))
			{
				LsmIdxIdSetActiveIdxId(idxId, newSubIdxId);
				LsmOverThresholdQueueRemove(pNode);
				LsmSubPoolAdd(subIdxId, idxId, heapId, LSM_SUBSTATE_FROZEN,
					LSM_SUBPRIO_SOON, 0, LsmNextTransactionId(), false);
			}
		}
	}

	return NULL;
}
/*******************************************************************************
**                freeze&&active end
*******************************************************************************/













/*******************************************************************************
**                lsm export functions start
*******************************************************************************/
static LsmSubNode **LsmSubNodesInSubPool(Relation lsmIdxRel, uint32& nSubNodes)
{
	uint32 nSubIdxs = MtInfo_GetIndexCount(lsmIdxRel->mt_info);
	Oid idxId = RelationGetRelid(lsmIdxRel);
	LsmSubNode **subNodes = (LsmSubNode**)MemoryContextAllocZero(LsmContext,
									nSubIdxs*sizeof(LsmSubNode*));

	nSubNodes = 0;

	for(uint32 priority = 0; priority < LSM_SUBPRIO_MAX; priority++)
	{
		LsmSubNode *prioHead = lsmSubFrozenPool.prioQs[priority];
		if(prioHead != NULL)
		{
			LsmSubNode *tmpLsmSubNode = prioHead;
			do
			{
				if(idxId == tmpLsmSubNode->idxId)
				{
					subNodes[nSubNodes++] = tmpLsmSubNode;
				}
				tmpLsmSubNode = tmpLsmSubNode->next;
			}
			while(tmpLsmSubNode != prioHead);
		}
	}

	Assert(nSubNodes <= nSubIdxs);

	return subNodes;
}

static Oid LsmSubIdxIdNotInSubNodes(Relation lsmIdxRel, uint32 nNodes,
	Oid exceptSubIdxId, LsmSubNode **subNodes)
{
	uint32 nSubIdxs = MtInfo_GetIndexCount(lsmIdxRel->mt_info);

	if(nNodes < ((exceptSubIdxId == InvalidOid) ? nSubIdxs : (nSubIdxs - 1)))
	{
		for(uint32 i = 0; i < nSubIdxs; i++)
		{
			Oid subIdxId = MtInfo_GetIndexOid(lsmIdxRel->mt_info)[i];

			if(subIdxId != exceptSubIdxId)
			{
				bool found = false;
				for(uint32 j = 0; j < nNodes; j++)
				{
					if(subIdxId == subNodes[j]->subIdxId)
					{
						found = true;
						break;
					}
				}
				if(!found)
				{
					return subIdxId;
				}
			}
		}
	}

	return InvalidOid;
}

static void LsmUpdateSubNodesPriority(uint32 nNodes, LsmSubNode **subNodes,
	Oid newPerfectActiveIdxId, Oid newActiveIdxId)
{
	LsmSubPriority newPriority = ((newPerfectActiveIdxId == InvalidOid)
									? LSM_SUBPRIO_IMMEDIATELY : LSM_SUBPRIO_SOON);
	uint32 nImmediatelyPrios = 0;
	uint32 maxImmediatelyPrioCount = 1;

	for(uint32 i = 0; i < nNodes; i++)
	{
		if((subNodes[i]->subIdxId == lsmSubFrozenPool.subIdxIdInprogress)
		|| (subNodes[i]->subIdxId == newActiveIdxId))
		{
			continue;
		}
		if(subNodes[i]->priority != newPriority)
		{
			LsmSubPoolUpdate(subNodes[i], subNodes[i]->state,
					newPriority, subNodes[i]->nerrors, true);
		}
		if(newPriority == LSM_SUBPRIO_IMMEDIATELY)
		{
			nImmediatelyPrios++;
			if(nImmediatelyPrios == maxImmediatelyPrioCount)
			{
				newPriority = LSM_SUBPRIO_SOON;
			}
		}
	}
}

static void LsmSelectNewActiveIdxId(Relation lsmIdxRel,
	Oid curActiveIdxId, Oid& newPerfectActiveIdxId, Oid& newActiveIdxId)
{
	Oid idxId = RelationGetRelid(lsmIdxRel);
	Oid heapId = lsmIdxRel->mt_info.parentId;

	LWLockAcquire(lsmSubFrozenPool.lockid, LW_EXCLUSIVE);

	uint32 nNodes;
	LsmSubNode **subNodes = LsmSubNodesInSubPool(lsmIdxRel, nNodes);
	newPerfectActiveIdxId = LsmSubIdxIdNotInSubNodes(lsmIdxRel, nNodes, curActiveIdxId, subNodes);

	newActiveIdxId = ((newPerfectActiveIdxId != InvalidOid) ? newPerfectActiveIdxId : curActiveIdxId);
	LsmSubNode *newActiveNode = NULL;

	/* set newActiveIdxId */
	if(newActiveIdxId == InvalidOid)
	{
		for(uint32 i = 0; i < nNodes; i++)
		{
			if((subNodes[i]->subIdxId != lsmSubFrozenPool.subIdxIdInprogress)
			&& (subNodes[i]->state == LSM_SUBSTATE_FROZEN))
			{
				newActiveIdxId = subNodes[i]->subIdxId;
				newActiveNode = subNodes[i];
				break;
			}
		}
	}
	if(newActiveIdxId == InvalidOid)
	{
		for(uint32 i = 0; i < nNodes; i++)
		{
			if(subNodes[i]->subIdxId != lsmSubFrozenPool.subIdxIdInprogress)
			{
				newActiveIdxId = subNodes[i]->subIdxId;
				newActiveNode = subNodes[i];
				break;
			}
		}
	}
	Assert(newActiveIdxId != InvalidOid);

	/* set priority */
	LsmUpdateSubNodesPriority(nNodes, subNodes, newPerfectActiveIdxId, newActiveIdxId);

	/* remove newActiveNode */
	if(newActiveNode != NULL)
	{
		LsmSubPoolRemove(newActiveNode, true);
	}

	LWLockRelease(lsmSubFrozenPool.lockid);

	pfree(subNodes);

	return;
}

static void *LsmFindLsmData(Relation lsmIdxRel)
{
	Oid idxId = RelationGetRelid(lsmIdxRel);
	Oid heapId = lsmIdxRel->mt_info.parentId;

	LWLockAcquire(LsmLock, LW_EXCLUSIVE);
	LsmData *pLsmData = (LsmData *)hash_search(lsmHash, &idxId, HASH_FIND, NULL);
	if(pLsmData != NULL)
	{
		LWLockRelease(LsmLock);
		return pLsmData;
	}

	/* new LsmData */
	pLsmData = (LsmData *)hash_search(lsmHash, &idxId, HASH_ENTER, NULL);
	pLsmData->lsmIdxId = idxId;

	/* get active lsm data */
	Oid newActiveIdxId;
	Oid newPerfectActiveIdxId;
	LsmSelectNewActiveIdxId(lsmIdxRel, InvalidOid, newPerfectActiveIdxId, newActiveIdxId);

	/* init LsmData */
	LsmDataInit(pLsmData, newActiveIdxId);

	LWLockRelease(LsmLock);

	/* add newActiveIdxId to OverThresholdQueue */
	if(newPerfectActiveIdxId == InvalidOid)
	{
		LsmOverThresholdQueueAdd(newActiveIdxId, idxId, heapId);
	}

	return pLsmData;
}

Oid LsmGetActiveIdxId(Relation lsmIdxRel)
{
	Assert(lsmIdxRel->mt_info.type == LSM_TYPE);

	if(lsmIdxRel->pLsmData == NULL)
	{
		lsmIdxRel->pLsmData = (void *)LsmFindLsmData(lsmIdxRel);
	}

	return LsmDataGetActiveIdxId((LsmData *)(lsmIdxRel->pLsmData));
}

void LsmOverThreshold(Oid subIdxId)
{
	Assert(subIdxId != InvalidOid);
	ereport(LOG, (errmsg("lsm over threshold %d.", subIdxId)));

	Relation subIdxRel = index_open(subIdxId, AccessShareLock);
	Assert(subIdxRel->mt_info.type == LSM_SUBTYPE);

	Oid idxId = subIdxRel->mt_info.parentId;
	Relation idxRel = index_open(idxId, AccessShareLock);
	Assert(idxRel->mt_info.type == LSM_TYPE);
	LsmData *pLsmData = (LsmData *)(idxRel->pLsmData);
	Assert(LsmDataGetActiveIdxId(pLsmData) == subIdxId);

	Oid heapId = idxRel->mt_info.parentId;

	/* get newActiveIdxId */
	Oid newActiveIdxId;
	Oid newPerfectActiveIdxId;
	LsmSelectNewActiveIdxId(idxRel, subIdxId, newPerfectActiveIdxId, newActiveIdxId);

	index_close(idxRel, NoLock);
	index_close(subIdxRel, AccessShareLock);

	if(newPerfectActiveIdxId == InvalidOid)
	{
		Assert(newActiveIdxId == subIdxId);
		LsmOverThresholdQueueAdd(subIdxId, idxId, heapId);
		pgkill(lsm_freeze_handle->thid, SIGUSR1);
	}
	else
	{
		LsmDataSetActiveIdxId(pLsmData, newActiveIdxId);
		LsmSubPoolAdd(subIdxId, idxId, heapId, LSM_SUBSTATE_FROZEN,
					LSM_SUBPRIO_SOON, 0, LsmNextTransactionId(), false);
	}
}
/*******************************************************************************
**                lsm export functions end
*******************************************************************************/










/*******************************************************************************
**                lsm init start
*******************************************************************************/
void *LsmMergeThreadFunc(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	fxdb_SubPostmaster_Main(params);
	free(params);

	LsmMergeThread(NULL);

	proc_exit(0);
	return NULL;
}

void *LsmFreezeThreadFunc(void *params)
{
	extern void* fxdb_SubPostmaster_Main(void *);
	fxdb_SubPostmaster_Main(params);
	free(params);

	LsmFreezeThread(NULL);

	proc_exit(0);
	return NULL;
}

void LsmInit(storage_params *params)
{
	if((params != NULL) && (params->lsm_subtree_threshold_nblocks > 0)){
		//g_lsm_tree_threshold_lsm = params->lsm_tree_threshold_nblocks;
		g_lsm_subtree_threshold_lsm = params->lsm_subtree_threshold_nblocks;
	}

	LsmContext = AllocSetContextCreate(ProcessTopMemoryContext,
						"Lsm Context",
						ALLOCSET_SMALL_MINSIZE,
						ALLOCSET_SMALL_INITSIZE,
						ALLOCSET_SMALL_MAXSIZE);

	LsmInitHash();
	LsmInitSubPool();

	/* start freeze thread */
	BackendParameters *freezeParam = (BackendParameters *)malloc(sizeof(BackendParameters));
	freezeParam->MyThreadType = backend;
	lsm_freeze_handle = fxdb_Thread_Create(LsmFreezeThreadFunc, freezeParam);
	pg_sleep(1000000);
#if 1
	/* start merge thread */
	BackendParameters *mergeParam = (BackendParameters *)malloc(sizeof(BackendParameters));
	mergeParam->MyThreadType = backend;
	lsm_merge_handle = fxdb_Thread_Create(LsmMergeThreadFunc, mergeParam);
#endif
	pg_sleep(1000000);
}
/*******************************************************************************
**                lsm init end
*******************************************************************************/


#if 0
static void LsmDeleteAllTuples(Relation heapRel, Relation subIdxRel,
	Tuplesortstate* pState)
{
	SnapshotData SnapshotMVCC= {HeapTupleSatisfiesMVCC};
	//IndexScanDesc index_scan = index_beginscan(heapRel, subIdxRel, &SnapshotMVCC,
	IndexScanDesc index_scan = index_beginscan(heapRel, subIdxRel, SnapshotNow,
									0, 0, 0,0);
	index_rescan(index_scan, NULL, 0, NULL, 0);

	TIDBitmap * bitmap = tbm_create(work_mem*1024L);
	index_getbitmap(index_scan, bitmap);
	BitmapHeapScanState* bmpScanState = ExecInitBitmapHeapScan(heapRel, bitmap,
											index_scan->xs_snapshot,
											index_scan->numberOfKeys,
											index_scan->keyData);

	HeapTuple tuple;
	while((tuple = BitmapHeapNext(bmpScanState)) != NULL)
	{
		ItemPointerData it = tuple->t_self;
		simple_heap_delete(heapRel, &it);

		if(pState != NULL)
		{
			Datum values = fdxdb_form_index_datum(heapRel, subIdxRel, tuple);
			if (values != 0 && values != -1)
			{// skip if no index key needed for this tuple    
				tuplesort_putdatum(pState, values, &it);
			}
			else if (values == -1)
			{// multiple keys per heaptuple.
				size_t nkeys = 0;
				Datum *pvalues = combineIndexKeys(heapRel, subIdxRel, tuple, &nkeys);
				for (size_t i = 0; i < nkeys; i++)
				{
					tuplesort_putdatum(pState, pvalues[i], &it);
				}
				pfree(pvalues);
			}
		}
	}

	ExecEndBitmapHeapScan(bmpScanState);
	index_endscan(index_scan);
}
static void LsmMergeTuples(Relation heapRel, uint32 ntuples, HeapTuple *tuples)
{
	if(ntuples == 0)
	{
		return;
	}
	struct timeval start_time;
	struct timeval after_delete_time;
	struct timeval after_insert_heap_time;
	struct timeval after_insert_indexes_time;
	gettimeofday(&start_time, NULL);

	/* delete heap */
	#if 0
	for(uint32 i = 0; i < ntuples; i++)
	{
		simple_heap_delete(heapRel, &(tuples[i]->t_self));
	}
	#endif
	gettimeofday(&after_delete_time, NULL);

	/* insert heap */
	BulkInsertState bistate = GetBulkInsertState();
	heap_multi_insert(heapRel, tuples, ntuples, bistate);
	FreeBulkInsertState(bistate);
	gettimeofday(&after_insert_heap_time, NULL);

	/* insert indexes */
	for(uint32 i = 0; i < MtInfo_GetIndexCount(heapRel->mt_info); ++i)
	{
		Oid idxId = MtInfo_GetIndexOid(heapRel->mt_info)[i];
		Relation idxRel = index_open(idxId, AccessShareLock);

		for(uint32 j = 0; j < ntuples; j++)
		{
			insert_tuple_to_index(heapRel, idxRel, tuples[j]);
		}

		index_close(idxRel, NoLock);
	}
	gettimeofday(&after_insert_indexes_time, NULL);

	double use_s = after_delete_time.tv_sec - start_time.tv_sec;
	double use_us = after_delete_time.tv_usec - start_time.tv_usec;
	double use_time = use_us/1000000 + use_s;
	ereport(LOG, (errmsg("delete %fs", use_time)));
	use_s = after_insert_heap_time.tv_sec - after_delete_time.tv_sec;
	use_us = after_insert_heap_time.tv_usec - after_delete_time.tv_usec;
	use_time = use_us/1000000 + use_s;
	ereport(LOG, (errmsg("insert heap %fs", use_time)));
	use_s = after_insert_indexes_time.tv_sec - after_insert_heap_time.tv_sec;
	use_us = after_insert_indexes_time.tv_usec - after_insert_heap_time.tv_usec;
	use_time = use_us/1000000 + use_s;
	ereport(LOG, (errmsg("insert indexes %fs", use_time)));

	/* free tuples */
	for(uint32 i = 0; i < ntuples; i++)
	{
		pfree(tuples[i]);
	}
}

while(1)
		{
			HeapTuple tuple = index_getnext(indexscan, ForwardScanDirection);
			if(tuple == NULL)
			{
				break;
			}
			else
			{
				HeapTuple newtuple = heap_copytuple(tuple);
				simple_heap_delete(heapRel, &(newtuple->t_self));
				heap_insert(heapRel, newtuple, GetCurrentCommandId(true), 0, NULL);
				for(uint32 i = 0; i < MtInfo_GetIndexCount(heapRel->mt_info); ++i)
				{
					Oid idxId = MtInfo_GetIndexOid(heapRel->mt_info)[i];
					Relation idxRel = index_open(idxId, AccessShareLock);
					insert_tuple_to_index(heapRel, idxRel, newtuple);
					index_close(idxRel, NoLock);
				}
				pfree(newtuple);
			}
		}

		/* truncate */
		//__index_drop(lsmSubIdxId, heapId, heapRel->mt_info.spcid);
	PG_TRY();
	{
		Relation heapRel = heap_open(heapId, AccessShareLock);
		Relation lsmIdxRel = index_open(lsmIdxId, AccessShareLock);

		RelationTruncateIndex(heapRel, lsmSubIdxId);

		__fxdb_index_create(lsmSubIdxId, heapRel, LSM_SUBTYPE,
			lsmIdxRel->mt_info.colid, lsmIdxRel);
		Relation lsmSubIdxRel = index_open(lsmSubIdxId, AccessShareLock);
		CacheInvalidateRelcache(lsmIdxRel);
		CacheInvalidateRelcache(lsmSubIdxRel);
		heap_close(heapRel, NoLock);
	}
	PG_CATCH();
	{
		ereport(WARNING, (errmsg("truncate failed.")));
		StartTransactionCommand();
		UserAbortTransactionBlock();
		CommitTransactionCommand();
		return false;
	}
	PG_END_TRY();

	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();
#endif
