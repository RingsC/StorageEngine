#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/timer.hpp>

#include "utils/util.h"
#include "utils/tqual.h"
#include "utils/tuplesort.h"
#include "catalog/pg_database.h"
#include "commands/tablespace.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/FDPGAdapter.h"

using std::cout;
using std::endl;
using namespace FounderXDB::StorageEngineNS;

static void testlsm_HeapSplit(RangeData& range, const char *str, int col, size_t len = 0)
{
	Assert(len > sizeof(uint32));

	if(col == 1)
	{
		range.start = 0;
		range.len = sizeof(uint32);
	}

	if(col == 2)
	{
		range.start = sizeof(uint32);
		range.len = len - sizeof(uint32);
	}
}

static void testlsm_IndexSplit(RangeData& range, const char *str, int col, size_t len = 0)
{
	Assert(len == sizeof(uint32));

	if (1 == col)
	{
		range.start = 0;
		range.len = len;
	}
}

static int testlsm_KeyCompare(const char* a, size_t len1, const char* b, size_t len2)
{
	uint32 cmp_1 = *(uint32*)a;
	uint32 cmp_2 = *(uint32*)b;

	return cmp_1 - cmp_2;
}

void testlsm_SetColid(Oid lsm_heap_colid, Oid lsm_idx_colid)
{
	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 2;
	colinfo->col_number = NULL;
	colinfo->rd_comfunction = NULL;
	colinfo->split_function = testlsm_HeapSplit;
	setColInfo(lsm_heap_colid,colinfo);

	colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc0(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = 1;
	colinfo->rd_comfunction = (CompareCallback*)palloc0(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = testlsm_KeyCompare;
	colinfo->split_function = testlsm_IndexSplit;
	setColInfo(lsm_idx_colid, colinfo);
}

static void testlsm_BuildData(uint32 key, const char *value, std::string& data)
{
	data.append((const char *)&key, sizeof(uint32));
	data.append(value);
	char cdata[20];
	snprintf(cdata, 20, "%d", key);
	data.append(cdata);
}

static void testlsm_PrintData(const char *name, const char *data, uint32 length)
{
	uint32 data_key = *((uint32 *)data);
	const char *data_value = (data + sizeof(uint32));
	uint32 data_value_size = length - sizeof(uint32);

	cout << name << ": key[" << data_key << "], value[" << data_value
		<< "], Size[" << data_value_size << "]"<< endl;
}

bool testlsm_QueryOneData(Relation heap_rel, Relation idx_rel,
	const std::string& data, ItemPointer it)
{
	bool found = false;

	Datum datum_data = FDPG_Common::fd_string_formdatum(data.c_str(), sizeof(uint32));
	ScanKeyData keys;
	FDPG_ScanKey::fd_ScanKeyInitWithCallbackInfo(&keys, 1, BTEqualStrategyNumber,
						testlsm_KeyCompare, datum_data);
	IndexScanDesc index_scan = FDPG_Index::fd_index_beginscan(heap_rel,
									idx_rel, SnapshotNow, 1, 0, 0,0);
	FDPG_Index::fd_index_rescan(index_scan, &keys, 1, NULL, 0);
	HeapTuple tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection);
	if(HeapTupleIsValid(tuple))
	{
		int len = 0;
		char *result_data = FDPG_Common::fd_tuple_to_chars_with_len(tuple, len);

		testlsm_PrintData("Data", data.c_str(), data.length());
		testlsm_PrintData("ResultData", result_data, len);

		if((data.length() == len)
		&& (memcmp(data.c_str(), result_data, len) == 0))
		{
			found = true;
			if(PointerIsValid(it))
			{
				ItemPointerCopy(&tuple->t_self, it);
			}
		}

		pfree(result_data);
	}
	FDPG_Index::fd_index_endscan(index_scan);

	return found;
}

bool testlsm_QueryAllData(Relation heap_rel, Relation idx_rel,
	const std::multiset<std::string>& expectedDataSet)
{
	bool found = false;
	std::multiset<std::string> resultDataSet;

	IndexScanDesc index_scan = FDPG_Index::fd_index_beginscan(heap_rel,
									idx_rel, SnapshotNow, 0, 0, 0,0);
	FDPG_Index::fd_index_rescan(index_scan, NULL, 0, NULL, 0);
	HeapTuple tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection);

	while(tuple != NULL)
	{
		int len = 0;
		char *result_data = FDPG_Common::fd_tuple_to_chars_with_len(tuple, len);

		//testlsm_PrintData("resultData", result_data, len);

		resultDataSet.insert(std::string(result_data, len));

		pfree(result_data);
		tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection);
	}
	FDPG_Index::fd_index_endscan(index_scan);

	return (resultDataSet == expectedDataSet);
}

bool testlsm_Drop(Oid heap_id)
{
	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_drop(heap_id);
	}
	CATCHEXCEPTION
	commit_transaction();

	return true;
}

bool testlsm_SubTypeOperation(void)
{
	Oid heap_id = OIDGenerator::instance().GetHeapID();
	Oid idx_id = OIDGenerator::instance().GetIndexID();
	Oid heap_colid = heap_id;
	Oid idx_colid = idx_id;
	IndexType idx_type = LSM_SUBTYPE;

	testlsm_SetColid(heap_colid, idx_colid);

	std::string data_insert_before;
	testlsm_BuildData(1, "data_insert_before_create_index", data_insert_before);

	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_create(0, heap_id, 0, heap_colid);
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(data_insert_before.c_str(),
								data_insert_before.length());
	    FDPG_Heap::fd_simple_heap_insert(rel, tup);
		CommandCounterIncrement();

		/* create LSM_SUBTYPE index */
		FDPG_Index::fd_index_create(rel, idx_type, idx_id, idx_colid);

		/* can't query data_insert_before */
		Relation idx_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);
		if(testlsm_QueryOneData(rel, idx_rel, data_insert_before, NULL))
		{
			throw false;
		}

		FDPG_Index::fd_index_close(idx_rel, NoLock);
		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	CATCHEXCEPTION
	commit_transaction();

	/* query insert update delete */
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		Relation idx_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);

		/* can't query data_insert_before */
		if(testlsm_QueryOneData(rel, idx_rel, data_insert_before, NULL))
		{
			throw false;
		}

		std::string data_insert1;
		std::string data_insert2;
		std::string data_update1_new;
		testlsm_BuildData(1, "data_insert", data_insert1);
		testlsm_BuildData(2, "data_insert", data_insert2);
		testlsm_BuildData(1, "data_update", data_update1_new);

		/* insert data */
		{
			HeapTuple tup1 = FDPG_Heap::fd_heap_form_tuple(data_insert1.c_str(),
								data_insert1.length());
			HeapTuple tup2 = FDPG_Heap::fd_heap_form_tuple(data_insert2.c_str(),
								data_insert2.length());
	    	FDPG_Heap::fd_simple_heap_insert(rel, tup1);
			FDPG_Heap::fd_simple_heap_insert(rel, tup2);
			FDPG_Memory::fd_pfree(tup1);
			FDPG_Memory::fd_pfree(tup2);
			CommandCounterIncrement();
		}

		/* update */
		{
			ItemPointerData it;
			if(!testlsm_QueryOneData(rel, idx_rel, data_insert1, &it)){
				throw false;
			}

			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(data_update1_new.c_str(),
								data_update1_new.length());
			FDPG_Heap::fd_simple_heap_update(rel, &it, tup);
			FDPG_Memory::fd_pfree(tup);
   			CommandCounterIncrement();
		}

		/* delete */
		{
			ItemPointerData it;
			if(!testlsm_QueryOneData(rel, idx_rel, data_insert2, &it)){
				throw false;
			}

			FDPG_Heap::fd_simple_heap_delete(rel, &it);
   			CommandCounterIncrement();
		}

		/* query all */
		{
			std::multiset<std::string> expectedDataSet;
			expectedDataSet.insert(data_update1_new);
			if(!testlsm_QueryAllData(rel, idx_rel, expectedDataSet))
			{
				throw false;
			}
		}

		FDPG_Index::fd_index_close(idx_rel, NoLock);
		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	catch(bool &ret)
	{
		user_abort_transaction();
		return false;
	}
	CATCHEXCEPTION
	commit_transaction();

	testlsm_Drop(heap_id);

	return true;
}

void testlsm_InsertThread(Oid heap_id, Oid idx_id, bool *create_ok, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_ret = false;

	cout << "*************insert thread start**************" << endl;

	begin_transaction();
	try{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);

		/* insert data */
		for(uint32 i = 0; i < 10; i++)
		{
			std::string data;
			testlsm_BuildData(i, "insert_data", data);

			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(data.c_str(), data.length());
			FDPG_Heap::fd_simple_heap_insert(rel, tup);
			FDPG_Memory::fd_pfree(tup);
		}
		CommandCounterIncrement();

		/* let create index thread run */
		boost::thread::sleep(boost::get_system_time()
					+ boost::posix_time::milliseconds(3000));

		if(*create_ok)
		{
#if 0
			/* insert tuple */
			std::string data_insert_after;
			testlsm_BuildData(22, "data_insert_after", data_insert_after);
			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(data_insert_after.c_str(),
								data_insert_after.length());
			FDPG_Heap::fd_simple_heap_insert(rel, tup);
			FDPG_Memory::fd_pfree(tup);
			CommandCounterIncrement();
#endif
			std::multiset<std::string> expectedDataSet;
			//expectedDataSet.insert(data_insert_after);
			Relation idx_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);
			if(testlsm_QueryAllData(rel, idx_rel, expectedDataSet))
			{
				*p_ret = true;
			}
		}

		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return;
	}
	cout << "Start commit insert......" << endl;
	commit_transaction();

	return;
}

void testlsm_CreateIdxThread(Oid heap_id, Oid idx_id, Oid idx_colid,
	IndexType idx_type, bool *create_ok, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_ret = false;

	cout << "*************create idx thread start**************" << endl;

	begin_transaction();
	try{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		cout << "before create" << endl;
		FDPG_Index::fd_index_create(rel, idx_type, idx_id, idx_colid);
		*create_ok = true;
		cout << "after create" << endl;

		*p_ret = true;
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return;
	}
	commit_transaction();

	return;
}

void testlsm_DropThread(Oid heap_id, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_ret = testlsm_Drop(heap_id);

	proc_exit(0);
}

bool testlsm_SubtypeCreateWhenInsert(void)
{
	Oid heap_id = OIDGenerator::instance().GetHeapID();
	Oid idx_id = OIDGenerator::instance().GetIndexID();
	Oid heap_colid = heap_id;
	Oid idx_colid = idx_id;
	IndexType idx_type = LSM_SUBTYPE;
	//IndexType idx_type = BTREE_TYPE;

	testlsm_SetColid(heap_colid, idx_colid);

	/* create heap */
	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_create(0, heap_id, 0, heap_colid);
	}
	CATCHEXCEPTION
	commit_transaction();

	boost::thread_group tg;
	bool ret1 = false;
	bool ret2 = false;
	bool create_ok = false;

	tg.create_thread(boost::bind(&testlsm_InsertThread, heap_id, idx_id, &create_ok, &ret1));

	/* sleep 1s, let create lsm_subtype index thread do first*/
	boost::thread::sleep(boost::get_system_time()
					+ boost::posix_time::milliseconds(1000));

	tg.create_thread(boost::bind(&testlsm_CreateIdxThread, heap_id, idx_id,
		idx_colid, idx_type, &create_ok, &ret2));

	tg.join_all();

	cout << "ret1: " << ret1 << endl;
	cout << "ret2: " << ret2 << endl;

	bool drop_ret;
	tg.create_thread(boost::bind(&testlsm_DropThread, heap_id, &drop_ret));
	tg.join_all();

	if(!drop_ret)
	{
		return false;
	}

	return ret1 && ret2;
}

extern void RelationTruncateIndex(Relation heapRelation, Oid indexId);
void testlsm_TruncateThread(Oid heap_id, Oid idx_id, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_ret = false;

	cout << "*************truncate thread start**************" << endl;

	begin_transaction();
	try{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		cout << "before truncate" << endl;
		//RelationTruncateIndex(rel, idx_id);
		FDPG_Heap::fd_heap_truncate(rel);
		FDPG_Heap::fd_heap_close(rel, NoLock);
		cout << "after truncate" << endl;
		*p_ret = true;
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return;
	}
	commit_transaction();

	return;
}

bool testlsm_Insert(void)
{
	Oid heap_id = OIDGenerator::instance().GetHeapID();
	Oid idx_id = OIDGenerator::instance().GetIndexID();
	Oid heap_colid = heap_id;
	Oid idx_colid = idx_id;
	//IndexType idx_type = LSM_SUBTYPE;
	IndexType idx_type = BTREE_TYPE;

	testlsm_SetColid(heap_colid, idx_colid);

	std::multiset<std::string> insertDataSet;

#if 1
	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_create(0, heap_id, 0, heap_colid);
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		FDPG_Index::fd_index_create(rel, idx_type, idx_id, idx_colid);
	}
	CATCHEXCEPTION
	commit_transaction();

	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		Relation index_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);

		srand(1);
		for(uint32 i = 0; i < 100000; i++)
		{
			std::string data;
			testlsm_BuildData(rand(), "insert_data", data);

			insertDataSet.insert(data);

			char data_name[20];
			snprintf(data_name, 20, "Insert%d", i);
			//testlsm_PrintData(data_name, data.c_str(), data.length());

			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(data.c_str(), data.length());
			FDPG_Heap::fd_simple_heap_insert(rel, tup);
			FDPG_Memory::fd_pfree(tup);
		}
	}
	CATCHEXCEPTION
	commit_transaction();

	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		Relation index_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);

		std::string data;
		testlsm_BuildData(rand(), "insert_data", data);
		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(data.c_str(), data.length());
		FDPG_Heap::fd_simple_heap_insert(rel, tup);
		FDPG_Memory::fd_pfree(tup);
	}
	CATCHEXCEPTION
	commit_transaction();

	/* truncate this index in other thread */
	boost::thread_group tg;
	bool ret = false;
	tg.create_thread(boost::bind(&testlsm_TruncateThread, heap_id, idx_id, &ret));
	tg.join_all();

	/* insert again */
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		std::string data;
		testlsm_BuildData(100, "insert_data", data);
		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(data.c_str(), data.length());
		FDPG_Heap::fd_simple_heap_insert(rel, tup);
		FDPG_Memory::fd_pfree(tup);
	}
	CATCHEXCEPTION
	commit_transaction();
#else
	/* create && insert */
	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_create(0, heap_id, 0, heap_colid);
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		FDPG_Index::fd_index_create(rel, idx_type, idx_id, idx_colid);

		srand(1);
		for(uint32 i = 0; i < 10; i++)
		{
			std::string data;
			testlsm_BuildData(rand(), "insert_data", data);

			insertDataSet.insert(data);

			char data_name[20];
			snprintf(data_name, 20, "Insert%d", i);
			testlsm_PrintData(data_name, data.c_str(), data.length());

			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(data.c_str(), data.length());
			FDPG_Heap::fd_simple_heap_insert(rel, tup);
			FDPG_Memory::fd_pfree(tup);
		}
	}
	CATCHEXCEPTION
	commit_transaction();

	/* scan */
	bool ret = false;
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		Relation idx_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);
		ret = testlsm_QueryAllData(rel, idx_rel, insertDataSet);
	}
	CATCHEXCEPTION
	commit_transaction();
#endif
	return ret;
}

extern void insert_tuple_to_index(Relation heapRel, Relation indexRel, HeapTuple tup);
//static void testlsm_MergeTuples(Relation heapRel, uint32 ntuples,
//	HeapTuple *tuples, ItemPointerData *itDatas)
static HeapTuple testlsm_CopyTuple(HeapTuple tuple)
{
	 return heap_copytuple(tuple);
}

static void testlsm_DeleteTuples(Relation heapRel, uint32 ntuples, HeapTuple *tuples)
{
	for(uint32 i = 0; i < ntuples; i++)
	{
		simple_heap_delete(heapRel, &(tuples[i]->t_self));
	}
}

static void testlsm_InsertHeapTuples(Relation heapRel, uint32 ntuples, HeapTuple *tuples)
{
	/* insert heap */
	if(ntuples == 1)
	{
		heap_insert(heapRel, tuples[0], GetCurrentCommandId(true), 0, NULL);
	}
	else
	{
		BulkInsertState bistate = GetBulkInsertState();
		heap_multi_insert(heapRel, tuples, ntuples, bistate);
		FreeBulkInsertState(bistate);
	}
}

static void testlsm_InsertIndexTuples(Relation heapRel, uint32 ntuples, HeapTuple *tuples)
{
	/* insert indexes */
	for(uint32 i = 0; i < MtInfo_GetIndexCount(heapRel->mt_info); ++i)
	{
		Oid idxId = MtInfo_GetIndexOid(heapRel->mt_info)[i];
		Relation idxRel = index_open(idxId, AccessShareLock);

		for(uint32 j = 0; j < ntuples; j++)
		{
			insert_tuple_to_index(heapRel, idxRel, tuples[j]);
		}

		index_close(idxRel, NoLock);
	}
}

static void testlsm_FreeTuples(uint32 ntuples, HeapTuple *tuples)
{
	for(uint32 i = 0; i < ntuples; i++)
	{
		pfree(tuples[i]);
	}
}

static void testlsm_MergeTuples(Relation heapRel, uint32 ntuples, HeapTuple *tuples)
{
	if(ntuples == 0)
	{
		return;
	}

	/* delete heap */
	//testlsm_DeleteTuples(heapRel, ntuples, tuples);

	/* insert heap */
	testlsm_InsertHeapTuples(heapRel, ntuples, tuples);

	/* insert indexes */
	testlsm_InsertIndexTuples(heapRel, ntuples, tuples);

	testlsm_FreeTuples(ntuples, tuples);
}

static void testlsm_DeleteAllTuples(Relation heapRel, Relation subIdxRel,
	Tuplesortstate* pState)
{
	//SnapshotData ssdata = {HeapTupleSatisfiesMVCC};
	SnapshotData ssdata = {HeapTupleSatisfiesNow};
	IndexScanDesc index_scan = FDPG_Index::fd_index_beginscan(heapRel,
				subIdxRel, &ssdata, 0, 0, 0,0);
	FDPG_Index::fd_index_rescan(index_scan, NULL, 0, NULL, 0);

	TIDBitmap * bitmap = tbm_create(work_mem * 1024L);
	index_getbitmap(index_scan, bitmap);
	BitmapHeapScanState* bmpScanState = ExecInitBitmapHeapScan(heapRel, bitmap,
											index_scan->xs_snapshot,
											index_scan->numberOfKeys,
											index_scan->keyData);

PG_TRY();
{
	HeapTuple tuple;
	while((tuple = BitmapHeapNext(bmpScanState)) != NULL)
	{
		ItemPointerData it = tuple->t_self;
		simple_heap_delete(heapRel, &it);

		if(pState != NULL)
		{
			HeapTuple ctuple = heap_copytuple(tuple);
			//ItemPointer tid = (ItemPointer)palloc(sizeof(ItemPointerData));
			//*tid = tuple->t_self;
			Datum values = fdxdb_form_index_datum(heapRel, subIdxRel, tuple);
			if (values != 0 && values != -1)
			{// skip if no index key needed for this tuple    
				tuplesort_putdatum(pState, values, (ItemPointer)ctuple);
			}
			else if (values == -1)
			{// multiple keys per heaptuple.
				size_t nkeys = 0;
				Datum *pvalues = combineIndexKeys(heapRel, subIdxRel, tuple, &nkeys);
				for (size_t i = 0; i < nkeys; i++)
				{
					tuplesort_putdatum(pState, pvalues[i], (ItemPointer)ctuple);
				}
				pfree(pvalues);
			}
		}
	}
}
PG_CATCH();
{
	cout << "error........" << endl;
}
PG_END_TRY();

	ExecEndBitmapHeapScan(bmpScanState);
	FDPG_Index::fd_index_endscan(index_scan);
}

static void testlsm_MergeDeleteUseBitmap(Relation heapRel,
	Relation subIdxRel)
{
	/* bitmap delete all */
	testlsm_DeleteAllTuples(heapRel, subIdxRel, NULL);

	IndexScanDesc index_scan = FDPG_Index::fd_index_beginscan(heapRel,
		subIdxRel, SnapshotNow, 0, 0, 0,0);
	FDPG_Index::fd_index_rescan(index_scan, NULL, 0, NULL, 0);

	//static const int insertCountOnce = 10000;
	static const int insertCountOnce = 100000;
	HeapTuple tuples[insertCountOnce];
	uint32 ntuple = 0;

	while(1)
	{
		HeapTuple tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection);
		if(tuple == NULL)
		{
			testlsm_MergeTuples(heapRel, ntuple, tuples);
			break;
		}
		else
		{
			tuples[ntuple] = testlsm_CopyTuple(tuple);

			ntuple++;
			if(ntuple == insertCountOnce)
			{
				testlsm_MergeTuples(heapRel, ntuple, tuples);
				ntuple = 0;
			}
		}
	}

	FDPG_Index::fd_index_endscan(index_scan);
	FDPG_Index::fd_index_close(subIdxRel, AccessShareLock);
}

static void testlsm_MergeAllUseBitmap(Relation heapRel,
	Relation subIdxRel)
{
	/* tuplesort begin */
	Tuplesortstate *pState;
	int workMem  = MaxAllocSize/1024;//need external sorting when memory used greater than this value
	pState = tuplesort_begin_index_datum(workMem, true, subIdxRel);
	//pState = tuplesort_begin_heap(workMem, true, subIdxRel);

	testlsm_DeleteAllTuples(heapRel, subIdxRel, pState);

	tuplesort_performsort(pState);
PG_TRY();
{
	/* insert tuple */
	static const int insertCountOnce = 10000;
	//static const int insertCountOnce = 1;
	HeapTuple tuples[insertCountOnce];
	uint32 ntuple = 0;
	while(1)
	{
		Datum dat_value;
		HeapTuple newtuple;
#if 1
		if(!tuplesort_getdatum1(pState, true, &dat_value, (void **)&newtuple))
		{
			newtuple = NULL;
		}
		else
		{
			if(DatumGetPointer(dat_value) != NULL)
			{
				pfree(DatumGetPointer(dat_value));
			}
		}
#else
		HeapTupleData tupleData;
		if(tuplesort_getdatum(pState, true, &dat_value, &tupleData.t_self))
		{
			Buffer buf;
			if(heap_fetch(heapRel, SnapshotAny, &tupleData, &buf, false, NULL))
			{
				newtuple = testlsm_CopyTuple(&tupleData);
			}
			else
			{
				cout << "heap fetch failed" << endl;
			}

			if(DatumGetPointer(dat_value) != NULL)
			{
				pfree(DatumGetPointer(dat_value));
			}
		}
#endif
		if(newtuple == NULL)
		{
			testlsm_MergeTuples(heapRel, ntuple, tuples);
			break;
		}
		else
		{
			tuples[ntuple] = newtuple;

			ntuple++;
			if(ntuple == insertCountOnce)
			{
				testlsm_MergeTuples(heapRel, ntuple, tuples);
				ntuple = 0;
			}
		}
	}
}
PG_CATCH();
{
	int errCode = 0;
	char* errMsg = get_errno_errmsg(errCode);
	cout << "testlsm_MergeAllUseBitmap error: " << errCode << " " << errMsg << endl;
	FlushErrorState();
}
PG_END_TRY();
	tuplesort_end(pState);
}

bool testlsm_Merge(void)
{
	Oid heap_id = OIDGenerator::instance().GetHeapID();
	Oid idx_id = OIDGenerator::instance().GetIndexID();
	Oid heap_colid = heap_id;
	Oid idx_colid = idx_id;
	IndexType idx_type = LSM_TYPE;
	//IndexType idx_type = BTREE_TYPE;

	testlsm_SetColid(heap_colid, idx_colid);

	/* create index */
	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_create(0, heap_id, 0, heap_colid);
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		FDPG_Index::fd_index_create(rel, idx_type, idx_id, idx_colid);
	}
	CATCHEXCEPTION
	commit_transaction();

	/* insert data */
	static const uint32 insert_count = 3000000;
	//static const uint32 insert_count = 5;
	std::multiset<std::string> insertDataSet;
	srand(1);
	boost::timer t1;
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		Relation index_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);

		for(uint32 i = 0; i < insert_count; i++)
		{
			std::string data;
			//testlsm_BuildData(i, "insert_data", data);
			testlsm_BuildData(rand()%10000, "insert_data", data);

			//insertDataSet.insert(data);

			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(data.c_str(), data.length());
			FDPG_Heap::fd_simple_heap_insert(rel, tup);
			FDPG_Memory::fd_pfree(tup);
		}
	}
	CATCHEXCEPTION
	commit_transaction();
	double d_time1 = t1.elapsed();
	cout << "insert " << insert_count << " data cost " << d_time1 << "s" << endl;
#if 1
	/* merge */
	uint32 mergeCount = 0;
	boost::timer t;
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		Relation index_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);

		for(uint32 i = 0; i < MtInfo_GetIndexCount(index_rel->mt_info); ++i)
		{
			Oid subIdxId = MtInfo_GetIndexOid(index_rel->mt_info)[i];
			Relation sub_index_rel = FDPG_Index::fd_index_open(subIdxId, AccessShareLock);

			//testlsm_MergeDeleteUseBitmap(rel, sub_index_rel);
			testlsm_MergeAllUseBitmap(rel, sub_index_rel);
		}
	}
	CATCHEXCEPTION
	commit_transaction();
	double d_time = t.elapsed();
	cout << "merge " << mergeCount << " data cost " << d_time << "s" << endl;
#endif
	return true;
}

