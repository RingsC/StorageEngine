#include <boost/assign.hpp>
#include <vector>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <boost/thread/thread.hpp>

#include "utils/util.h"
#include "catalog/xdb_catalog.h"
#include "catalog/index.h"
#include "access/xact.h"
#include "interface/FDPGAdapter.h"
#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "access/skey.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "utils/snapmgr.h"
#include "heap/test_delete.h"
#include "test_fram.h"
#include "interface/StorageEngineExceptionUniversal.h"

using namespace FounderXDB::StorageEngineNS;

extern void vacuum(VacuumStmt *vacstmt, Oid relid, bool do_toast,
	   BufferAccessStrategy bstrategy, bool for_wraparound, bool isTopLevel);


/*
* create heap and index
* Desciption:
*    	create heap
*       	heap has 4 columns, and data length must be larger than 12(4+4+4)
*	 	col1_len -- 4, col2_len -- 4, col3_len -- 4, col4_len -- rest
*
*
* 	create index
* 		index at most on 3 columns(must be col1, col2, or col3)
*
* 		In testvacuum_SetIndexColinfo function:
* 		if ncolumns == 1, create on columns[0]
* 		if ncolumns == 2, create on columns[0] && columns[1]
* 		if ncolumns == 3, create on columns[0] && columns[1] && columns[2]
*
*/
void testvacuum_GenRandStr(std::string &str, unsigned int nLen)
{
	for(unsigned int i = 0; i < nLen - 1; i++){
		str += 'a' + (rand()%26);
	}
	str += '\0';

	return;
}

void testvacuum_GenOneData(std::string &data, unsigned int nLen)
{
	Assert(data.size() == 0);

	char bytes[8];
	for(int i = 0; i < 3; i++)
	{
		snprintf(bytes, 8, "%04d", rand()%9999);
		data += bytes;
	}
	testvacuum_GenRandStr(data, nLen - 12);
}

void testvacuum_GenDataSet(std::multiset<std::string> &gen_data,
	unsigned int data_count, bool is_toast)
{
	int mod = ((is_toast) ? 5000 : 100);
	int min_data_length = 13;

	int i = 0;

	if(is_toast)
	{
		std::string one_data;
		testvacuum_GenOneData(one_data, (rand()%mod + 3000));
		gen_data.insert(one_data);
		i = 1;
	}

	for(; i < data_count; i++)
	{
		std::string one_data;
		testvacuum_GenOneData(one_data, (rand()%mod + 20));
		gen_data.insert(one_data);
	}
}

void testvacuum_GenDataVector(std::vector<std::string> &gen_data,
	unsigned int data_count, bool is_toast)
{
	int mod = ((is_toast) ? 5000 : 100);
	int min_data_length = 13;

	gen_data.resize(data_count);

	int i = 0;

	if(is_toast)
	{
		std::string one_data;
		testvacuum_GenOneData(one_data, (rand()%mod + 3000));
		gen_data[0] = one_data;
		i = 1;
	}

	for(; i < data_count; i++)
	{
		std::string one_data;
		testvacuum_GenOneData(one_data, (rand()%mod + 20));
		gen_data[i] = one_data;
	}
}


int testvacuum_CompareBytes4(const char *str1, size_t len1,
									const char *str2, size_t len2)
{
	unsigned int i1 = 0;
	unsigned int i2 = 0;

	Assert(len1 == 4);
	Assert(len2 == 4);

	for(unsigned int i = 0; i < 4; i++){
		Assert((str1[i] >= '0') && (str1[i] <= '9'));
		Assert((str2[i] >= '0') && (str2[i] <= '9'));

		i1 = (i1 * 10) + (str1[i] - '0');
		i2 = (i2 * 10) + (str2[i] - '0');
	}

	Assert(i1 <= 9999);
	Assert(i2 <= 9999);

	if(i1 > i2){
		return 1;
	}else if(i1 < i2){
		return -1;
	}else{
		return 0;
	}
}

void testvacuum_HeapSplitFunc(RangeData& rangeData, const char *str,
									int col, size_t data_len)
{
	if(data_len <= 12){
		Assert(false);
		return;
	}

	switch(col){
		case 1:
			rangeData.start = 0;
			rangeData.len = 4;
			break;
		case 2:
			rangeData.start = 4;
			rangeData.len = 4;
			break;
		case 3:
			rangeData.start = 8;
			rangeData.len = 4;
			break;
		case 4:
			rangeData.start = 12;
			rangeData.len = data_len - 12;
		default:
			Assert(false);
			break;
	}
}

void testvacuum_IndexSplitFunc(RangeData& rangeData, const char *str,
									int col, size_t data_len)
{
	if(col == 1){
		rangeData.start = 0;
		rangeData.len = 4;
	}else if(col == 2){
		rangeData.start = 4;
		rangeData.len = 4;
	}else if(col == 3){
		rangeData.start = 8;
		rangeData.len = 4;
	}else{
		Assert(false);
	}
}

void testvacuum_SetHeapColinfo(Oid heap_id)
{
	Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));

	Assert(colinfo != NULL);

	colinfo->col_number = 0;
	colinfo->keys = 0;
	colinfo->rd_comfunction = NULL;
	colinfo->split_function = testvacuum_HeapSplitFunc;

	setColInfo(heap_id, colinfo);
	return;
}

void testvacuum_SetIndexColinfo(Oid idx_id,
	unsigned int ncolumns, unsigned int *columns)
{
	Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));

	colinfo->keys = ncolumns;
	colinfo->col_number = (size_t *)malloc(colinfo->keys * sizeof(size_t));
	colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
	for(unsigned int i = 0; i < ncolumns; i++){
		colinfo->col_number[i] = columns[i];
		colinfo->rd_comfunction[i] = testvacuum_CompareBytes4;
	}
	colinfo->split_function =  testvacuum_IndexSplitFunc;//indexµÄsplit

	setColInfo(idx_id, colinfo);
	return;
}

void testvacuum_CreateHeap(Oid &heap_id, bool &is_new_heap)
{
	Oid tableSpaceId = OIDGenerator::instance().GetTableSpaceID();
	heap_id = OIDGenerator::instance().GetHeapID();

	testvacuum_SetHeapColinfo(heap_id);

	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_create(tableSpaceId, heap_id, MyDatabaseId, heap_id);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;

		is_new_heap = false;
		if(ex.getErrorNo() != ERRCODE_DUPLICATE_FILE)
		{
			heap_id = InvalidOid;
		}
		return;
	}
	commit_transaction();
	is_new_heap= true;
	return;
}

void testvacuum_CreateIndex(Oid &idx_id, bool &is_new_index,
	unsigned int ncolumns, unsigned int *columns,
	Oid heap_id, IndexType idxtype = BTREE_TYPE)
{
	idx_id = OIDGenerator::instance().GetIndexID();

	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, RowExclusiveLock);

		testvacuum_SetIndexColinfo(idx_id, ncolumns, columns);
		FDPG_Index::fd_index_create(rel, idxtype, idx_id, idx_id);
		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;

		is_new_index = false;
		if(ex.getErrorNo() != ERRCODE_DUPLICATE_FILE)
		{
			idx_id = InvalidOid;
		}
		return;
	}
	commit_transaction();
	is_new_index = true;
	return;
}

void testvacuum_HeapGetAll(Relation rel, std::multiset<std::string> &all_data)
{
	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	HeapTuple tup = NULL;

	all_data.clear();
	while((tup = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL){	  
		int len = 0;
		char *data_tuple = fdxdb_tuple_to_chars_with_len(tup, len);
		all_data.insert(std::string((char*)(data_tuple), len));
		pfree(data_tuple);
	}
	FDPG_Heap::fd_heap_endscan(scan);

	return;
}

void testvacuum_IndexGetAll(Relation rel, Relation idx_rel,
	std::multiset<std::string> &all_data)
{
	const int nkeys = 1;
	char cmp_data[4] = {'9', '9', '9', '9'};
	Datum datum = FDPG_Common::fd_string_formdatum(cmp_data, sizeof(cmp_data));
	ScanKeyData scan_key[nkeys];
	IndexScanDesc idx_scan = NULL;
	HeapTuple tup = NULL;

	/* init scankey */
	FDPG_ScanKey::fd_ScanKeyInitWithCallbackInfo(&scan_key[0], 1,
		BTLessEqualStrategyNumber, testvacuum_CompareBytes4, datum);

	idx_scan = FDPG_Index::fd_index_beginscan(rel, idx_rel, SnapshotNow, nkeys, 0);
	FDPG_Index::fd_index_rescan(idx_scan, scan_key, nkeys, NULL, 0);
	all_data.clear();
	while((tup = FDPG_Index::fd_index_getnext(idx_scan, BackwardScanDirection)) != NULL)
	{
		int data_len = 0;
		char *data_tuple = FDPG_Common::fd_tuple_to_chars_with_len(tup, data_len);
		all_data.insert(std::string((char*)(data_tuple), data_len));
		pfree(data_tuple);
	}
	FDPG_Index::fd_index_endscan(idx_scan);

	return;
}

static void testvacuum_InsertUpdateDelete(Relation rel,
	unsigned int data_count, bool is_toast)
{
	std::multiset<std::string> gen_data;
	testvacuum_GenDataSet(gen_data, data_count, is_toast);

	/* insert */
	for (std::multiset<std::string>::const_iterator it = gen_data.begin();
		it != gen_data.end();
		++it)
	{
		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple((*it).c_str(), (*it).length());
		FDPG_Heap::fd_simple_heap_insert(rel, tup);
	}
	FDPG_Transaction::fd_CommandCounterIncrement();

	/* update  && delete */
	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	HeapTuple tup = NULL;

	int ntups = 0;
	while((tup = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		ntups++;
		ItemPointerData itemPointData;
		ItemPointer tid = (ItemPointerData *)(&itemPointData);
		tid->ip_blkid.bi_hi = tup->t_self.ip_blkid.bi_hi;
        tid->ip_blkid.bi_lo = tup->t_self.ip_blkid.bi_lo;
        tid->ip_posid = tup->t_self.ip_posid;

		if(ntups%3 == 1)
		{
			/* update */
			int len = 0;
			char *data_tuple = fdxdb_tuple_to_chars_with_len(tup, len);
			std::string new_data(data_tuple);
			new_data.replace((new_data.length() - 5), 5, "uuuuu");
			HeapTuple tup_update = FDPG_Heap::fd_heap_form_tuple(new_data.c_str(),new_data.length());
		    FDPG_Heap::fd_simple_heap_update(rel, tid, tup_update);
		    pfree(data_tuple);
		}
		else if(ntups%3 == 0)
		{
			/* delete */
			FDPG_Heap::fd_simple_heap_delete(rel, tid);
		}
	}
	FDPG_Heap::fd_heap_endscan(scan);
	CommandCounterIncrement();
}

static void testvacuum_Vacuum(Oid heap_id)
{
	FDPG_Heap::fd_VacuumRelations(1, &heap_id);
}

bool testvacuum_Heap(unsigned int data_count, bool is_toast)
{
	Oid heap_id;
	bool is_new_heap;
	testvacuum_CreateHeap(heap_id, is_new_heap);
	if(heap_id == InvalidOid)
	{
		return false;
	}

	std::multiset<std::string> all_data;
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);

		/* if rel is new created heap, we do insert, update, and delete */
		if(is_new_heap)
		{
			testvacuum_InsertUpdateDelete(rel, data_count, is_toast);
		}

		testvacuum_HeapGetAll(rel, all_data);
		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();

	/* do vacuum */
	testvacuum_Vacuum(heap_id);

	/* get data */
	std::multiset<std::string> all_data_after_vacuum;
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		testvacuum_HeapGetAll(rel, all_data_after_vacuum);
		FDPG_Heap::fd_heap_close(rel, NoLock);

		/* drop heap */
		FDPG_Heap::fd_heap_drop(heap_id);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();

	return (all_data_after_vacuum == all_data);
}

bool testvacuum_HeapWithIndex(unsigned int data_count, bool is_toast)
{
	Oid heap_id;
	bool is_new_heap;
	testvacuum_CreateHeap(heap_id, is_new_heap);
	if(heap_id == InvalidOid)
	{
		return false;
	}

	Oid idx_id;
	bool is_new_index;
	const unsigned int ncolumns = 1;
	unsigned int columns[ncolumns] = {1};
	testvacuum_CreateIndex(idx_id, is_new_index, ncolumns, columns, heap_id);
	if(idx_id == InvalidOid)
	{
		return false;
	}
	if(is_new_index != is_new_heap)
	{
		return false;
	}

	std::multiset<std::string> all_data;
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);

		/* if rel is new created heap, we do insert, update, and delete */
		if(is_new_heap)
		{
			testvacuum_InsertUpdateDelete(rel, data_count, is_toast);
		}

		testvacuum_HeapGetAll(rel, all_data);
		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();

	/* do vacuum */
	testvacuum_Vacuum(heap_id);

	/* get data */
	std::multiset<std::string> all_data_after_vacuum;
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		Relation idx_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);
		testvacuum_IndexGetAll(rel, idx_rel, all_data_after_vacuum);
		FDPG_Heap::fd_heap_close(rel, NoLock);
		FDPG_Index::fd_index_close(idx_rel, NoLock);

		/* drop heap & index */
		FDPG_Heap::fd_heap_drop(heap_id);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();

	return (all_data_after_vacuum == all_data);
}

bool testvacuum_SmallHeap(void)
{
	return testvacuum_Heap(3, false);
}

bool testvacuum_LargeHeap(void)
{
	return testvacuum_Heap(5000, false);
}

bool testvacuum_SmallToast(void)
{
	return testvacuum_Heap(3, true);
}

bool testvacuum_LargeToast(void)
{
	return testvacuum_Heap(500, true);
}

bool testvacuum_SmallHeapWithIndex(void)
{
	return testvacuum_HeapWithIndex(3, false);
}

bool testvacuum_LargeHeapWithIndex(void)
{
	return testvacuum_HeapWithIndex(5000, false);
}

bool testvacuum_SmallToastWithIndex(void)
{
	return testvacuum_HeapWithIndex(3, true);
}

bool testvacuum_LargeToastWithIndex(void)
{
	return testvacuum_HeapWithIndex(500, true);
}










/* concurrence test */
bool testvacuum_ConInsert(Oid heap_id, std::vector<std::string> &datas,
	std::vector<ItemPointerData> &tids)
{
	int ndatas = datas.size();
	tids.resize(ndatas);

	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);

		/* insert */
		for(int i = 0; i < ndatas; i++)
		{
			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(datas[i].c_str(),
														datas[i].length());
			FDPG_Heap::fd_simple_heap_insert(rel, tup);

			/* fill in tid */
			tids[i] = tup->t_self;
		}

		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();
	return true;
}

bool testvacuum_ConDelete(Oid heap_id, std::vector<ItemPointerData> &tids)
{
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);

		/* delete tuples */
		ItemPointer p_tid = NULL;
		for(int i = 0; i < tids.size(); i++)
		{
			p_tid = (ItemPointer)(&tids[i]);
			FDPG_Heap::fd_simple_heap_delete(rel, p_tid);
		}

		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();
	return true;
}

bool testvacuum_ConUpdate(Oid heap_id, std::vector<ItemPointerData> &tids,
	std::vector<std::string> &datas)
{
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);

		/* delete tuples */
		ItemPointer p_tid = NULL;
		for(int i = 0; i < tids.size(); i++)
		{
			HeapTuple tup_update = FDPG_Heap::fd_heap_form_tuple(datas[i].c_str(),
										datas[i].length());
			p_tid = (ItemPointer)(&tids[i]);
		    FDPG_Heap::fd_simple_heap_update(rel, p_tid, tup_update);
		}

		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();
	return true;
}

bool testvacuum_ConQuery(Oid heap_id, std::multiset<std::string> &all_data)
{
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		testvacuum_HeapGetAll(rel, all_data);
		FDPG_Heap::fd_heap_close(rel, NoLock);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();
	return true;
}

bool testvacuum_ConQueryByIndex(Oid heap_id, Oid idx_id,
	std::multiset<std::string> &all_data)
{
	begin_transaction();
	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
		Relation idx_rel = FDPG_Index::fd_index_open(idx_id, AccessShareLock);
		testvacuum_IndexGetAll(rel, idx_rel, all_data);
		FDPG_Heap::fd_heap_close(rel, NoLock);
		FDPG_Index::fd_index_close(idx_rel, NoLock);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();
	return true;
}

bool testvacuum_ConVacuum(Oid heap_id)
{
	try
	{
		FDPG_Heap::fd_VacuumRelations(1, &heap_id);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}

	return true;
}

bool testvacuum_ConDrop(Oid heap_id)
{
	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_drop(heap_id);
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}
	commit_transaction();

	return true;
}

void testvacuum_ConInsertThread(Oid heap_id, std::vector<std::string> *p_datas, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	std::vector<ItemPointerData> tids;
	*p_ret = testvacuum_ConInsert(heap_id, *p_datas, tids);

	proc_exit(0);
}

void testvacuum_ConDeleteThread(Oid heap_id, std::vector<ItemPointerData> *p_tids, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_ret = testvacuum_ConDelete(heap_id, *p_tids);

	proc_exit(0);
}

void testvacuum_ConUpdateThread(Oid heap_id, std::vector<ItemPointerData> *p_tids,
	std::vector<std::string> *p_datas, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_ret = testvacuum_ConUpdate(heap_id, *p_tids, *p_datas);

	proc_exit(0);
}

void testvacuum_ConQueryThread(Oid heap_id, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	std::multiset<std::string> all_data;
	*p_ret = testvacuum_ConQuery(heap_id, all_data);

	proc_exit(0);
}

void testvacuum_ConQueryByIndexThread(Oid heap_id, Oid idx_id, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	std::multiset<std::string> all_data;
	*p_ret = testvacuum_ConQueryByIndex(heap_id, idx_id, all_data);

	proc_exit(0);
}

void testvacuum_ConVacuumThread(Oid heap_id, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_ret = testvacuum_ConVacuum(heap_id);

	proc_exit(0);
}

void testvacuum_ConDropThread(Oid heap_id, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_ret = testvacuum_ConDrop(heap_id);

	proc_exit(0);
}

bool testvacuum_ConTestHeap(unsigned int data_count, bool is_toast,
	const int n_insert_threads, const int n_delete_threads, const int n_update_threads,
	const int n_query_threads, const int n_vacuum_threads)
{
	Assert(data_count > 0);

	Oid heap_id;
	bool is_new_heap;
	testvacuum_CreateHeap(heap_id, is_new_heap);
	if((heap_id == InvalidOid) || (!is_new_heap))
	{
		return false;
	}

	int nthreads = n_insert_threads + n_delete_threads + n_update_threads
					+ n_query_threads + n_vacuum_threads;
	int ndatas_per_thread = ((data_count % nthreads == 0)
							? (data_count / nthreads)
							:(data_count / nthreads  + 1));
	data_count = ndatas_per_thread * nthreads;

	/* insert data */
	std::vector<std::string> org_data;
	testvacuum_GenDataVector(org_data, data_count, is_toast);
	std::vector<ItemPointerData> tids;
	if(!testvacuum_ConInsert(heap_id, org_data, tids))
	{
		return false;
	}

	/* build data for thread */
	std::multiset<std::string> all_data;
	std::vector<std::vector<std::string> > idata4threads(n_insert_threads);
	std::vector<std::vector<std::string> > udata4threads(n_update_threads);
	/* insert data */
	for(int i = 0; i < n_insert_threads; i++)
	{
		testvacuum_GenDataVector(idata4threads[i], ndatas_per_thread, is_toast);
		for(int j = 0; j < ndatas_per_thread; j++)
		{
			all_data.insert(idata4threads[i][j]);
		}
	}
	/* update data */
	for(int i = 0; i < n_update_threads; i++)
	{
		testvacuum_GenDataVector(udata4threads[i], ndatas_per_thread, is_toast);
		for(int j = 0; j < ndatas_per_thread; j++)
		{
			all_data.insert(udata4threads[i][j]);
		}
	}

	/* get delete tid & updata tid */
	std::vector<std::vector<ItemPointerData> > dtid4threads(n_delete_threads);
	std::vector<std::vector<ItemPointerData> > utid4threads(n_update_threads);
	for(int i = 0; i < data_count; i++)
	{
		if(i % nthreads < n_insert_threads)
		{
			all_data.insert(org_data[i]);
		}
		else if(i % nthreads < n_insert_threads + n_delete_threads)
		{
			dtid4threads[i%nthreads - n_insert_threads].push_back(tids[i]);
		}
		else if(i % nthreads < n_insert_threads + n_delete_threads + n_update_threads)
		{
			utid4threads[i%nthreads - n_insert_threads - n_delete_threads].push_back(tids[i]);
		}
		else
		{
			all_data.insert(org_data[i]);
		}
	}

	/* create thread */
	boost::thread_group g;
	bool *ret = (bool *)malloc(nthreads * sizeof(bool));
	int roff = 0;
	for(int i = 0; i < n_insert_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConInsertThread, heap_id, &idata4threads[i], &ret[i+roff]));
	}
	roff += n_insert_threads;
	for(int i = 0; i < n_delete_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConDeleteThread, heap_id, &dtid4threads[i], &ret[i+roff]));
	}
	roff += n_delete_threads;
	for(int i = 0; i < n_update_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConUpdateThread, heap_id, &utid4threads[i], &udata4threads[i], &ret[i+roff]));
	}
	roff += n_update_threads;
	for(int i = 0; i < n_query_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConQueryThread, heap_id, &ret[i+roff]));
	}
	roff += n_query_threads;
	for(int i = 0; i < n_vacuum_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConVacuumThread, heap_id, &ret[i+roff]));
	}
	g.join_all();

	/* check result */
	for(int i = 0; i < nthreads; i++)
	{
		if(!ret[i])
		{
			free(ret);
			return false;
		}
	}
	free(ret);
	
	std::multiset<std::string> all_data_get;
	if(!testvacuum_ConQuery(heap_id, all_data_get))
	{
		return false;
	}

	/* drop heap */
	bool drop_ret;
	g.create_thread(boost::bind(testvacuum_ConDropThread, heap_id, &drop_ret));
	g.join_all();
	if(!drop_ret)
	{
		return false;
	}

	return (all_data_get == all_data);
}

bool testvacuum_ConTestHeapWithIndex(unsigned int data_count, bool is_toast,
	const int n_insert_threads, const int n_delete_threads, const int n_update_threads,
	const int n_query_threads, const int n_vacuum_threads)
{
	Assert(data_count > 0);

	Oid heap_id;
	bool is_new_heap;
	testvacuum_CreateHeap(heap_id, is_new_heap);
	if((heap_id == InvalidOid) || (!is_new_heap))
	{
		return false;
	}

	Oid idx_id;
	bool is_new_index;
	const unsigned int ncolumns = 1;
	unsigned int columns[ncolumns] = {1};
	testvacuum_CreateIndex(idx_id, is_new_index, ncolumns, columns, heap_id);
	if((idx_id == InvalidOid) || (!is_new_index))
	{
		return false;
	}

	int nthreads = n_insert_threads + n_delete_threads + n_update_threads
					+ n_query_threads + n_vacuum_threads;
	int ndatas_per_thread = ((data_count % nthreads == 0)
							? (data_count / nthreads)
							:(data_count / nthreads  + 1));
	data_count = ndatas_per_thread * nthreads;

	/* insert data */
	std::vector<std::string> org_data;
	testvacuum_GenDataVector(org_data, data_count, is_toast);
	std::vector<ItemPointerData> tids;
	if(!testvacuum_ConInsert(heap_id, org_data, tids))
	{
		return false;
	}

	/* build data for thread */
	std::multiset<std::string> all_data;
	std::vector<std::vector<std::string> > idata4threads(n_insert_threads);
	std::vector<std::vector<std::string> > udata4threads(n_update_threads);
	/* insert data */
	for(int i = 0; i < n_insert_threads; i++)
	{
		testvacuum_GenDataVector(idata4threads[i], ndatas_per_thread, is_toast);
		for(int j = 0; j < ndatas_per_thread; j++)
		{
			all_data.insert(idata4threads[i][j]);
		}
	}
	/* update data */
	for(int i = 0; i < n_update_threads; i++)
	{
		testvacuum_GenDataVector(udata4threads[i], ndatas_per_thread, is_toast);
		for(int j = 0; j < ndatas_per_thread; j++)
		{
			all_data.insert(udata4threads[i][j]);
		}
	}

	/* get delete tid & updata tid */
	std::vector<std::vector<ItemPointerData> > dtid4threads(n_delete_threads);
	std::vector<std::vector<ItemPointerData> > utid4threads(n_update_threads);
	for(int i = 0; i < data_count; i++)
	{
		if(i % nthreads < n_insert_threads)
		{
			all_data.insert(org_data[i]);
		}
		else if(i % nthreads < n_insert_threads + n_delete_threads)
		{
			dtid4threads[i%nthreads - n_insert_threads].push_back(tids[i]);
		}
		else if(i % nthreads < n_insert_threads + n_delete_threads + n_update_threads)
		{
			utid4threads[i%nthreads - n_insert_threads - n_delete_threads].push_back(tids[i]);
		}
		else
		{
			all_data.insert(org_data[i]);
		}
	}

	/* create thread */
	boost::thread_group g;
	bool *ret = (bool *)malloc(nthreads * sizeof(bool));
	int roff = 0;
	for(int i = 0; i < n_insert_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConInsertThread, heap_id, &idata4threads[i], &ret[i+roff]));
	}
	roff += n_insert_threads;
	for(int i = 0; i < n_delete_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConDeleteThread, heap_id, &dtid4threads[i], &ret[i+roff]));
	}
	roff += n_delete_threads;
	for(int i = 0; i < n_update_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConUpdateThread, heap_id, &utid4threads[i], &udata4threads[i], &ret[i+roff]));
	}
	roff += n_update_threads;
	for(int i = 0; i < n_query_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConQueryByIndexThread, heap_id, idx_id, &ret[i+roff]));
	}
	roff += n_query_threads;
	for(int i = 0; i < n_vacuum_threads; i++)
	{
		g.create_thread(boost::bind(testvacuum_ConVacuumThread, heap_id, &ret[i+roff]));
	}
	g.join_all();

	/* check result */
	for(int i = 0; i < nthreads; i++)
	{
		if(!ret[i])
		{
			free(ret);
			return false;
		}
	}
	free(ret);
	
	std::multiset<std::string> all_data_get;
	if(!testvacuum_ConQueryByIndex(heap_id, idx_id, all_data_get))
	{
		return false;
	}

	/* drop heap & index */
	bool drop_ret;
	g.create_thread(boost::bind(testvacuum_ConDropThread, heap_id, &drop_ret));
	g.join_all();
	if(!drop_ret)
	{
		return false;
	}

	return (all_data_get == all_data);
}

bool testvacuum_ConTestSimpleHeap(void)
{
	const int n_insert_threads = 5;
	const int n_delete_threads = 5;
	const int n_update_threads = 5;
	const int n_query_threads = 5;
	const int n_vacuum_threads = 5;
	return testvacuum_ConTestHeap(50000, false, n_insert_threads, n_delete_threads,
				n_update_threads, n_query_threads, n_vacuum_threads);
}

bool testvacuum_ConTestToast(void)
{
	const int n_insert_threads = 5;
	const int n_delete_threads = 5;
	const int n_update_threads = 5;
	const int n_query_threads = 5;
	const int n_vacuum_threads = 5;
	return testvacuum_ConTestHeap(5000, true, n_insert_threads, n_delete_threads,
				n_update_threads, n_query_threads, n_vacuum_threads);
}

bool testvacuum_ConTestHeapWithIndex(void)
{
	const int n_insert_threads = 5;
	const int n_delete_threads = 5;
	const int n_update_threads = 5;
	const int n_query_threads = 5;
	const int n_vacuum_threads = 5;
	return testvacuum_ConTestHeap(50000, false, n_insert_threads, n_delete_threads,
				n_update_threads, n_query_threads, n_vacuum_threads);
}

bool testvacuum_ConTestToastWithIndex(void)
{
	const int n_insert_threads = 5;
	const int n_delete_threads = 5;
	const int n_update_threads = 5;
	const int n_query_threads = 5;
	const int n_vacuum_threads = 5;
	return testvacuum_ConTestHeapWithIndex(5000, true, n_insert_threads,
		n_delete_threads, n_update_threads, n_query_threads, n_vacuum_threads);
}


/* for test frozenxid */
bool testvacuum_InsertManyXact(Oid heap_id,
	std::vector<std::string> &datas,
	std::vector<ItemPointerData> &tids)
{
	int ndatas = datas.size();
	tids.resize(ndatas);

	for(int i = 0; i < ndatas; i++)
	{
		begin_transaction();
		try
		{
			Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);
			HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(datas[i].c_str(),
															datas[i].length());
			FDPG_Heap::fd_simple_heap_insert(rel, tup);
			/* fill in tid */
			tids[i] = tup->t_self;
			FDPG_Heap::fd_heap_close(rel, NoLock);
		}
		catch(StorageEngineException &ex)
		{
			user_abort_transaction();
			std::cout << ex.getErrorNo() << std::endl;
			std::cout << ex.getErrorMsg() << std::endl;
			return false;
		}
		commit_transaction();
	}
	
	return true;
}

bool testvacuum_DeleteManyXact(Oid heap_id,
	std::vector<ItemPointerData> &tids)
{
	for(int i = 0; i < tids.size(); i++)
	{
		begin_transaction();
		try
		{
			Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);

			/* delete tuples */
			ItemPointer p_tid = (ItemPointer)(&tids[i]);
			FDPG_Heap::fd_simple_heap_delete(rel, p_tid);
			FDPG_Heap::fd_heap_close(rel, NoLock);
		}
		catch(StorageEngineException &ex)
		{
			user_abort_transaction();
			std::cout << ex.getErrorNo() << std::endl;
			std::cout << ex.getErrorMsg() << std::endl;
			return false;
		}
		commit_transaction();
	}
	
	return true;
}

bool testvacuum_UpdateManyXact(Oid heap_id,
	std::vector<ItemPointerData> &tids,
	std::vector<std::string> &datas)
{
	for(int i = 0; i < tids.size(); i++)
	{
		begin_transaction();
		try
		{
			Relation rel = FDPG_Heap::fd_heap_open(heap_id, AccessShareLock);

			/* delete tuples */
			ItemPointer p_tid = (ItemPointer)(&tids[i]);
			HeapTuple tup_update = FDPG_Heap::fd_heap_form_tuple(datas[i].c_str(),
											datas[i].length());
		    FDPG_Heap::fd_simple_heap_update(rel, p_tid, tup_update);
			FDPG_Heap::fd_heap_close(rel, NoLock);
		}
		catch(StorageEngineException &ex)
		{
			user_abort_transaction();
			std::cout << ex.getErrorNo() << std::endl;
			std::cout << ex.getErrorMsg() << std::endl;
			return false;
		}
		commit_transaction();
	}
	
	return true;
}

bool testvacuum_InsertUpdateDeleteManyXact(Oid heap_id,
	unsigned int data_count, bool is_toast)
{
	std::vector<std::string> org_data;
	std::vector<std::string> update_data;
	testvacuum_GenDataVector(org_data, data_count, false);
	testvacuum_GenDataVector(update_data, data_count/3, false);

	/* insert all org_data */
	std::vector<ItemPointerData> tids;
	if(!testvacuum_InsertManyXact(heap_id, org_data, tids))
	{
		return false;
	}

	/* get delete tid & updata tid */
	std::vector<ItemPointerData> delete_tids;
	std::vector<ItemPointerData> update_tids;
	for(int i = 0; i < data_count; i++)
	{
		switch(i % 3)
		{
		case 0:
			update_tids.push_back(tids[i]);
			break;
		case 2:
			delete_tids.push_back(tids[i]);
			break;
		default:
			break;
		}
	}

	/* update */
	if(!testvacuum_UpdateManyXact(heap_id, update_tids, update_data))
	{
		return false;
	}

	/* delete */
	if(!testvacuum_DeleteManyXact(heap_id, delete_tids))
	{
		return false;
	}

	return true;
}

bool testvacuum_DatabaseOnce(Oid heap_id, unsigned int data_cout)
{
	testvacuum_InsertUpdateDeleteManyXact(heap_id, data_cout, false);

	std::multiset<std::string> all_data;
	if(!testvacuum_ConQuery(heap_id, all_data))
	{
		return false;
	}

	/* vacuum database */
	try
	{
		FDPG_Database::fd_VacuumDatabase();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return false;
	}

	/* get data */
	std::multiset<std::string> all_data_2;
	if(!testvacuum_ConQuery(heap_id, all_data_2))
	{
		return false;
	}

	return (all_data == all_data_2);
}

bool testvacuum_Database(void)
{
	extern THREAD_LOCAL int	vacuum_freeze_min_age;
	extern THREAD_LOCAL int	vacuum_freeze_table_age;

	int old_vacuum_freeze_min_age = vacuum_freeze_min_age;
	int old_vacuum_freeze_table_age = vacuum_freeze_table_age;
	
	vacuum_freeze_min_age = 100;
	vacuum_freeze_table_age = 400;
	
	Oid heap_id;
	bool is_new_heap;
	testvacuum_CreateHeap(heap_id, is_new_heap);
	if((heap_id == InvalidOid) || (!is_new_heap))
	{
		return false;
	}

	/* 
	 * data_count_1 * 5 / 3
	 * must > vacuum_freeze_min_age && <  vacuum_freeze_table_age
	 */
	unsigned int data_count_1 = 90;
	if(!testvacuum_DatabaseOnce(heap_id, data_count_1))
	{
		return false;
	}

	/* 
	 * (data_count_2 + data_count_1) * 5 / 3
	 * must > vacuum_freeze_table_age
	 */
	unsigned int data_count_2 = 300;
	if(!testvacuum_DatabaseOnce(heap_id, data_count_2))
	{
		return false;
	}

	vacuum_freeze_min_age = old_vacuum_freeze_min_age;
	vacuum_freeze_table_age = old_vacuum_freeze_table_age;

	/* drop heap */
	if(!testvacuum_ConDrop(heap_id))
	{
		return false;
	}

	return true;
}

