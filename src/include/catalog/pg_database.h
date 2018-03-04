/*-------------------------------------------------------------------------
 *
 * pg_database.h
 *	  definition of the system "database" relation (pg_database)
 *	  along with the relation's initial contents.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/pg_database.h
 *
 * NOTES
 *	  the genbki.pl script reads this file and generates .bki
 *	  information from the DATA() statements.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_DATABASE_H
#define PG_DATABASE_H

#include "catalog/genbki.h"
#include "storage/s_lock.h"

#ifndef FOUNDER_XDB_SE
/* ----------------
 *		pg_database definition.  cpp turns this into
 *		typedef struct FormData_pg_database
 * ----------------
 */
#define DatabaseRelationId	1262
#define DatabaseRelation_Rowtype_Id  1248
#endif //FOUNDER_XDB_SE
#ifndef FOUNDER_XDB_SE
CATALOG(pg_database,1262) BKI_SHARED_RELATION BKI_ROWTYPE_OID(1248) BKI_SCHEMA_MACRO
{
	NameData	datname;		/* database name */
	Oid			datdba;			/* owner of database */
	int4		encoding;		/* character encoding */
	NameData	datcollate;		/* LC_COLLATE setting */
	NameData	datctype;		/* LC_CTYPE setting */
	bool		datistemplate;	/* allowed as CREATE DATABASE template? */
	bool		datallowconn;	/* new connections allowed? */
	int4		datconnlimit;	/* max connections allowed (-1=no limit) */
	Oid			datlastsysoid;	/* highest OID to consider a system OID */
	TransactionId datfrozenxid; /* all Xids < this are frozen in this DB */
	Oid			dattablespace;	/* default table space for this DB */
	aclitem		datacl[1];		/* access permissions (VAR LENGTH) */
} FormData_pg_database;

/* ----------------
 *		Form_pg_database corresponds to a pointer to a tuple with
 *		the format of pg_database relation.
 * ----------------
 */
typedef FormData_pg_database *Form_pg_database;
#endif
extern THREAD_LOCAL NameData CurrentDatabaseName;
/* ----------------
 *		compiler constants for pg_database
 * ----------------
 */
#define Natts_pg_database				12
#define Anum_pg_database_datname		1
#define Anum_pg_database_datdba			2
#define Anum_pg_database_encoding		3
#define Anum_pg_database_datcollate		4
#define Anum_pg_database_datctype		5
#define Anum_pg_database_datistemplate	6
#define Anum_pg_database_datallowconn	7
#define Anum_pg_database_datconnlimit	8
#define Anum_pg_database_datlastsysoid	9
#define Anum_pg_database_datfrozenxid	10
#define Anum_pg_database_dattablespace	11
#define Anum_pg_database_datacl			12

DATA(insert OID = 1 (  template1 PGUID ENCODING "LC_COLLATE" "LC_CTYPE" t t -1 0 0 1663 _null_));
SHDESCR("default template for new databases");
#define TemplateDbOid			1

#define DEFAULT_DATABASE_NAME "defaultdatabase"
#define DEFAULT_DATABASE_OID 11967
#pragma pack(4)
struct Form_meta_database_data
{
	Oid db_id;                //db id
	NameData db_name;         //data base name
	TransactionId xid;        //create transaction
	Oid		defTablespace;    //defaulttablespace
	Oid 	seqRelId;         //sequence table oid
	Oid 	seqIdIdxId;       // sequence table index on seqId
	Oid 	seqNameIdxId;     //sequence table index on seqName
	char    reserved[16];
	text    extraData;         // extra user data,such as db creator, 
	// create data, etc that can be used by caller.
	// Such data is seen as a byte array in storage engine.
} ;
#pragma pack()

#define  MAX_DB_EXTRADATA_LEN 1024
#define  MAX_FORM_DBMETA_SIZE (sizeof(Form_meta_database_data) + MAX_DB_EXTRADATA_LEN)
typedef Form_meta_database_data* Form_meta_database;

typedef struct
{
	Oid dbid;
	NameData dbname;
	Oid seqRelId;
	Oid seqIdIdxId;
	Oid seqNameIdxId;
	volatile Oid next_loid;
	volatile slock_t loid_lock;
} DBInfo;

#pragma pack(4)
struct Form_meta_sequence_data
{
	Oid seqId;           //sequence id
	NameData seqName;    //sequence name
	int64 value;         //sequence value
	uint32 hasRange;     //has value range, if = 1, minValue and maxValue is usefull, else invalid
	int64 minValue;      //sequence min value
	int64 maxValue;      //sequence max value
	uint32 flags;        //flags
};
#pragma pack()
typedef Form_meta_sequence_data* Form_meta_sequence;

/************************************************************************** 
* @brief CreateDatabase  Used to create a database
* 
* Detailed description.
* @param[in] xid                Transaction in who create this database
* @param[in] datname            name of this database
* @param[in] defaultTblspcName  when create heap in this database will use this tablespace if no tablesapce specified
* @param[in] void*extraData     user data
* @param[in] extraDataLen       len(extraData)
**************************************************************************/
Oid CreateDatabase(TransactionId xid
					, const char *datname 
					, const char *defaultTblspcName
					, const void*extraData = NULL
					, size_t extraDataLen = 0
					, Oid dbid = 0);

/************************************************************************** 
* @brief DropDatabase Drop the database that named dbname
* 
* Detailed description.
* @param[in] dbname  the name of the db need droped
**************************************************************************/
void DropDatabase(const char *dbname);


/************************************************************************** 
* @brief GetDatabaseOid Get the oid of the database named dbname
* 
* Detailed description.
* @param[in] dbname 
* @return Oid  
**************************************************************************/
Oid GetDatabaseOid(const char* dbname);

/************************************************************************** 
* @brief GetDatabaseMetaData 
* 
* Detailed description.
* @param[in] dbname 
* @return Form_meta_database  
**************************************************************************/
Form_meta_database GetDatabaseMetaDataByID(Oid db_id, ItemPointer it = NULL);
Form_meta_database GetDatabaseMetaData(const char* dbname,ItemPointer it = NULL);

void UpdateExtraData(const char* dbname,const void* data,size_t len);

Form_meta_database GetAllDatabaseInfos(size_t& nCnt, bool useXactMemory = true);

void InitDatabaseColiId( void );
int GetDatabaseColiId( void );

void InitSequenceColiId( void );

Oid SetCurrentDatabase(const char* dbname,Oid dbId,Oid tablespaceId);
Oid GetCurrentDatabase(const char*& dbname);
Oid GetCurrentDatabaseTablespace();
bool IsSomeOnesDefaultTablespace(const char* tablespaceName);
HASH_SEQ_STATUS *GetDBInfoHashStatus();
DBInfo *GetDBInfo(Oid dbid);
void DestoryDBInfo();
void RebuildDBInfo(bool isInXactBlock);


void GetSeqTableByDbid(Oid dbid, Oid& seqRelId,
	Oid& seqIdIdxId, Oid& seqNameIdxId);
void GetCurrSeqTable(Oid& seqRelId, Oid& seqIdIdxId, Oid& seqNameIdxId);
void InsertSeqInfo(Oid seqRelId, Oid seqId, const char *seqName, int64 value,
					bool hasRange, int64 minValue, int64 maxValue, uint32 flags);
void DeleteSeqInfoById(Oid seqRelId, Oid seqIdIdxId, Oid seqId);
void DeleteSeqInfo(Oid seqRelId, Oid seqNameIdxId, const char *seqName, Oid& seqId);
void GetMaxSeqId(Oid seqRelId, Oid seqIdIdxId, Oid& maxSeqId);

void GetSeqInfoById(Oid seqRelId, Oid seqIdIdxId, Oid seqId,
					char **pSeqName, int64& value, bool& hasRange, int64& minValue,
					int64& maxValue, uint32& flags);
void GetSeqInfo(Oid seqRelId, Oid seqNameIdxId, const char *seqName,
					Oid& seqId, int64& value, bool& hasRange, int64& minValue,
					int64& maxValue, uint32& flags);
void GetSeqValueById(Oid seqRelId, Oid seqIdIdxId, Oid seqId, int64& value);
void UpdateSeqValueById(Oid seqRelId, Oid seqIdIdxId,
					Oid seqId, bool isDeltaValue, int64 delta, int64& oldValue);

#endif   /* PG_DATABASE_H */
