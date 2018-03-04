#ifndef XDB_CATALOG_H
#define XDB_CATALOG_H

#include "utils/rel.h"
#include "access/xdb_common.h"
#include "nodes/execnodes.h"
#include "utils/memutils.h"
#include "storage/bufmgr.h"
#include "utils/tqual.h"
#include "storage/large_object.h"
#include "nodes/primnodes.h"

extern THREAD_LOCAL Oid	MyDatabaseTableSpace;
extern FormData_pg_am btree_am;
extern FormData_pg_am hash_am;

enum SysTblId
{
 MinMetaId = 254,
 MetaTableId = MinMetaId,
 MetaTableColFirstIndex,
 MetaTableColThirdIndex,
 DatabaseNameIdxId,
 DatabaseUniqueId,
 DatabaseRelationId,
 SequenceRelId,
 SequenceIdIdxId,
 SequenceNameIdxId,
 TableSpaceRelationId,
 TableSpaceNameIdxId,
 TableSpaceOidIdxId,
 LargeObjMetaTableRelId,
 LargeObjMetaTableIndexDBID,
 MetaLargeObjId,
 MetaLargeObjIndexColLOID,
 MetaLargeObjIndexColLONAME,
 LargeObjHeapId,
 LargeObjHeapIndex,
 LargeObjHeapTbcIndex,
 LargeObjDataId,
 LargeObjDataIndex,
 RDF_Meta_Models,
 RDF_Meta_Models_Index_ID,
 RDF_Meta_RBL,
 RDF_Meta_RBL_Index_ID,
 MaxMetaId = RDF_Meta_RBL_Index_ID,
 RDF_Meta_Statements_COLID = MaxMetaId + 1,
 RDF_Meta_Statements_Index_PO_COLID,
 RDF_Meta_Statements_Index_OS_COLID,
 RDF_Meta_Statements_Index_SPO_COLID,
 RDF_Meta_Statements_Index_INF_ID_COLID
};

#define MIN_USER_OID 1024

enum MetaTableColNumber
{
	META_TABLE_COL_OID = 1,
	META_TABLE_COL_TYPE,
	META_TABLE_COL_POID,
	META_TABLE_COL_FILENODE,
	META_TABLE_COL_OWNER,
	META_TABLE_COL_DB_ID,
	META_TABLE_COL_TBL_ID,
	META_TABLE_COL_FROZENXID,
	META_TABLE_COL_COLINFO_ID,
	META_TABLE_COL_TIME,
	META_TABLE_COL_TUPLES,
	META_TABLE_COL_RELPERSISTENCE,
	META_TABLE_COL_NUM = META_TABLE_COL_FILENODE
};

/*
 * An ObjectAddress represents a database object of any type.
 */
typedef struct ObjectAddress
{
	Oid			classId;		/* Class Id from pg_class */
	Oid			objectId;		/* OID of the object */
	int32		objectSubId;	/* Subitem within object (eg column), or 0 */
} ObjectAddress;

#pragma pack(4)
typedef struct Form_meta_table_data
{
	Oid rel_id;									/* Relation id */
	unsigned int type;					/* Relation type */
	Oid rel_pid;								/* Parent relation id */
	Oid rel_filenode;						/* Physical file name */
	int owner;
	Oid database_id;						/* Database id */
	Oid tablespace_id;					/* Table space id */
	TransactionId relfrozenxid;	/* all Xids < this are frozen in this rel */
	unsigned int colinfo_id;		/* Col info id */
	pg_time_t time;							/* Create time */
	double reltuples;						/* How many valid tuples in this relation */
	char relpersistence;				/* see RELPERSISTENCE_xxx constants below */
	char reserved[16];
	text userdata;            /* user data */
} Form_meta_table_data;

typedef struct Form_large_data
{
	Oid lo_id;
	int64 pageno;
	struct
	{
		bytea hdr;
		char data[LOBLKSIZE];	/* make struct big enough */
		int32 align_it;			/* ensure struct is aligned well enough */
	} workbuf;
} Form_large_data;

#pragma pack()

typedef Form_meta_table_data* Form_meta_table;
typedef Form_large_data* Form_large;

enum OnCommitAction;

#define OID_BTREE_OPS_OID 1981
#define INT4_BTREE_OPS_OID 1978

/* Get an EState's per-output-tuple exprcontext, making it if first use */
#define GetPerTupleExprContext(estate) \
    ((estate)->es_per_tuple_exprcontext ? \
    (estate)->es_per_tuple_exprcontext : \
    MakePerTupleExprContext(estate))
extern void fdxdb_heap_drop(Oid relid,Oid dbid = MyDatabaseId);
extern Oid fxdb_heap_create(Oid relid,
							 Oid colid,
							 Oid reltablespace = MyDatabaseTableSpace,
							 Oid dbid = MyDatabaseId,							 
							 char relkind = RELKIND_RELATION,
							 char relpersistence = RELPERSISTENCE_PERMANENT,
							 OnCommitAction action = ONCOMMIT_NOOP);
extern Oid fxdb_index_create(Oid relid,Relation heapRelation, IndexType indextype,Oid coliId, void* userData = 0,filter_func_t filterfunc = 0, uint32 userdataLength = 0, bool skipInsert = false);

#ifndef FOUNDER_XDB_SE
extern EState *CreateExecutorState(void);
extern TupleTableSlot *MakeSingleTupleTableSlot(TupleDesc tupdesc);
extern TupleTableSlot *ExecStoreTuple(HeapTuple tuple,TupleTableSlot *slot,Buffer buffer,bool shouldFree);
extern void ExecSetSlotDescriptor(TupleTableSlot *slot,TupleDesc tupdesc);
extern ExprContext *MakePerTupleExprContext(EState *estate);
void FreeExprContext(ExprContext *econtext, bool isCommit);
void FreeExecutorState(EState *estate);
#endif


#ifndef FOUNDER_XDB_SE
void ExecDropSingleTupleTableSlot(TupleTableSlot *slot);
#endif

//unsigned int fxdb_get_tblspace(Oid object_id);
//unsigned int fxdb_get_type(Oid object_id);
unsigned int fxdb_get_heapid(Oid index_id);
bool  fxdb_insert_meta(Oid object_id, int  type, Oid  parent_table_id,  pg_time_t time,  int owner,  Oid  database_id,
                       Oid  tablespace_id, unsigned int colinfo_id, char relpersistence = RELPERSISTENCE_PERMANENT,
                       void *userdata = NULL, uint32 userdataLength = 0);

List* fxdb_get_all_table_in_db(Oid dbid);

bool  fxdb_delete_meta(Oid object_id);
bool fxdb_get_heap_info(Oid  m_index_entry_id, ColinfoData& colinfo);
//bool fxdb_get_index_info(Oid  index_id, IndinfoData& indinfo);

//// doesn't delete 
unsigned int fxdb_get_max_id();
bool fxdb_get_heapInfo(Oid  m_index_entry_id
					   , MtInfo mt_info);
bool fxdb_get_indexInfo(Oid  table_id, IndinfoData& indinfo);
bool fxdb_get_mete_info(Oid object_id,MtInfo mt);
// true is metatable, false is other
bool isMetaTable(Oid relid);

bool IsSystemTable(Oid relid);

HeapTuple fxdb_search_meta_table_copy(Oid relid, Snapshot snapshot = SnapshotNow);
void *fxdb_get_metastruct_ptr(HeapTuple tup, bool *shuold_free);
Form_meta_table fxdb_get_meta_table_struct(HeapTuple tuple);

void clean_rel(Oid begin, size_t size, Oid dbid, Oid backendid);
void DoUnlinkRel(Oid relid, Oid dbid, Oid tblspace, Oid backendid);
int relid_compare(const char* a, size_t len1, const char* b, size_t len2);
#endif //XDB_CATALOG_H
