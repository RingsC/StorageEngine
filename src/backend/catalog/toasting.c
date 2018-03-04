/*-------------------------------------------------------------------------
 *
 * toasting.c
 *	  This file contains routines to support creation of toast tables
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/catalog/toasting.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/heapam.h"
#include "access/tuptoaster.h"
#include "access/xact.h"
#ifndef FOUNDER_XDB_SE
#include "catalog/dependency.h"
#endif //FOUNDER_XDB_SE
#include "catalog/heap.h"
#include "catalog/index.h"
#include "catalog/indexing.h"
#ifndef FOUNDER_XDB_SE
#include "catalog/namespace.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_opclass.h"
#endif //FOUNDER_XDB_SE
#include "catalog/pg_type.h"
#include "catalog/toasting.h"
#include "miscadmin.h"
#ifndef FOUNDER_XDB_SE
#include "nodes/makefuncs.h"
#endif
#include "utils/builtins.h"
#include "utils/syscache.h"
#ifdef FOUNDER_XDB_SE
#include "catalog/xdb_catalog.h"
#include "port/thread_commu.h"
#endif //FOUNDER_XDB_SE


/* Potentially set by contrib/pg_upgrade_support functions */
extern Oid	binary_upgrade_next_toast_pg_class_oid;

Oid			binary_upgrade_next_toast_pg_type_oid = InvalidOid;

bool create_toast_table(Relation rel, Oid toastOid, Oid toastIndexOid,
				   Datum reloptions);
static bool needs_toast_table(Relation rel);

#ifdef FOUNDER_XDB_SE
int toast_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
    Assert(sizeof(Oid) == len1 && sizeof(Oid) == len2);
	Oid l,r;
	memcpy(&l,str1,sizeof(Oid));
	memcpy(&r,str2,sizeof(Oid));

	if (l < r)
	{
		return -1;
	}
	else if (l > r)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void toast_split(RangeData& range, const char *pszNeedSplit,int iIndexOfColumn, size_t len = 0)
{
	if (1 == iIndexOfColumn)
	{
		range.start = 0;
		range.len = sizeof(Oid);
	}
	else if (2 == iIndexOfColumn)
	{
		range.start = sizeof(Oid);
		range.len = sizeof(Oid);
	}
	else
	{
		range.start = 2 * sizeof(Oid);
	}
	return ;
}
extern TupleDesc toast_attibute_tupDesc;
static const Oid TOAST_INDEX_COLINFO_ID = 252;
static TupleDesc g_pToastTupleDesc = NULL;
static pthread_once_t once_init_toast_index_colinfo = PTHREAD_ONCE_INIT;
static pthread_once_t once_init_toast_tuple_desc = PTHREAD_ONCE_INIT;
void init_toast_index_colinfo(void)
{
	static ColinfoData colinfo;
	static size_t col_num[2] = {1,2};
	static CompareCallback call_back[2] = {toast_compare,toast_compare};
	colinfo.keys = 2;
	colinfo.col_number = col_num;
	colinfo.rd_comfunction = call_back;
	colinfo.split_function = toast_split;

	extern void setColInfo(Oid colid, Colinfo pcol_info);
	setColInfo(TOAST_INDEX_COLINFO_ID,&colinfo);
}

Oid get_toast_index_colinfo( void )
{
	pthread_once(&once_init_toast_index_colinfo,init_toast_index_colinfo);

	return TOAST_INDEX_COLINFO_ID;
}

void init_toast_tuple_desc(void)
{
	static FormData_pg_attribute text_attribute =
	{ 0,
	{ "text" }, 25, -1, -1, 1, 0, -1, -1, false, 'p', 'i', false, false, false,
	true, 0 ,100};
	g_pToastTupleDesc = toast_attibute_tupDesc;

	*(g_pToastTupleDesc->attrs) = &text_attribute;
}

TupleDesc get_toast_tuple_desc(void)
{
   pthread_once(&once_init_toast_tuple_desc,init_toast_tuple_desc);
   return g_pToastTupleDesc;
}
/*
 * create_toast_table --- internal workhorse
 *
 * rel is already opened and locked
 * toastOid and toastIndexOid are normally InvalidOid, but during
 * bootstrap they can be nonzero to specify hand-assigned OIDs
 */
bool
create_toast_table(Relation rel, Oid toastOid, Oid toastIndexOid, Datum reloptions)
{
	Oid			relOid = RelationGetRelid(rel);

	TupleDesc	tupdesc;
	bool		shared_relation;
	bool		mapped_relation;
	Relation	toast_rel;

	Oid			toast_relid = toastOid;
	Oid			toast_typid = InvalidOid;
	Oid			namespaceid = 0;
	char		toast_relname[NAMEDATALEN];
	char		toast_idxname[NAMEDATALEN];




#ifndef FOUNDER_XDB_SE	
	ObjectAddress baseobject,
				toastobject;
#endif //FOUNDER_XDB_SE				

	/*
	 * Toast table is shared if and only if its parent is.
	 *
	 * We cannot allow toasting a shared relation after initdb (because
	 * there's no way to mark it toasted in other databases' pg_class).
	 */
	shared_relation = rel->rd_rel->relisshared;
	if (shared_relation && !IsBootstrapProcessingMode())
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("shared tables cannot be toasted after initdb")));

	/* It's mapped if and only if its parent is, too */
	mapped_relation = RelationIsMapped(rel);

#ifndef FOUNDER_XDB_SE
	/*
	 * Is it already toasted?
	 */
	if (rel->rd_rel->reltoastrelid != InvalidOid)
		return false;
#endif //FOUNDER_XDB_SE

	/*
	 * Check to see whether the table actually needs a TOAST table.
	 *
	 * If an update-in-place toast relfilenode is specified, force toast file
	 * creation even if it seems not to need one.
	 */
	//if (!needs_toast_table(rel) &&
	//	(!IsBinaryUpgrade ||
	//	 !OidIsValid(binary_upgrade_next_toast_pg_class_oid)))
	//	return false;

	/*
	 * Create the toast table and its index
	 */
	snprintf(toast_relname, sizeof(toast_relname),
			 "pg_toast_%u", relOid);
	snprintf(toast_idxname, sizeof(toast_idxname),
			 "pg_toast_%u_index", relOid);
	tupdesc = get_toast_tuple_desc();

	/*
	 * Ensure that the toast table doesn't itself get toasted, or we'll be
	 * toast :-(.  This is essential for chunk_data because type bytea is
	 * toastable; hit the other two just to be sure.
	 */
	toast_relid = fxdb_heap_create(toast_relid, 
		0, 
		rel->rd_node.spcNode
		,rel->rd_node.dbNode
		,RELKIND_TOASTVALUE, 
		rel->rd_rel->relpersistence);
	rel->temp_toast_id = toast_relid;
	toast_rel = heap_open(toast_relid,NoLock);
	//toast_rel = heap_create(toast_relname
	//	, 1
	//	, rel->rd_node.spcNode
	//	, toast_relid
	//	, tupdesc
	//	, RELKIND_TOASTVALUE
	//	, RELPERSISTENCE_PERMANENT
	//	, false, false, false, NULL);

									   
	Assert(toast_relid != InvalidOid);

	/* make the toast relation visible, else heap_open will fail */
	CommandCounterIncrement();

	/* ShareLock is not really needed here, but take it anyway */
	//toast_rel = heap_open(toast_relid,rel->rd_node.spcNode, ShareLock);

	/*
	 * Create unique index on chunk_id, chunk_seq.
	 *
	 * NOTE: the normal TOAST access routines could actually function with a
	 * single-column index on chunk_id only. However, the slice access
	 * routines use both columns for faster access to an individual chunk. In
	 * addition, we want it to be unique as a check against the possibility of
	 * duplicate TOAST chunk OIDs. The index might also be a little more
	 * efficient this way, since btree isn't all that happy with large numbers
	 * of equal keys.
	 */

	fxdb_index_create(toastIndexOid, 
		toast_rel, 
		BTREE_TYPE, 
		get_toast_index_colinfo());

	heap_close(toast_rel, NoLock);
	/*
	 * Make changes visible
	 */
	CommandCounterIncrement();

#ifdef FOUNDER_XDB_SE
	rel->rd_rel->reltoastrelid = toastOid;
	rel->rd_rel->reltoastidxid = toastIndexOid;
#endif //FOUNDER_XDB_SE

	return true;
}
#endif
#ifndef FOUNDER_XDB_SE

/*
 * AlterTableCreateToastTable
 *		If the table needs a toast table, and doesn't already have one,
 *		then create a toast table for it.
 *
 * reloptions for the toast table can be passed, too.  Pass (Datum) 0
 * for default reloptions.
 *
 * We expect the caller to have verified that the relation is a table and have
 * already done any necessary permission checks.  Callers expect this function
 * to end with CommandCounterIncrement if it makes any changes.
 */
void
AlterTableCreateToastTable(Oid relOid, Datum reloptions)
{
	Relation	rel;

	/*
	 * Grab an exclusive lock on the target table, since we'll update its
	 * pg_class tuple. This is redundant for all present uses, since caller
	 * will have such a lock already.  But the lock is needed to ensure that
	 * concurrent readers of the pg_class tuple won't have visibility issues,
	 * so let's be safe.
	 */
	rel = heap_open(relOid, AccessExclusiveLock);

	/* create_toast_table does all the work */
	(void) create_toast_table(rel, InvalidOid, InvalidOid, reloptions);

	heap_close(rel, NoLock);
}

/*
 * Create a toast table during bootstrap
 *
 * Here we need to prespecify the OIDs of the toast table and its index
 */
void
BootstrapToastTable(char *relName, Oid toastOid, Oid toastIndexOid)
{
	Relation	rel;

	rel = heap_openrv(makeRangeVar(NULL, relName, -1), AccessExclusiveLock);

	/* Note: during bootstrap may see uncataloged relation */
	if (rel->rel_kind != RELKIND_RELATION &&
		rel->rel_kind != RELKIND_UNCATALOGED)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("\"%s\" is not a table",
						relName)));

	/* create_toast_table does all the work */
	if (!create_toast_table(rel, toastOid, toastIndexOid, (Datum) 0))
		elog(ERROR, "\"%s\" does not require a toast table",
			 relName);

	heap_close(rel, NoLock);
}


/*
 * create_toast_table --- internal workhorse
 *
 * rel is already opened and locked
 * toastOid and toastIndexOid are normally InvalidOid, but during
 * bootstrap they can be nonzero to specify hand-assigned OIDs
 */
static bool
create_toast_table(Relation rel, Oid toastOid, Oid toastIndexOid, Datum reloptions)
{
	Oid			relOid = RelationGetRelid(rel);
	HeapTuple	reltup;
	TupleDesc	tupdesc;
	bool		shared_relation;
	bool		mapped_relation;
	Relation	toast_rel;
	Relation	class_rel;
	Oid			toast_relid = 0;
	Oid			toast_typid = InvalidOid;
	Oid			namespaceid = 0;
	char		toast_relname[NAMEDATALEN];
	char		toast_idxname[NAMEDATALEN];
	IndexInfo  *indexInfo;
	Oid			collationObjectId[2];
	Oid			classObjectId[2];
	int16		coloptions[2];
#ifndef FOUNDER_XDB_SE	
	ObjectAddress baseobject,
				toastobject;
#endif //FOUNDER_XDB_SE				

	/*
	 * Toast table is shared if and only if its parent is.
	 *
	 * We cannot allow toasting a shared relation after initdb (because
	 * there's no way to mark it toasted in other databases' pg_class).
	 */
	shared_relation = rel->rd_rel->relisshared;
	if (shared_relation && !IsBootstrapProcessingMode())
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("shared tables cannot be toasted after initdb")));

	/* It's mapped if and only if its parent is, too */
	mapped_relation = RelationIsMapped(rel);

	/*
	 * Is it already toasted?
	 */
	if (rel->rd_rel->reltoastrelid != InvalidOid)
		return false;

	/*
	 * Check to see whether the table actually needs a TOAST table.
	 *
	 * If an update-in-place toast relfilenode is specified, force toast file
	 * creation even if it seems not to need one.
	 */
	if (!needs_toast_table(rel) &&
		(!IsBinaryUpgrade ||
		 !OidIsValid(binary_upgrade_next_toast_pg_class_oid)))
		return false;

	/*
	 * Create the toast table and its index
	 */
	snprintf(toast_relname, sizeof(toast_relname),
			 "pg_toast_%u", relOid);
	snprintf(toast_idxname, sizeof(toast_idxname),
			 "pg_toast_%u_index", relOid);

	/* this is pretty painful...  need a tuple descriptor */
	tupdesc = CreateTemplateTupleDesc(3, false);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1,
					   "chunk_id",
					   OIDOID,
					   -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2,
					   "chunk_seq",
					   INT4OID,
					   -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 3,
					   "chunk_data",
					   BYTEAOID,
					   -1, 0);

	/*
	 * Ensure that the toast table doesn't itself get toasted, or we'll be
	 * toast :-(.  This is essential for chunk_data because type bytea is
	 * toastable; hit the other two just to be sure.
	 */
	tupdesc->attrs[0]->attstorage = 'p';
	tupdesc->attrs[1]->attstorage = 'p';
	tupdesc->attrs[2]->attstorage = 'p';

	/*
	 * Toast tables for regular relations go in pg_toast; those for temp
	 * relations go into the per-backend temp-toast-table namespace.
	 */
#ifndef FOUNDER_XDB_SE
	if (RelationUsesTempNamespace(rel))
		namespaceid = GetTempToastNamespace();
	else
		namespaceid = PG_TOAST_NAMESPACE;
#endif //FOUNDER_XDB_SE

	/* Use binary-upgrade override for pg_type.oid, if supplied. */
	if (IsBinaryUpgrade && OidIsValid(binary_upgrade_next_toast_pg_type_oid))
	{
		toast_typid = binary_upgrade_next_toast_pg_type_oid;
		binary_upgrade_next_toast_pg_type_oid = InvalidOid;
	}
#ifndef FOUNDER_XDB_SE
	toast_relid = heap_create_with_catalog(toast_relname,
										   namespaceid,
										   rel->rd_rel->reltablespace,
										   toastOid,
										   toast_typid,
										   InvalidOid,
										   rel->rd_rel->relowner,
										   tupdesc,
										   NIL,
										   RELKIND_TOASTVALUE,
										   rel->rd_rel->relpersistence,
										   shared_relation,
										   mapped_relation,
										   true,
										   0,
										   ONCOMMIT_NOOP,
										   reloptions,
										   false,
										   true);
#endif //FOUNDER_XDB_SE										   
	Assert(toast_relid != InvalidOid);

	/* make the toast relation visible, else heap_open will fail */
	CommandCounterIncrement();

	/* ShareLock is not really needed here, but take it anyway */
	toast_rel = heap_open(toast_relid, ShareLock);

	/*
	 * Create unique index on chunk_id, chunk_seq.
	 *
	 * NOTE: the normal TOAST access routines could actually function with a
	 * single-column index on chunk_id only. However, the slice access
	 * routines use both columns for faster access to an individual chunk. In
	 * addition, we want it to be unique as a check against the possibility of
	 * duplicate TOAST chunk OIDs. The index might also be a little more
	 * efficient this way, since btree isn't all that happy with large numbers
	 * of equal keys.
	 */

	indexInfo = makeNode(IndexInfo);
	indexInfo->ii_NumIndexAttrs = 2;
	indexInfo->ii_KeyAttrNumbers[0] = 1;
	indexInfo->ii_KeyAttrNumbers[1] = 2;
	indexInfo->ii_Expressions = NIL;
	indexInfo->ii_ExpressionsState = NIL;
	indexInfo->ii_Predicate = NIL;
	indexInfo->ii_PredicateState = NIL;
	indexInfo->ii_ExclusionOps = NULL;
	indexInfo->ii_ExclusionProcs = NULL;
	indexInfo->ii_ExclusionStrats = NULL;
	indexInfo->ii_Unique = true;
	indexInfo->ii_ReadyForInserts = true;
	indexInfo->ii_Concurrent = false;
	indexInfo->ii_BrokenHotChain = false;

	collationObjectId[0] = InvalidOid;
	collationObjectId[1] = InvalidOid;

	classObjectId[0] = OID_BTREE_OPS_OID;
	classObjectId[1] = INT4_BTREE_OPS_OID;

	coloptions[0] = 0;
	coloptions[1] = 0;

	index_create(toast_rel, toast_idxname, toastIndexOid,
				 indexInfo,
				 list_make2("chunk_id", "chunk_seq"),
				 BTREE_AM_OID,
				 rel->rd_rel->reltablespace,
				 collationObjectId, classObjectId, coloptions, (Datum) 0,
				 true, false, false, false,
				 true, false, false);

	heap_close(toast_rel, NoLock);

	/*
	 * Store the toast table's OID in the parent relation's pg_class row
	 */
	class_rel = heap_open(RelationRelationId, RowExclusiveLock);

	reltup = SearchSysCacheCopy1(RELOID, ObjectIdGetDatum(relOid));
	if (!HeapTupleIsValid(reltup))
		elog(ERROR, "cache lookup failed for relation %u", relOid);

	((Form_pg_class) GETSTRUCT(reltup))->reltoastrelid = toast_relid;

	if (!IsBootstrapProcessingMode())
	{
		/* normal case, use a transactional update */
		simple_heap_update(class_rel, &reltup->t_self, reltup);

		/* Keep catalog indexes current */
		CatalogUpdateIndexes(class_rel, reltup);
	}
	else
	{
		/* While bootstrapping, we cannot UPDATE, so overwrite in-place */
		heap_inplace_update(class_rel, reltup);
	}

	heap_freetuple(reltup);

	heap_close(class_rel, RowExclusiveLock);

	/*
	 * Register dependency from the toast table to the master, so that the
	 * toast table will be deleted if the master is.  Skip this in bootstrap
	 * mode.
	 */
	if (!IsBootstrapProcessingMode())
	{
#ifndef FOUNDER_XDB_SE
		baseobject.classId = RelationRelationId;
		baseobject.objectId = relOid;
		baseobject.objectSubId = 0;
		toastobject.classId = RelationRelationId;
		toastobject.objectId = toast_relid;
		toastobject.objectSubId = 0;
		recordDependencyOn(&toastobject, &baseobject, DEPENDENCY_INTERNAL);
#endif //FOUNDER_XDB_SE	
		
	}

	/*
	 * Make changes visible
	 */
	CommandCounterIncrement();

	return true;
}
#endif //FOUNDER_XDB_SE
/*
 * Check to see whether the table needs a TOAST table.	It does only if
 * (1) there are any toastable attributes, and (2) the maximum length
 * of a tuple could exceed TOAST_TUPLE_THRESHOLD.  (We don't want to
 * create a toast table for something like "f1 varchar(20)".)
 */
static bool
needs_toast_table(Relation rel)
{
	int32		data_length = 0;
	bool		maxlength_unknown = false;
	bool		has_toastable_attrs = false;
	TupleDesc	tupdesc;
	Form_pg_attribute *att;
	int32		tuple_length;
	int			i;

	tupdesc = rel->rd_att;
	att = tupdesc->attrs;

	for (i = 0; i < tupdesc->natts; i++)
	{
		if (att[i]->attisdropped)
			continue;
		data_length = att_align_nominal(data_length, att[i]->attalign);
		if (att[i]->attlen > 0)
		{
			/* Fixed-length types are never toastable */
			data_length += att[i]->attlen;
		}
		else
		{
			int32		maxlen = type_maximum_size(att[i]->atttypid,
												   att[i]->atttypmod);

			if (maxlen < 0)
				maxlength_unknown = true;
			else
				data_length += maxlen;
			if (att[i]->attstorage != 'p')
				has_toastable_attrs = true;
		}
	}
	if (!has_toastable_attrs)
		return false;			/* nothing to toast? */
	if (maxlength_unknown)
		return true;			/* any unlimited-length attrs? */
	tuple_length = MAXALIGN(offsetof(HeapTupleHeaderData, t_bits) +
							BITMAPLEN(tupdesc->natts)) +
		MAXALIGN(data_length);
	return (tuple_length > TOAST_TUPLE_THRESHOLD);
}
