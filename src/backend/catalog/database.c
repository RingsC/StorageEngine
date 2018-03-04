#include <time.h>
#include "postgres.h"
#include "pgstat.h"
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
#include "access/transam.h"
#include "postmaster/bgwriter.h"
#include "storage/procarray.h"
#include "storage/smgr.h"
#include "access/xact.h"
#include "storage/proc.h"
#include "port/thread_commu.h"
#include <sys/stat.h>
#if !defined(_MSC_VER)
#pragma GCC diagnostic ignored "-Wclobbered"
#endif
enum DatabaseMetaColumn
{
	COLUMN_ID = 1
	,COLUMN_NAME
	,COLUMN_XID
	,COLUMN_DEFAULTTABLESPACE
	,COLUMN_SEQ_RELID
	,COLUMN_SEQID_IDXID
	,COLUMN_SEQNAME_IDXID
	,COLUMN_EXTRADATA
	,COLUMN_MAX
};

extern int char_compare(const char *str1, size_t len1, const char *str2, size_t len2);
extern int int4_compare(const char* a, size_t len1, const char* b, size_t len2);
extern Oid get_last_loid(Oid dbid, bool ignoreError);
THREAD_LOCAL NameData CurrentDatabaseName = {DEFAULT_DATABASE_NAME};

HTAB *DBInfoHash = NULL;
MemoryContext DBInfoContext = NULL;

Form_meta_database Tuple2FormDatabaseData(HeapTuple tup)
{
	return (Form_meta_database)(VARDATA_ANY((char*)tup->t_data + tup->t_data->t_hoff));
}

static void DatabaseMetaSplit(RangeData& range, const char *str, int col, size_t len = 0)
{

    switch (col)
    {
	case COLUMN_NAME:
		range.start = offsetof(Form_meta_database_data,db_name);
		range.len = sizeof(NameData);
		break;
	case COLUMN_ID:
		range.start = offsetof(Form_meta_database_data,db_id);
		range.len = sizeof(Oid);
		break;
	case COLUMN_XID:
		range.start = offsetof(Form_meta_database_data,xid);
		range.len = sizeof(TransactionId);
		break;
	case COLUMN_DEFAULTTABLESPACE:
		range.start = offsetof(Form_meta_database_data,defTablespace);
		range.len = sizeof(Oid);
		break;
	case COLUMN_SEQ_RELID:
		range.start = offsetof(Form_meta_database_data,seqRelId);
		range.len = sizeof(Oid);
		break;
	case COLUMN_SEQID_IDXID:
		range.start = offsetof(Form_meta_database_data,seqIdIdxId);
		range.len = sizeof(Oid);
		break;
	case COLUMN_SEQNAME_IDXID:
		range.start = offsetof(Form_meta_database_data,seqNameIdxId);
		range.len = sizeof(Oid);
		break;
	case COLUMN_EXTRADATA:
		range.start = offsetof(Form_meta_database_data,extraData);
		range.len = len - range.start;
		break;
    }

}

static void DatabaseNameMetaSplit(RangeData& range, const char *str, int col, size_t len = 0)
{
	if (1 == col)
	{
		range.start = 0;
		range.len = len;
	}
}

static void DatabaseIdMetaSplit(RangeData& range, const char *str, int col, size_t len = 0)
{
	if (1 == col)
	{
		range.start = 0;
		range.len = sizeof(Oid);
	}
}

static void CheckNameParma(const char* strName)
{
	if(strlen(strName) > NAMEDATALEN - 1)
	{
		ereport(ERROR,
			(errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("Database name out of size.")));
	}
}

int OidCompare(const void* p1,const void* p2)
{
    if (NULL == p1)
    {
		return NULL == p2 ? 0 : -1;
    }
	else 
	{
		return NULL == p2 ? 1 : *(Oid*)p1 - *(Oid*)p2;
	}

}

HeapTuple FormatDBMetaTuple( const void* extraData, size_t extraDataLen,
	Oid dbid, const char * datname, TransactionId xid, Oid defaultTablespace,
	Oid seqRelId, Oid seqIdIdxId, Oid seqNameIdxId)
{
	size_t szTuple = offsetof(Form_meta_database_data,extraData) + sizeof(Oid) + extraDataLen;
	Form_meta_database formDatabase = (Form_meta_database)palloc0(szTuple);
	memset(formDatabase,0,szTuple);
	formDatabase->db_id =  dbid;
	memcpy(&formDatabase->db_name, datname, strlen(datname) + 1);
	formDatabase->xid = xid;
	formDatabase->defTablespace = defaultTablespace;
	formDatabase->seqRelId = seqRelId;
	formDatabase->seqIdIdxId = seqIdIdxId;
	formDatabase->seqNameIdxId = seqNameIdxId;
	SET_VARSIZE(&(formDatabase->extraData),(extraDataLen));
	memcpy(VARDATA(&formDatabase->extraData), extraData, extraDataLen);

	HeapTuple tuple = fdxdb_heap_formtuple((char*)formDatabase, szTuple);
	pfree(formDatabase);

	return tuple;
}

void CopyFormDBMetaData(Form_meta_database dest,Form_meta_database src)
{
	dest->db_id = src->db_id;
	strcpy(dest->db_name.data,src->db_name.data);
	dest->defTablespace = src->defTablespace;
	dest->xid = src->xid;
	dest->seqRelId = src->seqRelId;
	dest->seqIdIdxId = src->seqIdIdxId;
	dest->seqNameIdxId = src->seqNameIdxId;
	memcpy(&dest->extraData,&src->extraData,sizeof(Oid) + MAX_DB_EXTRADATA_LEN);
}

enum SequenceMetaColumn
{
	SEQ_COLUMN_ID = 1
	,SEQ_COLUMN_NAME
	,SEQ_COLUMN_VALUE
	,SEQ_COLUMN_EXISTRANGE
	,SEQ_COLUMN_MINVALUE
	,SEQ_COLUMN_MAXVALUE
	,SEQ_COLUMN_FLAGS
	,SEQ_COLUMN_MAX
};

static void SequenceMetaSplit(RangeData& range, const char *str, int col, size_t len = 0)
{
    switch (col)
    {
	case SEQ_COLUMN_ID:
		range.start = offsetof(Form_meta_sequence_data, seqId);
		range.len = sizeof(Oid);
		break;
	case SEQ_COLUMN_NAME:
		range.start = offsetof(Form_meta_sequence_data, seqName);
		range.len = sizeof(NameData);
		break;
	case SEQ_COLUMN_VALUE:
		range.start = offsetof(Form_meta_sequence_data, value);
		range.len = sizeof(int64);
		break;
	case SEQ_COLUMN_EXISTRANGE:
		range.start = offsetof(Form_meta_sequence_data, hasRange);
		range.len = sizeof(uint32);
		break;
	case SEQ_COLUMN_MINVALUE:
		range.start = offsetof(Form_meta_sequence_data, minValue);
		range.len = sizeof(int64);
		break;
	case SEQ_COLUMN_MAXVALUE:
		range.start = offsetof(Form_meta_sequence_data, maxValue);
		range.len = sizeof(int64);
		break;
	case SEQ_COLUMN_FLAGS:
		range.start = offsetof(Form_meta_sequence_data, flags);
		range.len = sizeof(uint32);
		break;
    }
}

static void SequenceNameMetaSplit(RangeData& range, const char *str, int col, size_t len = 0)
{
	if (1 == col)
	{
		range.start = 0;
		//range.len = len;
		Assert(len == sizeof(NameData));
		range.len = sizeof(NameData);
	}
}

static void SequenceIdMetaSplit(RangeData& range, const char *str, int col, size_t len = 0)
{
	if (1 == col)
	{
		range.start = 0;
		Assert(len == sizeof(Oid));
		range.len = sizeof(Oid);
	}
}

void InitSequenceColiId( void )
{
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = SEQ_COLUMN_MAX - 1;
	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;
	colinfo->split_function = SequenceMetaSplit;
	setColInfo(SequenceRelId,colinfo);

	colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc0(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = SEQ_COLUMN_ID;
	colinfo->rd_comfunction = (CompareCallback*)palloc0(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = int4_compare;
	colinfo->split_function = SequenceIdMetaSplit;
	setColInfo(SequenceIdIdxId,colinfo);

	
	colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc0(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = SEQ_COLUMN_NAME;
	colinfo->rd_comfunction = (CompareCallback*)palloc0(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = char_compare;
	colinfo->split_function = SequenceNameMetaSplit;
	setColInfo(SequenceNameIdxId,colinfo);

	MemoryContextSwitchTo(ocxt);
}

static inline int GetSequenceColiId( void )
{
	return SequenceRelId;
}

static inline int GetSequenceIdIndexColiId( void )
{
	return SequenceIdIdxId;
}

static inline int GetSequenceNameIndexColiId( void )
{
	return SequenceNameIdxId;
}

static void CreateSequenceTable4Database(Oid dbid, Oid defaultTablespace,
	Oid& seqRelId, Oid& seqIdIdxId, Oid& seqNameIdxId)
{
	Oid createArgSeqRelid = InvalidOid;
	Oid createArgSeqIdIdxId = InvalidOid;
	Oid createArgSeqNameIdxId = InvalidOid;

	if(dbid == DEFAULTDATABASE_OID)
	{
		createArgSeqRelid = SequenceRelId;
		createArgSeqIdIdxId = SequenceIdIdxId;
		createArgSeqNameIdxId = SequenceNameIdxId;
	}

	Relation seqRel = NULL;

	PG_TRY();
	{
		seqRelId = fxdb_heap_create(createArgSeqRelid, GetSequenceColiId(), defaultTablespace, dbid);
		seqRel = heap_open(seqRelId, AccessExclusiveLock);
		seqIdIdxId = fxdb_index_create(createArgSeqIdIdxId, seqRel, BTREE_UNIQUE_TYPE, GetSequenceIdIndexColiId());
		seqNameIdxId = fxdb_index_create(createArgSeqNameIdxId, seqRel, BTREE_UNIQUE_TYPE, GetSequenceNameIndexColiId());
		heap_close(seqRel, NoLock);
	}
	PG_CATCH(); 
	{ 
		if(seqRel != NULL) { heap_close(seqRel, NoLock); }
		PG_RE_THROW();
	} 
	PG_END_TRY();

	return;
}

static void DropSequenceTable4Database(Oid dbid, Oid seqRelId,
	Oid seqIdIdxId, Oid seqNameIdxId)
{
	fdxdb_heap_drop(seqRelId, dbid);
}

/*
* CreateDatabase  ����һ���µ���ݿ�?
*/
Oid CreateDatabase(TransactionId xid
					, const char *datname
					, const char *defaultTblspcName
					, const void*extraData
					, size_t extraDataLen
					, Oid dbid) 
{
	Assert(datname != NULL && strlen(datname) > 0);

	CheckStandbyUpdateError("Can not execute create database during recovery.");

    CheckNameParma(datname);

	Form_meta_database metaData = GetDatabaseMetaData(datname);
	if(NULL != metaData)
	{
		pfree(metaData);
		ereport(ERROR,
			(errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("There is already a database named %s. please select another name",datname)));
	}

	Oid defaultTablespace = get_tablespace_oid(defaultTblspcName,true);

	if (!OidIsValid(defaultTablespace))
	{
		ereport(ERROR,
			(errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("There is no tablespace named %s.",defaultTblspcName)));
	}

	char* szLocation = get_tablespace_location(defaultTblspcName);
    struct stat statbuf;
	if (NULL == szLocation || lstat(szLocation, &statbuf) != 0)
	{
		if(NULL != szLocation)
		{
			pfree(szLocation);
		}
		ereport(ERROR,
			(errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("The tablespace dir %s does not exists.",defaultTblspcName)));
	}
	else
	{
		pfree((szLocation));
	}


	/*
	* Force a checkpoint before starting the copy. This will force dirty
	* buffers out to disk, to ensure source database is up-to-date on disk
	* for the copy. FlushDatabaseBuffers() would suffice for that, but we
	* also want to process any pending unlink requests. Otherwise, if a
	* checkpoint happened while we're copying files, a file might be deleted
	* just when we're about to copy it, causing the lstat() call in copydir()
	* to fail with ENOENT.
	*/
	RequestCheckpoint(CHECKPOINT_IMMEDIATE | CHECKPOINT_FORCE | CHECKPOINT_WAIT);

	Relation rel = heap_open(DatabaseRelationId, AccessShareLock);

	bool flag = false;
	PG_TRY();
	{
		if (0 == dbid)

			do 
			{
				dbid = GetNewObjectId();
			} while (dbid == DEFAULTDATABASE_OID);

		Oid seqRelId = InvalidOid;
		Oid seqIdIdxId = InvalidOid;
		Oid seqNameIdxId = InvalidOid;
	    CreateSequenceTable4Database(dbid, defaultTablespace, seqRelId, seqIdIdxId, seqNameIdxId);

	    HeapTuple tuple = FormatDBMetaTuple(extraData, extraDataLen, dbid,
	    					datname, xid, defaultTablespace,
	    					seqRelId, seqIdIdxId, seqNameIdxId);

	    simple_heap_insert(rel,tuple);
	    //heap_insert(rel, tuple, GetCurrentCommandId(true), 0, NULL);//insert metadata
	    heap_close(rel, AccessShareLock);
	    heap_freetuple(tuple);
	}
	PG_CATCH(); 
	{ 
		flag=true;
	} 
	PG_END_TRY();
	if (flag)
	{
		heap_close(rel, AccessShareLock);
		PG_RE_THROW();
	}

	/*
	* Force synchronous commit, thus minimizing the window between
	* creation of the database files and commital of the transaction. If
	* we crash before committing, we'll have a DB that's taking up disk
	* space but is not in pg_database, which is not good.
	*/
	ForceSyncCommit();

	if (dbid != DEFAULTDATABASE_OID)
	{
		/* create largeobject rel */
		CreateMetaLORel(DEFAULTTABLESPACE_OID, dbid);
		CommandCounterIncrement();
		DatabaseCreateLORel(dbid);
		DestoryLORelList();
		DestoryDBInfo();
	}

	return dbid;
}

void DropDatabase(const char *dbname)
{
	CheckStandbyUpdateError("Can not execute drop database during recovery.");

	CheckNameParma(dbname);
	Relation rel = heap_open(DatabaseRelationId, AccessShareLock);
	if(NULL != rel)
	{
		NameData name;
		memset(&name,0,sizeof(NameData));
		memcpy(name.data,dbname,strlen(dbname)+1);
		Datum datumName = fdxdb_string_formdatum(name.data,sizeof(name));
		ScanKeyData entry;
		Fdxdb_ScanKeyInitWithCallbackInfo(&entry
			,COLUMN_NAME
			,BTEqualStrategyNumber
			,char_compare
			,datumName);

		HeapScanDesc  scandesc = heap_beginscan(rel, SnapshotNow, 1, &entry, 0, 0);
		HeapTuple tuple = heap_getnext(scandesc, ForwardScanDirection);
		/* We assume that there can be at most one matching tuple */
		if (HeapTupleIsValid(tuple))
		{
			int			notherbackends;
			int			npreparedxacts;
			Form_meta_database metaData = Tuple2FormDatabaseData(tuple);

			/* Obviously can't drop my own database */
			if (metaData->db_id == MyDatabaseId)
				ereport(ERROR,
				(errcode(ERRCODE_OBJECT_IN_USE),
				errmsg("cannot drop the currently open database")));

			/*
			* Check for other backends in the target database.  (Because we hold the
			* database lock, no new ones can start after this.)
			*
			* As in CREATE DATABASE, check this after other error conditions.
			*/
			if (CountOtherDBBackends(metaData->db_id, &notherbackends, &npreparedxacts))
				ereport(ERROR,
				(errcode(ERRCODE_OBJECT_IN_USE),
				errmsg("database \"%s\" is being accessed by other users",
				dbname)));

			/*
			* Drop pages for this database that are in the shared buffer cache. This
			* is important to ensure that no remaining backend tries to write out a
			* dirty buffer to the dead database later...
			*/
			DropDatabaseBuffers(metaData->db_id);

			/*
			* Tell the stats collector to forget it immediately, too.
			*/
			pgstat_drop_database(metaData->db_id);

			/*
			* Tell bgwriter to forget any pending fsync and unlink requests for files
			* in the database; else the fsyncs will fail at next checkpoint, or
			* worse, it will delete files that belong to a newly created database
			* with the same OID.
			*/
			ForgetDatabaseFsyncRequests(metaData->db_id);

			/*
			* Force a checkpoint to make sure the bgwriter has received the message
			* sent by ForgetDatabaseFsyncRequests. On Windows, this also ensures that
			* the bgwriter doesn't hold any open files, which would cause rmdir() to
			* fail.
			*/
			RequestCheckpoint(CHECKPOINT_IMMEDIATE | CHECKPOINT_FORCE | CHECKPOINT_WAIT);

		
			List *pTabs = fxdb_get_all_table_in_db(metaData->db_id);
			
			if (NULL != pTabs)
			{
				int len = list_length(pTabs);
				Oid *tablespaceIds = (Oid *)palloc0(sizeof(Oid) * len);
				int idx = 0;
				ListCell *cell = NULL;
				foreach(cell,pTabs)
				{
					Oid relid = ((RelFileNode*)lfirst(cell))->relNode;
					Oid tbspace = ((RelFileNode*)lfirst(cell))->spcNode;
					if (!is_toast_id(relid))
					{
						fdxdb_heap_drop(relid,metaData->db_id);
						tablespaceIds[idx++] = tbspace;
					}
				}	

				qsort(tablespaceIds,idx,sizeof(Oid),OidCompare);
				Oid prevTablespace = InvalidOid;
				for (int i = 0; i < idx; ++i)
				{
					if (prevTablespace != tablespaceIds[i])
					{
						prevTablespace = tablespaceIds[i];
						char* dstpath = GetDatabasePath(metaData->db_id,prevTablespace);
                        struct stat st;
						if (lstat(dstpath, &st) < 0 || !S_ISDIR(st.st_mode))
						{
							/* Assume we can ignore it */
							pfree(dstpath);
							continue;
						}

						if (!rmtree(dstpath, true))
							ereport(WARNING,
							(errmsg("some useless files may be left behind in old database directory \"%s\"",
							dstpath)));

					}
				}
				pfree(tablespaceIds);
				list_free_deep(pTabs);
			}

     		simple_heap_delete(rel,&tuple->t_self);
				/* Delete large object metatable */
				pfree(GetDatabaseLOMetaTable(metaData->db_id, true));
				DestoryLORelList();
				DestoryDBInfo();

			/* drop sequence relation */
			//DropSequenceTable4Database(metaData->db_id, metaData->seqRelId,
			//	metaData->seqIdIdxId, metaData->seqNameIdxId);
		}
		else
		{
			elog(WARNING,"there is no database named %s",dbname);
		}
		heap_endscan(scandesc);
		heap_close(rel, AccessShareLock);

		/*
		* Force synchronous commit, thus minimizing the window between removal of
		* the database files and commital of the transaction. If we crash before
		* committing, we'll have a DB that's gone on disk but still there
		* according to pg_database, which is not good.
		*/
		ForceSyncCommit();
	}
	else
	{
		ereport(ERROR,
                (errcode(ERRCODE_HEAP_OPEN_WITH_OID_FAILURE),
                errmsg("could not open database meta table")));
	}
}

Oid GetDatabaseOid(const char* dbname)
{
    Assert(dbname != NULL && strlen(dbname) > 0);
    CheckNameParma(dbname);

	Form_meta_database dbMetaData = GetDatabaseMetaData(dbname);
	if (NULL != dbMetaData)
	{
		Oid result = dbMetaData->db_id;
		pfree(dbMetaData);
		return result;
	}

	return InvalidOid;
}

Form_meta_database GetDatabaseMetaDataByID(Oid db_id, ItemPointer it)
{
	Relation rel = heap_open(DatabaseRelationId, AccessShareLock);

	if(NULL != rel)
	{
		Datum datumID = fdxdb_string_formdatum((const char *)&db_id, sizeof(db_id));
		ScanKeyData entry;
		Fdxdb_ScanKeyInitWithCallbackInfo(&entry
			,COLUMN_ID
			,BTEqualStrategyNumber
			,int4_compare
			,datumID);

		HeapScanDesc  scandesc = heap_beginscan(rel, SnapshotNow, 1, &entry, 0, 0);
		HeapTuple tuple = heap_getnext(scandesc, ForwardScanDirection);
		/* We assume that there can be at most one matching tuple */

		Form_meta_database metaData = NULL;
		if (HeapTupleIsValid(tuple))
		{
			if (PointerIsValid(it))
			{
				ItemPointerCopy(&tuple->t_self,it);
			}
		
			{
				Form_meta_database info = Tuple2FormDatabaseData(tuple);
				metaData = (Form_meta_database)palloc0(MAX_FORM_DBMETA_SIZE);
				CopyFormDBMetaData(metaData,info);
			}
		}
		else
		{
			metaData = NULL;
			if (PointerIsValid(it))
			{
				ItemPointerSetInvalid(it);
			}
			//elog(WARNING,"there is no database named %s",dbname);
		}
		heap_endscan(scandesc);

		heap_close(rel, AccessShareLock);

		return metaData;
	}
	else
	{
		ereport(ERROR,
                (errcode(ERRCODE_HEAP_OPEN_WITH_OID_FAILURE),
                errmsg("could not open database meta table")));
	}

	return NULL;
}

Form_meta_database GetDatabaseMetaData(const char* dbname,ItemPointer it)
{
	CheckNameParma(dbname);
	Relation rel = heap_open(DatabaseRelationId, AccessShareLock);
	if(NULL != rel)
	{
		NameData name;
		memset(&name,0,sizeof(NameData));
		memcpy(name.data,dbname,strlen(dbname) + 1);
		Datum datumName = fdxdb_string_formdatum(name.data,sizeof(NameData));
		ScanKeyData entry;
		Fdxdb_ScanKeyInitWithCallbackInfo(&entry
			,COLUMN_NAME
			,BTEqualStrategyNumber
			,char_compare
			,datumName);

		HeapScanDesc  scandesc = heap_beginscan(rel, SnapshotNow, 1, &entry, 0, 0);
		HeapTuple tuple = heap_getnext(scandesc, ForwardScanDirection);
		/* We assume that there can be at most one matching tuple */

		Form_meta_database metaData = NULL;
		if (HeapTupleIsValid(tuple))
		{
			if (PointerIsValid(it))
			{
				ItemPointerCopy(&tuple->t_self,it);
			}
		
			{
				Form_meta_database info = Tuple2FormDatabaseData(tuple);
				metaData = (Form_meta_database)palloc0(MAX_FORM_DBMETA_SIZE);
				CopyFormDBMetaData(metaData,info);
			}
		}
		else
		{
			metaData = NULL;
			if (PointerIsValid(it))
			{
				ItemPointerSetInvalid(it);
			}
			//elog(WARNING,"there is no database named %s",dbname);
		}
		heap_endscan(scandesc);

		heap_close(rel, AccessShareLock);

		return metaData;
	}
	else
	{
		ereport(ERROR,
                (errcode(ERRCODE_HEAP_OPEN_WITH_OID_FAILURE),
                errmsg("could not open database meta table")));
	}

	return NULL;
}

void UpdateExtraData(const char* dbname,const void* data,size_t len)
{
	CheckNameParma(dbname);
	ItemPointerData itTuple;
	Form_meta_database dbInfo = NULL;
	dbInfo = GetDatabaseMetaData(dbname,&itTuple);
    if (NULL != dbInfo && ItemPointerIsValid(&itTuple))
    {
		HeapTuple tup_update = FormatDBMetaTuple(data,len,dbInfo->db_id,dbname,
								dbInfo->xid,dbInfo->defTablespace,
								dbInfo->seqRelId,dbInfo->seqIdIdxId,dbInfo->seqNameIdxId);

		Relation rel = heap_open(DatabaseRelationId, AccessShareLock);
		simple_heap_update(rel, &itTuple, tup_update);
		heap_close(rel,AccessShareLock);
		pfree(dbInfo);
		CommandCounterIncrement();
    }
}

/*
 * Inplace update the xid of database(db->xid)
 *
 * If db->xid is older than xid, then update the db->xid
 * If not, do nothing
 */
void InplaceUpdateXid(Oid db_id, TransactionId xid, bool *isUpdated)
{
	Relation rel = heap_open(DatabaseRelationId, RowExclusiveLock);
	HeapTuple tup_update = NULL;

	*isUpdated = false;

	/* get db update tuple */
	{
		Relation dbid_index = index_open(DatabaseUniqueId, AccessShareLock);

		Datum datumID = fdxdb_string_formdatum((const char *)&db_id, sizeof(db_id));
		ScanKeyData key;
		Fdxdb_ScanKeyInitWithCallbackInfo(&key, 1, BTEqualStrategyNumber,
										int4_compare, datumID);

		IndexScanDesc idx_scan = index_beginscan(rel, dbid_index, SnapshotNow,1, 0, 0, 0);
		index_rescan(idx_scan, &key, 1, NULL, 0);
		HeapTuple tuple = index_getnext(idx_scan, ForwardScanDirection);
		Assert(HeapTupleIsValid(tuple));
		Form_meta_database info = Tuple2FormDatabaseData(tuple);

		if(TransactionIdPrecedes(info->xid, xid))
		{
			tup_update = heap_copytuple(tuple);
		}
		index_endscan(idx_scan);
		index_close(dbid_index, AccessShareLock);
	}

	if(tup_update != NULL)
	{
		Form_meta_database info = Tuple2FormDatabaseData(tup_update);
		info->xid = xid;
		heap_inplace_update(rel, tup_update);

		heap_freetuple(tup_update);

		*isUpdated = true;
	}
	heap_close(rel, RowExclusiveLock);
}

Form_meta_database GetAllDatabaseInfos(size_t& nCnt, bool useXactMemory)
{
	Relation rel = heap_open(DatabaseRelationId,AccessShareLock);

	HeapScanDesc dbScanDes = heap_beginscan(rel,SnapshotNow,0,NULL, 0, 0);

	heap_rescan(dbScanDes, NULL);

	HeapTuple tuple = NULL;

	nCnt = 0;
	int nCurCnt = 16;
	Form_meta_database pOids = NULL;
	if(useXactMemory)
	{
		pOids = (Form_meta_database)palloc0(nCurCnt * MAX_FORM_DBMETA_SIZE);
	}
	else
	{
		pOids = (Form_meta_database)malloc(nCurCnt * MAX_FORM_DBMETA_SIZE);
	}

	while(NULL != (tuple = heap_getnext(dbScanDes, BackwardScanDirection)))
	{
        if (nCnt == (size_t)nCurCnt)
        {
			nCurCnt *= 2;
			if(useXactMemory)
			{
				pOids = (Form_meta_database)repalloc(pOids,nCurCnt * MAX_FORM_DBMETA_SIZE);
			}
			else
			{
				pOids = (Form_meta_database)realloc(pOids,nCurCnt * MAX_FORM_DBMETA_SIZE);
			}
        }

		Form_meta_database info = Tuple2FormDatabaseData(tuple);
		CopyFormDBMetaData((Form_meta_database)((char*)pOids + nCnt * MAX_FORM_DBMETA_SIZE),info);
		++nCnt;
	}

	heap_endscan(dbScanDes);
	heap_close(rel,AccessShareLock);

	return pOids;
}

Oid SetCurrentDatabase(const char* dbname,Oid dbId,Oid tablespaceId)
{
	if((dbname != NULL) && (dbId != InvalidOid) && (tablespaceId != InvalidOid))
	{
		MyDatabaseId = dbId;
		MyDatabaseTableSpace = tablespaceId;
		strcpy(CurrentDatabaseName.data, dbname);
		return MyDatabaseId;
	}
	else
	{
		Form_meta_database dbMetaData = NULL;
		if(dbname != NULL)
		{
			dbMetaData = GetDatabaseMetaData(dbname, NULL);
		}
		else if(dbId != InvalidOid)
		{
			dbMetaData = GetDatabaseMetaDataByID(dbId, NULL);
		}
		else
		{
			return InvalidOid;
		}

		if(dbMetaData != NULL)
		{
			MyDatabaseId = dbMetaData->db_id;
			MyDatabaseTableSpace = dbMetaData->defTablespace;
			strcpy(CurrentDatabaseName.data, dbMetaData->db_name.data);
			pfree(dbMetaData);
			return MyDatabaseId;
		}
		else
		{
			return InvalidOid;
		}
	}
}

Oid GetCurrentDatabase(const char*& dbname)
{
    dbname = CurrentDatabaseName.data;
	return MyDatabaseId;
}

Oid GetCurrentDatabaseTablespace( )
{ 
	return MyDatabaseTableSpace;
}

void InitDatabaseColiId( void )
{
	MemoryContext ocxt = MemoryContextSwitchTo(TopMemoryContext);

	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = COLUMN_MAX - 1;
	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;
	colinfo->split_function = DatabaseMetaSplit;

	setColInfo(DatabaseRelationId,colinfo);

	colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc0(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = COLUMN_ID;
	colinfo->rd_comfunction = (CompareCallback*)palloc0(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = int4_compare;
	colinfo->split_function = DatabaseIdMetaSplit;
	setColInfo(DatabaseUniqueId,colinfo);

	
	colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc0(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = COLUMN_NAME;
	colinfo->rd_comfunction = (CompareCallback*)palloc0(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = char_compare;
	colinfo->split_function = DatabaseNameMetaSplit;

	setColInfo(DatabaseNameIdxId,colinfo);
	MemoryContextSwitchTo(ocxt);
}

int GetDatabaseColiId( void )
{
	return DatabaseRelationId;
}

bool IsSomeOnesDefaultTablespace(const char* tablespaceName)
{
	Relation rel = heap_open(DatabaseRelationId, AccessShareLock);
	if(NULL != rel)
	{
		NameData name;
		memset(&name,0,sizeof(name));
		Datum datumName = fdxdb_string_formdatum(name.data,sizeof(name));
		ScanKeyData entry;
		Fdxdb_ScanKeyInitWithCallbackInfo(&entry
			,COLUMN_DEFAULTTABLESPACE
			,BTEqualStrategyNumber
			,char_compare
			,datumName);

		HeapScanDesc  scandesc = heap_beginscan(rel, SnapshotNow, 1, &entry, 0, 0);
		HeapTuple tuple = heap_getnext(scandesc, ForwardScanDirection);
		/* We assume that there can be at most one matching tuple */


		heap_endscan(scandesc);

		heap_close(rel, AccessShareLock);

		return HeapTupleIsValid(tuple);
	}
	else
	{
		ereport(ERROR,
                (errcode(ERRCODE_HEAP_OPEN_WITH_OID_FAILURE),
                errmsg("could not open database meta table")));
	}

	return false;
}

static 
void init_dbinfo_hash(bool isInXactBlock)
{
	HASHCTL		ctl;

	DBInfoContext = AllocSetContextCreate(ProcessTopMemoryContext,
		"DBInfo Context",
		ALLOCSET_SMALL_MINSIZE,
		ALLOCSET_SMALL_INITSIZE,
		ALLOCSET_SMALL_MAXSIZE);

	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(Oid);
	ctl.entrysize = sizeof(DBInfo);

	/* hash style Oid  */
	ctl.hash = oid_hash;
	ctl.hcxt = DBInfoContext;
	DBInfoHash = hash_create("DBInfo hash", 10, &ctl, HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);

	if (!isInXactBlock)
	{
		StartTransactionCommand();
		BeginTransactionBlock();
		CommitTransactionCommand();
	}

	Relation rel = heap_open(DatabaseRelationId, AccessShareLock);
	HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);
	HeapTuple tuple = NULL;
	while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		Form_meta_database metaData = Tuple2FormDatabaseData(tuple);
		DBInfo *p_dbinfo = (DBInfo *)hash_search(DBInfoHash, &metaData->db_id, HASH_ENTER, NULL);
		p_dbinfo->dbid = metaData->db_id;
		p_dbinfo->dbname = metaData->db_name;
		p_dbinfo->seqRelId = metaData->seqRelId;
		p_dbinfo->seqIdIdxId = metaData->seqIdIdxId;
		p_dbinfo->seqNameIdxId = metaData->seqNameIdxId;
	}
	heap_endscan(scan);
	heap_close(rel, AccessShareLock);

	if (!isInXactBlock)
	{
		StartTransactionCommand();
		EndTransactionBlock();
		CommitTransactionCommand();
	}
}

HASH_SEQ_STATUS *GetDBInfoHashStatus()
{
	HASH_SEQ_STATUS *hstat = (HASH_SEQ_STATUS *)palloc0(sizeof(HASH_SEQ_STATUS));
	hash_seq_init(hstat, DBInfoHash);
	return hstat;
}

DBInfo *GetDBInfo(Oid dbid)
{	
	LWLockAcquire(DBInfoLock, LW_SHARED);
	DBInfo *info = (DBInfo *)hash_search(DBInfoHash, &dbid, HASH_FIND, NULL);
	LWLockRelease(DBInfoLock);

	if (info == NULL)
	{
		Form_meta_database formClass = 
			GetDatabaseMetaDataByID(dbid, NULL);
		if (formClass != NULL)
		{
			bool found = false;
			LWLockAcquire(DBInfoLock, LW_EXCLUSIVE);
			info = (DBInfo *)hash_search(DBInfoHash, &dbid, HASH_ENTER, &found);

			if (!found)
			{
				info->dbid = formClass->db_id;
				info->dbname = formClass->db_name;
				info->seqIdIdxId = formClass->seqIdIdxId;
				info->seqNameIdxId = formClass->seqNameIdxId;
				info->seqRelId = formClass->seqRelId;
				info->next_loid = get_last_loid(dbid, false) + 1;
				SpinLockInit(&info->loid_lock);
			}
			LWLockRelease(DBInfoLock);

			pfree(formClass);
		}
	}
	return info;
}

void DestoryDBInfo()
{
	LWLockAcquire(DBInfoLock, LW_EXCLUSIVE);
	if (DBInfoContext != NULL)
		MemoryContextDelete(DBInfoContext);
	DBInfoContext = NULL;
	DBInfoHash = NULL;
	LWLockRelease(DBInfoLock);
}

void RebuildDBInfo(bool isInXactBlock)
{
	if (DBInfoContext != NULL)
		return;

	init_dbinfo_hash(isInXactBlock);
	InitLOInfo(isInXactBlock);
}

char *get_database_name(Oid dbid)
{
    static THREAD_LOCAL char dbName[NAMEDATALEN];

	DBInfo *info = GetDBInfo(dbid);
	if(info != NULL)
	{
		memcpy(dbName, info->dbname.data, NAMEDATALEN);
    	return dbName;
	}
    return NULL;
}



/* some operations on sequence table */
void GetSeqTableByDbid(Oid dbid, Oid& seqRelId,
	Oid& seqIdIdxId, Oid& seqNameIdxId)
{
	/*if (DBInfoContext == NULL)
	{
		LWLockAcquire(DBInfoLock, LW_EXCLUSIVE);
		RebuildDBInfo(true);
		LWLockRelease(DBInfoLock);
	}*/

	//DBInfo *pDbInfo = GetDBInfo(dbid);
	Form_meta_database formClass = 
			GetDatabaseMetaDataByID(dbid, NULL);

	if(formClass != NULL){
		seqRelId = formClass->seqRelId;
		seqIdIdxId = formClass->seqIdIdxId;
		seqNameIdxId = formClass->seqNameIdxId;
		pfree(formClass);
	}else{
		seqRelId = InvalidOid;
		seqIdIdxId = InvalidOid;
		seqNameIdxId = InvalidOid;
	}
}

void GetCurrSeqTable(Oid& seqRelId, Oid& seqIdIdxId, Oid& seqNameIdxId)
{
	GetSeqTableByDbid(MyDatabaseId, seqRelId, seqIdIdxId, seqNameIdxId);

	if((seqRelId == InvalidOid) || (seqIdIdxId == InvalidOid)
	||(seqNameIdxId == InvalidOid))
	{
		Assert(false);
	}
}

static HeapTuple FormSequenceMetaTuple(Oid seqId, const char *seqName,
	int64 value, bool hasRange, int64 minValue, int64 maxValue, uint32 flags)
{
	Form_meta_sequence_data seqData;

	seqData.seqId = seqId;
	memset((char *)&seqData.seqName, 0, sizeof(seqData.seqName));
	strcpy(seqData.seqName.data, seqName);
	seqData.value = value;
	seqData.hasRange = (hasRange ? 1 : 0);
	seqData.minValue = minValue;
	seqData.maxValue = maxValue;
	seqData.flags = flags;
	
	return fdxdb_heap_formtuple((char*)&seqData, sizeof(Form_meta_sequence_data));
}

Form_meta_sequence __GetSequenceMetaDataByID(Relation seqRel,
	Relation seqIdIdxRel, Oid seqId, ItemPointer it)
{
	IndexScanDesc index_scan = NULL;
	Form_meta_sequence pSeqData = NULL;

	if (PointerIsValid(it))
	{
		ItemPointerSetInvalid(it);
	}

	PG_TRY();
	{
		Datum datumID = fdxdb_string_formdatum((const char *)&seqId, sizeof(seqId));
		ScanKeyData keys;
		Fdxdb_ScanKeyInitWithCallbackInfo(&keys
			,1 //SEQ_COLUMN_ID
			,BTEqualStrategyNumber
			,int4_compare
			,datumID);
		index_scan = index_beginscan(seqRel, seqIdIdxRel, SnapshotNow, 1, 0, 0, 0);
		index_rescan(index_scan, &keys, 1, NULL, 0);

		HeapTuple tuple = index_getnext(index_scan, ForwardScanDirection);
		if(HeapTupleIsValid(tuple))
		{
			if (PointerIsValid(it))
			{
				ItemPointerCopy(&tuple->t_self, it);
			}

			int len = 0;
			pSeqData = (Form_meta_sequence)fdxdb_tuple_to_chars_with_len(tuple, len);
			Assert(len == sizeof(Form_meta_sequence_data));
		}

		index_endscan(index_scan);
	}
	PG_CATCH(); 
	{ 
		if(index_scan != NULL) { index_endscan(index_scan); }
		PG_RE_THROW();
	} 
	PG_END_TRY();

	if(pSeqData == NULL)
	{
		ereport(ERROR,
                (errcode(ERRCODE_ACCESS_OBJECT_NOT_EXIST),
                errmsg("could not find sequence %d", seqId)));
	}

	return pSeqData;
}

Form_meta_sequence __GetSequenceMetaData(Relation seqRel,
	Relation seqNameIdxRel, const char *seqName, ItemPointer it)
{
	IndexScanDesc index_scan = NULL;
	Form_meta_sequence pSeqData = NULL;

	if (PointerIsValid(it))
	{
		ItemPointerSetInvalid(it);
	}

	PG_TRY();
	{
		NameData nameData;
		memset((char *)&nameData, 0, sizeof(NameData));
		strcpy(nameData.data, seqName);
		Datum datumName = fdxdb_string_formdatum((char *)&nameData, sizeof(NameData));
		ScanKeyData keys;
		Fdxdb_ScanKeyInitWithCallbackInfo(&keys
			,1 //SEQ_COLUMN_NAME
			,BTEqualStrategyNumber
			,char_compare
			,datumName);
		index_scan = index_beginscan(seqRel, seqNameIdxRel, SnapshotNow, 1, 0, 0, 0);
		index_rescan(index_scan, &keys, 1, NULL, 0);

		HeapTuple tuple = index_getnext(index_scan, ForwardScanDirection);
		if(HeapTupleIsValid(tuple))
		{
			if (PointerIsValid(it))
			{
				ItemPointerCopy(&tuple->t_self, it);
			}

			int len = 0;
			pSeqData = (Form_meta_sequence)fdxdb_tuple_to_chars_with_len(tuple, len);
			Assert(len == sizeof(Form_meta_sequence_data));
		}

		index_endscan(index_scan);
	}
	PG_CATCH(); 
	{ 
		if(index_scan != NULL){ index_endscan(index_scan); }
		PG_RE_THROW();
	} 
	PG_END_TRY();

	if(pSeqData == NULL)
	{
		ereport(ERROR,
                (errcode(ERRCODE_ACCESS_OBJECT_NOT_EXIST),
                errmsg("could not find sequence %s", seqName)));
	}

	return pSeqData;
}

Form_meta_sequence GetSequenceMetaDataByID(Oid seqRelId,
	Oid seqIdIdxId, Oid seqId, ItemPointer it)
{
	Relation seqRel = NULL;
	Relation seqIdIdxRel = NULL;
	Form_meta_sequence pSeqData = NULL;

	PG_TRY();
	{
		seqRel = heap_open(seqRelId, AccessShareLock);
		seqIdIdxRel = index_open(seqIdIdxId, AccessShareLock);

		pSeqData = __GetSequenceMetaDataByID(seqRel, seqIdIdxRel, seqId, it);

		index_close(seqIdIdxRel, NoLock);
		heap_close(seqRel, NoLock);
	}
	PG_CATCH(); 
	{ 
		if(seqIdIdxRel != NULL) { index_close(seqIdIdxRel, NoLock); }
		if(seqRel != NULL) { heap_close(seqRel, NoLock); }
		PG_RE_THROW();
	} 
	PG_END_TRY();

	return pSeqData;
}

Form_meta_sequence GetSequenceMetaData(Oid seqRelId,
	Oid seqNameIdxId, const char *seqName, ItemPointer it)
{
	Relation seqRel = NULL;
	Relation seqNameIdxRel = NULL;
	Form_meta_sequence pSeqData = NULL;

	PG_TRY();
	{
		seqRel = heap_open(seqRelId, AccessShareLock);
		seqNameIdxRel = index_open(seqNameIdxId, AccessShareLock);

		pSeqData = __GetSequenceMetaData(seqRel, seqNameIdxRel, seqName, it);

		index_close(seqNameIdxRel, NoLock);
		heap_close(seqRel, NoLock);
	}
	PG_CATCH(); 
	{ 
		if(seqNameIdxRel != NULL) { index_close(seqNameIdxRel, NoLock); }
		if(seqRel != NULL) { heap_close(seqRel, NoLock); }
		PG_RE_THROW();
	} 
	PG_END_TRY();

	return pSeqData;
}

void InsertSeqInfo(Oid seqRelId, Oid seqId, const char *seqName, int64 value,
	bool hasRange, int64 minValue, int64 maxValue, uint32 flags)
{
	HeapTuple tuple = NULL;
	Relation seqRel = NULL;

	PG_TRY();
	{
		tuple = FormSequenceMetaTuple(seqId, seqName, value, hasRange,
							minValue, maxValue, flags);
		seqRel = heap_open(seqRelId, RowExclusiveLock);

		simple_heap_insert(seqRel, tuple);
		heap_close(seqRel, NoLock);
		heap_freetuple(tuple);
		CommandCounterIncrement();
	}
	PG_CATCH(); 
	{ 
		if(seqRel != NULL){ heap_close(seqRel, NoLock); }
		if(tuple != NULL){ heap_freetuple(tuple); }
		PG_RE_THROW();
	} 
	PG_END_TRY();
}

void DeleteSeqInfoById(Oid seqRelId, Oid seqIdIdxId, Oid seqId)
{
	ItemPointerData it;
	Form_meta_sequence pMetaData = NULL;

	pMetaData = GetSequenceMetaDataByID(seqRelId, seqIdIdxId, seqId, &it);
	Relation seqRel = NULL;
	seqRel = heap_open(seqRelId, RowExclusiveLock);
	simple_heap_delete(seqRel, &it);
	heap_close(seqRel, NoLock);
	CommandCounterIncrement();
	pfree(pMetaData);
}

void DeleteSeqInfo(Oid seqRelId, Oid seqNameIdxId, const char *seqName, Oid& seqId)
{
	ItemPointerData it;
	Form_meta_sequence pMetaData = NULL;

	pMetaData = GetSequenceMetaData(seqRelId, seqNameIdxId, seqName, &it);
	Relation seqRel = NULL;
	seqRel = heap_open(seqRelId, RowExclusiveLock);
	simple_heap_delete(seqRel, &it);
	heap_close(seqRel, NoLock);
	CommandCounterIncrement();

	seqId = pMetaData->seqId;
	pfree(pMetaData);
}

void GetMaxSeqId(Oid seqRelId, Oid seqIdIdxId, Oid& maxSeqId)
{
	LOCKMODE lockmode = AccessShareLock;
	Relation seqRel = NULL;
	Relation seqIdIdxRel = NULL;
	IndexScanDesc index_scan = NULL;

	PG_TRY();
	{
		seqRel = heap_open(seqRelId, lockmode);
		seqIdIdxRel = index_open(seqIdIdxId, lockmode);
		index_scan = index_beginscan(seqRel, seqIdIdxRel, SnapshotDirtyPtr, 0, 0, 0, 0);
		index_rescan(index_scan, NULL, 0, NULL, 0);

		HeapTuple tuple = index_getnext(index_scan, BackwardScanDirection);
		if(HeapTupleIsValid(tuple))
		{
			int len = 0;
			Form_meta_sequence pSeqData = (Form_meta_sequence)fdxdb_tuple_to_chars_with_len(tuple, len);
			Assert(len == sizeof(Form_meta_sequence_data));
			maxSeqId = pSeqData->seqId + 1;
			pfree(pSeqData);
		}else{
			maxSeqId = 1;
		}

		index_endscan(index_scan);
		index_close(seqIdIdxRel, NoLock);
		heap_close(seqRel, NoLock);
	}
	PG_CATCH(); 
	{ 
		if(index_scan != NULL){ index_endscan(index_scan); }
		if(seqIdIdxRel != NULL){ index_close(seqIdIdxRel, NoLock); }
		if(seqRel != NULL){ heap_close(seqRel, NoLock); }
		PG_RE_THROW();
	}
	PG_END_TRY();
}
void GetSeqInfoById(Oid seqRelId, Oid seqIdIdxId, Oid seqId, char **pSeqName, int64& value,
	bool& hasRange, int64& minValue, int64& maxValue, uint32& flags)
{
	Form_meta_sequence pMetaData = GetSequenceMetaDataByID(seqRelId, seqIdIdxId, seqId, NULL);

	uint32 nameLen = (uint32)strlen(pMetaData->seqName.data) + 1;
	*pSeqName = (char *)malloc(nameLen);
	memcpy(*pSeqName, pMetaData->seqName.data, nameLen);
	value = pMetaData->value;
	hasRange = (pMetaData->hasRange == 1);
	minValue = pMetaData->minValue;
	maxValue = pMetaData->maxValue;
	flags = pMetaData->flags;

	pfree(pMetaData);
}

void GetSeqInfo(Oid seqRelId, Oid seqNameIdxId, const char *seqName, Oid& seqId, int64& value,
	bool& hasRange, int64& minValue, int64& maxValue, uint32& flags)
{
	Form_meta_sequence pMetaData = GetSequenceMetaData(seqRelId, seqNameIdxId, seqName, NULL);

	seqId = pMetaData->seqId;
	value = pMetaData->value;
	hasRange = (pMetaData->hasRange == 1);
	minValue = pMetaData->minValue;
	maxValue = pMetaData->maxValue;
	flags = pMetaData->flags;

	pfree(pMetaData);
}

void GetSeqValueById(Oid seqRelId, Oid seqIdIdxId, Oid seqId, int64& value)
{
	Form_meta_sequence pMetaData = GetSequenceMetaDataByID(seqRelId, seqIdIdxId, seqId, NULL);

	value = pMetaData->value;

	pfree(pMetaData);
}

static void __UpdateSeqInfo(Relation seqRel, Form_meta_sequence pNewMetaData, ItemPointer it)
{
	HeapTuple tuple = NULL;

	if(pNewMetaData->hasRange)
	{
		if((pNewMetaData->value < pNewMetaData->minValue)
		|| (pNewMetaData->value > pNewMetaData->maxValue))
		{
			Assert(false);
		}
	}

	PG_TRY();
	{
		tuple = fdxdb_heap_formtuple((char *)pNewMetaData, sizeof(Form_meta_sequence_data));
		tuple->t_self = *it;

		heap_inplace_update(seqRel, tuple);
		CommandCounterIncrement();
		pfree(tuple);
	}
	PG_CATCH();
	{
		if(tuple != NULL) { pfree(tuple); }
		PG_RE_THROW();
	}
	PG_END_TRY();
}

void UpdateSeqValueById(Oid seqRelId, Oid seqIdIdxId,
			Oid seqId, bool isDeltaValue, int64 value, int64& oldValue)
{
	LOCKMODE lockmode = RowExclusiveLock;
	Relation seqRel = NULL;
	Relation seqIdIdxRel = NULL;
	ItemPointerData it;
	Form_meta_sequence pMetaData = NULL;

	PG_TRY();
	{
		seqRel = heap_open(seqRelId, lockmode);
		seqIdIdxRel = index_open(seqIdIdxId, lockmode);

		pMetaData = __GetSequenceMetaDataByID(seqRel, seqIdIdxRel, seqId, &it);
		oldValue = pMetaData->value;
		if(isDeltaValue)
		{
			pMetaData->value = oldValue + value;
		}
		else
		{
			pMetaData->value = value;
		}

		/* just for test concurrency update */
		//pg_sleep(1000);

		if(oldValue != pMetaData->value)
		{
			__UpdateSeqInfo(seqRel, pMetaData, &it);
		}

		index_close(seqIdIdxRel, NoLock);
		heap_close(seqRel, NoLock);
		pfree(pMetaData);
	}
	PG_CATCH(); 
	{ 
		if(seqIdIdxRel != NULL) { index_close(seqIdIdxRel, NoLock); }
		if(seqRel != NULL) { heap_close(seqRel, NoLock); }
		if(pMetaData != NULL) { pfree(pMetaData); }
		PG_RE_THROW();
	} 
	PG_END_TRY();
}

#if !defined(_MSC_VER)
#pragma GCC diagnostic warning "-Wclobbered"
#endif
