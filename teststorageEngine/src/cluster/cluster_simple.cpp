#include "utils/util.h"
#include "postgres.h"
#include "commands/cluster.h"
#include "utils/tqual.h"
#include "test_fram.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include <time.h>
#include <algorithm>

using namespace std;
using namespace FounderXDB::StorageEngineNS;

extern vector<string> *ran_do_delete(const Oid rel_id, bool &is_deadlock);
extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);
extern vector<string> *prepare_test_data(Oid &relid, Oid &idxid, bool isTemp = false);
extern bool check_test_result(vector<string> *v_data, Oid relid, Oid idxid, vector<string> &v_key, int *start_pos);

void get_all_tuple_in_heap(Oid relid, vector<string> &v_data)
{
	Relation rel = FDPG_Heap::fd_heap_open(relid, AccessShareLock);
	HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);

	HeapTuple tuple = NULL;
	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		char *data = FDPG_Common::fd_tuple_to_chars(tuple);
		v_data.push_back(data);
		pfree(data);
	}

	FDPG_Heap::fd_heap_endscan(scan);
	FDPG_Heap::fd_heap_close(rel, NoLock);
}

/* 该函数使用相等策略查找，只适用于索引建立在表第一列上的情况 */
void index_find_strtuple(Oid relid, Oid indexid, string index_key, vector<string>& v_data)
{
	Relation rel = FDPG_Heap::fd_heap_open(relid, AccessShareLock);
	Relation indexrel = FDPG_Index::fd_index_open(indexid, AccessShareLock);

	Datum value[1];
	value[0] = fdxdb_string_formdatum(index_key.c_str(), index_key.length());
	ScanKeyData key[1];
	//binding value to scankey
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], 1, BTEqualStrategyNumber, my_compare_str, value[0]);

	IndexScanDesc scan = FDPG_Index::fd_index_beginscan(rel, indexrel, SnapshotNow, 1, 0);
	FDPG_Index::fd_index_rescan(scan, key, 1, NULL, 0);
	HeapTuple tuple = NULL;
	while((tuple = FDPG_Index::fd_index_getnext(scan, ForwardScanDirection)) != NULL)
	{
		char *data = FDPG_Common::fd_tuple_to_chars(tuple);
		v_data.push_back(data);
		pfree(data);
	}
	FDPG_Index::fd_index_endscan(scan);
	FDPG_Index::fd_index_close(indexrel, NoLock);
	FDPG_Heap::fd_heap_close(rel, NoLock);
}

bool test_cluster_heap_01()
{
	INTENT("创建表插入数据然后删除部分数据，随后cluster这个表，测试结构是否正确。");

	bool return_sta = true;
	const int DATA_ROW = 100;
	const int DATA_WIDTH = DATA_LEN * 10;

	clear_all();
	Oid relid = get_heap_id();
	Oid indexid = get_index_id();
	const int INDEX_COL_LEN = 3;
	DataGenerater dg(DATA_ROW, DATA_WIDTH);
	SpliterGenerater sg;
	Colinfo colinfo = sg.buildHeapColInfo(1, INDEX_COL_LEN);
	int col_num[] = {1};
	CompareCallback cmp_func[] = {my_compare_str};
	Colinfo indexcolinfo = sg.buildIndexColInfo(1, col_num, cmp_func, SpliterGenerater::index_split_to_any<1>);
	setColInfo(relid, colinfo);
	setColInfo(indexid, indexcolinfo);
	char *data[DATA_ROW];

	dg.dataGenerate();
	for (int i = 0; i < DATA_ROW; ++i)
	{
		data[i] = dg[i];
	}

	do 
	{
		try
		{
			begin_transaction();
			FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, relid, 0, relid);
			Relation rel = FDPG_Heap::fd_heap_open(relid, ShareLock);
			FDPG_Index::fd_index_create(rel, BTREE_TYPE, indexid, indexid);
			FDPG_Heap::fd_heap_close(rel, NoLock);
			commit_transaction();
			insert_large_data(data, DATA_ROW, DATA_WIDTH, relid);
			
			/* 随机删除数据 */
			begin_transaction();
			bool isDeadLock = false;
			srand(time(NULL));
			vector<string> *v_deleteData = ran_do_delete(relid, isDeadLock);

			int index = 0;
			/* cluster表，这里反复cluster多次 */
			const int CLUST_TIME = 5;
			for (int i = 0; i < CLUST_TIME; ++i)
			{
				FDPG_Heap::fd_ClusterRel(relid, indexid, false);
			}


			/* 检查cluster表后表中数据是否还正确 */
			vector<string> v_data;
			for(int i = 0; i < DATA_ROW; ++i)
			{
				v_data.push_back(data[i]);
			}
			vector<string> v_unDeleteData(v_data.size() - v_deleteData->size());

			sort(v_data.begin(), v_data.end());
			sort(v_deleteData->begin(), v_deleteData->end());
			set_difference(v_data.begin(), v_data.end(),
				v_deleteData->begin(), v_deleteData->end(),
				v_unDeleteData.begin());
			delete v_deleteData;

			/* 获取索引查找的key */
			string find_data = v_unDeleteData[v_unDeleteData.size()/2];
			string find_key = find_data.substr(0, INDEX_COL_LEN);

			v_data.clear();
			get_all_tuple_in_heap(relid, v_data);

			sort(v_unDeleteData.begin(), v_unDeleteData.end());
			sort(v_data.begin(), v_data.end());
			if (v_unDeleteData != v_data)
				return_sta = false;
		
			/* 使用索引的key查找 */
			v_data.clear();
			index_find_strtuple(relid, indexid, find_key, v_data);
			if (v_data.size() != 1 || v_data[0].compare(find_data) != 0)
			{
				return_sta = false;
			}
			
			commit_transaction();

			insert_large_data(data, DATA_ROW, DATA_WIDTH, relid);

			/* 再次插入数据检查结果 */
			for(int i = 0; i < DATA_ROW; ++i)
			{
				v_unDeleteData.push_back(data[i]);
			}

			begin_transaction();

			v_data.clear();
			get_all_tuple_in_heap(relid, v_data);

			sort(v_unDeleteData.begin(), v_unDeleteData.end());
			sort(v_data.begin(), v_data.end());
			if (v_data != v_unDeleteData)
				return_sta = false;

			commit_transaction();
		} catch (StorageEngineException &se)
		{
			return_sta = false;
			printf("%s", se.getErrorMsg());
			user_abort_transaction();
		}
	} while (false);

	begin_transaction();
	FDPG_Heap::fd_heap_drop(relid);
	commit_transaction();

	return return_sta;
}

static
void insert_test_data(Oid relid, const int data_rows, 
											vector<string> &v_data, int data_lens,
											bool insert_only_once)
{
	vector<ItemPointerData> v_it;
	if (v_data.size() == 0)
	{
		const int INSERT_ROWS = 5000;

		int insert_time = data_rows / INSERT_ROWS;
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(relid, RowExclusiveLock);
		HeapTuple tuple = NULL;
		for (int i = 0; i < insert_time; ++i)
		{
			DataGenerater dg(INSERT_ROWS, data_lens);
			dg.dataGenerate();
			for (int j = 0; j < INSERT_ROWS; ++j)
			{
				tuple = FDPG_Heap::fd_heap_form_tuple(dg[j], data_lens);
				FDPG_Heap::fd_simple_heap_insert(rel, tuple);
				v_data.push_back(dg[j]);
				v_it.push_back(tuple->t_self);
				pfree(tuple);
			}
		}
		commit_transaction();

		if (insert_only_once)
		{
			int pos = v_it.size() / 3;
			v_data.erase(v_data.begin(), v_data.begin() + pos);
		}
	} else
	{
		insert_data(v_data, relid, &v_it);
		int pos = v_it.size() / 3;
		v_data.erase(v_data.begin(), v_data.begin() + pos);
	}
	int pos = v_it.size() / 3;
	vector<ItemPointerData> v_it2;
	v_it2.assign(v_it.begin(), v_it.begin() + pos);
	delete_data_(v_it2, relid);
}

bool test_cluster_heap_02()
{
	INTENT("性能测试，建立多个索引，根据其中某一个索引对表进行cluster"
				 "比较前后所有索引扫描的性能情况。");

	bool return_sta = true;

	clear_all();
	const int IDXNUM = 3;
	Oid idxid[IDXNUM];
	Oid relid = get_heap_id();
	for (int i = 0; i < IDXNUM; ++i)
		idxid[i] = get_index_id();
	time_t start_time, end_time;
	const int DATA_ROWS = 100000;
	const int DATA_LENS = 1024;

	SpliterGenerater sg;
	Colinfo heapinfo = sg.buildHeapColInfo(3, 3, 3, 3);
	int colnum[] = {1};
	CompareCallback cmpfunc[] = {my_compare_str};
	Colinfo indexinfo1 = sg.buildIndexColInfo(1, colnum, cmpfunc, SpliterGenerater::index_split_to_any<1>);
	colnum[0] = 2;
	Colinfo indexinfo2 = sg.buildIndexColInfo(1, colnum, cmpfunc, SpliterGenerater::index_split_to_any<2>);
	colnum[0] = 3;
	Colinfo indexinfo3 = sg.buildIndexColInfo(1, colnum, cmpfunc, SpliterGenerater::index_split_to_any<3>);
	setColInfo(relid, heapinfo);
	setColInfo(idxid[0], indexinfo1);
	setColInfo(idxid[1], indexinfo2);
	setColInfo(idxid[2], indexinfo3);

	vector<string> v_data;
	map<Oid, pair<int, int> > m_time;

	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, relid, 0, relid);
		Relation rel = FDPG_Heap::fd_heap_open(relid, ShareLock);
		FDPG_Index::fd_index_create(rel, BTREE_TYPE, idxid[0], idxid[0]);
		FDPG_Index::fd_index_create(rel, BTREE_TYPE, idxid[1], idxid[1]);
		FDPG_Index::fd_index_create(rel, BTREE_TYPE, idxid[2], idxid[2]);
		FDPG_Heap::fd_heap_close(rel, NoLock);
		commit_transaction();

		/* 构建测试数据，分批插入一共DATA_ROWS条数据 */
		insert_test_data(relid, DATA_ROWS, v_data, DATA_LENS, true);

		/* 使用索引扫描并计算每个索引扫描的时间 */
		sort(v_data.begin(), v_data.end());
		int pos = v_data.size() / 2;
		
		for (int i = 0; i < IDXNUM; ++i)
		{
			string key = v_data[pos];
			key = key.substr(0 + 3 * i, 3);
			vector<string> v_key;
			v_key.push_back(key);
			start_time = GetCurrentTimestamp();
			int start_pos[] = {0 + 3 * i};
			if (!check_test_result(&v_data, relid, idxid[i], v_key, start_pos))
				return_sta = false;
			end_time = GetCurrentTimestamp();
			long secs;
			int usecs;
			TimestampDifference(start_time, end_time, &secs, &usecs);
			m_time[idxid[i]] = pair<int, int>(usecs, 0);
		}

		/* 根据第一个索引cluster */
		begin_transaction();
		FDPG_Heap::fd_ClusterRel(relid, idxid[0], false);
		commit_transaction();

		/* cluster后再次记录索引扫描的时间 */
		for (int i = 0; i < IDXNUM; ++i)
		{
			string key = v_data[pos];
			key = key.substr(0 + 3 * i, 3);
			vector<string> v_key;
			v_key.push_back(key);
			start_time = GetCurrentTimestamp();
			int start_pos[] = {0 + 3 * i};
			if (!check_test_result(&v_data, relid, idxid[i], v_key, start_pos))
				return_sta = false;
			end_time = GetCurrentTimestamp();
			long secs;
			int usecs;
			TimestampDifference(start_time, end_time, &secs, &usecs);
			m_time[idxid[i]].second = usecs;
		}

		printf("cluster base on %u!!!\n", idxid[0]);
		for (int i = 0; i < IDXNUM; ++i)
			printf("index %u scan [%dmicrosecs --cluster--> %dmicrosecs]\n", idxid[i], m_time[idxid[i]].first, m_time[idxid[i]].second);

		begin_transaction();
		FDPG_Heap::fd_heap_drop(relid);
		commit_transaction();
	} catch (StorageEngineException &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		return_sta = false;
	}

	return return_sta;
}

bool test_cluster_exit_heap_01()
{
	INTENT("创建表和索引并插入部分数据和更新部分数据，"
		"cluster表后关闭服务器再启动并检测结果。");

	bool return_sta = true;
	char data[][DATA_LEN] = 
	{
		"data1test1",
		"data2test2",
		"lumia800",
		"ipad200",
		"galaxy s4",
		"HTC ONE ",
		"iphone 100"
	};
	const int UPDATE_ROW = 4;
	const int INDEX_KEY_LEN = 5;
	char *update_data = "nokia1080";
	SpliterGenerater sg;
	Colinfo relinfo = sg.buildHeapColInfo(2, INDEX_KEY_LEN, 2);
	int colnumber[] = {1};
	CompareCallback cmp_func[] = {my_compare_str};
	Colinfo indexinfo = sg.buildIndexColInfo(1, colnumber, cmp_func, SpliterGenerater::index_split_to_any<1>);

	try
	{
		SHUTDOWN_TEST_STEP_1(TransExit)
		{
			clear_all();
			Oid relid = get_heap_id();
			Oid indexid = get_index_id();
			setColInfo(relid, relinfo);
			setColInfo(indexid, indexinfo);

			begin_transaction();
			
			FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, relid, 0, relid);
			Relation rel = FDPG_Heap::fd_heap_open(relid, ShareLock);
			FDPG_Index::fd_index_create(rel, BTREE_TYPE, indexid, indexid);

			commit_transaction();

			insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN, relid);

			begin_transaction();

			/* 更新数据 */
			int sta = 0, count = 0;
			rel = FDPG_Heap::fd_heap_open(relid, AccessShareLock);
			ItemPointerData it = findTuple(data[UPDATE_ROW], rel, sta, count, strlen(data[UPDATE_ROW]));
			HeapTuple tuple = FDPG_Heap::fd_heap_form_tuple(update_data, strlen(update_data) + 1);
			FDPG_Heap::fd_simple_heap_update(rel, &it, tuple);
			FDPG_Heap::fd_heap_close(rel, NoLock);
			FDPG_Transaction::fd_CommandCounterIncrement();

			/* cluster */
			FDPG_Heap::fd_ClusterRel(relid, indexid, false);

			commit_transaction();

			/* 保存数据并退出程序 */
			EXIT_TEST_SAVE_DATA(&relid, sizeof(relid));
			EXIT_TEST_SAVE_DATA(&indexid, sizeof(indexid));
			exit(0);
		}
		SHUTDOWN_TEST_STEP_2()
		{
			Oid *relid = (Oid *)EXIT_TEST_READ_DATA();
			Oid *idxid = (Oid *)EXIT_TEST_READ_DATA();
			setColInfo(*relid, relinfo);
			setColInfo(*idxid, indexinfo);
			int sta = 0, count = 0;
			vector<string> v_data;
			/* 检测结果 */
			begin_transaction();
			Relation rel = FDPG_Heap::fd_heap_open(*relid, AccessShareLock);

			/* 找不到更新前的数据 */
			findTuple(data[UPDATE_ROW], rel, sta, count);
			if (sta == 1)
				return_sta = false;
			/* 找到更新后的数据 */
			findTuple(update_data, rel, sta, count);
			if (sta == 0)
				return_sta = false;
			/* 使用索引找更新后的数据 */
			string index_key;
			index_key.assign(update_data, INDEX_KEY_LEN);
			v_data.clear();
			index_find_strtuple(*relid, *idxid, index_key, v_data);
			if (v_data.size() <= 0 || v_data[0].compare(update_data) != 0)
				return_sta = false;
			
			v_data.clear();
			get_all_tuple_in_heap(*relid, v_data);

			FDPG_Heap::fd_heap_close(rel, NoLock);
			FDPG_Heap::fd_heap_drop(*relid);

			commit_transaction();

			memset(&data[UPDATE_ROW], 0, DATA_LEN);
			memcpy(&data[UPDATE_ROW], update_data, strlen(update_data) + 1);

			/* 比较结果 */
			vector<string> v_data2;
			for(int i = 0; i < ARRAY_LEN_CALC(data); ++i)
			{
				v_data2.push_back(data[i]);
			}
			sort(v_data.begin(), v_data.end());
			sort(v_data2.begin(), v_data2.end());
			if (v_data != v_data2)
				return_sta = false;
			
			free(relid);
			free(idxid);
			CLEAN_SHUTDOWN_TEST();
		}
		END_SHUTDOWN_TEST();
	} catch (StorageEngineException &se)
	{
		printf("%s\n", se.getErrorMsg());
		return_sta = false;
		user_abort_transaction();
		}

	return return_sta;
}

extern void index_get_result(Oid relid, Oid idxid, int colcount, 
														 int *column, int *strategynumber,
														 vector<string> &v_key, vector<string> &v_data);
extern void vector_index_get_result(vector<string> &v_data, 
																		int *start_pos,
																		vector<string> &key, 
																		vector<string> &result);

bool test_cluster_exit_heap_02()
{
	INTENT("性能测试。使用两个具有相同数据的表做比较。"
				 "一个表执行cluster操作，然后关闭程序再次启动。"
				 "对两个表使用索引扫描相同数据的速度进行比较，"
				 "这样就排除某些表中的块缓存到内存中导致比较结果"
				 "有误差的情况。");

	clear_all();
	bool return_sta = true;
	const int HEAPNUM = 2;
	Oid idxid[HEAPNUM];
	Oid relid[HEAPNUM];
	
	time_t start_time, end_time;
	const int DATA_ROWS = 500000;
	const int DATA_LENS = 400;

	SpliterGenerater sg;
	Colinfo heapinfo = sg.buildHeapColInfo(1, 7);
	int colnum[] = {1};
	CompareCallback cmpfunc[] = {my_compare_str};
	Colinfo indexinfo = sg.buildIndexColInfo(1, colnum, cmpfunc, SpliterGenerater::index_split_to_any<1>);

	vector<string> v_data;
	map<Oid, pair<int, int> > m_time;

	for (int i = 0; i < HEAPNUM; ++i)
	{
		idxid[i] = get_index_id();
		relid[i] = get_heap_id();
	}

	try
	{
		begin_transaction();
		Relation trel = relation_open(relid[0], AccessShareLock);
		if (trel == NULL)
		{
			commit_transaction();

			for (int i = HEAPNUM - 1; i >= 0; --i)
			{
				setColInfo(relid[i], heapinfo);
				setColInfo(idxid[i], indexinfo);

				begin_transaction();
				FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, relid[i], 0, relid[i]);
				Relation rel = FDPG_Heap::fd_heap_open(relid[i], ShareLock);
				FDPG_Index::fd_index_create(rel, BTREE_TYPE, idxid[i], idxid[i]);
				FDPG_Heap::fd_heap_close(rel, NoLock);
				commit_transaction();

				/* 构建测试数据，分批插入一共DATA_ROWS条数据 */
				insert_test_data(relid[i], DATA_ROWS, v_data, DATA_LENS, false);
			}
			int data_rows = v_data.size();
			EXIT_TEST_SAVE_DATA(&data_rows, sizeof(data_rows));
			/* 保存测试数据 */
			EXIT_TEST_SAVE_LARGE_DATA(v_data);
			
			v_data.clear();
			/* 对第一个表执行cluster操作 */
			begin_transaction();
			time_t t = GetCurrentTimestamp();
			FDPG_Heap::fd_ClusterRel(relid[0], idxid[0], true);
			time_t t2 = GetCurrentTimestamp();
			long secs;
			int usecs;
			TimestampDifference(t, t2, &secs, &usecs);
			double time = secs + ((double)usecs / 1000000);
			commit_transaction();
			printf("cluster cost %lf sec\n", time);

			/* 保存数据并退出程序 */
 			for (int i = 0; i < HEAPNUM; ++i)
			{
				EXIT_TEST_SAVE_DATA(&relid[i], sizeof(relid[i]));
				EXIT_TEST_SAVE_DATA(&idxid[i], sizeof(idxid[i]));
			}
		} else
		{
			relation_close(trel, NoLock);
			commit_transaction();
			vector<string> v_data;

			int *data_rows = (int *)EXIT_TEST_READ_DATA();
			EXIT_TEST_READ_LARGE_DATA(v_data, *data_rows);
			free(data_rows);

			for (int i = 0; i < HEAPNUM; ++i)
			{
				Oid *id1 = (Oid *)EXIT_TEST_READ_DATA();
				Oid *id2 = (Oid *)EXIT_TEST_READ_DATA();
				relid[i] = *id1;
				idxid[i] = *id2;
				free(id1);
				free(id2);
				setColInfo(relid[i], heapinfo);
				setColInfo(idxid[i], indexinfo);
			}

			/* 检测结果，比较cluster前后的索引扫描速度 */
			map<Oid, double> m_time;
			sort(v_data.begin(), v_data.end());
			int pos1 = v_data.size() / 5 * 4;
			string key = v_data[pos1];
			key = key.substr(0, 7);
			vector<string> v_key;
			v_key.push_back(key);
			int start_pos[] = {0};
			int colnum[] = {1};
			int strategynumber[] = {BTLessEqualStrategyNumber};
			vector<string> v_rdata2;
			vector_index_get_result(v_data, start_pos, v_key, v_rdata2);
			v_data.clear();
			for(int i = 0; i < HEAPNUM; ++i)
			{
				begin_transaction();
				vector<string> v_rdata;
				time_t start_time, end_time;
				start_time = GetCurrentTimestamp();
				index_get_result(relid[i], idxid[i], v_key.size(), colnum, strategynumber, v_key, v_rdata);
				end_time = GetCurrentTimestamp();
				long secs;
				int usecs;
				TimestampDifference(start_time, end_time, &secs, &usecs);
				m_time[idxid[i]] = secs + ((double)usecs / 1000000);
				if (v_rdata != v_rdata2)
					return_sta = false;
				commit_transaction();
			}
			printf("\ncluster base on %u!!!", idxid[0]);
			for (int i = 0; i < HEAPNUM; ++i)
				printf("\nindex %u scan cost %lf sec!!!\n", idxid[i], m_time[idxid[i]]);

			if (m_time[idxid[0]] > m_time[idxid[1]])
				return_sta = false;
		}
	} catch (StorageEngineException &se)
	{
		printf("%s\n", se.getErrorMsg());
		return_sta = false;
		user_abort_transaction();
	}

	return return_sta;
}
