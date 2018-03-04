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
	INTENT("���Ը���rebootԪ�飬�ع����񣬲鿴��һ��Ԫ���Ƿ����仯��"
				 "��������仯˵������ع�ʧ�ܣ�����ع��ɹ���");
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
		//�ع�����
		user_abort_transaction();
		//����reboot��aaaaaԪ�飬���Գɹ���������ǰ�ߴ��ڶ����߲�����
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
	INTENT("���Ը���rebootԪ��Ϊnewreboot������100*100�Σ��ع���"
				 "�ٴ�ɨ�裬���Գɹ�����Ϊreboot���ڶ�newreboot�����ڡ�");
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
		//�ع�����
		FDPG_Heap::fd_heap_close(rel, NoLock);
		user_abort_transaction();
		//����reboot��newrebootԪ�飬���Գɹ���������ǰ�ߴ��ڶ����߲�����
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
	INTENT("���Ը���rebootԪ��Ϊnewreboot,�ύ����"
				 "����һ������ɨ�衣���Գɹ���������reboot"
				 "�����ڶ�newreboot����");
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
		//�ύ����
		commit_transaction();
		//����reboot��newrebootԪ�飬���Գɹ���������ǰ�߲����ڶ����ߴ���
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