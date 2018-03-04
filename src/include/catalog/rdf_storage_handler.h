#ifdef FOUNDER_XDB_SE

#ifndef RDF_STORAGE_HANDLER
#define RDF_STORAGE_HANDLER

#include "catalog/xdb_catalog.h"
#include "utils/tuplestore.h"

enum  RDFMetaModelsColNumber
{
	RDFMETA_TABLE_COL_ID = 1,
	RDFMETA_TABLE_COL_STID,
	RDFMETA_TABLE_COL_PO_INDEX_ID,
	RDFMETA_TABLE_COL_OS_INDEX_ID,
	RDFMETA_TABLE_COL_SPO_INDEX_ID,
	RDFMETA_TABLE_COL_INF_INDEX_ID,
	RDFMETA_TABLE_COL_DBID,
	RDFMETA_TABLE_COL_NAME,
	RDFMETA_TABLE_COL_NUM = RDFMETA_TABLE_COL_NAME
};

enum RDFMetaRBLColNumber
{
	RDFMETA_RBL_COL_ID = 1,
	RDFMETA_RBL_COL_NUM = RDFMETA_RBL_COL_ID
};

enum  RDFMetaStatementStoreColNumber
{
	RDF_STATEMENTMETA_TABLE_COL_SUBJECT = 1,
	RDF_STATEMENTMETA_TABLE_COL_PREDICATE,
	RDF_STATEMENTMETA_TABLE_COL_OBJECT,
	RDF_STATEMENTMETA_TABLE_COL_CONTEXT,
	RDF_STATEMENTMETA_TABLE_COL_INFERENCE,
	RDF_STATEMENTMETA_TABLE_COL_NUM = RDF_STATEMENTMETA_TABLE_COL_INFERENCE
};

enum  RDFStatementType
{
	RDF_TYPE_LITERAL,
	RDF_TYPE_RESOURCE,
	RDF_TYPE_BNODES,
	RDF_TYPE_UNKNOWN
};

#pragma pack(4)

typedef struct  
{
	uint64 id;						//由hash函数产生的唯一标识
	Oid stid;							//storage拥有的statement表id
	Oid index_po_id;			//_po索引
	Oid index_os_id;			//s_o索引
	Oid index_spo_id;			//spo索引
	Oid index_inf_id;		//推理id索引
	Oid db_id;						//storage所属数据库
	NameData name;				//storage名
}  RDFMeta_Models_Data;

#pragma pack()

typedef  RDFMeta_Models_Data* RDFMeta_Models;

typedef struct
{
	size_t value_lens;		//literal值的长度
	char * values;				//literal的值
	size_t language_lens;	//literal类型字符串长度
	char * language;			//literal类型字符串
	size_t data_type_lens;//literal数据类型字符串长度
	char * data_type;			//literal数据类型
}  RDF_Literal;

typedef struct
{
	size_t uri_lens;			//uri字符串长度
	char *uri;						//uri的值
}  RDF_Resource;

typedef struct
{
	size_t name_lens;			//blank node的名字字符串长度
	char *name;						//blank node的名字字符串
}  RDF_Bnode;

typedef struct
{
	int type;							//s、p或者o的类型
	union
	{
		RDF_Literal literal;
		RDF_Resource resource;
		RDF_Bnode bnode;
	} data;
}  nodes;

typedef struct 
{
	uint64 subject_id;		//subject唯一标识
	uint64 predicate_id;	//predicate唯一标识
	uint64 object_id;			//object唯一标识
	uint64 context;				//context唯一标识，目前context不会使用，所有操作都基于假设context都是0
	int		 inf_id;				//推理id
	nodes *subject;
	nodes *predicate;
	nodes *object;
	nodes *ctxt;
} RDF_Statements_Store;
typedef  RDF_Statements_Store *RDF_Statements;

#endif //RDF_STORAGE_HANDLER
#endif //FOUNDER_XDB_SE
