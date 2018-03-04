/*-------------------------------------------------------------------------
 *
 * rdf_storage_handler.c
 *		����ͼ���ݿ�Ĳ�������
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

/* ��ӡ������Ϣ */
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
 *		ÿ��statement��ķ��Ѻ���
 *
 *@��1: subject		uint64
 *@��2: predicate uint64
 *@��3:	object		uint64
 *@��4:	context		uint64
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
			Assert(0);	//���µ��ֶβ��Ὠ������������������Ե�
	}
}

/*
 * rdf_models_split
 *		models��ķ��Ѻ���
 *
 *@��1: id		uint64
 *@��2: stid	Oid
 *@��3:	name	NameData
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
			Assert(0);//�޷�����Ĵ����
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
 *		������models��id���ϵ�����
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
 *		��ʼ��RBL���colinfo
 *	
 *	RBL������Ԫ������ݣ�ֻ��Ҫ�ڱ�ĵ�һ�У�ID�У��Ͻ�һ���������ɡ�
 *	����RBL��ķ��Ѻ���ֻ�����һ�С�
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
 *		������RBL��id���ϵ�����
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
 *		����Models��Ԫ���ݱ�
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
 *		�������resource��literal��bnode���ݵ�Ԫ���ݱ�
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
 *		����RDFԪ���ݱ�
 */
void CreateRDFTable()
{
	create_rdf_models_table();
	create_rbl_table();
}

/*
 * storage_exists
 *		�ж�һ��storage�Ƿ��Ѿ����ڡ�id��name����Ϊ�գ�����
 *		ʹ��idȥ���ҡ�
 *
 *@id: storage�ı�ʶ
 *@name: storage������
 *@storage: ��������
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
 *		����statement������Ԫ������Ϣ���뵽models����
 *
 *@id: storage��Ψһ��ʶ
 *@name: storage��
 *@storage: ��������
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
		/* spo��������ҪΨһ����Ϊ������Ԫ��֮ǰ��Ҫ��������ɨ�������Ψһ�� */
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
 *		���ݲ��ҵ�statement�����������Ƚ�����
 *		[sp_  o_s  _po  s__  _p_  __o  spo  inf_id] 
 */
static
ScanKey statement_init_scankey(RDF_Statements statement, int *key_size)
{
	bool use_context = false;
	ScanKey key = (ScanKey)palloc0(sizeof(ScanKeyData) * 5);

	// o_s������Ҫ������
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
	 * ������ô�жϵ�ԭ���Ǳ���������µ�scankey��ǰ׺������ʽ :
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
 *		�ж�һ��statement�Ƿ���ڣ�ʹ��s��p��o��ϲ�ѯ
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

	/* ����type�Ĳ�ͬ��������literal��resource����bnodes */
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
	default: //δ֪������
		Assert(0);
	}

	return tlens;
}

/*
 * rdf_copy_literal
 *		��literal�ṹ������ݿ�����һ���ڴ��з���
 *
 *@node: ������Դnode
 *@lens:	node���ݳ���
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
 *		��resource�ṹ������ݿ�����һ���ڴ��з���
 *
 *@node: ������Դnode
 *@lens:	statements���ݳ���
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
 *		��bnode�ṹ������ݿ�����һ���ڴ��з���
 *
 *@node: ������Դnode
 *@lens:	statements���ݳ���
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

	/* ����type�Ĳ�ͬ��������literal��resource����bnodes */
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
	default: //δ֪������
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
 *		�ж�һ��statement�Ƿ��Ѿ���id cache��
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
 *		�ж�һ��nodes��id�Ƿ��Ѿ��ڱ��д���
 *
 *@id: nodes_id
 *@data:	��������������Ϊ��
 */
bool RDFRBLExists(uint64 id, char **data)
{
	return rdf_rbl_exists(id, data);
}

/*
 * rdf_rbl_formdata_from_node
 *		��nodes����һ��data�ڴ�飬���ڲ��뵽rbl����
 */
static
char *rdf_rbl_formdata_from_node(nodes *node, uint64 node_id, size_t *tlens)
{
	char *tdata = NULL, *data = NULL;
	size_t lens = 0;

	lens = rdf_nodes_size(node);
	tdata = rdf_copy_nodes(node, lens);
	*tlens = lens + sizeof(node_id) + sizeof(node->type);

	/* ���ݳ��Ȱ������ݳ��ȡ�id���ͳ��Ⱥ�type���ͳ��� */
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
 *		�����񻺴��в���һ��node
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
 *		��ʼ������Ľڵ�id cache
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
 *		�����񻺴��в���һ��statement
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
 *		���������id cache����
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

	/* ��data�������������ڼ�Ļ����ϣ������������ʱ��ͳһ�������ͷ� */
	return data;
}

static
nodes *rdf_nodes_from_id(uint64 id)
{
	int type;
	nodes *node = NULL;
	char *data = rdf_id_get_nodes(id);
	char *pdata = data;

	/* ��һ����id������ */
	pdata += sizeof(id);

	/* ����type */
	memcpy(&type, pdata, sizeof(type));
	pdata += sizeof(type);

	node = rdf_type_copy_nodes(&pdata, type);

	return node;
}

/*
 * rdf_statment_formtuple
 *		��һ��statement����һ��HeapTuple����
 *
 *@statement: Դstatement
 */
static
HeapTuple rdf_statment_formtuple(RDF_Statements statement)
{
	HeapTuple rtup = NULL;

	size_t tup_lens = 0;

	/* ÿ��statement��ǰ���ĸ�Ԫ���ǹ̶����ȵ� */
	tup_lens = sizeof(statement->subject_id) * RDF_STATEMENTMETA_TABLE_COL_CONTEXT;

	/* ����statement������id���� */
	tup_lens += sizeof(statement->inf_id);

	char *data = (char*)palloc0(tup_lens);
	char *pdata = data;

	/* ����statement�ĸ��̶��ֶε���ֵ */
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
 *		ͬ����rdf_statment_formtuple
 *
 *@node: Դnodes
 *@node_id: node id
 *@cxt: HeapTuple�����ڸ�cxt��
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
 *		ͬ����rdf_statment_formtuple
 *
 *@statement: Դstatement
 *@cxt: HeapTuple������cxt��
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
 *		������������Ԫ���������һ���ڴ��ﲢ����
 *		������˳���ܱ䣬˳����s - > p -> o -> context
 *
 *@subject: subject
 *@predicate: predicate
 *@object: object
 *@ctxt: context
 *@data_len: �����ڴ�ĳ���
 *
 *return-values: ���ؿմ���ʧ��
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

	/* ����subject��predicate��object��������һ���ڴ��� */
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

	/* ������Ԫ������ */
	for (int i = 0; i < 4; ++i)
	{
		if (tdata[i])
		{
			memcpy(pdata, tdata[i], tlens[i]);
			pdata += tlens[i];
			pfree(tdata[i]);/* ���治��Ҫ�ˣ�ֱ���ͷŵ� */
		}
	}

	return data;
}

/*
 * rdf_statement_store_from_heaptuple
 *		��һ��#HeapTuple����һ����Ԫ�鷵��
 *
 *@tuple: ����Դ
 *
 *return-values: ���ؿմ���ʧ��
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

	/* �����ĸ�uint64���͵�ֵ(subject��predicate��object��context) */

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
	 *�ֱ���subject��predicate��object������
	 */
	subject = rdf_nodes_from_id(statement_store->subject_id);
	predicate = rdf_nodes_from_id(statement_store->predicate_id);
	object = rdf_nodes_from_id(statement_store->object_id);
	if (statement_store->context != 0) /* statement��û�д�context��ʱ�������ŵ���0 */
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
 *		����һ���µ�����
 */
void RDFNewXactStart()
{
	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();
}

/*
 * RDFNewXactCommit
 *		�ύ����
 */
void RDFNewXactCommit()
{
	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();
}

/*
 * RDFNewXactAbort
 *		�ع�����
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
 *		��node�����ݲ��뵽RBL����
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
 *		��statement�����ݲ��뵽RBL����
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
 *		��һ��statement���뵽����
 *
 *@statement: Ҫ�����statement
 *@storage:	storage��Ϣ
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
 *		��һ��statement���뵽����
 *
 *@arr_statements: Ҫ�����statement����
 *@arr_lens: statement���鳤��
 *@arr_rbls: nodes��������
 *@rbl_arr_lens: nodes�������鳤��
 *@storage:	storage��Ϣ
 *
 * ע��ú��������������ʱ�򲻻�ȥ���Ψһ�ԣ����Ե������������ͷ����
 * �����ʱ��ʹ��RDFStatementExists������RDFRBLExists�������˺���������
 */
void RDFStatementMultiAdd(HeapTuple *arr_statements, int statement_arr_lens,
													HeapTuple *arr_rbls, int rbl_arr_lens,
													RDFMeta_Models storage)
{
	BulkInsertState state = GetBulkInsertState();

	/* ��statement��id���в������� */
	Relation rel = heap_open(storage->stid, RowExclusiveLock);
	heap_multi_insert(rel, arr_statements, statement_arr_lens, state);
	index_multi_insert(rel, arr_statements, statement_arr_lens);
	heap_close(rel, NoLock);

	FreeBulkInsertState(state);

	state = GetBulkInsertState();

	/* ��RBL���в������� */
	rel = heap_open(RDF_Meta_RBL, RowExclusiveLock);
	heap_multi_insert(rel, arr_rbls, rbl_arr_lens, state);
	index_multi_insert(rel, arr_rbls, rbl_arr_lens);
	heap_close(rel, NoLock);

	FreeBulkInsertState(state);

	CommandCounterIncrement();
}

/*
 * RDFStatementRemove
 *		ɾ��һ��statement�����statementΪnull����ɾ���������е����ݡ�
 *
 *@statement: Ҫɾ����statement
 *@storage:	storage����Ϣ
 *
 *	����ɾ����statement����
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
 *		ɾ��һ��statement�����contextΪ0����ɾ���������е����ݡ�
 *
 *@context: Ҫɾ������Ԫ���context
 *@storage:	storage����Ϣ
 */
void RDFStatementContextRemove(uint64 context,RDFMeta_Models storage)
{
	Assert(storage != NULL);

	/* �����ѯ���� */
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
 *		����statement�������з���������statement,����һ��SysScanDesc
 *
 *@statement: ��������
 *@storage:	storage����Ϣ
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
 *		ʹ��˳��ɨ�����һ������Ԫ�������
 *
 *@relid: ��id
 *@count: ������
 */
void RDFCounterStorageSize(Oid relid, long *count)
{
	Assert(relid != InvalidOid);

	*count = 0;

	*count = (long)CounterHeapSize(relid);
}

/*
 * RDFStatementExists
 *		��ѯһ��statement�Ƿ����
 *
 *@statement: Դstatement
 *@storage:	storage����Ϣ
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
 *		�������ڴ�鹹���һ��#RDF_Statements
 *		����ڴ������ݸ�ʽ�ǹ̶��ģ������ʱ����
 *		�ο�rdf_data_store_from_heaptuple����
 *
 *@data: �ڴ��
 */
static
RDF_Statements rdf_data_store_to_statement(char *data)
{
	int subject_type, predicate_type, object_type, ctxt_type;
	RDF_Statements statement_store = (RDF_Statements)palloc0(sizeof(RDF_Statements_Store));

	/* ������Ԫ���id */
	memcpy(&statement_store->subject_id, data, sizeof(statement_store->subject_id));
	data += sizeof(statement_store->subject_id);
	memcpy(&statement_store->predicate_id, data, sizeof(statement_store->predicate_id));
	data += sizeof(statement_store->predicate_id);
	memcpy(&statement_store->object_id, data, sizeof(statement_store->object_id));
	data += sizeof(statement_store->object_id);
	memcpy(&statement_store->context, data, sizeof(statement_store->context));
	data += sizeof(statement_store->context);

	/* ������Ԫ���type */
	memcpy(&subject_type, data, sizeof(subject_type));
	data += sizeof(subject_type);
	memcpy(&predicate_type, data, sizeof(predicate_type));
	data += sizeof(predicate_type);
	memcpy(&object_type, data, sizeof(object_type));
	data += sizeof(object_type);
	memcpy(&ctxt_type, data, sizeof(ctxt_type));
	data += sizeof(ctxt_type);

	/* ������Ԫ������� */
	statement_store->subject = rdf_type_copy_nodes(&data, subject_type);
	statement_store->predicate = rdf_type_copy_nodes(&data, predicate_type);
	statement_store->object = rdf_type_copy_nodes(&data, object_type);
	if (ctxt_type != RDF_TYPE_UNKNOWN)
		statement_store->ctxt = rdf_type_copy_nodes(&data, ctxt_type);

	return statement_store;
}

/*
 * RDFStatementGetNext
 *		��ȡ��һ��statement
 *
 *@state: �����
 *@statments_store:	��������
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
 *		�ͷ�һ��statement
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
 *		����context�������з���������statement,����һ��#SysScanDesc
 *
 *@statement: ��������
 *@storage:	storage����Ϣ
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
 *		���#SysScanDesc
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
 *		����id����name��ȡ���ڸ�storage��statement���id,����
 *		ʹ��idƥ�����
 *@id: hash id
 *@name: storage name
 *@storage: statement����Ϣ [�������]
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

/* ɾ��storage��statement�� */
static
void rdf_drop_statement_table(RDFMeta_Models storage)
{
	fdxdb_heap_drop(storage->stid);
}

/* ɾ��storage��Ԫ���� */
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
 *		ɾ��statement��
 *
 *@storage:	storage����Ϣ
 *@startTrans: �Ƿ��ں�������������
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
 *		��ȡ����graph����Ϣ
 *
 *@count:	���صĽ������
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
 *		��ʼ������Ľڵ�cache
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
 *		��������Ľڵ�cache
 */
void AtEOXactRDFNodeCache(bool isCommit)
{
	/* 
	 *����cache����cache�����ݶ������������context�� 
	 *�п��ܻ��кܶ��element��Ϊ�˲����������ύ���߻ع�
	 *��Ч�ʣ����ﲻ�������ͷ��ڴ�Ĳ�����������ͳһ�ͷ��ڴ�
	 */
	hash_destroy(XactNodeCache);
	XactNodeCache = NULL;
}

/*
 * AtEOXactRDFNodeIDCache
 *		��������Ľڵ�id cache
 */
void AtEOXactRDFNodeIDCache(bool isCommit)
{
	/* ͬAtEOXactRDFNodeCache */
	if (XactNodeIDCache)
	{
		hash_destroy(XactNodeIDCache);
		XactNodeIDCache = NULL;
	}
}

/*
 * RDFGetStorageOid
 *		��ʼ��ͼ���ݿ�Ԫ���ݱ��colinfo
 */
void InitRDFMetaTableColinfo()
{
	init_rdf_models_colinfo();
	init_rdf_models_index_colinfo();
	init_rdf_rbl_colinfo();
	init_rdf_rbl_index_colinfo();

	// ͬʱ��ʼ��statement���������colinfo
	init_rdf_statements_colinfo();
	init_rdf_statements_index_colinfo();
}

/*
 * RDFGetTempContext
 *		����һ����ʱ��MemoryContext
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
 *		ɾ��һ����ʱ��MemoryContext
 */
void RDFDeleteTempContext(MemoryContext cxt)
{
	MemoryContextDelete(cxt);
}

#endif //FOUNDER_XDB_SE
