#ifdef _DEBUG
#include <iostream>
#endif
#include <fstream>

#include "boost/thread.hpp"
#include <algorithm>
#include <cassert>
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
#include <stdio.h>
#include <time.h> 
#include <cstdarg>
#include <vector>
#include <algorithm>
#include "interface/StorageEngineExceptionUniversal.h"

extern unsigned int data_len_calc(const char data[], unsigned int len);

extern int RELID;
extern int INDEX_ID;
BackendId BID = 1;

void exit_test_save_data(std::vector<std::string> &v_data, const char *filename)
{
	FILE *file = fopen(filename, "a+");
	for (int i = 0; i < v_data.size(); ++i)
	{
		int len = v_data[i].length() + 1;
		fwrite(&len, sizeof(len), 1, file);
		fwrite(v_data[i].c_str(), len, 1, file);
	}
	fclose(file);
}

void exit_test_save_data(void *data, int len, const char *filename)
{
	FILE *file = fopen(filename, "a+");
	fwrite(&len, sizeof(len), 1, file);
	fwrite(data, len, 1, file);
	fclose(file);
}

static std::map<std::string, unsigned int> file_pos;

void exit_test_read_data(const char *filename, std::vector<std::string> &v_data, int read_count)
{
	int len;
	FILE *file = fopen(filename, "r");
	fseek(file, file_pos[filename], SEEK_SET);
	for (int i = 0; i < read_count; ++i)
	{
		fread(&len, sizeof(len), 1, file);
		void *data = malloc(len);
		fread(data, len, 1, file);
		file_pos[filename] += (sizeof(int) + len);
		v_data.push_back((char*) data);
		free(data);
	}
	fclose(file);
}

void *exit_test_read_data(const char *filename)
{
	int len;
	FILE *file = fopen(filename, "r");
	fseek(file, file_pos[filename], SEEK_SET);
	fread(&len, sizeof(len), 1, file);
	void *data = malloc(len);
	fread(data, len, 1, file);
	fclose(file);
	file_pos[filename] += (sizeof(int) + len);
	return data;
}

int start_engine_()
{
	extern std::string g_strDataDir;
	storage_params * params = (storage_params *)malloc(sizeof(storage_params));	
    memset(params,0,sizeof(storage_params));	
		//master server
	/*params->XLogArchiveMode = true;
	 params->XLogArchiveCommand =  "copy \"%p\" \"C:\\archivedir\\%f\"";*/
	//params->masterEnableStandby = 1;
	
	//standby server
	//params->StandbyMode = true;
	//params->slaveEnableHotStandby = true;
	//params->recoveryRestoreCommand = "copy  \"C:\\archive_hot\\%f\" \"%p\"";
	//params->PrimaryConnInfo="host=localhost port=5432 user=foo password=foopass";
		params->startVac = true;
	FDPG_StorageEngine::fd_start_engine(g_strDataDir.c_str(), 80, false, params);
	free(params);
	return 1;
}

int start_thread_engine_()
{
	return start_engine_();
}

int stop_engine_()
{
	stop_engine(0);
	return 1;
}

int exit_proc()
{
	exit(0);
}

void begin_first_transaction()
{	
	FDPG_Transaction::fd_AbortOutOfAnyTransaction();
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_BeginTransactionBlock();
	FDPG_Transaction::fd_CommitTransactionCommand();
}

void begin_subtransaction( void )
{
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_BeginSubTransaction("");
	//FDPG_Transaction::fd_CommandCounterIncrement();
	//FDPG_Transaction::fd_CommitTransactionCommand();
}

void commit_subtransaction( void )
{
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_CommitSubTransaction();
	FDPG_Transaction::fd_CommandCounterIncrement();
	//FDPG_Transaction::fd_CommitTransactionCommand();
}

void abort_subtransaction( void )
{
	FDPG_Transaction::fd_StartTransactionCommand();
	FDPG_Transaction::fd_AbortSubTransaction();
	FDPG_Transaction::fd_CommandCounterIncrement();
	//FDPG_Transaction::fd_CommitTransactionCommand();
}

void unlink_heap(void *relid)
{
	using namespace FounderXDB::StorageEngineNS;

	try
	{
		begin_transaction();
		Oid *rel_id = static_cast<Oid *>(relid);
		FDPG_Heap::fd_heap_drop(*rel_id);
		commit_transaction();
		delete rel_id;
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
}

static void thread_drop_heap2(Oid rel_id)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	BackendParameters *param = (BackendParameters*)malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);

	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_drop(rel_id);
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
	free(param);

	proc_exit(0);
}

EXTERN_SIMPLE_FUNCTION

void thread_unlink_heap(void *relid)
{
	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	try
	{
		Oid *rel_id = static_cast<Oid *>(relid);
		int drop_sta = 0;
		param = get_param();
		SAVE_PARAM(param);
		tg.create_thread(bind(&thread_drop_heap2, *rel_id));
		tg.join_all();
		FREE_PARAM(BackendParameters *);
		delete rel_id;
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
}

BackendParameters *
get_param(BackendId bid, ThreadType type)
{
	extern void fxdb_save_thread_variables(BackendParameters *, BackendId, ThreadType);
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, bid, backend);
	++BID;
	return param;
}
extern void setColInfo(Oid colid, Colinfo pcol_info);
void create_heap(const int rel_id, const Colinfo heap_info)
{
	using namespace FounderXDB::StorageEngineNS;

	try
	{

		begin_transaction();
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_heap_create(relspace, rel_id, 0,rel_id);
		//Relation r = RelationIdGetRelation(rel_id);

		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
}

void create_index(int heap_id, Colinfo heap_colinfo, Colinfo ind_colinfo, int ind_id, bool unique)
{
	using namespace FounderXDB::StorageEngineNS;

	try
	{
		setColInfo(ind_id,ind_colinfo);
		begin_transaction();
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		Relation heap_relation = FDPG_Heap::fd_heap_open(heap_id,RowExclusiveLock, MyDatabaseId);

        if(unique)
        {
		    FDPG_Index::fd_index_create(heap_relation, BTREE_UNIQUE_TYPE,ind_id,ind_id);
        }
        else
        {
            FDPG_Index::fd_index_create(heap_relation,BTREE_TYPE,ind_id,ind_id);
        }
		FDPG_Heap::fd_heap_close(heap_relation, RowExclusiveLock);
		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
	catch(UniqueViolationException &ex)
	{
		printf("%s\n", ex.getErrorMsg());
		user_abort_transaction();
		throw UniqueViolationException(ex);
	}
}

void remove_heap(int &rel_id)
{
	using namespace FounderXDB::StorageEngineNS;

	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_drop(rel_id, MyDatabaseId);
		++rel_id;
		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
}

void remove_heap(Oid rel_id, bool create_in_thread)
{
	using namespace FounderXDB::StorageEngineNS;

	try
	{
		Oid *relid = new Oid(rel_id);
		if(!create_in_thread)
		{
			on_test_exit_at_fornt(unlink_heap, (void *)relid);
		} else {
			on_test_exit_at_fornt(thread_unlink_heap, (void *)relid);
		}
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
}

void remove_index(const int heap_id, int &ind_id)
{
	using namespace FounderXDB::StorageEngineNS;

	try
	{
		//begin_transaction();
		//FDPG_Index::fd_index_drop(heap_id, MyDatabaseTableSpace, ind_id);
		++ind_id;
		//commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
}

void init_scan_key(IndexScanInfo &isi)
{
	for(int i = 0; i < isi.ncol; ++i)
	{
		Fdxdb_ScanKeyInitWithCallbackInfo
		(
			&isi.scan_key[i], 
			isi.col_array[i], 
			isi.cmp_strategy[i], 
			isi.cmp_func[i], 
			isi.cmp_values[i]
		);
	}
}


void alloc_scan_space(const unsigned int size, IndexScanInfo &isi)
{
	isi.ncol = size;
	isi.cmp_func = (CompareCallback*)malloc(size * sizeof(CompareCallback));
	isi.cmp_strategy = (StrategyNumber*)malloc(size * sizeof(StrategyNumber));
	isi.cmp_values = (Datum*)malloc(size * sizeof(Datum));
	isi.col_array = (unsigned int*)malloc(size * sizeof(unsigned int));
	isi.scan_key = (ScanKeyData*)malloc(size * sizeof(ScanKeyData));
}


void free_scan_key(IndexScanInfo &isi)
{
	free(isi.cmp_func);
	free(isi.cmp_strategy);
	free(isi.cmp_values);
	free(isi.col_array);
	free(isi.scan_key);
}

void insert_data(const char insert_data[][DATA_LEN], 
								 const int array_len, 
								 const int data_len,
								 const int rel_id)
{
	using namespace FounderXDB::StorageEngineNS;
	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
//	CHECK_BOOL(rel != NULL);
	HeapTuple tuple = NULL;
	try
	{
		for(int i = 0; i < array_len; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(insert_data[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			CommandCounterIncrement();
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		heap_close(rel, RowExclusiveLock);
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
}

void insert_data(const std::vector<std::string> &insert_data,
								 const int rel_id,
								 std::vector<ItemPointerData> *v_it)
{
	using namespace FounderXDB::StorageEngineNS;
	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);

	HeapTuple tuple = NULL;
	try
	{
		for(int i = 0; i < insert_data.size(); ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(insert_data[i].c_str(), insert_data[i].length() + 1);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			if (v_it)
				v_it->push_back(tuple->t_self);
			pfree(tuple);
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	heap_close(rel, NoLock);
	commit_transaction();
}

void delete_data_(const std::vector<ItemPointerData> &d_data,
								 const int rel_id)
{
	using namespace FounderXDB::StorageEngineNS;
	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);

	HeapTuple tuple = NULL;
	try
	{
		for (int i = 0; i < d_data.size(); ++i)
		{
			ItemPointerData it = d_data[i];
			FDPG_Heap::fd_simple_heap_delete(rel, &it);
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	heap_close(rel, NoLock);
	commit_transaction();
}

void index_scan_hs(    const Oid index_id, 
					   const Colinfo ind_info, 
					   const Oid heap_id, 
					   const Colinfo heap_info,
					   const IndexScanInfo *isi, 
					   DataGenerater *dg,
					   unsigned int *array_len)
{
	SAVE_INFO_FOR_DEBUG();
	using namespace FounderXDB::StorageEngineNS;
	try
	{
		begin_transaction();
		Relation heap = FDPG_Heap::fd_heap_open(heap_id,AccessShareLock, MyDatabaseId);
		Relation index = FDPG_Index::fd_index_open(index_id,AccessShareLock, MyDatabaseId);

		IndexScanDesc isd = FDPG_Index::fd_index_beginscan(heap, index, SnapshotNow, isi->ncol, 0);
		FDPG_Index::fd_index_rescan(isd, isi->scan_key, isi->ncol, NULL, 0);
		HeapTuple tuple = NULL;
		unsigned int count = 0;
		while((tuple = FDPG_Index::fd_index_getnext(isd, ForwardScanDirection)) != NULL)
		{
			std::pair<char*, int> pci = tuple_get_string_and_len(tuple);
			dg->dataGenerate(pci.first, pci.second, count++);
			++(*array_len);
			pfree(pci.first);
		}
		FDPG_Index::fd_index_endscan(isd);
		FDPG_Index::fd_index_close(index, AccessShareLock);
		FDPG_Heap::fd_heap_close(heap, AccessShareLock);
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
	}
}

void insert_with_index(const char data[][DATA_LEN], 
							  const int data_len,
							  const int array_len,  
							  const HeapIndexRelation *hir, 
							  IndexUniqueCheck iuc)
{
	using namespace FounderXDB::StorageEngineNS;

	SAVE_INFO_FOR_DEBUG();
	using namespace FounderXDB::StorageEngineNS;
	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = FDPG_Heap::fd_heap_open(hir->heap_id, RowExclusiveLock, MyDatabaseId);
	HeapTuple tuple = NULL;
	try
	{
		for(int i = 0; i < array_len; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(data[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			//for(int j = 0; j < hir->index_array_len; ++j)
			//{
			//	Relation indrel = FDPG_Index::fd_index_open(BTREE_TYPE, relspace, hir->index_id[j], RowExclusiveLock, hir->index_info[j]);
			//	Datum values[1];
			//	bool isnull[1];
			//	isnull[0] = false;
			//	values[0] = fdxdb_form_index_datum(rel, indrel, tuple);
			//	index_insert(indrel, values, isnull, &(tuple->t_self), rel, iuc);
			//	FDPG_Index::fd_index_close(indrel, RowExclusiveLock);
			//}
			CommandCounterIncrement();
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		heap_close(rel, RowExclusiveLock);
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());

		return;
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
}

void form_index(const char insert_data[][DATA_LEN], 
								const int array_len, 
								const int data_len,
								const int rel_id,
								const int index_id,
								Colinfo heap_info,
								Colinfo ind_info,
								IndexUniqueCheck iuc)
{
	using namespace FounderXDB::StorageEngineNS;
	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
	Relation ind_rel = FDPG_Index::fd_index_open(index_id,RowExclusiveLock, MyDatabaseId);
	//	CHECK_BOOL(rel != NULL);
	HeapTuple tuple = NULL;
	try
	{
		for(int i = 0; i < array_len; ++i)
		{
			tuple = fdxdb_heap_formtuple(insert_data[i], data_len);
			Datum values[1];
			bool isnull[1];
			isnull[0] = false;
			values[0] = fdxdb_form_index_datum(rel, ind_rel, tuple);
			index_insert(ind_rel, values, isnull, &(tuple->t_self), rel, iuc);
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	FDPG_Index::fd_index_close(ind_rel, RowExclusiveLock);
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
}

void insert_large_data(char *pdataArr[], 
				 const int array_len, 
				 const int data_len,
				 const int rel_id)
{
	using namespace FounderXDB::StorageEngineNS;
	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
//	CHECK_BOOL(rel != NULL);
	HeapTuple tuple = NULL;
	try
	{
		for(int i = 0; i < array_len; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(pdataArr[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			pfree(tuple);
			CommandCounterIncrement();
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
}

void calc_tuple_num(Oid relid, int &count)
{
	PG_TRY();
	{
		count = 0;
		begin_transaction();
		Relation rel = heap_open(relid, RowExclusiveLock);
		HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, 0, 0);
		HeapTuple tuple = NULL;
		while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			char *c = fxdb_tuple_to_chars(tuple);
//			printf("%s\n", c);
			++count;
		}
		heap_endscan(scan);
		heap_close(rel, RowExclusiveLock);
		commit_transaction();
	}
	PG_CATCH();
	{
		count = -1;
	}
	PG_END_TRY();
}

//查找元组
ItemPointerData findTuple(const char *data, Relation rel, int &sta, int &count, int cmp_len)
{
	if(cmp_len == 0)
	{
		cmp_len = strlen(data) + 1;
	}
	ItemPointerData ipd;
	memset(&ipd, 0, sizeof(ItemPointerData));
	//assert(NULL != rel);
	sta = -1;
	count = 0;
	char *tmp_data;
	HeapTuple tuple;
	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		tmp_data = fxdb_tuple_to_chars(tuple);
		if(memcmp(data, tmp_data, cmp_len) == 0)
		{
			ipd = tuple->t_self;
			if(ipd.ip_posid==0)
			{
			    int i=0;
            }
                        ++count;
                        sta = 1;
        }
        pfree(tmp_data);
    }
    heap_endscan(scan);
    return ipd;
}

//检查数据是否存在于集合里(如果有两个内容相同的元组，则理应视为两个不同的元组)
static bool check_exists(const char *det_data, 
                         const int cmp_len, 
                         const char data[][DATA_LEN], 
                         const int array_len, 
                         bool *flag)
{
    for(int i = 0; i < array_len; ++i)
    {
        if(flag[i] == false && memcmp(det_data, data[i], cmp_len) == 0)
        {
            flag[i] = true;
            return true;
        }
    }
    return false;
}

//检查表中所有数据是否完整、正确
bool check_equal_all(const int rel_id, const char data[][DATA_LEN], const int array_len)
{
    using namespace FounderXDB::StorageEngineNS;

    try
    {
        begin_transaction();
        Relation rel = FDPG_Heap::fd_heap_open(rel_id,AccessShareLock, MyDatabaseId);
        HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
        HeapTuple tuple = NULL;
        bool *flag = new bool[array_len];
        memset(flag, 0, array_len);
        bool _sta = true;
        int count = 0;
        while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
        {
            //表中数据过多
            if(count > array_len)
            {
                _sta = false;
            }
            char *_data = fxdb_tuple_to_chars(tuple);
            check_exists(_data, strlen(_data), data, array_len, flag);
            ++count;
        }
        FDPG_Heap::fd_heap_endscan(scan);
        FDPG_Heap::fd_heap_close(rel, AccessShareLock);
        commit_transaction();
        if(_sta)
        {
            //检测所有位都应该被设置
            for(int i = 0; i < array_len; ++i)
            {
                if(flag[i] != true)
                {
                    _sta = false;
                    break;
                }
            }
        }

        return _sta;
    }catch(StorageEngineExceptionUniversal &se)
    {
        printf("%s\n", se.getErrorMsg());
        user_abort_transaction();
        return false;
    }
}

//检查两个数组中的数据是否完全相等
bool check_array_equal(const char array1[][DATA_LEN], 
                       const int array1_len, 
                       const char array2[][DATA_LEN],
                       const int array2_len)
{
    if(array1_len != array2_len)
    {
        return false;
    }
    std::vector<string> v1(0);
    std::vector<string> v2(0);
    for(int i = 0; i < array1_len; ++i)
    {
        v1.push_back(array1[i]);
        v2.push_back(array2[i]);
    }

    sort(v1.begin(), v1.end());
    sort(v2.begin(), v2.end());
    return v1 == v2;
}

bool check_all_in_hash_tbl(const int *relid, const int len)
{
    using namespace FounderXDB::StorageEngineNS;

    int _return = true;
    try
    {
        begin_transaction();

        for(int i = 0; i < len; ++i)
        {
            if(_return == false)
            {
                break;
            }
            LockRelationOid(relid[i], AccessShareLock);
            Relation rel = RelationIdGetRelation(relid[i]);
            if(rel == NULL)
            {
                SAVE_INFO_FOR_DEBUG();
                _return = false;
            }
            UnlockRelationOid(relid[i], AccessShareLock);
            FDPG_Heap::fd_heap_close(rel, AccessShareLock);
        }

        commit_transaction();
    }catch(StorageEngineExceptionUniversal &se)
    {
        printf("%s\n", se.getErrorMsg());
        user_abort_transaction();
    }
    return _return;
}

bool check_all_in_array(char src[][DATA_LEN], 
                        char det[][DATA_LEN],
                        unsigned int src_array_len,
                        unsigned int det_array_len, 
                        unsigned int data_len)
{
    assert(data_len <= DATA_LEN);
    bool result = true;
    for(int i = 0; i < det_array_len; ++i)
    {
        int sta = false;
        for(int j = 0; j < src_array_len; ++j)
        {
            if(memcmp(src[j], det[i], data_len) == 0)
            {
                sta = true;
                break;
            }
        }
        if(sta == false)
        {
            result = false;
            break;
        }
    }
    return result;
}

std::pair<char*, int> tuple_get_string_and_len(HeapTuple tuple)
{
    extern TupleDesc single_attibute_tupDesc;
    Datum * values = (Datum *) palloc(sizeof(Datum));
    bool * isnull = (bool *) palloc(sizeof(bool));
    char *chars;
    heap_deform_tuple(tuple, single_attibute_tupDesc, values, isnull);
    chars=TextDatumGetCString(values[0]);
    int len = VARSIZE_ANY_EXHDR(DatumGetCString(values[0]));
    pfree(isnull);
    return std::pair<char*, int>(chars, len);
}



static
const char* get_sub_str(const char *data,
                        const unsigned int col_num,
                        const unsigned int *col_array)
{
    int tmp_sum = 0;
    for(int i = 0; i < col_num - 1; ++i)
    {
        tmp_sum += col_array[i];
    }
    return &data[tmp_sum];
}

static
bool is_satisfy(const int sta, const StrategyNumber sn)
{
    if(sn == BTEqualStrategyNumber)
    {
        return sta == 0 ? true : false;
    }
    if(sn == BTLessEqualStrategyNumber)
    {
        return sta <= 0 ? true : false;
    }
    if(sn == BTLessStrategyNumber)
    {
        return sta < 0 ? true : false;
    }
    if(sn == BTGreaterEqualStrategyNumber)
    {
        return sta >= 0 ? true : false;
    }
    if(sn == BTGreaterStrategyNumber)
    {
        return sta > 0 ? true : false;
    }
}

static
void sub_vector(std::vector<std::string> &v_tmp,
                const unsigned int *col_array,
                const unsigned int col,
                const char *cmp_data,
                const StrategyNumber sn,
                std::vector<std::string> &vc_det)
{
    std::vector<std::string>::iterator begin = v_tmp.begin();
    while(begin != v_tmp.end())
    {
        const char *tmp = get_sub_str(begin->c_str(), col, col_array);
        std::string cmp_str(tmp);
        int sta = cmp_str.compare(0, col_array[col-1], cmp_data);
        if(is_satisfy(sta, sn))
        {
            vc_det.push_back(begin->c_str());
        }
        ++begin;
    }
}

//检查索引应该扫描到的数据，用于后续索引扫描的比较
void get_result_in_array(char src[][DATA_LEN],
                         const unsigned int src_array_len,
                         const CheckResultInfo &cri,
                         char det[][DATA_LEN],
                         unsigned int &return_len)
{
    //初始化vector并且按升序排序
    std::vector<std::string> v_tmp(0);
    for(int i = 0; i < src_array_len; ++i)
    {
        v_tmp.push_back(src[i]);
    }
    std::vector<std::string> vc_det;

    for(int i = 0; i < cri.ncol; ++i)
    {
        if(vc_det.size() > 0)
        {
            v_tmp.clear();
            v_tmp = vc_det;
            vc_det.clear();
        }
        sub_vector(v_tmp, cri.col_array, cri.col_num[i], cri.cmp_data[i], cri.cmp_stn[i], vc_det);
        if(vc_det.size() == 0)
        {
            break;
        }
    }
    for(int i = 0; i < vc_det.size(); ++i)
    {
        memcpy(det[i], vc_det[i].c_str(), vc_det[i].length());
        ++return_len;
    }
}


void commit_transaction()
{
    using namespace FounderXDB::StorageEngineNS;
    try
    {
        FDPG_Transaction::fd_StartTransactionCommand();
        FDPG_Transaction::fd_EndTransactionBlock();
        FDPG_Transaction::fd_CommitTransactionCommand();
    }catch(StorageEngineExceptionUniversal &se)
    {
        printf("%s\n", se.getErrorMsg());
        printf("%s\n", se.getErrorNo());
    }
}

void user_abort_transaction()
{
    FDPG_Transaction::fd_StartTransactionCommand();
    FDPG_Transaction::fd_UserAbortTransactionBlock();
    FDPG_Transaction::fd_CommitTransactionCommand();
} 

void begin_transaction()
{
    FDPG_Transaction::fd_StartTransactionCommand();
    FDPG_Transaction::fd_BeginTransactionBlock();
    FDPG_Transaction::fd_CommitTransactionCommand();
}

std::vector<int> SpliterGenerater::m_vc_col_array;

void heap_split_to_any(RangeData& rd, const char *str, int col, size_t data_len)
{
    while(NULL != str && data_len > 0)
    {
        int len = SpliterGenerater::m_vc_col_array.size();
        for(int i = 1; i <= len; ++i)
        {
            if(col == i)
            {
                int sum = 0;
                for(int j = 0; j < i - 1; ++j)
                {
                    sum += SpliterGenerater::m_vc_col_array[j];
                }
                rd.start = sum;
                rd.len = SpliterGenerater::m_vc_col_array[i - 1];
                break;
            }
        }
        break;
    }
}

std::vector<int> HeapFuncTest::m_heap_col_array;
std::vector<int> IndexFuncTest::m_index_col_array;

void heap_split(RangeData &rd, const char *str, int col, size_t data_len)
{
    while(NULL != str && data_len > 0)
    {
        int num = HeapFuncTest::m_heap_col_array.size();
        for(int i = 1; i <= num; ++i)
        {
            if(col == i)
            {
                int sum = 0;
                for(int j = 0; j < i - 1; ++j)
                {
                    sum += HeapFuncTest::m_heap_col_array[j];
                }
                rd.start = sum;
                rd.len = HeapFuncTest::m_heap_col_array[i - 1];
                break;
            }
        }
        break;
    }
    return;
}

void index_split(RangeData &rd, const char *str, int col, size_t data_len)
{
	rd.start = 0;
	rd.len = 0;
    while (NULL != str && data_len > 0)
    {
        int num = IndexFuncTest::m_index_col_array.size();
        for(int i = 1; i <= num; ++i)
        {
            if(col == i )
            {
                for(int j = 0; j < i - 1; ++j)
                {
					rd.start += HeapFuncTest::m_heap_col_array[j];
                }
                rd.len = HeapFuncTest::m_heap_col_array[i - 1];
                break;
            }
        }
        break;
    }
    return;
}

//template<typename T>
//VectorMgr<T>::VectorMgr()
//{
//
//}
//
//template<typename T>
//VectorMgr<T>::~VectorMgr()
//{
//	for(int i = 0; i < v_t.size(); ++i)
//	{
//		delete (v_t[i]);
//	}
//}
//
//template<typename T>
//void VectorMgr<T>::vPush(T t)
//{
//	v_t.push_back(t);
//}
//
//template<typename T>
//std::vector<T>& VectorMgr<T>::getVMgr()
//{
//	return v_t;
//}

SpliterGenerater::SpliterGenerater()
{
    m_pheap_cf = NULL;
    m_pindex_cf = NULL;
}

SpliterGenerater::~SpliterGenerater()
{
	if(m_pheap_cf && m_pindex_cf)
	{
		free(m_pheap_cf);
		m_pheap_cf = NULL;
	}
	if(m_pindex_cf)
	{
		free(m_pindex_cf->col_number);
		free(m_pindex_cf->rd_comfunction);
		free(m_pindex_cf);
		m_pindex_cf = NULL;
	}
	SpliterGenerater::m_vc_col_array.clear();
}

Colinfo SpliterGenerater::buildHeapColInfo(const int col_count, ...)
{
    m_pheap_cf = (Colinfo)malloc(sizeof(ColinfoData));
    m_pheap_cf->col_number = 0;
    m_pheap_cf->keys = 0;
    m_pheap_cf->rd_comfunction = NULL;

    va_list arg_ptr;
    va_start(arg_ptr, col_count);
    for(int i = 0; i < col_count; ++i)
    {
        SpliterGenerater::m_vc_col_array.push_back(va_arg(arg_ptr, int));
    }

    m_pheap_cf->split_function = heap_split_to_any;
    return m_pheap_cf;
}

Colinfo SpliterGenerater::buildIndexColInfo(const int col_count, 
                                            const int *col_number, 
                                            const CompareCallback *cmp_func_array,
                                            const Split split_func)
{
    m_pindex_cf = (Colinfo)malloc(sizeof(ColinfoData));
    m_pindex_cf->keys = col_count;
    m_pindex_cf->col_number = (size_t*)malloc(sizeof(size_t) * col_count);
    for(int i = 0; i < col_count; ++i)
    {
        m_pindex_cf->col_number[i] = col_number[i];
    }
    m_pindex_cf->rd_comfunction = (CompareCallback*)malloc(sizeof(CompareCallback) * col_count);
    for(int i = 0; i < col_count; ++i)
    {
        m_pindex_cf->rd_comfunction[i] = cmp_func_array[i];
    }
    m_pindex_cf->split_function = split_func;
    return m_pindex_cf;
}

std::vector<int> FixSpliter::g_vecOfSplitPos;
FixSpliter::FixSpliter(const std::vector<int> &vec)
{
    g_vecOfSplitPos.clear();
    for (std::vector<int>::const_iterator it = vec.begin();
        it != vec.end();
        ++it)
    {
        g_vecOfSplitPos.push_back(*it + (g_vecOfSplitPos.size() > 0 ? g_vecOfSplitPos.back() : 0));
    }
}

void FixSpliter::split(RangeData& rangeData, const char *pszNeedSplit,int iIndexOfColumn, size_t len)
{
    pszNeedSplit;
    assert(iIndexOfColumn >= 1 && iIndexOfColumn <= (int)g_vecOfSplitPos.size());

    if (iIndexOfColumn > 1)
    {
        rangeData.start = g_vecOfSplitPos[iIndexOfColumn - 2];
        rangeData.len = g_vecOfSplitPos[iIndexOfColumn - 1] -  g_vecOfSplitPos[iIndexOfColumn - 2];
    }
    else
    {
        rangeData.len = g_vecOfSplitPos[iIndexOfColumn - 1];
    }
}

char VarSpliter::g_cSep = '\000';
void VarSpliter::split(RangeData& rangeData, char *pszNeedSplit,int iIndexOfColumn)
{
    assert(NULL != pszNeedSplit);

    const char* pszStart = pszNeedSplit;
    const char* pszEnd = pszStart + strlen(pszNeedSplit);
    for (int iTimes = 0; iTimes < iIndexOfColumn; ++iTimes)
    {
        pszStart = std::find(pszStart,pszEnd,g_cSep);
        if (pszEnd == pszStart)
        {
            return ;
        }
        ++pszStart;
    }
    pszEnd = std::find(pszStart,pszEnd,g_cSep);

    rangeData.len = pszEnd - pszStart;
}


SimpleHeap::SimpleHeap(Oid idRelTableSpace,Oid idRelation,Split spliter,bool needCreate,bool needDrop):
m_idReltableSpace(idRelTableSpace)
,m_idReleation(idRelation)
,m_lockMode(RowExclusiveLock)
,m_bNeedDrop(needDrop)
{
	if(needCreate)
	{
		static ColinfoData pColInfo;
		pColInfo.col_number = 0;
		pColInfo.col_number = NULL;
		pColInfo.rd_comfunction = NULL;
		pColInfo.split_function =  spliter;
		setColInfo(m_idReleation,&pColInfo);
		FDPG_Heap::fd_heap_create(m_idReltableSpace,m_idReleation,MyDatabaseId,m_idReleation);
		CommandCounterIncrement();
	}
}

void SimpleHeap::Open(LOCKMODE lockMode)
{
    m_lockMode = lockMode;
    m_relation = FDPG_Heap::fd_heap_open(m_idReleation,lockMode, MyDatabaseId);
}

void SimpleHeap::Insert(const std::string& pszInserted)
{
    assert(NULL != m_relation);
    HeapTuple tup = FDPG_Heap::fd_heap_form_tuple(pszInserted.c_str(),pszInserted.length());
    FDPG_Heap::fd_simple_heap_insert(m_relation,tup);

    //for (std::vector<SimpleIndex*>::iterator it = m_vecIndexs.begin();
    //	it != m_vecIndexs.end();
    //	++it)
    //{
    //	(*it)->Insert(tup);
    //}
}

void SimpleHeap::Delete(const std::string& pszDeleted)
{
    ItemPointer itemPtr = Find(pszDeleted);
    assert(NULL != itemPtr);
    FDPG_Heap::fd_simple_heap_delete(m_relation,itemPtr);
}

ItemPointer SimpleHeap::Find(const string& pszValue)
{
    ItemPointer itemPointer = NULL;
    HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(m_relation, SnapshotNow, 0, NULL);

    bool alareadFind = false;
    HeapTuple tuple;
    while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
    {     
        int len;
        char *data_tuple= fdxdb_tuple_to_chars_with_len(tuple,len);
        if ( (len == pszValue.length()) &&
            0 == strncmp(data_tuple, pszValue.c_str(),pszValue.length())) 
        {
            if(!alareadFind)
            {
                itemPointer = (ItemPointer)palloc(sizeof(ItemPointerData));
                itemPointer->ip_blkid.bi_hi = tuple->t_self.ip_blkid.bi_hi;
                itemPointer->ip_blkid.bi_lo = tuple->t_self.ip_blkid.bi_lo;
                itemPointer->ip_posid = tuple->t_self.ip_posid;
                alareadFind = true;
            }
            else
            {
                FDPG_Heap::fd_heap_endscan(scan);
                throw std::runtime_error("find failed! tuple should different each other.");
            }
        }
        pfree(data_tuple);
    }
    FDPG_Heap::fd_heap_endscan(scan);

    CommandCounterIncrement();

    return itemPointer;
}

std::set<std::string>& SimpleHeap::GetAll(void)
{
    m_setData.clear();
    ItemPointer itemPointer = NULL;
    HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(m_relation, SnapshotNow, 0, NULL);

    bool alareadFind = false;
    HeapTuple tuple;
    while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
    {     
        int len;
        char *data_tuple= fdxdb_tuple_to_chars_with_len(tuple,len);
        m_setData.insert(std::string((char*)(data_tuple),len));
        pfree(data_tuple);
    }
    FDPG_Heap::fd_heap_endscan(scan);

    return m_setData;

}

void SimpleHeap::Update( const std::string& szOldValue,const std::string& szNewValue) 
{
    ItemPointer otid = Find(szOldValue);
    if(NULL == otid)
    {
#ifdef _DEBUG
        std::cout<<"can not find tuple: "<<szOldValue<<std::endl;
#endif
        return ;
    }

    //update the heaptuple 
    HeapTuple tup_update = FDPG_Heap::fd_heap_form_tuple(szNewValue.c_str(),szNewValue.length());
    FDPG_Heap::fd_simple_heap_update(m_relation, otid, tup_update);
    CommandCounterIncrement();

    //for (std::vector<SimpleIndex*>::iterator it = m_vecIndexs.begin();
    //	it != m_vecIndexs.end();
    //	++it)
    //{
    //	(*it)->Insert(tup_update);
    //}
    pfree(otid);
}

SimpleHeap::~SimpleHeap()
{
    if(NULL != m_relation)
    {
        FDPG_Heap::fd_heap_close(m_relation, m_lockMode);
        m_relation = NULL;
    }

    if(m_bNeedDrop)
    {
			FDPG_Heap::fd_heap_drop(m_idReleation, MyDatabaseId);
    }
}

static std::vector<Oid> vc_heap_id;
static std::vector<Oid> vc_index_id;

Oid get_heap_id()
{
    static Oid heap_id = 410000;
    vc_heap_id.push_back(heap_id);
    return heap_id++;
}

Oid get_index_id()
{
    static Oid index_id = 400000;
    vc_index_id.push_back(index_id);
    return index_id++;
}

Oid get_remove_heap_id()
{
    std::vector<Oid>::iterator it = vc_heap_id.begin();
    Oid tmp_heap_id = *it;
    vc_heap_id.erase(it);
    return tmp_heap_id;
}

Oid get_remove_index_id()
{
    std::vector<Oid>::iterator it = vc_index_id.begin();
    Oid tmp_index_id = *it;
    vc_index_id.erase(it);
    return tmp_index_id;
}

void clear_heap_id()
{
    vc_heap_id.clear();
}

void clear_index_id()
{
    vc_index_id.clear();
}

void clear_all()
{
    vc_index_id.clear();
    vc_heap_id.clear();
}

SimpleIndex::SimpleIndex(SimpleHeap& heap,
                         Oid idIndex,
                         const std::map<int,CompareCallback> &vecKeys,
                         bool bCreate,
                         bool needDrop):
r_heap(heap)
,m_indexRelation(NULL)
,m_idIndex(idIndex)
,m_pCollInfo(NULL)
,m_bNeedDrop(needDrop)
{
	m_pCollInfo = (Colinfo)malloc(sizeof(ColinfoData));
	m_pCollInfo->keys = vecKeys.size();
	m_pCollInfo->split_function = FixSpliter::split;
	m_pCollInfo->col_number = (size_t*)malloc(m_pCollInfo->keys * sizeof(size_t));
	m_pCollInfo->rd_comfunction = (CompareCallback*)malloc(m_pCollInfo->keys * sizeof(CompareCallback));

    setColInfo(idIndex,m_pCollInfo);
    int iPos = 0;
    for (std::map<int,CompareCallback>::const_iterator it = vecKeys.begin();
        it != vecKeys.end();
        ++it)
    {
        m_pCollInfo->col_number[iPos] = it->first;
        m_pCollInfo->rd_comfunction[iPos++] = it->second;
    }

    if(bCreate)
    {
        FDPG_Index::fd_index_create(r_heap.Get(),BTREE_TYPE,idIndex,idIndex);
        CommandCounterIncrement();
    }
    r_heap.m_vecIndexs.push_back(this);
}

std::set<string>&  SimpleIndex::Find(SearchCondition& scanCondition)
{
    m_setResult.clear();
    m_indexRelation = FDPG_Index::fd_index_open(m_idIndex,AccessShareLock, MyDatabaseId);

    IndexScanDesc index_scan = FDPG_Index::fd_index_beginscan(r_heap.Get(), m_indexRelation, GetTransactionSnapshot(),scanCondition.Count(), 0);
    FDPG_Index::fd_index_rescan(index_scan, scanCondition.Keys(), scanCondition.Count(), NULL, 0);

    HeapTuple tup;
    while((tup = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
    {
        int len;
        char *ptup = fdxdb_tuple_to_chars_with_len(tup,len);
        m_setResult.insert(string(ptup,len));
        pfree(ptup);
    }

    FDPG_Index::fd_index_endscan(index_scan);
    FDPG_Index::fd_index_close(m_indexRelation, AccessShareLock);

    return m_setResult;
}

std::vector<string>& SimpleIndex::Find(class SearchCondition& scanCondition,int hold)
{
    m_vResult.clear();
    m_indexRelation = FDPG_Index::fd_index_open(m_idIndex,AccessShareLock, MyDatabaseId);

    HeapTuple ttup;
    SysScanDesc toastscan = systable_beginscan_ordered(r_heap.Get(), m_indexRelation,
        SnapshotToast, scanCondition.Count(), scanCondition.Keys());
    while ((ttup = systable_getnext_ordered(toastscan, ForwardScanDirection)) != NULL)
    {
        int len;
        char *ptup = fdxdb_tuple_to_chars_with_len(ttup,len);
        m_vResult.push_back(string(ptup,len));
        pfree(ptup);
    }
    systable_endscan_ordered(toastscan);
    FDPG_Index::fd_index_close(m_indexRelation, AccessShareLock);

    return m_vResult;
}

void SimpleIndex::Insert(HeapTuple updatedTuple)
{
    Datum		values[1];
    bool		isnull[1];
    isnull[0] = false;
    m_indexRelation = FDPG_Index::fd_index_open(m_idIndex,AccessShareLock, MyDatabaseId);
    values[0] = fdxdb_form_index_datum(r_heap.Get(), m_indexRelation, updatedTuple);

    FDPG_Index::fd_index_insert(m_indexRelation, /* index relation */
        values,	/* array of index Datums */
        isnull,	/* null flags */
        &(updatedTuple->t_self),		/* tid of heap tuple */
        r_heap.Get(),	/* heap relation */
        UNIQUE_CHECK_NO);	/* type of uniqueness check to do */
    FDPG_Index::fd_index_close(m_indexRelation, AccessShareLock);
    CommandCounterIncrement();
}

SimpleIndex::~SimpleIndex()
{
    if(m_bNeedDrop)
    {
        m_indexRelation = NULL;
        //FDPG_Index::fd_index_drop(r_heap.GetRelationId(), r_heap.GetRelTableSpaceId(),m_idIndex); 
				FreeColiInfo(m_pCollInfo);
    }
}

SearchCondition::SearchCondition():
m_nCount(0)
{

}

SearchCondition::~SearchCondition()
{

}

void SearchCondition::Add(AttrNumber nColumn,EScanOp op,const char* pszValue,CompareCallback compare)
{
    m_values[m_nCount] = fdxdb_string_formdatum(pszValue,strlen(pszValue));
    Fdxdb_ScanKeyInitWithCallbackInfo(&m_keys[m_nCount],
        nColumn,
        (StrategyNumber)op, compare,
        m_values[m_nCount]);
    ++m_nCount;
}

OIDGenerator& OIDGenerator::instance()
{
    static OIDGenerator g;
    return g;
}

Oid OIDGenerator::GetTableSpaceID( void )
{
    return DEFAULTTABLESPACE_OID;
}

Oid OIDGenerator::GetHeapID( void )
{
    static Oid id = (1 << 27) + 1;
    id += 2;
    return id;
}

Oid OIDGenerator::GetIndexID( void )
{
    static Oid id = (1 << 27) + 2;
    id += 2;
    return id;
}


int str_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
    int i = 0;
    while(i < len1 && i < len2)
    {
        if(str1[i] < str2[i])
            return -1;
        else if(str1[i] > str2[i])
            return 1;
        else i++;

    }
    if(len1 == len2)
        return 0;
    if(len1 > len2)
        return 1;
    return -1;
}

void FreeColiInfo(Colinfo pInfo)
{
	if (pInfo->col_number)
		free(pInfo->col_number);
	if (pInfo->rd_comfunction)
		free(pInfo->rd_comfunction);
	free(pInfo);
}

void regist_free_colinfo(void *col_info)
{
	Colinfo pInfo = (Colinfo)col_info;
	if (pInfo->col_number)
	{
		free(pInfo->col_number);
		pInfo->col_number = NULL;
	}
	if (pInfo->rd_comfunction)
	{
		free(pInfo->rd_comfunction);
		pInfo->rd_comfunction = NULL;
	}
	free(pInfo);
	col_info = NULL;
}


char *make_data(unsigned int size)
{
    char *p = (char *)malloc(size);
    for(unsigned int i = 0; i < size-1; i++)
    {
        p[i] = (char)(rand() % 95 + 32);
        //		p[i] = (char)(rand() % 26 + 65);
    }
    p[size-1] = '\0';
    return p;
}

char *make_unique_data(unsigned int size,unsigned int row)
{
    extern char *my_itoa(int value, char *string, int radix);
    char string[20];
    my_itoa(row,string,10);
    char *p = (char *)malloc(size+strlen(string)+1);
    for(unsigned int i = 0; i < size; i++)
    {
        p[i] = (char)(rand() % 95 + 32);
        //		p[i] = (char)(rand() % 26 + 65);
    }
    p[size] = '\0';
    strcat(p,string);
    return p;
}

DataGenerater::DataGenerater(const unsigned int row_num, const unsigned int data_len = DATA_LEN)
{
    int i = 0;
    //如果两次调用构造函数的时间间隔很短，该
    //循环可以避免产生相同的时间
    while(i++ < 1000 * 1000);
    srand((unsigned int)time(NULL));
    this->m_row_num = row_num;
    this->m_data_len = data_len;
    this->m_size = row_num;
    m_rand_data = NULL;
    m_rand_data = (char*)malloc(row_num * data_len);
	memset(m_rand_data, 0, row_num * data_len);
}

DataGenerater::DataGenerater(const DataGenerater& dg)
{
    m_row_num = dg.m_row_num;
    m_size = dg.m_size;
    m_data_len = dg.m_data_len;
    m_rand_data = (char*)malloc(m_row_num * m_data_len);
    memcpy(m_rand_data, dg.m_rand_data, m_row_num * m_data_len);
}

DataGenerater::~DataGenerater()
{
    free(m_rand_data);
    m_rand_data = NULL;
}

void DataGenerater::dataGenerate()
{	
	char *p = (char *)malloc(m_data_len);
    for(unsigned int j = 0; j < this->m_row_num; ++j)
    {
		memset(p, 0, m_data_len);
        for(unsigned int i = 0; i < m_data_len - 1; i++)
        {
            p[i] = (char)(rand() % 95 + 32);
        }
        p[m_data_len - 1] = '\0';
        memcpy(m_rand_data + j * m_data_len, p, m_data_len);
    }
	free(p);
}

void DataGenerater::dataGenerate(const char *src_data, const int data_len, int pos)
{
    if(m_size == m_row_num)
    {
        m_size = 0;
    }
    if(pos >= m_row_num)
    {
        space_increment();
    }
    memcpy(m_rand_data + pos * m_data_len, src_data, data_len);
    ++m_size;
}

void DataGenerater::dataToDataArray2D(char data[][DATA_LEN])
{
    m_row_num = m_size;
	memset(data, 0, m_row_num * DATA_LEN);
    for(unsigned int i = 0; i < m_row_num; ++i)
    {
        memcpy(data[i], m_rand_data + i * m_data_len, m_data_len);
    }
}

void DataGenerater::dataToDataArray2D(const int size, char data[][DATA_LEN])
{
    for(unsigned int i = 0; i < size; ++i)
    {
        memcpy(data[i], m_rand_data + i * m_data_len, m_data_len);
    }
}

void DataGenerater::space_increment()
{
    char *tmp_data = NULL;
    tmp_data = (char*)malloc(m_row_num * m_data_len);
    memcpy(tmp_data, m_rand_data, m_row_num * m_data_len);
    free(m_rand_data);
    m_rand_data = (char*)malloc((m_row_num + 10) * m_data_len);
    memcpy(m_rand_data, tmp_data, m_row_num * m_data_len);
    m_row_num += 10;
    free(tmp_data);
}

bool check_array_equal(char src[][DATA_LEN], 
                       char det[][DATA_LEN],
                       unsigned int array_len,
                       unsigned int array_row)//比较两个二维数组是否相等，
                       //array_len为数组每行的长度，或数组的列数
                       //array_row为数组的行数
{
    bool result = TRUE;
    for(int i = 0; i < array_row; i++)
    {
        if(memcmp(src[i], det[i],array_len) != 0)
        {
            result = FALSE;
            break;
        }
    }
    return result;
}

void OutPutToastErrorInter(std::set<std::string>& sDesired,std::set<std::string>& sResult,const char* pFile,size_t nLen)
{
    std::fstream outFile("toast_error.txt",std::ios_base::out | std::ios_base::app);
    if(!outFile)
    {
        std::cout<<"Can not open toast_error.txt for write!"<<std::endl;
    }
    outFile.write(pFile,strlen(pFile));
    outFile<<": "<<nLen<<std::endl;

    std::cout.write(pFile,strlen(pFile));
    std::cout<<": "<<nLen<<std::endl;

    typedef std::set<std::string> ResultT;
    ResultT::iterator itDesired = sDesired.begin();
    for (ResultT::iterator itResult = sResult.begin();itResult != sResult.end();++itResult,++itDesired)
    {
        bool bStore2File = false;
        std::cout<<"["<<itDesired->length()<<","<<itResult->length()<<"]";
        size_t nMinLen = itDesired->length() < itResult->length() ? itDesired->length() : itResult->length() ;
        for (size_t nPos = 0; nPos < nMinLen;++nPos)
        {
            if (itDesired->at(nPos) != itResult->at(nPos))
            {
                bStore2File = true;
                std::cout<<"("<<nPos<<","<<itDesired->at(nPos)<<","<<itResult->at(nPos)<<")";
            }
        }
        std::cout<<std::endl;

        if (bStore2File)
        {
            outFile<<"<<<<<<<<<<<Desired Result:"<<std::endl;
            outFile.write(itDesired->c_str(),itDesired->length());
            outFile<<std::endl<<">>>>>>>>>>Real Result: "<<std::endl;
            outFile.write(itResult->c_str(),itResult->length());
		}
	}
}

extern int my_compare_select(const char *str1, size_t len1, const char *str2, size_t len2);

bool HeapFuncTest::resultCheck( char *scanData, struct ScanKeyInfo *scanKey, const int nkeys )
{
	assert( scanData != NULL );
	assert( scanKey != NULL );

	int i = 0, j = 0;
	std::vector<int> keyAd( nkeys, 0 );
	for ( ; i < nkeys; ++i )
	{
		for ( j = 0; j < ( scanKey + i )->colid - 1; ++j )
		{
			keyAd[i] = keyAd[i] + m_heap_col_array[( scanKey + j )->colid - 1];
		}
	}
	for ( i = 0; i < nkeys; ++i )
	{
		if ( ((scanKey + i)->cmpStrategy < BTEqualStrategyNumber) 
			&& ( memcmp((scanKey + i)->argument, scanData + keyAd[i], m_heap_col_array[(scanKey + i)->colid - 1]) >= 0 )
			&& ( *( scanData + keyAd[i] ) != 0 ))
		{
			continue;
		}
		else if ( ((scanKey + i)->cmpStrategy > BTEqualStrategyNumber) 
			&& ( memcmp((scanKey + i)->argument, scanData + keyAd[i], m_heap_col_array[(scanKey + i)->colid - 1]) <= 0 ) )
		{
			continue;
		}
		else if ( ((scanKey + i)->cmpStrategy == BTEqualStrategyNumber)
			&& memcmp((scanKey + i)->argument, scanData + keyAd[i], m_heap_col_array[(scanKey + i)->colid - 1]) == 0 )
		{
			continue;
		}
		return false;
	}
	return true;
}

int HeapFuncTest::getDigitLength( int digit )
{
	int sum = 0;
	if ( digit == 0 )
	{
		return 1;
	}
	while( digit )
	{
		digit = digit / 10;
		sum++;
	}
	return sum;
}

void pointerFree(void *pointer)
{
	if( pointer )
	{
		free( pointer );
		pointer = NULL;
	}
}

using namespace FounderXDB::StorageEngineNS;

HeapFuncTest::HeapFuncTest():m_nRelid(++RID), m_bSuccess(false)
{
	m_pHeap = NULL;
	m_pRelation = NULL;
	m_pKey = NULL;
}

HeapFuncTest::~HeapFuncTest()
{
	pointerFree( m_pHeap );
	pointerFree( m_pKey );
	HeapFuncTest::m_heap_col_array.clear();
}

void HeapFuncTest::buildHeapColInfo()
{
	m_pHeap = (Colinfo)malloc(sizeof(ColinfoData));
	if ( m_pHeap == NULL )
	{
		return;
	}
	m_pHeap->col_number = 0;
	m_pHeap->keys = 0;
	m_pHeap->rd_comfunction = NULL;
	m_pHeap->split_function = NULL;

	createHeap( m_nRelid, m_pHeap );
}

void HeapFuncTest::buildHeapColInfo( int colnum, const int length[] )
{
	m_pHeap = (Colinfo)malloc(sizeof(ColinfoData));
	if ( m_pHeap == NULL )
	{
		return;
	}
	m_pHeap->col_number = 0;
	m_pHeap->keys = 0;
	m_pHeap->rd_comfunction = NULL;
	m_pHeap->split_function = heap_split;

	HeapFuncTest::m_heap_col_array.clear();
	for (int i = 0; i < colnum; ++i)
    {
		HeapFuncTest::m_heap_col_array.push_back( length[i] );
    }

	createHeap( m_nRelid, m_pHeap );
}

void HeapFuncTest::createHeap(const int rel_id, const Colinfo heap_info)
{
	try
	{
		begin_first_transaction();
		Oid relspace = MyDatabaseTableSpace;
		setColInfo(rel_id,heap_info);
		FDPG_Heap::fd_heap_create(relspace, rel_id, 0, rel_id);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
	}	
}

void HeapFuncTest::openHeap( LOCKMODE lockMode )
{
	m_pRelation = FDPG_Heap::fd_heap_open( m_nRelid, lockMode, MyDatabaseId );
}

void HeapFuncTest::insertRangeTuples( const int begin, const int end, char header[], const int headerLen )
{
	int tailLen = 0;
	tailLen = getDigitLength(end) + 1;
	char *tailString = new char[tailLen];
	char *dataBuff = new char[headerLen + tailLen];
	memcpy( dataBuff, header, headerLen );
	memset( tailString, 0, tailLen );
	HeapTuple ptuple = NULL;
	//int radix = 10;
	
	for ( int i = begin; i < end; ++i )
	{
		//itoa( i, tailString, radix );
		sprintf( tailString, "%d", i );
		memcpy( &dataBuff[headerLen - 1], tailString, tailLen );
		ptuple = FDPG_Heap::fd_heap_form_tuple( dataBuff, headerLen + tailLen );
		FDPG_Heap::fd_simple_heap_insert( m_pRelation, ptuple );
		//printf("插入数据为：	%s\n", dataBuff);
		memset( &dataBuff[headerLen - 1], 0, tailLen );
	}
	FDPG_Transaction::fd_CommandCounterIncrement();
	
	delete []tailString;
	delete []dataBuff;
}

void HeapFuncTest::insertTuples( char (*data)[20], const int amount )
{
	HeapTuple pTuple = NULL;
	
	for ( int i = 0; i < amount; ++i )
	{
		pTuple = FDPG_Heap::fd_heap_form_tuple( data[i], sizeof(data[i]) );
		FDPG_Heap::fd_simple_heap_insert( m_pRelation, pTuple );
		//printf("插入数据为：	%s\n", data[i]);
	}
	FDPG_Transaction::fd_CommandCounterIncrement();
	pfree( pTuple );
	pTuple = NULL;
}

void HeapFuncTest::insert( char data[], const int dataLen )
{
	HeapTuple pTuple = NULL;
	pTuple = FDPG_Heap::fd_heap_form_tuple( data, dataLen );
	FDPG_Heap::fd_simple_heap_insert( m_pRelation, pTuple );

	pfree( pTuple );
	pTuple = NULL;
}

void HeapFuncTest::scanRangeTuplesWithHeader( const int amount, char header[], const int headerLen )
{	
	int tailLen = 0;
	tailLen = getDigitLength(amount);
	char *tailString = new char[tailLen];
	char *dataBuff = new char[headerLen + tailLen];
	memcpy( dataBuff, header, headerLen);
	memset( tailString, 0, tailLen );
	//int radix = 10;
	HeapScanDesc scan;
	scan = FDPG_Heap::fd_heap_beginscan( m_pRelation, SnapshotNow, 0, NULL);
	HeapTuple pTuple = NULL;
	char *tempTuple = NULL;
	int notMatch = 1;
	int scanCounter = 0;

	//printf("\n*********下边为查询结果*********\n\n");
	while ( (pTuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )
	{
		//itoa( scanCounter++, tailString, radix );
		sprintf( tailString, "%d", scanCounter++ );
		memcpy( &dataBuff[headerLen - 1], tailString, tailLen );
		tempTuple = fxdb_tuple_to_chars( pTuple );
		notMatch = memcmp( tempTuple, dataBuff, headerLen + tailLen - 1 );
		memset( &dataBuff[headerLen - 1], 0, tailLen );

		if ( notMatch )
		{
			printf( "Data inconsist！\n" );
			FDPG_Heap::fd_heap_endscan( scan );
			return;
		}
		/*else
		{
			printf( "查询数据为：	%s\t\t行数：%d\n", 
				tempTuple, 
				scanCounter);
		}*/
	}

	if ( scanCounter != amount )
	{
		printf( "Rows inconsist！\n" );
		FDPG_Heap::fd_heap_endscan( scan );
		return;
	}

	delete []tailString;
	delete []dataBuff;
 	FDPG_Heap::fd_heap_endscan( scan );
	
	m_bSuccess = true;
}

void HeapFuncTest::scanTupleInserted( const int amount, char data[], const int dataLen )
{
	char *tempTuple = NULL;
	int scanCounter = 0;
	HeapScanDesc scan;
	scan = FDPG_Heap::fd_heap_beginscan( m_pRelation, SnapshotNow, 0, NULL);

	HeapTuple pTuple = NULL;
	//printf("\n*********下边为查询结果*********\n\n");
	while ( (pTuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )
	{
		tempTuple = fxdb_tuple_to_chars( pTuple );
		++scanCounter;
		/*printf( "查询数据为：	%s\t\t行数：%d\n", 
				tempTuple, 
				scanCounter );*/
		if( memcmp( tempTuple, data, dataLen ) )
		{
			printf("\nData inconsist！\n");
			FDPG_Heap::fd_heap_endscan( scan );
			return;
		}
	}
	if ( scanCounter != amount )
	{
		printf( "Rows inconsist！\n" );
		FDPG_Heap::fd_heap_endscan( scan );
		return;
	}	
	FDPG_Heap::fd_heap_endscan( scan );

	m_bSuccess = true;
}

void HeapFuncTest::deleteTupleById( ItemPointer itemPointer )
{
	FDPG_Heap::fd_simple_heap_delete( m_pRelation, itemPointer );
	FDPG_Transaction::fd_CommandCounterIncrement();
}

ItemPointer HeapFuncTest::GetOffsetByNo( const int Num )
{
	ItemPointer itemPointer = (ItemPointer)malloc( sizeof(ItemPointer) );
	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan( m_pRelation, SnapshotNow, 0, NULL );
	HeapTuple pTuple = NULL;
	for ( int i = 0; i < Num; ++i )
	{
		pTuple = FDPG_Heap::fd_heap_getnext( scan, ForwardScanDirection );
	}
	itemPointer->ip_blkid.bi_hi = pTuple->t_self.ip_blkid.bi_hi;
	itemPointer->ip_blkid.bi_lo = pTuple->t_self.ip_blkid.bi_lo;
	itemPointer->ip_posid = pTuple->t_self.ip_posid;
	FDPG_Heap::fd_heap_endscan( scan );
	return itemPointer;
}

void HeapFuncTest::scanTuplesInserted( char (*data)[20], const int amount, const int dataLen )
{
	char *tempTuple;
	int scanCounter = 0;
	HeapScanDesc scan;
	scan = FDPG_Heap::fd_heap_beginscan( m_pRelation, SnapshotNow, 0, NULL);

	HeapTuple pTuple = NULL;
	//printf("\n*********下边为查询结果*********\n\n");
	while ( (pTuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )
	{
		tempTuple = fxdb_tuple_to_chars( pTuple );
		if ( memcmp( tempTuple, data[scanCounter], dataLen ) )
		{
			printf("\n插入数据与查询数据不一致！\n");
			FDPG_Heap::fd_heap_endscan( scan );
			return;
		}
		/*for ( int i = 0; i < dataLen; ++i )
		{
			printf( "%c", data[scanCounter][i] );
		}
		printf( "\t\t第 %d 行\n", scanCounter );*/
		++scanCounter;
	}
	if ( scanCounter != amount )
	{
		printf( "插入数据行数与返回行数不相等！\n" );
		FDPG_Heap::fd_heap_endscan( scan );
		return;
	}
	
	FDPG_Heap::fd_heap_endscan( scan );
	m_bSuccess = true;
}

void HeapFuncTest::scanWithKey( struct ScanKeyInfo *scanKey, int nkeys )
{
	assert( m_pKey != NULL );
	assert( scanKey != NULL );

	HeapScanDesc scan;
	scan = FDPG_Heap::fd_heap_beginscan( m_pRelation, SnapshotNow, nkeys, m_pKey );

	//printf("\n*****************查询结果如下*****************\n\n");
	HeapTuple pTuple = NULL;
	char *tempTuple = NULL;
	int resultCnt = 0;
	while ( (pTuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )
	{
		tempTuple = fxdb_tuple_to_chars( pTuple );
		if( resultCheck( tempTuple, scanKey, nkeys ) == false )
		{
			printf( "\n查询中止！\n中止原因： %s 与查询条件不一致！\n", tempTuple );
			FDPG_Heap::fd_heap_endscan( scan );
			return;
		}
		//printf("%s\n", tempTuple);
		++resultCnt;
	}

	if( tempTuple != NULL )
	{
		pfree( tempTuple );
		tempTuple = NULL;
	}
	//printf("\n查询结果：共找到 %d 个符合条件的数据\n", resultCnt);
	FDPG_Heap::fd_heap_endscan( scan );
	
	m_bSuccess = true;
}

void HeapFuncTest::scanDeleteTuple( char *data, int dataLen, int amount )
{
	HeapTuple pTuple = NULL;
	HeapScanDesc scan = NULL;
	char *tempTuple = NULL;
	int tupleCnt = 0;

	scan = FDPG_Heap::fd_heap_beginscan( m_pRelation, SnapshotNow, 0, NULL );
	while ( (pTuple = FDPG_Heap::fd_heap_getnext( scan, ForwardScanDirection )) != NULL )
	{
		++tupleCnt;
		tempTuple = fxdb_tuple_to_chars( pTuple );
		//printf("%s\t行数: %d\n", tempTuple, pTuple->t_self.ip_posid);
		if ( memcmp( tempTuple, data, dataLen ) == 0 )
		{
			printf("tuple exist, delete faild\n");
			FDPG_Heap::fd_heap_endscan(scan);
			return;
		}
	}
	if ( tupleCnt + 1 != amount )
	{
		printf("result incorrect！\n");
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(m_pRelation, RowExclusiveLock);
		return;
	}

	FDPG_Heap::fd_heap_endscan(scan);
	m_bSuccess = true;
}

void HeapFuncTest::dropHeap()
{
	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_close( m_pRelation, RowExclusiveLock );
		FDPG_Heap::fd_heap_drop( m_nRelid, MyDatabaseId );
		commit_transaction();
	}	
	catch (StorageEngineExceptionUniversal &ex)
	{
		user_abort_transaction();
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
	}
}

void HeapFuncTest::constructKeys(ScanKeyInfo *scanKey, int nkeys)
{
	Datum *values = (Datum *) palloc( sizeof(Datum) );
	
	//ScanKeyData *key = new ScanKeyData[nkeys];
	m_pKey = new ScanKeyData[nkeys];
	for ( int i = 0; i < nkeys; ++i )
	{
		values[0] = fdxdb_string_formdatum( scanKey[i].argument, m_heap_col_array[scanKey[i].colid - 1] );
		Fdxdb_ScanKeyInitWithCallbackInfo(&m_pKey[i], scanKey[i].colid, scanKey[i].cmpStrategy, my_compare_select, values[0]);	
	}
	
	pfree(values);
	values = NULL;
}

IndexFuncTest::IndexFuncTest() 
{
	m_pIndex = NULL;
	m_nIndid = ++INDEX_ID;
	m_bSuccess = false;
}

IndexFuncTest::~IndexFuncTest()
{
}

void IndexFuncTest::createIndex( int indid, Colinfo indCol )
{
	setColInfo( indid, indCol );
	FDPG_Index::fd_index_create( m_pRelation, BTREE_TYPE, indid, indid );
}

void IndexFuncTest::buildIndexColInfo( int colnum, const int length[], ScanKeyInfo *scanKey)
{
	m_pIndex = (Colinfo)palloc(sizeof(ColinfoData));
	if ( m_pIndex == NULL )
	{
		return;
	}

	m_pIndex->keys = colnum;
	m_pIndex->col_number = ( size_t *)palloc( sizeof(size_t) * colnum );
	m_pIndex->rd_comfunction = (CompareCallback *)palloc( sizeof(CompareCallback) * colnum );
	for ( int i = 0; i < colnum; ++i )
	{
		m_pIndex->col_number[i] = scanKey[i].colid;
		m_pIndex->rd_comfunction[i] = my_compare_select;
		m_index_col_array.push_back( length[scanKey[i].colid - 1] );
	}
	m_pIndex->split_function = index_split;
	
	createIndex( m_nIndid, m_pIndex );
}

void IndexFuncTest::scanIndex( ScanKeyInfo *pKeys, const int nkeys )
{
	//open index
	Relation indexRelation = NULL;
	indexRelation = FDPG_Index::fd_index_open( m_nIndid, AccessShareLock, MyDatabaseId );
	IndexScanDesc index_scan = NULL;

	//begin scan
	index_scan = FDPG_Index::fd_index_beginscan( m_pRelation, indexRelation, SnapshotNow, nkeys, 0 );
	FDPG_Index::fd_index_rescan(index_scan, m_pKey, nkeys, NULL, 0);

	int resultCnt = 0;
	HeapTuple tuple = NULL;
	char *tempTuple = NULL;
	while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
	{
		tempTuple = fxdb_tuple_to_chars(tuple);
		//printf("%s\n",tempTuple);
		if( resultCheck( tempTuple, pKeys, nkeys ) == false )
		{
			printf( "\n查询中止！\n中止原因： %s 与查询条件不一致！\n", tempTuple );
			FDPG_Index::fd_index_endscan( index_scan );
			FDPG_Index::fd_index_close( indexRelation, AccessShareLock );
			return;
		}
		++resultCnt;
	}
	//printf("查询结果: 共计 %d 个\n", resultCnt);

	//end and close index
	FDPG_Index::fd_index_endscan( index_scan );
	FDPG_Index::fd_index_close( indexRelation, AccessShareLock );
	m_bSuccess = true;
}