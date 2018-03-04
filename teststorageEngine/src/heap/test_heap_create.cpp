#include "boost/thread/thread.hpp"

#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "interface/FDPGAdapter.h"
#include "utils/util.h"

#include "heap/test_heap_create.h"
#include "test_fram.h"
#include "iostream"
#include "interface/StorageEngineExceptionUniversal.h"
using namespace std;
using namespace FounderXDB::StorageEngineNS;


bool test_heap_multcreate()
{
	INTENT("测试FDPG_Heap::fd_heap_create能否多次创建相同ID的heap。");
	Oid relid = rid;
	Oid table_space = DEFAULTTABLESPACE_OID;
	try
	{	
		begin_transaction();
		FDPG_Heap::fd_heap_create(table_space, relid);
		FDPG_Transaction::fd_CommandCounterIncrement();
		FDPG_Heap::fd_heap_drop(relid, MyDatabaseId);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) {
		printf("无法多次创建相同ID的heap报错:\n");
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		printf("测试成功!\n");
		user_abort_transaction();
		return true;
	}
	return true;

}

#define THREAD_NUM_10 50
#define THREAD_NUM_5 5
#define THREAD_NUM_3 3

extern int RELID;

void thread_create_heap(const int rel_id, BackendParameters *GET_PARAM(), int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	*sta = 0;
	try
	{
		create_heap(rel_id);
		*sta = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		*sta = -1;
	}
	proc_exit(0);
}

EXTERN_SIMPLE_FUNCTION

int test_thread_create_heap_000()
{
	INTENT("创建多个线程去创建相同id的表。"
		"该测试的正确结果是1个线程创建"
		"表成功，其他所有线程均失败。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	int sta[THREAD_NUM_10] = {0};
	setColInfo(RELID, NULL);
	for(int i = 0; i < THREAD_NUM_10; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_create_heap, RELID, GET_PARAM(), &sta[i]));
	}
	tg.join_all();

	int count = 0;
	for(int i = 0; i < ARRAY_LEN_CALC(sta); ++i)
	{
		if(sta[i] == 1)
		{
			++count;
		}
	}

	if(count != 1)
	{
		int drop_sta = 0;
		SIMPLE_DROP_HEAP(RELID, drop_sta);
		FREE_PARAM(BackendParameters *);
		return 0;
	}
	int drop_sta = 0;
	SIMPLE_DROP_HEAP(RELID, drop_sta);
	FREE_PARAM(BackendParameters *);
	return 1;
}
int test_thread_create_heap_001()
{
	INTENT("创建多个线程去创建不同id的表。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	int sta[THREAD_NUM_5];
	clear_heap_id();
	int id_array[THREAD_NUM_5];
	for(int i = 0; i < THREAD_NUM_5; ++i)
	{
		id_array[i] = get_heap_id();
	}
	for(int i = 0; i < THREAD_NUM_5; ++i)
	{
		BackendParameters *GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		setColInfo(id_array[i], NULL);
		tg.create_thread(bind(&thread_create_heap, id_array[i], GET_PARAM(), &sta[i]));
	}
	tg.join_all();

	int count = 0;
	for(int i = 0; i < ARRAY_LEN_CALC(sta); ++i)
	{
		if(sta[i] == 1)
		{
			++count;
		}
	}

	if(count != THREAD_NUM_5)
	{
		for(int i = 0; i < THREAD_NUM_5; ++i)
		{
			int id = get_remove_heap_id();
			int drop_sta = 0;
			SIMPLE_DROP_HEAP(id, drop_sta);
		}
		FREE_PARAM(BackendParameters *);
		return 0;
	}
	for(int i = 0; i < THREAD_NUM_5; ++i)
	{
		int id = get_remove_heap_id();
		int drop_sta = 0;
		SIMPLE_DROP_HEAP(id, drop_sta);
	}
	FREE_PARAM(BackendParameters *);
	return 1;
}

static unsigned int end = 0;
static unsigned int current = 0;

void end_when()
{
	while(current < end)
		pg_sleep(1000L);
}

void thread_create_some_heap(Oid * const heap_arr, int arr_len, BackendParameters *GET_PARAM(), bool *sta_arr)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	memset(sta_arr, 0, sizeof(bool) * arr_len);
 /* 这里所有的heap需要在同一个事务里创建 */
	begin_transaction();
	for(int i = 0; i < arr_len; ++i)
	{
		try
		{
			FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID, heap_arr[i]);
			sta_arr[i] = true;
		}catch(StorageEngineExceptionUniversal &se)
		{
			printf("%s\n", se.getErrorMsg());
			sta_arr[i] = false;
		}
	}
	commit_transaction();
	proc_exit(0);
}

static
void thread_insert_some_heap(Oid * const heap_arr, 
														 const int arr_len, 
														 const char *data,
														 const int data_len,
														 bool *insert_sta,
														 BackendParameters *GET_PARAM())
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	memset(insert_sta, 0, sizeof(bool) * arr_len);

	/* 这里所有的插入操作需要在一个事务里 */
	begin_transaction();
	for(int i = 0; i < arr_len; ++i)
	{
		try
		{
			Relation rel = FDPG_Heap::fd_heap_open(heap_arr[i], RowExclusiveLock, MyDatabaseId);
			HeapTuple tuple = fdxdb_heap_formtuple(data, data_len);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			pfree(tuple);
			insert_sta[i] = true;
		}catch(StorageEngineExceptionUniversal &se)
		{
			printf("%s\n", se.getErrorMsg());
			insert_sta[i] = false;
		}
	}
	++current;
	end_when();
	commit_transaction();
	proc_exit(0);
}

static 
void thread_delete_some_heap(Oid * const heap_arr, const int arr_len, BackendParameters *GET_PARAM())
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	for(int i = 0; i < arr_len; ++i)
	{
		begin_transaction();
		try
		{
			FDPG_Heap::fd_heap_drop(heap_arr[i], MyDatabaseId);
		}catch(StorageEngineExceptionUniversal &se)
		{
			printf("%s\n", se.getErrorMsg());
		}
		commit_transaction();
	}
	proc_exit(0);
}

int test_thread_create_heap_002()
{
	INTENT("启动若干个线程，每个线程至少创建1000个heap。"
				 "由于创建一个heap会打开一个文件句柄，在提交事务"
				 "之前该句柄会保留，直到超过安全的句柄数量。本测"
				 "试同时也测文件句柄的淘汰算法是否能正确执行。");

	using namespace boost;
	using namespace std;
	PREPARE_TEST();

#undef HEAP_NUM
#define HEAP_NUM 200

	const unsigned int THREAD_NUM_GO = 80;
	end = THREAD_NUM_GO;

	clear_heap_id();
	vector<Oid *> v_heap_arr;
	vector<bool *> v_sta, v_insert_sta;
	for(int i = 0; i < THREAD_NUM_GO; ++i)
	{
		Oid *heap_arr = new Oid[HEAP_NUM];
		bool *sta = new bool[HEAP_NUM];
		bool *insert_sta = new bool[HEAP_NUM];
		v_heap_arr.push_back(heap_arr);
		v_sta.push_back(sta);
		v_insert_sta.push_back(insert_sta);
		for(int j = 0; j < HEAP_NUM; ++j)
		{
			heap_arr[j] = get_heap_id();
		}
	}

	for(int i = 0; i < THREAD_NUM_GO; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(&thread_create_some_heap, v_heap_arr[i], HEAP_NUM, GET_PARAM(), v_sta[i]));
		GET_THREAD_GROUP().join_all();
	}

	for(int i = 0; i < THREAD_NUM_GO; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(&thread_insert_some_heap, v_heap_arr[i], 
																					HEAP_NUM, "test_data", strlen("test_data"), 
																					v_insert_sta[i], GET_PARAM()));
	}
	GET_THREAD_GROUP().join_all();
	
	bool return_sta = true;
	for(int i = 0; i < v_sta.size(); ++i)
	{
		for(int j = 0; j < HEAP_NUM; ++j)
		{
			if(v_sta[i][j] != true || v_insert_sta[i][j] != true)
			{
				return_sta = false;
				break;
			}
		}
		if(return_sta == false)
		{
			break;
		}
	}

	for(int i = 0; i < THREAD_NUM_GO; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(&thread_delete_some_heap, v_heap_arr[i], HEAP_NUM, GET_PARAM()));
	}
	GET_THREAD_GROUP().join_all();
	FREE_PARAM(BackendParameters *);

	for(int i = 0; i < THREAD_NUM_GO; ++i)
	{
		delete v_heap_arr[i];
		delete v_sta[i];
		delete v_insert_sta[i];
	}

	return return_sta;
}

static 
void insert_temp_data(const char insert_data[][DATA_LEN], 
								 const int array_len, 
								 const int data_len,
								 const int rel_id)
{
	using namespace FounderXDB::StorageEngineNS;
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
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	heap_close(rel, RowExclusiveLock);
}

/* 将表的数据全部更新成update_data,返回更新的条数 */
static
uint32 update_temp_data(Relation rel, const char *update_data)
{
	HeapScanDesc scan = NULL;
	HeapTuple tuple = NULL;
	uint32 count = 0;

	scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		HeapTuple tup = fdxdb_heap_formtuple(update_data, strlen(update_data) + 1);
		FDPG_Heap::fd_simple_heap_update(rel, &tuple->t_self, tup);
		pfree(tup);
		++count;
	}
	heap_endscan(scan);

	return count;
}

/* 检查是否表中所有的数据都是data */
static
bool check_all_data(Relation rel, const char *data)
{
	HeapScanDesc scan = NULL;
	HeapTuple tuple = NULL;
	bool retval = true;

	scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		char *cmpdata = fxdb_tuple_to_chars(tuple);
		if(strcmp(cmpdata, data) != 0)
		{
			retval = false;
		}
		pfree(cmpdata);
	}
	heap_endscan(scan);

	return retval;
}

/* 删除表中所有数据 */
static
void delete_tmp_data(Relation rel)
{
	HeapScanDesc scan = NULL;
	HeapTuple tuple = NULL;

	scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		FDPG_Heap::fd_simple_heap_delete(rel, &tuple->t_self);
	}
	heap_endscan(scan);
}

static 
void tmp_rel_split(RangeData& rd, const char *str, int col, size_t data_len)
{
	memset(&rd, 0, sizeof(rd));
	rd.start = 0;
	rd.len = 3;
}

int simple_test_temprel()
{
	INTENT("单线程测试临时表的创建、插入、更新和删除操作。");

	using namespace FounderXDB::StorageEngineNS;

	int retval = true;

	Oid tmp_relid1, tmp_relid2;
	Relation rel1, rel2;

	tmp_relid1 = tmp_relid2 = InvalidOid;
	rel1 = rel2 = NULL;

	const char DATA[][DATA_LEN] = 
	{
		"test_data1",
		"test_data2",
		"test_data3",
		"test_data4",
		"test_data5",
		"test_data6",
		"test_data7",
	};

	try
	{
		begin_transaction();

		begin_subtransaction();

		/* 创建临时表 */
		tmp_relid1 = FDPG_Heap::fd_temp_heap_create(DEFAULTTABLESPACE_OID, MyDatabaseId, 1, ONCOMMIT_DROP);
		tmp_relid2 = FDPG_Heap::fd_temp_heap_create(DEFAULTTABLESPACE_OID, MyDatabaseId, 1, ONCOMMIT_DROP);

		commit_subtransaction();

		begin_subtransaction();

		/* 创建临时表索引 */
		Relation rel1 = FDPG_Heap::fd_heap_open(tmp_relid1, ShareLock);
		Relation rel2 = FDPG_Heap::fd_heap_open(tmp_relid2, ShareLock);

		Oid tmp_index_id = FDPG_Index::fd_index_create(rel1, BTREE_TYPE, 0, 2);
		FDPG_Index::fd_index_create(rel2, BTREE_TYPE, 0, 2);
		Oid tmp_index_id2 = FDPG_Index::fd_index_create(rel1, BTREE_TYPE, 0, 2);
		Oid tmp_index_id3 = FDPG_Index::fd_index_create(rel2, BTREE_TYPE, 0, 2);

		FDPG_Heap::fd_heap_close(rel1, ShareLock);
		FDPG_Heap::fd_heap_close(rel2, ShareLock);

		FDPG_Transaction::fd_CommandCounterIncrement();

		//FDPG_Index::fd_index_drop(tmp_relid1, DEFAULTTABLESPACE_OID, tmp_index_id);
		//FDPG_Index::fd_index_drop(tmp_relid1, DEFAULTTABLESPACE_OID, tmp_index_id2);
		//FDPG_Index::fd_index_drop(tmp_relid2, DEFAULTTABLESPACE_OID, tmp_index_id3);

		/* 插入数据 */
		insert_temp_data(DATA, ARRAY_LEN_CALC(DATA), DATA_LEN, tmp_relid1);
		insert_temp_data(DATA, ARRAY_LEN_CALC(DATA), DATA_LEN, tmp_relid2);
		
		commit_subtransaction();

		begin_subtransaction();

		/* 检查插入的数据 */
		int *sta = new int[ARRAY_LEN_CALC(DATA) * 2];
		int *count = new int[ARRAY_LEN_CALC(DATA) * 2];
		
		rel1 = FDPG_Heap::fd_heap_open(tmp_relid1, AccessShareLock);
		rel2 = FDPG_Heap::fd_heap_open(tmp_relid2, AccessShareLock);

		for(int i = 0; i < ARRAY_LEN_CALC(DATA); ++i)
		{
			findTuple(DATA[i], rel1, sta[i], count[i], strlen(DATA[0]));
			findTuple(DATA[i], rel1, sta[i + ARRAY_LEN_CALC(DATA)], count[i + ARRAY_LEN_CALC(DATA)], strlen(DATA[0]));
		}

		for(int i = 0; i < ARRAY_LEN_CALC(DATA) * 2; ++i)
		{
			if(sta[i] != 1 || count[i] != 1)
			{
				retval = false;
				break;
			}
		}
		FDPG_Heap::fd_heap_close(rel1, AccessShareLock);
		FDPG_Heap::fd_heap_close(rel2, AccessShareLock);

		FDPG_Transaction::fd_CommandCounterIncrement();

		rel1 = FDPG_Heap::fd_heap_open(tmp_relid1, RowExclusiveLock);
		rel2 = FDPG_Heap::fd_heap_open(tmp_relid2, RowExclusiveLock);

		/* 更新表中所有数据 */
		uint32 count1 = update_temp_data(rel1, "update");
		uint32 count2 = update_temp_data(rel2, "update2");

		if(count1 != ARRAY_LEN_CALC(DATA) || count2 != ARRAY_LEN_CALC(DATA))
		{
			retval = false;
		}

		FDPG_Transaction::fd_CommandCounterIncrement();

		/* 删除表中所有数据 */
		delete_tmp_data(rel1);
		delete_tmp_data(rel2);

		FDPG_Transaction::fd_CommandCounterIncrement();

		retval = (retval ? check_all_data(rel1, "update") : false);
		retval = (retval ? check_all_data(rel2, "update2") : false);

		/* 检查表中数据，此时应该是没有数据了 */
		count1 = (retval ? update_temp_data(rel1, "") : -1);
		count2 = (retval ? update_temp_data(rel2, "") : -1);

		if(count1 != 0 || count2 != 0)
		{
			retval = false;
		}

		FDPG_Heap::fd_heap_close(rel1, RowExclusiveLock);
		FDPG_Heap::fd_heap_close(rel2, RowExclusiveLock);

		commit_subtransaction();
		
		/* 提交事务，自动删除临时表 */
		commit_transaction();
	} catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		retval = false;
	}
	
	/*尝试再去打开表，表不存在，应该是报异常*/
	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_open(tmp_relid1, AccessShareLock);
		retval = false;
	} catch(StorageEngineExceptionUniversal &se) {
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		retval = (retval ? true : false);
	}

	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_open(tmp_relid2, AccessShareLock);
		retval = false;
	} catch(StorageEngineExceptionUniversal &se) {
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		retval = (retval ? true : false);
	}

	return retval;
}

static
void thread_temprel_func(int *sta, BackendParameters *param)
{
	void* fxdb_SubPostmaster_Main(void*);
	fxdb_SubPostmaster_Main(param);

	*sta = simple_test_temprel();

	proc_exit(0);
}

static
void set_test_tmp_rel_colinfo()
{
	extern int my_compare_str(const char *, size_t, const char *, size_t);

	Colinfo heap_info = (Colinfo)palloc0(sizeof(ColinfoData));
	heap_info->keys = 1;
	heap_info->split_function = tmp_rel_split;

	Colinfo index_info = (Colinfo)palloc0(sizeof(ColinfoData));
	index_info->keys = 1;
	index_info->col_number = (size_t*)palloc0(1 * sizeof(size_t));
	index_info->col_number[0] = 1;
	index_info->rd_comfunction = (CompareCallback*)palloc0(sizeof(CompareCallback) * 1);
	index_info->rd_comfunction[0] = my_compare_str;
	index_info->split_function = tmp_rel_split;

	setColInfo(1, heap_info);
	setColInfo(2, index_info);
}

int thread_test_temprel()
{
	INTENT("测试多线程情况下临时表的多种操作是否能正确执行");

	using namespace boost;

	const int THREAD_NUM = 50;
	int retval = true;

	int *sta = new int[THREAD_NUM];

	PREPARE_TEST();

	set_test_tmp_rel_colinfo();

	for(int i = 0; i < THREAD_NUM; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(thread_temprel_func, &sta[i], GET_PARAM()));
	}
	GET_THREAD_GROUP().join_all();
	FREE_PARAM(BackendParameters *);

	for(int i = 0; i < THREAD_NUM; ++i)
	{
		if(sta[i] != true)
		{
			retval = false;
			break;
		}
	}
	delete []sta;

	return retval;
}

static
void thread_create_temprel(int *sta, const int thread_num, BackendParameters *GET_PARAM())
{
	void* fxdb_SubPostmaster_Main(void*);
	fxdb_SubPostmaster_Main(GET_PARAM());

	const int TEMP_MAX = 30000000;
	const int CREATE_NUM = TEMP_MAX / thread_num;
	uint32 sleep_time = 0;

	srand(time(NULL));

	DataGenerater dg(1, 3000);
	dg.dataGenerate();

	try
	{
		for(int i = 0; i < CREATE_NUM; ++i)
		{
			begin_transaction();

			Oid relid = FDPG_Heap::fd_temp_heap_create(DEFAULTTABLESPACE_OID, MyDatabaseId, 1, ONCOMMIT_DROP);

			/* 创建临时表索引 */
			Relation rel1 = FDPG_Heap::fd_heap_open(relid, ShareLock);

			FDPG_Index::fd_index_create(rel1, BTREE_TYPE, 0, 2);
			FDPG_Index::fd_index_create(rel1, BTREE_TYPE, 0, 2);

			FDPG_Heap::fd_heap_close(rel1, ShareLock);

			CommandCounterIncrement();

			Relation rel = FDPG_Heap::fd_heap_open(relid, RowExclusiveLock);

			HeapTuple tuple = fdxdb_heap_formtuple(dg[0], strlen(dg[0]));

			for(int j = 0; j < 100; ++j)
				FDPG_Heap::fd_simple_heap_insert(rel, tuple);

			/* 创建临时表索引 */
			FDPG_Index::fd_index_create(rel1, BTREE_TYPE, 0, 2);

			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);

			/* 事务等待若干时间再提交 */
			sleep_time = rand() % 1000000;
			pg_sleep(sleep_time + 1000000);

			commit_transaction();
		}
	} catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s", se.getErrorMsg());
		*sta = false;
	}

	*sta = true;

	proc_exit(0);
}

int thread_test_maxnum_create_tmprel()
{
	INTENT("多线程测试创建N(N>15000)张临时表。由于临时表"
				 "的ID分配在15000个以内，所以会出现ID循环使用的时候。");

	using namespace boost;

	int retval = true;

	const int THREAD_NUM = 20;

	PREPARE_TEST();

	set_test_tmp_rel_colinfo();

	int *sta = new int[THREAD_NUM];

	for(int i = 0; i < THREAD_NUM; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(thread_create_temprel, &sta[i], THREAD_NUM, GET_PARAM()));
	}
	GET_THREAD_GROUP().join_all();
	FREE_PARAM(BackendParameters *);

	for(int i = 0; i < THREAD_NUM; ++i)
	{
		if(sta[i] != true)
		{
			retval = false;
			break;
		}
	}
	delete []sta;

	return retval;	
}
