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
	uint64 id;						//��hash����������Ψһ��ʶ
	Oid stid;							//storageӵ�е�statement��id
	Oid index_po_id;			//_po����
	Oid index_os_id;			//s_o����
	Oid index_spo_id;			//spo����
	Oid index_inf_id;		//����id����
	Oid db_id;						//storage�������ݿ�
	NameData name;				//storage��
}  RDFMeta_Models_Data;

#pragma pack()

typedef  RDFMeta_Models_Data* RDFMeta_Models;

typedef struct
{
	size_t value_lens;		//literalֵ�ĳ���
	char * values;				//literal��ֵ
	size_t language_lens;	//literal�����ַ�������
	char * language;			//literal�����ַ���
	size_t data_type_lens;//literal���������ַ�������
	char * data_type;			//literal��������
}  RDF_Literal;

typedef struct
{
	size_t uri_lens;			//uri�ַ�������
	char *uri;						//uri��ֵ
}  RDF_Resource;

typedef struct
{
	size_t name_lens;			//blank node�������ַ�������
	char *name;						//blank node�������ַ���
}  RDF_Bnode;

typedef struct
{
	int type;							//s��p����o������
	union
	{
		RDF_Literal literal;
		RDF_Resource resource;
		RDF_Bnode bnode;
	} data;
}  nodes;

typedef struct 
{
	uint64 subject_id;		//subjectΨһ��ʶ
	uint64 predicate_id;	//predicateΨһ��ʶ
	uint64 object_id;			//objectΨһ��ʶ
	uint64 context;				//contextΨһ��ʶ��Ŀǰcontext����ʹ�ã����в��������ڼ���context����0
	int		 inf_id;				//����id
	nodes *subject;
	nodes *predicate;
	nodes *object;
	nodes *ctxt;
} RDF_Statements_Store;
typedef  RDF_Statements_Store *RDF_Statements;

#endif //RDF_STORAGE_HANDLER
#endif //FOUNDER_XDB_SE
