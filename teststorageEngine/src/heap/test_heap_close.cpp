#include <boost/thread/mutex.hpp> 
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "utils/fmgroids.h"

#include "utils/util.h"
#include "test_fram.h"
#include "heap/test_heap_close.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/utils.h"
#include "DataItem.h"
using namespace std;
using namespace FounderXDB::StorageEngineNS;

bool test_heap_close()
{

	INTENT("测试heap_close能否关闭heap。")
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

	try
	{	
		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Relation testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock, MyDatabaseId);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		Oid res;
		HeapTuple tup;
		tup = FDPG_Heap::fd_heap_form_tuple("testtesttest", 10);
		res = FDPG_Heap::fd_simple_heap_insert(testRelation , tup);
		CHECK_BOOL(res == NULL);
		if(res != NULL)
		{
			printf("插入了异常数据!\n");
			user_abort_transaction();
			return false;
		}
		commit_transaction();

		remove_heap(THREAD_RID);//删表
	}
	catch (StorageEngineExceptionUniversal &ex) {
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		user_abort_transaction();
	}
	return true;
}

bool test_heap_close_WrongLock()
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
		INTENT("测试heap_close中锁的参数不一致能否关闭heap。");
		begin_transaction();//带RowExclusiveLock的FDPG_Heap::fd_heap_open检测
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Relation testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		printf("open与close的锁类型不一致报警告:\n");
		FDPG_Heap::fd_heap_close(testRelation, ShareLock);
		Oid res;
		HeapTuple tup;
		tup = FDPG_Heap::fd_heap_form_tuple("testtesttest", 10);
		res = FDPG_Heap::fd_simple_heap_insert(testRelation , tup);
		CHECK_BOOL(res == NULL);
		if(res != NULL)
		{
			printf("插入了异常数据!\n");
			user_abort_transaction();
			return false;
		}
		commit_transaction();

		remove_heap(THREAD_RID);//删表
	}
	catch (StorageEngineExceptionUniversal &ex) {
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		user_abort_transaction();
	}
	return true;
}

int sta;
#define THREAD_NUMS_10 10
bool test_thread_heap_close()
{
	INTENT("创建多个线程，打开表然后关闭");
	++THREAD_RID;
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);//这里可以随便设置,因为只是简单插入数据，没用到colinfo
	thread_create_heap_xy(THREAD_NUMS_10 ,rid,heap_info);
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_10];	
	prepare_param(paramArr,THREAD_NUMS_10);
	sta=0;
	//启动线程
	for(int i = 0;i < THREAD_NUMS_10;i++)
	{
		tg.create_thread(boost::bind(&thread_simple_heap_close,(void*)paramArr[i],rid+i,sta));
	}

	tg.join_all();

	free_param(paramArr,THREAD_NUMS_10);
	thread_drop_heap(THREAD_NUMS_10);
	CHECK_BOOL(sta==0);
	if (sta==1)
	{
		return FALSE;
	}
	return true;
}

void thread_simple_heap_close(void *param,Oid rid,int sta)
{
	fxdb_SubPostmaster_Main(param);
	INTENT("通过验证已经关闭的表中是否能够插入数据验证表的关闭。");
	try
	{
		SAVE_INFO_FOR_DEBUG();
		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = rid;
		Relation testRelation;

		//open heap and insert a tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		Oid res;
		HeapTuple tup;
		tup = FDPG_Heap::fd_heap_form_tuple("testtesttest", 10);
		res = FDPG_Heap::fd_simple_heap_insert(testRelation , tup);		
		if(res != NULL)
		{
			printf("插入了异常数据!\n");
			user_abort_transaction();
			sta=1;
		}
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;

	}
	proc_exit(0);
}

