/*-------------------------------------------------------------------------
 *
 * large_object.h
 *	  Declarations for PostgreSQL large objects.  POSTGRES 4.2 supported
 *	  zillions of large objects (internal, external, jaquith, inversion).
 *	  Now we only support inversion.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/storage/large_object.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef LARGE_OBJECT_H
#define LARGE_OBJECT_H

#include "utils/snapshot.h"


/*----------
 * Data about a currently-open large object.
 *
 * id is the logical OID of the large object
 * snapshot is the snapshot to use for read/write operations
 * subid is the subtransaction that opened the desc (or currently owns it)
 * offset is the current seek offset within the LO
 * flags contains some flag bits
 *
 * NOTE: before 7.1, we also had to store references to the separate table
 * and index of a specific large object.  Now they all live in pg_largeobject
 * and are accessed via a common relation descriptor.
 *----------
 */
typedef struct LargeObjectDesc
{
	Oid			id;				/* LO's identifier */
	Snapshot	snapshot;		/* snapshot to use */
	SubTransactionId subid;		/* owning subtransaction ID */
	uint64		offset;			/* current seek pointer */
	int			flags;			/* locking info, etc */

/* flag bits: */
#define IFS_RDLOCK		(1 << 0)
#define IFS_WRLOCK		(1 << 1)

} LargeObjectDesc;

#ifdef FOUNDER_XDB_SE

#define MIN_LARGE_OBJ_ID 1

#pragma pack(4)

typedef struct Form_meta_large_table_rel
{
	Oid relid;
	Oid index_loid;
	Oid index_loname;
	Oid relid2;
	Oid relid2_index;
	Oid relid2_tbcindex;
	Oid dbid;
} Form_meta_large_table_rel;

typedef struct Form_meta_large_data
{
	Oid lo_id;
	NameData lo_name;
	Oid lo_tblspace;
	Oid rel_id;
	Oid index_id;
	uint32 extra_data_size;
} Form_meta_large_data;

typedef struct Form_class_large_heap
{
	Oid lo_heap_id;
	Oid lo_index_id;
	Oid tblspace;
} Form_class_large_heap;

#pragma pack()

typedef Form_meta_large_data* Form_meta_large;
typedef Form_class_large_heap* Form_large_heap;

#ifdef FOUNDER_XDB_SE

enum MetaLargeObjRelColNumber
{
	META_LARGE_OBJ_REL_RELID = 1,
	META_LARGE_OBJ_REL_LOID,
	META_LARGE_OBJ_REL_NAMEID,
	META_LARGE_OBJ_REL2_RELID,
	META_LARGE_OBJ_REL2_INDEXID,
	META_LARGE_OBJ_REL2_TBCID,
	META_LARGE_OBJ_REL_DBID,
	META_LARGE_OBJ_REL_COL_NUM = META_LARGE_OBJ_REL_DBID
};

enum MetaLargeObjColNumber
{
	META_LARGE_OBJ_ID = 1,
	META_LARGE_OBJ_NAME,
	META_LARGE_OBJ_TBLSPACE,
	META_LARGE_OBJ_REL_ID,
	META_LARGE_OBJ_INDEX_ID,
	META_LARGE_OBJ_COL_NUM = META_LARGE_OBJ_INDEX_ID
};

enum MetaLargeObjDataColNumber
{
	LARGE_OBJ_LO_ID = 1,
	LARGE_OBJ_PAGENO,
	LARGE_OBJ_DATA,
	LARGE_OBJ_DATA_COL_NUM = LARGE_OBJ_DATA
};

enum MetaLargeObjHeapColNumber
{
	LARGE_OBJ_HEAP_ID = 1,
	LARGE_OBJ_INDEX_ID = 2,
	LARGE_HEAP_INDEX_TBLSPACE = 3,
	LARGE_HEAP_INDEX_DBID = 4,
	LARGE_OBJ_HEAP_COL_NUM = LARGE_HEAP_INDEX_DBID
};

#endif //FOUNDER_XDB_SE

#endif //FOUNDER_XDB_SE

/*
 * Each "page" (tuple) of a large object can hold this much data
 *
 * We could set this as high as BLCKSZ less some overhead, but it seems
 * better to make it a smaller value, so that not as much space is used
 * up when a page-tuple is updated.  Note that the value is deliberately
 * chosen large enough to trigger the tuple toaster, so that we will
 * attempt to compress page tuples in-line.  (But they won't be moved off
 * unless the user creates a toast-table for pg_largeobject...)
 *
 * Also, it seems to be a smart move to make the page size be a power of 2,
 * since clients will often be written to send data in power-of-2 blocks.
 * This avoids unnecessary tuple updates caused by partial-page writes.
 */
#define LOBLKSIZE		(BLCKSZ / 4)


/*
 * Function definitions...
 */

/* inversion stuff in inv_api.c */
extern void close_lo_relation(bool isCommit);

#ifndef FOUNDER_XDB_SE
extern Oid	inv_create(Oid lobjId);
#else
extern Oid	inv_create(char* name, 
											 int extraDataLen, 
											 void *extraData);
#endif

#ifdef FOUNDER_XDB_SE
extern LargeObjectDesc *fxdb_inv_name_open(char *name, int flags, MemoryContext mcxt);
extern int	fxdb_inv_name_drop(char *name);
extern bool lo_exit(Oid lo_id, Form_meta_large *formClass = NULL);
extern Oid name_get_loid(char *name);
extern char* loid_get_name(Oid lo_id);
extern Form_meta_large fxdb_inv_update_lo(Form_meta_large formClass, 
																					void *extraData, size_t extraDataLen);
extern void InitLOMemCxt();
extern void CreateNewLORel(Oid tblspace, Oid dbid = MyDatabaseId);
extern void CreateMetaLORel(Oid tblspace, Oid dbid);
extern void DatabaseCreateLORel(Oid dbid);
extern void TablespaceCreateLORel(Oid tbcid);
extern void CreateDefaultLORel();
extern void DropLORel(Oid tblspace);
extern void DestoryLORelList();
extern void SetOpenLORelation();
extern void RebuildLORelList(bool ignoreError);
extern void InitLORelList();
extern bool IsLOEmpty(Oid relid);
extern List *ListLo(Oid tblspace, uint32 scan_num, 
										Oid *idx_loid, bool *has_next);
extern void InsertMetaLORelInfo(Oid relid, Oid index_loid, Oid index_loname,
																Oid relid2, Oid relid2_index, Oid relid2_tbcindex,
																Oid dbid);
extern Form_meta_large_table_rel *GetDatabaseLOMetaTable(Oid dbid = MyDatabaseId, bool isRemove = false);
#endif //FOUNDER_XDB_SE

extern LargeObjectDesc *inv_open(Oid lobjId, int flags, MemoryContext mcxt);
extern void inv_close(LargeObjectDesc *obj_desc);
extern int	inv_drop(Oid lobjId);
extern uint64	inv_seek(LargeObjectDesc *obj_desc, int64 offset, int whence);
extern uint64	inv_tell(LargeObjectDesc *obj_desc);
extern int	inv_read(LargeObjectDesc *obj_desc, char *buf, int nbytes);
extern int	inv_write(LargeObjectDesc *obj_desc, const char *buf, int nbytes);
extern void inv_truncate(LargeObjectDesc *obj_desc, int64 len);

#ifdef FOUNDER_XDB_SE
extern void Fxdb_AtEOXact_LargeObject(bool isCommit);
extern void Fxdb_AtEndStatement_LargeObject();
extern void InitLOInfo(bool isInXactBlock);
#endif //FOUNDER_XDB_SE

#endif   /* LARGE_OBJECT_H */
