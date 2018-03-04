#include <iostream>

#include "utils/util.h"
#include "utils/tqual.h"
#include "catalog/pg_database.h"
#include "commands/tablespace.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/FDPGAdapter.h"

using std::cout;
using std::endl;
using namespace FounderXDB::StorageEngineNS;

static bool testdb_SequenceDataEqual(Form_meta_sequence pSeqData1,
	Form_meta_sequence pSeqData2)
{
	return ((pSeqData1->seqId == pSeqData2->seqId)
		&& (memcmp(pSeqData1->seqName.data, pSeqData2->seqName.data, sizeof(NameData)) == 0)
		&& (pSeqData1->value == pSeqData2->value)
		&& (pSeqData1->hasRange == pSeqData2->hasRange)
		&& (pSeqData1->minValue == pSeqData2->minValue)
		&& (pSeqData1->maxValue == pSeqData2->maxValue)
		&& (pSeqData1->flags == pSeqData2->flags));
}

static bool testdb_SequenceDataInsert(Oid seqRelId, Form_meta_sequence pSeqData)
{
	try
	{
		FDPG_Database::fd_InsertSeqInfo(seqRelId, pSeqData->seqId, pSeqData->seqName.data,
					pSeqData->value, (pSeqData->hasRange == 1), pSeqData->minValue,
					pSeqData->maxValue, pSeqData->flags);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}

	return true;
}

static bool testdb_QueryDataBySeqRel(Oid seqRelId, Form_meta_sequence pSeqData)
{
	Relation seqRel = NULL;
	HeapScanDesc scan = NULL;
	bool found = false;

	try{
		seqRel = FDPG_Heap::fd_heap_open(seqRelId, AccessShareLock);
		scan = FDPG_Heap::fd_heap_beginscan(seqRel, SnapshotNow, 0, NULL);
		HeapTuple tuple;
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			int len = 0;
	        Form_meta_sequence pTmpSeqData = (Form_meta_sequence)fdxdb_tuple_to_chars_with_len(tuple, len);
			if(len != sizeof(Form_meta_sequence_data))
			{
				pfree(pTmpSeqData);
				break;
			}
			if(testdb_SequenceDataEqual(pTmpSeqData, pSeqData))
			{	
				pfree(pTmpSeqData);
				found = true;
				break;
			}
	        pfree(pTmpSeqData);
		}
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		if(scan != NULL)
		{
			FDPG_Heap::fd_heap_endscan(scan);
		}
		if(seqRel != NULL)
		{
			FDPG_Heap::fd_heap_close(seqRel, AccessShareLock);
		}
		return false;
	}

	FDPG_Heap::fd_heap_endscan(scan);
	FDPG_Heap::fd_heap_close(seqRel, AccessShareLock);
	return found;
}

static bool testdb_QueryDataBySeqIdIdx(Oid seqRelId, Oid seqIdIdxId, Form_meta_sequence pSeqData)
{
	bool found = false;

	try
	{
		Form_meta_sequence_data seqData;
		seqData.seqId = pSeqData->seqId;
		bool hasRange;
		std::string seqName;
		FDPG_Database::fd_GetSeqInfoById(seqRelId, seqIdIdxId, seqData.seqId,
			seqName, seqData.value, hasRange, seqData.minValue,
			seqData.maxValue, seqData.flags);
		seqData.hasRange = (hasRange ? 1 : 0);
		memset((char *)&seqData.seqName, 0, sizeof(NameData));
		strcpy(seqData.seqName.data, seqName.c_str());

		found = testdb_SequenceDataEqual(&seqData, pSeqData);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}

	return found;
}

static bool testdb_QueryDataBySeqNameIdx(Oid seqRelId, Oid seqNameIdxId, Form_meta_sequence pSeqData)
{
	bool found = false;

	try
	{
		Form_meta_sequence_data seqData;
		memset((char *)&seqData.seqName, 0, sizeof(NameData));
		strcpy(seqData.seqName.data, pSeqData->seqName.data);
		bool hasRange;
		FDPG_Database::fd_GetSeqInfo(seqRelId, seqNameIdxId, seqData.seqName.data,
			seqData.seqId, seqData.value, hasRange, seqData.minValue,
			seqData.maxValue, seqData.flags);
		seqData.hasRange = (hasRange ? 1 : 0);

		found = testdb_SequenceDataEqual(&seqData, pSeqData);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}

	return found;
}

bool testdb_UpdateBySeqIdIdx(Oid seqRelId, Oid seqIdIdxId,
	Form_meta_sequence pSeqData, int64 newValue)
{
	try
	{
		int64 delta = newValue - pSeqData->value;
		int64 oldValue = pSeqData->value;
		int64 tmpOldValue;

		FDPG_Database::fd_UpdateSeqValueById(seqRelId, seqIdIdxId,
			pSeqData->seqId, true, delta, tmpOldValue);
		if(tmpOldValue != pSeqData->value)
		{
			return false;
		}

		pSeqData->value = newValue;
		if(!testdb_QueryDataBySeqIdIdx(seqRelId, seqIdIdxId, pSeqData)){
			return false;
		}

		FDPG_Database::fd_UpdateSeqValueById(seqRelId, seqIdIdxId,
			pSeqData->seqId, false, oldValue, tmpOldValue);
		if(tmpOldValue != newValue)
		{
			return false;
		}

		pSeqData->value = oldValue;
		if(!testdb_QueryDataBySeqIdIdx(seqRelId, seqIdIdxId, pSeqData)){
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}

	return true;
}

bool testdb_CheckMaxSeqId(Oid seqRelId, Oid seqIdIdxId, Oid maxSeqId)
{
	Oid curMaxSeqId;
	try
	{
		FDPG_Database::fd_GetMaxSeqId(seqRelId, seqIdIdxId, curMaxSeqId);
		cout << "curr max seq id: " << curMaxSeqId << endl;
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}

	return (curMaxSeqId == (maxSeqId + 1));
}

bool testdb_SequenceTable4ExistDb(Oid dbid)
{
	Oid seqRelId = InvalidOid;
	Oid seqIdIdxId = InvalidOid;
	Oid seqNameIdxId = InvalidOid;
	Oid maxSeqId;

	begin_transaction();
	try
	{
		GetSeqTableByDbid(dbid, seqRelId, seqIdIdxId, seqNameIdxId);
	}
	CATCHEXCEPTION
	commit_transaction();

	if((seqRelId == InvalidOid) || (seqIdIdxId == InvalidOid) || (seqNameIdxId == InvalidOid))
	{
		cout << "Not have sequence table for database" << dbid << endl;
		return false;
	}

	Form_meta_sequence_data seqData;

	begin_transaction();
	try
	{
		FDPG_Database::fd_GetMaxSeqId(seqRelId, seqIdIdxId, seqData.seqId);
	}
	CATCHEXCEPTION
	commit_transaction();
	
	cout << "max seq id: " << seqData.seqId << endl;
	memset((char *)&seqData.seqName, 0, sizeof(NameData));
	memcpy(seqData.seqName.data, "testSeq", sizeof("testSeq"));
	seqData.value = 100;
	seqData.hasRange = 1;
	seqData.minValue = 1;
	seqData.maxValue = 1000;
	seqData.flags = 1;

	bool ret = false;

	begin_transaction();
	if(testdb_SequenceDataInsert(seqRelId, &seqData)
	&& testdb_QueryDataBySeqRel(seqRelId, &seqData)
	&& testdb_QueryDataBySeqIdIdx(seqRelId, seqIdIdxId, &seqData)
	&& testdb_QueryDataBySeqNameIdx(seqRelId, seqNameIdxId, &seqData)
	&& testdb_CheckMaxSeqId(seqRelId, seqIdIdxId, seqData.seqId)
	&& testdb_UpdateBySeqIdIdx(seqRelId, seqIdIdxId, &seqData, 100))
	{
		ret = true;
	}
	user_abort_transaction();

	return ret;
}

bool testdb_SequenceTable4NonExistDb(Oid dbid)
{
	Oid seqRelId = InvalidOid;
	Oid seqIdIdxId = InvalidOid;
	Oid seqNameIdxId = InvalidOid;

	begin_transaction();
	try
	{
		GetSeqTableByDbid(dbid, seqRelId, seqIdIdxId, seqNameIdxId);
	}
	CATCHEXCEPTION
	commit_transaction();

	if((seqRelId == InvalidOid) && (seqIdIdxId == InvalidOid)
	&& (seqNameIdxId == InvalidOid))
	{
		return true;
	}

	return false;
}


bool testdb_DefaultDatabaseSequenceTable(void)
{
	return testdb_SequenceTable4ExistDb(DEFAULTDATABASE_OID);
}

bool testdb_NewDatabaseSequenceTable(void)
{
	begin_transaction();

	Oid dbid = InvalidOid;
	try
	{
		dbid = CreateDatabase(GetCurrentTransactionId(), "testdb", DEFAULT_TABLESPACE_NAME);
	}
	CATCHEXCEPTION
	commit_transaction();

	if(dbid == InvalidOid)
	{
		return false;
	}

	if(!testdb_SequenceTable4ExistDb(dbid))
	{
		return false;
	}

	begin_transaction();
	try
	{
		DropDatabase("testdb");
	}
	CATCHEXCEPTION

	return testdb_SequenceTable4NonExistDb(dbid);
}
