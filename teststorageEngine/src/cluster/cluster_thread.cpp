#include <boost/thread/mutex.hpp> 
#include <boost/assign.hpp>
#include <boost/thread/thread.hpp>
#include <vector>
#include <string>
#include "test_fram.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/utils.h"
#include "utils/tqual.h"
#include "postgres.h"
#include "utils/util.h"

using namespace FounderXDB::StorageEngineNS;
using namespace std;
using namespace boost;

extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);
extern void *fxdb_SubPostmaster_Main(void *params);
extern void get_all_tuple_in_heap(Oid relid, vector<string> &v_data);
extern void thread_random_do(const int minutes, map<Oid, vector<string> > *rel_info, 
														 Oid *heap_arr, Oid *ind_arr, int heap_arr_len, int ind_arr_len, 
														 BackendParameters *params);

EXTERN_SIMPLE_FUNCTION

/* 该函数用于对单个索引扫描并返回结果，仅限字符串类型的数据 */
void index_get_result(Oid relid, Oid idxid, int colcount, 
											int *column, int *strategynumber,
											vector<string> &v_key, vector<string> &v_data)
{
	ScanKey scankey = (ScanKey) malloc (sizeof(ScanKeyData) * colcount);
	Datum *values = (Datum *) malloc (sizeof(Datum) * colcount);

	for (int i = 0; i < colcount; ++i)
	{
		values[i] = FDPG_Common::fd_string_formdatum(v_key[i].c_str(), v_key[i].length());
		Fdxdb_ScanKeyInitWithCallbackInfo(&scankey[i], column[i],
			strategynumber[i], my_compare_str, values[i]);
	}

	Relation rel = FDPG_Heap::fd_heap_open(relid, AccessShareLock);
	Relation idxrel = FDPG_Index::fd_index_open(idxid, AccessShareLock);

	IndexScanDesc scan = FDPG_Index::fd_index_beginscan(rel, idxrel, SnapshotNow, colcount, NULL);
	FDPG_Index::fd_index_rescan(scan, scankey, colcount, NULL, 0);

	HeapTuple tuple = NULL;
	while((tuple = FDPG_Index::fd_index_getnext(scan, ForwardScanDirection)) != NULL)
	{
		char *d = FDPG_Common::fd_tuple_to_chars(tuple);
		v_data.push_back(d);
		pfree(d);
	}

	FDPG_Index::fd_index_endscan(scan);
	FDPG_Heap::fd_heap_close(rel, NoLock);
	FDPG_Index::fd_index_close(idxrel, NoLock);

	free(scankey);
	free(values);
}

static
void thread_cluster_one_rel(BackendParameters *GET_PARAM(), Oid relid, 
														Oid indexid, shared_mutex *ready, bool *sta)
{
	*sta = true;
	fxdb_SubPostmaster_Main(GET_PARAM());

	try
	{
		ready->lock_shared();
		ready->unlock_shared();
		begin_transaction();
		FDPG_Heap::fd_ClusterRel(relid, indexid, false);
		commit_transaction();
	} catch (StorageEngineException &se)
	{
		printf("%s\n", se.getErrorMsg());
		*sta = false;
		user_abort_transaction();
	}

	proc_exit(0);
}

/* 
 * 模拟索引扫描表，使用vector集合和关键字返回vector中的子集
 * 该函数返回的是小于或者等于关键字的集合
 * 一个vector传进来的时候在外头需要使用sort函数排好序
 * 该函数仅适用于字符串类型的数据
 */
void vector_index_get_result(vector<string> &v_data, 
														 int *start_pos,
														 vector<string> &key, 
														 vector<string> &result)
{
	vector<string>::iterator it = v_data.begin();
	while(it != v_data.end())
	{
		bool is_equal = true;
		for (int i = 0; i < key.size(); ++i)
		{
			string k  = key[i];
			/* 从v_data[i]中截取出索引列来比较 */
			if (it->substr(start_pos[i], key[i].length()).compare(key[i]) > 0)
			{
				is_equal = false;
				break;
			}
		}
		if (is_equal)
			result.push_back(*it);

		++it;
	}
}

/* 用于更新和删除数据后重新构造表中数据的vector */
void reset_data_update(vector<string> *v_data, vector<pair<string, string> > *v_update_data)
{
	for(int i = 0; i < v_update_data->size(); ++i)
	{
		string key = v_update_data->operator [](i).first;
		vector<string>::iterator it;
		it = find(v_data->begin(), v_data->end(), key);
		if (it == v_data->end())
		{
			delete v_data;
			delete v_update_data;
			throw StorageEngineExceptionUniversal(1, "test data prepare error!!!\n");
		}
		it->clear();
		*it = v_update_data->operator [](i).second;
	}
}

/* 准备测试数据并返回结果 */
vector<string> *prepare_test_data(Oid &relid, Oid &idxid, bool isTemp = false)
{
	bool isDeadLock = false;
	Oid idxcolid = idxid;
	srand(time(NULL));

	if (!isTemp)
		begin_transaction();

	if (!isTemp)
		FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, relid, 0, relid);
	else
	{
		relid = FDPG_Heap::fd_temp_heap_create(MyDatabaseTableSpace, 0, relid);
		idxid = InvalidOid;
	}

	Relation rel = FDPG_Heap::fd_heap_open(relid, ShareLock);
	idxid = FDPG_Index::fd_index_create(rel, BTREE_TYPE, idxid, idxcolid);
	FDPG_Heap::fd_heap_close(rel, NoLock);
	FDPG_Transaction::fd_CommandCounterIncrement();

	if (!isTemp)
		commit_transaction();

	/* 插入数据 */
	if (!isTemp)
		begin_transaction();

	vector<string> *v_data = ran_do_insert(relid, isDeadLock);
	FDPG_Transaction::fd_CommandCounterIncrement();
	vector<pair<string, string> > *v_update = ran_do_update(relid, isDeadLock);
	FDPG_Transaction::fd_CommandCounterIncrement();

	if (!isTemp)
		commit_transaction();

	/* 重新计算表中应该存在的数据 */
	reset_data_update(v_data, v_update);

	delete v_update;
	return v_data;
}

/* 检测测试结果,v_data需要在外头排序过后再传进来 */
bool check_test_result(vector<string> *v_data, Oid relid, Oid idxid, vector<string> &v_key, int *start_pos)
{
	bool return_sta = true;
	vector<string> v_rdata, v_alldata, v_rdata2;
	int *colnum = new int[v_key.size()];
	int *strategynumber = new int[v_key.size()];
	for (int i = 0; i < v_key.size(); ++i)
	{
		colnum[i] = i + 1;
		strategynumber[i] = BTLessEqualStrategyNumber;
	}

	begin_transaction();
	index_get_result(relid, idxid, v_key.size(), colnum, strategynumber, v_key, v_rdata);
	get_all_tuple_in_heap(relid, v_alldata);
	commit_transaction();

	vector_index_get_result(*v_data, start_pos, v_key, v_rdata2);
	sort(v_rdata.begin(), v_rdata.end());
	sort(v_alldata.begin(), v_alldata.end());
	sort(v_rdata2.begin(), v_rdata2.end());

	if ((*v_data) != v_alldata || v_rdata2 != v_rdata)
		return_sta = false;

	delete []colnum;
	delete []strategynumber;

	return return_sta;
}

#define THREAD_PREPARE_TEST_DATA(RELID, IDXID, V_DATA) \
	GET_PARAM() = get_param(); \
	SAVE_PARAM(GET_PARAM()); \
	GET_THREAD_GROUP().create_thread(bind(&thread_prepare_test_data, GET_PARAM(), RELID, IDXID, V_DATA)); \
	GET_THREAD_GROUP().join_all();

void thread_prepare_test_data(BackendParameters *GET_PARAM(), Oid relid, Oid idxid, vector<string> **v_data)
{
	fxdb_SubPostmaster_Main(GET_PARAM());

	try {
		*v_data = prepare_test_data(relid, idxid);
	} catch (StorageEngineException &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
	}

	proc_exit(0);
}

bool thread_test_cluster_001()
{
	INTENT("测试多线程同时cluster同一表。");

	PREPARE_TEST();
	bool return_sta = true;

	clear_all();
	Oid relid = get_heap_id();
	Oid idxid = get_index_id();
	SpliterGenerater sg;
	vector<string> *v_data = NULL;
	Colinfo info = sg.buildHeapColInfo(3, 3, 2, 2);
	int colnumber[] = {1, 2};
	CompareCallback cmp_func[] = {my_compare_str, my_compare_str};
	/* 多列索引 */
	Colinfo idxinfo = sg.buildIndexColInfo(2, colnumber, cmp_func, SpliterGenerater::index_split_to_any<1, 2>);
	setColInfo(relid, info);
	setColInfo(idxid, idxinfo);

	do 
	{
		try
		{
			THREAD_PREPARE_TEST_DATA(relid, idxid, &v_data);

			/* 启动多个线程去并发cluster这个表 */
			const int THREAD_NUM = 10;
			bool sta[THREAD_NUM];
			shared_mutex ready[THREAD_NUM];
			for ( int i = 0; i < THREAD_NUM; ++i )
			{
				GET_PARAM() = get_param();
				SAVE_PARAM(GET_PARAM());
				GET_THREAD_GROUP().create_thread(
					bind(
					&thread_cluster_one_rel, 
					GET_PARAM(), 
					relid,
					idxid,
					&ready[i],
					&sta[i]
				)
					);
			}

			GET_THREAD_GROUP().join_all();

			/* 检测结果 */
			sort(v_data->begin(), v_data->end());
			int pos = v_data->size() / 2;
			string key = v_data->operator [](pos);//找出表中一半的数据
			vector<string> v_key;
			v_key.push_back(key.substr(0, 3));
			v_key.push_back(key.substr(3, 3));
			int start_pos[] = {0, 3};
			return_sta = check_test_result(v_data, relid, idxid, v_key, start_pos);

			for (int i = 0; i < THREAD_NUM; ++i)
			{
				if (!sta[i])
				{
					return_sta = false;
					break;
				}
			}
		} catch (StorageEngineException &se)
		{
			return_sta = false;
			printf("%s\n", se.getErrorMsg());
			user_abort_transaction();
		}
	} while (false);

	int drop_sta;
	SIMPLE_DROP_HEAP(relid, drop_sta);

	if (v_data)
		delete v_data;

	FREE_PARAM(BackendParameters *);
	return return_sta;
}

void thread_do_update(BackendParameters *GET_PARAM(), Oid relid, 
											Oid idxid, shared_mutex *ready,
											vector<string> *v_data, bool *sta)
{
	*sta = true;
	bool isDeadLock = false;
	vector<pair<string, string> > *v_update = NULL;
	fxdb_SubPostmaster_Main(GET_PARAM());

	try
	{
		ready->lock_shared();
		ready->unlock_shared();

		begin_transaction();

		v_update = ran_do_update(relid, isDeadLock);

		reset_data_update(v_data, v_update);

		commit_transaction();
	} catch (StorageEngineException &se)
	{
		*sta = false;
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
	}

	if(v_update)
		delete v_update;
	proc_exit(0);
}

bool thread_test_cluster_002()
{
	INTENT("测试更新表和cluster并发执行。");

	PREPARE_TEST();
	bool return_sta = true;

	clear_all();
	Oid relid = get_heap_id();
	Oid idxid = get_index_id();
	SpliterGenerater sg;
	vector<string> *v_data = NULL;
	Colinfo info = sg.buildHeapColInfo(3, 3, 2, 2);
	int colnumber[] = {1, 2};
	CompareCallback cmp_func[] = {my_compare_str, my_compare_str};
	/* 多列索引 */
	Colinfo idxinfo = sg.buildIndexColInfo(2, colnumber, cmp_func, SpliterGenerater::index_split_to_any<1, 2>);
	setColInfo(relid, info);
	setColInfo(idxid, idxinfo);

	try
	{
		THREAD_PREPARE_TEST_DATA(relid, idxid, &v_data);

		/* 启动两个线程,一个更新数据，一个cluster */
		const int THREAD_NUM = 2;
		bool sta[THREAD_NUM];
		shared_mutex ready[THREAD_NUM];

		/* 更新线程 */
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(
			bind(
			&thread_do_update, 
			GET_PARAM(),
			relid, 
			idxid,
			&ready[0], 
			v_data, 
			&sta[0]
			));
			/* cluster线程 */
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(
				bind(
				&thread_cluster_one_rel, 
				GET_PARAM(),
				relid, 
				idxid,
				&ready[1],  
				&sta[1]
			));
		GET_THREAD_GROUP().join_all();

		/* 检测结果 */
		sort(v_data->begin(), v_data->end());
		int pos = v_data->size() / 2;
		string key = v_data->operator [](pos);//找出表中一半的数据
		vector<string> v_key;
		v_key.push_back(key.substr(0, 3));
		v_key.push_back(key.substr(3, 3));
		int start_pos[] = {0, 3};
		return_sta = check_test_result(v_data, relid, idxid, v_key, start_pos);

		for (int i = 0; i < THREAD_NUM; ++i)
		{
			if (!sta[i])
			{
				return_sta = false;
				break;
			}
		}
	} catch (StorageEngineException &se)
	{
		return_sta = false;
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
	}

	int drop_sta;
	SIMPLE_DROP_HEAP(relid, drop_sta);

	if (v_data)
		delete v_data;

	FREE_PARAM(BackendParameters *);
	return return_sta;
}

static
void thread_random_do_cluster(BackendParameters *GET_PARAM(), 
															Oid *relid, Oid *idxid, 
															int heap_num, int minutes, bool *sta)
{
	*sta = true;

	fxdb_SubPostmaster_Main(GET_PARAM());
	TimestampTz start_time = GetCurrentTimestamp();

	try
	{
		while (true)
		{
			int base = rand() % heap_num;

			begin_transaction();

			/* cluster */
			FDPG_Heap::fd_ClusterRel(relid[base], idxid[base], false);

			commit_transaction();

			TimestampTz end_time = GetCurrentTimestamp();
			long secs;
			int usecs;
			TimestampDifference(start_time, end_time, &secs, &usecs);
			if(secs >= minutes * 60)
			{
				break;
			}

			pg_sleep(3000000);
		}
	} catch (StorageEngineException &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		*sta = false;
	}

	proc_exit(0);
}

bool thread_test_cluster_003()
{
	INTENT("长时间测例，模拟随机线程更新表，随机线程cluster表。");
	
	PREPARE_TEST();
	bool return_sta = true;

	clear_all();
	const int HEAP_NUM = 2;
	Oid relid[HEAP_NUM];
	Oid idxid[HEAP_NUM];
	map<Oid, vector<string> > rel_info;

	SpliterGenerater sg;
	Colinfo info = sg.buildHeapColInfo(3, 3, 2, 2);
	int colnumber[] = {1, 2};
	CompareCallback cmp_func[] = {my_compare_str, my_compare_str};
	/* 多列索引 */
	Colinfo idxinfo = sg.buildIndexColInfo(2, colnumber, cmp_func, SpliterGenerater::index_split_to_any<1, 2>);

	try
	{
		begin_transaction();

		for (int i = 0; i < HEAP_NUM; ++i)
		{
			int create_sta;
			relid[i] = get_heap_id();
			idxid[i] = get_index_id();
			Colinfo heap_info = info;

			SIMPLE_CREATE_HEAP(relid[i], create_sta);
			SIMPLE_CREATE_INDEX(relid[i], idxid[i], info, idxinfo, create_sta);
			rel_info[relid[i] ] = vector<string>();
		}

		commit_transaction();

		const int THREADS = 2;
		const int TIME_RUN_ = 1;//每个线程并发运行一分钟
		bool sta[THREADS];

		/*
		 * 这里启动N个线程用于更新表和cluster表
		 * 其中一半的线程用于更新表，一半的线程执行cluster操作
		 */
		for (int i = 0; i < THREADS; ++i)
		{
			/* 更新线程 */
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(
				bind(&thread_random_do,
				TIME_RUN_,
				&rel_info,
				relid,
				idxid,
				HEAP_NUM,
				HEAP_NUM,
				GET_PARAM()
				)
				);

			/* cluster线程 */
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(
				bind(&thread_random_do_cluster,
				GET_PARAM(),
				relid,
				idxid,
				HEAP_NUM,
				TIME_RUN_,
				&sta[i]
				)
				);
		}
		GET_THREAD_GROUP().join_all();

		/* 检测结果 */
		map<Oid, vector<string> >::iterator it = rel_info.begin();
		while (it != rel_info.end())
		{
			SAVE_INFO_FOR_DEBUG();

			string key = it->second[0];
			vector<string> v_key;
			SAVE_INFO_FOR_DEBUG();
			v_key.push_back(key.substr(0, 3));
			v_key.push_back(key.substr(3, 3));

			Oid index = InvalidOid;
			/* 找出表的索引id */
			for (int i = 0; i < HEAP_NUM; ++i)
			{
				if (relid[i] == it->first)
				{
					index = idxid[i];
					break;
				}
			}

			SAVE_INFO_FOR_DEBUG();
			sort(it->second.begin(), it->second.end());
			int start_pos[] = {0, 3};
			if (!check_test_result(&it->second, it->first, index, v_key, start_pos))
			{
				return_sta = false;
				break;
			}
			++it;
		}
	} catch (StorageEngineException &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		return_sta = false;
	}

	for (int i = 0; i < HEAP_NUM; ++i)
	{
		int drop_sta;
		SIMPLE_DROP_HEAP((relid[i]), drop_sta);
	}

	FREE_PARAM(BackendParameters *);
	return return_sta;
}
