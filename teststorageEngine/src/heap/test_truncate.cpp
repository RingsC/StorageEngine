#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "utils/fmgroids.h"
#include "catalog/heap.h"

#include "utils/util.h"
#include "test_fram.h"
#include "heap/test_truncate.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/utils.h"
#include "DataItem.h"
#include "exception/XdbExceptions.h"

using namespace FounderXDB::StorageEngineNS;
using namespace FounderXDB::EXCEPTION;

//#define TESTTRUNCATE_DEBUG 1
//#define TESTTRUNCATE_DEBUG_ENTRY 1
#ifdef TESTTRUNCATE_DEBUG_ENTRY
#define testtruncate_EnterFunc() \
do{ \
	std::cout << "==Enter " << __FUNCTION__ << std::endl; \
}while(0)
#define testtruncate_ExitFunc() \
do{ \
	std::cout << "==Exit " << __FUNCTION__ << std::endl; \
}while(0)
#else
#define testtruncate_EnterFunc()
#define testtruncate_ExitFunc()
#endif

typedef std::multiset<std::string> data_t;

#define TESTTRUNCATE_MAX_IDX_COUNT 2

#define TESTTRUNCATE_CATCH_THROW \
catch(StorageEngineExceptionUniversal &ex)\
{\
	throw ex; \
}

#define TESTTRUNCATE_CATCH_THROW_ABTTRANS \
catch(StorageEngineExceptionUniversal &ex)\
{\
	user_abort_transaction();\
	throw ex; \
}

#define TESTTRUNCATE_CATCH_RETURN \
catch(StorageEngineExceptionUniversal &ex){ \
	std::cout << ex.getErrorNo() << std::endl; \
	std::cout << ex.getErrorMsg() << std::endl; \
	return false; \
}

#define TESTTRUNCATE_CATCH_RETURN_ABTTRANS \
catch(StorageEngineExceptionUniversal &ex){ \
	std::cout << ex.getErrorNo() << std::endl; \
	std::cout << ex.getErrorMsg() << std::endl; \
	user_abort_transaction();\
	return false; \
}






void testtruncate_DumpData(data_t data, bool is_dump = false)
{
#ifdef TESTTRUNCATE_DEBUG
	is_dump = true;
#endif

	if(is_dump){
		std::cout << std::endl << std::endl << "========================"
			<< "===========================================" << std::endl;

		for(data_t::const_iterator it = data.begin();
			it != data.end();
			++it)
		{
			std::cout << *it << std::endl;
		}

		std::cout << "================================================="
			<< "==================" << std::endl << std::endl << std::endl;
	}
}


int testtruncate_CompareBytes4(const char *str1, size_t len1,
									const char *str2, size_t len2)
{
	testtruncate_EnterFunc();

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

	//std::cout << "i1: "<< i1 << "   i2: " << i2 << std::endl;

	if(i1 > i2){
		return 1;
	}else if(i1 < i2){
		return -1;
	}else{
		return 0;
	}

	testtruncate_ExitFunc();

	return 0;
}

void testtruncate_HeapSplitFunc(RangeData& rangeData, const char *str, int col, size_t data_len)
{
	memset(&rangeData, 0, sizeof(rangeData));
	if(data_len <= 12){
		Assert(false);
		rangeData.start = 0;
		rangeData.len = 0;
		return ;
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
		default:
			std::cerr << "wrong col " << col << std::endl;
			break;
	}
}

void testtruncate_IndexSplitFunc(RangeData& rangeData, const char *str, int col, size_t data_len)
{
	memset(&rangeData, 0, sizeof(rangeData));

	if(col == 1){
		rangeData.start = 0;
		rangeData.len = 4;
	}else if(col == 2){
		rangeData.start = 4;
		rangeData.len = 4;
	}else{
		rangeData.start = 0;
		rangeData.len = 0;
	}
}









/*
* create heap and index
* Desciption:		create heap which has 4 columns(col1_len -- 4, col2_len -- 4, col3_len -- 4, col4_len -- rest)
*				And create idx_count indexes which type match idx_type array
* Input:			idx_count must <= 2
* 				if idx_type[i] is unique, index[i] create on col1, or create on col2;
* return:			heap_id -- the Oid of newly created heap
* 				idx_id -- the Oids of newly created indexes
*
* Note: 			data length larger than 12(4+4+4) is better
*
*/
void testtruncate_SetHeapColinfo(Oid heap_id)
{
	Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));

	Assert(colinfo != NULL);

	colinfo->col_number = 0;
	colinfo->keys = 0;
	colinfo->rd_comfunction = NULL;
	colinfo->split_function = testtruncate_HeapSplitFunc;

	setColInfo(heap_id, colinfo);
	return;
}
void testtruncate_SetIndexColinfo(Oid idx_id,
	unsigned int count_of_index_on_col, unsigned int *col_number)
{
	Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));

	colinfo->keys = count_of_index_on_col;
	colinfo->col_number = (size_t *)malloc(colinfo->keys * sizeof(size_t));
	colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
	for(unsigned int i = 0; i < count_of_index_on_col; i++){
		colinfo->col_number[i] = col_number[i];
		colinfo->rd_comfunction[i] = testtruncate_CompareBytes4;
	}
	colinfo->split_function =  testtruncate_IndexSplitFunc;//indexµÄsplit

	/* set idx_col_id = idx_id */
	setColInfo(idx_id, colinfo);
	return;
}

bool testtruncate_CreateHeapAndIndexes(Oid &heap_id,
	unsigned int idx_count, IndexType *idx_type, Oid *idx_id)
{
	testtruncate_EnterFunc();

	if(idx_count != 0){
		Assert(idx_id != NULL);
		Assert(idx_type != NULL);
	}

	try
	{
		Assert(heap_id != InvalidOid);
		begin_transaction();
		testtruncate_SetHeapColinfo(heap_id);
		heap_id = FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, heap_id, MyDatabaseId, heap_id);
	}
	TESTTRUNCATE_CATCH_RETURN_ABTTRANS

	try
	{
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, RowExclusiveLock, MyDatabaseId);

		for(unsigned int i = 0; i < idx_count; i++){
			Assert(idx_id[i] != InvalidOid);
			if(idx_type[i] >= UINQUE_TYPE_BASE){
				unsigned int col_number[1] = {1};
				testtruncate_SetIndexColinfo(idx_id[i], 1, col_number);
				idx_id[i] = FDPG_Index::fd_index_create(rel, idx_type[i], idx_id[i], idx_id[i]);
			}else{
				#if 1
				unsigned int col_number[1] = {2};
				testtruncate_SetIndexColinfo(idx_id[i], 1, col_number);
				idx_id[i] = FDPG_Index::fd_index_create(rel, idx_type[i], idx_id[i], idx_id[i]);
				#else
				size_t col_number[2] = {2, 3};
				testtruncate_SetIndexColinfo(idx_id[i], 2, (unsigned int *)col_number);
				idx_id[i] = FDPG_Index::fd_index_create(rel, idx_type[i], idx_id[i], idx_id[i]);
				#endif
			}
		}

		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	}
	TESTTRUNCATE_CATCH_RETURN_ABTTRANS

	commit_transaction();

	testtruncate_ExitFunc();

	return true;
}

bool testtruncate_DropHeapAndIndex(Oid heap_id)
{
	testtruncate_EnterFunc();

	try{
		begin_transaction();
		FDPG_Heap::fd_heap_drop(heap_id, MyDatabaseId);
		commit_transaction();
	}
	TESTTRUNCATE_CATCH_RETURN_ABTTRANS

	testtruncate_ExitFunc();

	return true;
}

void testtruncate_OpenHeapAndIndexes(Oid heap_id,
	unsigned int idx_count, Oid *idx_id,
	Relation &rel, Relation *idx_rel)
{
	testtruncate_EnterFunc();

	rel = FDPG_Heap::fd_heap_open(heap_id, RowExclusiveLock, MyDatabaseId);
	for(unsigned int i = 0; i < idx_count; i++){
		idx_rel[i] = FDPG_Index::fd_index_open(idx_id[i], RowExclusiveLock, MyDatabaseId);
	}

	testtruncate_ExitFunc();

	return;
}

void testtruncate_CloseHeapAndIndexes(Relation rel,
	unsigned int idx_count, Relation *idx_rel)
{
	testtruncate_EnterFunc();

	FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	for(unsigned int i = 0; i < idx_count; i++){
		FDPG_Index::fd_index_close(idx_rel[i], RowExclusiveLock);
	}

	testtruncate_ExitFunc();

	return;
}

void testtruncate_HeapGetAll(Relation rel, data_t &scan_data)
{
	testtruncate_EnterFunc();

	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	HeapTuple tup = NULL;

	scan_data.clear();
	while((tup = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL){	  
		int len = 0;
		char *data_tuple = fdxdb_tuple_to_chars_with_len(tup, len);
		scan_data.insert(std::string((char*)(data_tuple), len));
		pfree(data_tuple);
	}

	FDPG_Heap::fd_heap_endscan(scan);

	testtruncate_ExitFunc();

	return;
}

void testtruncate_IndexGetAll(Relation rel, Relation idx_rel,
	data_t &scan_data)
{
	testtruncate_EnterFunc();

	const int nkeys = 1;
	char cmp_data[4] = {'9', '9', '9', '9'};
	Datum datum = FDPG_Common::fd_string_formdatum(cmp_data, sizeof(cmp_data));
	ScanKeyData scan_key[nkeys];
	IndexScanDesc idx_scan = NULL;
	HeapTuple tup = NULL;

	/* init scankey */
	FDPG_ScanKey::fd_ScanKeyInitWithCallbackInfo(&scan_key[0], 1,
		BTLessEqualStrategyNumber, testtruncate_CompareBytes4, datum);

	idx_scan = FDPG_Index::fd_index_beginscan(rel, idx_rel, SnapshotNow, nkeys, 0);
	FDPG_Index::fd_index_rescan(idx_scan, scan_key, nkeys, NULL, 0);
	scan_data.clear();
	while((tup = FDPG_Index::fd_index_getnext(idx_scan, BackwardScanDirection)) != NULL)
	{
		int data_len = 0;
		char *data_tuple = FDPG_Common::fd_tuple_to_chars_with_len(tup, data_len);
		scan_data.insert(std::string((char*)(data_tuple), data_len));
		pfree(data_tuple);
	}
	FDPG_Index::fd_index_endscan(idx_scan);

	testtruncate_ExitFunc();

	return;
}

bool testtruncate_InsertAndScanData(Relation rel,
	Relation *idx_rel, unsigned int idx_count, data_t &data)
{
	testtruncate_EnterFunc();

	data_t scan_data;

	/* insert data */
	for(data_t::const_iterator it = data.begin();it != data.end();++it){
		HeapTuple tup = FDPG_Heap::fd_heap_form_tuple((*it).c_str(), (*it).length());
		FDPG_Heap::fd_simple_heap_insert(rel, tup);
	}
	FDPG_Transaction::fd_CommandCounterIncrement();

	/* heap scan */
	testtruncate_HeapGetAll(rel, scan_data);
	if(scan_data != data){
		std::cerr << "heap scan result is not match insert data" << std::endl;
		testtruncate_DumpData(scan_data, true);
		testtruncate_DumpData(data, true);
		return false;
	}

	/* index scan */
	for(unsigned int i = 0; i < idx_count; i++){
		testtruncate_IndexGetAll(rel, idx_rel[i], scan_data);
		if(scan_data != data){
			std::cerr << "index scan result is not match insert data" << std::endl;
			testtruncate_DumpData(scan_data, true);
			testtruncate_DumpData(data, true);
			return false;
		}
	}

	testtruncate_ExitFunc();

	return true;
}

bool testtruncate_CheckTruncateResult(Relation rel,
	Relation *idx_rel, unsigned int idx_count)
{
	testtruncate_EnterFunc();

	data_t scan_data;
	testtruncate_HeapGetAll(rel, scan_data);
	if(!scan_data.empty()){
		std::cerr << "truncate failed, can also get data." << std::endl;
		return false;
	}

	testtruncate_ExitFunc();

	return true;
}








bool testtruncate_InOneTrans(Oid heap_id,
	unsigned int idx_count, Oid *idx_id, 
	data_t &data, data_t &data2)
{
	testtruncate_EnterFunc();

	bool ret = true;

	/* check param */
	Assert(idx_count <= 2);
	if(idx_count != 0){
		Assert(idx_id != NULL);
	}

	try{
		Relation rel;
		Relation idx_rel[TESTTRUNCATE_MAX_IDX_COUNT];

		begin_transaction();

		testtruncate_OpenHeapAndIndexes(heap_id, idx_count, idx_id, rel, idx_rel);

		/* Step1: insert data */
		if(!testtruncate_InsertAndScanData(rel, idx_rel, idx_count, data)){
			ret = false;
			goto out;
		}

		/* Step2: truncate */
		//std::cout << "FDPG_Heap::fd_heap_truncate start." << std::endl;
		FDPG_Heap::fd_heap_truncate(rel);
		//std::cout << "FDPG_Heap::fd_heap_truncate stop." << std::endl;

		/* Step3: check truncate result */
		if(!testtruncate_CheckTruncateResult(rel, idx_rel, idx_count)){
			ret = false;
			goto out;
		}

		/* Step4: insert data after truncate */
		if(!testtruncate_InsertAndScanData(rel, idx_rel, idx_count, data2)){
			ret = false;
			goto out;
		}
	out:
		/* close rel & idx rel */
		testtruncate_CloseHeapAndIndexes(rel, idx_count, idx_rel);

		if(ret){
			commit_transaction();
		}else{
			user_abort_transaction();
		}
	}
	TESTTRUNCATE_CATCH_THROW_ABTTRANS
		
	testtruncate_ExitFunc();

	return ret;
}

bool testtruncate_InMultiTrans(Oid heap_id,
	unsigned int idx_count, Oid *idx_id, 
	data_t &data)
{
	testtruncate_EnterFunc();

	bool ret = true;

	/* Step1: truncate in another trans */
	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(heap_id, RowExclusiveLock, MyDatabaseId);
		FDPG_Heap::fd_heap_truncate(rel);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
	}
	TESTTRUNCATE_CATCH_THROW_ABTTRANS

	/* Step2: check truncate result and insert */
	try
	{
		Relation rel;
		Relation idx_rel[TESTTRUNCATE_MAX_IDX_COUNT];

		begin_transaction();
		testtruncate_OpenHeapAndIndexes(heap_id, idx_count, idx_id, rel, idx_rel);

		if(!testtruncate_CheckTruncateResult(rel, idx_rel, idx_count)){
			ret = false;
			goto out;
		}

		if(!testtruncate_InsertAndScanData(rel, idx_rel, idx_count, data)){
			ret = false;
			goto out;
		}

		
	out:
		/* close rel & idx rel */
		testtruncate_CloseHeapAndIndexes(rel, idx_count, idx_rel);

		if(!ret){
			user_abort_transaction();
		}else{
			commit_transaction();
		}
	}
	TESTTRUNCATE_CATCH_THROW_ABTTRANS

	testtruncate_ExitFunc();

	return ret;
}

bool testtruncate(data_t &data1, data_t &data2, data_t &data3,
	IndexType *idx_type, unsigned int idx_count)
{
	testtruncate_EnterFunc();

	Oid heap_id = OIDGenerator::instance().GetHeapID();
	Oid *idx_id = NULL;

	if(idx_count != 0){
		idx_id = (Oid *)malloc(idx_count * sizeof(Oid));
		for(unsigned int i = 0; i < idx_count; i++){
			idx_id[i] = OIDGenerator::instance().GetIndexID();
		}
	}

	if(!testtruncate_CreateHeapAndIndexes(heap_id, idx_count, idx_type, idx_id)){
		std::cerr << "testtruncate_CreateHeapAndIndexes failed" << std::endl;
		return false;
	}

	try{
		if(!testtruncate_InOneTrans(heap_id, idx_count, idx_id, data1, data2)){
			std::cerr << "testtruncate_InOneTrans failed" << std::endl;
			return false;
		}

		if(!testtruncate_InMultiTrans(heap_id, idx_count, idx_id, data3)){
			std::cerr << "testtruncate_InMultiTrans failed" << std::endl;
			return false;
		}
	}
	catch(StorageEngineExceptionUniversal &ex)
	{
		testtruncate_DropHeapAndIndex(heap_id);
		throw ex;
	}

	if(!testtruncate_DropHeapAndIndex(heap_id)){
		std::cerr << "testtruncate_DropHeapAndIndex failed" << std::endl;
		return false;
	}

	testtruncate_ExitFunc();

	return true;
}




#if 0
void testtruncate_StopEngine_SetIds(Oid &heap_id,
	unsigned int idx_count, Oid *idx_id)
{
	testtruncate_EnterFunc();

	heap_id = 64889;

	for(unsigned i = 0; i < idx_count; i++){
		idx_id[i] = heap_id + (i + 1);
	}

	return;
}

bool testtruncate_StopEngine_step1(data_t &data,
	IndexType *idx_type, unsigned int idx_count)
{
	testtruncate_EnterFunc();

	Oid heap_id;
	Oid idx_id[TESTTRUNCATE_MAX_IDX_COUNT];
	Relation rel;
	Relation idx_rel[TESTTRUNCATE_MAX_IDX_COUNT];
	bool ret = true;

	if(idx_count != 0){
		Assert(idx_type != NULL);
	}

	/* set heap & indexes id */
	testtruncate_StopEngine_SetIds(heap_id, idx_count, idx_id);

	if(!testtruncate_CreateHeapAndIndexes(heap_id, idx_count, idx_type, idx_id)){
		std::cerr << "testtruncate_CreateHeapAndIndexes failed" << std::endl;
		return false;
	}

	try{
		begin_transaction();

		/* open heap & indexes */
		testtruncate_OpenHeapAndIndexes(heap_id, idx_count, idx_id, rel, idx_rel);

		if(!testtruncate_InsertAndScanData(rel, idx_rel, idx_count, data)){
			std::cerr << "testtruncate_CreateHeapAndIndexes failed" << std::endl;
			ret = false;
			goto out;
		}

		FDPG_Heap::fd_heap_truncate(rel);

	out:
		/* close heap & indexes */
		testtruncate_CloseHeapAndIndexes(rel, idx_count, idx_rel);

		if(ret){
			commit_transaction();
		}else{
			user_abort_transaction();
		}
	}
	TESTTRUNCATE_CATCH_RETURN_ABTTRANS

	testtruncate_ExitFunc();

	return ret;
}

bool testtruncate_StopEngine_step2(data_t &data,
	IndexType *idx_type, unsigned int idx_count)
{
	testtruncate_EnterFunc();

	Oid heap_id;
	Oid idx_id[TESTTRUNCATE_MAX_IDX_COUNT];
	Relation rel;
	Relation idx_rel[TESTTRUNCATE_MAX_IDX_COUNT];
	bool ret = true;

	/* set heap & indexes id */
	testtruncate_StopEngine_SetIds(heap_id, idx_count, idx_id);

	/* set colinfo */
	testtruncate_SetHeapColinfo(heap_id);
	for(unsigned int i = 0; i < idx_count; i++){
		if(idx_type[i] >= UINQUE_TYPE_BASE){
			unsigned int col_number[1] = {1};
			testtruncate_SetIndexColinfo(idx_id[i], 1, col_number);
		}else{
			unsigned int col_number[1] = {2};
			testtruncate_SetIndexColinfo(idx_id[i], 1, col_number);
		}
	}

	try
	{
		begin_transaction();

		/* open heap & indexes */
		testtruncate_OpenHeapAndIndexes(heap_id, idx_count, idx_id, rel, idx_rel);

		if(!testtruncate_CheckTruncateResult(rel, idx_rel, idx_count)){
			std::cerr << "testtruncate_CheckTruncateResult failed" << std::endl;
			ret = false;
			goto out;
		}

		if(!testtruncate_InsertAndScanData(rel, idx_rel, idx_count, data)){
			std::cerr << "testtruncate_CreateHeapAndIndexes failed" << std::endl;
			ret = false;
			goto out;
		}
	out:
		/* close heap & indexes */
		testtruncate_CloseHeapAndIndexes(rel, idx_count, idx_rel);

		if(ret){
			commit_transaction();
		}else{
			user_abort_transaction();
		}
	}
	catch(StorageEngineExceptionUniversal &ex)
	{
		testtruncate_DropHeapAndIndex(heap_id);
		user_abort_transaction();
		throw ex;
	}

	testtruncate_ExitFunc();	

	return true;
}
#endif







/*********************************************************************
*			test simple
*********************************************************************/
void testtruncate_GenRandStr(std::string &str, unsigned int nLen)
{
	for(unsigned int i = 0; i < nLen - 1; i++){
		str += 'a' + (rand()%26);
	}
	str += '\0';

	return;
}

/*
* Generate test datum
* Note:
* first 12 bytes composited by three 4bytes(int)
* last is random string
*/
void testtruncate_GenTestDatum(std::string &str, unsigned int nLen)
{
	static int data_count = 0;
	unsigned int i1 = ++data_count;
	//unsigned int i2 = rand()%9999;
	unsigned int i2 = i1;
	unsigned int i3 = rand()%9999;
	char first_4bytes[8];
	char second_4bytes[8];
	char third_4bytes[8];

	snprintf(first_4bytes, 8, "%04d", i1);
	snprintf(second_4bytes, 8, "%04d", i2);
	snprintf(third_4bytes, 8, "%04d", i3);

	str += first_4bytes;
	str += second_4bytes;
	str += third_4bytes;

	testtruncate_GenRandStr(str, nLen - 12);

#ifdef TESTTRUNCATE_DEBUG
	std::cout << "------------------------------------"
		<< "------------------------------------" << std::endl;
	std::cout << "Data[" << data_count << "]" << " ---- "
		<< "i1: " << i1 << "i2: " << i2 << "i3: " << i3
		<< std::endl << std::endl;
	std::cout << "len: <" << str.length() << " | " << nLen << ">" << std::endl;
	std::cout << "content: <" << str << ">" << std::endl;
	std::cout << "------------------------------------"
		<< "------------------------------------" << std::endl;
#endif

	return;
}

void testtruncate_GenUniqueTestData(bool is_toast,
	unsigned int data_count, std::set<std::string> &data)
{
	for(unsigned int i = 0; i < data_count; i++){
		std::string rand_str;
		unsigned int data_len = 0;
		if((is_toast) && (i%2 == 0)){
			data_len = rand() % 5000 + 5000;
		}else{
			data_len = rand() % 1000 + 100;
		}

		testtruncate_GenTestDatum(rand_str, data_len);

		data.insert(rand_str);
	}

	return;
}


/*
* Generate test data
* Note:
* first 12 bytes composited by three 4bytes(int)
* last is string
*/
void testtruncate_GenTestData(bool is_toast, bool is_unique,
	unsigned int data_count, data_t &data)
{
	unsigned int unique_data_count = (is_unique ? data_count : data_count/2);
	std::set<std::string> unique_data;

	//std::cout << std::endl << std::endl << "testtruncate_GenTestData" << std::endl;

	if(!is_unique){
		Assert(data_count >= 2);
		Assert(data_count % 2 == 0);
	}

	unique_data_count = (is_unique ? data_count : data_count/2);
	testtruncate_GenUniqueTestData(is_toast, unique_data_count, unique_data);

	for(unsigned int i = 0; i < (data_count/unique_data_count); i++){
		for(std::set<std::string>::const_iterator it = unique_data.begin();
			it != unique_data.end();
			++it)
		{
			Assert(data.size() < data_count);
			data.insert(*it);
		}
	}

	Assert(data.size() == data_count);

	testtruncate_DumpData(data);
	return;
}

bool testtruncate_NoIndex(bool is_toast)
{
	bool ret = true;
	data_t data;
	data_t unique_data;

	testtruncate_GenTestData(is_toast, true, 2, unique_data);
	testtruncate_GenTestData(is_toast, false, 2, data);

	try{
		ret = testtruncate(unique_data, unique_data, unique_data, NULL, 0);
		if(!ret){
			std::cerr << "testtruncate_Heap use unique data failed" << std::endl;
			return false;
		}

		ret = testtruncate(data, data, data, NULL, 0);
		if(!ret){
			std::cerr << "testtruncate_Heap use repetitive data failed" << std::endl;
			return false;
		}
	}
	TESTTRUNCATE_CATCH_RETURN

	return ret;
}

bool testtruncate_OneIndex(bool is_toast, bool is_unique)
{
	bool ret = true;
	data_t data;
	data_t unique_data;
	IndexType idx_type[1];

	testtruncate_GenTestData(is_toast, true, 2, unique_data);
	testtruncate_GenTestData(is_toast, false, 2, data);

	idx_type[0] = (is_unique ? BTREE_UNIQUE_TYPE : BTREE_TYPE);
	try{
		ret = testtruncate(unique_data, unique_data, unique_data, idx_type, 1);
		if(!ret){
			std::cerr << "testtruncate_OneIndex use unique data failed" << std::endl;
			return false;
		}
	}
	TESTTRUNCATE_CATCH_RETURN

	try{
		ret = testtruncate(unique_data, unique_data, data, idx_type, 1);
		if(is_unique){
			std::cerr << "testtruncate_OneIndex use repetitive data failed." << std::endl;
			std::cerr << "It should be throw exception when insert"
				<< " repetitive data into heap with unique index." << std::endl;
			return false;
		}else if(!ret){
			std::cerr << "testtruncate_OneIndex use repetitive data failed" << std::endl;
			return false;
		}
	}
	catch(UniqueViolationException& ex){
		printf("%s\n", ex.getErrorMsg());
		if((is_unique) && (ex.getErrorNo() == XdbBaseException::ERR_STORAGE_UNIQUE_VIOLATION)){
			return true;
		}else{
			return false;
		}
	}

	return ret;
}

bool testtruncate_TwoIndexes(bool is_toast)
{
	bool ret = true;
	data_t data;
	data_t unique_data;
	IndexType idx_type[2];

	testtruncate_GenTestData(is_toast, true, 2, unique_data);
	testtruncate_GenTestData(is_toast, false, 2, data);

	idx_type[0] = BTREE_UNIQUE_TYPE;
	idx_type[1] = BTREE_TYPE;

	try{
		ret = testtruncate(unique_data, unique_data, unique_data, idx_type, 2);
		if(!ret){
			std::cerr << "testtruncate_OneIndex use unique data failed" << std::endl;
			return false;
		}
	}
	TESTTRUNCATE_CATCH_RETURN

	try{
		ret = testtruncate(unique_data, unique_data, data, idx_type, 2);
		std::cerr << "testtruncate_OneIndex use repetitive data failed." << std::endl;
		std::cerr << "It should be throw exception when insert"
			<< " repetitive data into heap with unique index." << std::endl;
		return false;
	}
	catch(UniqueViolationException& ex){
		printf("%s\n", ex.getErrorMsg());
		if(ex.getErrorNo() == XdbBaseException::ERR_STORAGE_UNIQUE_VIOLATION){
			return true;
		}else{
			return false;
		}
	}

	return ret;
}

void testtruncate_StopEngine_SetIds(Oid &heap_id,
	unsigned int idx_count, Oid *idx_id)
{
	testtruncate_EnterFunc();

	heap_id = 64889;

	for(unsigned i = 0; i < idx_count; i++){
		idx_id[i] = heap_id + (i + 1);
	}

	return;
}

bool testtruncate_StopEngine_step1(void)
{
	testtruncate_EnterFunc();

	Oid heap_id;
	Oid idx_id[TESTTRUNCATE_MAX_IDX_COUNT];
	Relation rel;
	Relation idx_rel[TESTTRUNCATE_MAX_IDX_COUNT];
	bool ret = true;

	data_t unique_data;
	IndexType idx_type[TESTTRUNCATE_MAX_IDX_COUNT];
	unsigned int idx_count = TESTTRUNCATE_MAX_IDX_COUNT;
	testtruncate_GenTestData(true, true, 2, unique_data);
	idx_type[0] = BTREE_UNIQUE_TYPE;
	idx_type[1] = BTREE_TYPE;

	/* set heap & indexes id */
	testtruncate_StopEngine_SetIds(heap_id, idx_count, idx_id);

	if(!testtruncate_CreateHeapAndIndexes(heap_id, idx_count, idx_type, idx_id)){
		std::cerr << "testtruncate_CreateHeapAndIndexes failed" << std::endl;
		return false;
	}

	try{
		begin_transaction();

		/* open heap & indexes */
		testtruncate_OpenHeapAndIndexes(heap_id, idx_count, idx_id, rel, idx_rel);

		if(!testtruncate_InsertAndScanData(rel, idx_rel, idx_count, unique_data)){
			std::cerr << "testtruncate_CreateHeapAndIndexes failed" << std::endl;
			ret = false;
			goto out;
		}

		FDPG_Heap::fd_heap_truncate(rel);

	out:
		/* close heap & indexes */
		testtruncate_CloseHeapAndIndexes(rel, idx_count, idx_rel);

		if(ret){
			commit_transaction();
		}else{
			user_abort_transaction();
		}
	}
	TESTTRUNCATE_CATCH_RETURN_ABTTRANS

	testtruncate_ExitFunc();

	return ret;
}

bool testtruncate_StopEngine_step2(void)
{
	testtruncate_EnterFunc();

	Oid heap_id;
	Oid idx_id[TESTTRUNCATE_MAX_IDX_COUNT];
	Relation rel;
	Relation idx_rel[TESTTRUNCATE_MAX_IDX_COUNT];
	bool ret = true;

	data_t data;
	data_t unique_data;
	IndexType idx_type[TESTTRUNCATE_MAX_IDX_COUNT];
	unsigned int idx_count = TESTTRUNCATE_MAX_IDX_COUNT;

	testtruncate_GenTestData(true, false, 2, data);
	testtruncate_GenTestData(true, true, 2, unique_data);

	/* set heap & indexes id */
	testtruncate_StopEngine_SetIds(heap_id, idx_count, idx_id);

	/* set colinfo */
	testtruncate_SetHeapColinfo(heap_id);
	for(unsigned int i = 0; i < idx_count; i++){
		if(idx_type[i] >= UINQUE_TYPE_BASE){
			unsigned int col_number[1] = {1};
			testtruncate_SetIndexColinfo(idx_id[i], 1, col_number);
		}else{
			unsigned int col_number[1] = {2};
			testtruncate_SetIndexColinfo(idx_id[i], 1, col_number);
		}
	}

	/* insert unique data */
	try
	{
		begin_transaction();

		/* open heap & indexes */
		testtruncate_OpenHeapAndIndexes(heap_id, idx_count, idx_id, rel, idx_rel);

		if(!testtruncate_CheckTruncateResult(rel, idx_rel, idx_count)){
			std::cerr << "testtruncate_CheckTruncateResult failed" << std::endl;
			ret = false;
			goto out1;
		}

		if(!testtruncate_InsertAndScanData(rel, idx_rel, idx_count, unique_data)){
			std::cerr << "testtruncate_CreateHeapAndIndexes failed" << std::endl;
			ret = false;
			goto out1;
		}

		FDPG_Heap::fd_heap_truncate(rel);
	out1:
		/* close heap & indexes */
		testtruncate_CloseHeapAndIndexes(rel, idx_count, idx_rel);

		if(ret){
			commit_transaction();
		}else{
			user_abort_transaction();
			return false;
		}
	}
	catch(StorageEngineExceptionUniversal &ex)
	{
		user_abort_transaction();
		testtruncate_DropHeapAndIndex(heap_id);
		return false;
	}

	/* insert data */
	try
	{
		begin_transaction();

		/* open heap & indexes */
		testtruncate_OpenHeapAndIndexes(heap_id, idx_count, idx_id, rel, idx_rel);

		if(!testtruncate_CheckTruncateResult(rel, idx_rel, idx_count)){
			std::cerr << "testtruncate_CheckTruncateResult failed" << std::endl;
			ret = false;
			goto out2;
		}

		if(!testtruncate_InsertAndScanData(rel, idx_rel, idx_count, data)){
			std::cerr << "testtruncate_OneIndex use repetitive data failed." << std::endl;
			std::cerr << "It should be throw exception when insert"
				<< " repetitive data into heap with unique index." << std::endl;
			ret = false;
			goto out2;
		}
	out2:
		/* close heap & indexes */
		testtruncate_CloseHeapAndIndexes(rel, idx_count, idx_rel);

		if(ret){
			commit_transaction();
		}else{
			user_abort_transaction();
			return false;
		}
	}
	catch(UniqueViolationException& ex){
		printf("%s\n", ex.getErrorMsg());
		if(ex.getErrorNo() == XdbBaseException::ERR_STORAGE_UNIQUE_VIOLATION){
			user_abort_transaction();
			testtruncate_DropHeapAndIndex(heap_id);
			return true;
		}else{
			user_abort_transaction();
			testtruncate_DropHeapAndIndex(heap_id);
			return false;
		}
	}

	testtruncate_ExitFunc();	

	return true;
}

