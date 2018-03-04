#include <time.h>
#include "postgres.h"

#include "commands/tablecmds.h"
#include "catalog/catalog.h"
#include "catalog/heap.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "catalog/index.h"
#include "nodes/nodes.h"
#include "access/xact.h"
#include "catalog/storage.h"
#include "catalog/metaData.h"
#include "utils/inval.h"
#include "storage/s_lock.h"
#include "catalog/toasting.h"
#include "catalog/pg_database.h"
#include "commands/tablespace.h"
#include "storage/spin.h"
#include "access/tuptoaster.h"
#include "storage/smgr.h"
#include "storage/proc.h"
#include "storage/standby.h"
#include "utils/snapmgr.h"
#include "access/transam.h"
#include "storage/large_object.h"
#ifndef FOUNDER_XDB_SE
static void ShutdownExprContext(ExprContext *econtext, bool isCommit);
#endif
FormData_pg_am btree_am=
{{"btree"},5,1,1,0,1,1,1,1,1,0,1,1,0,331,333,330,636,334,335,336,337,338, 339, 328,332,972,1268,2785};

FormData_pg_am hash_am=
{{"hash"},1,1,1,0,1,1,1,1,1,0,1,1,23,441,443,440,637,444,445,446,447,448, 0, 327,442,425,438,2786};

RelationAmInfo btree_aminfo ;
RelationAmInfo hash_aminfo ;
extern TupleDesc single_attibute_tupDesc;
extern FormData_pg_attribute text_attribute;
extern Colinfo getColInfo(Oid colid);
extern void CreateRDFTable();
extern void InitRDFMetaTableColinfo();

int relid_compare(const char* a, size_t len1, const char* b, size_t len2)
{
	Oid cmp_1 = *(Oid*)a;
	Oid cmp_2 = *(Oid*)b;
	return (cmp_1 == cmp_2) ? 
						0 : (cmp_1 > cmp_2) ?
						1 : -1;
}

int char_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
	size_t i = 0;
	while(i < len1 && i < len2)
	{
		if(str1[i] < str2[i])
			return -1;
		else if(str1[i] > str2[i])
			return 1;
		else i++;

	}
	if(len1 == len2)
		return 0;
	if(len1 > len2)
		return 1;
	return -1;
}


int int4_compare(const char* a, size_t len1, const char* b, size_t len2)
{
	int4 cmp_1 = *(int4*)a;
	int4 cmp_2 = *(int4*)b;

	return cmp_1 - cmp_2;
}

int int64_compare(const char* a, size_t len1, const char* b, size_t len2)
{
	int64 cmp_1 = *(int64*)a;
	int64 cmp_2 = *(int64*)b;

	return (int)(cmp_1 - cmp_2);
}

int int_compare(const char* a, size_t len1, const char* b, size_t len2)
{
	int cmp_1 = *(int*)a;
	int cmp_2 = *(int*)b;

	return cmp_1 - cmp_2;
}

int name_data_compare(const char* a, size_t len1, const char* b, size_t len2)
{
	if(len1 == len2) {
		return strncmp(a, b, len1);
	}
	
	return len1 > len2 ? 1 : -1;
}

static const int OID_SIZE = sizeof(Oid);

static 
void  large_meta_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	Assert(col <= META_LARGE_OBJ_COL_NUM);
	/* 
	 * 大对象元数据表有六列 : 
	 * 第一列标识唯一的一个大对象；
	 * 第二列为一个唯一标识该大对象的字符串；
	 * 第三列为该大对象所在的表空间；
	 * 第四列为该大对象所在的表；
	 * 第五列为大对象所在表的索引
	 * 第六列是该大对象的描述信息
	 */
	memset(&rd, 0, sizeof(rd));
	if (col == 1) {
		rd.start = 0;
		rd.len = OID_SIZE;
	} else {
		large_meta_split(rd, str, col - 1);
		rd.start = rd.start + rd.len;
		if(col == 2) {
			rd.len = sizeof(NameData);
		} else {
			rd.len = OID_SIZE;
		}
	}

}

static 
void large_data_index_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	Assert(col <= LARGE_OBJ_DATA_COL_NUM - 1);
	memset(&rd, 0, sizeof(rd));
	Form_large_data formLargeClass;
	if(col == 1) {
		rd.start = 0;
		rd.len = sizeof(formLargeClass.lo_id);
	} else if(col == 2) {
		large_data_index_split(rd, str, col - 1, len);
		rd.start = rd.start + rd.len;
		rd.len = sizeof(formLargeClass.pageno);
	}
}

static
void large_data_rel_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	Assert(col <= LARGE_OBJ_DATA_COL_NUM);

	/* 
	 * 大对象数据内容存储的表有3列
	 *  第一列 : 标识一个大对象的Oid数据类型
	 *  第二列 : 该大对象当前数据块的块号
	 *  第三列 : 当前块数据内容
	 */
	memset(&rd, 0, sizeof(rd));
	Form_large_data formLargeClass;
	if(col == 1)
	{
		rd.start = 0;
		rd.len = sizeof(formLargeClass.lo_id);
	} else {
		large_data_rel_split(rd, str, col - 1);
		rd.start = rd.start + rd.len;
		if(col == 2) {
			rd.len = sizeof(formLargeClass.pageno);
		} else if(col == 3) {
			rd.len = sizeof(formLargeClass.workbuf);
		} else {
			Assert(0);
		}
	}
}

void meta_item_split(RangeData& rangeData, const char *str, int col, size_t len = 0)
{
	Assert(col <= META_TABLE_COL_RELPERSISTENCE);

	int i = col - 1;
    memset(&rangeData, 0, sizeof(rangeData));
	if(col <= META_TABLE_COL_TIME)
	{
		rangeData.start = OID_SIZE * i;
	} else {
		meta_item_split(rangeData, str, META_TABLE_COL_TIME, len);
		rangeData.start += (col - META_TABLE_COL_TIME) * (OID_SIZE * 2);
	}

	if(col < META_TABLE_COL_TIME)
	{
		rangeData.len = OID_SIZE;
	} else if (col >= META_TABLE_COL_TIME && col < META_TABLE_COL_RELPERSISTENCE){
		rangeData.len = OID_SIZE * 2;
	} else 
	{
		rangeData.len = 1;
	}

}

static
void coln_oid_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	memset(&rd, 0, sizeof(rd));
	rd.start = (col - 1) * OID_SIZE;
	rd.len = OID_SIZE;
}

static 
void col1_oid_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	memset(&rd, 0, sizeof(rd));
	rd.start = 0;
	rd.len = OID_SIZE;
}

static 
void col1_name_data_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	memset(&rd, 0, sizeof(rd));	
	rd.start = 0;
	rd.len = sizeof(NameData);
}

static 
void init_large_meta_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = META_LARGE_OBJ_COL_NUM;

	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;

	colinfo->split_function = large_meta_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(MetaLargeObjId, colinfo);
}

static 
void init_large_meta_index_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	/* 大对象元数据表第一列的colinfo */
	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = META_LARGE_OBJ_ID;

	colinfo->rd_comfunction = (CompareCallback*)palloc(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = relid_compare;

	colinfo->split_function = coln_oid_split;
	setColInfo(MetaLargeObjIndexColLOID, colinfo);

	/* 大对象元数据表第二列colinfo */
	colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = META_LARGE_OBJ_NAME;

	colinfo->rd_comfunction = (CompareCallback*)palloc(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = name_data_compare;

	colinfo->split_function = col1_name_data_split;
	setColInfo(MetaLargeObjIndexColLONAME, colinfo);

	MemoryContextSwitchTo(ocxt);
}

static
void init_large_heap_rel_colinfo()
{
	/*
	 * 存储所有表空间下的大对象数据表,该表有三列
	 * 第一列:表ID
	 * 第二列:索引ID
	 * 第三列:表空间
	 */
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = LARGE_OBJ_HEAP_COL_NUM;

	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;

	colinfo->split_function = coln_oid_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(LargeObjHeapId, colinfo);
}

static
void init_large_heap_index_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;

	colinfo->col_number = (size_t*)palloc(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = LARGE_OBJ_HEAP_ID;

	colinfo->rd_comfunction = (CompareCallback*)palloc(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = relid_compare;

	colinfo->split_function = col1_oid_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(LargeObjHeapIndex, colinfo);

	ocxt = MemoryContextSwitchTo(TopMemoryContext);

	colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;

	colinfo->col_number = (size_t*)palloc(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = LARGE_HEAP_INDEX_TBLSPACE;

	colinfo->rd_comfunction = (CompareCallback*)palloc(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = relid_compare;

	colinfo->split_function = col1_oid_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(LargeObjHeapTbcIndex, colinfo);
}

static
void init_large_data_rel_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = LARGE_OBJ_DATA_COL_NUM;

	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;

	colinfo->split_function = large_data_rel_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(LargeObjDataId, colinfo);
}

static 
void init_large_data_index_colinfo()
{
	/*
	 * 大对象数据表上的索引是一个两列索引，分别
	 * 建在大对象数据表上的第一和第二列，即lo_id
	 * 列和pageno列。
	 */
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 2;
	colinfo->col_number = (size_t*)palloc(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = LARGE_OBJ_LO_ID;
	colinfo->col_number[1] = LARGE_OBJ_PAGENO;

	colinfo->rd_comfunction = (CompareCallback*)palloc(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = relid_compare;
	colinfo->rd_comfunction[1] = int4_compare;

	colinfo->split_function = large_data_index_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(LargeObjDataIndex, colinfo);
}

static void init_metadata_heap_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = META_TABLE_COL_NUM;
	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;
	colinfo->split_function = meta_item_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(MetaTableId, colinfo);
}

//init index colinfo
static void init_metadata_index_col1_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = META_TABLE_COL_OID;
	colinfo->rd_comfunction = (CompareCallback*)palloc(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = relid_compare;
	colinfo->split_function =  col1_oid_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(MetaTableColFirstIndex, colinfo);
}

//init index colinfo
static void init_metadata_index_col3_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = META_TABLE_COL_POID;
	colinfo->rd_comfunction = (CompareCallback*)palloc(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = relid_compare;
	colinfo->split_function =  col1_oid_split;
	MemoryContextSwitchTo(ocxt);
    setColInfo(MetaTableColThirdIndex,colinfo);
}

void init_meta_colinfo( void )
{
	init_metadata_heap_colinfo();
	init_metadata_index_col1_colinfo();
	init_metadata_index_col3_colinfo();

	InitDatabaseColiId();
	InitSequenceColiId();
	InitTablespaceColiId();

	init_large_meta_colinfo();
	init_large_meta_index_colinfo();
	init_large_data_rel_colinfo();
	init_large_data_index_colinfo();
	init_large_heap_rel_colinfo();
	init_large_heap_index_colinfo();

	/* 图数据元数据表colinfo初始化 */
	InitRDFMetaTableColinfo();
}

void fxdb_mtinfo_append_index(MtInfo info,IndexType indexType,Oid idxId,Colinfo colinfo)
{
	Assert(NULL != info);
	if (0 == info->indinfo.index_num)
	{
		MemoryContext oldCtx = MemoryContextSwitchTo(CacheMemoryContext);
		info->indinfo.index_num = 1;
		info->indinfo.index_array = (unsigned int*)palloc(sizeof(unsigned int));
		info->indinfo.type = (unsigned int*)palloc(sizeof(unsigned int));
		info->indinfo.index_info = (Colinfo*)palloc(sizeof(Colinfo));
		info->indinfo.type[0] = indexType;
		info->indinfo.index_array[0] = idxId;
		info->indinfo.index_info[0] = colinfo;
		MemoryContextSwitchTo(oldCtx);
	}
	else
	{
		++info->indinfo.index_num;
		info->indinfo.index_array = (unsigned int*)repalloc(info->indinfo.index_array,sizeof(unsigned int) * info->indinfo.index_num);
		info->indinfo.type = (unsigned int*)repalloc(info->indinfo.type,sizeof(unsigned int) * info->indinfo.index_num);
		info->indinfo.index_info = (Colinfo*)repalloc(info->indinfo.index_info,sizeof(Colinfo) * info->indinfo.index_num);
		info->indinfo.type[info->indinfo.index_num - 1] = indexType;
		info->indinfo.index_array[info->indinfo.index_num - 1] = idxId;
		info->indinfo.index_info[info->indinfo.index_num - 1] = colinfo;
	}
}

double get_meta_table_tuples(char *data)
{
	Assert(data != NULL);

	double tuples = 0;
    RangeData rd = {0};
    meta_item_split(rd, data, META_TABLE_COL_TUPLES, 0);
	memcpy(&tuples, &data[rd.start], rd.len);
	return tuples;
}

Oid get_meta_oid(char *data)
{
	Assert(data != NULL);

	Oid tmp_oid = 0;
	memcpy(&tmp_oid, data, OID_SIZE);
	return tmp_oid;
}


void fxdb_create_catalog()
{
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();

	//initialize the colinfo for meta table
	init_meta_colinfo();

	/* Create meta table. */
	fxdb_heap_create(MetaTableId,MetaTableId);
	CommandCounterIncrement();

	Relation metaRelation = heap_open(MetaTableId,RowExclusiveLock);

	/*
	 * Create index of meta table.
	 */
	fxdb_index_create(MetaTableColFirstIndex, metaRelation, BTREE_UNIQUE_TYPE, MetaTableColFirstIndex);
	fxdb_index_create(MetaTableColThirdIndex, metaRelation, BTREE_TYPE, MetaTableColThirdIndex);

	heap_close(metaRelation, RowExclusiveLock);

	//create tablespace meta table
	fxdb_heap_create(TableSpaceRelationId,GetTablespaceMetaColiId());

	Relation tablespaceRel = heap_open(TableSpaceRelationId,RowExclusiveLock);
	fxdb_index_create(TableSpaceNameIdxId,tablespaceRel,BTREE_UNIQUE_TYPE,GetTablespaceNameIdxColiId());
	fxdb_index_create(TableSpaceOidIdxId,tablespaceRel,BTREE_UNIQUE_TYPE,GetTablespaceOidIdxColiId());
	heap_close(tablespaceRel, RowExclusiveLock);

	//create database meta table and create defaultdatabase
	fxdb_heap_create(DatabaseRelationId, GetDatabaseColiId());

	Relation dbRel = heap_open(DatabaseRelationId,RowExclusiveLock);
	fxdb_index_create(DatabaseUniqueId,dbRel,BTREE_UNIQUE_TYPE,DatabaseUniqueId);
	fxdb_index_create(DatabaseNameIdxId,dbRel,BTREE_UNIQUE_TYPE,DatabaseNameIdxId);
	heap_close(dbRel,RowExclusiveLock);
	CreateDatabase(GetCurrentTransactionId(),DEFAULT_DATABASE_NAME,DEFAULT_TABLESPACE_NAME,NULL,0,MyDatabaseId); 

	CreateDefaultLORel();

	CreateRDFTable();

	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();
}

static 
void build_table_colinfo(MtInfo mt_info, Oid colid)
{
	Colinfo pcolinfo = getColInfo(colid);
	if (pcolinfo != NULL) {
		mt_info->colid = colid;
		mt_info->tableColInfo = (Colinfo)palloc0(sizeof(ColinfoData));
		mt_info->tableColInfo->tuple_size = pcolinfo->tuple_size;
		mt_info->tableColInfo->keys = pcolinfo->keys;

		if(pcolinfo->col_number != NULL)
		{
			mt_info->tableColInfo->col_number = 
				(size_t*)palloc0(mt_info->tableColInfo->keys * sizeof(size_t));

			memcpy(mt_info->tableColInfo->col_number,
				pcolinfo->col_number,
				mt_info->tableColInfo->keys * sizeof(size_t));
		}

		if(pcolinfo->rd_comfunction != NULL)
		{
			mt_info->tableColInfo->rd_comfunction = 
				(CompareCallback*)palloc0(mt_info->tableColInfo->keys * sizeof(CompareCallback));

			memcpy(mt_info->tableColInfo->rd_comfunction, 
				pcolinfo->rd_comfunction, 
				mt_info->tableColInfo->keys * sizeof(CompareCallback));
		}

		mt_info->tableColInfo->split_function = pcolinfo->split_function;
	}
}

static 
void build_tmp_mt_info(MtInfo mt_info, 
											 Oid database_id, 
											 Oid spcid, 
											 Oid colid, 
											 unsigned int type)
{
	mt_info->database_id = database_id;
	mt_info->parentId = 0;
	mt_info->relpersistence = RELPERSISTENCE_TEMP;
	mt_info->spcid = spcid;
	mt_info->type = type;

	build_table_colinfo(mt_info, colid);

	mt_info->indinfo.index_num = 0;
	mt_info->indinfo.index_array = NULL;
	mt_info->indinfo.index_info = NULL;
	mt_info->indinfo.type = NULL;
}

Oid fxdb_heap_create(Oid relid,
					  Oid colid,
					  Oid reltablespace,
					  Oid dbid,
					  char relkind,
						char relpersistence,
						OnCommitAction action)

{
	if(relpersistence != RELPERSISTENCE_TEMP)
		CheckStandbyUpdateError("Can not execute create table during recovery.");

	if (0 == dbid)
	{
		dbid = MyDatabaseId;
	}

	if (0 == reltablespace)
	{
		reltablespace = MyDatabaseTableSpace;
	}

	extern TupleDesc get_toast_tuple_desc(void);
	extern Oid GetNewObjectId(void);
	extern Oid GetTempObjectId(void);
	extern bool IsTmpOid(Oid);

	if(relid == InvalidOid &&
		relpersistence == RELPERSISTENCE_TEMP)
	{
		relid = GetTempObjectId();
	} else if(relid == InvalidOid)
	{
		relid = GetNewObjectId();
	}
	Colinfo colinfo = getColInfo(colid);
	Relation newRelation;

	newRelation=  heap_create("", 1, reltablespace, relid,dbid
		,(relkind == RELKIND_RELATION) ? single_attibute_tupDesc : get_toast_tuple_desc(), relkind,
		relpersistence, false, false, false, colinfo);

	/* We don't put the temp rel id into meta table.*/
	if(!isMetaTable(relid) && !IsTmpOid(relid))
	{
		time_t now = time(NULL);
		fxdb_insert_meta(relid, UNKNOWN_TYPE, 0, (pg_time_t)time, 0, dbid, reltablespace, colid, relpersistence);
		CommandCounterIncrement();
	}

	if (!IsSystemTable(relid) && !is_toast_table_id(relid))
	{
		MemoryContext oldCtx = MemoryContextSwitchTo(CacheMemoryContext);

		if(IsTmpOid(relid))
		{
			build_tmp_mt_info(&newRelation->mt_info, dbid, reltablespace, colid, UNKNOWN_TYPE);
		} else {
			fxdb_get_mete_info(relid,&newRelation->mt_info);
		}

		MemoryContextSwitchTo(oldCtx);

		if (!IsSystemTable(relid) && !IsTmpOid(relid))
		{
			create_toast_table(newRelation,get_toast_table_id(relid),get_toast_index_id(relid),0);
		}
	} 

	heap_close(newRelation, NoLock);

	if(action != ONCOMMIT_NOOP)
	{
		register_on_commit_action(relid, action);
	}

	return relid;
}

static 
void heap_drop_index(unsigned int index_cnt, unsigned int *index_arr, Oid relid, Oid dbid)
{
	if (index_cnt <= 0)
		return;

	uint32 *tmp_arr = (unsigned int *)palloc0(sizeof(uint32) * index_cnt);
	memcpy(tmp_arr, index_arr, sizeof(uint32) * index_cnt);

	for (uint32 i = 0; i < index_cnt; ++i)
	{
		index_drop(tmp_arr[i], relid, dbid);
	}
	pfree(tmp_arr);
}

void fdxdb_heap_drop(Oid relid,Oid  dbid)
{
	extern Oid get_toast_table_id(Oid relId);
	extern Oid get_toast_index_id(Oid relId);

	Relation rel = relation_open(relid, AccessExclusiveLock);
	if (NULL == rel)
	{
		remove_on_commit_action(relid);
		return;
	}

	if (get_toast_table_id(relid) != relid)
	{
		index_drop(get_toast_index_id(relid),get_toast_table_id(relid),dbid);
		fdxdb_heap_drop(get_toast_table_id(relid));
	}

	extern bool IsTmpOid(Oid);
 
	if(!IsTmpOid(relid))
	{
		CheckStandbyUpdateError("Can not execute drop table during recovery.");
		GetCurrentTransactionId();
	}
	else
	{
		Fxdb_GetTmpCurrentTransactionId();
	}

	// drop all indexes of this heap	
	heap_drop_index(MtInfo_GetIndexCount(rel->mt_info), MtInfo_GetIndexOid(rel->mt_info), relid, dbid);

	/*
	* There can no longer be anyone *else* touching the relation, but we
	* might still have open queries or cursors, or pending trigger events, in
	* our own session.
	*/
	CheckTableNotInUse(rel, "DROP TABLE"); //this function is fired in tablecmds.c

	CacheInvalidateRelcache(rel);

	RelationDropStorage(rel);
	/*
	* Close relcache entry, but *keep* AccessExclusiveLock on the relation
	* until transaction commit.  This ensures no one else will try to do
	* something with the doomed relation.
	*/
	relation_close(rel, NoLock);

	/*
	* Forget any ON COMMIT action for the rel
	*/
	remove_on_commit_action(relid); //no commit action is ever registered this fuction is fired in tablecmds.c
	RelationForgetRelation(relid);

	fxdb_delete_meta(relid);
}

HeapTuple fxdb_search_meta_table_copy(Oid relid, Snapshot snapshot)
{
	Assert(relid > 0);

	//open MetaTableId
	Relation rel = heap_open(MetaTableId,
		AccessShareLock);

	//open MetaTableColFirstIndex
	Relation indrel = index_open(MetaTableColFirstIndex,
		AccessShareLock);

	char compare_data[4];//compare string
	memset(compare_data, 0, sizeof(compare_data));
	memcpy(compare_data, &relid, sizeof(relid));

	Datum value[1] ;
	value[0] = fdxdb_string_formdatum(compare_data, sizeof(compare_data));
	ScanKeyData key[1];
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], 1, BTEqualStrategyNumber, relid_compare, value[0]);
	IndexScanDesc indscan = index_beginscan(rel, indrel, snapshot, 1, 0, 0, 0);
	index_rescan(indscan, key, 1, NULL, 0);

	HeapTuple tuple = NULL;

	//found object_id
	tuple = index_getnext(indscan, ForwardScanDirection);
	HeapTuple new_tuple = NULL;
	new_tuple = heap_copytuple(tuple);

	/* 这里需要消除警告,所以要两次getnext */
	if(tuple)
	{
		index_getnext(indscan, ForwardScanDirection);
	}

	index_endscan(indscan);
	index_close(indrel, AccessShareLock);
	heap_close(rel, AccessShareLock);
	return new_tuple;
}

/*
 *fxdb_get_metastruct_ptr --
 *  
 *  返回tuple中的数据引用，可用于在外部改变该tuple在relation中的值
 */
void *fxdb_get_metastruct_ptr(HeapTuple tup, bool *shuold_free)
{
	extern TupleDesc single_attibute_tupDesc;

	char *result = NULL;

	Datum  values[1] ;
	bool   isnull[1] ;

	heap_deform_tuple(tup, single_attibute_tupDesc, values, isnull);
	text *t = (text *) DatumGetPointer(values[0]);
	text *tunpacked = pg_detoast_datum_packed((struct varlena *) t);

	if(t != tunpacked)
	{
		*shuold_free = true;
		int len = VARSIZE_ANY_EXHDR(tunpacked);
		result = (char*)palloc0(len + 1);
		memcpy(result, VARDATA_ANY(tunpacked), len);
		pfree(tunpacked);
	} else {
		*shuold_free = false;
		result = (char*)VARDATA_ANY(tunpacked);
	}

	return result;
}

Form_meta_table fxdb_get_meta_table_struct_without_copy(HeapTuple tuple)
{
	bool should_free;
	Form_meta_table meta = (Form_meta_table_data *)fxdb_get_metastruct_ptr(tuple, &should_free);
	Assert(!should_free);
	return meta;
}

Form_meta_table fxdb_get_meta_table_struct(HeapTuple tuple)
{
	char *data = fxdb_tuple_to_chars(tuple);
	return (Form_meta_table)data;
}

void DoUnlinkRel(Oid relid, Oid dbid, Oid tblspace, Oid backendid)
{
	RelFileNode fileNode;
	fileNode.dbNode = dbid;
	fileNode.relNode = relid;
	fileNode.spcNode = tblspace;
	SMgrRelation srel = smgropen(fileNode, backendid);
	for (int i = 0; i <= MAX_FORKNUM; i++)
	{
		if (smgrexists(srel, (ForkNumber)i))
			smgrdounlink(srel, (ForkNumber)i, false);
	}
	smgrclose(srel);
}

void clean_rel(Oid begin, size_t size, Oid dbid, Oid backendid)
{
	ListCell *cell = NULL;
	List *l = ListTableSpace();
	bool gonext = false;

	for(Oid i = begin; i < begin + size; ++i)
	{
		foreach(cell, l)
		{
			gonext = false;

			FormTablespaceData tmpClass = 
				(FormTablespaceData)lfirst(cell);

			Oid toast_id = get_toast_table_id(i);
			Oid toast_index_id = get_toast_index_id(i);
			
			DoUnlinkRel(i, dbid, tmpClass->id, MyBackendId);
			DoUnlinkRel(toast_id, dbid, tmpClass->id, -1);
			DoUnlinkRel(toast_index_id, dbid, tmpClass->id, -1);

			if(gonext)
				break;
		}
	}

	list_free_deep(l);
}

Oid __fxdb_index_create(Oid relid, Relation heapRelation, IndexType indextype,
	Oid coliId, Relation parentRelation, void* userData,filter_func_t filterfunc, uint32 userdataLength, bool skipInsert = false)
{
	char relpersistence = 0;

	extern Oid GetNewObjectId(void);
	extern Oid GetTempObjectId(void);
	extern bool IsTmpOid(Oid);

	if(parentRelation == NULL)
	{
		parentRelation = heapRelation;
	}

	if(indextype != LSM_SUBTYPE){
		Assert(heapRelation == parentRelation);
	}

	/*
	cluster indexes stores data and don't have transaction isolation properties, always used as temp tables.
	*/
    if (IS_CLUSTER_INDEX(indextype))
		{
        relpersistence = RELPERSISTENCE_TEMP;
		}
    else
		{
			if(!RelationIsTemp(heapRelation))
				CheckStandbyUpdateError("Can not execute create index during recovery.");
			
			relpersistence = heapRelation->rd_rel->relpersistence;
		}
	if(relid == InvalidOid &&
		relpersistence == RELPERSISTENCE_TEMP)
	{
		relid = GetTempObjectId();
	} else if(relid == InvalidOid)
	{
		relid = GetNewObjectId();
	}
	MemoryContext indexcxt, oldcxt;
	if (!CacheMemoryContext)
		CreateCacheMemoryContext();
	oldcxt = MemoryContextSwitchTo(CacheMemoryContext);
	Relation	indexRelation;	
	IndexInfo* indexInfo;
	indexInfo = makeNode(IndexInfo);
	indexInfo->ii_KeyAttrNumbers[0]=1;
	indexInfo->ii_NumIndexAttrs = 1;
	indexInfo->ii_Expressions = NIL;	/* for now */
	indexInfo->ii_ExpressionsState = NIL;
	indexInfo->ii_Predicate = NIL;
	indexInfo->ii_PredicateState = NIL;
	indexInfo->ii_ExclusionOps = 0;
	indexInfo->ii_ExclusionProcs = 0;
	indexInfo->ii_ExclusionStrats = 0;
	indexInfo->ii_Unique = indextype >= UINQUE_TYPE_BASE;
	/* In a concurrent build, mark it not-ready-for-inserts */
	indexInfo->ii_ReadyForInserts = !skipInsert;
	indexInfo->ii_Concurrent = false;
	indexInfo->ii_BrokenHotChain = false;
	
	Oid idxdbid = heapRelation ? heapRelation->rd_node.dbNode : MyDatabaseId;
	Oid idxspcid = heapRelation == 0 ? MyDatabaseTableSpace : heapRelation->rd_node.spcNode;
	
	char name_index[1024];
	memset(name_index, 0, 1024);

	if (IS_CLUSTER_INDEX(indextype))
		sprintf(name_index, "testindex");
	else {
		if (RelationIsTemp(heapRelation))
			sprintf(name_index, "temp index %u in %u", relid, heapRelation->rd_node.relNode);
		else
			sprintf(name_index, "index %u in %u", relid, heapRelation->rd_node.relNode);
	}
	
	indexRelation = heap_create(name_index,
		1,
		idxspcid,
		relid,
		idxdbid,
		single_attibute_tupDesc,
		RELKIND_INDEX,
		relpersistence,
		false,
		false,
		false,
		getColInfo(coliId));
		
	if (BTREE_TYPE == indextype || BTREE_UNIQUE_TYPE == indextype
	|| BTREE_CLUSTER_TYPE == indextype || BTREE_UNIQUE_CLUSTER_TYPE == indextype
	|| LSM_TYPE == indextype || LSM_SUBTYPE == indextype)
	{
		indexRelation->rd_am = &btree_am;
		indexRelation->rd_aminfo = &btree_aminfo;
	}
	else if (HASH_TYPE == indextype)
	{
		indexRelation->rd_am = &hash_am;
		indexRelation->rd_aminfo = &hash_aminfo;
	}
	indexRelation->rel_kind = RELKIND_INDEX;// for test
	indexcxt = AllocSetContextCreate(CacheMemoryContext,
		RelationGetRelationName(indexRelation),
		ALLOCSET_SMALL_MINSIZE,
		ALLOCSET_SMALL_INITSIZE,
		ALLOCSET_SMALL_MAXSIZE);

	indexRelation->rd_indexcxt = indexcxt;
	indexRelation->mt_info.type = indextype;
	indexRelation->user_data = userData;
	if (!skipInsert) {
		index_build(heapRelation,
			indexRelation,
			indexInfo,
			false,
			false, userData, filterfunc);
	}

	if(!isMetaTable(relid) && !IsTmpOid(relid))
	{
		time_t now = time(NULL);
		fxdb_insert_meta(relid, indextype, parentRelation == 0 ? InvalidOid : parentRelation->rd_node.relNode
			, (pg_time_t)time,0, idxdbid, idxspcid, coliId, relpersistence, userData, userdataLength);
		CommandCounterIncrement();
	}

	if(!isMetaTable(relid) && !is_toast_id(relid))
	{
		if(IsTmpOid(relid))
		{
			build_tmp_mt_info(&indexRelation->mt_info, 
				idxdbid, 
				idxspcid,
				coliId,
				indextype);
		} else {
			fxdb_get_mete_info(relid,&indexRelation->mt_info);
		}
        if (parentRelation) {
		    fxdb_mtinfo_append_index(&parentRelation->mt_info,indextype,relid,getColInfo(coliId));
		    CacheInvalidateRelcache(parentRelation);
		}
	}
	indexRelation->user_data = NULL;
	index_close(indexRelation, NoLock);
	MemoryContextSwitchTo(oldcxt);

	pfree(indexInfo);

	return relid;
}

Oid fxdb_index_create(Oid relid,Relation heapRelation, IndexType indextype,Oid coliId,
	void* userData,filter_func_t filterfunc, uint32 userdataLength, bool skipInsert)
{
	if(indextype == LSM_TYPE)
	{
		const static uint32 lsm_sub_idx_count = 4;
		Oid lsm_idx_id = __fxdb_index_create(relid, heapRelation, indextype, coliId, NULL, userData,0, userdataLength);
		Relation lsm_idx_rel = index_open(lsm_idx_id, AccessShareLock);
		for(uint32 i = 0; i < lsm_sub_idx_count; i++)
		{
			Oid lsm_sub_idx_id = InvalidOid;
			lsm_sub_idx_id = __fxdb_index_create(lsm_sub_idx_id, heapRelation, LSM_SUBTYPE, coliId, lsm_idx_rel, 0, 0,0);
			Relation newSubIdxRel = index_open(lsm_sub_idx_id, AccessShareLock);
			CacheInvalidateRelcache(newSubIdxRel);
			index_close(newSubIdxRel, AccessShareLock);
		}
		index_close(lsm_idx_rel, NoLock);
		return lsm_idx_id;
	}

	return __fxdb_index_create(relid, heapRelation, indextype, coliId, NULL, userData,filterfunc, userdataLength, skipInsert);
}

#ifndef FOUNDER_XDB_SE
/* --------------------------------
*		ExecStoreTuple
*
*		This function is used to store a physical tuple into a specified
*		slot in the tuple table.
*
*		tuple:	tuple to store
*		slot:	slot to store it in
*		buffer: disk buffer if tuple is in a disk page, else InvalidBuffer
*		shouldFree: true if ExecClearTuple should pfree() the tuple
*					when done with it
*
* If 'buffer' is not InvalidBuffer, the tuple table code acquires a pin
* on the buffer which is held until the slot is cleared, so that the tuple
* won't go away on us.
*
* shouldFree is normally set 'true' for tuples constructed on-the-fly.
* It must always be 'false' for tuples that are stored in disk pages,
* since we don't want to try to pfree those.
*
* Another case where it is 'false' is when the referenced tuple is held
* in a tuple table slot belonging to a lower-level executor Proc node.
* In this case the lower-level slot retains ownership and responsibility
* for eventually releasing the tuple.	When this method is used, we must
* be certain that the upper-level Proc node will lose interest in the tuple
* sooner than the lower-level one does!  If you're not certain, copy the
* lower-level tuple with heap_copytuple and let the upper-level table
* slot assume ownership of the copy!
*
* Return value is just the passed-in slot pointer.
*
* NOTE: before PostgreSQL 8.1, this function would accept a NULL tuple
* pointer and effectively behave like ExecClearTuple (though you could
* still specify a buffer to pin, which would be an odd combination).
* This saved a couple lines of code in a few places, but seemed more likely
* to mask logic errors than to be really useful, so it's now disallowed.
* --------------------------------
*/
TupleTableSlot *
ExecStoreTuple(HeapTuple tuple,
			   TupleTableSlot *slot,
			   Buffer buffer,
			   bool shouldFree)
{
	/*
	* sanity checks
	*/
	Assert(tuple != NULL);
	Assert(slot != NULL);
	Assert(slot->tts_tupleDescriptor != NULL);
	/* passing shouldFree=true for a tuple on a disk page is not sane */
	Assert(BufferIsValid(buffer) ? (!shouldFree) : true);

	/*
	* Free any old physical tuple belonging to the slot.
	*/
	if (slot->tts_shouldFree)
		heap_freetuple(slot->tts_tuple);
	if (slot->tts_shouldFreeMin)
		heap_free_minimal_tuple(slot->tts_mintuple);

	/*
	* Store the new tuple into the specified slot.
	*/
	slot->tts_isempty = false;
	slot->tts_shouldFree = shouldFree;
	slot->tts_shouldFreeMin = false;
	slot->tts_tuple = tuple;
	slot->tts_mintuple = NULL;

	/* Mark extracted state invalid */
	slot->tts_nvalid = 0;

	/*
	* If tuple is on a disk page, keep the page pinned as long as we hold a
	* pointer into it.  We assume the caller already has such a pin.
	*
	* This is coded to optimize the case where the slot previously held a
	* tuple on the same disk page: in that case releasing and re-acquiring
	* the pin is a waste of cycles.  This is a common situation during
	* seqscans, so it's worth troubling over.
	*/
	if (slot->tts_buffer != buffer)
	{
		if (BufferIsValid(slot->tts_buffer))
			ReleaseBuffer(slot->tts_buffer);
		slot->tts_buffer = buffer;
		if (BufferIsValid(buffer))
			IncrBufferRefCount(buffer);
	}

	return slot;
}

/* ----------------------------------------------------------------
*				 Executor state and memory management functions
* ----------------------------------------------------------------
*/

/* ----------------
*		CreateExecutorState
*
*		Create and initialize an EState node, which is the root of
*		working storage for an entire Executor invocation.
*
* Principally, this creates the per-query memory context that will be
* used to hold all working data that lives till the end of the query.
* Note that the per-query context will become a child of the caller's
* CurrentMemoryContext.
* ----------------
*/
EState *
CreateExecutorState(void)
{
	EState	   *estate;
	MemoryContext qcontext;
	MemoryContext oldcontext;

	/*
	* Create the per-query context for this Executor run.
	*/
	qcontext = AllocSetContextCreate(CurrentMemoryContext,
		"ExecutorState",
		ALLOCSET_DEFAULT_MINSIZE,
		ALLOCSET_DEFAULT_INITSIZE,
		ALLOCSET_DEFAULT_MAXSIZE);

	/*
	* Make the EState node within the per-query context.  This way, we don't
	* need a separate pfree() operation for it at shutdown.
	*/
	oldcontext = MemoryContextSwitchTo(qcontext);

	estate = makeNode(EState);

	/*
	* Initialize all fields of the Executor State structure
	*/
	estate->es_direction = ForwardScanDirection;
	estate->es_snapshot = SnapshotNow;
	estate->es_crosscheck_snapshot = InvalidSnapshot;	/* no crosscheck */
	estate->es_range_table = NIL;
	estate->es_plannedstmt = NULL;

	estate->es_junkFilter = NULL;

	estate->es_output_cid = (CommandId) 0;

	estate->es_result_relations = NULL;
	estate->es_num_result_relations = 0;
	estate->es_result_relation_info = NULL;

	estate->es_trig_target_relations = NIL;
#ifndef FOUNDER_XDB_SE
	estate->es_trig_tuple_slot = NULL;
	estate->es_trig_oldtup_slot = NULL;
#endif
	estate->es_param_list_info = NULL;
	estate->es_param_exec_vals = NULL;

	estate->es_query_cxt = qcontext;

	estate->es_tupleTable = NIL;

	estate->es_rowMarks = NIL;

	estate->es_processed = 0;
	estate->es_lastoid = InvalidOid;

	estate->es_top_eflags = 0;
	estate->es_instrument = 0;
	estate->es_select_into = false;
	estate->es_into_oids = false;
	estate->es_finished = false;

	estate->es_exprcontexts = NIL;

	estate->es_subplanstates = NIL;

	estate->es_auxmodifytables = NIL;

	estate->es_per_tuple_exprcontext = NULL;

	estate->es_epqTuple = NULL;
	estate->es_epqTupleSet = NULL;
	estate->es_epqScanDone = NULL;

	/*
	* Return the executor state structure
	*/
	MemoryContextSwitchTo(oldcontext);

	return estate;
}

/* ----------------
*		CreateExprContext
*
*		Create a context for expression evaluation within an EState.
*
* An executor run may require multiple ExprContexts (we usually make one
* for each Plan node, and a separate one for per-output-tuple processing
* such as constraint checking).  Each ExprContext has its own "per-tuple"
* memory context.
*
* Note we make no assumption about the caller's memory context.
* ----------------
*/
ExprContext *
CreateExprContext(EState *estate)
{
	ExprContext *econtext;
	MemoryContext oldcontext;

	/* Create the ExprContext node within the per-query memory context */
	oldcontext = MemoryContextSwitchTo(estate->es_query_cxt);

	econtext = makeNode(ExprContext);

	/* Initialize fields of ExprContext */
#ifndef FOUNDER_XDB_SE
	econtext->ecxt_scantuple = NULL;
	econtext->ecxt_innertuple = NULL;
	econtext->ecxt_outertuple = NULL;
#endif
	econtext->ecxt_per_query_memory = estate->es_query_cxt;

	/*
	* Create working memory for expression evaluation in this context.
	*/
	econtext->ecxt_per_tuple_memory =
		AllocSetContextCreate(estate->es_query_cxt,
		"ExprContext",
		ALLOCSET_DEFAULT_MINSIZE,
		ALLOCSET_DEFAULT_INITSIZE,
		ALLOCSET_DEFAULT_MAXSIZE);

	econtext->ecxt_param_exec_vals = estate->es_param_exec_vals;
	econtext->ecxt_param_list_info = estate->es_param_list_info;

	econtext->ecxt_aggvalues = NULL;
	econtext->ecxt_aggnulls = NULL;

	econtext->caseValue_datum = (Datum) 0;
	econtext->caseValue_isNull = true;

	econtext->domainValue_datum = (Datum) 0;
	econtext->domainValue_isNull = true;

	econtext->ecxt_estate = estate;

	econtext->ecxt_callbacks = NULL;

	/*
	* Link the ExprContext into the EState to ensure it is shut down when the
	* EState is freed.  Because we use lcons(), shutdowns will occur in
	* reverse order of creation, which may not be essential but can't hurt.
	*/
	estate->es_exprcontexts = lcons(econtext, estate->es_exprcontexts);

	MemoryContextSwitchTo(oldcontext);

	return econtext;
}

/* --------------------------------
*		MakeTupleTableSlot
*
*		Basic routine to make an empty TupleTableSlot.
* --------------------------------
*/
TupleTableSlot *
MakeTupleTableSlot(void)
{
	TupleTableSlot *slot = makeNode(TupleTableSlot);

	slot->tts_isempty = true;
	slot->tts_shouldFree = false;
	slot->tts_shouldFreeMin = false;
	slot->tts_tuple = NULL;
	slot->tts_tupleDescriptor = NULL;
	slot->tts_mcxt = CurrentMemoryContext;
	slot->tts_buffer = InvalidBuffer;
	slot->tts_nvalid = 0;
	slot->tts_values = NULL;
	slot->tts_isnull = NULL;
	slot->tts_mintuple = NULL;

	return slot;
}

/* --------------------------------
*		ExecClearTuple
*
*		This function is used to clear out a slot in the tuple table.
*
*		NB: only the tuple is cleared, not the tuple descriptor (if any).
* --------------------------------
*/
TupleTableSlot *				/* return: slot passed */
ExecClearTuple(TupleTableSlot *slot)	/* slot in which to store tuple */
{
	/*
	* sanity checks
	*/
	Assert(slot != NULL);

	/*
	* Free the old physical tuple if necessary.
	*/
	if (slot->tts_shouldFree)
		heap_freetuple(slot->tts_tuple);
	if (slot->tts_shouldFreeMin)
		heap_free_minimal_tuple(slot->tts_mintuple);

	slot->tts_tuple = NULL;
	slot->tts_mintuple = NULL;
	slot->tts_shouldFree = false;
	slot->tts_shouldFreeMin = false;

	/*
	* Drop the pin on the referenced buffer, if there is one.
	*/
	if (BufferIsValid(slot->tts_buffer))
		ReleaseBuffer(slot->tts_buffer);

	slot->tts_buffer = InvalidBuffer;

	/*
	* Mark it empty.
	*/
	slot->tts_isempty = true;
	slot->tts_nvalid = 0;

	return slot;
}

/* --------------------------------
*		ExecSetSlotDescriptor
*
*		This function is used to set the tuple descriptor associated
*		with the slot's tuple.  The passed descriptor must have lifespan
*		at least equal to the slot's.  If it is a reference-counted descriptor
*		then the reference count is incremented for as long as the slot holds
*		a reference.
* --------------------------------
*/
void
ExecSetSlotDescriptor(TupleTableSlot *slot,		/* slot to change */
					  TupleDesc tupdesc)		/* new tuple descriptor */
{
	/* For safety, make sure slot is empty before changing it */
	ExecClearTuple(slot);

	/*
	* Release any old descriptor.	Also release old Datum/isnull arrays if
	* present (we don't bother to check if they could be re-used).
	*/
	if (slot->tts_tupleDescriptor)
		ReleaseTupleDesc(slot->tts_tupleDescriptor);

	if (slot->tts_values)
		pfree(slot->tts_values);
	if (slot->tts_isnull)
		pfree(slot->tts_isnull);

	/*
	* Install the new descriptor; if it's refcounted, bump its refcount.
	*/
	slot->tts_tupleDescriptor = tupdesc;
	PinTupleDesc(tupdesc);

	/*
	* Allocate Datum/isnull arrays of the appropriate size.  These must have
	* the same lifetime as the slot, so allocate in the slot's own context.
	*/
	slot->tts_values = (Datum *)
		MemoryContextAlloc(slot->tts_mcxt, tupdesc->natts * sizeof(Datum));
	slot->tts_isnull = (bool *)
		MemoryContextAlloc(slot->tts_mcxt, tupdesc->natts * sizeof(bool));
}
/* --------------------------------
*		MakeSingleTupleTableSlot
*
*		This is a convenience routine for operations that need a
*		standalone TupleTableSlot not gotten from the main executor
*		tuple table.  It makes a single slot and initializes it
*		to use the given tuple descriptor.
* --------------------------------
*/
TupleTableSlot *
MakeSingleTupleTableSlot(TupleDesc tupdesc)
{
	TupleTableSlot *slot = MakeTupleTableSlot();

	ExecSetSlotDescriptor(slot, tupdesc);

	return slot;
}

/*
* Build a per-output-tuple ExprContext for an EState.
*
* This is normally invoked via GetPerTupleExprContext() macro,
* not directly.
*/
ExprContext *
MakePerTupleExprContext(EState *estate)
{
	if (estate->es_per_tuple_exprcontext == NULL)
		estate->es_per_tuple_exprcontext = CreateExprContext(estate);

	return estate->es_per_tuple_exprcontext;
}

void
ExecDropSingleTupleTableSlot(TupleTableSlot *slot)
{
	/* This should match ExecResetTupleTable's processing of one slot */
	Assert(IsA(slot, TupleTableSlot));
	ExecClearTuple(slot);
	if (slot->tts_tupleDescriptor)
		ReleaseTupleDesc(slot->tts_tupleDescriptor);
	if (slot->tts_values)
		pfree(slot->tts_values);
	if (slot->tts_isnull)
		pfree(slot->tts_isnull);
	pfree(slot);
}


void
FreeExecutorState(EState *estate)
{
	/*
	* Shut down and free any remaining ExprContexts.  We do this explicitly
	* to ensure that any remaining shutdown callbacks get called (since they
	* might need to release resources that aren't simply memory within the
	* per-query memory context).
	*/
	while (estate->es_exprcontexts)
	{
		/*
		* XXX: seems there ought to be a faster way to implement this than
		* repeated list_delete(), no?
		*/
		FreeExprContext((ExprContext *) linitial(estate->es_exprcontexts),
			true);
		/* FreeExprContext removed the list link for us */
	}

	/*
	* Free the per-query memory context, thereby releasing all working
	* memory, including the EState node itself.
	*/
	MemoryContextDelete(estate->es_query_cxt);
}

static void
ShutdownExprContext(ExprContext *econtext, bool isCommit)
{
	ExprContext_CB *ecxt_callback;
	MemoryContext oldcontext;

	/* Fast path in normal case where there's nothing to do. */
	if (econtext->ecxt_callbacks == NULL)
		return;

	/*
	* Call the callbacks in econtext's per-tuple context.  This ensures that
	* any memory they might leak will get cleaned up.
	*/
	oldcontext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);

	/*
	* Call each callback function in reverse registration order.
	*/
	while ((ecxt_callback = econtext->ecxt_callbacks) != NULL)
	{
		econtext->ecxt_callbacks = ecxt_callback->next;
		if (isCommit)
			(*ecxt_callback->function) (ecxt_callback->arg);
		pfree(ecxt_callback);
	}

	MemoryContextSwitchTo(oldcontext);
}

void
FreeExprContext(ExprContext *econtext, bool isCommit)
{
	EState	   *estate;

	/* Call any registered callbacks */
	ShutdownExprContext(econtext, isCommit);
	/* And clean up the memory used */
	MemoryContextDelete(econtext->ecxt_per_tuple_memory);
	/* Unlink self from owning EState, if any */
	estate = econtext->ecxt_estate;
	if (estate)
		estate->es_exprcontexts = list_delete_ptr(estate->es_exprcontexts,
		econtext);
	/* And delete the ExprContext node */
	pfree(econtext);
}

/*
int relid_compare(char* a,int len1, char* b, int len2)
{
return *(int*)a - *(int*)b;
}

RangeData meta_item_split(char *str, int col)
{
RangeData rangeData;
rangeData.start = 0;
rangeData.len = 0;
int i = col - 1;
int position = 0;
//char str[] = "123\00023\0009\0";

while(i)
{
if (str[position] == '\0')
{
i--;
position++;
}
else
position++;
}
rangeData.start = position;
int len = 0;
while(str[position] != '\0')
{
len++;
position++;
}

rangeData.len = len + 1;

return rangeData;

}
*/
#endif


//copy object to meta_data, and move &meta_data to next addr
static void copy_and_move(const void *object, const int len, char **meta_data)
{
	memcpy(*meta_data, object, len);//copy object
	*meta_data += len;							//move addr
}

static slock_t s_heap;
static slock_t s_index;

/*
*	get_tuple_in_BTEqualStrategyNumber:
*	using BTEqualStrategyNumber to find a tuple
*/
char* get_tuple_in_BTEqualStrategyNumber(const Oid& index_id,
										 const Oid& object_id,
										 const Oid& table_id = MetaTableId,
										 const Oid& table_space = DEFAULTTABLESPACE_OID)
{
	Assert(index_id > 0 && object_id > 0);
	//open MetaTableId
	Relation rel = heap_open(table_id,
		AccessShareLock);

	//open MetaTableColFirstIndex
	Relation indrel = index_open(index_id,
		AccessShareLock);

	char compare_data[4];//compare string
	memcpy(compare_data, &object_id, sizeof(compare_data));

	Datum value[1] ;
	value[0] = fdxdb_string_formdatum(compare_data, sizeof(compare_data));
	ScanKeyData key[1];
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], META_TABLE_COL_OID, BTEqualStrategyNumber, relid_compare, value[0]);
	IndexScanDesc indscan = index_beginscan(rel, indrel, SnapshotNow, META_TABLE_COL_OID, 0, 0, 0);
	index_rescan(indscan, key, META_TABLE_COL_OID, NULL, 0);

	HeapTuple tuple = NULL;
	char *tmp_data = NULL;
	//found object_id
	while((tuple = index_getnext(indscan, ForwardScanDirection)) != NULL)
	{
		tmp_data = fxdb_tuple_to_chars(tuple);
	}
	index_endscan(indscan);
	index_close(indrel, NoLock);
	heap_close(rel, NoLock);
	return tmp_data;
}

//must return 0 if catch exception
//return MIN_USER_OID if heap is empty
unsigned int fxdb_get_max_id()
{
	Oid max_id = 0;
	Relation rel = NULL;
	Relation indrel = NULL;
	//open MetaTableId
	rel = heap_open(MetaTableId,AccessShareLock);
	indrel = index_open(MetaTableColFirstIndex, AccessShareLock);

	/* Can not return toast id */
	Oid min_toast_oid = 1;
	min_toast_oid = ((Oid)(MIN_TOAST_ID));
	char cmp_data[4];
	memset(cmp_data, 0, sizeof(cmp_data));
	memcpy(cmp_data, &min_toast_oid, sizeof(min_toast_oid));
	Datum datum = fdxdb_string_formdatum(cmp_data, sizeof(cmp_data));
	ScanKeyData key[1];
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], META_TABLE_COL_OID, BTLessStrategyNumber, relid_compare, datum);
	IndexScanDesc indscan = index_beginscan(rel, indrel, SnapshotNow, 1, 0, 0, 0);
	index_rescan(indscan, key, 1, NULL, 0);

	HeapTuple tuple = NULL;
	if((tuple = index_getnext(indscan, BackwardScanDirection)) != NULL)
	{
		char *temp_ = fxdb_tuple_to_chars(tuple);
		memcpy(&max_id, temp_, sizeof(max_id));
		pfree(temp_);
	}
	index_endscan(indscan);
	index_close(indrel, AccessShareLock);
	heap_close(rel, AccessShareLock);
	if(max_id == 0)
	{
		return MIN_USER_OID;
	}
	return max_id;
}
//
///*
//*init attr mt_info
//*return NULL if catch exception
//*/
//static MetaTable alloc_mt_info(const Oid object_id, const Oid tblspace)
//{
//	Assert(object_id > InvalidOid);
//
//	Relation rel = NULL;
//	MetaTable mt = NULL;
//	//not found table sapce
//	if(InvalidOid != tblspace)
//	{
//		rel = relation_open(object_id, RowExclusiveLock);
//		if(NULL == rel->mt_info)
//		{
//			MemoryContext ocxt = MemoryContextSwitchTo(CacheMemoryContext);
//			rel->mt_info = (MetaTable)palloc(sizeof(MetaTableInfo));
//			memset(rel->mt_info, 0, sizeof(MetaTableInfo));
//			mt = rel->mt_info;
//			MemoryContextSwitchTo(ocxt);
//		}else
//		{
//			mt = rel->mt_info;
//		}
//	}
//
//	if(NULL != rel)
//	{
//		relation_close(rel, RowExclusiveLock);
//	}
//	return mt;
//}

/*
*return 1 if mt_info is null 
*or not found tblspace
*or catch exception
*/
//static int meta_info_is_null(const Oid object_id, MetaTable *mt)
//{
//	Assert(object_id > 0);
//	 *mt = NULL;
//	//get table space to open table
//	unsigned int tblspace = 0;
//	char *tmp_data = get_tuple_in_BTEqualStrategyNumber(MetaTableColFirstIndex, object_id);
//	if(NULL == tmp_data)
//	{
//		return 0;
//	}
//	//get col 7
//	RangeData rd = meta_item_split(tmp_data, META_TABLE_COL_TBL_ID);
//	memcpy(&tblspace, &tmp_data[rd.start], rd.len);
//
//	Relation rel = NULL;
//	int sta = 1;
//	//not found table sapce
//	if(0 != tblspace)
//	{
//		rel = relation_open(object_id, RowExclusiveLock);
//		if(NULL == rel->mt_info)
//		{
//			sta = 1;
//		}else
//		{
//			*mt = rel->mt_info;
//			sta = 0;
//		}
//	}
//	if(rel)
//	{
//		relation_close(rel, RowExclusiveLock);
//	}
//	if(sta)
//	{
//		pfree(tmp_data);
//		return 1;
//	}
//	pfree(tmp_data);
//	return 0;
//}
//
////must return 0 if catch exception or tmp_data is null
//unsigned int fxdb_get_tblspace(const Oid object_id)
//{
//	Assert(object_id > InvalidOid);
//	MetaTable mt = NULL;
//	int is_null = meta_info_is_null(object_id, &mt);
//	if(!is_null && NULL != mt && InvalidOid < mt->tblspace)
//	{
//		return mt->tblspace;
//	}
//	unsigned int table_space_id = 0;
//	char *tmp_data = get_tuple_in_BTEqualStrategyNumber(MetaTableColFirstIndex, object_id);
//	if(NULL == tmp_data)
//	{
//		return 0;
//	}
//	//get col 7
//	RangeData rd = meta_item_split(tmp_data, META_TABLE_COL_TBL_ID);
//	memcpy(&table_space_id, &tmp_data[rd.start], rd.len);
//	//get object_id's MetaTable
//	mt = alloc_mt_info(object_id, table_space_id);
//	mt->tblspace = table_space_id;
//	pfree(tmp_data);
//	return table_space_id;
//}

//copy source to dest
static void copy_indinfo(const IndinfoData &source, IndinfoData &dest){
	Assert(source.index_array != NULL);
	Assert(source.index_info != NULL);
	//free dest first
	if(NULL != dest.index_array){
		pfree(dest.index_array);
	}
	if(NULL != dest.index_info)
	{
		pfree(dest.index_info);
	}

	//allocate dest
	dest.index_array = (unsigned int*)
		palloc(sizeof(unsigned int) * source.index_num);
	dest.index_info = (Colinfo*)
		palloc(sizeof(Colinfo) * source.index_num);

	//copy source to dest
	dest.index_num = source.index_num;
	memcpy(dest.index_array,
				 source.index_array,
				 source.index_num * sizeof(unsigned int));
	memcpy(dest.index_info,
				 source.index_info,
				 source.index_num * sizeof(Colinfo));
}
//
////fill attr mt_info.indinfo
//static void fill_relation_indinfo(const Oid object_id, const IndinfoData &indinfo)
//{
//	Assert(object_id > 0);
//	Assert(indinfo.index_array != NULL);
//	Assert(indinfo.index_info != NULL);
//	unsigned int tblspace = fxdb_get_tblspace(object_id);
//	alloc_mt_info(object_id, tblspace);
//	Relation rel = NULL;
//	//not found table sapce
//	if(0 != tblspace)
//	{
//		rel = relation_open(object_id, RowExclusiveLock);
//		//allocate memory in CacheMemoryContext
//		MemoryContext ocxt = MemoryContextSwitchTo(CacheMemoryContext);
//		if(NULL != rel->mt_info->indinfo)
//		{
//			if(NULL != rel->mt_info->indinfo->index_array)
//			{
//				pfree(rel->mt_info->indinfo->index_array);
//			}
//			if(NULL != rel->mt_info->indinfo->index_info)
//			{
//				pfree(rel->mt_info->indinfo->index_info);
//			}
//		}
//		rel->mt_info->indinfo = (Indinfo)palloc(sizeof(IndinfoData));
//		memset(rel->mt_info->indinfo, 0, sizeof(IndinfoData));
//		copy_indinfo(indinfo, *rel->mt_info->indinfo);
//		MemoryContextSwitchTo(ocxt);
//		relation_close(rel, RowExclusiveLock);
//	}
//}

//must return 0 if catch exception
//unsigned int fxdb_get_type(Oid object_id)
//{
//	Assert(object_id > InvalidOid);
//	MetaTable mt = NULL;
//	int is_null = meta_info_is_null(object_id, &mt);
//	if(!is_null && NULL != mt && InvalidOid < mt->type)
//	{
//		return mt->type;
//	}
//	unsigned int type = 0;
//	char *tmp_data = get_tuple_in_BTEqualStrategyNumber(MetaTableColFirstIndex, object_id);
//	if(NULL == tmp_data)
//	{
//		return 0;
//	}
//	//get col 2
//	RangeData rd = meta_item_split(tmp_data, META_TABLE_COL_TYPE);
//	memcpy(&type, &tmp_data[rd.start], rd.len);
//
//	//get object_id's MetaTable
//	Oid tblspace = 0;
//	rd = meta_item_split(tmp_data, META_TABLE_COL_TBL_ID);
//	memcpy(&tblspace, &tmp_data[rd.start], rd.len);
//	mt = alloc_mt_info(object_id, tblspace);
//	mt->type = type;
//	pfree(tmp_data);
//	return type;
//}

//must return 0 if catch exception
unsigned int fxdb_get_heapid(Oid index_id)
{
	Assert(index_id > 0);
	unsigned int heapid = 0;
	char *tmp_data = get_tuple_in_BTEqualStrategyNumber(MetaTableColFirstIndex, index_id);
	if(tmp_data == NULL)
	{
		return 0;
	}
	//get col 3
    RangeData rd = {0};
    meta_item_split(rd, tmp_data, META_TABLE_COL_POID);
	memcpy(&heapid, &tmp_data[rd.start], rd.len);
	pfree(tmp_data);
	return heapid;
}

//free attr mt_info
//static void free_meta_info(const Oid object_id)
//{
//	//invalid oid
//	if(object_id <= 0)
//	{
//		return;
//	}
//	Oid tblspace = fxdb_get_tblspace(object_id);
//	//can not found table space
//	if(tblspace == InvalidOid)
//	{
//		return;
//	}
//	Relation rel = NULL;
//	rel = relation_open(object_id, RowExclusiveLock);
//	//free mt_info
//	if(NULL != rel->mt_info)
//	{
//		if(NULL != rel->mt_info->indinfo)
//		{
//			//free index_array(indinfo)
//			if(NULL != rel->mt_info->indinfo->index_array)
//			{
//				pfree(rel->mt_info->indinfo->index_array);
//				rel->mt_info->indinfo->index_array = NULL;
//			}
//			//free index_info(indinfo)
//			if(NULL != rel->mt_info->indinfo->index_info)
//			{
//				pfree(rel->mt_info->indinfo->index_info);
//				rel->mt_info->indinfo->index_info = NULL;
//			}
//			pfree(rel->mt_info->indinfo);
//			rel->mt_info->indinfo = NULL;
//		}
//		//free mt_info(rel)
//		pfree(rel->mt_info);
//		rel->mt_info = NULL;
//	}
//	relation_close(rel, RowExclusiveLock);
//}
List* fxdb_get_all_table_in_db(Oid dbid)
{
    List	   *result = NULL;
    Relation rel = relation_open(MetaTableId,AccessShareLock);
	if (NULL != rel)
	{
		Datum datumOid = fdxdb_string_formdatum((char*)&dbid,sizeof(Oid));
		ScanKeyData entry;
		Fdxdb_ScanKeyInitWithCallbackInfo(&entry
			,META_TABLE_COL_DB_ID
			,BTEqualStrategyNumber
			,relid_compare
			,datumOid);

		HeapScanDesc  scandesc = heap_beginscan(rel, SnapshotNow, 1, &entry, 0, 0);
		HeapTuple tuple = NULL;
		while( (tuple = heap_getnext(scandesc, ForwardScanDirection)) != NULL)
		{
            RelFileNode *cell = (RelFileNode *)palloc0(sizeof(RelFileNode));
            Form_meta_table meta =  fxdb_get_meta_table_struct(tuple);
			cell->dbNode = dbid;
			cell->relNode = meta->rel_id;
			cell->spcNode = meta->tablespace_id;
			pfree(meta);
			result = lcons(cell, result);
		}

		heap_endscan(scandesc);
		relation_close(rel,AccessShareLock);
	}

    


	return result;
}

//return 0 if fail to insert or catch exception
bool  fxdb_insert_meta(Oid object_id, 
											 int type, 
											 Oid parent_table_id, 
											 pg_time_t time, 
											 int owner, 
											 Oid database_id,
											 Oid tablespace_id, 
											 unsigned int colinfo_id,
											 char relpersistence,
											 void *userdata,
											 uint32 userdataLength)
{
	Assert(object_id > 0);

	//free_meta_info(object_id);
	//free_meta_info(parent_table_id);

	Relation rel = NULL;
	Relation indrel1 = NULL, indrel2 = NULL;
	uint32 form_meta_length = offsetof(Form_meta_table_data, userdata) + sizeof(Oid) + userdataLength;
	Form_meta_table form_meta = (Form_meta_table)palloc0(form_meta_length);
	form_meta->reltuples = 0;
	form_meta->rel_id = object_id;
	form_meta->type = type;
	form_meta->rel_pid = parent_table_id;
	form_meta->rel_filenode = object_id;
	form_meta->time = time;
	form_meta->owner = owner;
	form_meta->database_id = database_id;
	form_meta->tablespace_id = tablespace_id;
	form_meta->colinfo_id = colinfo_id;
	form_meta->relpersistence = relpersistence;
	form_meta->relfrozenxid = RecentXmin;
	SET_VARSIZE(&(form_meta->userdata),(userdataLength));
	if(userdataLength != 0)
	{
		memcpy(VARDATA(&form_meta->userdata), userdata, userdataLength);
	}

	//open metadata table
	rel = heap_open(MetaTableId,
		RowExclusiveLock);

	//open index
	indrel1 = index_open(MetaTableColFirstIndex,
		RowExclusiveLock);

	indrel2 = index_open(MetaTableColThirdIndex,
		RowExclusiveLock);
	HeapTuple tuple = fdxdb_heap_formtuple((char*)form_meta, form_meta_length);
	Datum		values;
	bool		isnull;
	isnull = false;
	values = fdxdb_form_index_datum(rel, indrel1, tuple);
	heap_insert(rel, tuple, GetCurrentCommandId(true), 0, NULL);//insert metadata
	//update index
	index_insert(indrel1,
		&values,
		&isnull,
		&(tuple->t_self),
		rel,
		UNIQUE_CHECK_YES);
	pfree(DatumGetPointer(values));
	Datum		values1;
	values1 = fdxdb_form_index_datum(rel, indrel2, tuple);
	index_insert(indrel2,
		&values1,
		&isnull,
		&(tuple->t_self),
		rel,
		UNIQUE_CHECK_NO);
	pfree(DatumGetPointer(values1));
	index_close(indrel1, RowExclusiveLock);
	index_close(indrel2, RowExclusiveLock);
	heap_close(rel, RowExclusiveLock);
	pfree(form_meta);
	pfree(tuple);

	return true;
}

//return 0 if fail to delete or catch exception
bool  fxdb_delete_meta(Oid object_id)
{
	Assert(object_id > 0);

	Relation rel = NULL;
	Relation indrel = NULL;
	bool found = false;
	//open MetaTableId
	rel = heap_open(MetaTableId, 
		RowExclusiveLock);

	//compare string init
	char compare_data[4];
	memset(compare_data, 0, sizeof(compare_data));
	memcpy(compare_data, &object_id, sizeof(object_id));

	//open MetaTableColFirstIndex
	indrel = index_open(MetaTableColFirstIndex,
		RowExclusiveLock);
	Datum *value = (Datum*)palloc(sizeof(Datum));
	value[0] = fdxdb_string_formdatum(compare_data, sizeof(compare_data));
	ScanKeyData key[1];
	//binding value to scankey
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], META_TABLE_COL_OID, BTEqualStrategyNumber, relid_compare, value[0]);
	IndexScanDesc indscan = index_beginscan(rel, indrel, SnapshotNow, META_TABLE_COL_OID, 0, 0, 0);
	index_rescan(indscan, key, META_TABLE_COL_OID, NULL, 0);
	HeapTuple tuple = NULL;
	//using MetaTableColFirstIndex index to scan
	//object_id is unique and using BTEqualStrategyNumber,so just scan once
	while((tuple = index_getnext(indscan, ForwardScanDirection)) != NULL)
	{
		/*
		*delete tuple
		*/
		simple_heap_delete(rel, &tuple->t_self);
		found = true;
	}
	index_endscan(indscan);	
	index_close(indrel, RowExclusiveLock);
	heap_close(rel, RowExclusiveLock);
	pfree(value);
	//free_meta_info(object_id);
	return found;
}

bool fxdb_get_heapInfo(Oid  m_index_entry_id
					   , MtInfo mt_info)
{
	char tmp[4];
	memset(tmp, 0, sizeof(tmp));
	memcpy(tmp, &m_index_entry_id, sizeof(m_index_entry_id));
	Datum values = fdxdb_string_formdatum(tmp, sizeof(tmp));
	Relation metaRelation = heap_open(MetaTableId, AccessShareLock);
	Relation indexrelation = index_open(MetaTableColFirstIndex, AccessShareLock);

	IndexScanDesc index_scan;
	ScanKeyData key[1];
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],
		META_TABLE_COL_OID,
		BTEqualStrategyNumber, relid_compare,
		values);

	index_scan = index_beginscan(metaRelation, indexrelation, SnapshotNow, META_TABLE_COL_OID, 0, 0, 0);
	index_rescan(index_scan, key, META_TABLE_COL_OID, NULL, 0);

	HeapTuple tup;
	//memset(&range, 0, sizeof(range));
	Oid oid = 0;
	int found = false;
	while((tup = index_getnext(index_scan, ForwardScanDirection)) != NULL)
	{
		int len;
		Form_meta_table data = (Form_meta_table)fdxdb_tuple_to_chars_with_len(tup, len);

		oid = data->colinfo_id;
		mt_info->spcid = data->tablespace_id;

		mt_info->parentId = data->rel_pid;

		mt_info->type = data->type;
		mt_info->database_id = data->database_id;
		mt_info->relpersistence = data->relpersistence;
		mt_info->rel_filenode = data->rel_filenode;
		mt_info->userdataLength = VARSIZE_ANY(&data->userdata);
		if(mt_info->userdataLength != 0)
		{
			mt_info->userdata = (void *)palloc(mt_info->userdataLength);
			memcpy((char *)mt_info->userdata, VARDATA_ANY(&data->userdata), mt_info->userdataLength);
		}
		else
		{
			mt_info->userdata = NULL;
		}
		found = true;
		pfree(data);
	}
	index_endscan(index_scan);	
	index_close(indexrelation, AccessShareLock);
	heap_close(metaRelation, AccessShareLock);
	
	build_table_colinfo(mt_info, oid);
	return found != 0;
}

bool fxdb_get_heap_info(Oid  m_index_entry_id, ColinfoData& colinfo)
{
	Datum values[1] ;
	char tmp[4];
	memcpy(tmp, &m_index_entry_id, sizeof(tmp));
	values[0] = fdxdb_string_formdatum(tmp, sizeof(tmp));
	Relation metaRelation = heap_open(MetaTableId,  AccessShareLock);
	Relation indexrelation = index_open(MetaTableColFirstIndex, AccessShareLock);

	IndexScanDesc index_scan;
	ScanKeyData key[1];
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],
		META_TABLE_COL_OID,
		BTEqualStrategyNumber, relid_compare,
		values[0]);

	index_scan = index_beginscan(metaRelation, indexrelation, SnapshotNow, META_TABLE_COL_OID, 0, 0, 0);
	index_rescan(index_scan, key, META_TABLE_COL_OID, NULL, 0);

	HeapTuple tup;
	char *tt;
	RangeData range;
	memset(&range, 0, sizeof(range));
	Oid oid = 0;
	bool found = false;
	while((tup = index_getnext(index_scan, ForwardScanDirection)) != NULL)
	{
		tt=fxdb_tuple_to_chars(tup);
		meta_item_split(range, tt, META_TABLE_COL_COLINFO_ID);

		oid = *(int*)(&tt[range.start]);
		found = true;
		pfree(tt);
	}
	index_endscan(index_scan);
	index_close(indexrelation, AccessShareLock);
	heap_close(metaRelation, AccessShareLock);
	Colinfo pcolinfo = getColInfo(oid);

	if (pcolinfo != NULL) {
		colinfo.keys = pcolinfo->keys;
		colinfo.col_number = (size_t*)palloc(colinfo.keys*sizeof(size_t));
		memcpy(colinfo.col_number,pcolinfo->col_number,colinfo.keys*sizeof(size_t));
		colinfo.rd_comfunction = (CompareCallback*)palloc(colinfo.keys*sizeof(CompareCallback));
		memcpy(colinfo.rd_comfunction, pcolinfo->rd_comfunction, colinfo.keys*sizeof(CompareCallback));
		colinfo.split_function = pcolinfo->split_function;
	}

	return found;
}

bool fxdb_get_indexInfo(Oid  table_id, IndinfoData& indinfo)
{
	Assert(table_id > InvalidOid);
	memset(&indinfo, 0, sizeof(IndinfoData));
	Relation rel = NULL;
	Relation indrel = NULL;

	rel = heap_open(MetaTableId,
		RowExclusiveLock);

	indrel = index_open(MetaTableColThirdIndex,
		RowExclusiveLock);
	char compare_data[4];//compare string
	memset(compare_data, 0, sizeof(compare_data));
	memcpy(compare_data, &table_id, sizeof(table_id));
	Datum value[1];
	memset(value, 0, sizeof(value));
	value[0] = fdxdb_string_formdatum(compare_data, sizeof(compare_data));
	ScanKeyData key[1];
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], META_TABLE_COL_OID, BTEqualStrategyNumber, relid_compare, value[0]);
	IndexScanDesc indscan = index_beginscan(rel, indrel, SnapshotNow, META_TABLE_COL_OID, 0, 0, 0);
	index_rescan(indscan, key, META_TABLE_COL_OID, NULL, 0);
	HeapTuple tuple = NULL;
	unsigned int index_id = 0;
	unsigned int type = 0;
	indinfo.index_num = 0;
	//Init colinfo.index_array、colinfo.index_info.
	//Allocate 3 size.If overflow later,than allocate
	//3 size each time.
	indinfo.index_array = (unsigned int *)palloc(sizeof(unsigned int) * 3);
	indinfo.type = (unsigned int *)palloc(sizeof(unsigned int) * 3);
	indinfo.index_info = (Colinfo*)palloc(sizeof(Colinfo) * 3);
	int max_len = 3;
	bool found = false;
	while((tuple = index_getnext(indscan, ForwardScanDirection)) != NULL)
	{
		found = true;
		Form_meta_table form_meta = (Form_meta_table)fxdb_tuple_to_chars(tuple);

		index_id = form_meta->rel_id;
		type = form_meta->type;

		//when overflow
		if(indinfo.index_num != 0 && indinfo.index_num % 3 == 0)
		{
			//init new unsigned int* and Colinfo*
			unsigned int *tmp_int = (unsigned int *)
				palloc(sizeof(unsigned int) * (max_len + 3));
			Colinfo *tmp_colinfo = (Colinfo*)
				palloc(sizeof(Colinfo) * (max_len + 3));
			unsigned int *tmp_type = (unsigned int *)palloc(sizeof(unsigned int) *(max_len + 3));
			//copy index_array to new memory
			memcpy(tmp_int, indinfo.index_array, sizeof(unsigned int) * indinfo.index_num);
			//copy index_info to new memory
			memcpy(tmp_colinfo, indinfo.index_info, sizeof(Colinfo) * indinfo.index_num);
			
			memcpy(tmp_type, indinfo.type, sizeof(unsigned int) * indinfo.index_num);

			max_len += 3;//get max len
			//free old unsigned int *
			pfree(indinfo.index_array);
			pfree(indinfo.index_info);
			pfree(indinfo.type);
			//point to new memory
			indinfo.index_array = tmp_int;
			indinfo.index_info = tmp_colinfo;
			indinfo.type = tmp_type;
		}
		//fill colinfo.index_array
		memcpy(&indinfo.index_array[indinfo.index_num], &index_id, sizeof(index_id));

		memcpy(&indinfo.type[indinfo.index_num], &type, sizeof(type));
		unsigned int colinfo_id = 0;
		colinfo_id = form_meta->colinfo_id;
		Colinfo cii = getColInfo(colinfo_id);
		memcpy(&indinfo.index_info[indinfo.index_num], &cii, sizeof(Colinfo));
		++indinfo.index_num;
		memset(&index_id, 0, sizeof(index_id));
		pfree(form_meta);
	}
	//fill_relation_indinfo(table_id, indinfo);
	index_endscan(indscan);
	index_close(indrel, RowExclusiveLock);
	heap_close(rel, RowExclusiveLock);
	if (!found) {
		pfree(indinfo.index_array);
		pfree(indinfo.index_info);
		pfree(indinfo.type);
	}
	return found;
}

bool fxdb_get_LsmIndexInfo(Oid lsmIdxId, IndinfoData& subIdxInfo)
{
	return fxdb_get_indexInfo(lsmIdxId, subIdxInfo);
}

bool isMetaTable(Oid relid)
{
	if(relid == MetaTableId
	   || relid == MetaTableColFirstIndex
	   || relid == MetaTableColThirdIndex)
	{
        return true;
	}

	return false;
}

bool IsSystemTable(Oid relid)
{
    return ((relid >= MinMetaId) && (relid <= MaxMetaId));
}

bool fxdb_get_mete_info(Oid object_id,MtInfo mt)
{
	MemoryContext oldctx = MemoryContextSwitchTo(CacheMemoryContext);
	memset(mt, 0 ,sizeof(MtInfoData));
		
	bool flag = false;
	flag = fxdb_get_heapInfo(object_id, mt);
	if (!flag)
	{
		if(NULL !=mt->tableColInfo)
		{
			if (NULL != mt->tableColInfo->col_number)
			{
				pfree(mt->tableColInfo->col_number);
				mt->tableColInfo->col_number = NULL;
			}
			if (NULL != mt->tableColInfo->rd_comfunction)
			{
				pfree(mt->tableColInfo->rd_comfunction);
				mt->tableColInfo->rd_comfunction = NULL;
			}
			pfree(mt->tableColInfo);
			mt->tableColInfo = NULL;
		}
	} 
	else
	{
		if (mt->parentId != 0)
		{ //index relation
			if(mt->type != LSM_TYPE)
			{
				memset(&mt->indinfo,0,sizeof(IndinfoData));
			}
			else
			{
				///get index info for lsm index
				flag = false;
				flag = fxdb_get_LsmIndexInfo(object_id, mt->indinfo);
				if (!flag)
				{ //no lsm subindex
					memset(&mt->indinfo,0,sizeof(IndinfoData));
					flag = true;
				}
			}
		}
		else
		{
			///get index info for heap relation
			flag = false;
			flag = fxdb_get_indexInfo(object_id, mt->indinfo);
			if (!flag)
			{ //no index
				memset(&mt->indinfo,0,sizeof(IndinfoData));
				flag = true;
			}
		}
	}
	MemoryContextSwitchTo(oldctx);
	return flag;
}

static
void build_meta_table_mtinfo(MtInfoData &mt)
{
	MtInfo_GetIndexCount(mt) = 2;
	MtInfo_GetIndexOid(mt) = (Oid *) palloc0 (sizeof(Oid) * 2);
	MtInfo_GetIndexOid(mt)[0] = MetaTableColFirstIndex;
	MtInfo_GetIndexOid(mt)[1] = MetaTableColThirdIndex;
	MtInfo_GetIndexType(mt) = (unsigned int *) palloc0 (sizeof(unsigned int) * 2);
	MtInfo_GetIndexType(mt)[0] = BTREE_UNIQUE_TYPE;
	MtInfo_GetIndexType(mt)[1] = BTREE_TYPE;
	MtInfo_GetIndexColinfo(mt) = (Colinfo *) palloc0 (sizeof(ColinfoData) * 2);
	Colinfo info = getColInfo(MetaTableColFirstIndex);
	memcpy(&MtInfo_GetIndexColinfo(mt)[0], &info, sizeof(Colinfo));
	info = getColInfo(MetaTableColThirdIndex);
	memcpy(&MtInfo_GetIndexColinfo(mt)[1], &info, sizeof(Colinfo));
	MtInfo_GetUserData(mt) = NULL;
}

bool fxdb_get_metatable_info(Oid tableid,IndexType& indexType,char& relkind,
														 Oid& tablespace,Colinfo& colinfo, MtInfoData &mt)
{
	tablespace = DEFAULTTABLESPACE_OID;

	switch(tableid)
	{
	case MetaTableId:
	{
			colinfo = getColInfo(tableid);
			indexType = UNKNOWN_TYPE;
			relkind = RELKIND_RELATION;
			build_meta_table_mtinfo(mt);
			return true;
	}
	case MetaLargeObjId:
	case LargeObjDataId:
	case DatabaseRelationId:
	case TableSpaceRelationId:
	case LargeObjHeapId:
		{
			colinfo = getColInfo(tableid);
			indexType = UNKNOWN_TYPE;
			relkind = RELKIND_RELATION;
			return true;
		}
	case MetaTableColFirstIndex:
	case  MetaLargeObjIndexColLOID:
	case  MetaLargeObjIndexColLONAME:
	case  LargeObjDataIndex:
	case TableSpaceOidIdxId:
	case LargeObjHeapIndex:
		{
			colinfo = getColInfo(tableid);
			indexType = BTREE_UNIQUE_TYPE;
			relkind = RELKIND_INDEX;
			return true;
		}
	case MetaTableColThirdIndex:
	case TableSpaceNameIdxId:
		{
			colinfo = getColInfo(MetaTableColThirdIndex);
			indexType = BTREE_TYPE;
			relkind = RELKIND_INDEX;
			return true;
		}
	case LargeObjHeapTbcIndex:
		{
			colinfo = getColInfo(tableid);
			indexType = BTREE_TYPE;
			relkind = RELKIND_INDEX;
			return true;
		}
	}
	
	return false;
}

void releaseRangeDataMemory(RangeData*rd)
{
    if (rd->ownMem) {
        pfree(rd->items);
        pfree(rd->itemSpace);
    }
}

void fxdb_MetaTableColDbIdScankeyInit(ScanKey entry, Oid db_id,
	StrategyNumber strategy)
{
	Datum datumOid = fdxdb_string_formdatum((char*)&db_id, sizeof(Oid));
	Fdxdb_ScanKeyInitWithCallbackInfo(entry
		,META_TABLE_COL_DB_ID
		,strategy
		,relid_compare
		,datumOid);
}

void fxdb_MetaTableColTypeScankeyInit(ScanKey entry, unsigned int type,
	StrategyNumber strategy)
{
	Datum datumType = fdxdb_string_formdatum((char*)&type, sizeof(unsigned int));
	Fdxdb_ScanKeyInitWithCallbackInfo(entry
		,META_TABLE_COL_TYPE
		,strategy
		,int_compare
		,datumType);
}

HeapTuple fxdb_GetMetaTableTupleCopy(Relation rel, Oid relid)
{
	//open MetaTableColFirstIndex
	Relation indrel = index_open(MetaTableColFirstIndex, AccessShareLock);
	Datum datumOid = fdxdb_string_formdatum((char*)&relid, sizeof(Oid));

	ScanKeyData key;
	Fdxdb_ScanKeyInitWithCallbackInfo(&key,
		1,
		BTEqualStrategyNumber,
		relid_compare,
		datumOid);

	IndexScanDesc indscan = index_beginscan(rel, indrel, SnapshotNow, 1, 0, 0, 0);
	index_rescan(indscan, &key, 1, NULL, 0);

	HeapTuple tup = index_getnext(indscan, ForwardScanDirection);
	HeapTuple ctup = heap_copytuple(tup);

	index_endscan(indscan);
	index_close(indrel, AccessShareLock);
	return ctup;
}

//return 0 if not found the index on table_id
//bool fxdb_get_index_info(Oid table_id, IndinfoData& indinfo)
//{
//	Assert(table_id > InvalidOid);
//	memset(&indinfo, 0, sizeof(IndinfoData));
//	Relation rel = NULL;
//	Relation indrel = NULL;
//	MetaTable mt;
//	int is_null = meta_info_is_null(table_id, &mt);
//	if(!is_null && NULL != mt && NULL != mt->indinfo){
//		//copy mt to indinfo
//		copy_indinfo(*mt->indinfo, indinfo);
//		return true;
//	}
//	rel = heap_open(MetaTableId,
//		RowExclusiveLock);
//
//	indrel = index_open(MetaTableColThirdIndex,
//		RowExclusiveLock);
//	char compare_data[4];//compare string
//	memcpy(compare_data, &table_id, sizeof(table_id));
//	Datum *value = (Datum*)palloc(sizeof(Datum));
//	value[0] = fdxdb_string_formdatum(compare_data, sizeof(compare_data));
//	ScanKeyData key[1];
//	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], META_TABLE_COL_OID, BTEqualStrategyNumber, relid_compare, value[0]);
//	IndexScanDesc indscan = index_beginscan(rel, indrel, SnapshotNow, META_TABLE_COL_OID, 0);
//	index_rescan(indscan, key, META_TABLE_COL_OID, NULL, 0);
//	HeapTuple tuple = NULL;
//	char *tmp_data = NULL;
//	unsigned int index_id = 0;
//	indinfo.index_num = 0;
//	//Init colinfo.index_array、colinfo.index_info.
//	//Allocate 3 size.If overflow later,than allocate
//	//3 size each time.
//	indinfo.index_array = (unsigned int *)palloc(sizeof(unsigned int) * 3);
//	indinfo.index_info = (Colinfo*)palloc(sizeof(Colinfo) * 3);
//	int max_len = 3;
//	RangeData rd;
//	bool found = false;
//	while((tuple = index_getnext(indscan, ForwardScanDirection)) != NULL)
//	{
//		found = true;
//		tmp_data = fxdb_tuple_to_chars(tuple);
//		//get col 1
//		rd = meta_item_split(tmp_data, META_TABLE_COL_OID);
//		memcpy(&index_id, &tmp_data[rd.start], rd.len);
//		//when overflow
//		if(indinfo.index_num != 0 && indinfo.index_num % 3 == 0)
//		{
//			//init new unsigned int* and Colinfo*
//			unsigned int *tmp_int = (unsigned int *)
//				palloc(sizeof(unsigned int) * (max_len + 3));
//			Colinfo *tmp_colinfo = (Colinfo*)
//				palloc(sizeof(Colinfo) * (max_len + 3));
//			//copy index_array to new memory
//			memcpy(tmp_int, indinfo.index_array, sizeof(unsigned int) * indinfo.index_num);
//			//copy index_info to new memory
//			memcpy(tmp_colinfo, indinfo.index_info, sizeof(Colinfo) * indinfo.index_num);
//
//			max_len += 3;//get max len
//			//free old unsigned int *
//			pfree(indinfo.index_array);
//			pfree(indinfo.index_info);
//			//point to new memory
//			indinfo.index_array = tmp_int;
//			indinfo.index_info = tmp_colinfo;
//		}
//		//fill colinfo.index_array
//		memcpy(&indinfo.index_array[indinfo.index_num], &index_id, sizeof(index_id));
//		//move to col 8 and get	Colum_info
//		rd = meta_item_split(tmp_data, META_TABLE_COL_COLINFO_ID);
//		unsigned int colinfo_id = 0;
//		memcpy(&colinfo_id, &tmp_data[rd.start], rd.len);
//		Colinfo cii = getColInfo(colinfo_id);
//		memcpy(&indinfo.index_info[indinfo.index_num], &cii, sizeof(Colinfo));
//		++indinfo.index_num;
//		memset(&index_id, 0, sizeof(index_id));
//		pfree(tmp_data);
//	}
//	fill_relation_indinfo(table_id, indinfo);
//	index_endscan(indscan);
//	pfree(DatumGetPointer(value[0]));
//	index_close(indrel, RowExclusiveLock);
//	heap_close(rel, RowExclusiveLock);
//	return found;
//}