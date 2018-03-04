#include "boost/thread/thread.hpp"

#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "postmaster/postmaster.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "utils/util.h"
#include "heap/test_delete.h"
#include "test_fram.h"

#ifndef WIN32
#include <pthread.h>
#endif //WIN32

int RELID = 10000;
int INDEX_ID = 200000;

//屏幕打印当前表内容
void printfHeapTuple(Relation rel)
{
	CHECK_BOOL(rel != NULL);
	if(rel != NULL)
	{
		HeapScanDesc scan;
		HeapTuple tuple;
		char *data;
		SAVE_INFO_FOR_DEBUG();
		scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			data = fxdb_tuple_to_chars(tuple);
			printf("\t%s\n", data);
		}
		FDPG_Heap::fd_heap_endscan(scan);
	}
}
extern int stop_engine_();
extern int start_engine_();

//删除测试用例创建的表
void endTestNotStopEngine()
{
	using namespace FounderXDB::StorageEngineNS;
	try
	{
		begin_transaction();
		Oid relid = RELID;
		FDPG_Heap::fd_heap_drop(relid, MyDatabaseId);
		commit_transaction();
		++RELID;
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		std::cout << se.getErrorMsg() << std::endl;
		std::cout << se.getErrorNo() << std::endl;
		throw;
	}
}

//创建一张表,插入若干元组
void beginTestNotStartEngine()
{
	try
	{
		begin_first_transaction();
		Relation rel;
		++RELID;
		Oid relid = RELID;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		extern Colinfo heap_cf;
		extern void setColInfo(Oid colid, Colinfo pcol_info);
		extern void initHeapAndIndexColinfo();
		initHeapAndIndexColinfo();
		setColInfo(relid, heap_cf);
		FDPG_Heap::fd_heap_create(relspace, RELID, 0, relid);
		CommandCounterIncrement();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		CHECK_BOOL(rel != NULL);
		HeapTuple tuple;
		char data[20][20] = {"testdata_1", "testdata_2", "testdata_3", "testdata_1",\
			"testdata_2", "testdata_1", "apple", "reboot", "apple"};
		for(int i = 0; i < 9; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(data[i], 20);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
		}
		CommandCounterIncrement();
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
	}
	catch(...)
	{
		throw;
	}
}

//创建一张表,插入若干元组
void beginTest()
{
	try
	{
		start_engine_();
		begin_first_transaction();
		Relation rel = NULL;
		Oid relid = RELID;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_heap_create(relspace, relid);
		CommandCounterIncrement();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		CHECK_BOOL(rel != NULL);
		HeapTuple tuple;
		char data[20][20] = {"testdata_1", "testdata_2", "testdata_3", "testdata_1",\
												 "testdata_2", "testdata_1", "apple", "reboot", "apple"};
		getchar();
		for(int i = 0; i < 9; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(data[i], 20);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
		}
		CommandCounterIncrement();
		printf("\t===表初始数据===\n");
		printfHeapTuple(rel);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);		
		commit_transaction();
	}
	catch(...)
	{
		PG_RE_THROW();
	}
	
}

//删除测试用例创建的表
void endTest()
{
	try
	{
		AbortOutOfAnyTransaction();
		StartTransactionCommand();
		BeginTransactionBlock();
		CommitTransactionCommand();
		Oid relid = RELID;
		FDPG_Heap::fd_heap_drop(relid, MyDatabaseId);
		StartTransactionCommand();
		EndTransactionBlock();
		CommitTransactionCommand();
		++RELID;
		getchar();
		stop_engine_();
	}
	catch(...)
	{
		PG_RE_THROW();
	}
	
}

//测试删除数据表中第一个元组
int testDeleteFirstTupleFromHeap()
{
	INTENT("插入一个单列元组，测试删除数据表中第一个元组,"
		"即删除插入的单列元组。屏幕打印删除前第一个元组的数据"
		"如果删除失败，测试失败，函数返回0，屏幕打印相关信息。");

	using namespace FounderXDB::StorageEngineNS;
	try
	{
		begin_transaction();

		HeapScanDesc scan;
		HeapTuple tuple;
		ItemPointer tid;
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = RELID;
		Relation testRelation;

		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		//扫描表的第一个元组，并删除掉
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);

		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple != NULL);
		//如果表中没有元组则关闭扫描并退出测试
		if(tuple == NULL)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			return 0;
		}
		tid = &(tuple->t_self);
		FDPG_Heap::fd_simple_heap_delete(testRelation, tid);
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);

		CommandCounterIncrement();

		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		//重新扫描表，寻找第一个元组是否还存在
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		//表中原先有多条数据，所以当前表应该还是不为空
		CHECK_BOOL(NULL != tuple);
		if(NULL == tuple)
		{
			printf("\tthere is no more tuple!\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			return 0;
		}
		char *first_tuple = fxdb_tuple_to_chars(tuple);
		//如果第一个元组不再是预先插入的testdata_1，则说明测试成功
		if (strcmp(first_tuple, "testdata_1") == 0) {
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			return 0;
		}
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);

		//delete relation
		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		return 0;
	}
	return 1;
}

//插入多个不同元组，测试删除其中一个元组
int testDeleteOneTupleFromManyTuple()
{
	INTENT("插入多个不同元组，测试删除其中一个元组。"
		"根据给定的数据扫描表，查找到相应数据然后删除。"
		"如果找不到要删除的数据，则可能是扫描失败，或则是插入失败。如果"
		"找到该元组则删除。屏幕打印删除前表内容，打印删除后表内容。");

	using namespace FounderXDB::StorageEngineNS;
	try
	{
		begin_transaction();
		HeapScanDesc scan;
		HeapTuple tuple;
		ItemPointer tid;
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = RELID;
		Relation testRelation;

		char *tmp_tuple;
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		int status = 0;
		//扫描表，找到要删除的元组
		while (true)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			if(tuple == NULL)
			{
				break;
			}
			SAVE_INFO_FOR_DEBUG();
			tmp_tuple = fxdb_tuple_to_chars(tuple);
			//这里要删除的元组是数据为"testdata_3"的元组
			if(strcmp(tmp_tuple, "testdata_3") == 0)
			{
				tid = &(tuple->t_self);
				SAVE_INFO_FOR_DEBUG();
				FDPG_Heap::fd_simple_heap_delete(testRelation, tid);
				status = 1;
				break;
			}
		}
		if(!status)
		{
			printf("\t找不到要删除的元组。\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			return 0;
		}
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);

		CommandCounterIncrement();

		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		status = 0;
		//重新扫描表，查看是否还存在"testdata_3"
		while (true)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			if (tuple == NULL) {
				break;
			}
			tmp_tuple = fxdb_tuple_to_chars(tuple);
			//找到唯一的"testdata_3"还存在，说明测试失败
			if(strcmp(tmp_tuple, "testdata_3") == 0)
			{
				status = 1;
				break;
			}
		}
		if(status == 1)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			return 0;
		}
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);

		//delete relation
		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		throw;
	}
	
	return 1;
}

//插入多个相同元组，测试删除其中一个元组
int testDeleteOneTupleFromTheSameTuple()
{
	INTENT("插入多个相同元组，测试删除其中一个元组。"
		"根据给定的数据扫描表，查找到相应数据然后删除。"
		"如果找不到要删除的数据，则可能是扫描失败，或则是插入失败。如果"
		"找到该元组则删除。屏幕打印删除前表内容，打印删除后表内容。多个"
		"相同元组同时存在则删除的应该是表的第一个元组。");

	using namespace FounderXDB::StorageEngineNS;
	try
	{
		begin_transaction();
		HeapScanDesc scan;
		HeapTuple tuple;
		ItemPointer tid;
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = RELID;
		Relation testRelation;

		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		int status = 0;
		char *tmp_tuple;
		//扫描表，找到元组"testdata_2"删除
		while (true)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			//表中应该存在有testdata_2，如果没有，说明测试失败
			if(tuple == NULL)
			{
				break;
			}
			SAVE_INFO_FOR_DEBUG();
			tmp_tuple=fxdb_tuple_to_chars(tuple);
			//找到了testdata_2，删除
			if(strcmp(tmp_tuple, "testdata_2") == 0)
			{
				tid = &(tuple->t_self);
				SAVE_INFO_FOR_DEBUG();
				FDPG_Heap::fd_simple_heap_delete(testRelation, tid);
				status = 1;
				break;
			}
		}
		//找不到testdata_2
		if(!status)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			return 0;
		}
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);

		CommandCounterIncrement();

		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		status = 0;
		while (true)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			//表中应该有多行数据，所以能找到元组
			if (tuple == NULL) {
				break;
			}
			tmp_tuple = fxdb_tuple_to_chars(tuple);
			//计数剩下的testdata_2
			if(strcmp(tmp_tuple, "testdata_2") == 0)
			{
				++status;
			}
		}
		//表中原来存在有两个testdata_2，现在应该还剩下一个
		if(status != 1)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			return 0;
		}
		
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		//delete relation
		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		throw;
	}
	
	return 1;
}

//测试由给定的步数删除一个元组
int testDeleteFromGiveSteps()
{
	INTENT("测试根据给定的步数删除一个元组，屏幕打印删除前数据，打印删除后数据。");

	using namespace FounderXDB::StorageEngineNS;
	try
	{
		begin_transaction();
		//给定步数为4，即删除第四个插入的元组
		int step = 4;
		Oid relid = RELID;
		Relation rel;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		HeapScanDesc scan;
		HeapTuple tuple;
		scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		//扫描到表的第四行
		while(step > 1)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			CHECK_BOOL(tuple != NULL);
			if(tuple == NULL)
			{
				printf("已到达表的末尾!\n");
				return 0;
			}
			--step;
		}
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple != NULL);
		//表中的第四行应该不是表的结尾
		if(tuple == NULL)
		{
			return 0;
		}
		ItemPointer ip = &(tuple->t_self);
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_delete(rel, ip);
		FDPG_Heap::fd_heap_endscan(scan);
		CommandCounterIncrement();

		scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		//重新扫描至表第四个元组
		while(step <= 4)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			++step;
		}
		//当前tuple不为空
		SAVE_INFO_FOR_DEBUG();
		if(tuple == NULL)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			return 0;
		}
		char *tmp_tuple = fxdb_tuple_to_chars(tuple);
		FDPG_Heap::fd_heap_endscan(scan);
		//当前第四个元组应该不是testdata_1
		SAVE_INFO_FOR_DEBUG();
		if(strcmp(tmp_tuple, "testdata_1") == 0)
		{
			user_abort_transaction();
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			return 0;
		}
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		throw;
	}
	
	return 1;
}

//测试删除数据表中不存在的块中的元组
int testDeleteNoExistTuple()
{
	INTENT("测试删除表中并不存在的元组，PG是否会正确汇报信息。"
		"可以先删除表中所有的数据再去删除，也可以建一个空表"
		"去删除，也可以删除那么超出表元组集合的元组(即不存在"
		"的元组)。不管哪种，效果都一样。");
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Relation rel;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,AccessShareLock, MyDatabaseId);
		CHECK_BOOL(rel != NULL);
		//不存在的表
		if(rel == NULL)
		{
			return 0;
		}
		//构造一个随机的ItemPointerData
		ItemPointerData ip;
		ip.ip_blkid.bi_hi = 65535;
		ip.ip_blkid.bi_lo = 65535;
		ip.ip_posid = 65535;
		//删除不存在的元组
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_delete(rel, &ip);
		commit_transaction();
	}
	catch(...)
	{
		return 1;
	}
	
	return 0;
}
//测试在没有CommandCounterIncrement的情况下连续删除同一个元组
int testDeleteTheSameTupleWithoutIncrement()
{
	INTENT("删除表里的一个元组，但删除后不increment，使得后续的"
				 "操作看不到该删除。但是后面再删除同一张表里的同一个元"
				 "组。正常情况下后续的删除应该是成功的。");

	using namespace FounderXDB::StorageEngineNS;
	HeapScanDesc scan = NULL;
	Relation rel = NULL;
	try
	{
		begin_transaction();

		Oid relid = RELID;
		Oid relspace = MyDatabaseTableSpace;

		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		CHECK_BOOL(rel != NULL);

		scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		//扫描到第一个元组并删除
		HeapTuple tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		SAVE_INFO_FOR_DEBUG();
		CHECK_BOOL(tuple != NULL);
		if(tuple == NULL)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			return 0;
		}
		ItemPointer ip = &(tuple->t_self);
		//删除表中第一个元组testdata_1
		FDPG_Heap::fd_simple_heap_delete(rel, ip);
		//这里不调用CommandCounterIncrement，防止后面的操作看到新的表
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);

		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		SAVE_INFO_FOR_DEBUG();
		CHECK_BOOL(tuple != NULL);
		if(tuple == NULL)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			return 0;
		}
		char *tmp_tuple = fxdb_tuple_to_chars(tuple);
		//表中第一个元组是testdata_1
		CHECK_EQUAL(tmp_tuple, "testdata_1");
		//现在应该看不到前面的删除操作，所以第一个元组依然是testdata_1
		if(strcmp(tmp_tuple, "testdata_1") == 0)
		{
			ip = &(tuple->t_self);
			SAVE_INFO_FOR_DEBUG();
			FDPG_Heap::fd_simple_heap_delete(rel, ip);//报错，不允许修改
		}else
		{
			return 0;
		}
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);

		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
		return 1;
	}
	
	return 0;
}
//删除表中一个唯一的元组，测试插入相同的元组再根据老的块号和偏移量去删除
int testDeleteByBlockAndOffset()
{
	INTENT("获取表中一个唯一元组的块号和偏移量值并删除该元组。"
				 "随后插入相同数值的元组，根据之前获取的块号和偏移量"
				 "值去删除表中的元组。该测试的目的在于测删除一个元组"
				 "后腾出的空间是否会立刻重新利用于新的元组。这样的"
				 "话有可能会导致误删。");

	using namespace FounderXDB::StorageEngineNS;
	Relation rel = NULL;
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		HeapTuple tuple;
		int sta = 0;
		int count = 0;
		ItemPointerData ipd = findTuple("reboot", rel, sta, count);
		FDPG_Heap::fd_simple_heap_delete(rel, &ipd);
		CommandCounterIncrement();
		//构造一个reboot元组并插入
		SAVE_INFO_FOR_DEBUG();
		tuple = fdxdb_heap_formtuple("reboot", 6);
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_insert(rel, tuple);
		CommandCounterIncrement();
		SAVE_INFO_FOR_DEBUG();
		//根据之前删除元组的块信息删除当前新插入的元组
		FDPG_Heap::fd_simple_heap_delete(rel, &ipd);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		return 1;
	}
	commit_transaction();
	return 0;
}
//插入一个新元组，再根据插入成功之后获取到的块信息删除该元组
int testDeleteByTupleInsertGetBlockAndOffset()
{
	INTENT("插入一个唯一的元组，测试是否在插入成功后，该元组中"
				 "已经保存有新元组在磁盘上的块信息。该测试假定已经保"
				 "存有块信息。然后根据块信息再去删除掉该元组。");

	using namespace FounderXDB::StorageEngineNS;
	Relation rel = NULL;
	Oid relid = RELID;
	Oid relspace = MyDatabaseTableSpace;
	try
	{
		begin_transaction();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		HeapTuple tuple;
		SAVE_INFO_FOR_DEBUG();
		tuple = fdxdb_heap_formtuple("unique_data", strlen("unique_data"));
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_insert(rel, tuple);
		CommandCounterIncrement();
		//获取块信息
		ItemPointer ip = &(tuple->t_self);
		//根据块信息删除，如果ip为NULL，则测试失败
		if(ip == NULL)
		{
			return 0;
		}
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_delete(rel, ip);
		CommandCounterIncrement();
		int sta = 0;
		int count = 0;
		findTuple("unique_data", rel, sta, count);
		if(sta == 1)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			user_abort_transaction();
			return 0;
		}
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		throw;
	}
	commit_transaction();
	return 1;
}
//获取表中testdata_3的块信息，并更新为unique_data
int testUpdateSimple()
{
	INTENT("扫描testdata_3，并将其更新为unique_data。扫描更新"
				 "后的表，如果testdata_3没有被unique_data取代则测试"
				 "失败。");
	Relation rel;
	Oid relid = RELID;
	Oid relspace = MyDatabaseTableSpace;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		//扫描表，找到testdata_3，获取块信息
		int sta = 0;
		int count = 0;
		ItemPointerData ipd = findTuple("testdata_3", rel, sta, count);
		CommandCounterIncrement();
		HeapTuple tuple_new = fdxdb_heap_formtuple("unique_data", sizeof("unique_data"));
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple_new);
		CommandCounterIncrement();
		//扫描表，查看是否testdata_3被unique_data替换
		int sta2 = 0;
		findTuple("testdata_3", rel, sta, count);
		findTuple("unique_data", rel, sta2, count);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		//testdata_3不存在且unique_data存在则测试成功
		if(sta == -1 && sta2 == 1)
		{
			commit_transaction();
			return 1;
		}else 
		{
			user_abort_transaction();
			return 0;
		}
	}
	catch(...)
	{
		user_abort_transaction();
		return 0;
	}
}
//先删除一个元组但不increment然后更新该元组
int testUpdateAfterDeleteWithoutIncrement()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("先删除一个唯一的元组，但是不调用CommandCounterIncrement函数，"
				 "使得后续的操作无法看到删除元组后的表。然后更新该元组。该测试"
				 "使得系统应该能检测到更新没有存在的元组，从而抛出一个异常。");
	Relation rel = NULL;
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		ItemPointerData ipd;
		//扫描testdata_3元组并删除
		int sta = 0;
		int count = 0;
		ipd = findTuple("testdata_3", rel, sta, count);
		if(sta == 1)
		{
			SAVE_INFO_FOR_DEBUG();
			FDPG_Heap::fd_simple_heap_delete(rel, &ipd);
			//这里没有CommandCounterIncrement
			HeapTuple tuple_new = fdxdb_heap_formtuple("new_data", sizeof("new_data"));
			SAVE_INFO_FOR_DEBUG();
			FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple_new);
		}
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		std::cout << se.getErrorNo() << std::endl;
		std::cout << se.getErrorMsg() << std::endl;
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		user_abort_transaction();
		return 1;
	}
	
	return 0;
}
//随即生成表的块信息，然后更新
int testUpdateOnNoExistBlock()
{
	INTENT("自己创建一个ItemPointerData对象并为其随机初始化，然后根据"
				 "该ItemPointerData去更新表中该块的信息。系统应该会捕捉该异"
				 "常。否则测试失败。");
	Relation rel = NULL;
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		ItemPointer ip = (ItemPointer)palloc(sizeof(ItemPointerData));
		//构造一个随即的ItemPointerData
		ip->ip_blkid.bi_hi = 65535;
		ip->ip_blkid.bi_lo = 65535;
		ip->ip_posid = 65535;
		HeapTuple tuple = fdxdb_heap_formtuple("new_data", sizeof("new_data"));
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_update(rel, ip, tuple);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	}
	catch(...)
	{
		//捕捉到异常，测试成功
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
		return 1;
	}
	
	return 0;
}
//连着多次update同一个元组但只在最后一次increment，测试查看结果
int testUpdateTupleWithoutIncrementManyTimes()
{
	INTENT("多次更新同一个元组，但都不调用CommandCounterIncrement函数,"
				 "只在最后一次掉用一次。然后查看最后的结果。该测试会报异常。"
				 "因为每次更新过后的数据将产生一个新的版本，对于旧的版本，如"
				 "果没有调用CommandCounterIncrement则使得旧的版本的更新产生多"
				 "个分支，这是不允许的。如果报异常说明测试成功，不报异常则说明"
				 "测试失败。");

	using namespace FounderXDB::StorageEngineNS;
	Relation rel = NULL;
	Oid relid = RELID;
	Oid relspace = MyDatabaseTableSpace;
	ItemPointerData ipd;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		//该测试更新表中reboot元组
		int sta = 0;
		int count = 0;
		ipd = findTuple("reboot", rel, sta, count);
		if(sta == -1)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			user_abort_transaction();
			return 0;
		}
		HeapTuple new_tuple;
		//循环更新同一个元组1000*1000次
		for(int i = 0; i < 1000*1000; ++i)
		{
			new_tuple = fdxdb_heap_formtuple("new_data", sizeof("new_data"));
			SAVE_INFO_FOR_DEBUG();
			FDPG_Heap::fd_simple_heap_update(rel, &ipd, new_tuple);//在第二次更新的时候将报异常
			ipd = new_tuple->t_self;
		}
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		//报异常，测试成功
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
		return 1;
	}
	FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	user_abort_transaction();
	return 0;
}
//连着多次update同一个元组并且都increment，测试查看结果
int testUpdateTupleManyTimes()
{
	INTENT("多次更新同一个元组，且调用CommandCounterIncrement函数."
				 "。然后查看最后的结果。该测试不会报异常。该测试将产生多"
				 "个版本的元组，使得数据文件比逻辑上的大小大很多。");
	Relation rel = NULL;
	Oid relid = RELID;
	Oid relspace = MyDatabaseTableSpace;
	ItemPointerData ipd;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		SAVE_INFO_FOR_DEBUG();
		//该测试更新表中reboot元组
		int sta = 0;
		int count = 0;
		ipd = findTuple("reboot", rel, sta, count);
		if(sta == -1)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			return 0;
		}
		HeapTuple new_tuple;
		//循环更新同一个元组1000次
		for(int i = 0; i < 1000; ++i)
		{
			new_tuple = fdxdb_heap_formtuple("new_data", sizeof("new_data"));
			SAVE_INFO_FOR_DEBUG();
			FDPG_Heap::fd_simple_heap_update(rel, &ipd, new_tuple);
			CommandCounterIncrement();
			ipd = new_tuple->t_self;
		}
		//最后更新一次，并CommandCounterIncrement
		new_tuple = fdxdb_heap_formtuple("finally_data", sizeof("finally_data"));
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_update(rel, &ipd, new_tuple);
		CommandCounterIncrement();
		SAVE_INFO_FOR_DEBUG();
		//重新扫描查找最新更新的元组
		ipd = findTuple("finally_data", rel, sta, count);
		//表用剩余超过一个的finally_data元组则测试失败
		if(count > 1)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			commit_transaction();
			return 0;
		}
		int sta2 = 0;
		ipd = findTuple("reboot", rel, sta2, count);
		//找不到元组或则原来的元组依然存在，测试失败
		if(sta == -1 || sta2 == 1)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			commit_transaction();
			return 0;
		}
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	}
	catch(...)
	{
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
		throw;
	}
	commit_transaction();
	return 1;
}

extern BackendId BID;

void thread_delete(int *found, const char* det_data, const int det_data_len, BackendParameters *params, const Oid rel_id)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(params);
	*found = 0;
	int have=*found ;
	char cmp_data[DATA_LEN];
	memset(cmp_data, 0, sizeof(cmp_data));
	memcpy(cmp_data, det_data, det_data_len);
	try
	{
		begin_transaction();
		Relation rel = heap_open(rel_id,RowExclusiveLock);
		int count = 0;
		ItemPointerData ipd = findTuple(cmp_data, rel, have, count, strlen(det_data));
		if(have == 1)
		{
			FDPG_Heap::fd_simple_heap_delete(rel, &ipd);

			CommandCounterIncrement();
		}
		heap_close(rel, RowExclusiveLock);
		commit_transaction();
		*found = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*found = -1;
		user_abort_transaction();
	}
	catch(TupleUpdateConcurrent& ex)
	{
		printf("%s\n", ex.getErrorMsg());
		*found = -1;
		user_abort_transaction();
	}
	proc_exit(0);
}

#define THREAD_NUM 80


EXTERN_SIMPLE_FUNCTION

int test_thread_delete_000()
{

	INTENT("创建多个backend线程去同时删除同一个表中的"
				 "同一个数据。测试在多线程情况下能否正确删除。");
	using namespace boost;
	//创建表并插入若干测试数据
	Colinfo heap_info = NULL;

	PREPARE_TEST();
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(RELID, create_sta);
	}
	char data[][DATA_LEN] = 
	{
		"test_data1",
		"test_data2"
	};
	
	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(RELID, data, DATA_LEN, insert_sta);
	}

	//这里启动多个线程同时去删除test_data1
	//在多线程情况下，其中必须有一个线程找不到test_data1
	int found[THREAD_NUM] = {0};

	for(int i = 1; i <= THREAD_NUM; ++i)
	{
		GET_PARAM() = get_param(i);

		SAVE_PARAM(GET_PARAM());

		tg.create_thread(bind(&thread_delete, &found[i-1], "test_data1", strlen("test_data1"), GET_PARAM(), RELID));
	}
	tg.join_all();

	int count = 0;
	{
		begin_transaction();
		Relation rel = heap_open(RELID, RowExclusiveLock);
		int sta;
		int drop_sta = 0;
		findTuple("test_data1", rel, sta, count);
		if(sta == 1)//找到test_data1，失败
		{
			SIMPLE_DROP_HEAP(RELID, drop_sta);
			commit_transaction();
			FREE_PARAM(BackendParameters *);
			return 0;
		}
		findTuple("test_data2", rel, sta, count);
		if(sta == -1)//找不到test_data2，失败
		{
			SIMPLE_DROP_HEAP(RELID, drop_sta);
			commit_transaction();
			FREE_PARAM(BackendParameters *);
			return 0;
		}
		count = 0;
		heap_close(rel, RowExclusiveLock);
		commit_transaction();

		calc_tuple_num(RELID, count);
		if(count != 1)//余下元组大于1个，失败
		{
			SIMPLE_DROP_HEAP(RELID, drop_sta);
			FREE_PARAM(BackendParameters *);
			return 0;
		}
	}
	int drop_sta = 0;
	SIMPLE_DROP_HEAP(RELID, drop_sta);
	//found数组中元素内容为只有一个元素为1，其他全是-1
	count = 0;
	for(int i = 0; i < THREAD_NUM; ++i)
	{
		if(found[i] == 1)
		{
			++count;
		}
	}
	FREE_PARAM(BackendParameters *);
	return count == THREAD_NUM ? 1 : 0;
}

int test_thread_delete_001()
{

	INTENT("创建多个backend线程去同时删除同一个表中的"
				 "所有数据。测试在多线程情况下能否正确删除。");
	using namespace boost;
	//创建表并插入若干测试数据
	PREPARE_TEST();

	Colinfo heap_info = NULL;
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(RELID, create_sta);
	}
	DataGenerater dg(THREAD_NUM, DATA_LEN);
	dg.dataGenerate();
	char data[THREAD_NUM][DATA_LEN];
	dg.dataToDataArray2D(data);

	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(RELID, data, DATA_LEN, insert_sta);
	}

	//这里启动多个线程同时删除表中的所有数据
	int found[THREAD_NUM] = {0};

	for(int i = 0; i < THREAD_NUM; ++i)
	{
		GET_PARAM() = get_param(i + 1);

		SAVE_PARAM(GET_PARAM());

		tg.create_thread(bind(&thread_delete, &found[i], data[i], DATA_LEN, GET_PARAM(), RELID));
	}
	tg.join_all();

	int count = 0;
	{
		calc_tuple_num(RELID, count);
		if(count != 0)//余下元组不为0，失败
		{
			{
				int drop_sta = 0;
				SIMPLE_DROP_HEAP(RELID, drop_sta);
			}
			FREE_PARAM(BackendParameters *);
			return 0;
		}
	}

	{
		int drop_sta = 0;
		SIMPLE_DROP_HEAP(RELID, drop_sta);
	}
	//found数组中元素内容为全部为1
	count = 0;
	for(int i = 0; i < THREAD_NUM; ++i)
	{
		if(found[i] == 1)
		{
			++count;
		}
	}
	FREE_PARAM(BackendParameters *);
	return count == THREAD_NUM ? 1 : 0;
}

int test_thread_delete_002()
{

	INTENT("创建多个backend线程去同时删除同一个表中的"
				 "不同数据并余下若干数据不删除。测试在多线程"
				 "情况下能否正确删除。");
	using namespace boost;
	using namespace std;
	Colinfo heap_info = NULL;
	PREPARE_TEST();
	//创建表并插入若干测试数据
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(RELID, create_sta);
	}
	char data[][DATA_LEN] = 
	{
		"test_data1",
		"test_data2",
		"test_data3",
		"test_data4",
		"test_data5",
		"test_data6",
		"test_data7",
		"test_data8",
		"test_data9",
		"test_data10"
	};

	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(RELID, data, DATA_LEN, insert_sta);
	}

	//这里启动多个线程删除多个数据
	int found[THREAD_NUM] = {0};

	//删除的目标数据(随即生成)
	vector<string> v_delete_data;
	srand(time(NULL));
	for(int i = 0; i < THREAD_NUM; ++i)
	{
		int num = rand()%10;
		v_delete_data.push_back(data[num]);
	}

	for(int i = 1; i <= THREAD_NUM; ++i)
	{
		GET_PARAM() = get_param(i);

		SAVE_PARAM(GET_PARAM());

		tg.create_thread(bind(&thread_delete, &found[i-1], v_delete_data[i-1].c_str(), DATA_LEN, GET_PARAM(), RELID));
	}
	tg.join_all();

	int count = 0;
	{
		//将表中原来的数据放进vector里
		vector<string> v_data;
		for(int i = 0; i < ARRAY_LEN_CALC(data); ++i)
		{
			v_data.push_back(data[i]);
		}
		//利用差集求余下的数据
		set<string> tmp_set(v_delete_data.begin(), v_delete_data.end());
		vector<string> v_find_data(v_data.size() - tmp_set.size());
		sort(v_data.begin(), v_data.end());
		sort(v_delete_data.begin(), v_delete_data.end());
		set_difference(v_data.begin(), v_data.end(), v_delete_data.begin(), v_delete_data.end(), v_find_data.begin());
		
		begin_transaction();
		Relation rel = heap_open(RELID,  RowExclusiveLock);
		int sta;
		for(int i = 0; i < v_find_data.size(); ++i)
		{
			findTuple(v_find_data[i].c_str(), rel, sta, count);
			if(sta == -1)//找不到find_data[i]，失败
			{
				commit_transaction();
				FREE_PARAM(BackendParameters *);
				{
					int drop_sta = 0;
					SIMPLE_DROP_HEAP(RELID, drop_sta);
				}
				return 0;
			}
		}
		count = 0;
		heap_close(rel, RowExclusiveLock);
		commit_transaction();

		calc_tuple_num(RELID, count);
		if(count != v_find_data.size())//余下元组不等于size个，失败
		{
			{
				int drop_sta = 0;
				SIMPLE_DROP_HEAP(RELID, drop_sta);
			}
			FREE_PARAM(BackendParameters *);
			return 0;
		}
	}

	{
		int drop_sta = 0;
		SIMPLE_DROP_HEAP(RELID, drop_sta);
	}
	//found数组中元素内容为全部1，即删除操作没有抛异常
	count = 0;
	for(int i = 0; i < THREAD_NUM; ++i)
	{
		if(found[i] == 1)
		{
			++count;
		}
	}
	FREE_PARAM(BackendParameters *);
	return count == THREAD_NUM ? 1 : 0;
}

void thread_find(const char det_data[][DATA_LEN],
								 const Oid table_id,
								 const int array_len,
								 const BackendParameters *params,
								 int32 *found)
{
	using namespace FounderXDB::StorageEngineNS;
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();

	fxdb_SubPostmaster_Main((void*)params);

	try
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(table_id,AccessShareLock, MyDatabaseId);
		if(NULL == rel)
		{
			memset(found, 1, array_len * sizeof(int));
			user_abort_transaction();
			proc_exit(0);
			return;
		}
		for(int i = 0; i < array_len; ++i)
		{
			try
			{
				int32 sta = 0, count = 0;
				findTuple(det_data[i], rel, sta, count);
				if(found != NULL)
				{
					found[i] = sta;
				}
			}
			catch(StorageEngineExceptionUniversal &se)
			{
				printf("%s\n", se.getErrorMsg());
				found[i] = -1;
			}
		}
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);
		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
	}
	proc_exit(0);
}

void thread_delete_n(const char data_array[][DATA_LEN], 
										 const Oid *table_array, 
										 const int array_len,
										 const BackendParameters *params,
										 int *found)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main((void*)params);
	*found = 0;
	for(int i = 0; i < array_len; ++i)
	{
		try
		{
			begin_transaction();
			Relation rel = FDPG_Heap::fd_heap_open(table_array[i],  RowExclusiveLock, MyDatabaseId);
			int count = 0;
			ItemPointerData ipd = findTuple(data_array[i], rel, found[i], count);
			if(found[i] == 1)
			{
				FDPG_Heap::fd_simple_heap_delete(rel, &ipd);
				CommandCounterIncrement();
			}
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			commit_transaction();
		}catch(StorageEngineExceptionUniversal &se)
		{
			printf("%s\n", se.getErrorMsg());
			printf("%d\n", se.getErrorNo());
			user_abort_transaction();
		}
	}
	proc_exit(0);
}

int test_thread_delete_003()
{

	INTENT("创建多张表并插入测试数据。启动多个线程"
					 "去相互删除多个表中的信息。");
	using namespace boost;
	PREPARE_TEST();

	clear_heap_id();
	int table1 = get_heap_id(),
			table2 = get_heap_id(),
			table3 = get_heap_id();

	//创建三张表
	Colinfo heap_info = NULL;
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(table1, create_sta);
		SIMPLE_CREATE_HEAP(table2, create_sta);
		SIMPLE_CREATE_HEAP(table3, create_sta);
	}

	/*
	*	构建测试数据---
	* 各个表之间存在相同的测试数据
	* 让单个线程同时去删除3张表中的数据
	*/
	char table1_data[][DATA_LEN] = 
	{
		"test_data1",
		"test_data2",
		"test_data3",
		"test_data5"
	};
	
	char table2_data[][DATA_LEN] = 
	{
		"test_data10",
		"test_data1",
		"test_data8"
	};

	char table3_data[][DATA_LEN] = 
	{
		"test_data2",
		"test_data1",
		"test_data7",
		"test_data9",
	};

	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(table1, table1_data, DATA_LEN, insert_sta);
		SIMPLE_INSERT_DATA(table2, table2_data, DATA_LEN, insert_sta);
		SIMPLE_INSERT_DATA(table3, table3_data, DATA_LEN, insert_sta);
	}

	//启动线程删除数据
	char thread1_delete_data[][DATA_LEN] = 
	{
		"test_data1", //table1
		"test_data1", //table2
		"test_data1", //table3
		"test_data3"  //table1
	};
	char thread2_delete_data[][DATA_LEN] = 
	{
		"test_data1", //table1
		"test_data1", //table2
		"test_data1", //table3
		"test_data8"  //table2
	};
	char thread3_delete_data[][DATA_LEN] = 
	{
		"test_data1", //table1
		"test_data1", //table2
		"test_data1", //table3
		"test_data9"  //table3
	};
	Oid thread1_table[] = {table1, table2, table3, table1};
	Oid thread2_table[] = {table1, table2, table3, table2};
	Oid thread3_table[] = {table1, table2, table3, table3};
	int found[(ARRAY_LEN_CALC(table1_data))] = {0};

	//启动三个线程
	GET_PARAM() = get_param();
	SAVE_PARAM(GET_PARAM());
	for(int i = 0; i < THREAD_NUM; ++i)
	{
		tg.create_thread(bind(&thread_delete_n, 
			thread1_delete_data, 
			thread1_table, 
			ARRAY_LEN_CALC(thread1_delete_data),
			GET_PARAM(), 
			found));
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_delete_n, 
			thread2_delete_data, 
			thread2_table, 
			ARRAY_LEN_CALC(thread2_delete_data), 
			GET_PARAM(), 
			found));
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_delete_n, 
			thread3_delete_data, 
			thread3_table, 
			ARRAY_LEN_CALC(thread3_delete_data),
			GET_PARAM(), 
			found));
	}
	tg.join_all();

	/*
	* 线程结束后，三张表的数据相应应该为:
	*     
	*     table1: test_data2, test_data5
	*			table2: test_data10
	*			table3: test_data2, test_data7
	*/

	//测试结果
	begin_transaction();
	Relation rel1 = heap_open(table1, RowExclusiveLock);
	Relation rel2 = heap_open(table2, RowExclusiveLock);
	Relation rel3 = heap_open(table3, RowExclusiveLock);
	int sta[5] = {0};
	int count = 0;
	findTuple("test_data2", rel1, sta[0], count);
	findTuple("test_data5", rel1, sta[1], count);
	findTuple("test_data10", rel2, sta[2], count);
	findTuple("test_data2", rel3, sta[3], count);
	findTuple("test_data7", rel3, sta[4], count);
	heap_close(rel1, RowExclusiveLock);
	heap_close(rel2, RowExclusiveLock);
	heap_close(rel3, RowExclusiveLock);
	commit_transaction();

	for(int i = 0; i < 5; ++i)
	{
		if(sta[i] == -1)
		{
			int drop_sta = 0;
			SIMPLE_DROP_HEAP(table1, drop_sta);
			SIMPLE_DROP_HEAP(table2, drop_sta);
			SIMPLE_DROP_HEAP(table3, drop_sta);
			FREE_PARAM(BackendParameters *);
			return 0;
		}
	}

	int count_array[3] = {0};
	calc_tuple_num(table1, count_array[0]);
	calc_tuple_num(table2, count_array[1]);
	calc_tuple_num(table3, count_array[2]);

	if(count_array[0] != 2 ||
		 count_array[1] != 1 ||
		 count_array[2] != 2)
	{
		int drop_sta = 0;
		SIMPLE_DROP_HEAP(table1, drop_sta);
		SIMPLE_DROP_HEAP(table2, drop_sta);
		SIMPLE_DROP_HEAP(table3, drop_sta);
		FREE_PARAM(BackendParameters *);
		return 0;
	}

	int drop_sta = 0;
	SIMPLE_DROP_HEAP(table1, drop_sta);
	SIMPLE_DROP_HEAP(table2, drop_sta);
	SIMPLE_DROP_HEAP(table3, drop_sta);
	FREE_PARAM(BackendParameters *);
	return 1;
}

int test_thread_delete_004()
{
#define THREAD_NUM_1 20

	using namespace FounderXDB::StorageEngineNS;

	INTENT("创建多张表并插入测试数据。启动多个线程"
		"去删除其中的若干数据的同时查找这些删除的数据。");
	using namespace boost;
	PREPARE_TEST();

	clear_all();
	int table1 = get_heap_id(),
			table2 = get_heap_id();
	Colinfo heap_info = NULL;

	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(table1, create_sta);
		SIMPLE_CREATE_HEAP(table2, create_sta);
	}

	char table1_data[][DATA_LEN] = 
	{
		"test_data1",
		"test_data2",
		"test_data3"
	};

	char table2_data[][DATA_LEN] = 
	{
		"test_data10",
		"test_data11",
		"test_data12"
	};

	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(table1, table1_data, DATA_LEN, insert_sta);
		SIMPLE_INSERT_DATA(table2, table2_data, DATA_LEN, insert_sta);
	}

	{
		int found = 0;
		int found1 = 0;
		PREPARE_PARAM(BackendParameters *);
		for(int i = 0; i < THREAD_NUM_1; ++i)
		{
			/*
			* 创建删除线程
			*/
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			tg.create_thread(bind(&thread_delete, &found, "test_data1", strlen("test_data1"), GET_PARAM(), table1));
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			tg.create_thread(bind(&thread_delete, &found1, "test_data10", strlen("test_data10"), GET_PARAM(), table2));

			/*
			* 创建搜索线程
			*/
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			int32 table1_found_arr[ARRAY_LEN_CALC(table1_data)] = {0};
			tg.create_thread(bind(&thread_find, 
				table1_data, 
				table1,
				ARRAY_LEN_CALC(table1_data), 
				GET_PARAM(), 
				table1_found_arr));
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			int32 table2_found_arr[ARRAY_LEN_CALC(table2_data)] = {0};
			tg.create_thread(bind(&thread_find, 
				table2_data, 
				table2,
				ARRAY_LEN_CALC(table2_data), 
				GET_PARAM(), 
				table2_found_arr));
		}
		tg.join_all();
	}

	/*
	* 正确的结果是删除table1的test_data1和
	* table2的test_data10而不受search的影响。
	*/
	try
	{
		begin_transaction();
		Relation rel1 = FDPG_Heap::fd_heap_open(table1,AccessShareLock, MyDatabaseId);
		Relation rel2 = FDPG_Heap::fd_heap_open(table2,AccessShareLock, MyDatabaseId);
		int found[4] = {0};
		int count[2] = {0};
		findTuple("test_data2", rel1, found[0], count[0]);
		findTuple("test_data3", rel1, found[1], count[0]);
		findTuple("test_data11", rel2, found[2], count[1]);
		findTuple("test_data12", rel2, found[3], count[1]);
		for(int i= 0; i < 4; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			if(found[i] != 1)//错删，测试失败
			{
				int drop_sta = 0;
				SIMPLE_DROP_HEAP(table1, drop_sta);
				SIMPLE_DROP_HEAP(table2, drop_sta);
				commit_transaction();
				FREE_PARAM(BackendParameters *);
				return 0;
			}
		}
		FDPG_Heap::fd_heap_close(rel1, AccessShareLock);
		FDPG_Heap::fd_heap_close(rel2, AccessShareLock);
		commit_transaction();

		/*
		* 计算余下的元组数量，并根据找到的元组和余下的元组数量来确定
		* 是否删除成功。
		*/
		calc_tuple_num(table1, count[0]);
		calc_tuple_num(table2, count[1]);

		if(count[0] != 2 || count[1] != 2)
		{
			int drop_sta = 0;
			SIMPLE_DROP_HEAP(table1, drop_sta);
			SIMPLE_DROP_HEAP(table2, drop_sta);
			FREE_PARAM(BackendParameters *);
			return 0;
		}
	}catch (StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		int drop_sta = 0;
		SIMPLE_DROP_HEAP(table1, drop_sta);
		SIMPLE_DROP_HEAP(table2, drop_sta);
		commit_transaction();
		FREE_PARAM(BackendParameters *);
		return 0;
	}
	int drop_sta = 0;
	SIMPLE_DROP_HEAP(table1, drop_sta);
	SIMPLE_DROP_HEAP(table2, drop_sta);
	FREE_PARAM(BackendParameters *);
	return 1;
}

void thread_update(const char *src_data,
									 const char *det_data,
									 const unsigned int det_data_len,
									 BackendParameters *params,
									 const Oid rel_id,
									 int *found)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(params);
	*found = 0;
	Relation rel = NULL;
	try
	{
		begin_transaction();
		int count = 0;
		rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		ItemPointerData ipd = findTuple(src_data, rel, *found, count);
		if(*found == 1)
		{
			HeapTuple tuple = fdxdb_heap_formtuple(det_data, det_data_len);
			FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple);
		}
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();}
	catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*found = -1;
		user_abort_transaction();
	}
	catch(TupleUpdateConcurrent& ex)
	{
		printf("%s\n", ex.getErrorMsg());
		*found = -1;
		user_abort_transaction();
	}
	proc_exit(0);
}

void thread_update_untill_found(const char *src_data,
									 const char *det_data,
									 const unsigned int det_data_len,
									 BackendParameters *params,
									 const Oid rel_id,
									 int *found)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(params);
	*found = 0;
	Relation rel = NULL;
	try
	{
		begin_transaction();
		int count = 0;
		rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		/*
		*	循环10次(避免死循环)直到找到src_data
		*/
		for(int i = 0; ; ++i)
		{
			ItemPointerData ipd = findTuple(src_data, rel, *found, count);
			if(*found == 1)
			{
				HeapTuple tuple = fdxdb_heap_formtuple(det_data, det_data_len);
				FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple);
				break;
			}
#ifdef WIN32
			Sleep(100);
#else
       //     sleep(500);
#endif
			//sleep最多10秒防止死循环
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*found = -1;
	}
	FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	commit_transaction();
	proc_exit(0);
}

int test_thread_update_000()
{
#define THREAD_NUM_5 5

	INTENT("多线程同时更新同一张表中的同一个元组。"
				 "测试目的在于，当一个线程更新该元组后，"
				 "另外一个线程能看到old的元组但是更新会"
				 "报异常；或者是看不到old的元组。");

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
		"test_data1",
		"test_data2",
		"test_data3"
	};

	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(RELID, data, DATA_LEN, insert_sta);
	}

	int32 found[THREAD_NUM_5] = {0};
	for(int i = 0; i < THREAD_NUM_5; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_update, "test_data1", "update", strlen("update"), GET_PARAM(), RELID, &found[i]));
	}
	tg.join_all();

	int count = 0;
	for(int i = 0; i < THREAD_NUM_5; ++i)
	{
		if(found[i] == -1)
		{
			++count;
		}
	}

	/*
	* 统计更新失败的次数，启动了THREAD_NUM_5个线程去更新同一个
	* 数据，所以将会有THREAD_NUM_5 - 1个线程更新失败。
	*/
	if(count != THREAD_NUM_5 - 1)
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

int test_thread_update_001()
{
	INTENT("创建多个线程执行更新操作。每个线程更新的前提条件是"
				 "另外一个线程更新完毕。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	const int LEN = 10;
	DataGenerater dg(THREAD_NUM, LEN);
	dg.dataGenerate();

	Colinfo heap_info = NULL;
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(RELID, create_sta);
	}

	char data[1][DATA_LEN];
	memcpy(data[0], dg[0], LEN);

	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(RELID, data, DATA_LEN, insert_sta);
	}

	/*
	* 按顺序更新
	*/
	char det_data[THREAD_NUM][DATA_LEN];
	dg.dataToDataArray2D(det_data);

	{
		int32 found[THREAD_NUM] = {0};

		//创建顺序更新线程
		for(int i = 0; i < THREAD_NUM - 1; ++i)
		{
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			tg.create_thread(bind(&thread_update_untill_found, det_data[i], det_data[i + 1], DATA_LEN, GET_PARAM(), RELID, &found[i]));
		}
		tg.join_all();
	}

	/*
	* 检查结果
	*/
	try
	{
		begin_transaction();
		int32 found[THREAD_NUM] = {0};
		Relation rel = FDPG_Heap::fd_heap_open(RELID,AccessShareLock, MyDatabaseId);
		int count = 0;
		//查找是否之前的四个元组都被更新
		for(int i = 0; i < THREAD_NUM; ++i)
		{
			findTuple(det_data[i], rel, found[i], count);
		}
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);
		commit_transaction();
		for(int i = 0; i < ARRAY_LEN_CALC(found) - 1; ++i)
		{
			if(found[i] != -1)
			{
				int drop_sta = 0;
				SIMPLE_DROP_HEAP(RELID, drop_sta);
				FREE_PARAM(BackendParameters *);
				return 0;
			}
		}
		if(found[ARRAY_LEN_CALC(found) - 1] != 1)
		{
			int drop_sta = 0;
			SIMPLE_DROP_HEAP(RELID, drop_sta);
			FREE_PARAM(BackendParameters *);
			return 0;
		}
	}
	catch (StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
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

void thread_update_n(const char src_data[][DATA_LEN],
										const char det_data[][DATA_LEN],
										const unsigned int det_data_len,
										const Oid *rel_id,
										const int array_len,
										BackendParameters *params,
										int *found)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(params);
	Relation rel = NULL;
	begin_transaction();
	int count = 0;
	for(int i = 0; i < array_len; ++i)
	{
		try
		{
			rel = FDPG_Heap::fd_heap_open(rel_id[i],RowExclusiveLock, MyDatabaseId);
			int f;
			ItemPointerData ipd = findTuple(src_data[i], rel, f, count);
			if(f == 1)
			{
				HeapTuple tuple = fdxdb_heap_formtuple(det_data[i], det_data_len);
				FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple);
			}
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		}catch(StorageEngineExceptionUniversal &se)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			printf("%s\n", se.getErrorMsg());
		}
		catch(TupleUpdateConcurrent &ex)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			std::cout<<ex.getErrorMsg()<<std::endl;
		}
	}	
	commit_transaction();
	proc_exit(0);
}



#define THREAD_NUMS_1 50
int test_thread_update_rollback()
{
	INTENT("测试更新后调用user_abort_transaction回滚");
	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_1];	
	prepare_param(paramArr,THREAD_NUMS_1);
	++THREAD_RID;
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);//这里可以随便设置,因为只是简单插入数据，没用到colinfo
	thread_create_heap_xy(THREAD_NUMS_1,rid,heap_info);

	const unsigned int data_row = 10;
	const unsigned int data_len = 20;

	char data[][DATA_LEN] = //插入表中的数据
	{
		"test_data1",
		"test_data2",
		"test_data3",
		"test_data4"
	};
	int array_size=sizeof("test_data1");
	thread_insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN,rid);//启动线程插入数据

	bool main_flag = false;
	char cmpData[][DATA_LEN] = 	{//用于比较的结果(预计更新后的结果)
		"test_data1",
		"test_data2",
		"test_data3",
		"test_data4"
	};	
 	for(int i = 0;i < THREAD_NUMS_1;i++)
	{
 		tg.create_thread(boost::bind(&thread_update_rollback,(void*)paramArr[i],rid+i));//更新线程
 	}

	tg.join_all();
	free_param(paramArr,THREAD_NUMS_1);

	const int dataRows = 4;
	main_flag = check_long_result(rid, cmpData, dataRows);//检测结果与预期结果是否一致
	assert(main_flag==true);

	thread_drop_heap(THREAD_NUMS_1);//删表
	return true;
}


 void thread_update_rollback(void *param,Oid rid)
 {
	 INTENT("启动新的线程，在这个线程中更新表的数据，但是在最后调用user_abort_transaction()"
			"事务将回滚，这个将在主线中检测" );

	 using namespace FounderXDB::StorageEngineNS;
	 using namespace boost;

	 /*
	 * 构建更新数据
	 */
	 char thread1_update_src[] =   "test_data3"; //table1
	 char thread1_update_det[] =   "test_updat"; //table1
	 fxdb_SubPostmaster_Main(param);
	 SAVE_INFO_FOR_DEBUG();
	 Relation rel = NULL;
	 begin_transaction();
	 int count = 0;
	 int found[DATA_LEN];
	 int i = 0; 
	 
		 try
		 {
			 rel = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock, MyDatabaseId);
			 ItemPointerData ipd = findTuple(thread1_update_src, rel, found[i], count);
			 if(found[i] == 1)
			 {
				 HeapTuple tuple = fdxdb_heap_formtuple(thread1_update_det, sizeof(thread1_update_det));
				 FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple);
			 }
			 FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			 CommandCounterIncrement();
		 }catch(StorageEngineExceptionUniversal &se)
		 {
			 printf("%s\n", se.getErrorMsg());
			 found[i] = -1;
		 }
	
	 user_abort_transaction();//user_abort_transaction(),提交不成功，事务将回滚。
	 proc_exit(0);
 }

//多线程查询的同时更新元组
int test_thread_update_002()
{
	INTENT("启动多个线程，其中部分线程做更新操作，部分线程做查询操作。"
				 "更新操作必然成功，而查询操作可能成功也可能失败。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();
	clear_heap_id();
	int table1 = get_heap_id(),
		table2 = get_heap_id(),
		table3 = get_heap_id();

	std::cout<<"create heap starting ... \n"<<std::endl;
	Colinfo heap_info = NULL;
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(table1, create_sta);
		SIMPLE_CREATE_HEAP(table2, create_sta);
		SIMPLE_CREATE_HEAP(table3, create_sta);
	}
	std::cout<<"create heap end ... \n"<<std::endl;

	/*
	* 构建3张表的测试数据
	* 3张表的数据没有重复内容
	*/
	char table1_data[][DATA_LEN] = 
	{
		"test_data1",
		"test_data2",
		"test_data3",
		"test_data4"
	};

	char table2_data[][DATA_LEN] = 
	{
		"test_data5",
		"test_data6",
		"test_data7",
		"test_data8"
	};

	char table3_data[][DATA_LEN] = 
	{
		"test_data9",
		"test_data10",
		"test_data11"
	};

	std::cout<<"heap insert starting ... \n"<<std::endl;
	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(table1, table1_data, DATA_LEN, insert_sta);
		SIMPLE_INSERT_DATA(table2, table2_data, DATA_LEN, insert_sta);
		SIMPLE_INSERT_DATA(table3, table3_data, DATA_LEN, insert_sta);
	}
	std::cout<<"heap insert end ... \n"<<std::endl;

	/*
	* 构建更新数据
	*/
	char thread1_update_src[][DATA_LEN] = 
	{
		"test_data3", //table1
		"test_data8", //table2
		"test_data9"  //table3
	};
	char thread1_update_det[][DATA_LEN] = 
	{
		"thread1_update_", //table1
		"thread1_update_", //table2
		"thread1_update_"  //table3
	};
	Oid thread1_rel[] = {table1, table2, table3};

	char thread2_update_src[][DATA_LEN] = 
	{
		"test_data1", //table1
		"test_data7", //table2
		"test_data11"  //table3
	};
	char thread2_update_det[][DATA_LEN] = 
	{
		"thread2_update_", //table1
		"thread2_update_", //table2
		"thread2_update_"  //table3
	};
	Oid thread2_rel[] = {table1, table2, table3};

	char thread3_update_src[][DATA_LEN] = 
	{
		"test_data4", //table1
		"test_data5", //table2
		"test_data10"  //table3
	};
	char thread3_update_det[][DATA_LEN] = 
	{
		"thread3_update_", //table1
		"thread3_update_", //table2
		"thread3_update_"  //table3
	};
	{
		Oid thread3_rel[] = {table1, table2, table3};
		int found[4] = {0};
		/*
		* 启动测试线程,创建3个更新线程和3个查询线程
		*/
		const int MAX_THRAD_NUM = 80 / 6; 

		for(int i = 0; i < MAX_THRAD_NUM; ++i)
		{
			std::cout<<"heap update starting ... \n"<<std::endl;
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(bind(&thread_update_n, 
				thread1_update_src, 
				thread1_update_det, 
				DATA_LEN, 
				thread1_rel, 
				ARRAY_LEN_CALC(thread1_rel), 
				GET_PARAM(),
				found));

			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(bind(&thread_update_n, 
				thread2_update_src, 
				thread2_update_det, 
				DATA_LEN, 
				thread2_rel, 
				ARRAY_LEN_CALC(thread2_rel), 
				GET_PARAM(),
				found));

			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(bind(&thread_update_n, 
				thread3_update_src, 
				thread3_update_det, 
				DATA_LEN, 
				thread3_rel, 
				ARRAY_LEN_CALC(thread3_rel), 
				GET_PARAM(),
				found));
			std::cout<<"heap update end ... \n"<<std::endl;

			/*
			* 创建查询线程
			*/
			std::cout<<"heap query starting ... \n"<<std::endl;
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(bind(&thread_find, table1_data, table1, ARRAY_LEN_CALC(table1_data), GET_PARAM(), found));
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(bind(&thread_find, table2_data, table2, ARRAY_LEN_CALC(table2_data), GET_PARAM(), found));
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(bind(&thread_find, table3_data, table3, ARRAY_LEN_CALC(table3_data), GET_PARAM(), found));
			std::cout<<"heap query end ... \n"<<std::endl;
		}

		GET_THREAD_GROUP().join_all();
	}

	/*
	* 检测结果
	*/
	{

		{
			//检测元组长度是否变化
			int count[3] = {0};
			calc_tuple_num(table1, count[0]);
			calc_tuple_num(table2, count[1]);
			calc_tuple_num(table3, count[2]);

			if(!(count[0] == 4 && count[1] == 4 && count[2] == 3))
			{
				int drop_sta = 0;
				SIMPLE_DROP_HEAP(table1, drop_sta);
				SIMPLE_DROP_HEAP(table2, drop_sta);
				SIMPLE_DROP_HEAP(table3, drop_sta);
				FREE_PARAM(BackendParameters *);
				return 0;
			}
		}

		try
		{
			begin_transaction();
			Relation rel1 = FDPG_Heap::fd_heap_open(table1,AccessShareLock, MyDatabaseId);
			Relation rel2 = FDPG_Heap::fd_heap_open(table2,AccessShareLock, MyDatabaseId);
			Relation rel3 = FDPG_Heap::fd_heap_open(table3,AccessShareLock, MyDatabaseId);

			char det_data[][DATA_LEN] = 
			{
				"thread3_update_",
				"thread2_update_",
				"thread1_update_"
			};
			int sta[ARRAY_LEN_CALC(det_data)] = {0};
			int count[ARRAY_LEN_CALC(det_data)] = {0};
			for(int i = 0; i < ARRAY_LEN_CALC(det_data); ++i)
			{
				findTuple(det_data[i], rel1, sta[0], count[0]);
				findTuple(det_data[i], rel2, sta[1], count[1]);
				findTuple(det_data[i], rel3, sta[2], count[2]);
				//每个表中都应该有thread3_update_、thread2_update_和thread1_update_
				for(int j = 0; j < ARRAY_LEN_CALC(sta); ++j)
				{
					if(sta[j] != 1 || count[j] != 1)//没找到数据或者找到了但是数据大于1个
					{
						commit_transaction();
						int drop_sta = 0;
						SIMPLE_DROP_HEAP(table1, drop_sta);
						SIMPLE_DROP_HEAP(table2, drop_sta);
						SIMPLE_DROP_HEAP(table3, drop_sta);
						FREE_PARAM(BackendParameters *);
						return 0;
					}
				}
			}

			//查找未更新的数据
			findTuple("test_data2", rel1, sta[0], count[0]);
			findTuple("test_data6", rel2, sta[1], count[1]);
			if(!(sta[0] == 1 && sta[1] == 1 && count[0] == 1 && count[1] == 1))
			{
				commit_transaction();
				int drop_sta = 0;
				SIMPLE_DROP_HEAP(table1, drop_sta);
				SIMPLE_DROP_HEAP(table2, drop_sta);
				SIMPLE_DROP_HEAP(table3, drop_sta);
				FREE_PARAM(BackendParameters *);
				return 0;
			}
			FDPG_Heap::fd_heap_close(rel1, AccessShareLock);
			FDPG_Heap::fd_heap_close(rel2, AccessShareLock);
			FDPG_Heap::fd_heap_close(rel3, AccessShareLock);
			commit_transaction();
		}catch(StorageEngineExceptionUniversal &se)
		{
			printf("%s\n", se.getErrorMsg());
			commit_transaction();
			int drop_sta = 0;
			SIMPLE_DROP_HEAP(table1, drop_sta);
			SIMPLE_DROP_HEAP(table2, drop_sta);
			SIMPLE_DROP_HEAP(table3, drop_sta);
			FREE_PARAM(BackendParameters *);
			return 0;
		}
	}

	int drop_sta = 0;
	SIMPLE_DROP_HEAP(table1, drop_sta);
	SIMPLE_DROP_HEAP(table2, drop_sta);
	SIMPLE_DROP_HEAP(table3, drop_sta);
	FREE_PARAM(BackendParameters *);
	return 1;
}

static void thread_delete_sync(const Oid rel_id, 
															 const char *det_data,
															 const bool wait,
															 BackendParameters *params)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(params);
	Relation rel = NULL;
	begin_transaction();
	int count = 0;
	int found = 0;
	try
	{
		rel = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		ItemPointerData ipd = findTuple(det_data, rel, found, count);
		if(found == 1)
		{
			FDPG_Heap::fd_simple_heap_delete(rel, &ipd);
		}
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		CommandCounterIncrement();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		return;
	}
	if(!wait)
	{
		commit_transaction();
	}
}

int test_thread_sync_000()
{
	INTENT("启动多个线程同时删除同一个表的同一个元组。"
				 "测试在读提交事务级别下是否在前一个事务提交"
				 "之前后一个事务会等待。");

	using namespace boost;

	setColInfo(RELID, 0);
	create_heap();

	char data[][DATA_LEN] = {"test_data"};
	insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN);

	PREPARE_PARAM(BackendParameters *);
	{
		//启动不提交线程
		thread_group tg;
		BackendParameters *params = get_param();
		SAVE_PARAM(params);
		tg.create_thread(bind(&thread_delete_sync, RELID, "test_data", true, params));
		tg.join_all();
	}
	{
		//启动提交线程
		thread_group tg;
		BackendParameters *params = get_param();
		SAVE_PARAM(params);
		tg.create_thread(bind(&thread_delete_sync, RELID, "test_data", false, params));
		tg.join_all();
	}
	FREE_PARAM(BackendParameters *);

	return 1;
}

int test_delete_no()
{
	SHUTDOWN_TEST_STEP_1(TransPersistence)
		printf("函数执行第一次!!!");
	SHUTDOWN_TEST_STEP_2()
		printf("函数执行第二次!!!");
	END_SHUTDOWN_TEST()

		return 1;
}

using namespace std;
using namespace FounderXDB::StorageEngineNS;

bool myTest_heap_delete_000()
{
	INTENT("插入99个元组，测试删除任意一个元组\
		   预期结果：删除后查询结果比删除前少一个元组\
		   且删除的元组查询不到");
	try
	{
		//initial test environment
		HeapFuncTest heap;
		heap.buildHeapColInfo();
		begin_transaction();

		//open heap
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );

		//insert data
		char testData[] = "deleteFun_test_";
		const int start = 0;
		const int end = 99;
		const int dataLen = sizeof( testData );
		heap.insertRangeTuples( start, end, testData, dataLen );

		//delete the first tuple
		const int tupNo = 98;
		ItemPointer tid = heap.GetOffsetByNo( tupNo );
		heap.deleteTupleById( tid );

		//rescan first tuple of heap
		heap.scanDeleteTuple( "deleteFun_test_97", sizeof("deleteFun_test_97"), end - start );

		//drop heap
		heap.dropHeap();

		if( heap.m_bSuccess == true )
			END_TRANSACTION
		return heap.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}

bool myTest_heap_delete_001()
{
	INTENT("测试重复删除一个已被删除的元组，即不存在的元组");
	HeapFuncTest heap;
	try
	{
		//initial test environment
		heap.buildHeapColInfo();
		begin_transaction();

		//open heap
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );

		//insert data
		char testData[] = "deleteFun_test_";
		const int start = 0;
		const int end = 99;
		const int dataLen = sizeof( testData );
		heap.insertRangeTuples( start, end, testData, dataLen );

		//delete 5th tuple
		const int delRowNum = 5;
		ItemPointer tid = heap.GetOffsetByNo( delRowNum );
		heap.deleteTupleById( tid );
		heap.m_bSuccess = true;

		//delete 5th tuple again
		heap.deleteTupleById( tid );

		heap.dropHeap();
		commit_transaction();
		printf("test failed！\n");
	}
	catch ( StorageEngineExceptionUniversal &ex )
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		if( heap.m_bSuccess )
		{
			printf("test success！\n");
		}
		else
		{
			printf("test failed！\n");
		}
	}

	return heap.m_bSuccess;
}