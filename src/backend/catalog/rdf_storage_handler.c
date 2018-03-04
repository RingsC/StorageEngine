/*-------------------------------------------------------------------------
 *
 * rdf_storage_handler.c
 *		处理图数据库的操作请求
 *
 *
 * IDENTIFICATION
 *	  src/backend/catalog/rdf_storage_handler.c
 *
 *-------------------------------------------------------------------------
 */
#ifdef FOUNDER_XDB_SE

#include "postgres.h"

#include "utils/rel.h"
#include "utils/relcache.h"
#include "catalog/rdf_storage_handler.h"
#include "catalog/pg_database.h"
#include "access/xact.h"
#include "storage/proc.h"

void setColInfo(Oid colid, Colinfo pcol_info);
size_t CounterHeapSize(Oid relid);
int int_compare(const char* a, size_t len1, const char* b, size_t len2);
int relid_compare(const char* a, size_t len1, const char* b, size_t len2);
static void node_free(nodes *node);

#define StorageResultsMemoryBytes 1024 *10 /* KBytes */
#define XactNodeCacheSize 1024*10
#define XactNodeIDCacheSize 1024 * 1

#define XactNodeCache CurrentTransactionState->mTopLevelState->mNodeCache
#define XactNodeIDCache CurrentTransactionState->mTopLevelState->mNodeIDCache

extern MemoryContext ProcessTopMemoryContext;

typedef struct  
{
	uint64 id;
	char *content;
} NodeInfo;

typedef struct
{
	uint64 sid;
	uint64 pid;
	uint64 oid;
} StatementID;

/* 打印错误信息 */
static 
void rdf_error_print()
{
	int errCode = 0;
	int level = get_exception_level();
	char* errMsg = get_errno_errmsg(errCode);

	ereport(WARNING, (errmsg("%s\n", errMsg)));
	FlushErrorState();
}

/*
 * rdf_statement_split
 *		每个statement表的分裂函数
 *
 *@列1: subject		uint64
 *@列2: predicate uint64
 *@列3:	object		uint64
 *@列4:	context		uint64
 */
static
void rdf_statement_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	switch (col)
	{
		case RDF_STATEMENTMETA_TABLE_COL_SUBJECT:
		case RDF_STATEMENTMETA_TABLE_COL_PREDICATE:
		case RDF_STATEMENTMETA_TABLE_COL_OBJECT:
		case RDF_STATEMENTMETA_TABLE_COL_CONTEXT:
			rd.start = sizeof(uint64) * (col - 1);
			rd.len = sizeof(uint64);
			break;
		case RDF_STATEMENTMETA_TABLE_COL_INFERENCE:
			rd.start = sizeof(uint64) * (col - 1);
			rd.len = sizeof(int);
			break;
		default:
			Assert(0);	//余下的字段不会建立索引，所以这里忽略掉
	}
}

/*
 * rdf_models_split
 *		models表的分裂函数
 *
 *@列1: id		uint64
 *@列2: stid	Oid
 *@列3:	name	NameData
 */
static
void rdf_models_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	if (col == RDFMETA_TABLE_COL_ID)
	{
		rd.start = 0;
		rd.len = sizeof(uint64);
		return;
	} else {
		rdf_models_split(rd, str, col - 1, len);
		rd.start += rd.len;
	}

	switch (col)
	{
		case RDFMETA_TABLE_COL_STID:
		case RDFMETA_TABLE_COL_PO_INDEX_ID:
		case RDFMETA_TABLE_COL_OS_INDEX_ID:
		case RDFMETA_TABLE_COL_SPO_INDEX_ID:
		case RDFMETA_TABLE_COL_INF_INDEX_ID:
		case RDFMETA_TABLE_COL_DBID:
			rd.len = sizeof(Oid);
			break;
		case RDFMETA_TABLE_COL_NAME:
			rd.len = sizeof(NameData);
			break;
		default:
			Assert(0);//无法到达的代码块
	}
}

static
void rdf_statement_index_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	switch(col)
	{
	case 1:
	case 2:
	case 3:
		rd.start = (col - 1) * sizeof(uint64);
		rd.len = sizeof(uint64);
		break;
	case 4:
		rd.start = (col - 1) * sizeof(uint64);
		rd.len = sizeof(int);
	}
}

static
void rdf_statement_index_spo_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	switch(col)
	{
	case 1:
	case 2:
	case 3:
	case 4:
		rd.start = (col - 1) * sizeof(uint64);
		rd.len = sizeof(uint64);
		break;
	case 5:
		rd.start = (col - 1) * sizeof(uint64);
		rd.len = sizeof(int);
	}
}

static
void rdf_statement_index_inf_id_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	rd.start = 0;
	rd.len = sizeof(int);
}

static
void coln_uint64_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	rd.start = (col - 1) * sizeof(uint64);
	rd.len = sizeof(uint64);
}

static
void rdf_model_index_split(RangeData& rd, const char *str, int col, size_t len = 0)
{
	if (col == 1)
	{
		rd.start = (col - 1) * sizeof(uint64);
		rd.len = sizeof(uint64);
	} else if (col == 2)
	{
		rd.start = (col - 1) * sizeof(uint64);
		rd.len = sizeof(Oid);
	}
}

static
int uint64_compare(const char* a, size_t len1, const char* b, size_t len2)
{
	if (*(uint64*)a > *(uint64*)b)
		return 1;
	else if(*(uint64*)a < *(uint64*)b)
		return -1;
	else
		return 0;
}
static
int uint64_compare_hash(const void* a, const void* b, Size len)
{
	return (int)(*(uint64*)a - *(uint64*)b);
}

static
int statement_id_cmpfunc(const void* a, const void* b, Size len)
{
	StatementID *sa, *sb;
	sa = (StatementID *)a;
	sb = (StatementID *)b;

	if (sa->sid < sb->sid)
		return -1;
	else if (sa->sid > sb->sid)
		return 1;

	/* sa->sid == sb-> sid */
	if (sa->pid < sb->pid)
		return -1;
	else if (sa->pid > sb->pid)
		return 1;

	/* sa->pid == sb->pid */
	if (sa->oid < sb->oid)
		return -1;
	else if (sa->oid > sb->oid)
		return 1;

	/* sa->oid == sb->oid */
	return 0;
}

static
void init_rdf_models_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(ProcessTopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc0(sizeof(ColinfoData));
	colinfo->keys = RDFMETA_TABLE_COL_NUM;
	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;
	colinfo->split_function = rdf_models_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(RDF_Meta_Models, colinfo);
}

/*
 * init_rdf_models_index_colinfo
 *		建立在models表id列上的索引
 */
static
void init_rdf_models_index_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(ProcessTopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc0(sizeof(ColinfoData));
	colinfo->keys = 2;
	colinfo->col_number = (size_t *)palloc0(sizeof(size_t) * colinfo->keys);
	colinfo->col_number[0] = RDFMETA_TABLE_COL_ID;
	colinfo->col_number[1] = RDFMETA_TABLE_COL_DBID;
	colinfo->rd_comfunction = (CompareCallback *)palloc0(sizeof(CompareCallback) * colinfo->keys);
	colinfo->rd_comfunction[0] = uint64_compare;
	colinfo->rd_comfunction[1] = relid_compare;
	colinfo->split_function = rdf_model_index_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(RDF_Meta_Models_Index_ID, colinfo);
}

/*
 * init_rdf_rbl_colinfo
 *		初始化RBL表的colinfo
 *	
 *	RBL表存放三元组的内容，只需要在表的第一列（ID列）上建一个索引即可。
 *	所以RBL表的分裂函数只处理第一列。
 */
static
void init_rdf_rbl_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(ProcessTopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc0(sizeof(ColinfoData));
	colinfo->keys = RDFMETA_RBL_COL_NUM;
	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;
	colinfo->split_function = coln_uint64_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(RDF_Meta_RBL, colinfo);
}

/*
 * init_rdf_rbl_index_colinfo
 *		建立在RBL表id列上的索引
 */
static
void init_rdf_rbl_index_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(ProcessTopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc0(sizeof(ColinfoData));
	colinfo->keys = RDFMETA_RBL_COL_ID;
	colinfo->col_number = (size_t *)palloc0(sizeof(size_t) * RDFMETA_RBL_COL_ID);
	colinfo->col_number[0] = RDFMETA_RBL_COL_ID;
	colinfo->rd_comfunction = (CompareCallback *)palloc0(sizeof(CompareCallback) * RDFMETA_RBL_COL_ID);
	colinfo->rd_comfunction[0] = uint64_compare;
	colinfo->split_function = coln_uint64_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(RDF_Meta_RBL_Index_ID, colinfo);
}

static
void init_rdf_statements_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(ProcessTopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc0(sizeof(ColinfoData));
	colinfo->keys = RDF_STATEMENTMETA_TABLE_COL_NUM;
	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;
	colinfo->split_function = rdf_statement_split;
	MemoryContextSwitchTo(ocxt);
	setColInfo(RDF_Meta_Statements_COLID, colinfo);
}

static
void init_rdf_statements_index_colinfo()
{
	MemoryContext ocxt = MemoryContextSwitchTo(ProcessTopMemoryContext);
	Colinfo colinfo = (Colinfo)palloc0(sizeof(ColinfoData));

	/* _po index */
	colinfo = (Colinfo)palloc0(sizeof(ColinfoData));
	colinfo->keys = 4;
	colinfo->col_number = (size_t *)palloc0(sizeof(size_t) * colinfo->keys);
	colinfo->col_number[0] = RDF_STATEMENTMETA_TABLE_COL_PREDICATE;
	colinfo->col_number[1] = RDF_STATEMENTMETA_TABLE_COL_OBJECT;
	colinfo->col_number[2] = RDF_STATEMENTMETA_TABLE_COL_CONTEXT;
	colinfo->col_number[3] = RDF_STATEMENTMETA_TABLE_COL_INFERENCE;
	colinfo->rd_comfunction = (CompareCallback *)palloc0(sizeof(CompareCallback) * colinfo->keys);
	colinfo->rd_comfunction[0] = uint64_compare;
	colinfo->rd_comfunction[1] = uint64_compare;
	colinfo->rd_comfunction[2] = uint64_compare;
	colinfo->rd_comfunction[3] = int_compare;
	colinfo->split_function = rdf_statement_index_split;
	setColInfo(RDF_Meta_Statements_Index_PO_COLID, colinfo);

	/* 
	 * o_s index
	 */
	colinfo = (Colinfo)palloc0(sizeof(ColinfoData));
	colinfo->keys = 4;
	colinfo->col_number = (size_t *)palloc0(sizeof(size_t) * colinfo->keys);
	colinfo->col_number[0] = RDF_STATEMENTMETA_TABLE_COL_OBJECT;
	colinfo->col_number[1] = RDF_STATEMENTMETA_TABLE_COL_SUBJECT;
	colinfo->col_number[2] = RDF_STATEMENTMETA_TABLE_COL_CONTEXT;
	colinfo->col_number[3] = RDF_STATEMENTMETA_TABLE_COL_INFERENCE;
	colinfo->rd_comfunction = (CompareCallback *)palloc0(sizeof(CompareCallback) * colinfo->keys);
	colinfo->rd_comfunction[0] = uint64_compare;
	colinfo->rd_comfunction[1] = uint64_compare;
	colinfo->rd_comfunction[2] = uint64_compare;
	colinfo->rd_comfunction[3] = int_compare;
	colinfo->split_function = rdf_statement_index_split;
	setColInfo(RDF_Meta_Statements_Index_OS_COLID, colinfo);

	/* spo index */
	colinfo = (Colinfo)palloc0(sizeof(ColinfoData));
	colinfo->keys = 5;
	colinfo->col_number = (size_t *)palloc0(sizeof(size_t) * colinfo->keys);
	colinfo->col_number[0] = RDF_STATEMENTMETA_TABLE_COL_SUBJECT;
	colinfo->col_number[1] = RDF_STATEMENTMETA_TABLE_COL_PREDICATE;
	colinfo->col_number[2] = RDF_STATEMENTMETA_TABLE_COL_OBJECT;
	colinfo->col_number[3] = RDF_STATEMENTMETA_TABLE_COL_CONTEXT;
	colinfo->col_number[4] = RDF_STATEMENTMETA_TABLE_COL_INFERENCE;
	colinfo->rd_comfunction = (CompareCallback *)palloc0(sizeof(CompareCallback) * colinfo->keys);
	colinfo->rd_comfunction[0] = uint64_compare;
	colinfo->rd_comfunction[1] = uint64_compare;
	colinfo->rd_comfunction[2] = uint64_compare;
	colinfo->rd_comfunction[3] = uint64_compare;
	colinfo->rd_comfunction[4] = int_compare;
	colinfo->split_function = rdf_statement_index_spo_split;
	setColInfo(RDF_Meta_Statements_Index_SPO_COLID, colinfo);

	/* inf id index */
	colinfo = (Colinfo)palloc0(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t *)palloc0(sizeof(size_t) * colinfo->keys);
	colinfo->col_number[0] = RDF_STATEMENTMETA_TABLE_COL_INFERENCE;
	colinfo->rd_comfunction = (CompareCallback *)palloc0(sizeof(CompareCallback) * colinfo->keys);
	colinfo->rd_comfunction[0] = int_compare;
	colinfo->split_function = rdf_statement_index_inf_id_split;
	setColInfo(RDF_Meta_Statements_Index_INF_ID_COLID, colinfo);

	MemoryContextSwitchTo(ocxt);
}

/*
 * create_rdf_models_table
 *		创建Models的元数据表
 */
static
void create_rdf_models_table()
{
	fxdb_heap_create(RDF_Meta_Models, RDF_Meta_Models);

	Relation rel = heap_open(RDF_Meta_Models, ShareLock);
	fxdb_index_create(RDF_Meta_Models_Index_ID, rel, BTREE_UNIQUE_TYPE, RDF_Meta_Models_Index_ID);
	heap_close(rel, NoLock);
}

/*
 * create_rbl_table
 *		创建存放resource、literal和bnode内容的元数据表
 */
static
void create_rbl_table()
{
	fxdb_heap_create(RDF_Meta_RBL, RDF_Meta_RBL);

	Relation rel = heap_open(RDF_Meta_RBL, ShareLock);
	fxdb_index_create(RDF_Meta_RBL_Index_ID, rel, BTREE_UNIQUE_TYPE, RDF_Meta_RBL_Index_ID);
	heap_close(rel, NoLock);
}

/*
 * CreateRDFTable
 *		创建RDF元数据表
 */
void CreateRDFTable()
{
	create_rdf_models_table();
	create_rbl_table();
}

/*
 * storage_exists
 *		判断一个storage是否已经存在。id和name任意为空，优先
 *		使用id去查找。
 *
 *@id: storage的标识
 *@name: storage的名字
 *@storage: 传出参数
 */
static
bool storage_exists(uint64 id, const char *name, RDFMeta_Models storage)
{
	bool statu = false;
	if (id == 0 && name == NULL)
		return statu;

	Relation rel = heap_open(RDF_Meta_Models, AccessShareLock);
	HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);
	HeapTuple tup = NULL;

	while ((tup = heap_getnext(scan, BackwardScanDirection)) != NULL)
	{
		RDFMeta_Models formClass = (RDFMeta_Models)fxdb_tuple_to_chars(tup);
		if (id != 0)
		{
			if (formClass->id == id && formClass->db_id == MyDatabaseId)
			{
				statu = true;
			}
		} else {
			if (strcmp(formClass->name.data, name) == 0 && formClass->db_id == MyDatabaseId)
			{
				statu = true;
			}
		}

		if (statu)
		{
			if (storage)
			{
				memcpy(storage, formClass, sizeof(RDFMeta_Models_Data));
			}
			pfree(formClass);
			break;
		}
		pfree(formClass);
	}

	heap_endscan(scan);
	heap_close(rel, AccessShareLock);

	return statu;
}

/*
 * RDFCreateStatementsTable
 *		创建statement表，并将元数据信息插入到models表中
 *
 *@id: storage的唯一标识
 *@name: storage名
 *@storage: 传出参数
 */
void RDFCreateStatementsTable(uint64 id, const char *name,  RDFMeta_Models storage)
{
		RDFMeta_Models_Data formClass;
		if (storage_exists(id, name, storage))
		{
			ereport(ERROR, 
				(errmsg("storage %s already exists.", name)));
			return;
		}

		/* Create statementxxx */
		Oid relid = fxdb_heap_create(InvalidOid, RDF_Meta_Statements_COLID, DEFAULTTABLESPACE_OID, MyDatabaseId);

		/* Create statementxxx index */
		Relation rel = heap_open(relid, ShareLock);
		formClass.index_po_id = fxdb_index_create(InvalidOid, rel, BTREE_TYPE, RDF_Meta_Statements_Index_PO_COLID);
		formClass.index_os_id = fxdb_index_create(InvalidOid, rel, BTREE_TYPE, RDF_Meta_Statements_Index_OS_COLID);
		/* spo索引不需要唯一，因为插入三元组之前需要调用索引扫描来检查唯一性 */
		formClass.index_spo_id = fxdb_index_create(InvalidOid, rel, BTREE_UNIQUE_TYPE, RDF_Meta_Statements_Index_SPO_COLID);
		formClass.index_inf_id = fxdb_index_create(InvalidOid, rel, BTREE_TYPE, RDF_Meta_Statements_Index_INF_ID_COLID);
		formClass.db_id = MyDatabaseId;
		heap_close(rel, NoLock);

		formClass.id = id;
		formClass.stid = relid;
		memset(formClass.name.data, 0, NAMEDATALEN);
		strcpy(formClass.name.data, name);

		rel = heap_open(RDF_Meta_Models, RowExclusiveLock);
		HeapTuple tup = fdxdb_heap_formtuple((char*)(&formClass), sizeof(formClass));
		simple_heap_insert(rel, tup);
		pfree(tup);
		heap_close(rel, RowExclusiveLock);

		memcpy(storage, &formClass, sizeof(formClass));
}

static
bool is_one_nodes_search(RDF_Statements statement)
{
	if (statement->subject_id != 0 && statement->predicate_id == 0 && 
			statement->object_id == 0 && statement->inf_id < 0)
		return true;

	if (statement->subject_id == 0 && statement->predicate_id != 0 && 
			statement->object_id == 0&& statement->inf_id < 0)
		return true;

	if (statement->subject_id == 0 && statement->predicate_id == 0 && 
			statement->object_id != 0 && statement->inf_id < 0)
		return true;

	if (statement->subject_id == 0 && statement->predicate_id == 0 && 
		statement->object_id == 0 && statement->inf_id >= 0)
		return true;

	return false;
}

static
void statement_init_o_s_scankey(RDF_Statements statement, int *key_size, ScanKey key)
{
	Datum values, values2, values3, values4;

	values = fdxdb_string_formdatum((char*)&statement->subject_id, sizeof(statement->subject_id));
	Fdxdb_ScanKeyInitWithCallbackInfo(
		&key[1],
		2,
		BTEqualStrategyNumber,
		uint64_compare, values);

	values2 = fdxdb_string_formdatum((char*)&statement->object_id, sizeof(statement->object_id));
	Fdxdb_ScanKeyInitWithCallbackInfo(
		&key[0],
		1,
		BTEqualStrategyNumber,
		uint64_compare, values2);

	values3 = fdxdb_string_formdatum((char*)&statement->context, sizeof(statement->context));
	Fdxdb_ScanKeyInitWithCallbackInfo(
		&key[2],
		3,
		BTEqualStrategyNumber,
		uint64_compare, values3);

	*key_size = 3;

	if (statement->inf_id >= 0)
	{
		values4 = fdxdb_string_formdatum((char*)&statement->inf_id, sizeof(statement->inf_id));
		Fdxdb_ScanKeyInitWithCallbackInfo(
			&key[3],
			3,
			BTEqualStrategyNumber,
			int_compare, values4);
		++(*key_size);
	}
}

static 
bool statement_is_null(RDF_Statements statement)
{
	if (statement->subject_id == 0 && statement->predicate_id == 0 &&
		statement->object_id == 0 && statement->inf_id < 0)
		return true;

	return false;
}

/*
 * statement_init_scankey
 *		根据查找的statement内容来决定比较条件
 *		[sp_  o_s  _po  s__  _p_  __o  spo  inf_id] 
 */
static
ScanKey statement_init_scankey(RDF_Statements statement, int *key_size)
{
	bool use_context = false;
	ScanKey key = (ScanKey)palloc0(sizeof(ScanKeyData) * 5);

	// o_s索引需要反向构造
	if (statement->subject_id != 0 && statement->predicate_id == 0 && statement->object_id != 0)
	{
		statement_init_o_s_scankey(statement, key_size, key);
		return key;
	}

	//subject not null
	if (statement->subject_id != 0)
	{
		Datum values1;
		AttrNumber number;
		++(*key_size);

		if (is_one_nodes_search(statement))
			number = 1;
		else
			number = *key_size;

		values1 = fdxdb_string_formdatum((char*)&statement->subject_id, sizeof(statement->subject_id));
		Fdxdb_ScanKeyInitWithCallbackInfo(
			&key[(*key_size) - 1],
			number,
			BTEqualStrategyNumber,
			uint64_compare, values1);
	}

	//predicate not null
	if (statement->predicate_id != 0)
	{
		Datum values2;
		AttrNumber number;
		++(*key_size);

		if (is_one_nodes_search(statement))
			number = 1;
		else
			number = *key_size;

		values2 = fdxdb_string_formdatum((char*)&statement->predicate_id, sizeof(statement->predicate_id));
		Fdxdb_ScanKeyInitWithCallbackInfo(
			&key[(*key_size) - 1],
			number,
			BTEqualStrategyNumber, 
			uint64_compare, values2);
	}

	//object not null
	if (statement->object_id != 0)
	{
		Datum values3;
		AttrNumber number;
		++(*key_size);

		if (is_one_nodes_search(statement))
			number = 1;
		else
			number = *key_size;

		values3 = fdxdb_string_formdatum((char*)&statement->object_id, sizeof(statement->object_id));
		Fdxdb_ScanKeyInitWithCallbackInfo(
			&key[(*key_size) - 1],
			number,
			BTEqualStrategyNumber, 
			uint64_compare, values3);
	}

	/* 
	 * 这里这么判断的原因是避免出现以下的scankey非前缀构造形式 :
	 * s__c
	 * sp_c
	 * _p_c
	 * __oc
	 * ___c
	 * _poc
	 * s_oc
	 */
	if (!is_one_nodes_search(statement) && 
		!statement_is_null(statement) &&
		*key_size == 3)
	{
		Datum values4;
		AttrNumber number;
		++(*key_size);

		number = *key_size;

		values4 = fdxdb_string_formdatum((char*)&statement->context, sizeof(statement->context));
		Fdxdb_ScanKeyInitWithCallbackInfo(
			&key[(*key_size) - 1],
			number,
			BTEqualStrategyNumber, 
			uint64_compare, values4);
	}

	if (statement->inf_id >= 0)
	{
		Datum values5;
		AttrNumber number;
		++(*key_size);

		number = *key_size;

		values5 = fdxdb_string_formdatum((char*)&statement->inf_id, sizeof(statement->inf_id));
		Fdxdb_ScanKeyInitWithCallbackInfo(
			&key[(*key_size) - 1],
			number,
			BTEqualStrategyNumber, 
			int_compare, values5);
	}

	return key;
}

static
Oid get_scan_index_id(RDF_Statements statement, RDFMeta_Models storage)
{
	/* only inf id */
	if (statement->subject_id == 0 && statement->predicate_id == 0 && 
			statement->object_id == 0 && statement->inf_id >= 0)
			return storage->index_inf_id;

	/* spo */
	if (statement->subject_id != 0 && statement->predicate_id != 0 && statement->object_id != 0)
		return storage->index_spo_id;

	/* o_s */
	if (statement->subject_id != 0 && statement->object_id != 0)
		return storage->index_os_id;

	/* sp_ */
	if (statement->subject_id != 0 && statement->predicate_id != 0)
		return storage->index_spo_id;

	/* po */
	if (statement->predicate_id != 0 && statement->object_id != 0)
		return storage->index_po_id;

	/* s__ */
	if (statement->subject_id != 0)
		return storage->index_spo_id;

	/* _p_ */
	if (statement->predicate_id != 0)
		return storage->index_po_id;

	/* __o */
	if (statement->object_id != 0)
		return storage->index_os_id;

	/* ?s ?p ?o */
	return InvalidOid;
}

/*
 * statement_exists
 *		判断一个statement是否存在，使用s、p和o组合查询
 */
static
bool statement_exists(RDF_Statements statement, RDFMeta_Models storage, HeapTuple *ctuple)
{
	Assert(statement);

	bool statu = false;
	int key_size = 0;
	Oid idx_id;
	SysScanDesc sd;
	
	if (statement->subject_id == 0 && 
		statement->predicate_id == 0 && 
		statement->object_id == 0 &&
		statement->context == 0)
		return false;

	ScanKey key = statement_init_scankey(statement, &key_size);
	idx_id = get_scan_index_id(statement, storage);

	Relation rel = heap_open(storage->stid, AccessShareLock);
	Relation irel = index_open(idx_id, AccessShareLock);

	sd = systable_beginscan_ordered(rel, irel,
		SnapshotNow, key_size, key);

	HeapTuple tup = NULL;

	if ((tup =  systable_getnext_ordered(sd, ForwardScanDirection)) != NULL)
	{
		if (ctuple)
			*ctuple = heap_copytuple(tup);
		statu = true;
	}

	systable_endscan_ordered(sd);
	index_close(irel, NoLock);
	heap_close(rel, NoLock);
	pfree(key);

	return statu;
}

static
size_t rdf_literal_size(nodes *node)
{
	size_t lens = 0;
	lens += node->data.literal.value_lens + sizeof(node->data.literal.value_lens);
	lens += node->data.literal.language_lens + sizeof(node->data.literal.language_lens);
	lens += node->data.literal.data_type_lens + sizeof(node->data.literal.data_type_lens);

	return lens;
}

static
size_t rdf_resource_size(nodes *node)
{
	size_t lens = 0;
	lens += node->data.resource.uri_lens + sizeof(node->data.resource.uri_lens);

	return lens;
}

static
size_t rdf_bnode_size(nodes *node)
{
	size_t lens = 0;
	lens += node->data.bnode.name_lens + sizeof(node->data.bnode.name_lens);

	return lens;
}

static
size_t rdf_nodes_size(nodes *node)
{
	size_t tlens = 0;

	/* 根据type的不同，决定是literal、resource还是bnodes */
	switch (node->type)
	{
	case RDF_TYPE_LITERAL:
		tlens = rdf_literal_size(node);
		break;
	case RDF_TYPE_RESOURCE:
		tlens = rdf_resource_size(node);
		break;
	case RDF_TYPE_BNODES:
		tlens = rdf_bnode_size(node);
		break;
	default: //未知的类型
		Assert(0);
	}

	return tlens;
}

/*
 * rdf_copy_literal
 *		将literal结构体的数据拷贝到一段内存中返回
 *
 *@node: 拷贝的源node
 *@lens:	node数据长度
 */
static
char *rdf_copy_literal(nodes *node, size_t lens)
{
	char *rdata = (char*)palloc0(lens);
	char *pdata = rdata;

	memcpy(pdata, &node->data.literal.value_lens, sizeof(node->data.literal.value_lens));
	pdata += sizeof(node->data.literal.value_lens);
	memcpy(pdata, node->data.literal.values, node->data.literal.value_lens);
	pdata += node->data.literal.value_lens;
	memcpy(pdata, &node->data.literal.language_lens, sizeof(node->data.literal.language_lens));
	pdata += sizeof(node->data.literal.language_lens);
	memcpy(pdata, node->data.literal.language, node->data.literal.language_lens);
	pdata += node->data.literal.language_lens;
	memcpy(pdata, &node->data.literal.data_type_lens, sizeof(node->data.literal.data_type_lens));
	pdata += sizeof(node->data.literal.data_type_lens);
	memcpy(pdata, node->data.literal.data_type, node->data.literal.data_type_lens);
	pdata += node->data.literal.data_type_lens;

	Assert(pdata == (rdata + lens));

	return rdata;
}

/*
 * rdf_copy_resource
 *		将resource结构体的数据拷贝到一段内存中返回
 *
 *@node: 拷贝的源node
 *@lens:	statements数据长度
 */
static
char *rdf_copy_resource(nodes *node, size_t lens)
{
	char *rdata = (char*)palloc0(lens);
	char *pdata = rdata;

	memcpy(pdata, &node->data.resource.uri_lens, sizeof(node->data.resource.uri_lens));
	pdata += sizeof(node->data.resource.uri_lens);
	memcpy(pdata, node->data.resource.uri, node->data.resource.uri_lens);
	pdata += node->data.resource.uri_lens;

	Assert(pdata == (rdata + lens));

	return rdata;
}

/*
 * rdf_copy_bnode
 *		将bnode结构体的数据拷贝到一段内存中返回
 *
 *@node: 拷贝的源node
 *@lens:	statements数据长度
 */
static
char *rdf_copy_bnode(nodes *node, size_t lens)
{
	char *rdata = (char*)palloc0(lens);
	char *pdata = rdata;

	memcpy(pdata, &node->data.bnode.name_lens, sizeof(node->data.bnode.name_lens));
	pdata += sizeof(node->data.bnode.name_lens);
	memcpy(pdata, node->data.bnode.name, node->data.bnode.name_lens);
	pdata += node->data.bnode.name_lens;

	Assert(pdata == (rdata + lens));

	return rdata;
}

static
char *rdf_copy_nodes(nodes *node, size_t lens)
{
	char *tdata = NULL;

	/* 根据type的不同，决定是literal、resource还是bnodes */
	switch (node->type)
	{
	case RDF_TYPE_LITERAL:
		tdata = rdf_copy_literal(node, lens);
		break;
	case RDF_TYPE_RESOURCE:
		tdata = rdf_copy_resource(node, lens);
		break;
	case RDF_TYPE_BNODES:
		tdata = rdf_copy_bnode(node, lens);
		break;
	default: //未知的类型
		Assert(0);
	}

	return tdata;
}

static
void rdf_resource_from_data(char **data, nodes *node)
{
	memcpy(&node->data.resource.uri_lens, *data, sizeof(node->data.resource.uri_lens));
	*data += sizeof(node->data.resource.uri_lens);
	if (node->data.resource.uri_lens > 0)
	{
		node->data.resource.uri = (char *)palloc0(node->data.resource.uri_lens + 1);
		memcpy(node->data.resource.uri, *data, node->data.resource.uri_lens);
		node->data.resource.uri[node->data.resource.uri_lens] = '\0';
		*data += node->data.resource.uri_lens;
	}
}

static
void rdf_bnode_from_data(char **data, nodes *node)
{
	memcpy(&node->data.bnode.name_lens, *data, sizeof(node->data.bnode.name_lens));
	*data += sizeof(node->data.bnode.name_lens);
	if (node->data.bnode.name_lens > 0)
	{
		node->data.bnode.name = (char *)palloc0(node->data.bnode.name_lens + 1);
		memcpy(node->data.bnode.name, *data, node->data.bnode.name_lens);
		node->data.bnode.name[node->data.bnode.name_lens] = '\0';
		*data += node->data.bnode.name_lens;
	}
}

static
void rdf_literal_from_data(char **data, nodes *node)
{
	memcpy(&node->data.literal.value_lens, *data, sizeof(node->data.literal.value_lens));
	*data += sizeof(node->data.literal.value_lens);
	if (node->data.literal.value_lens > 0)
	{
		node->data.literal.values = (char *)palloc0(node->data.literal.value_lens + 1);
		memcpy(node->data.literal.values, *data, node->data.literal.value_lens);
		node->data.literal.values[node->data.literal.value_lens] = '\0';
		*data += node->data.literal.value_lens;
	}

	memcpy(&node->data.literal.language_lens, *data, sizeof(node->data.literal.language_lens));
	*data += sizeof(node->data.literal.language_lens);
	if (node->data.literal.language_lens > 0)
	{
		node->data.literal.language = (char *)palloc0(node->data.literal.language_lens + 1);
		memcpy(node->data.literal.language, *data, node->data.literal.language_lens);
		node->data.literal.language[node->data.literal.language_lens] = '\0';
		*data += node->data.literal.language_lens;
	}

	memcpy(&node->data.literal.data_type_lens, *data, sizeof(node->data.literal.data_type_lens));
	*data += sizeof(node->data.literal.data_type_lens);
	if (node->data.literal.data_type_lens > 0)
	{
		node->data.literal.data_type = (char *)palloc0(node->data.literal.data_type_lens + 1);
		memcpy(node->data.literal.data_type, *data, node->data.literal.data_type_lens);
		node->data.literal.data_type[node->data.literal.data_type_lens] = '\0';
		*data += node->data.literal.data_type_lens;
	}
}

static
nodes *rdf_type_copy_nodes(char **data, int type)
{
	nodes *node = NULL;
	node = (nodes *)palloc0(sizeof(nodes));
	node->type = type;

	switch(type)
	{
		case RDF_TYPE_LITERAL :
			rdf_literal_from_data(data, node);
			break;
		case RDF_TYPE_RESOURCE:
			rdf_resource_from_data(data, node);
			break;
		case RDF_TYPE_BNODES:
			rdf_bnode_from_data(data, node);
			break;
	}

	return node;
}

static
bool rdf_rbl_cache_exists(uint64 id, char **data)
{
	NodeInfo *ni = (NodeInfo *)hash_search(XactNodeCache, &id, HASH_FIND, NULL);
	if (ni == NULL)
		return false;
	else
	{
		if (data != NULL)
			*data = ni->content;
	}

	return true;
}

static
bool rdf_statement_id_cache_exists(uint64 sid, uint64 pid, uint64 oid)
{
	StatementID tstatement;
	bool found = false;

	tstatement.sid = sid;
	tstatement.pid = pid;
	tstatement.oid = oid;

	hash_search(XactNodeIDCache, &tstatement, HASH_FIND, &found);

	return found;
}

static
void rdf_rbl_cache_insert(uint64 id, char **data)
{
	bool found = false;
	NodeInfo *ni = (NodeInfo *)hash_search(XactNodeCache, &id, HASH_ENTER, &found);

	Assert(!found);

	ni->id = id;
	ni->content = *data;
}

static
void rdf_statement_cache_insert(uint64 sid, uint64 pid, uint64 oid)
{
	bool found = false;
	StatementID tstatement;
	tstatement.sid = sid;
	tstatement.pid = pid;
	tstatement.oid = oid;

	StatementID *statement = (StatementID *)hash_search(XactNodeIDCache, &tstatement, HASH_ENTER, &found);

	Assert(!found);

	statement->sid = sid;
	statement->pid = pid;
	statement->oid = oid;
}

bool rdf_rbl_exists(uint64 id, char **data)
{
	if (rdf_rbl_cache_exists(id, data))
		return true;

	bool ret = false;
	Relation rel = heap_open(RDF_Meta_RBL, AccessShareLock);
	Relation irel = index_open(RDF_Meta_RBL_Index_ID, AccessShareLock);
	ScanKeyData skey;
	SysScanDesc sd;

	Datum datum = fdxdb_string_formdatum((char *)(&id), sizeof(id));
	Fdxdb_ScanKeyInitWithCallbackInfo(&skey,
		RDFMETA_RBL_COL_ID,
		BTEqualStrategyNumber,
		uint64_compare,
		datum);

	sd = systable_beginscan_ordered(rel, irel,
		SnapshotNow, 1, &skey);

	HeapTuple tuple = systable_getnext_ordered(sd, ForwardScanDirection);

	if (tuple)
	{
		ret = true;
		char *tdata = fxdb_tuple_to_chars(tuple);
		rdf_rbl_cache_insert(id, &tdata);
		if (data)
			*data = tdata;
	}

	systable_endscan_ordered(sd);
	index_close(irel, NoLock);
	heap_close(rel, NoLock);

	return ret;
}

/*
 * RDFStatementCacheExists
 *		判断一个statement是否已经在id cache中
 *
 *@sid: subject id
 *@pid: predicate id
 *@oid: object id
 */
bool RDFStatementCacheExists(uint64 sid, uint64 pid, uint64 oid)
{
	if (!XactNodeIDCache)
		return false;

	return rdf_statement_id_cache_exists(sid, pid, oid);
}

/*
 * RDFStatementCacheExists
 *		判断一个nodes的id是否已经在表中存在
 *
 *@id: nodes_id
 *@data:	传出参数，可以为空
 */
bool RDFRBLExists(uint64 id, char **data)
{
	return rdf_rbl_exists(id, data);
}

/*
 * rdf_rbl_formdata_from_node
 *		由nodes返回一块data内存块，用于插入到rbl表中
 */
static
char *rdf_rbl_formdata_from_node(nodes *node, uint64 node_id, size_t *tlens)
{
	char *tdata = NULL, *data = NULL;
	size_t lens = 0;

	lens = rdf_nodes_size(node);
	tdata = rdf_copy_nodes(node, lens);
	*tlens = lens + sizeof(node_id) + sizeof(node->type);

	/* 数据长度包括内容长度、id类型长度和type类型长度 */
	data = (char *)palloc0(*tlens);
	char *pdata = data;
	memcpy(pdata, &node_id, sizeof(node_id));
	pdata += sizeof(node_id);
	memcpy(pdata, &node->type, sizeof(node->type));
	pdata += sizeof(node->type);
	memcpy(pdata, tdata, lens);
	pdata += lens;

	pfree(tdata);

	return data;
}

/*
 * RDFRBLCacheInsert
 *		往事务缓存中插入一个node
 *
 *@id: nodes_id
 *@node:	#nodes object
 */
void RDFRBLCacheInsert(uint64 id, nodes *node)
{
	Assert(id != 0 && node != NULL);
	
	size_t lens = 0;
	char *data = rdf_rbl_formdata_from_node(node, id, &lens);
	
	rdf_rbl_cache_insert(id, &data);
}

/*
 * InitXactRDFNodeIDCache
 *		初始化事务的节点id cache
 */
void InitXactRDFNodeIDCache()
{
	HASHCTL		ctl;

	/** Initialize the hash table. */
	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(StatementID);
	ctl.entrysize = sizeof(StatementID);
	/* hash style uint64  */
	ctl.hash = tag_hash;
	ctl.match = statement_id_cmpfunc;
	ctl.hcxt = TopTransactionContext;
	char cacheName[200];
	memset(cacheName, 0, 200);
	sprintf(cacheName, "Node ID Cache %u", CurrentTransactionState->transactionId);
 
	XactNodeIDCache = 
		hash_create(cacheName, XactNodeIDCacheSize, &ctl, HASH_ELEM | HASH_FUNCTION | HASH_COMPARE | HASH_CONTEXT);
}

/*
 * RDFStatementCacheInsert
 *		往事务缓存中插入一个statement
 *
 *@sid: subject id
 *@pid: predicate id
 *@oid: object id
 */
void RDFStatementCacheInsert(uint64 sid, uint64 pid, uint64 oid)
{
	Assert(sid != 0 && pid != 0 && oid != 0);

	if (XactNodeIDCache == NULL)
		InitXactRDFNodeIDCache();
	
	rdf_statement_cache_insert(sid, pid, oid);
}

/*
 * RDFStatementCacheDestroy
 *		销毁事务的id cache缓存
 */
void RDFStatementCacheDestroy()
{
	if (XactNodeIDCache)
		hash_destroy(XactNodeIDCache);

	XactNodeIDCache = NULL;
}

static
char *rdf_id_get_nodes(uint64 id)
{
	char *data = NULL;
	bool statu  = rdf_rbl_exists(id, &data);

	Assert(statu);

	/* 该data将保存在事务期间的缓存上，在事务结束的时候统一由事务释放 */
	return data;
}

static
nodes *rdf_nodes_from_id(uint64 id)
{
	int type;
	nodes *node = NULL;
	char *data = rdf_id_get_nodes(id);
	char *pdata = data;

	/* 第一列是id，跳过 */
	pdata += sizeof(id);

	/* 拷贝type */
	memcpy(&type, pdata, sizeof(type));
	pdata += sizeof(type);

	node = rdf_type_copy_nodes(&pdata, type);

	return node;
}

/*
 * rdf_statment_formtuple
 *		由一个statement构造一个HeapTuple返回
 *
 *@statement: 源statement
 */
static
HeapTuple rdf_statment_formtuple(RDF_Statements statement)
{
	HeapTuple rtup = NULL;

	size_t tup_lens = 0;

	/* 每个statement的前面四个元组是固定长度的 */
	tup_lens = sizeof(statement->subject_id) * RDF_STATEMENTMETA_TABLE_COL_CONTEXT;

	/* 加上statement的推理id长度 */
	tup_lens += sizeof(statement->inf_id);

	char *data = (char*)palloc0(tup_lens);
	char *pdata = data;

	/* 拷贝statement四个固定字段的列值 */
	memcpy(pdata, &statement->subject_id, sizeof(statement->subject_id));
	pdata += sizeof(statement->subject_id);
	memcpy(pdata, &statement->predicate_id, sizeof(statement->predicate_id));
	pdata += sizeof(statement->predicate_id);
	memcpy(pdata, &statement->object_id, sizeof(statement->object_id));
	pdata += sizeof(statement->object_id);
	memcpy(pdata, &statement->context, sizeof(statement->context));
	pdata += sizeof(statement->context);
	memcpy(pdata, &statement->inf_id, sizeof(statement->inf_id));
	pdata += sizeof(statement->inf_id);

	Assert(pdata == (data + tup_lens));

	rtup = fdxdb_heap_formtuple(data, tup_lens);
	pfree(data);

	return rtup;
}

/*
 * RDFRBLFormtuple
 *		同函数rdf_statment_formtuple
 *
 *@node: 源nodes
 *@node_id: node id
 *@cxt: HeapTuple分配在该cxt里
 */
HeapTuple RDFRBLFormtuple(nodes *node, uint64 node_id, MemoryContext cxt)
{
	size_t lens = 0;

	MemoryContext oldcxt = MemoryContextSwitchTo(cxt);
	char *tdata = rdf_rbl_formdata_from_node(node, node_id, &lens);
	HeapTuple tuple = fdxdb_heap_formtuple(tdata, lens);
	MemoryContextSwitchTo(oldcxt);

	pfree(tdata);

	return tuple;
}

/*
 * RDFStatmentFormtuple
 *		同函数rdf_statment_formtuple
 *
 *@statement: 源statement
 *@cxt: HeapTuple分配在cxt中
 */
HeapTuple RDFStatmentFormtuple(RDF_Statements statement, MemoryContext cxt)
{
	HeapTuple retTup = NULL;
	MemoryContext oldcxt = MemoryContextSwitchTo(cxt);
	retTup = rdf_statment_formtuple(statement);
	MemoryContextSwitchTo(oldcxt);

	return retTup;
}

/*
 * rdf_data_from_spo
 *		拷贝给定的三元组的内容至一块内存里并返回
 *		拷贝的顺序不能变，顺序是s - > p -> o -> context
 *
 *@subject: subject
 *@predicate: predicate
 *@object: object
 *@ctxt: context
 *@data_len: 返回内存的长度
 *
 *return-values: 返回空代表失败
 */
static
char *rdf_data_from_spo(nodes *subject,
												nodes *predicate,
												nodes *object,
												nodes *ctxt,
												size_t *data_len)
{
	size_t tlens[4] = {0, 0, 0, 0};
	char *tdata[4] = {NULL, NULL, NULL, NULL};
	char *data = NULL, *pdata = NULL;

	/* 拷贝subject、predicate和object的内容至一块内存里 */
	tlens[0] = rdf_nodes_size(subject);
	tdata[0] = rdf_copy_nodes(subject, tlens[0]);
	tlens[1] = rdf_nodes_size(predicate);
	tdata[1] = rdf_copy_nodes(predicate, tlens[1]);
	tlens[2] = rdf_nodes_size(object);
	tdata[2] = rdf_copy_nodes(object, tlens[2]);
	if (ctxt)
	{
		tlens[3] = rdf_nodes_size(ctxt);
		tdata[3] = rdf_copy_nodes(ctxt, tlens[3]);
	}

	*data_len += tlens[0] + tlens[1] + tlens[2] + tlens[3];
	data = (char *)palloc0(*data_len);
	pdata = data;

	/* 拷贝三元组内容 */
	for (int i = 0; i < 4; ++i)
	{
		if (tdata[i])
		{
			memcpy(pdata, tdata[i], tlens[i]);
			pdata += tlens[i];
			pfree(tdata[i]);/* 下面不需要了，直接释放掉 */
		}
	}

	return data;
}

/*
 * rdf_statement_store_from_heaptuple
 *		由一个#HeapTuple构造一个三元组返回
 *
 *@tuple: 构造源
 *
 *return-values: 返回空代表失败
 */
static
RDF_Statements rdf_statement_store_from_heaptuple(HeapTuple tuple)
{
	Assert(tuple != NULL);

	RDF_Statements statement_store = (RDF_Statements)palloc0(sizeof(RDF_Statements_Store));
	char *data = fxdb_tuple_to_chars(tuple);
	char *pdata = data;
	char *tdata = NULL;
	nodes *subject = NULL, *predicate = NULL, *object = NULL, *ctxt = NULL;
	size_t lens = 0;
	int ctxt_type = 0;

	/* 拷贝四个uint64类型的值(subject、predicate、object和context) */

	/* subject */
	memcpy(&statement_store->subject_id, pdata, sizeof(statement_store->subject_id));
	pdata += sizeof(statement_store->subject_id);

	/* predicate */
	memcpy(&statement_store->predicate_id, pdata, sizeof(statement_store->predicate_id));
	pdata += sizeof(statement_store->predicate_id);

	/* object */
	memcpy(&statement_store->object_id, pdata, sizeof(statement_store->object_id));
	pdata += sizeof(statement_store->object_id);

	/* context */
	memcpy(&statement_store->context, pdata, sizeof(statement_store->context));
	pdata += sizeof(statement_store->context);

	/* inference id */
	memcpy(&statement_store->inf_id, pdata, sizeof(statement_store->inf_id));
	pdata += sizeof(statement_store->inf_id);

	pfree(data);

	/* 
	 *分别构造subject、predicate和object的内容
	 */
	subject = rdf_nodes_from_id(statement_store->subject_id);
	predicate = rdf_nodes_from_id(statement_store->predicate_id);
	object = rdf_nodes_from_id(statement_store->object_id);
	if (statement_store->context != 0) /* statement在没有存context的时候这里存放的是0 */
		ctxt = rdf_nodes_from_id(statement_store->context);

	//tdata = rdf_data_from_spo(subject, predicate, object, ctxt, &lens);
	statement_store->subject = subject;
	statement_store->predicate = predicate;
	statement_store->object = object;
	statement_store->ctxt = ctxt;

	return statement_store;
}

/*
 * RDFNewXactStart
 *		启动一个新的事务
 */
void RDFNewXactStart()
{
	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();
}

/*
 * RDFNewXactCommit
 *		提交事务
 */
void RDFNewXactCommit()
{
	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();
}

/*
 * RDFNewXactAbort
 *		回滚事务
 */
void RDFNewXactAbort()
{
	StartTransactionCommand();
	UserAbortTransactionBlock();
	CommitTransactionCommand();
}

static
void rdf_rbl_add_internal(char *data, size_t lens)
{
	uint64 id;
	memcpy(&id, data, sizeof(id));

	if (!rdf_rbl_exists(id, NULL))
	{
		HeapTuple tuple = fdxdb_heap_formtuple(data, lens);
		Relation rel = heap_open(RDF_Meta_RBL, RowExclusiveLock);
		simple_heap_insert(rel, tuple);
		heap_close(rel, NoLock);
		pfree(tuple);
		CommandCounterIncrement();
	}
}

/*
 * rdf_rbl_add
 *		将node的内容插入到RBL表中
 */
static
void rdf_rbl_add(nodes *node, uint64 node_id)
{
	char *data = NULL;
	size_t tlens = 0;

	data = rdf_rbl_formdata_from_node(node, node_id, &tlens);

	rdf_rbl_add_internal(data, tlens);
	pfree(data);
}

/*
 * rdf_statement_rbl_add
 *		将statement的内容插入到RBL表中
 */
static
void rdf_statement_rbl_add(RDF_Statements statement)
{
	if (statement->subject != NULL)
		rdf_rbl_add(statement->subject, statement->subject_id);

	if (statement->predicate != NULL)
		rdf_rbl_add(statement->predicate, statement->predicate_id);

	if (statement->object != NULL)
		rdf_rbl_add(statement->object, statement->object_id);

	if (statement->ctxt != NULL)
		rdf_rbl_add(statement->ctxt, statement->context);
}

/*
 * RDFStatementAdd
 *		将一个statement加入到表中
 *
 *@statement: 要插入的statement
 *@storage:	storage信息
 */
void RDFStatementAdd(RDF_Statements statement, RDFMeta_Models storage)
{
	Assert(statement);

	if (statement_exists(statement, storage, NULL))
	{
		ereport(LOG, 
			(errmsg("statement already exists.")));
		return;
	}

	rdf_statement_rbl_add(statement);

	Relation rel = heap_open(storage->stid, RowExclusiveLock);
	HeapTuple tup = rdf_statment_formtuple(statement);
	simple_heap_insert(rel, tup);
	pfree(tup);
	heap_close(rel, NoLock);
}

/*
 * RDFStatementMultiAdd
 *		将一个statement加入到表中
 *
 *@arr_statements: 要插入的statement数组
 *@arr_lens: statement数组长度
 *@arr_rbls: nodes内容数组
 *@rbl_arr_lens: nodes内容数组长度
 *@storage:	storage信息
 *
 * 注意该函数在批量插入的时候不会去检查唯一性，所以调用者务必在外头构造
 * 数组的时候使用RDFStatementExists函数和RDFRBLExists函数过滤好数组内容
 */
void RDFStatementMultiAdd(HeapTuple *arr_statements, int statement_arr_lens,
													HeapTuple *arr_rbls, int rbl_arr_lens,
													RDFMeta_Models storage)
{
	BulkInsertState state = GetBulkInsertState();

	/* 往statement的id表中插入数据 */
	Relation rel = heap_open(storage->stid, RowExclusiveLock);
	heap_multi_insert(rel, arr_statements, statement_arr_lens, state);
	index_multi_insert(rel, arr_statements, statement_arr_lens);
	heap_close(rel, NoLock);

	FreeBulkInsertState(state);

	state = GetBulkInsertState();

	/* 往RBL表中插入数据 */
	rel = heap_open(RDF_Meta_RBL, RowExclusiveLock);
	heap_multi_insert(rel, arr_rbls, rbl_arr_lens, state);
	index_multi_insert(rel, arr_rbls, rbl_arr_lens);
	heap_close(rel, NoLock);

	FreeBulkInsertState(state);

	CommandCounterIncrement();
}

/*
 * RDFStatementRemove
 *		删除一个statement。如果statement为null，则删除表中所有的数据。
 *
 *@statement: 要删除的statement
 *@storage:	storage表信息
 *
 *	返回删除的statement条数
 */
unsigned int RDFStatementRemove(RDF_Statements statement, RDFMeta_Models storage)
{
	Assert(storage != NULL);

	Relation rel = heap_open(storage->stid, RowExclusiveLock);
	HeapTuple tuple = NULL;
	unsigned int rev_rows = 0;
	if (statement)
	{
		int key_size = 0;
		ScanKey key = statement_init_scankey(statement, &key_size);
		Oid idx_id = get_scan_index_id(statement, storage);

		Relation rel = heap_open(storage->stid, AccessShareLock);
		Relation irel = index_open(idx_id, AccessShareLock);

		SysScanDesc sd = systable_beginscan_ordered(rel, irel,
			SnapshotNow, key_size, key);

		while ((tuple =  systable_getnext_ordered(sd, ForwardScanDirection)) != NULL)
		{
			simple_heap_delete(rel, &tuple->t_self);
			++rev_rows;
		}

		systable_endscan_ordered(sd);
		index_close(irel, NoLock);
		heap_close(rel, NoLock);
		pfree(key);
	} else {
		HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);
		while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			simple_heap_delete(rel, &tuple->t_self);
			++rev_rows;
		}
		heap_endscan(scan);
	}

	heap_close(rel, NoLock);

	CommandCounterIncrement();

	return rev_rows;
}

/*
 * RDFStatementContextRemove
 *		删除一个statement。如果context为0，则删除表中所有的数据。
 *
 *@context: 要删除的三元组的context
 *@storage:	storage表信息
 */
void RDFStatementContextRemove(uint64 context,RDFMeta_Models storage)
{
	Assert(storage != NULL);

	/* 构造查询条件 */
	int key_size = 0;
	RDF_Statements_Store statement;
	statement.subject_id = 0;
	statement.predicate_id = 0;
	statement.object_id = 0;
	statement.context = context;

	ScanKey key = statement_init_scankey(&statement, &key_size);

	Relation rel = heap_open(storage->stid, RowExclusiveLock);
	Relation irel = index_open(storage->index_spo_id, AccessShareLock);
	SysScanDesc sd = systable_beginscan_ordered(rel, irel, SnapshotNow, key_size, key);
	HeapTuple tuple = NULL;

	while ((tuple = systable_getnext_ordered(sd, ForwardScanDirection)) != NULL)
	{
		simple_heap_delete(rel, &tuple->t_self);
	}

	systable_endscan_ordered(sd);
	index_close(irel, NoLock);
	heap_close(rel, NoLock);
	pfree(key);
}

/*
 * RDFStatementSearch
 *		根据statement查找所有符合条件的statement,返回一个SysScanDesc
 *
 *@statement: 查找条件
 *@storage:	storage表信息
 */
SysScanDesc RDFStatementSearch(RDF_Statements statement, RDFMeta_Models storage)
{
	Assert(statement);

	int key_size = 0;
	ScanKey key = NULL;
	SysScanDesc sd = NULL;

	key = statement_init_scankey(statement, &key_size);
	Oid idx_id = get_scan_index_id(statement, storage);

	Relation rel = heap_open(storage->stid, AccessShareLock);
	sd = systable_beginscan(rel, idx_id, idx_id != InvalidOid, SnapshotNow, key_size, key);
	if (key)
		pfree(key);

	return sd;
}

/*
 * RDFCounterStorageSize
 *		使用顺序扫描计算一个表中元组的数量
 *
 *@relid: 表id
 *@count: 计数器
 */
void RDFCounterStorageSize(Oid relid, long *count)
{
	Assert(relid != InvalidOid);

	*count = 0;

	*count = (long)CounterHeapSize(relid);
}

/*
 * RDFStatementExists
 *		查询一个statement是否存在
 *
 *@statement: 源statement
 *@storage:	storage表信息
 */
bool RDFStatementExists(RDF_Statements statement, RDFMeta_Models storage)
{
	Assert(statement);
	bool result = false;

	result = statement_exists(statement, storage, NULL);

	return result;
}

/*
 * rdf_data_store_to_statement
 *		给定的内存块构造出一个#RDF_Statements
 *		这个内存块的内容格式是固定的，构造的时候请
 *		参考rdf_data_store_from_heaptuple函数
 *
 *@data: 内存块
 */
static
RDF_Statements rdf_data_store_to_statement(char *data)
{
	int subject_type, predicate_type, object_type, ctxt_type;
	RDF_Statements statement_store = (RDF_Statements)palloc0(sizeof(RDF_Statements_Store));

	/* 构造三元组的id */
	memcpy(&statement_store->subject_id, data, sizeof(statement_store->subject_id));
	data += sizeof(statement_store->subject_id);
	memcpy(&statement_store->predicate_id, data, sizeof(statement_store->predicate_id));
	data += sizeof(statement_store->predicate_id);
	memcpy(&statement_store->object_id, data, sizeof(statement_store->object_id));
	data += sizeof(statement_store->object_id);
	memcpy(&statement_store->context, data, sizeof(statement_store->context));
	data += sizeof(statement_store->context);

	/* 构造三元组的type */
	memcpy(&subject_type, data, sizeof(subject_type));
	data += sizeof(subject_type);
	memcpy(&predicate_type, data, sizeof(predicate_type));
	data += sizeof(predicate_type);
	memcpy(&object_type, data, sizeof(object_type));
	data += sizeof(object_type);
	memcpy(&ctxt_type, data, sizeof(ctxt_type));
	data += sizeof(ctxt_type);

	/* 构造三元组的内容 */
	statement_store->subject = rdf_type_copy_nodes(&data, subject_type);
	statement_store->predicate = rdf_type_copy_nodes(&data, predicate_type);
	statement_store->object = rdf_type_copy_nodes(&data, object_type);
	if (ctxt_type != RDF_TYPE_UNKNOWN)
		statement_store->ctxt = rdf_type_copy_nodes(&data, ctxt_type);

	return statement_store;
}

/*
 * RDFStatementGetNext
 *		获取下一个statement
 *
 *@state: 结果集
 *@statments_store:	传出参数
 */
RDF_Statements RDFStatementGetNext(SysScanDesc sd)
{
	Assert(sd != NULL);

	RDF_Statements statement_store = NULL;
	HeapTuple mTuple = NULL;

	mTuple = systable_getnext(sd);
	if (!mTuple)
		return NULL;

	statement_store = rdf_statement_store_from_heaptuple(mTuple);

	return statement_store;
}

static
void resource_free(nodes *node)
{
	if (node->data.resource.uri)
		pfree(node->data.resource.uri);
}

static
void bnode_free(nodes *node)
{
	if (node->data.bnode.name)
		pfree(node->data.bnode.name);
}

static
void literal_free(nodes *node)
{
	if (node->data.literal.values)
		pfree(node->data.literal.values);
	if (node->data.literal.language)
		pfree(node->data.literal.language);
	if (node->data.literal.data_type)
		pfree(node->data.literal.data_type);
}

static
void node_free(nodes *node)
{
	switch(node->type)
	{
	case RDF_TYPE_RESOURCE:
		resource_free(node);
		break;
	case RDF_TYPE_LITERAL:
		literal_free(node);
		break;
	case RDF_TYPE_BNODES:
		bnode_free(node);
		break;
	}

	pfree(node);
}

/*
 * RDFStetementFree
 *		释放一个statement
 *
 *@statement_store: statement objects
 */
void RDFStetementFree(RDF_Statements statement_store)
{
	Assert(statement_store != NULL);

	if (statement_store->subject)
		node_free(statement_store->subject);

	if (statement_store->predicate)
		node_free(statement_store->predicate);

	if (statement_store->object)
		node_free(statement_store->object);

	if (statement_store->ctxt)
		node_free(statement_store->ctxt);

	pfree(statement_store);
}

/*
 * RDFStatementContextSearch
 *		根据context查找所有符合条件的statement,返回一个#SysScanDesc
 *
 *@statement: 查找条件
 *@storage:	storage表信息
 */
SysScanDesc RDFStatementContextSearch(uint64 ctxt, RDFMeta_Models storage)
{
	int key_size = 0;
	ScanKeyData key[1];
	Datum values;
	int strategy;
	SysScanDesc sd = NULL;

	if (ctxt)
		strategy = BTEqualStrategyNumber;
	else
		strategy = BTGreaterStrategyNumber;

	values = fdxdb_string_formdatum((char*)&ctxt, sizeof(ctxt));
	Fdxdb_ScanKeyInitWithCallbackInfo(
		&key[0],
		RDF_STATEMENTMETA_TABLE_COL_CONTEXT,
		strategy, 
		uint64_compare, values);

	Relation rel = heap_open(storage->stid, AccessShareLock);
	sd = systable_beginscan(rel, storage->index_spo_id, true, SnapshotNow, 1, key);

	return sd;
}

/*
 * RDFResultsClear
 *		清空#SysScanDesc
 *
 *@sd: #SysScanDesc
 */
void RDFResultsClear(SysScanDesc sd)
{
	Relation rel = sd->heap_rel;
	systable_endscan(sd);

	if (rel)
		heap_close(rel, NoLock);
}

/*
 * RDFGetStorageOid
 *		根据id或者name获取属于该storage的statement表的id,优先
 *		使用id匹配查找
 *@id: hash id
 *@name: storage name
 *@storage: statement表信息 [输出参数]
 */
void RDFGetStorageOid(uint64 id, const char *name, RDFMeta_Models storage)
{
	Assert(storage != NULL);

	if (!storage_exists(id, name, storage))
	{
		ereport(ERROR, 
			(errcode(ERRCODE_UNDEFINED_OBJECT),
			errmsg("storage not exists.")));
	}
}

/* 删除storage的statement表 */
static
void rdf_drop_statement_table(RDFMeta_Models storage)
{
	fdxdb_heap_drop(storage->stid);
}

/* 删除storage的元数据 */
static
void rdf_remove_storage_meta_data(RDFMeta_Models storage)
{
	Relation rel = heap_open(RDF_Meta_Models, RowExclusiveLock);
	HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);
	HeapTuple tuple = NULL;

	while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		RDFMeta_Models formClass = (RDFMeta_Models)fxdb_tuple_to_chars(tuple);
		if (formClass->id == storage->id && storage->db_id == MyDatabaseId)
		{
			simple_heap_delete(rel, &tuple->t_self);
			pfree(formClass);
			break;
		}
		pfree(formClass);
	}

	heap_endscan(scan);
	heap_close(rel, RowExclusiveLock);
}

/*
 * RDFStatementTableDrop
 *		删除statement表
 *
 *@storage:	storage表信息
 *@startTrans: 是否在函数中启动事务
 */
void RDFStatementTableDrop(RDFMeta_Models storage)
{
	Assert(storage != NULL);

	if (!storage_exists(0, storage->name.data, NULL))
	{
		ereport(ERROR, 
			(errcode(ERRCODE_UNDEFINED_OBJECT),
			errmsg("storage %s not exists.", storage->name.data)));
	}
	else
	{
		rdf_drop_statement_table(storage);
		rdf_remove_storage_meta_data(storage);
	}
}

/*
 * RDFGetAllGraph
 *		获取所有graph的信息
 *
 *@count:	返回的结果个数
 */
RDFMeta_Models RDFGetAllGraph(unsigned int *count)
{
	Assert(count != NULL);

	RDFMeta_Models models = NULL;

	Relation rel = heap_open(RDF_Meta_Models, AccessShareLock);
	HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);
	HeapTuple tuple = NULL;
	*count = 0;
	while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		RDFMeta_Models formClass = (RDFMeta_Models)fxdb_tuple_to_chars(tuple);
		if (formClass->db_id == MyDatabaseId)
		{
			++(*count);
			if (models == NULL)
				models = (RDFMeta_Models)palloc0(sizeof(RDFMeta_Models_Data));
			else 
				models = (RDFMeta_Models)repalloc(models, *count * sizeof(RDFMeta_Models_Data));

			memcpy(&models[(*count) - 1], formClass, sizeof(RDFMeta_Models_Data));
		}
		pfree(formClass);
	}
	heap_endscan(scan);
	heap_close(rel, AccessShareLock);
	return models;
}

/*
 * InitXactRDFNodeCache
 *		初始化事务的节点cache
 */
void InitXactRDFNodeCache()
{
	HASHCTL		ctl;

	/** Initialize the hash table. */
	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(uint64);
	ctl.entrysize = sizeof(NodeInfo);
	/* hash style uint64  */
	ctl.hash = uint64_hash;
	ctl.match = uint64_compare_hash;
	ctl.hcxt = TopTransactionContext;
	char cacheName[200];
	memset(cacheName, 0, 200);
	sprintf(cacheName, "Node Cache %u", CurrentTransactionState->transactionId);
 
	XactNodeCache = 
		hash_create(cacheName, XactNodeCacheSize, &ctl, HASH_ELEM | HASH_FUNCTION | HASH_COMPARE |HASH_CONTEXT);
}

/*
 * AtEOXactRDFNodeCache
 *		销毁事务的节点cache
 */
void AtEOXactRDFNodeCache(bool isCommit)
{
	/* 
	 *这里cache保护cache的内容都分配在事务的context中 
	 *有可能会有很多的element，为了不拖慢事务提交或者回滚
	 *的效率，这里不做遍历释放内存的操作，由事务统一释放内存
	 */
	hash_destroy(XactNodeCache);
	XactNodeCache = NULL;
}

/*
 * AtEOXactRDFNodeIDCache
 *		销毁事务的节点id cache
 */
void AtEOXactRDFNodeIDCache(bool isCommit)
{
	/* 同AtEOXactRDFNodeCache */
	if (XactNodeIDCache)
	{
		hash_destroy(XactNodeIDCache);
		XactNodeIDCache = NULL;
	}
}

/*
 * RDFGetStorageOid
 *		初始化图数据库元数据表的colinfo
 */
void InitRDFMetaTableColinfo()
{
	init_rdf_models_colinfo();
	init_rdf_models_index_colinfo();
	init_rdf_rbl_colinfo();
	init_rdf_rbl_index_colinfo();

	// 同时初始化statement表和索引的colinfo
	init_rdf_statements_colinfo();
	init_rdf_statements_index_colinfo();
}

/*
 * RDFGetTempContext
 *		构建一个临时的MemoryContext
 */
MemoryContext RDFGetTempContext(char *tname)
{
	return AllocSetContextCreate(TopMemoryContext,
		tname,
		ALLOCSET_SMALL_MINSIZE,
		ALLOCSET_SMALL_INITSIZE,
		ALLOCSET_SMALL_MAXSIZE);
}

/*
 * RDFDeleteTempContext
 *		删除一个临时的MemoryContext
 */
void RDFDeleteTempContext(MemoryContext cxt)
{
	MemoryContextDelete(cxt);
}

#endif //FOUNDER_XDB_SE
