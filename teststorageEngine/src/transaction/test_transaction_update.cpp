#include <assert.h>
#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"

#include "test_fram.h"
#include "utils/util.h"
#include "transaction/test_transaction_update.h"
#include "utils/fmgroids.h"
#include "interface/FDPGAdapter.h"
#include "iostream"
#include "interface/StorageEngineExceptionUniversal.h"
using namespace FounderXDB::StorageEngineNS;

extern int RELID;

#define UNIQUE_DATA "reboot"

ItemPointerData findTuple(char *data, Relation rel, int &sta)
{
	assert(rel);
	sta = -1;
	char *tmp_data;
	HeapScanDesc scan;
	scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
	ItemPointerData ipd;
	memset(&ipd, 0, sizeof(ipd));
	HeapTuple tuple;
	while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		tmp_data = fxdb_tuple_to_chars(tuple);
		if(strcmp(tmp_data, data) == 0)
		{
			sta = 1;
			ipd = tuple->t_self;
			pfree(tmp_data);
			break;
		}
		pfree(tmp_data);
	}
	FDPG_Heap::fd_heap_endscan(scan);
	return ipd;
}

int testTransactionUpdate001()
{
	INTENT("测试更新reboot元组，回滚事务，查看第一个元组是否发生变化。"
				 "如果发生变化说明事务回滚失败，否则回滚成功。");
	Relation rel;
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Oid relspace = DEFAULTTABLESPACE_OID;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		int sta;
		ItemPointerData ipd = findTuple(UNIQUE_DATA, rel, sta);
		HeapTuple tuple = fdxdb_heap_formtuple("aaaaa", strlen("aaaaa"));
		FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple);
		FDPG_Heap::fd_heap_close(rel, NoLock);
		//回滚事务
		user_abort_transaction();
		//查找reboot和aaaaa元组，测试成功的条件是前者存在而后者不存在
		begin_transaction();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		findTuple(UNIQUE_DATA, rel, sta);
		int sta2;
		findTuple("aaaaa", rel, sta2);
		if(sta == 1 && sta2 == -1)
		{
			FDPG_Heap::fd_heap_close(rel, NoLock);
			commit_transaction();
			return 1;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		std::cout << se.getErrorNo() << std::endl;
		std::cout << se.getErrorMsg() << std::endl;
	}
	FDPG_Heap::fd_heap_close(rel, NoLock);
	commit_transaction();
	return 0;
}

int testTransactionUpdate002()
{
	INTENT("测试更新reboot元组为newreboot，更新100*100次，回滚。"
				 "再次扫描，测试成功条件为reboot存在而newreboot不存在。");
	Relation rel;
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Oid relspace = DEFAULTTABLESPACE_OID;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		int sta;
		ItemPointerData ipd = findTuple(UNIQUE_DATA, rel, sta);
		for(int i = 0; i < 100*100; ++i)
		{
			HeapTuple tuple = fdxdb_heap_formtuple("newreboot", strlen("newreboot"));
			FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple);
			ipd = tuple->t_self;
			CommandCounterIncrement();
		}
		//回滚事务
		FDPG_Heap::fd_heap_close(rel, NoLock);
		user_abort_transaction();
		//查找reboot和newreboot元组，测试成功的条件是前者存在而后者不存在
		begin_transaction();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		findTuple(UNIQUE_DATA, rel, sta);
		int sta2;
		findTuple("newreboot", rel, sta2);
		if(sta == 1 && sta2 == -1)
		{
			FDPG_Heap::fd_heap_close(rel, NoLock);
			commit_transaction();
			return 1;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		FDPG_Heap::fd_heap_close(rel, NoLock);
		user_abort_transaction();
		std::cout << se.getErrorNo() << std::endl;
		std::cout << se.getErrorMsg() << std::endl;
		return 0;
	}
	FDPG_Heap::fd_heap_close(rel, NoLock);
	commit_transaction();
	return 0;
}

int testTransactionUpdate003()
{
	INTENT("测试更新reboot元组为newreboot,提交事务。"
				 "重启一个事务，扫描。测试成功的条件是reboot"
				 "不存在而newreboot存在");
	Relation rel;
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Oid relspace = DEFAULTTABLESPACE_OID;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		int sta;
		ItemPointerData ipd = findTuple(UNIQUE_DATA, rel, sta);
		HeapTuple tuple = fdxdb_heap_formtuple("newreboot", strlen("newreboot"));
		FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple);
		CommandCounterIncrement();
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		//提交事务
		commit_transaction();
		//查找reboot和newreboot元组，测试成功的条件是前者不存在而后者存在
		begin_transaction();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		findTuple(UNIQUE_DATA, rel, sta);
		int sta2;
		findTuple("newreboot", rel, sta2);
		if(sta == -1 && sta2 == 1)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			commit_transaction();
			return 1;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		std::cout << se.getErrorNo() << std::endl;
		std::cout << se.getErrorMsg() << std::endl;
	}
	commit_transaction();
	return 0;
}