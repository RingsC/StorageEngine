#include <boost/assign.hpp>
#include "boost/thread/thread.hpp"
#include "utils/util.h"

#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "catalog/metaData.h"
#include "test_fram.h"
#include "index/test_index_update.h"
#include "utils/fmgroids.h"
#include "interface/FDPGAdapter.h"
#include "iostream"
#include "interface/StorageEngineExceptionUniversal.h"

using namespace FounderXDB::StorageEngineNS;

bool testIndexUpdate_SingleColumn( void )
{
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	try
	{
		begin_first_transaction();
		{
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("3");
			heap.Insert("4");
			heap.Insert("2");
			heap.Insert("6");
			heap.Insert("9");
			CommandCounterIncrement();


			//create a index
			std::map<int,CompareCallback> vecKeys;
			insert(vecKeys)(1,str_compare);
			SimpleIndex index(heap,indid,vecKeys);
			heap.Insert("1");
			CommandCounterIncrement();

			//construct sacn condition
			SearchCondition searchCondition;
			searchCondition.Add(1,LessThan,"6",str_compare);

			std::set<std::string> sNeedFind;
			sNeedFind += "1","2","3","4";
			CHECK_BOOL(sNeedFind == index.Find(searchCondition));
			//construct sacn condition
			SearchCondition searchCondition2;
			searchCondition2.Add(1,LessThan,"6",str_compare);
			//update the tuple
			heap.Update("2","8");
			heap.Update("9","0");

			sNeedFind.clear();
			sNeedFind += "0","1","3","4";
			CHECK_BOOL(sNeedFind == index.Find(searchCondition2));
		}
		commit_transaction();
	}
	CATCHEXCEPTION

		return true;
}

bool testIndexUpdate_MultiColumn( void )
{
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	try
	{
		begin_first_transaction();
		{
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("123456");
			heap.Insert("123789");
			heap.Insert("012562");
			heap.Insert("012120");
			heap.Insert("012456");
			CommandCounterIncrement();

			//create a index on testRelation
			std::map<int,CompareCallback> vecKeys;
			insert(vecKeys)(1,str_compare)(2,str_compare);
			SimpleIndex index(heap,indid,vecKeys);
			heap.Insert("012506");
			CommandCounterIncrement();

			//construct the find condition
			SearchCondition searchCondition;
			searchCondition.Add(1,LessThan,"123",str_compare);
			searchCondition.Add(2,GreaterThan,"45",str_compare);

			//find and check
			std::set<std::string> sNeedFind;
			sNeedFind += "012506","012562";
			CHECK_BOOL(sNeedFind == index.Find(searchCondition));

			//construct the find condition
			SearchCondition searchCondition2;
			searchCondition2.Add(1,LessThan,"123",str_compare);
			searchCondition2.Add(2,GreaterThan,"45",str_compare);
			//update the tuple
			heap.Update("012506","124889");
			heap.Update("123456","121671");

			sNeedFind.clear();
			sNeedFind += "012562","121671";
			CHECK_BOOL(sNeedFind == index.Find(searchCondition2));
		}
		commit_transaction();

	}
	CATCHEXCEPTION

		return true;
}

bool testIndexUpdate_InAnotherTrans( void )
{
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	using namespace FounderXDB::StorageEngineNS;


	try
	{
		begin_first_transaction();
		{
			//construct the find condition
			SearchCondition searchCondition;
			searchCondition.Add(1,LessThan,"123",str_compare);
			searchCondition.Add(2,GreaterThan,"45",str_compare);
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true,false);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("123456");
			heap.Insert("123789");
			heap.Insert("012562");
			heap.Insert("012120");
			heap.Insert("012456");
			CommandCounterIncrement();

			std::map<int,CompareCallback> vecKeys;
			insert(vecKeys)(1,str_compare)(2,str_compare);
			SimpleIndex index(heap,indid,vecKeys,true,false);
			heap.Insert("012506");
			CommandCounterIncrement();

			std::set<std::string> sNeedFind;
			sNeedFind += "012506","012562";
			CHECK_BOOL(sNeedFind == index.Find(searchCondition));
		}
		commit_transaction();
	}
	CATCHEXCEPTION

		try
	{
		begin_transaction();

		{
			//construct the find condition
			SearchCondition searchCondition;
			searchCondition.Add(1,LessThan,"123",str_compare);
			searchCondition.Add(2,GreaterThan,"45",str_compare);
			//open the heap
			SimpleHeap heap(reltablespace,relid,FixSpliter::split);
			using namespace boost::assign;
			std::vector<int> v;
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//open the index
			std::map<int,CompareCallback> vecKeys;
			insert(vecKeys)(1,str_compare)(2,str_compare);
			SimpleIndex index(heap,indid,vecKeys,false);

			//update the tuple
			heap.Update("012506","124889");
			heap.Update("123456","121671");


			std::set<string> sNeedFind;
			sNeedFind.insert("012562");
			sNeedFind.insert("121671");
			CHECK_BOOL(sNeedFind == index.Find(searchCondition));
		}
		commit_transaction();
	}
	CATCHEXCEPTION

		return true;
}
#define THREAD_NUMS_5 5
bool test_thread_index_update_01()
{
	++INDEX_ID;
	INTENT("测试启动多个线程分别更新表,这些表是单列的,每个线程独享数据。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	int i=0;
	SpliterGenerater sg;
	const int COL_NUM = 1;

	try
	{
		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,1);//创建表的colinfo
		int rid=++THREAD_RID;

		//setColInfo(rid,heap_info);
		thread_create_heap_xy(THREAD_NUMS_5,rid,heap_info);//建表

		const unsigned int data_row = 10;
		const unsigned int data_len = 20;
		DataGenerater dg(data_row, data_len);
		dg.dataGenerate();

		char data[][DATA_LEN] = {"a", "b"};//初始化插入表的数据
		char predict_data[][DATA_LEN] = {"c", "d"};//预计更新后的表中的数据
		int array_size=sizeof("a");

		for (rid=THREAD_RID;rid<THREAD_RID+THREAD_NUMS_5;rid++)
		{
			thread_insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN,rid);//向表中插入数据
		}


		const int INDEX_COL_NUM = 1;
		int col_number[INDEX_COL_NUM] = {1};
		CompareCallback cmp_func[INDEX_COL_NUM] = 
		{
			my_compare_str,
		};
		Colinfo index_info = sg.buildIndexColInfo(1, col_number, cmp_func, SpliterGenerater::index_split_to_any<1>);//构建index的colinfo
		int index_id=INDEX_ID;

		for (rid=THREAD_RID;rid<THREAD_RID+THREAD_NUMS_5;rid++,index_id++)
		{
			setColInfo(index_id,index_info);
			thread_create_index(rid, heap_info,index_info,index_id);//启动线程建索引
		}


		boost::thread_group tg;
		BackendParameters *paramArr[THREAD_NUMS_5];	
		prepare_param(paramArr,THREAD_NUMS_5);
		index_id=INDEX_ID;
		rid=THREAD_RID;
		for(i = 0;i < THREAD_NUMS_5;i++,index_id++,rid++)
		{
			tg.create_thread(boost::bind(&thread_index_update_01,(void*)paramArr[i],rid,heap_info,index_info,index_id));//启动线程更新表
		}

		tg.join_all();
		free_param(paramArr,THREAD_NUMS_5);

		IndexScanInfo isi[THREAD_NUMS_5];
		for(i = 0; i < THREAD_NUMS_5; i++,index_id++,rid++)
		{
		alloc_scan_space(INDEX_COL_NUM, isi[i]);
		isi[i].cmp_values[0] = fdxdb_string_formdatum("a", strlen("a"));//初始化scankey
		isi[i].cmp_func[0] = my_compare_str;
		isi[i].cmp_strategy[0] = BTGreaterEqualStrategyNumber;
		isi[i].col_array[0] = 1;
		init_scan_key(isi[i]);
		}
		/*
		* 创建线程利用索引查询数据
		*/
		PREPARE_PARAM(BackendParameters *);

		char found_array[data_row][DATA_LEN];

		index_id=INDEX_ID;
		rid=THREAD_RID;
		DataGenerater *found[THREAD_NUMS_5] ;
		unsigned int *len[THREAD_NUMS_5]={0};


		for(i = 0; i < THREAD_NUMS_5; i++,index_id++,rid++)
		{
			DataGenerater *found_dg = new DataGenerater(10, DATA_LEN);
			unsigned int *array_len =new unsigned int(0);
			BackendParameters *param = get_param();
			SAVE_PARAM(param);
			tg.create_thread(bind(&thread_index_scan, param, index_id, index_info, rid, heap_info, &isi[i], found_dg, array_len));
			found[i]=found_dg;
			len[i]=array_len;
		}
		tg.join_all();
		FREE_PARAM(BackendParameters *);
		//free_scan_key(isi);

		//生成二维字符串数组
		for (i=0;i<THREAD_NUMS_5;i++)
		{
			found[i]->dataToDataArray2D(*len[i], found_array);
			//检查索引查找出来的结果都在预测的表中
			bool sta = check_array_equal(predict_data, found_array, array_size,ARRAY_LEN_CALC(data));
			assert(sta == TRUE);
		}

		for (i=0;i<THREAD_NUMS_5;i++)
		{
			delete found[i];
			delete len[i];
		}

		thread_drop_heap(THREAD_NUMS_5);//删表

		THREAD_RID=THREAD_RID+THREAD_NUMS_5;
		INDEX_ID=INDEX_ID+THREAD_NUMS_5;
	}
	catch(StorageEngineExceptionUniversal& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		user_abort_transaction();
	}
	return TRUE;
}

#define THREAD_NUMS_10 1
bool test_thread_index_update_02()
{
	++INDEX_ID;
	INTENT("测试启动多个线程分别更新表,这些表是多列的，每个线程独享数据。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	int i=0;
	SpliterGenerater sg;
	const int COL_NUM = 3;

	try
	{
		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,5,3);//创建表的colinfo
		int rid=THREAD_RID;
		//setColInfo(THREAD_RID,heap_info);
		thread_create_heap_xy(THREAD_NUMS_10,rid,heap_info);//建表

		const unsigned int data_row = 10;
		const unsigned int data_len = 20;
		DataGenerater dg(data_row, data_len);
		dg.dataGenerate();

		char data[][DATA_LEN] = {"aqwevadrghj", "bvzweuhnbvx"};//初始化插入表的数据
		char predict_data[][DATA_LEN] = {"abcevadrghj", "bvzabcdebvx"};//预计更新后的表中的数据
		int array_size=sizeof("aqwevadrghj");

		for (rid=THREAD_RID;rid<THREAD_RID+THREAD_NUMS_10;rid++)
		{
			thread_insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN,rid);//向表中插入数据
		}

		const int INDEX_COL_NUM = 3;
		int col_number[INDEX_COL_NUM] = {1,2,3};
		CompareCallback cmp_func[INDEX_COL_NUM] = 
		{
			my_compare_str,
			my_compare_str,
			my_compare_str
		};
		Colinfo index_info = sg.buildIndexColInfo(3, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2,3>);//构建index的colinfo
		int index_id=INDEX_ID;
		setColInfo(index_id,index_info);
		for (rid=THREAD_RID;rid<THREAD_RID+THREAD_NUMS_10;rid++,index_id++)
		{
			thread_create_index(rid, heap_info,index_info,index_id);//启动线程建索引
		}

		boost::thread_group tg;
		BackendParameters *paramArr[THREAD_NUMS_10];	
		prepare_param(paramArr,THREAD_NUMS_10);
		index_id=INDEX_ID;
		rid=THREAD_RID;
		for(i = 0;i < THREAD_NUMS_10;i++,index_id++,rid++)
		{
			tg.create_thread(boost::bind(&thread_index_update_02,(void*)paramArr[i],rid,heap_info,index_info,index_id));//启动线程更新表
		}
		tg.join_all();
		free_param(paramArr,THREAD_NUMS_10);

		IndexScanInfo isi[THREAD_NUMS_10];
		for(i = 0; i < THREAD_NUMS_10; i++,index_id++,rid++)
		{
		alloc_scan_space(INDEX_COL_NUM, isi[i]);
		isi[i].cmp_values[0] = fdxdb_string_formdatum("abc", strlen("abc"));//初始化scankey
		isi[i].cmp_values[1] = fdxdb_string_formdatum("abcde", strlen("abcde"));
		isi[i].cmp_values[2] = fdxdb_string_formdatum("abc", strlen("abc"));
		isi[i].cmp_func[0] = my_compare_str;
		isi[i].cmp_func[1] = my_compare_str;
		isi[i].cmp_func[2] = my_compare_str;
		isi[i].cmp_strategy[0] = BTGreaterEqualStrategyNumber;
		isi[i].cmp_strategy[1] = BTGreaterEqualStrategyNumber;
		isi[i].cmp_strategy[2] = BTGreaterEqualStrategyNumber;
		isi[i].col_array[0] = 1;
		isi[i].col_array[1] = 2;
		isi[i].col_array[2] = 3;
		init_scan_key(isi[i]);
		}
		/*
		* 创建线程利用索引查询数据
		*/
		PREPARE_PARAM(BackendParameters *);

		char found_array[data_row][DATA_LEN];

		index_id=INDEX_ID;
		rid=THREAD_RID;
		DataGenerater *found[THREAD_NUMS_10] ;
		unsigned int *len[THREAD_NUMS_10]={0};


		for(i = 0; i < THREAD_NUMS_10; i++,index_id++,rid++)
		{
			DataGenerater *found_dg = new DataGenerater(10, DATA_LEN);
			unsigned int *array_len =new unsigned int(0);
			BackendParameters *param = get_param();
			SAVE_PARAM(param);
			tg.create_thread(bind(&thread_index_scan, param, index_id, index_info, rid, heap_info, &isi[i], found_dg, array_len));
			found[i]=found_dg;
			len[i]=array_len;
		}
		tg.join_all();
		FREE_PARAM(BackendParameters *);
//		free_scan_key(isi);

		//生成二维字符串数组
		for (i=0;i<THREAD_NUMS_10;i++)
		{
			found[i]->dataToDataArray2D(*len[i], found_array);
			//检查索引查找出来的结果都在预测的表中
			bool sta = check_array_equal(predict_data, found_array, array_size,ARRAY_LEN_CALC(data));
			assert(sta == TRUE);
		}

		for (i=0;i<THREAD_NUMS_10;i++)
		{
			delete found[i];
			delete len[i];
		}

		rid=THREAD_RID;
		for(int i = 0; i < THREAD_NUMS_10; i++,rid++,index_id++)
		{
			thread_remove_index(rid,INDEX_ID);//删索引
		}
		thread_drop_heap(THREAD_NUMS_10);//删表
		THREAD_RID=THREAD_RID+THREAD_NUMS_10;
		INDEX_ID=INDEX_ID+THREAD_NUMS_10;
	}
	catch(StorageEngineExceptionUniversal& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		user_abort_transaction();
	}
	return TRUE;
}

#define THREAD_NUMS_1 1
bool test_thread_index_update_03()
{
	++INDEX_ID;
	INTENT("测试在一个表中插入数据，启动多个线程利用索引更新这个表,然后利用索引查询更新后表的结果。这些表是多列的。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	int i=0;
	SpliterGenerater sg;
	const int COL_NUM = 5;

	try
	{
		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2,3,4,5);//创建表的colinfo
		int rid=THREAD_RID;
		int index_id=INDEX_ID;
		thread_create_heap_xy(THREAD_NUMS_1,rid,heap_info);//建表

		const unsigned int data_row = 10;
		const unsigned int data_len = 20;
		DataGenerater dg(data_row, data_len);
		dg.dataGenerate();

		char data[][DATA_LEN] = {"aqwevadrghjvcdefs", "bvzweuhnbvxzehbvf","efdcsefdsafebghjy"};//初始化插入表的数据
		char predict_data[][DATA_LEN] = {"abcdstscvczcvrefs", "jjwesbhjjdefgehbvf","nujhttegxswrdegsq"};//预计更新后的表中的数据
		int array_size=sizeof("aqwevadrghjvcdefs");

		thread_insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN,rid);//向表中插入数据

		const int INDEX_COL_NUM = 4;
		int col_number[INDEX_COL_NUM] = {1,2,3,4};
		CompareCallback cmp_func[INDEX_COL_NUM] = 
		{
			my_compare_str,
			my_compare_str,
			my_compare_str,
			my_compare_str
		};
		Colinfo index_info = sg.buildIndexColInfo(4, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2,3,4>);//构建index的colinfo
		setColInfo(index_id,index_info);
		thread_create_index(rid, heap_info,index_info,index_id);//启动线程建索引
		boost::thread_group tg;
		BackendParameters *paramArr[THREAD_NUMS_1];	
		prepare_param(paramArr,THREAD_NUMS_1);

		tg.create_thread(boost::bind(&thread_index_update_0301,(void*)paramArr[i],rid,heap_info,index_info,index_id));//启动线程更新表
		tg.create_thread(boost::bind(&thread_index_update_0302,(void*)paramArr[i],rid,heap_info,index_info,index_id));
		tg.create_thread(boost::bind(&thread_index_update_0303,(void*)paramArr[i],rid,heap_info,index_info,index_id));

		tg.join_all();
		free_param(paramArr,THREAD_NUMS_1);

		IndexScanInfo isi;
		alloc_scan_space(INDEX_COL_NUM, isi);
		isi.cmp_values[0] = fdxdb_string_formdatum("abc", strlen("abc"));//初始化scankey
		isi.cmp_values[1] = fdxdb_string_formdatum("ds", strlen("aa"));
		isi.cmp_values[2] = fdxdb_string_formdatum("abc", strlen("abc"));
		isi.cmp_values[3] = fdxdb_string_formdatum("abcd", strlen("abcd"));
		isi.cmp_func[0] = my_compare_str;
		isi.cmp_func[1] = my_compare_str;
		isi.cmp_func[2] = my_compare_str;
		isi.cmp_func[3] = my_compare_str;
		isi.cmp_strategy[0] = BTGreaterEqualStrategyNumber;
		isi.cmp_strategy[1] = BTGreaterEqualStrategyNumber;
		isi.cmp_strategy[2] = BTGreaterEqualStrategyNumber;
		isi.cmp_strategy[3] = BTGreaterEqualStrategyNumber;
		isi.col_array[0] = 1;
		isi.col_array[1] = 2;
		isi.col_array[2] = 3;
		isi.col_array[3] = 4;
		init_scan_key(isi);

		/*
		* 创建线程利用索引查询数据
		*/
		PREPARE_PARAM(BackendParameters *);

		char found_array[data_row][DATA_LEN];

		DataGenerater found_dg(10, DATA_LEN);
		unsigned int array_len=0;
		BackendParameters *param = get_param();
		SAVE_PARAM(param);
		tg.create_thread(bind(&thread_index_scan, param, index_id, index_info, rid, heap_info, &isi, &found_dg, &array_len));

		tg.join_all();
		FREE_PARAM(BackendParameters *);
		free_scan_key(isi);

		//生成二维字符串数组
		found_dg.dataToDataArray2D(array_len, found_array);
		//检查索引查找出来的结果都在预测的表中
		bool sta = check_array_equal(predict_data, found_array, array_size,ARRAY_LEN_CALC(data));
		assert(sta == TRUE);

		thread_drop_heap(THREAD_NUMS_1);//删表

		THREAD_RID=THREAD_RID+THREAD_NUMS_1;
		INDEX_ID=INDEX_ID+THREAD_NUMS_1;
	}
	catch(StorageEngineExceptionUniversal& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		user_abort_transaction();
	}
	return TRUE;
}

void thread_index_update_0301(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id)
{
	INTENT("更新单列表");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	/*
	* 构建更新数据
	*/
	char *thread1_update_src_1 =  "aqwevadrghjvcdefs"; 
	char *thread1_update_det_1 =   "abcdstscvczcvrefs"; 
	fxdb_SubPostmaster_Main(param);
	SAVE_INFO_FOR_DEBUG();
	Relation rel = NULL;
	begin_transaction();
	int count = 0;
	int found[DATA_LEN];
	int i = 0; 
	Datum		values[1];
	bool		isnull[1];

	try
	{
		Relation indrel1 = NULL;
		rel = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);
		ItemPointerData ipd1 = findTuple(thread1_update_src_1, rel, found[1], count);//找到src

		HeapTuple tuple1 = fdxdb_heap_formtuple(thread1_update_det_1, sizeof("abcdstscvczcvrefs"));
		FDPG_Heap::fd_simple_heap_update(rel, &ipd1, tuple1);//更新表

		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		CommandCounterIncrement();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		found[i] = -1;
	}

	commit_transaction();
	proc_exit(0);
}

void thread_index_update_0302(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id)
{
	INTENT("更新单列表");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	/*
	* 构建更新数据
	*/
	char *thread1_update_src_1 =  "bvzweuhnbvxzehbvf"; 
	char *thread1_update_det_1 =  "jjwesbhjjdefgehbvf"; 
	fxdb_SubPostmaster_Main(param);
	SAVE_INFO_FOR_DEBUG();
	Relation rel = NULL;
	begin_transaction();
	int count = 0;
	int found[DATA_LEN];
	int i = 0; 
	Datum		values[1];
	bool		isnull[1];

	try
	{
		Relation indrel1 = NULL;
		rel = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);
		ItemPointerData ipd1 = findTuple(thread1_update_src_1, rel, found[1], count);//找到src

		HeapTuple tuple1 = fdxdb_heap_formtuple(thread1_update_det_1, sizeof("bvzweuhnbvxzehbvf"));
		FDPG_Heap::fd_simple_heap_update(rel, &ipd1, tuple1);//更新表
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		CommandCounterIncrement();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		found[i] = -1;
	}

	commit_transaction();
	proc_exit(0);
}

void thread_index_update_0303(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id)
{
	INTENT("更新单列表");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;

	/*
	* 构建更新数据
	*/
	char *thread1_update_src_1 =  "efdcsefdsafebghjy"; 
	char *thread1_update_det_1 =   "nujhttegxswrdegsq"; 
	fxdb_SubPostmaster_Main(param);
	SAVE_INFO_FOR_DEBUG();
	Relation rel = NULL;
	begin_transaction();
	int count = 0;
	int found[DATA_LEN];
	int i = 0; 
	Datum		values[1];
	bool		isnull[1];

	try
	{
		Relation indrel1 = NULL;
		rel = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);
		ItemPointerData ipd1 = findTuple(thread1_update_src_1, rel, found[1], count);//找到src

		HeapTuple tuple1 = fdxdb_heap_formtuple(thread1_update_det_1, sizeof("efdcsefdsafebghjy"));
		FDPG_Heap::fd_simple_heap_update(rel, &ipd1, tuple1);//更新表
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		CommandCounterIncrement();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		found[i] = -1;
	}

	commit_transaction();
	proc_exit(0);
}


void thread_index_update_01(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id)
{
	INTENT("更新单列表");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;

	/*
	* 构建更新数据
	*/
	char thread1_update_src_1[] =  "a"; 
	char thread1_update_det_1[] =   "c"; 
	char thread1_update_src_2[] =  "b"; 
	char thread1_update_det_2[] =   "d"; 
	fxdb_SubPostmaster_Main(param);
	SAVE_INFO_FOR_DEBUG();
	Relation rel = NULL;
	begin_transaction();
	int count = 0;
	int found[DATA_LEN];
	int i = 0; 
	Datum		values[1];
	bool		isnull[1];

	try
	{
		Relation indrel1 = NULL, indrel2 = NULL;
		rel = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);
		ItemPointerData ipd1 = findTuple(thread1_update_src_1, rel, found[1], count);//找到src
		ItemPointerData ipd2 = findTuple(thread1_update_src_2, rel, found[2], count);

		HeapTuple tuple1 = fdxdb_heap_formtuple(thread1_update_det_1, sizeof(thread1_update_det_1));
		FDPG_Heap::fd_simple_heap_update(rel, &ipd1, tuple1);

		HeapTuple tuple2 = fdxdb_heap_formtuple(thread1_update_det_2, sizeof(thread1_update_det_2));
		FDPG_Heap::fd_simple_heap_update(rel, &ipd2, tuple2);

		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		CommandCounterIncrement();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		found[i] = -1;
	}

	commit_transaction();
	proc_exit(0);
}

void thread_index_update_02(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id)
{
	INTENT("更新多列表");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;

	/*
	* 构建更新数据
	*/
	char thread1_update_src_1[] =  "aqwevadrghj"; 
	char thread1_update_det_1[] =  "abcevadrghj";  
	char thread1_update_src_2[] =  "bvzweuhnbvx"; 
	char thread1_update_det_2[] =  "bvzabcdebvx";  

	fxdb_SubPostmaster_Main(param);
	SAVE_INFO_FOR_DEBUG();
	Relation rel = NULL;
	begin_transaction();
	int count = 0;
	int found[DATA_LEN];
	int i = 0; 
	Datum		values[1];
	bool		isnull[1];

	try
	{
		Relation indrel1 = NULL, indrel2 = NULL;
		rel = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);
		ItemPointerData ipd1 = findTuple(thread1_update_src_1, rel, found[1], count);//找到src
		ItemPointerData ipd2 = findTuple(thread1_update_src_2, rel, found[2], count);

		HeapTuple tuple1 = fdxdb_heap_formtuple(thread1_update_det_1, sizeof(thread1_update_det_1));
		FDPG_Heap::fd_simple_heap_update(rel, &ipd1, tuple1);//更新表

		HeapTuple tuple2 = fdxdb_heap_formtuple(thread1_update_det_2, sizeof(thread1_update_det_2));
		FDPG_Heap::fd_simple_heap_update(rel, &ipd2, tuple2);

		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		CommandCounterIncrement();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		found[i] = -1;
	}

	commit_transaction();
	proc_exit(0);
}

