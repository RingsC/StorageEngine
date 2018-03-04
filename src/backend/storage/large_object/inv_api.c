/*-------------------------------------------------------------------------
 *
 * inv_api.c
 *	  routines for manipulating inversion fs large objects. This file
 *	  contains the user-level large object application interface routines.
 *
 *
 * Note: we access pg_largeobject.data using its C struct declaration.
 * This is safe because it immediately follows pageno which is an int4 field,
 * and therefore the data field will always be 4-byte aligned, even if it
 * is in the short 1-byte-header format.  We have to detoast it since it's
 * quite likely to be in compressed or short format.  We also need to check
 * for NULLs, since initdb will mark loid and pageno but not data as NOT NULL.
 *
 * Note: many of these routines leak memory in CurrentMemoryContext, as indeed
 * does most of the backend code.  We expect that CurrentMemoryContext will
 * be a short-lived context.  Data that must persist across function calls
 * is kept either in CacheMemoryContext (the Relation structs) or in the
 * memory context given to inv_open (for LargeObjectDesc structs).
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/large_object/inv_api.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/sysattr.h"
#include "access/tuptoaster.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#ifndef FOUNDER_XDB_SE
#include "catalog/dependency.h"
#endif //FOUNDER_XDB_SE
#include "catalog/indexing.h"
//#include "catalog/objectaccess.h"
#include "catalog/pg_largeobject.h"
#include "catalog/pg_largeobject_metadata.h"

#ifndef FOUNDER_XDB_SE
#include "commands/comment.h"
#else 
#include "catalog/xdb_catalog.h"
#include "storage/smgr.h"
#include "storage/proc.h"
#include "commands/tablespace.h"
#include "catalog/pg_database.h"
#endif //FOUNDER_XDB_SE

#include "libpq/libpq-fs.h"
#include "miscadmin.h"
#include "storage/large_object.h"
#include "utils/fmgroids.h"
#include "utils/rel.h"
#include "utils/resowner.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"
#include "utils/tqual.h"


/*
 * All accesses to pg_largeobject and its index make use of a single Relation
 * reference, so that we only need to open pg_relation once per transaction.
 * To avoid problems when the first such reference occurs inside a
 * subtransaction, we execute a slightly klugy maneuver to assign ownership of
 * the Relation reference to TopTransactionResourceOwner.
 */
static THREAD_LOCAL Relation lo_heap_r = NULL;
static THREAD_LOCAL Relation lo_index_r = NULL;

#ifdef FOUNDER_XDB_SE

static THREAD_LOCAL Oid lo_heap_id = InvalidOid;
static THREAD_LOCAL Oid lo_index_id = InvalidOid;
static THREAD_LOCAL Oid lo_tbl_space = InvalidOid;
static MemoryContext LOMemCxt = NULL;

typedef struct  
{
	List *loheap_list;
	ListCell *lo_cell;
} LOMgr;

typedef struct 
{
	Oid dbid;
	LOMgr *mgr;
} DBLOMgr;

static LOMgr LORelArr = {NULL, NULL};
static List *DBLOList = NULL;

#define LO_REL_HASH_SIZE 10

typedef struct  
{
	Oid loid;
	Oid dbid;
	Relation heap_rel;
	Relation index_rel;
} LORel;

typedef struct  
{
	Oid loid;
	Oid dbid;
} LOHashKey;

#ifndef FOUNDER_XDB_SE
static THREAD_LOCAL HTAB *LocalLOInRelHash = NULL;
#else
#define LocalLOInRelHash (CurrentTransactionState->mTopLevelState->mLocalLOInRelHash)
#endif
extern int relid_compare(const char*, size_t, const char*, size_t);
extern int int4_compare(const char*, size_t, const char*, size_t);
extern int int64_compare(const char*, size_t, const char*, size_t);

static HeapTuple id_get_lo_copy(Oid lo_id);

#endif //FOUNDER_XDB_SE

#ifdef FOUNDER_XDB_SE

static 
void loid_equal_init_scankey(Oid loid, ScanKey skey)
{
	char data[4];
	memset(data, 0, sizeof(data));
	memcpy(data, &loid, sizeof(loid));
	Datum datum = fdxdb_string_formdatum(data, sizeof(data));
	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[0],
		1,
		BTEqualStrategyNumber,
		relid_compare,
		datum);
}

static
LOMgr get_db_lorel_list(Oid dbid = MyDatabaseId)
{
	Assert(DBLOList != NULL);

	ListCell *cell = NULL;
	LOMgr retval = {NULL, NULL};

	foreach (cell, DBLOList)
	{
		DBLOMgr *tmgr = (DBLOMgr *) lfirst (cell);
		if (tmgr->dbid == dbid)
		{
			retval = *tmgr->mgr;
			break;
		}
	}

	return retval;
}

void InitLOMemCxt()
{
	LOMemCxt = AllocSetContextCreate(TopMemoryContext,
		"Large Object Context",
		ALLOCSET_DEFAULT_MINSIZE,
		ALLOCSET_DEFAULT_INITSIZE,
		ALLOCSET_DEFAULT_MAXSIZE);
}

/*
 * 该函数假定外头已经持有LOHeapListLock排他锁
 */
void RebuildLORelList(bool ignoreError)
{
	extern char *get_tablespace_name(Oid spc_oid);

	size_t cnt = 0;
	Form_meta_database formClassArr = GetAllDatabaseInfos(cnt);
	Form_meta_database tClass = NULL;

	MemoryContext oldcxt = MemoryContextSwitchTo(ProcessTopMemoryContext);

	volatile size_t i = 0;
	for(i = 0; i < cnt; ++i)
	{
		tClass = (Form_meta_database) ((char*)formClassArr + i * MAX_FORM_DBMETA_SIZE);
		Form_meta_large_table_rel *tmpClass = GetDatabaseLOMetaTable(tClass->db_id);
		LOMgr *tmpArr = (LOMgr *) palloc0 (sizeof(LOMgr));

		Relation rel = heap_open(tmpClass->relid2, AccessShareLock);
		HeapScanDesc scan = NULL;
		HeapTuple tuple = NULL;
		Form_large_heap formClass = NULL;

		PG_TRY();
		{
			scan = heap_beginscan(rel, SnapshotNow, 0, NULL, 0, 0);
			while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
			{
				MemoryContext oldcxt = MemoryContextSwitchTo(LOMemCxt);
				formClass = (Form_large_heap)fxdb_tuple_to_chars(tuple);
				MemoryContextSwitchTo(oldcxt);

				if(get_tablespace_name(formClass->tblspace) != NULL) {
					oldcxt = MemoryContextSwitchTo(LOMemCxt);
					tmpArr->loheap_list = lappend(tmpArr->loheap_list, formClass);
					MemoryContextSwitchTo(oldcxt);
				} else {
					pfree(formClass);
					simple_heap_delete(rel, &tuple->t_self);
				}
			}
		}
		PG_CATCH();
		{
			if (!ignoreError)
				PG_RE_THROW();
			
			int no = 0;
			char *msg = NULL;
			
			msg = get_errno_errmsg(no);
			FlushErrorState();
			ereport(WARNING, (errmsg(msg)));
			pfree(msg);
		}
		PG_END_TRY();

		if (scan)
			heap_endscan(scan);
		heap_close(rel, NoLock);

		if (list_length(tmpArr->loheap_list) > 0)
		{
			DBLOMgr *mgr = (DBLOMgr *) palloc0 (sizeof(DBLOMgr));
			mgr->dbid = tmpClass->dbid;
			mgr->mgr = tmpArr;
			DBLOList = lappend(DBLOList, mgr);
		}

		pfree(tmpClass);
	}

	MemoryContextSwitchTo(oldcxt);

	pfree(formClassArr);
}

void Fxdb_Init_LORel_Hash()
{
	HASHCTL		ctl;

	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(LOHashKey);
	ctl.entrysize = sizeof(LORel);
	ctl.hash = tag_hash;
	ctl.hcxt = TopTransactionContext;

	LocalLOInRelHash = 
		hash_create("LargeObject In Relation Hash", LO_REL_HASH_SIZE, &ctl, HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);
}

void InitLORelList()
{
	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();

	/* 这里是在存储引擎启动的时候调用一次来初始化
	 * 列表，所以这里不需要加锁
	 */
	RebuildLORelList(true);

	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();

	//Fxdb_Init_LORel_Hash();
}

void DestoryLORelList()
{
	if (DBLOList == NULL)
		return;

	LWLockAcquire(LOHeapListLock, LW_EXCLUSIVE);

	ListCell *cell = NULL;
	foreach(cell, DBLOList)
	{
		DBLOMgr *mgr = (DBLOMgr *) lfirst (cell);
		if (mgr->mgr->loheap_list != NULL)
			list_free_deep(mgr->mgr->loheap_list);
	}

	list_free_deep(DBLOList);
	DBLOList = NULL;

	LWLockRelease(LOHeapListLock);
}

void SetOpenLORelation()
{
	if(lo_heap_r != NULL && lo_index_r != NULL)
		return;

	Assert(lo_index_r == NULL && lo_heap_r == NULL);

	Form_large_heap formClass;

	LWLockAcquire(LOHeapListLock, LW_EXCLUSIVE);
	if(DBLOList == NULL)
	{
		RebuildLORelList(false);
	}
	
	Assert(DBLOList != NULL);

	LORelArr = get_db_lorel_list(MyDatabaseId);

	Assert(LORelArr.loheap_list != NULL);

	do 
	{
		if(LORelArr.lo_cell == NULL)
		{
			LORelArr.lo_cell = list_head(LORelArr.loheap_list);
		}
		formClass = (Form_large_heap)(lfirst(LORelArr.lo_cell));
		/* 这里将索引和表一同放入list中，所以需要遍历 
		 * 直到找到的是表而不是索引
		 */
		LORelArr.lo_cell = lnext(LORelArr.lo_cell);
	} while (formClass->lo_index_id == 0);

	lo_heap_id = formClass->lo_heap_id;
	lo_index_id = formClass->lo_index_id;
	lo_tbl_space = formClass->tblspace;
	
	LWLockRelease(LOHeapListLock);
}

static 
HeapTuple id_get_heaptuple_copy(Oid lo_id)
{
	Relation	pg_lo_meta;
	ScanKeyData skey[1];
	SysScanDesc sd;
	HeapTuple	tuple;
	HeapTuple rettuple = NULL;

	loid_equal_init_scankey(lo_id, skey);

	Form_meta_large_table_rel *tClass = GetDatabaseLOMetaTable(MyDatabaseId);

	pg_lo_meta = heap_open(tClass->relid,
		AccessShareLock);

	sd = systable_beginscan(pg_lo_meta,
		tClass->index_loid, true,
		SnapshotNow, 1, skey);

	tuple = systable_getnext(sd);
	if (HeapTupleIsValid(tuple))
	{
		rettuple = heap_copytuple(tuple);
	}

	systable_endscan(sd);
	heap_close(pg_lo_meta, NoLock);
	pfree(tClass);

	return rettuple;
}

bool IsLOEmpty(Oid relid)
{
	bool retval = false;

	Relation rel = NULL;
	HeapScanDesc scan = NULL;

	rel = heap_open(relid, AccessExclusiveLock);
	scan = heap_beginscan(rel, SnapshotNow, 0, NULL, 0, 0);

	if(HeapTupleIsValid(heap_getnext(scan, ForwardScanDirection)))
	{
		retval  = false;
	} else
	{
		retval = true;
	}

	heap_close(rel, NoLock);
	heap_endscan(scan);

	return retval;
}

List *ListLo(Oid tblspace, uint32 scan_num, 
						 Oid *idx_loid, bool *has_next)
{
	ScanKeyData skey[1];
	SysScanDesc sd;

	List *ret_list = NULL;
	Relation rel = NULL;
	HeapScanDesc scan = NULL;
	HeapTuple tuple = NULL;
	Form_meta_large formClass = NULL;
	uint32 count = 0;

	char data[4];
	memset(data, 0, sizeof(data));
	memcpy(data, idx_loid, sizeof(*idx_loid));
	Datum datum = fdxdb_string_formdatum(data, sizeof(data));
	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[0],
		1,
		BTGreaterStrategyNumber,
		relid_compare,
		datum);

	Form_meta_large_table_rel *tClass = GetDatabaseLOMetaTable(MyDatabaseId);

	rel = heap_open(tClass->relid, AccessShareLock);

	sd = systable_beginscan(rel,
		tClass->index_loid, true,
		SnapshotNow, 1, skey);

	while((tuple = systable_getnext(sd)) != NULL)
	{
		formClass = (Form_meta_large)fxdb_tuple_to_chars(tuple);

		if(tblspace > 0)
		{
			if(formClass->lo_tblspace == tblspace)
			{
				ret_list = lappend(ret_list, formClass);
			}
		} else {
			ret_list = lappend(ret_list, formClass);
		}

		++count;
		++(*idx_loid);
		if(count == scan_num)
		{
			break;
		}
	}

	if(count == scan_num)
	{
		*has_next = true;
	} else {
		*has_next = false;
	}

	systable_endscan(sd);
	heap_close(rel, NoLock);
	pfree(tClass);

	return ret_list;
}

void InsertMetaLORelInfo(Oid relid, Oid index_loid, Oid index_loname,
												 Oid relid2, Oid relid2_index, Oid relid2_tbcindex,
												 Oid dbid)
{
	Form_meta_large_table_rel formClass;
	formClass.relid = relid;
	formClass.index_loid = index_loid;
	formClass.index_loname = index_loname;
	formClass.relid2 = relid2;
	formClass.relid2_index = relid2_index;
	formClass.relid2_tbcindex = relid2_tbcindex;
	formClass.dbid = dbid;

	Relation rel = heap_open(LargeObjMetaTableRelId, RowExclusiveLock);
	HeapTuple tuple = fdxdb_heap_formtuple((char*)&formClass, sizeof(Form_meta_large_table_rel));
	simple_heap_insert(rel, tuple);
	heap_close(rel, NoLock);

	pfree(tuple);
}

Form_meta_large_table_rel *GetDatabaseLOMetaTable(Oid dbid , bool isRemove)
{
	Form_meta_large_table_rel *formClass = NULL;

	if (dbid == DEFAULTDATABASE_OID)
	{
		Assert(!isRemove);

		formClass = (Form_meta_large_table_rel *) palloc0 (sizeof(Form_meta_large_table_rel));
		formClass->dbid = DEFAULTDATABASE_OID;
		formClass->relid = MetaLargeObjId;
		formClass->index_loid = MetaLargeObjIndexColLOID;
		formClass->index_loname = MetaLargeObjIndexColLONAME;
		formClass->relid2 = LargeObjHeapId;
		formClass->relid2_index = LargeObjHeapIndex;
		formClass->relid2_tbcindex = LargeObjHeapTbcIndex;
	} else
	{
		HeapTuple tuple = NULL;
		Relation rel = heap_open(LargeObjMetaTableRelId, AccessShareLock);
		HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);

		while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			formClass = (Form_meta_large_table_rel *)fxdb_tuple_to_chars(tuple);
			if(formClass->dbid == dbid)
			{
				if(isRemove)
					simple_heap_delete(rel, &tuple->t_self);
				break;
			}
			else
			{
				pfree(formClass);
				formClass = NULL;
			}
		}

		heap_endscan(scan);
		heap_close(rel, NoLock);
	}

	Assert(formClass != NULL);

	return formClass;
}

static
void regedit_lo_rel(Oid relid, Oid indid, Oid tblspace, Oid dbid = MyDatabaseId)
{
	Form_class_large_heap formClass;
	formClass.lo_heap_id = relid;
	formClass.lo_index_id = indid;
	formClass.tblspace = tblspace;

	Form_meta_large_table_rel *formClass2 = GetDatabaseLOMetaTable(dbid);

	Relation rel = heap_open(formClass2->relid2, RowExclusiveLock);
	HeapTuple tuple = fdxdb_heap_formtuple((char*)&formClass, sizeof(formClass));
	simple_heap_insert(rel, tuple);
	heap_close(rel, NoLock);
	pfree(tuple);
	pfree(formClass2);
}

void CreateNewLORel(Oid tblspace, Oid dbid)
{
	extern Oid GetNewObjectId(void);

	Oid relid = GetNewObjectId();
	Oid indid = GetNewObjectId();
	fxdb_heap_create(relid, LargeObjDataId, tblspace, dbid);
	Relation rel = heap_open(relid, ShareLock);
	fxdb_index_create(indid, rel, BTREE_UNIQUE_TYPE, LargeObjDataIndex);
	heap_close(rel, NoLock);

	regedit_lo_rel(relid, indid, tblspace, dbid);
	regedit_lo_rel(indid, 0, tblspace, dbid);
}

void CreateMetaLORel(Oid tblspace, Oid dbid)
{
	extern Oid GetNewObjectId(void);

	Oid relid = GetNewObjectId();
	Oid index_loid = GetNewObjectId();
	Oid index_loname = GetNewObjectId();
	Oid relid2 = GetNewObjectId();
	Oid relid2_index = GetNewObjectId();
	Oid relid2_tbcindex = GetNewObjectId();

	fxdb_heap_create(relid, MetaLargeObjId, tblspace, dbid);
	Relation rel = heap_open(relid, ShareLock);

	fxdb_index_create(index_loid, rel, BTREE_UNIQUE_TYPE, MetaLargeObjIndexColLOID);
	fxdb_index_create(index_loname, rel, BTREE_UNIQUE_TYPE, MetaLargeObjIndexColLONAME);
	heap_close(rel, NoLock);

	fxdb_heap_create(relid2, LargeObjHeapId, tblspace, dbid);
	rel = heap_open(relid2, ShareLock);

	fxdb_index_create(relid2_index, rel, BTREE_UNIQUE_TYPE, LargeObjHeapIndex);
	fxdb_index_create(relid2_tbcindex, rel, BTREE_TYPE, LargeObjHeapTbcIndex);
	heap_close(rel, NoLock);

	CommandCounterIncrement();
	InsertMetaLORelInfo(relid, index_loid, index_loname, 
		relid2, relid2_index, relid2_tbcindex, dbid);
}

void DatabaseCreateLORel(Oid dbid)
{
	List *tbc_list = ListTableSpaceDefault();

	if(tbc_list != NULL)
	{
		ListCell *cell = NULL;
		foreach(cell, tbc_list)
		{
			FormTablespaceData formClass = (FormTablespaceData) lfirst (cell);
			CreateNewLORel(formClass->id, dbid);
		}

		list_free_deep(tbc_list);
	}
}

void TablespaceCreateLORel(Oid tbcid)
{
	size_t cnt = 0;
	Form_meta_database arrFormClass = GetAllDatabaseInfos(cnt);
	Form_meta_database tClass = NULL;

	for(size_t i = 0; i < cnt; ++i)
	{
		tClass = (Form_meta_database) ((char*)arrFormClass + i * MAX_FORM_DBMETA_SIZE);
		CreateNewLORel(tbcid, tClass->db_id);
	}

	pfree(arrFormClass);
}

void CreateDefaultLORel()
{
	Relation metaRelation = NULL;

	fxdb_heap_create(LargeObjMetaTableRelId, 0);

	/* Create large object meta table. */
	fxdb_heap_create(MetaLargeObjId, MetaLargeObjId);

	metaRelation = heap_open(MetaLargeObjId, RowExclusiveLock);
	/* Create index of large object meta table. */
	fxdb_index_create(MetaLargeObjIndexColLOID, metaRelation, BTREE_UNIQUE_TYPE, MetaLargeObjIndexColLOID);
	fxdb_index_create(MetaLargeObjIndexColLONAME, metaRelation, BTREE_UNIQUE_TYPE, MetaLargeObjIndexColLONAME);

	heap_close(metaRelation, NoLock);

	/* Create large object heap table. */
	fxdb_heap_create(LargeObjHeapId, LargeObjHeapId);

	metaRelation = heap_open(LargeObjHeapId, RowExclusiveLock);
	/* Create index of large object heap table. */
	fxdb_index_create(LargeObjHeapIndex, metaRelation, BTREE_UNIQUE_TYPE, LargeObjHeapIndex);
	fxdb_index_create(LargeObjHeapTbcIndex, metaRelation, BTREE_TYPE, LargeObjHeapTbcIndex);
	heap_close(metaRelation, NoLock);

	CommandCounterIncrement();
	InsertMetaLORelInfo(MetaLargeObjId, MetaLargeObjIndexColLOID, MetaLargeObjIndexColLONAME,
		LargeObjHeapId, LargeObjHeapIndex, LargeObjHeapTbcIndex, DEFAULTDATABASE_OID);

	///* Create default large object data table. */
	fxdb_heap_create(LargeObjDataId, LargeObjDataId, DEFAULTTABLESPACE_OID);

	metaRelation = heap_open(LargeObjDataId, RowExclusiveLock);
	/* Create index of default large object data table. */
	fxdb_index_create(LargeObjDataIndex, metaRelation, BTREE_UNIQUE_TYPE, LargeObjDataIndex);
	heap_close(metaRelation, NoLock);

	CommandCounterIncrement();
	regedit_lo_rel(LargeObjDataId, LargeObjDataIndex, DEFAULTTABLESPACE_OID);
	regedit_lo_rel(LargeObjDataIndex, 0, DEFAULTTABLESPACE_OID);
}

static
List* catalog_drop_lo_rel(Oid tblspace)
{
	size_t cnt = 0;
	Form_meta_database arrFormClass = GetAllDatabaseInfos(cnt);
	Form_meta_database tClass = NULL;

	List *retval = NULL;

	for(size_t i = 0; i < cnt; ++i)
	{
		tClass = (Form_meta_database) ((char*)arrFormClass + i * MAX_FORM_DBMETA_SIZE);
		Form_meta_large_table_rel *loMeta = GetDatabaseLOMetaTable(tClass->db_id);

		Relation rel = heap_open(loMeta->relid2, RowExclusiveLock);

		ScanKeyData key;
		char data[4];
		memset(data, 0, sizeof(data));
		memcpy(data, &tblspace, sizeof(tblspace));
		Datum datum = fdxdb_string_formdatum(data, sizeof(data));
		Fdxdb_ScanKeyInitWithCallbackInfo(&key, 
			1, 
			BTEqualStrategyNumber, 
			relid_compare, 
			datum);

		SysScanDesc sd = systable_beginscan(rel,
			loMeta->relid2_tbcindex, true,
			SnapshotNow, 1, &key);;

		HeapTuple tuple = NULL;
		Form_large_heap formClass = NULL;
		while((tuple = systable_getnext(sd)) != NULL)
		{
			formClass = (Form_large_heap)fxdb_tuple_to_chars(tuple);
			if(formClass->tblspace == tblspace)
			{
				/* Not index. */
				if(formClass->lo_index_id != 0)
				{
					Form_large_heap tmpClass = (Form_large_heap)palloc0(sizeof(Form_class_large_heap));
					memcpy(tmpClass, formClass, sizeof(Form_class_large_heap));
					retval = lappend(retval, tmpClass);
				}
				simple_heap_delete(rel, &tuple->t_self);
			}
			pfree(formClass);
		}
		systable_endscan(sd);
		heap_close(rel, NoLock);

		pfree(loMeta);
	}

	pfree(arrFormClass);

	return retval;
}

static
void unlink_lo_rel(Form_large_heap formClass)
{
	extern Oid get_toast_table_id(Oid relId);
	extern Oid get_toast_index_id(Oid relId);

	Relation rel = relation_open(formClass->lo_heap_id, AccessExclusiveLock);

	Oid dbid = rel->mt_info.database_id;
	Oid tblspace = formClass->tblspace;
	Oid toastId = get_toast_table_id(formClass->lo_heap_id);
	Oid toastIndexId = get_toast_index_id(formClass->lo_heap_id);

	DoUnlinkRel(formClass->lo_heap_id, dbid, tblspace, -1);
	DoUnlinkRel(formClass->lo_index_id, dbid, tblspace, -1);
	DoUnlinkRel(toastId, dbid, tblspace, -1);
	DoUnlinkRel(toastIndexId, dbid, tblspace, -1);

	relation_close(rel, NoLock);
}

void DropLORel(Oid tblspace)
{
	/* Remove rel from meta table. */
	List *rel_remove = catalog_drop_lo_rel(tblspace);

	if (rel_remove != NULL)
	{
		/* Drop storage. */
		ListCell *cell = NULL;
		
		foreach(cell, rel_remove)
		{
			Form_large_heap formClass = (Form_large_heap) lfirst (cell);

			if (!IsLOEmpty(formClass->lo_heap_id))
				ereport(ERROR,
				(errmsg("large object relation is not empty")));
			fdxdb_heap_drop(formClass->lo_heap_id);
		}

		list_free_deep(rel_remove);
	}
}

static 
Form_meta_large id_get_lorel(Oid loid)
{
	ScanKeyData skey[1];
	SysScanDesc sd;
	Relation rel = NULL;
	HeapTuple tuple = NULL;
	Form_meta_large formClass = NULL;
	
	loid_equal_init_scankey(loid, skey);

	Form_meta_large_table_rel *tClass = GetDatabaseLOMetaTable(MyDatabaseId);

	rel = heap_open(tClass->relid, AccessShareLock);
	
	sd = systable_beginscan(rel, 
		tClass->index_loid,
		true,
		SnapshotNow,
		1,
		skey);

	tuple = systable_getnext(sd);
	if(HeapTupleIsValid(tuple))
	{
		formClass = (Form_meta_large)fxdb_tuple_to_chars(tuple);
	}
	systable_endscan(sd);
	heap_close(rel, NoLock);

	pfree(tClass);

	return formClass;
}

bool lo_exit(Oid lo_id, Form_meta_large *formClass)
{
	bool retval = false;

	HeapTuple tuple = id_get_heaptuple_copy(lo_id);
	
	if (HeapTupleIsValid(tuple))
	{
		if(formClass != NULL)
		{
			*formClass = (Form_meta_large)fxdb_tuple_to_chars(tuple);
		}
		retval = true;

		pfree(tuple);
	}

	return retval;
}

Oid name_get_loid(char *name)
{
	extern int name_data_compare(const char* a, size_t len1, const char* b, size_t len2);

	Relation	pg_lo_meta;
	ScanKeyData skey[1];
	SysScanDesc sd;
	HeapTuple	tuple;
	Oid		lo_id = 0;
	NameData nd;

	memset(&nd, 0, sizeof(nd));
	memcpy(nd.data, name, strlen(name));
	Datum datum = fdxdb_string_formdatum((char*)&nd, sizeof(nd));
	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[0],
		1,
		BTEqualStrategyNumber,
		name_data_compare,
		datum);

	Form_meta_large_table_rel *tClass = GetDatabaseLOMetaTable(MyDatabaseId);

	pg_lo_meta = heap_open(tClass->relid,
		AccessShareLock);

	sd = systable_beginscan(pg_lo_meta,
		tClass->index_loname, true,
		SnapshotNow, 1, skey);

	tuple = systable_getnext(sd);
	if (HeapTupleIsValid(tuple))
	{
		char *data = (char*)fxdb_tuple_to_chars(tuple);
		lo_id = *((Oid*)data);
		pfree(data);
	}

	systable_endscan(sd);

	heap_close(pg_lo_meta, NoLock);

	pfree(tClass);

	return lo_id;
}

char* loid_get_name(Oid lo_id)
{
	extern int relid_compare(const char*, size_t, const char*, size_t);

	Relation	pg_lo_meta;
	ScanKeyData skey[1];
	SysScanDesc sd;
	HeapTuple	tuple;
	NameData nd;

	loid_equal_init_scankey(lo_id, skey);
	memset(&nd, 0, sizeof(nd));

	pg_lo_meta = heap_open(MetaLargeObjId,
		AccessShareLock);

	sd = systable_beginscan(pg_lo_meta,
		MetaLargeObjIndexColLOID, true,
		SnapshotNow, 1, skey);

	tuple = systable_getnext(sd);
	if (HeapTupleIsValid(tuple))
	{
		Form_meta_large data = (Form_meta_large)fxdb_tuple_to_chars(tuple);
		memcpy(&nd, &(data->lo_name), sizeof(nd));
		pfree(data);
	}

	systable_endscan(sd);

	heap_close(pg_lo_meta, NoLock);
	//return temp address, emit it - sunwf
	return strdup(nd.data);
}

Form_meta_large fxdb_inv_update_lo(Form_meta_large formClass, 
																	 void *extraData, size_t extraDataLen)
{
	Form_meta_large retClass = NULL;
	Relation	pg_lo_meta;
	Form_meta_large_table_rel *tClass = GetDatabaseLOMetaTable(MyDatabaseId);

	pg_lo_meta = heap_open(tClass->relid,
		RowExclusiveLock);

	HeapTuple tuple = id_get_heaptuple_copy(formClass->lo_id);

	if (HeapTupleIsValid(tuple))
	{
		size_t len = sizeof(Form_meta_large_data) + extraDataLen;
		char *update_data = (char*)palloc0(len);
		formClass->extra_data_size = (uint32)extraDataLen;
		memcpy(update_data, formClass, sizeof(Form_meta_large_data));
		memcpy(&update_data[sizeof(Form_meta_large_data)], extraData, extraDataLen);
		retClass = (Form_meta_large)update_data;
		HeapTuple update_tuple = fdxdb_heap_formtuple(update_data, len);
		simple_heap_update(pg_lo_meta, &tuple->t_self, update_tuple);

		pfree(tuple);
	} else {
		ereport(ERROR,
			(errcode(ERRCODE_UNDEFINED_OBJECT),
			errmsg("large object %u was already dropped", formClass->lo_id)));
	}

	heap_close(pg_lo_meta, NoLock);

	pfree(tClass);

	return retClass;
}

#endif //FOUNDER_XDB_SE


/*
 * Open pg_largeobject and its index, if not already done in current xact
 */
#ifndef FOUNDER_XDB_SE

static void
open_lo_relation(void)

#else

static void
open_lo_relation(LargeObjectDesc *obj_desc)

#endif //FOUNDER_XDB_SE
{
	ResourceOwner currentOwner;

#ifndef FOUNDER_XDB_SE

	if (lo_heap_r && lo_index_r)
		return;					/* already open in current xact */

#else

	LOHashKey lokey;
	lokey.loid = obj_desc->id;
	lokey.dbid = MyDatabaseId;
	bool found;

	LORel *tmp_lo_rel = 
		(LORel *) hash_search(LocalLOInRelHash, &lokey, HASH_ENTER, &found);

	if(found)
	{
		lo_heap_r = tmp_lo_rel->heap_rel;
		lo_index_r = tmp_lo_rel->index_rel;
		return;
	}

	tmp_lo_rel->loid = obj_desc->id;
	tmp_lo_rel->dbid = MyDatabaseId;
	tmp_lo_rel->heap_rel = NULL;
	tmp_lo_rel->index_rel = NULL;

#endif //FOUNDER_XDB_SE

	/* Arrange for the top xact to own these relation references */
	currentOwner = CurrentResourceOwner;
	PG_TRY();
	{
		CurrentResourceOwner = TopTransactionResourceOwner;

		/* Use RowExclusiveLock since we might either read or write */
#ifndef FOUNDER_XDB_SE
		if (lo_heap_r == NULL)
			lo_heap_r = heap_open(LargeObjectRelationId, RowExclusiveLock);
		if (lo_index_r == NULL)
			lo_index_r = index_open(LargeObjectLOidPNIndexId, RowExclusiveLock);
#else

		Form_meta_large formClass = id_get_lorel(obj_desc->id);
		if(formClass == NULL)
		{
			hash_search(LocalLOInRelHash, &lokey, HASH_REMOVE, NULL);
			ereport(ERROR,
				(errcode(ERRCODE_NO_DATA),
				errmsg("large object %u does not exist", obj_desc->id)));
		}
		lo_index_id = formClass->index_id;
		lo_heap_id = formClass->rel_id;

		tmp_lo_rel->heap_rel = heap_open(lo_heap_id, RowExclusiveLock);
		lo_heap_r = tmp_lo_rel->heap_rel;

		tmp_lo_rel->index_rel = index_open(lo_index_id, RowExclusiveLock);
		lo_index_r = tmp_lo_rel->index_rel;

		Assert(tmp_lo_rel->heap_rel != NULL && tmp_lo_rel->index_rel != NULL);

		pfree(formClass);
		
#endif //FOUDER_XDB_SE
	}
	PG_CATCH();
	{
		/* Ensure CurrentResourceOwner is restored on error */
		CurrentResourceOwner = currentOwner;
		PG_RE_THROW();
	}
	PG_END_TRY();
	CurrentResourceOwner = currentOwner;
}

#ifdef FOUNDER_XDB_SE

static
void clean_lo_hash()
{
	if(LocalLOInRelHash == NULL)
		return;

	HASH_SEQ_STATUS status;
	LORel *tmp_lo_rel = NULL;
	LOHashKey lokey;

	hash_seq_init(&status, LocalLOInRelHash);
	while((tmp_lo_rel = (LORel*)hash_seq_search(&status)) != NULL)
	{
		lokey.dbid = tmp_lo_rel->dbid;
		lokey.loid = tmp_lo_rel->loid;
		if(hash_search(LocalLOInRelHash, 
			(void*)&lokey,
			HASH_REMOVE,
			NULL) == NULL)
		{
			ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				errmsg("hash table corrupted")));
		}
		tmp_lo_rel->heap_rel = NULL;
		tmp_lo_rel->index_rel = NULL;
	}
}

#endif //FOUNDER_XDB_SE

/*
 * Clean up at main transaction end
 */
void
close_lo_relation(bool isCommit)
{
#ifndef FOUNDER_XDB_SE
	if (lo_heap_r || lo_index_r)
#else
	if (LocalLOInRelHash != NULL && 
		hash_get_num_entries(LocalLOInRelHash) > 0)
#endif //FOUNDER_XDB_SE
	{
		/*
		 * Only bother to close if committing; else abort cleanup will handle
		 * it
		 */
		if (isCommit)
		{
			ResourceOwner currentOwner;

			currentOwner = CurrentResourceOwner;
			PG_TRY();
			{
				CurrentResourceOwner = TopTransactionResourceOwner;

#ifndef FOUNDER_XDB_SE
				if (lo_index_r)
					index_close(lo_index_r, NoLock);
				if (lo_heap_r)
					heap_close(lo_heap_r, NoLock);
#else
				HASH_SEQ_STATUS status;
				LORel *tmp_lo_rel = NULL;
				LOHashKey lokey;

				hash_seq_init(&status, LocalLOInRelHash);
				while((tmp_lo_rel = (LORel*)hash_seq_search(&status)) != NULL)
				{
					if(tmp_lo_rel->index_rel != NULL)
					{
						index_close(tmp_lo_rel->index_rel, NoLock);
						tmp_lo_rel->index_rel = NULL;
					}

					if(tmp_lo_rel->heap_rel != NULL)
					{
						heap_close(tmp_lo_rel->heap_rel, NoLock);
						tmp_lo_rel->heap_rel = NULL;
					}
					
					lokey.loid = tmp_lo_rel->loid;
					lokey.dbid = tmp_lo_rel->dbid;
					if(hash_search(LocalLOInRelHash, 
								(void*)&lokey,
								HASH_REMOVE,
								NULL) == NULL)
					{
						ereport(ERROR,
							(errcode(ERRCODE_DATA_CORRUPTED),
							errmsg("hash table corrupted")));
					}
				}
#endif //FOUNDER_XDB_SE
			}
			PG_CATCH();
			{
				/* Ensure CurrentResourceOwner is restored on error */
				CurrentResourceOwner = currentOwner;
				PG_RE_THROW();
			}
			PG_END_TRY();
			CurrentResourceOwner = currentOwner;
		}
		lo_heap_r = NULL;
		lo_index_r = NULL;
	}
}


/*
 * Same as pg_largeobject.c's LargeObjectExists(), except snapshot to
 * read with can be specified.
 */
static bool
myLargeObjectExists(Oid loid, Snapshot snapshot)
{
#ifndef FOUNDER_XDB_SE
	Relation	pg_lo_meta;
	ScanKeyData skey[1];
	SysScanDesc sd;
	HeapTuple	tuple;
	bool		retval = false;

	ScanKeyInit(&skey[0],
		ObjectIdAttributeNumber,
		BTEqualStrategyNumber, F_OIDEQ,
		ObjectIdGetDatum(loid));

	pg_lo_meta = heap_open(LargeObjectMetadataRelationId,
		AccessShareLock);

	sd = systable_beginscan(pg_lo_meta,
		LargeObjectMetadataOidIndexId, true,
		snapshot, 1, skey);
#else
	Relation	pg_lo_meta;
	ScanKeyData skey[1];
	SysScanDesc sd;
	HeapTuple	tuple;
	bool		retval = false;

	Form_meta_large_table_rel *formClass = GetDatabaseLOMetaTable(MyDatabaseId);

	loid_equal_init_scankey(loid, skey);

	pg_lo_meta = heap_open(formClass->relid, AccessShareLock);

	sd = systable_beginscan(pg_lo_meta,
		formClass->index_loid, true,
		snapshot, 1, skey);
#endif //FOUNDER_XDB_SE

	tuple = systable_getnext(sd);
	if (HeapTupleIsValid(tuple))
		retval = true;

	systable_endscan(sd);

	heap_close(pg_lo_meta, NoLock);

#ifdef FOUNDER_XDB_SE
	pfree(formClass);
#endif //FOUNDER_XDB_SE

	return retval;
}


static int32
getbytealen(bytea *data)
{
	Assert(!VARATT_IS_EXTENDED(data));
	if (VARSIZE(data) < VARHDRSZ)
		ereport(ERROR,
                (errcode(ERRCODE_MOST_SPECIFIC_TYPE_MISMATCH),
                errmsg("invalid VARSIZE(data)")));
	return (VARSIZE(data) - VARHDRSZ);
}

#ifdef FOUNDER_XDB_SE

Oid get_last_loid(Oid dbid, bool ignoreError)
{
	Oid maxid = InvalidOid;
	
	Relation rel = heap_open(LargeObjMetaTableRelId, AccessShareLock);
	HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);
	HeapTuple tuple = NULL;
	Form_meta_large_table_rel *formClass = NULL;

	while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		formClass = (Form_meta_large_table_rel *)fxdb_tuple_to_chars(tuple);
		if (formClass->dbid == dbid)
			break;
		pfree(formClass);
		formClass = NULL;
	}
	heap_endscan(scan);
	heap_close(rel, AccessShareLock);

	if (formClass != NULL)
	{
		rel = NULL;
		scan = NULL;
		tuple = NULL;

		PG_TRY();
		{
			rel = heap_open(formClass->relid, AccessShareLock);
			scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);
			tuple = heap_getnext(scan, BackwardScanDirection);
			if (tuple != NULL)
			{
				Form_meta_large metaClass = (Form_meta_large)fxdb_tuple_to_chars(tuple);
				maxid = metaClass->lo_id;
				pfree(metaClass);
			}
		}
		PG_CATCH();
		{
			if (!ignoreError)
				PG_RE_THROW();
			
			int no = 0;
			char *msg = NULL;
			
			msg = get_errno_errmsg(no);
			FlushErrorState();
			ereport(WARNING, (errmsg(msg)));
			pfree(msg);
		}
		PG_END_TRY();

		if (scan)
			heap_endscan(scan);
		if (rel)
			heap_close(rel, NoLock);
		if (tuple)
			pfree(formClass);
	}

	return maxid;
}

static
Oid get_next_lo_id()
{
	extern MemoryContext DBInfoContext;
	Oid tmp_loid = 0;

	if (DBInfoContext == NULL)
	{
		LWLockAcquire(DBInfoLock, LW_EXCLUSIVE);
		RebuildDBInfo(true);
		LWLockRelease(DBInfoLock);
	}
	
	LWLockAcquire(DBInfoLock, LW_SHARED);
	DBInfo *info = GetDBInfo(MyDatabaseId);
	SpinLockAcquire(&info->loid_lock);
	tmp_loid = info->next_loid++;
	SpinLockRelease(&info->loid_lock);
	LWLockRelease(DBInfoLock);

	return tmp_loid;
}

static
Oid fxdb_large_object_create(const char *name,
														 const uint32 extraDataLen, 
														 const void *extraData)
{
	Form_meta_large formClass = (Form_meta_large)palloc0(sizeof(Form_meta_large_data));
	do
	{
		formClass->lo_id = get_next_lo_id();
	} while(lo_exit(formClass->lo_id));
	Oid lo_id = formClass->lo_id;
	memcpy(formClass->lo_name.data, name, strlen(name) + 1);
	formClass->lo_tblspace = lo_tbl_space;
	formClass->extra_data_size = extraDataLen;
	formClass->rel_id = lo_heap_id;
	formClass->index_id = lo_index_id;
	uint32 data_len = sizeof(Form_meta_large_data) + extraDataLen;
	char *data = (char*)palloc0(data_len);
	memcpy(data, formClass, sizeof(Form_meta_large_data));
	memcpy(data + sizeof(Form_meta_large_data), extraData, extraDataLen);

	Form_meta_large_table_rel *tClass = GetDatabaseLOMetaTable(MyDatabaseId);

	Relation lo_meta = heap_open(tClass->relid, RowExclusiveLock);
	HeapTuple tuple = fdxdb_heap_formtuple(data, data_len);
	simple_heap_insert(lo_meta, tuple);
	heap_close(lo_meta, NoLock);

	pfree(tClass);
	pfree(tuple);
	pfree(formClass);
	pfree(data);

	return lo_id;
}

#endif //FOUNDER_XDB_SE

/*
 *	inv_create -- create a new large object
 *
 *	Arguments:
 *	  lobjId - OID to use for new large object, or InvalidOid to pick one
 *
 *	Returns:
 *	  OID of new object
 *
 * If lobjId is not InvalidOid, then an error occurs if the OID is already
 * in use.
 */
#ifndef FOUNDER_XDB_SE
Oid
inv_create(Oid lobjId)
#else
Oid
inv_create(char *name,
					 int extraDataLen, 
					 void *extraData)
#endif //FOUNDER_XDB_SE
{
	Oid			lobjId_new;

#ifndef FOUNDER_XDB_SE
	/*
	 * Create a new largeobject with empty data pages
	 */
	lobjId_new = LargeObjectCreate(lobjId);

	/*
	 * dependency on the owner of largeobject
	 *
	 * The reason why we use LargeObjectRelationId instead of
	 * LargeObjectMetadataRelationId here is to provide backward compatibility
	 * to the applications which utilize a knowledge about internal layout of
	 * system catalogs. OID of pg_largeobject_metadata and loid of
	 * pg_largeobject are same value, so there are no actual differences here.
	 */
	recordDependencyOnOwner(LargeObjectRelationId,
							//lobjId_new, GetUserId());

	/* Post creation hook for new large object */
	InvokeObjectAccessHook(OAT_POST_CREATE,
						   LargeObjectRelationId, lobjId_new, 0);
#endif //FOUNDER_XDB_SE

	SetOpenLORelation();
	lobjId_new = fxdb_large_object_create(name, extraDataLen, extraData);

	/*
	 * Advance command counter to make new tuple visible to later operations.
	 */
	CommandCounterIncrement();

	return lobjId_new;
}

#ifdef FOUNDER_XDB_SE
LargeObjectDesc *fxdb_inv_name_open(char *name, int flags, MemoryContext mcxt)
{
	Oid lobjId = name_get_loid(name);
	if (lobjId == 0)
		ereport(ERROR,
		(errcode(ERRCODE_ACCESS_OBJECT_NOT_EXIST),
		errmsg("large object %s does not exist", name)));

	return inv_open(lobjId, flags, mcxt);
}

int	fxdb_inv_name_drop(char *name)
{
	Oid lobjId = name_get_loid(name);
	if (lobjId == 0)
		ereport(ERROR,
		(errcode(ERRCODE_ACCESS_OBJECT_NOT_EXIST),
		errmsg("large object %s does not exist", name)));

	return inv_drop(lobjId);
}
#endif //FOUNDER_XDB_SE

/*
 *	inv_open -- access an existing large object.
 *
 *		Returns:
 *		  Large object descriptor, appropriately filled in.  The descriptor
 *		  and subsidiary data are allocated in the specified memory context,
 *		  which must be suitably long-lived for the caller's purposes.
 */
LargeObjectDesc *
inv_open(Oid lobjId, int flags, MemoryContext mcxt)
{
	LargeObjectDesc *retval;

	retval = (LargeObjectDesc *) MemoryContextAlloc(mcxt,
													sizeof(LargeObjectDesc));

	retval->id = lobjId;
	retval->subid = GetCurrentSubTransactionId();
	retval->offset = 0;

	if (flags & INV_WRITE)
	{
		retval->snapshot = SnapshotNow;
		retval->flags = IFS_WRLOCK | IFS_RDLOCK;
	}
	else if (flags & INV_READ)
	{
		/*
		 * We must register the snapshot in TopTransaction's resowner, because
		 * it must stay alive until the LO is closed rather than until the
		 * current portal shuts down.
		 */
		retval->snapshot = RegisterSnapshotOnOwner(GetActiveSnapshot(),
												TopTransactionResourceOwner);
		retval->flags = IFS_RDLOCK;
	}
	else
		ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                errmsg("invalid flags: %d", flags)));

	/* Can't use LargeObjectExists here because it always uses SnapshotNow */
#ifndef FOUNDER_XDB_SE
	if (!myLargeObjectExists(lobjId, retval->snapshot))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("large object %u does not exist", lobjId)));
#else
	if (!myLargeObjectExists(lobjId, retval->snapshot))
	{
		if(flags & INV_READ)
			UnregisterSnapshotFromOwner(retval->snapshot, 
				TopTransactionResourceOwner);

		ereport(ERROR,
		(errcode(ERRCODE_UNDEFINED_OBJECT),
		errmsg("large object %u does not exist", lobjId)));
	}
#endif //FOUNDER_XDB_SE

	return retval;
}

/*
 * Closes a large object descriptor previously made by inv_open(), and
 * releases the long-term memory used by it.
 */
void
inv_close(LargeObjectDesc *obj_desc)
{
	Assert(PointerIsValid(obj_desc));

	if (obj_desc->snapshot->satisfies != SnapshotNow->satisfies)
		UnregisterSnapshotFromOwner(obj_desc->snapshot,
									TopTransactionResourceOwner);

	pfree(obj_desc);
}

#ifdef FOUNDER_XDB_SE

void Fxdb_AtEOXact_LargeObject(bool isCommit)
{
	close_lo_relation(isCommit);
	clean_lo_hash();
	hash_destroy(LocalLOInRelHash);
	LocalLOInRelHash = NULL;
}

void Fxdb_AtEndStatement_LargeObject()
{
	if (LocalLOInRelHash != NULL && 
		hash_get_num_entries(LocalLOInRelHash) > 0)
	{
		PG_TRY();
		{
			HASH_SEQ_STATUS status;
			LORel *tmp_lo_rel = NULL;
			LOHashKey lokey;

			hash_seq_init(&status, LocalLOInRelHash);
			while((tmp_lo_rel = (LORel*)hash_seq_search(&status)) != NULL)
			{
				if(tmp_lo_rel->index_rel != NULL)
				{
					index_close(tmp_lo_rel->index_rel, NoLock);
					tmp_lo_rel->index_rel = NULL;
				}

				if(tmp_lo_rel->heap_rel != NULL)
				{
					heap_close(tmp_lo_rel->heap_rel, NoLock);
					tmp_lo_rel->heap_rel = NULL;
				}

				lokey.loid = tmp_lo_rel->loid;
				lokey.dbid = tmp_lo_rel->dbid;
				if(hash_search(LocalLOInRelHash, 
					(void*)&lokey,
					HASH_REMOVE,
					NULL) == NULL)
				{
					ereport(ERROR,
						(errcode(ERRCODE_DATA_CORRUPTED),
						errmsg("hash table corrupted")));
				}
			}
		}
		PG_CATCH();
		{
			/* Ensure CurrentResourceOwner is restored on error */
			PG_RE_THROW();
		}
		PG_END_TRY();

		lo_heap_r = NULL;
		lo_index_r = NULL;
	}
}

static
bool fxdb_inv_lo_drop_data(Form_meta_large formClass)
{
	Assert(formClass != NULL);

	SysScanDesc sd;
	Relation lo_data, i_lo_data;
	bool retval = false;

	lo_data = heap_open(formClass->rel_id, RowExclusiveLock);
	i_lo_data = index_open(formClass->index_id, AccessShareLock);

	ScanKeyData skey;
	char *key = (char*)palloc0(sizeof(formClass->lo_id));
	memcpy(key, &formClass->lo_id, sizeof(formClass->lo_id));
	Datum kdatum = fdxdb_string_formdatum(key, sizeof(formClass->lo_id));

	Fdxdb_ScanKeyInitWithCallbackInfo(&skey,
		LARGE_OBJ_LO_ID,
		BTEqualStrategyNumber,
		relid_compare,
		kdatum);

	sd = systable_beginscan_ordered(lo_data, i_lo_data,
		SnapshotNow, 1, &skey);

	HeapTuple tuple = NULL;
	while ((tuple = systable_getnext_ordered(sd, ForwardScanDirection)) != NULL)
	{
		simple_heap_delete(lo_data, &tuple->t_self);
		retval = true;
	}
	systable_endscan(sd);

	heap_close(lo_data, NoLock);

	pfree(key);

	return retval;
}

static 
bool fxdb_inv_lo_drop_one(Oid lobjId)
{
	bool retval = false;
	Relation lo_meta;

	Form_meta_large_table_rel *tClass = GetDatabaseLOMetaTable(MyDatabaseId);

	lo_meta = heap_open(tClass->relid, RowExclusiveLock);

	HeapTuple tuple = id_get_heaptuple_copy(lobjId);
	Form_meta_large formClass = NULL;

	if (HeapTupleIsValid(tuple))
	{
		formClass = (Form_meta_large)fxdb_tuple_to_chars(tuple);
		simple_heap_delete(lo_meta, &tuple->t_self);
		retval = true;
	}
	
	/* 删除大对象数据 */
	if(retval)
		fxdb_inv_lo_drop_data(formClass);

	heap_close(lo_meta, NoLock);

	if(formClass)
		pfree(formClass);
	if(tuple)
		pfree(tuple);

	pfree(tClass);

	return retval;
}

#endif //FOUNDER_XDB_SE

/*
 * Destroys an existing large object (not to be confused with a descriptor!)
 *
 * returns -1 if failed
 */
int
inv_drop(Oid lobjId)
{
#ifndef FOUNDER_XDB_SE
	ObjectAddress object;

	/*
	 * Delete any comments and dependencies on the large object
	 */
	object.classId = LargeObjectRelationId;
	object.objectId = lobjId;
	object.objectSubId = 0;
	performDeletion(&object, DROP_CASCADE);
#else
	int retval = 0;
	retval = fxdb_inv_lo_drop_one(lobjId);
#endif //FOUNDER_XDB_SE

	/*
	 * Advance command counter so that tuple removal will be seen by later
	 * large-object operations in this transaction.
	 */
	CommandCounterIncrement();

#ifndef FOUNDER_XDB_SE
	return 1;
#else
	return retval;
#endif //FOUNDER_XDB_SE
}

/*
 * Determine size of a large object
 *
 * NOTE: LOs can contain gaps, just like Unix files.  We actually return
 * the offset of the last byte + 1.
 */
static uint64
inv_getsize(LargeObjectDesc *obj_desc)
{
	uint64		lastbyte = 0;
	ScanKeyData skey[1];
	SysScanDesc sd;
	HeapTuple	tuple;

	Assert(PointerIsValid(obj_desc));

#ifndef FOUNDER_XDB_SE
	open_lo_relation();

	ScanKeyInit(&skey[0],
				Anum_pg_largeobject_loid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(obj_desc->id));
#else
	open_lo_relation(obj_desc);

	char *key = (char*)palloc0(sizeof(obj_desc->id));
	memcpy(key, &obj_desc->id, sizeof(obj_desc->id));
	Datum datum = fdxdb_string_formdatum(key, sizeof(obj_desc->id));

	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[0],
		LARGE_OBJ_LO_ID,
		BTEqualStrategyNumber,
		relid_compare,
		datum);
#endif //FOUNDER_XDB_SE

	sd = systable_beginscan_ordered(lo_heap_r, lo_index_r,
									obj_desc->snapshot, 1, skey);

	/*
	 * Because the pg_largeobject index is on both loid and pageno, but we
	 * constrain only loid, a backwards scan should visit all pages of the
	 * large object in reverse pageno order.  So, it's sufficient to examine
	 * the first valid tuple (== last valid page).
	 */
	tuple = systable_getnext_ordered(sd, BackwardScanDirection);
	if (HeapTupleIsValid(tuple))
	{
#ifndef FOUNDER_XDB_SE
		Form_pg_largeobject data;
#else
		Form_large data;
		bool should_free = false;
#endif //FOUNDER_XDB_SE
		bytea	   *datafield;
		bool		pfreeit;

		if (HeapTupleHasNulls(tuple))	/* paranoia */
			ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                    errmsg("null field found in pg_largeobject")));
#ifndef FOUNDER_XDB_SE
		data = (Form_pg_largeobject) GETSTRUCT(tuple);
		datafield = &(data->data);		/* see note at top of file */
#else
		data = (Form_large) fxdb_get_metastruct_ptr(tuple, &should_free);
		datafield = &(data->workbuf.hdr);		/* see note at top of file */
#endif //FOUNDER_XDB_SE
		pfreeit = false;
		if (VARATT_IS_EXTENDED(datafield))
		{
			datafield = (bytea *)
				heap_tuple_untoast_attr((struct varlena *) datafield);
			pfreeit = true;
		}
		lastbyte = data->pageno * LOBLKSIZE + getbytealen(datafield);
		if (pfreeit)
			pfree(datafield);

#ifdef FOUNDER_XDB_SE
		if(should_free)
			pfree(data);
#endif //FOUNDER_XDB_SE
	}

	systable_endscan_ordered(sd);

#ifdef FOUNDER_XDB_SE
	pfree(key);
#endif //FOUNDER_XDB_SE

	return lastbyte;
}

uint64
inv_seek(LargeObjectDesc *obj_desc, int64 offset, int whence)
{
	Assert(PointerIsValid(obj_desc));

	switch (whence)
	{
		case SEEK_SET:
			if (offset < 0)
				ereport(ERROR,
                        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                        errmsg("invalid seek offset:" INT64_FORMAT, offset)));
			obj_desc->offset = offset;
			break;
		case SEEK_CUR:
			if (offset < 0 && obj_desc->offset < ((uint64) (-offset)))
				ereport(ERROR,
                        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                        errmsg("invalid seek offset:" INT64_FORMAT, offset)));
			obj_desc->offset += offset;
			break;
		case SEEK_END:
			{
				uint64		size = inv_getsize(obj_desc);

				if (offset < 0 && size < ((uint64) (-offset)))
					ereport(ERROR,
                            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                            errmsg("invalid seek offset:" INT64_FORMAT, offset)));
				obj_desc->offset = size + offset;
			}
			break;
		default:
			ereport(ERROR,
                    (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                    errmsg("invalid whence: %d", whence)));
	}
	return obj_desc->offset;
}

uint64
inv_tell(LargeObjectDesc *obj_desc)
{
	Assert(PointerIsValid(obj_desc));

	return obj_desc->offset;
}

int
inv_read(LargeObjectDesc *obj_desc, char *buf, int nbytes)
{
	int			nread = 0;
	int			n;
	int			off;
	int			len;
	int64		pageno = (int32) (obj_desc->offset / LOBLKSIZE);
	uint64		pageoff;
	ScanKeyData skey[2];
	SysScanDesc sd;
	HeapTuple	tuple;

	Assert(PointerIsValid(obj_desc));
	Assert(buf != NULL);

	if (nbytes <= 0)
		return 0;

#ifndef FOUNDER_XDB_SE
	open_lo_relation();

	ScanKeyInit(&skey[0],
				Anum_pg_largeobject_loid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(obj_desc->id));

	ScanKeyInit(&skey[1],
				Anum_pg_largeobject_pageno,
				BTGreaterEqualStrategyNumber, F_INT4GE,
				Int32GetDatum(pageno));
#else
	open_lo_relation(obj_desc);

	char *key = (char*)palloc0(sizeof(obj_desc->id));
	memcpy(key, &obj_desc->id, sizeof(obj_desc->id));
	Datum kdatum = fdxdb_string_formdatum(key, sizeof(obj_desc->id));

	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[0],
		LARGE_OBJ_LO_ID,
		BTEqualStrategyNumber,
		relid_compare,
		kdatum);

	char *key2 = (char*)palloc0(sizeof(pageno));
	memcpy(key2, &pageno, sizeof(pageno));
	Datum kdatum2 = fdxdb_string_formdatum(key2, sizeof(pageno));
	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[1],
		LARGE_OBJ_PAGENO,
		BTGreaterEqualStrategyNumber,
		int4_compare,
		kdatum2);
#endif //FOUNDER_XDB_SE

	sd = systable_beginscan_ordered(lo_heap_r, lo_index_r,
									obj_desc->snapshot, 2, skey);

	while ((tuple = systable_getnext_ordered(sd, ForwardScanDirection)) != NULL)
	{
#ifndef FOUNDER_XDB_SE
		Form_pg_largeobject data;
#else
		Form_large data;
		bool should_free = false;
#endif //FOUNDER_XDB_SE
		bytea	   *datafield;
		bool		pfreeit;

		if (HeapTupleHasNulls(tuple))	/* paranoia */
			ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                    errmsg("null field found in pg_largeobject")));
#ifndef FOUNDER_XDB_SE
		data = (Form_pg_largeobject) GETSTRUCT(tuple);
#else
		data = (Form_large)fxdb_get_metastruct_ptr(tuple, &should_free);
#endif //FOUNDER_XDB_SE

		/*
		 * We expect the indexscan will deliver pages in order.  However,
		 * there may be missing pages if the LO contains unwritten "holes". We
		 * want missing sections to read out as zeroes.
		 */
		pageoff = ((uint64) data->pageno) * LOBLKSIZE;
		if (pageoff > obj_desc->offset)
		{
			n = (int)(pageoff - obj_desc->offset);
			n = (n <= (nbytes - nread)) ? n : (nbytes - nread);
			MemSet(buf + nread, 0, n);
			nread += n;
			obj_desc->offset += n;
		}

		if (nread < nbytes)
		{
			Assert(obj_desc->offset >= pageoff);
			off = (int) (obj_desc->offset - pageoff);
			Assert(off >= 0 && off < LOBLKSIZE);

#ifndef FOUNDER_XDB_SE
			datafield = &(data->data);	/* see note at top of file */
#else
			datafield = &(data->workbuf.hdr);
#endif //FOUNDER_XDB_SE
			pfreeit = false;
			if (VARATT_IS_EXTENDED(datafield))
			{
				datafield = (bytea *)
					heap_tuple_untoast_attr((struct varlena *) datafield);
				pfreeit = true;
			}
			len = getbytealen(datafield);
			if (len > off)
			{
				n = len - off;
				n = (n <= (nbytes - nread)) ? n : (nbytes - nread);
				memcpy(buf + nread, VARDATA(datafield) + off, n);
				nread += n;
				obj_desc->offset += n;
			}
			if (pfreeit)
				pfree(datafield);
		}

#ifdef FOUNDER_XDB_SE
		if(should_free)
			pfree(data);
#endif //FOUNDER_XDB_SE

		if (nread >= nbytes)
			break;
	}

	systable_endscan_ordered(sd);

#ifdef FOUNDER_XDB_SE
	pfree(key);
	pfree(key2);
#endif //FOUNDER_XDB_SE

	return nread;
}


int
inv_write(LargeObjectDesc *obj_desc, const char *buf, int nbytes)
{
	int			nwritten = 0;
	int			n;
	int			off;
	int			len;
	int64		pageno = (int64) (obj_desc->offset / LOBLKSIZE);
	ScanKeyData skey[2];
	SysScanDesc sd;
	HeapTuple	oldtuple;

#ifndef FOUNDER_XDB_SE
	Form_pg_largeobject olddata;
#else
	Form_large olddata;
#endif //FOUNDER_XDB_SE
	bool		neednextpage;

	bytea	   *datafield;
	bool		pfreeit;
	struct
	{
		bytea		hdr;
		char		data[LOBLKSIZE];	/* make struct big enough */
		int32		align_it;	/* ensure struct is aligned well enough */
	}			workbuf;
	char	   *workb = VARDATA(&workbuf.hdr);
	HeapTuple	newtup;
#ifndef FOUNDER_XDB_SE
	Datum		values[Natts_pg_largeobject];
	bool		nulls[Natts_pg_largeobject];
	bool		replace[Natts_pg_largeobject];

	CatalogIndexState indstate;
#endif //FOUNDER_XDB_SE

	Assert(PointerIsValid(obj_desc));
	Assert(buf != NULL);

	/* enforce writability because snapshot is probably wrong otherwise */
	if ((obj_desc->flags & IFS_WRLOCK) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("large object %u was not opened for writing",
						obj_desc->id)));

	/* check existence of the target largeobject */
#ifndef FOUNDER_XDB_SE
	if (!LargeObjectExists(obj_desc->id))
#else
	if (!lo_exit(obj_desc->id, NULL))
#endif //FOUNDER_XDB_SE
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
			   errmsg("large object %u was already dropped", obj_desc->id)));

	if (nbytes <= 0)
		return 0;

#ifndef FOUNDER_XDB_SE
	open_lo_relation();

	indstate = CatalogOpenIndexes(lo_heap_r);

	ScanKeyInit(&skey[0],
				Anum_pg_largeobject_loid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(obj_desc->id));

	ScanKeyInit(&skey[1],
				Anum_pg_largeobject_pageno,
				BTGreaterEqualStrategyNumber, F_INT4GE,
				Int32GetDatum(pageno));
#else
	open_lo_relation(obj_desc);

	
	Datum datum = fdxdb_string_formdatum((char*)(&(obj_desc->id)), sizeof(Oid));
	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[0],
		LARGE_OBJ_LO_ID,
		BTEqualStrategyNumber,
		relid_compare,
		datum);

	Datum datum2 = fdxdb_string_formdatum((char*)&pageno, sizeof(pageno));
	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[1],
		LARGE_OBJ_PAGENO,
		BTGreaterEqualStrategyNumber,
		int64_compare,
		datum2);
#endif //FOUNDER_XDB_SE

	sd = systable_beginscan_ordered(lo_heap_r, lo_index_r,
									obj_desc->snapshot, 2, skey);

	oldtuple = NULL;
	olddata = NULL;
	neednextpage = true;

	while (nwritten < nbytes)
	{
#ifdef FOUNDER_XDB_SE
		bool should_free = false;
#endif //FOUNDER_XDB_SE
		/*
		 * If possible, get next pre-existing page of the LO.  We expect the
		 * indexscan will deliver these in order --- but there may be holes.
		 */
		if (neednextpage)
		{
			if ((oldtuple = systable_getnext_ordered(sd, ForwardScanDirection)) != NULL)
			{
				if (HeapTupleHasNulls(oldtuple))		/* paranoia */
					ereport(ERROR,
                            (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                            errmsg("null field found in pg_largeobject")));
#ifndef FOUNDER_XDB_SE
				olddata = (Form_pg_largeobject) GETSTRUCT(oldtuple);
#else
				olddata = (Form_large)fxdb_get_metastruct_ptr(oldtuple, &should_free);
#endif //FOUNDER_XDB_SE
				Assert(olddata->pageno >= pageno);
			}
			neednextpage = false;
		}

		/*
		 * If we have a pre-existing page, see if it is the page we want to
		 * write, or a later one.
		 */
		if (olddata != NULL && olddata->pageno == pageno)
		{
			/*
			 * Update an existing page with fresh data.
			 *
			 * First, load old data into workbuf
			 */
#ifndef FOUNDER_XDB_SE
			datafield = &(olddata->data);		/* see note at top of file */
#else
			datafield = &(olddata->workbuf.hdr);
#endif //FOUNDER_XDB_SE

			pfreeit = false;
			if (VARATT_IS_EXTENDED(datafield))
			{
				datafield = (bytea *)
					heap_tuple_untoast_attr((struct varlena *) datafield);
				pfreeit = true;
			}
			len = getbytealen(datafield);
			Assert(len <= LOBLKSIZE);
			memcpy(workb, VARDATA(datafield), len);
			if (pfreeit)
				pfree(datafield);

			/*
			 * Fill any hole
			 */
			off = (int) (obj_desc->offset % LOBLKSIZE);
			if (off > len)
				MemSet(workb + len, 0, off - len);

			/*
			 * Insert appropriate portion of new data
			 */
			n = LOBLKSIZE - off;
			n = (n <= (nbytes - nwritten)) ? n : (nbytes - nwritten);
			memcpy(workb + off, buf + nwritten, n);
			nwritten += n;
			obj_desc->offset += n;
			off += n;
			/* compute valid length of new page */
			len = (len >= off) ? len : off;
			SET_VARSIZE(&workbuf.hdr, len + VARHDRSZ);

#ifndef FOUNDER_XDB_SE
			/*
			 * Form and insert updated tuple
			 */
			memset(values, 0, sizeof(values));
			memset(nulls, false, sizeof(nulls));
			memset(replace, false, sizeof(replace));
			values[Anum_pg_largeobject_data - 1] = PointerGetDatum(&workbuf);
			replace[Anum_pg_largeobject_data - 1] = true;
			newtup = heap_modify_tuple(oldtuple, RelationGetDescr(lo_heap_r),
									   values, nulls, replace);

			simple_heap_update(lo_heap_r, &newtup->t_self, newtup);
			CatalogIndexInsert(indstate, newtup);
			heap_freetuple(newtup);
#else
			Form_large tmpClass = (Form_large)palloc0(sizeof(Form_large_data));
			tmpClass->lo_id = obj_desc->id;
			tmpClass->pageno = pageno;
			memcpy(&tmpClass->workbuf, &workbuf, sizeof(workbuf));
			newtup = fdxdb_heap_formtuple((char*)tmpClass, sizeof(Form_large_data));

			simple_heap_update(lo_heap_r, &oldtuple->t_self, newtup);

			heap_freetuple(newtup);
			pfree(tmpClass);
#endif //FOUNDER_XDB_SE

#ifdef FOUNDER_XDB_SE
			if(should_free)
				pfree(olddata);
#endif //FOUNDER_XDB_SE

			/*
			 * We're done with this old page.
			 */
			oldtuple = NULL;
			olddata = NULL;
			neednextpage = true;
		}
		else
		{
			/*
			 * Write a brand new page.
			 *
			 * First, fill any hole
			 */
			off = (int) (obj_desc->offset % LOBLKSIZE);
			if (off > 0)
				MemSet(workb, 0, off);

			/*
			 * Insert appropriate portion of new data
			 */
			n = LOBLKSIZE - off;
			n = (n <= (nbytes - nwritten)) ? n : (nbytes - nwritten);
			memcpy(workb + off, buf + nwritten, n);
			nwritten += n;
			obj_desc->offset += n;
			/* compute valid length of new page */
			len = off + n;
			SET_VARSIZE(&workbuf.hdr, len + VARHDRSZ);

			/*
			 * Form and insert updated tuple
			 */
#ifndef FOUNDER_XDB_SE
			memset(values, 0, sizeof(values));
			memset(nulls, false, sizeof(nulls));
			values[Anum_pg_largeobject_loid - 1] = ObjectIdGetDatum(obj_desc->id);
			values[Anum_pg_largeobject_pageno - 1] = Int32GetDatum(pageno);
			values[Anum_pg_largeobject_data - 1] = PointerGetDatum(&workbuf);
			newtup = heap_form_tuple(lo_heap_r->rd_att, values, nulls);

			simple_heap_insert(lo_heap_r, newtup);
			CatalogIndexInsert(indstate, newtup);
#else
			Form_large tmpClass = (Form_large)palloc0(sizeof(Form_large_data));
			tmpClass->lo_id = obj_desc->id;
			tmpClass->pageno = pageno;
			memcpy(&tmpClass->workbuf, &workbuf, sizeof(workbuf));
			newtup = fdxdb_heap_formtuple((char*)tmpClass, sizeof(Form_large_data));
#endif //FOUNDER_XDB_SE
			simple_heap_insert(lo_heap_r, newtup);
			heap_freetuple(newtup);
			pfree(tmpClass);
		}
		pageno++;
	}

	systable_endscan_ordered(sd);

#ifndef FOUNDER_XDB_SE
	CatalogCloseIndexes(indstate);
#endif //FOUNDER_XDB_SE

	/*
	 * Advance command counter so that my tuple updates will be seen by later
	 * large-object operations in this transaction.
	 */
	CommandCounterIncrement();

	return nwritten;
}


void
inv_truncate(LargeObjectDesc *obj_desc, int64 len)
{
	int64		pageno = (int64) (len / LOBLKSIZE);
	int			off;
	ScanKeyData skey[2];
	SysScanDesc sd;
	HeapTuple	oldtuple;

#ifndef FOUNDER_XDB_SE
	Form_pg_largeobject olddata;
#else
	Form_large olddata;
	bool should_free = false;
#endif //FOUNDER_XDB_SE

	struct
	{
		bytea		hdr;
		char		data[LOBLKSIZE];	/* make struct big enough */
		int32		align_it;	/* ensure struct is aligned well enough */
	}			workbuf;
	char	   *workb = VARDATA(&workbuf.hdr);
	HeapTuple	newtup;

#ifndef FOUNDER_XDB_SE
	Datum		values[Natts_pg_largeobject];
	bool		nulls[Natts_pg_largeobject];
	bool		replace[Natts_pg_largeobject];
	CatalogIndexState indstate;
#endif //FOUNDER_XDB_SE

	Assert(PointerIsValid(obj_desc));

	/* enforce writability because snapshot is probably wrong otherwise */
	if ((obj_desc->flags & IFS_WRLOCK) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("large object %u was not opened for writing",
						obj_desc->id)));

	/* check existence of the target largeobject */
#ifndef FOUNDER_XDB_SE
	if (!LargeObjectExists(obj_desc->id))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
			   errmsg("large object %u was already dropped", obj_desc->id)));
#else
	if (!lo_exit(obj_desc->id, NULL))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				errmsg("large object %u was already dropped", obj_desc->id)));
#endif //FOUNDER_XDB_SE

#ifndef FOUNDER_XDB_SE
	open_lo_relation();

	indstate = CatalogOpenIndexes(lo_heap_r);

	/*
	 * Set up to find all pages with desired loid and pageno >= target
	 */
	ScanKeyInit(&skey[0],
				Anum_pg_largeobject_loid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(obj_desc->id));

	ScanKeyInit(&skey[1],
				Anum_pg_largeobject_pageno,
				BTGreaterEqualStrategyNumber, F_INT4GE,
				Int32GetDatum(pageno));
#else
	open_lo_relation(obj_desc);

	char *key1 = (char*)palloc0(sizeof(obj_desc->id));
	memcpy(key1, &obj_desc->id, sizeof(obj_desc->id));
	Datum datum1 = fdxdb_string_formdatum(key1, sizeof(obj_desc->id));
	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[0],
		LARGE_OBJ_LO_ID,
		BTEqualStrategyNumber,
		relid_compare,
		datum1);

	char *key2 = (char*)palloc0(sizeof(pageno));
	memcpy(key2, &pageno, sizeof(pageno));
	Datum datum2 = fdxdb_string_formdatum(key2, sizeof(pageno));
	Fdxdb_ScanKeyInitWithCallbackInfo(&skey[1],
		LARGE_OBJ_PAGENO,
		BTGreaterEqualStrategyNumber,
		int64_compare,
		datum2);
#endif //FOUNDER_XDB_SE

	sd = systable_beginscan_ordered(lo_heap_r, lo_index_r,
									obj_desc->snapshot, 2, skey);

	/*
	 * If possible, get the page the truncation point is in. The truncation
	 * point may be beyond the end of the LO or in a hole.
	 */
	olddata = NULL;
	if ((oldtuple = systable_getnext_ordered(sd, ForwardScanDirection)) != NULL)
	{
		if (HeapTupleHasNulls(oldtuple))		/* paranoia */
			ereport(ERROR,
                    (errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
                    errmsg("null field found in pg_largeobject")));
#ifndef FOUNDER_XDB_SE
		olddata = (Form_pg_largeobject) GETSTRUCT(oldtuple);
#else
		olddata = (Form_large) fxdb_get_metastruct_ptr(oldtuple, &should_free);
#endif //FOUNDER_XDB_SE
		Assert(olddata->pageno >= pageno);
	}

	/*
	 * If we found the page of the truncation point we need to truncate the
	 * data in it.	Otherwise if we're in a hole, we need to create a page to
	 * mark the end of data.
	 */
	if (olddata != NULL && olddata->pageno == pageno)
	{
		/* First, load old data into workbuf */
#ifndef FOUNDER_XDB_SE
		bytea	   *datafield = &(olddata->data);		/* see note at top of
														 * file */
#else
		bytea	   *datafield = &(olddata->workbuf.hdr);		/* see note at top of
														 * file */
#endif //FOUNDER_XDB_SE
		bool		pfreeit = false;
		int			pagelen;

		if (VARATT_IS_EXTENDED(datafield))
		{
			datafield = (bytea *)
				heap_tuple_untoast_attr((struct varlena *) datafield);
			pfreeit = true;
		}
		pagelen = getbytealen(datafield);
		Assert(pagelen <= LOBLKSIZE);
		memcpy(workb, VARDATA(datafield), pagelen);
		if (pfreeit)
			pfree(datafield);

		/*
		 * Fill any hole
		 */
		off = len % LOBLKSIZE;
		if (off > pagelen)
			MemSet(workb + pagelen, 0, off - pagelen);

		/* compute length of new page */
		SET_VARSIZE(&workbuf.hdr, off + VARHDRSZ);

		/*
		 * Form and insert updated tuple
		 */
#ifndef FOUNDER_XDB_SE
		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));
		memset(replace, false, sizeof(replace));
		values[Anum_pg_largeobject_data - 1] = PointerGetDatum(&workbuf);
		replace[Anum_pg_largeobject_data - 1] = true;
		newtup = heap_modify_tuple(oldtuple, RelationGetDescr(lo_heap_r),
								   values, nulls, replace);

		simple_heap_update(lo_heap_r, &newtup->t_self, newtup);
		CatalogIndexInsert(indstate, newtup);
		heap_freetuple(newtup);
#else
		Form_large tmpData = (Form_large)palloc0(sizeof(Form_large_data));
		tmpData->lo_id = olddata->lo_id;
		tmpData->pageno = olddata->pageno;
		memcpy(&tmpData->workbuf, &workbuf, sizeof(workbuf));
		newtup = fdxdb_heap_formtuple((char*)tmpData, sizeof(Form_large_data));

		simple_heap_update(lo_heap_r, &oldtuple->t_self, newtup);
		heap_freetuple(newtup);
		pfree(tmpData);
#endif //FOUNDER_XDB_SE
	}
	else
	{
		/*
		 * If the first page we found was after the truncation point, we're in
		 * a hole that we'll fill, but we need to delete the later page
		 * because the loop below won't visit it again.
		 */
		if (olddata != NULL)
		{
			Assert(olddata->pageno > pageno);
			simple_heap_delete(lo_heap_r, &oldtuple->t_self);
		}

		/*
		 * Write a brand new page.
		 *
		 * Fill the hole up to the truncation point
		 */
		off = len % LOBLKSIZE;
		if (off > 0)
			MemSet(workb, 0, off);

		/* compute length of new page */
		SET_VARSIZE(&workbuf.hdr, off + VARHDRSZ);

		/*
		 * Form and insert new tuple
		 */
#ifndef FOUNDER_XDB_SE
		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));
		values[Anum_pg_largeobject_loid - 1] = ObjectIdGetDatum(obj_desc->id);
		values[Anum_pg_largeobject_pageno - 1] = Int32GetDatum(pageno);
		values[Anum_pg_largeobject_data - 1] = PointerGetDatum(&workbuf);
		newtup = heap_form_tuple(lo_heap_r->rd_att, values, nulls);
		simple_heap_insert(lo_heap_r, newtup);
		CatalogIndexInsert(indstate, newtup);
		heap_freetuple(newtup);
#else
		Form_large tmpData = (Form_large)palloc0(sizeof(Form_large_data));
		tmpData->lo_id = obj_desc->id;
		tmpData->pageno = pageno;
		memcpy(&tmpData->workbuf, &workbuf, sizeof(workbuf));
		newtup = fdxdb_heap_formtuple((char*)tmpData, sizeof(Form_large_data));
		simple_heap_insert(lo_heap_r, newtup);
		heap_freetuple(newtup);
		pfree(tmpData);
#endif //FOUNDER_XDB_SE
	}

	/*
	 * Delete any pages after the truncation point.  If the initial search
	 * didn't find a page, then of course there's nothing more to do.
	 */
	if (olddata != NULL)
	{
		while ((oldtuple = systable_getnext_ordered(sd, ForwardScanDirection)) != NULL)
		{
			simple_heap_delete(lo_heap_r, &oldtuple->t_self);
		}
	}

#ifdef FOUNDER_XDB_SE
	if(should_free)
		pfree(olddata);
#endif //FOUNDER_XDB_SE

	systable_endscan_ordered(sd);

#ifndef FOUNDER_XDB_SE
	CatalogCloseIndexes(indstate);
#endif //FOUNDER_XDB_SE

	/*
	 * Advance command counter so that tuple updates will be seen by later
	 * large-object operations in this transaction.
	 */
	CommandCounterIncrement();

#ifdef FOUNDER_XDB_SE
	pfree(key1);
	pfree(key2);
#endif //FOUNDER_XDB_SE
}

#ifdef FOUNDER_XDB_SE

void InitLOInfo(bool isInXactBlock)
{
	if (!isInXactBlock)
	{
		StartTransactionCommand();
		BeginTransactionBlock();
		CommitTransactionCommand();
	}

	HASH_SEQ_STATUS *hstat = GetDBInfoHashStatus();
	DBInfo *info = NULL;

	while((info = (DBInfo *)hash_seq_search(hstat)) != NULL)
	{
		SpinLockInit(&info->loid_lock);
		info->next_loid = get_last_loid(info->dbid, true) + 1;
		Assert(info->next_loid > 0);
	}

	if (!isInXactBlock)
	{
		StartTransactionCommand();
		EndTransactionBlock();
		CommitTransactionCommand();
	}
}
#endif //FOUNDER_XDB_SE
