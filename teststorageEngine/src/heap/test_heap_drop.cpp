/*
测试heap_drop采用多进程的方法，创建一个子进程，在test1中startEngine然后heap_create 
，heap_drop 再stopEngine,同时结束了该子进程。父进程中，在test2中重启startEngine，利
用heap_open测打开不存在的heap测heap_drop成功。
*/
#include "boost/thread/thread.hpp"

#include <unistd.h>
#include "postgres.h"
#include "storage/smgr.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "interface/FDPGAdapter.h"

#include "utils/util.h"
#include "test_fram.h"
#include "heap/test_heap_drop.h"
#include "iostream"
#include "interface/StorageEngineExceptionUniversal.h"
using namespace std;
using namespace FounderXDB::StorageEngineNS;

bool test_create_drop_heap()
{	
	INTENT("测试heap_create与heap_drop的功能是否完善。");
	try
	{
	printf("begin test_create_drop_heap...\n");
	begin_first_transaction();
 	Oid relid = 2384277;
 	Oid reltablespace = DEFAULTTABLESPACE_OID;
	printf("Hit for create process...\n");
 	FDPG_Heap::fd_heap_create(reltablespace, relid);
 	FDPG_Transaction::fd_CommandCounterIncrement();   
    FDPG_Heap::fd_heap_drop(relid, MyDatabaseId);
	printf("Hit for drop success...\n");
	commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) {
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		user_abort_transaction();
	}
	return true;
}

bool test_heap_drop()
 {
	printf("begin test_heap_drop...\n");
	INTENT("测试heap_drop能否删除heap。");
	try
	{
	Oid relid = 2384277;
	Oid reltablespace = DEFAULTTABLESPACE_OID;

	begin_first_transaction();
	FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
	commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) {
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		user_abort_transaction();
	}
	return true;
}

bool heap_drop_OpenWithoutClose()
 {
 
	try
	{
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 3;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//创建表的colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//建表
		Oid relid= THREAD_RID;
 		INTENT("测试FDPG_Heap::fd_heap_open能否多次打开关闭heap。");
 		begin_transaction();
 		Oid reltablespace = DEFAULTTABLESPACE_OID;
 
 		FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		printf("没有关闭heap的情况下drop该heap报警告:\n");
 		commit_transaction();
		remove_heap(THREAD_RID);//删表
 	}
	catch (StorageEngineExceptionUniversal &ex) {
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		user_abort_transaction();
		return false;
	}
 	return true;
}

#define THREAD_NUM_10 10
#define THREAD_NUM_5 5
#define THREAD_NUM_3 3

extern void thread_find(const char [][DATA_LEN],
												const Oid,
												const int,
												const BackendParameters *,
												int32 *);

void thread_insert_n(const char data[][DATA_LEN], 
										 const int array_len, 
										 const int data_len, 
										 const Oid rel_id, 
										 BackendParameters *GET_PARAM(), 
										 int *sta);

extern int RELID;

void thread_heap_drop(int rel_id, BackendParameters *GET_PARAM(), int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	*sta = 0;
	try
	{
		remove_heap(rel_id);
		++RELID;
		*sta = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		*sta = -1;
	}
	proc_exit(0);
}

void thread_heap_drop_when_ready(int rel_id, BackendParameters *GET_PARAM(), int *sta, int *ready)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	*sta = 0;
	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_drop(rel_id, MyDatabaseId);
		++rel_id;
		while(*ready != 1);
		commit_transaction();
		*sta = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		*sta = -1;
	}
}

bool check_pending_table(Oid rel_id)
{
	SMgrRelation reln = (SMgrRelation)palloc0(sizeof(SMgrRelationData));
	reln->smgr_rnode.node.relNode = rel_id;
	reln->smgr_rnode.node.dbNode = MyDatabaseId;
	reln->smgr_rnode.node.spcNode = (MyDatabaseTableSpace == InvalidOid ? DEFAULTTABLESPACE_OID : MyDatabaseTableSpace);
	reln->smgr_rnode.backend = InvalidBackendId;
	return mdexists(reln , MAIN_FORKNUM);
}

EXTERN_SIMPLE_FUNCTION

int test_thread_heap_drop_000()
{
	INTENT("启动多个线程去删除同一张表，测试的正确结果"
				 "是全部的线程都删除成功。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	Colinfo heap_info = NULL;
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(RELID, create_sta);
	}

	int sta[THREAD_NUM_10];
	for(int i = 0; i < THREAD_NUM_10; ++i)
	{
		BackendParameters *GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_heap_drop, RELID, GET_PARAM(), &sta[i]));
	}
	tg.join_all();
	FREE_PARAM(BackendParameters *);

	int count = 0;
	for(int i = 0; i < THREAD_NUM_10; ++i)
	{
		if(sta[i] == 1)
		{
			++count;
		}
	}

	if(count != THREAD_NUM_10)
	{
		return 0;
	}
	return 1;
}

int test_thread_heap_drop_001()
{
	INTENT("创建多个线程，其中部分线程对表做查询操作，部分"
				 "线程对表做删除操作。所有线程操作同一张表。测试"
				 "的正确结果是查询线程是否会找到数据依赖于是否先"
				 "于删除线程锁住表。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	Colinfo heap_info = NULL;
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(RELID, create_sta);
	}

	char data[][DATA_LEN] = 
	{
		"test_data1"
	};

	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(RELID, data, DATA_LEN, insert_sta);
	}

	/*
	*启动删除和查询线程
	*/

	char find_data[][DATA_LEN] = {"test_data1"};
	int search_sta[] = {0};
	for(int i = 0; i < THREAD_NUM_3; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_find, find_data, RELID, ARRAY_LEN_CALC(find_data), GET_PARAM(), search_sta));
	}

	int drop_sta[THREAD_NUM_3] = {0};
	for(int i = 0; i < THREAD_NUM_3; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_heap_drop, RELID, GET_PARAM(), &drop_sta[i]));
	}

	tg.join_all();
	FREE_PARAM(BackendParameters *);

	//测试成功
	if(drop_sta[0] == 1 && drop_sta[0] == 1 && drop_sta[0] == 1)
	{
		return 1;
	}
	return 0;
}

int test_thread_heap_drop_002()
{
	INTENT("创建多个线程，其中部分线程做drop table操作，部分"
				 "线程做插入操作。本测例成功的假设是如果插入线程先"
				 "于drop table线程执行，那么插入操作将会完整执行，"
				 "否者会报异常。(本测例操作三个表，所有的drop操作和"
				 "insert操作都是成对操作同一张表）");

	using namespace boost;
	using namespace FounderXDB::StorageEngineNS;
	PREPARE_TEST();

	clear_heap_id();
	int rel_id[THREAD_NUM_3];
	for(int i = 0; i < THREAD_NUM_3; ++i)
	{
		rel_id[i] = get_heap_id();
	};

	{
		int create_sta = 0;
		Colinfo heap_info = NULL;
		for(int i = 0; i < THREAD_NUM_3; ++i)
		{
			SIMPLE_CREATE_HEAP(rel_id[i], create_sta);
		};
	}

	int sta[THREAD_NUM_3];
	int drop_sta[THREAD_NUM_3];
	{
		/*
		* 创建多个drop线程
		*/
		for(int i = 0; i < THREAD_NUM_3; ++i)
		{
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			tg.create_thread(bind(&thread_heap_drop, rel_id[i], GET_PARAM(), &drop_sta[i]));
		}

		/*
		* 创建多个个插入线程
		*/
#define ROW 20
		const int LEN = 50;
		for(int i = 0; i < THREAD_NUM_3; ++i)
		{
			DataGenerater dg(ROW, LEN);
			char rel_data[ROW][DATA_LEN];
			dg.dataToDataArray2D(rel_data);
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			tg.create_thread(bind(&thread_insert_n, rel_data, ARRAY_LEN_CALC(rel_data), DATA_LEN, rel_id[i], GET_PARAM(), &sta[i]));
		}

		tg.join_all();
		FREE_PARAM(BackendParameters *);
	}

	/*
	* 检测结果。正确结果应该是插入线程如果先于删除线程获取到
	* 表的锁，那么插入线程会阻塞删除线程的执行，直到插入线程
	* 将所有数据都插入成功后才会执行删除操作，否者heap将是空
	* 的，没有任何数据就被删除了。
	*/
	int test_success = true;
	for(int i = 0; i < THREAD_NUM_3; ++i)
	{
		if(!(sta[i] == 0 || sta[i] == ROW))
		{
			SAVE_INFO_FOR_DEBUG();
			test_success = false;
		}
		if(test_success == false)
		{
			break;
		}
	}

	/*
	* 打开表扫描，此时表已经被删除
	*/
	if(!(drop_sta[0] == 1 && drop_sta[1] == 1 && drop_sta[2] == 1))
	{
		test_success = false;
	}
#undef ROW
	return test_success;
}