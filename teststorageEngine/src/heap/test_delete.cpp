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

//��Ļ��ӡ��ǰ������
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

//ɾ���������������ı�
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

//����һ�ű�,��������Ԫ��
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

//����һ�ű�,��������Ԫ��
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
		printf("\t===���ʼ����===\n");
		printfHeapTuple(rel);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);		
		commit_transaction();
	}
	catch(...)
	{
		PG_RE_THROW();
	}
	
}

//ɾ���������������ı�
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

//����ɾ�����ݱ��е�һ��Ԫ��
int testDeleteFirstTupleFromHeap()
{
	INTENT("����һ������Ԫ�飬����ɾ�����ݱ��е�һ��Ԫ��,"
		"��ɾ������ĵ���Ԫ�顣��Ļ��ӡɾ��ǰ��һ��Ԫ�������"
		"���ɾ��ʧ�ܣ�����ʧ�ܣ���������0����Ļ��ӡ�����Ϣ��");

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

		//ɨ���ĵ�һ��Ԫ�飬��ɾ����
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);

		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple != NULL);
		//�������û��Ԫ����ر�ɨ�貢�˳�����
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
		//����ɨ���Ѱ�ҵ�һ��Ԫ���Ƿ񻹴���
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		//����ԭ���ж������ݣ����Ե�ǰ��Ӧ�û��ǲ�Ϊ��
		CHECK_BOOL(NULL != tuple);
		if(NULL == tuple)
		{
			printf("\tthere is no more tuple!\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			return 0;
		}
		char *first_tuple = fxdb_tuple_to_chars(tuple);
		//�����һ��Ԫ�鲻����Ԥ�Ȳ����testdata_1����˵�����Գɹ�
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

//��������ͬԪ�飬����ɾ������һ��Ԫ��
int testDeleteOneTupleFromManyTuple()
{
	INTENT("��������ͬԪ�飬����ɾ������һ��Ԫ�顣"
		"���ݸ���������ɨ������ҵ���Ӧ����Ȼ��ɾ����"
		"����Ҳ���Ҫɾ�������ݣ��������ɨ��ʧ�ܣ������ǲ���ʧ�ܡ����"
		"�ҵ���Ԫ����ɾ������Ļ��ӡɾ��ǰ�����ݣ���ӡɾ��������ݡ�");

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
		//ɨ����ҵ�Ҫɾ����Ԫ��
		while (true)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			if(tuple == NULL)
			{
				break;
			}
			SAVE_INFO_FOR_DEBUG();
			tmp_tuple = fxdb_tuple_to_chars(tuple);
			//����Ҫɾ����Ԫ��������Ϊ"testdata_3"��Ԫ��
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
			printf("\t�Ҳ���Ҫɾ����Ԫ�顣\n");
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
		//����ɨ����鿴�Ƿ񻹴���"testdata_3"
		while (true)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			if (tuple == NULL) {
				break;
			}
			tmp_tuple = fxdb_tuple_to_chars(tuple);
			//�ҵ�Ψһ��"testdata_3"�����ڣ�˵������ʧ��
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

//��������ͬԪ�飬����ɾ������һ��Ԫ��
int testDeleteOneTupleFromTheSameTuple()
{
	INTENT("��������ͬԪ�飬����ɾ������һ��Ԫ�顣"
		"���ݸ���������ɨ������ҵ���Ӧ����Ȼ��ɾ����"
		"����Ҳ���Ҫɾ�������ݣ��������ɨ��ʧ�ܣ������ǲ���ʧ�ܡ����"
		"�ҵ���Ԫ����ɾ������Ļ��ӡɾ��ǰ�����ݣ���ӡɾ��������ݡ����"
		"��ͬԪ��ͬʱ������ɾ����Ӧ���Ǳ�ĵ�һ��Ԫ�顣");

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
		//ɨ����ҵ�Ԫ��"testdata_2"ɾ��
		while (true)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			//����Ӧ�ô�����testdata_2�����û�У�˵������ʧ��
			if(tuple == NULL)
			{
				break;
			}
			SAVE_INFO_FOR_DEBUG();
			tmp_tuple=fxdb_tuple_to_chars(tuple);
			//�ҵ���testdata_2��ɾ��
			if(strcmp(tmp_tuple, "testdata_2") == 0)
			{
				tid = &(tuple->t_self);
				SAVE_INFO_FOR_DEBUG();
				FDPG_Heap::fd_simple_heap_delete(testRelation, tid);
				status = 1;
				break;
			}
		}
		//�Ҳ���testdata_2
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
			//����Ӧ���ж������ݣ��������ҵ�Ԫ��
			if (tuple == NULL) {
				break;
			}
			tmp_tuple = fxdb_tuple_to_chars(tuple);
			//����ʣ�µ�testdata_2
			if(strcmp(tmp_tuple, "testdata_2") == 0)
			{
				++status;
			}
		}
		//����ԭ������������testdata_2������Ӧ�û�ʣ��һ��
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

//�����ɸ����Ĳ���ɾ��һ��Ԫ��
int testDeleteFromGiveSteps()
{
	INTENT("���Ը��ݸ����Ĳ���ɾ��һ��Ԫ�飬��Ļ��ӡɾ��ǰ���ݣ���ӡɾ�������ݡ�");

	using namespace FounderXDB::StorageEngineNS;
	try
	{
		begin_transaction();
		//��������Ϊ4����ɾ�����ĸ������Ԫ��
		int step = 4;
		Oid relid = RELID;
		Relation rel;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		HeapScanDesc scan;
		HeapTuple tuple;
		scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		//ɨ�赽��ĵ�����
		while(step > 1)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			CHECK_BOOL(tuple != NULL);
			if(tuple == NULL)
			{
				printf("�ѵ�����ĩβ!\n");
				return 0;
			}
			--step;
		}
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple != NULL);
		//���еĵ�����Ӧ�ò��Ǳ�Ľ�β
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
		//����ɨ��������ĸ�Ԫ��
		while(step <= 4)
		{
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
			++step;
		}
		//��ǰtuple��Ϊ��
		SAVE_INFO_FOR_DEBUG();
		if(tuple == NULL)
		{
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			return 0;
		}
		char *tmp_tuple = fxdb_tuple_to_chars(tuple);
		FDPG_Heap::fd_heap_endscan(scan);
		//��ǰ���ĸ�Ԫ��Ӧ�ò���testdata_1
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

//����ɾ�����ݱ��в����ڵĿ��е�Ԫ��
int testDeleteNoExistTuple()
{
	INTENT("����ɾ�����в������ڵ�Ԫ�飬PG�Ƿ����ȷ�㱨��Ϣ��"
		"������ɾ���������е�������ȥɾ����Ҳ���Խ�һ���ձ�"
		"ȥɾ����Ҳ����ɾ����ô������Ԫ�鼯�ϵ�Ԫ��(��������"
		"��Ԫ��)���������֣�Ч����һ����");
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Relation rel;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,AccessShareLock, MyDatabaseId);
		CHECK_BOOL(rel != NULL);
		//�����ڵı�
		if(rel == NULL)
		{
			return 0;
		}
		//����һ�������ItemPointerData
		ItemPointerData ip;
		ip.ip_blkid.bi_hi = 65535;
		ip.ip_blkid.bi_lo = 65535;
		ip.ip_posid = 65535;
		//ɾ�������ڵ�Ԫ��
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
//������û��CommandCounterIncrement�����������ɾ��ͬһ��Ԫ��
int testDeleteTheSameTupleWithoutIncrement()
{
	INTENT("ɾ�������һ��Ԫ�飬��ɾ����increment��ʹ�ú�����"
				 "������������ɾ�������Ǻ�����ɾ��ͬһ�ű����ͬһ��Ԫ"
				 "�顣��������º�����ɾ��Ӧ���ǳɹ��ġ�");

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
		//ɨ�赽��һ��Ԫ�鲢ɾ��
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
		//ɾ�����е�һ��Ԫ��testdata_1
		FDPG_Heap::fd_simple_heap_delete(rel, ip);
		//���ﲻ����CommandCounterIncrement����ֹ����Ĳ��������µı�
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
		//���е�һ��Ԫ����testdata_1
		CHECK_EQUAL(tmp_tuple, "testdata_1");
		//����Ӧ�ÿ�����ǰ���ɾ�����������Ե�һ��Ԫ����Ȼ��testdata_1
		if(strcmp(tmp_tuple, "testdata_1") == 0)
		{
			ip = &(tuple->t_self);
			SAVE_INFO_FOR_DEBUG();
			FDPG_Heap::fd_simple_heap_delete(rel, ip);//�����������޸�
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
//ɾ������һ��Ψһ��Ԫ�飬���Բ�����ͬ��Ԫ���ٸ����ϵĿ�ź�ƫ����ȥɾ��
int testDeleteByBlockAndOffset()
{
	INTENT("��ȡ����һ��ΨһԪ��Ŀ�ź�ƫ����ֵ��ɾ����Ԫ�顣"
				 "��������ͬ��ֵ��Ԫ�飬����֮ǰ��ȡ�Ŀ�ź�ƫ����"
				 "ֵȥɾ�����е�Ԫ�顣�ò��Ե�Ŀ�����ڲ�ɾ��һ��Ԫ��"
				 "���ڳ��Ŀռ��Ƿ�����������������µ�Ԫ�顣������"
				 "���п��ܻᵼ����ɾ��");

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
		//����һ��rebootԪ�鲢����
		SAVE_INFO_FOR_DEBUG();
		tuple = fdxdb_heap_formtuple("reboot", 6);
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_insert(rel, tuple);
		CommandCounterIncrement();
		SAVE_INFO_FOR_DEBUG();
		//����֮ǰɾ��Ԫ��Ŀ���Ϣɾ����ǰ�²����Ԫ��
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
//����һ����Ԫ�飬�ٸ��ݲ���ɹ�֮���ȡ���Ŀ���Ϣɾ����Ԫ��
int testDeleteByTupleInsertGetBlockAndOffset()
{
	INTENT("����һ��Ψһ��Ԫ�飬�����Ƿ��ڲ���ɹ��󣬸�Ԫ����"
				 "�Ѿ���������Ԫ���ڴ����ϵĿ���Ϣ���ò��Լٶ��Ѿ���"
				 "���п���Ϣ��Ȼ����ݿ���Ϣ��ȥɾ������Ԫ�顣");

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
		//��ȡ����Ϣ
		ItemPointer ip = &(tuple->t_self);
		//���ݿ���Ϣɾ�������ipΪNULL�������ʧ��
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
//��ȡ����testdata_3�Ŀ���Ϣ��������Ϊunique_data
int testUpdateSimple()
{
	INTENT("ɨ��testdata_3�����������Ϊunique_data��ɨ�����"
				 "��ı����testdata_3û�б�unique_dataȡ�������"
				 "ʧ�ܡ�");
	Relation rel;
	Oid relid = RELID;
	Oid relspace = MyDatabaseTableSpace;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		//ɨ����ҵ�testdata_3����ȡ����Ϣ
		int sta = 0;
		int count = 0;
		ItemPointerData ipd = findTuple("testdata_3", rel, sta, count);
		CommandCounterIncrement();
		HeapTuple tuple_new = fdxdb_heap_formtuple("unique_data", sizeof("unique_data"));
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_update(rel, &ipd, tuple_new);
		CommandCounterIncrement();
		//ɨ����鿴�Ƿ�testdata_3��unique_data�滻
		int sta2 = 0;
		findTuple("testdata_3", rel, sta, count);
		findTuple("unique_data", rel, sta2, count);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		//testdata_3��������unique_data��������Գɹ�
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
//��ɾ��һ��Ԫ�鵫��incrementȻ����¸�Ԫ��
int testUpdateAfterDeleteWithoutIncrement()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("��ɾ��һ��Ψһ��Ԫ�飬���ǲ�����CommandCounterIncrement������"
				 "ʹ�ú����Ĳ����޷�����ɾ��Ԫ���ı�Ȼ����¸�Ԫ�顣�ò���"
				 "ʹ��ϵͳӦ���ܼ�⵽����û�д��ڵ�Ԫ�飬�Ӷ��׳�һ���쳣��");
	Relation rel = NULL;
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		ItemPointerData ipd;
		//ɨ��testdata_3Ԫ�鲢ɾ��
		int sta = 0;
		int count = 0;
		ipd = findTuple("testdata_3", rel, sta, count);
		if(sta == 1)
		{
			SAVE_INFO_FOR_DEBUG();
			FDPG_Heap::fd_simple_heap_delete(rel, &ipd);
			//����û��CommandCounterIncrement
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
//�漴���ɱ�Ŀ���Ϣ��Ȼ�����
int testUpdateOnNoExistBlock()
{
	INTENT("�Լ�����һ��ItemPointerData����Ϊ�������ʼ����Ȼ�����"
				 "��ItemPointerDataȥ���±��иÿ����Ϣ��ϵͳӦ�ûᲶ׽����"
				 "�����������ʧ�ܡ�");
	Relation rel = NULL;
	try
	{
		begin_transaction();
		Oid relid = RELID;
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		ItemPointer ip = (ItemPointer)palloc(sizeof(ItemPointerData));
		//����һ���漴��ItemPointerData
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
		//��׽���쳣�����Գɹ�
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
		return 1;
	}
	
	return 0;
}
//���Ŷ��updateͬһ��Ԫ�鵫ֻ�����һ��increment�����Բ鿴���
int testUpdateTupleWithoutIncrementManyTimes()
{
	INTENT("��θ���ͬһ��Ԫ�飬����������CommandCounterIncrement����,"
				 "ֻ�����һ�ε���һ�Ρ�Ȼ��鿴���Ľ�����ò��Իᱨ�쳣��"
				 "��Ϊÿ�θ��¹�������ݽ�����һ���µİ汾�����ھɵİ汾����"
				 "��û�е���CommandCounterIncrement��ʹ�þɵİ汾�ĸ��²�����"
				 "����֧�����ǲ�����ġ�������쳣˵�����Գɹ��������쳣��˵��"
				 "����ʧ�ܡ�");

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
		//�ò��Ը��±���rebootԪ��
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
		//ѭ������ͬһ��Ԫ��1000*1000��
		for(int i = 0; i < 1000*1000; ++i)
		{
			new_tuple = fdxdb_heap_formtuple("new_data", sizeof("new_data"));
			SAVE_INFO_FOR_DEBUG();
			FDPG_Heap::fd_simple_heap_update(rel, &ipd, new_tuple);//�ڵڶ��θ��µ�ʱ�򽫱��쳣
			ipd = new_tuple->t_self;
		}
	}
	catch(StorageEngineExceptionUniversal &se)
	{
		//���쳣�����Գɹ�
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
		return 1;
	}
	FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	user_abort_transaction();
	return 0;
}
//���Ŷ��updateͬһ��Ԫ�鲢�Ҷ�increment�����Բ鿴���
int testUpdateTupleManyTimes()
{
	INTENT("��θ���ͬһ��Ԫ�飬�ҵ���CommandCounterIncrement����."
				 "��Ȼ��鿴���Ľ�����ò��Բ��ᱨ�쳣���ò��Խ�������"
				 "���汾��Ԫ�飬ʹ�������ļ����߼��ϵĴ�С��ܶࡣ");
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
		//�ò��Ը��±���rebootԪ��
		int sta = 0;
		int count = 0;
		ipd = findTuple("reboot", rel, sta, count);
		if(sta == -1)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			return 0;
		}
		HeapTuple new_tuple;
		//ѭ������ͬһ��Ԫ��1000��
		for(int i = 0; i < 1000; ++i)
		{
			new_tuple = fdxdb_heap_formtuple("new_data", sizeof("new_data"));
			SAVE_INFO_FOR_DEBUG();
			FDPG_Heap::fd_simple_heap_update(rel, &ipd, new_tuple);
			CommandCounterIncrement();
			ipd = new_tuple->t_self;
		}
		//������һ�Σ���CommandCounterIncrement
		new_tuple = fdxdb_heap_formtuple("finally_data", sizeof("finally_data"));
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_simple_heap_update(rel, &ipd, new_tuple);
		CommandCounterIncrement();
		SAVE_INFO_FOR_DEBUG();
		//����ɨ��������¸��µ�Ԫ��
		ipd = findTuple("finally_data", rel, sta, count);
		//����ʣ�೬��һ����finally_dataԪ�������ʧ��
		if(count > 1)
		{
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			commit_transaction();
			return 0;
		}
		int sta2 = 0;
		ipd = findTuple("reboot", rel, sta2, count);
		//�Ҳ���Ԫ�����ԭ����Ԫ����Ȼ���ڣ�����ʧ��
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

	INTENT("�������backend�߳�ȥͬʱɾ��ͬһ�����е�"
				 "ͬһ�����ݡ������ڶ��߳�������ܷ���ȷɾ����");
	using namespace boost;
	//�������������ɲ�������
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

	//������������߳�ͬʱȥɾ��test_data1
	//�ڶ��߳�����£����б�����һ���߳��Ҳ���test_data1
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
		if(sta == 1)//�ҵ�test_data1��ʧ��
		{
			SIMPLE_DROP_HEAP(RELID, drop_sta);
			commit_transaction();
			FREE_PARAM(BackendParameters *);
			return 0;
		}
		findTuple("test_data2", rel, sta, count);
		if(sta == -1)//�Ҳ���test_data2��ʧ��
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
		if(count != 1)//����Ԫ�����1����ʧ��
		{
			SIMPLE_DROP_HEAP(RELID, drop_sta);
			FREE_PARAM(BackendParameters *);
			return 0;
		}
	}
	int drop_sta = 0;
	SIMPLE_DROP_HEAP(RELID, drop_sta);
	//found������Ԫ������Ϊֻ��һ��Ԫ��Ϊ1������ȫ��-1
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

	INTENT("�������backend�߳�ȥͬʱɾ��ͬһ�����е�"
				 "�������ݡ������ڶ��߳�������ܷ���ȷɾ����");
	using namespace boost;
	//�������������ɲ�������
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

	//������������߳�ͬʱɾ�����е���������
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
		if(count != 0)//����Ԫ�鲻Ϊ0��ʧ��
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
	//found������Ԫ������Ϊȫ��Ϊ1
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

	INTENT("�������backend�߳�ȥͬʱɾ��ͬһ�����е�"
				 "��ͬ���ݲ������������ݲ�ɾ���������ڶ��߳�"
				 "������ܷ���ȷɾ����");
	using namespace boost;
	using namespace std;
	Colinfo heap_info = NULL;
	PREPARE_TEST();
	//�������������ɲ�������
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

	//������������߳�ɾ���������
	int found[THREAD_NUM] = {0};

	//ɾ����Ŀ������(�漴����)
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
		//������ԭ�������ݷŽ�vector��
		vector<string> v_data;
		for(int i = 0; i < ARRAY_LEN_CALC(data); ++i)
		{
			v_data.push_back(data[i]);
		}
		//���ò�����µ�����
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
			if(sta == -1)//�Ҳ���find_data[i]��ʧ��
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
		if(count != v_find_data.size())//����Ԫ�鲻����size����ʧ��
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
	//found������Ԫ������Ϊȫ��1����ɾ������û�����쳣
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

	INTENT("�������ű�����������ݡ���������߳�"
					 "ȥ�໥ɾ��������е���Ϣ��");
	using namespace boost;
	PREPARE_TEST();

	clear_heap_id();
	int table1 = get_heap_id(),
			table2 = get_heap_id(),
			table3 = get_heap_id();

	//�������ű�
	Colinfo heap_info = NULL;
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(table1, create_sta);
		SIMPLE_CREATE_HEAP(table2, create_sta);
		SIMPLE_CREATE_HEAP(table3, create_sta);
	}

	/*
	*	������������---
	* ������֮�������ͬ�Ĳ�������
	* �õ����߳�ͬʱȥɾ��3�ű��е�����
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

	//�����߳�ɾ������
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

	//���������߳�
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
	* �߳̽��������ű��������ӦӦ��Ϊ:
	*     
	*     table1: test_data2, test_data5
	*			table2: test_data10
	*			table3: test_data2, test_data7
	*/

	//���Խ��
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

	INTENT("�������ű�����������ݡ���������߳�"
		"ȥɾ�����е��������ݵ�ͬʱ������Щɾ�������ݡ�");
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
			* ����ɾ���߳�
			*/
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			tg.create_thread(bind(&thread_delete, &found, "test_data1", strlen("test_data1"), GET_PARAM(), table1));
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			tg.create_thread(bind(&thread_delete, &found1, "test_data10", strlen("test_data10"), GET_PARAM(), table2));

			/*
			* ���������߳�
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
	* ��ȷ�Ľ����ɾ��table1��test_data1��
	* table2��test_data10������search��Ӱ�졣
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
			if(found[i] != 1)//��ɾ������ʧ��
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
		* �������µ�Ԫ���������������ҵ���Ԫ������µ�Ԫ��������ȷ��
		* �Ƿ�ɾ���ɹ���
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
		*	ѭ��10��(������ѭ��)ֱ���ҵ�src_data
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
			//sleep���10���ֹ��ѭ��
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

	INTENT("���߳�ͬʱ����ͬһ�ű��е�ͬһ��Ԫ�顣"
				 "����Ŀ�����ڣ���һ���̸߳��¸�Ԫ���"
				 "����һ���߳��ܿ���old��Ԫ�鵫�Ǹ��»�"
				 "���쳣�������ǿ�����old��Ԫ�顣");

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
	* ͳ�Ƹ���ʧ�ܵĴ�����������THREAD_NUM_5���߳�ȥ����ͬһ��
	* ���ݣ����Խ�����THREAD_NUM_5 - 1���̸߳���ʧ�ܡ�
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
	INTENT("��������߳�ִ�и��²�����ÿ���̸߳��µ�ǰ��������"
				 "����һ���̸߳�����ϡ�");

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
	* ��˳�����
	*/
	char det_data[THREAD_NUM][DATA_LEN];
	dg.dataToDataArray2D(det_data);

	{
		int32 found[THREAD_NUM] = {0};

		//����˳������߳�
		for(int i = 0; i < THREAD_NUM - 1; ++i)
		{
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			tg.create_thread(bind(&thread_update_untill_found, det_data[i], det_data[i + 1], DATA_LEN, GET_PARAM(), RELID, &found[i]));
		}
		tg.join_all();
	}

	/*
	* �����
	*/
	try
	{
		begin_transaction();
		int32 found[THREAD_NUM] = {0};
		Relation rel = FDPG_Heap::fd_heap_open(RELID,AccessShareLock, MyDatabaseId);
		int count = 0;
		//�����Ƿ�֮ǰ���ĸ�Ԫ�鶼������
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
	INTENT("���Ը��º����user_abort_transaction�ع�");
	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	boost::thread_group tg;
	BackendParameters *paramArr[THREAD_NUMS_1];	
	prepare_param(paramArr,THREAD_NUMS_1);
	++THREAD_RID;
	Oid rid = THREAD_RID;
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);//��������������,��Ϊֻ�Ǽ򵥲������ݣ�û�õ�colinfo
	thread_create_heap_xy(THREAD_NUMS_1,rid,heap_info);

	const unsigned int data_row = 10;
	const unsigned int data_len = 20;

	char data[][DATA_LEN] = //������е�����
	{
		"test_data1",
		"test_data2",
		"test_data3",
		"test_data4"
	};
	int array_size=sizeof("test_data1");
	thread_insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN,rid);//�����̲߳�������

	bool main_flag = false;
	char cmpData[][DATA_LEN] = 	{//���ڱȽϵĽ��(Ԥ�Ƹ��º�Ľ��)
		"test_data1",
		"test_data2",
		"test_data3",
		"test_data4"
	};	
 	for(int i = 0;i < THREAD_NUMS_1;i++)
	{
 		tg.create_thread(boost::bind(&thread_update_rollback,(void*)paramArr[i],rid+i));//�����߳�
 	}

	tg.join_all();
	free_param(paramArr,THREAD_NUMS_1);

	const int dataRows = 4;
	main_flag = check_long_result(rid, cmpData, dataRows);//�������Ԥ�ڽ���Ƿ�һ��
	assert(main_flag==true);

	thread_drop_heap(THREAD_NUMS_1);//ɾ��
	return true;
}


 void thread_update_rollback(void *param,Oid rid)
 {
	 INTENT("�����µ��̣߳�������߳��и��±�����ݣ�������������user_abort_transaction()"
			"���񽫻ع���������������м��" );

	 using namespace FounderXDB::StorageEngineNS;
	 using namespace boost;

	 /*
	 * ������������
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
	
	 user_abort_transaction();//user_abort_transaction(),�ύ���ɹ������񽫻ع���
	 proc_exit(0);
 }

//���̲߳�ѯ��ͬʱ����Ԫ��
int test_thread_update_002()
{
	INTENT("��������̣߳����в����߳������²����������߳�����ѯ������"
				 "���²�����Ȼ�ɹ�������ѯ�������ܳɹ�Ҳ����ʧ�ܡ�");

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
	* ����3�ű�Ĳ�������
	* 3�ű������û���ظ�����
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
	* ������������
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
		* ���������߳�,����3�������̺߳�3����ѯ�߳�
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
			* ������ѯ�߳�
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
	* �����
	*/
	{

		{
			//���Ԫ�鳤���Ƿ�仯
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
				//ÿ�����ж�Ӧ����thread3_update_��thread2_update_��thread1_update_
				for(int j = 0; j < ARRAY_LEN_CALC(sta); ++j)
				{
					if(sta[j] != 1 || count[j] != 1)//û�ҵ����ݻ����ҵ��˵������ݴ���1��
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

			//����δ���µ�����
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
	INTENT("��������߳�ͬʱɾ��ͬһ�����ͬһ��Ԫ�顣"
				 "�����ڶ��ύ���񼶱����Ƿ���ǰһ�������ύ"
				 "֮ǰ��һ�������ȴ���");

	using namespace boost;

	setColInfo(RELID, 0);
	create_heap();

	char data[][DATA_LEN] = {"test_data"};
	insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN);

	PREPARE_PARAM(BackendParameters *);
	{
		//�������ύ�߳�
		thread_group tg;
		BackendParameters *params = get_param();
		SAVE_PARAM(params);
		tg.create_thread(bind(&thread_delete_sync, RELID, "test_data", true, params));
		tg.join_all();
	}
	{
		//�����ύ�߳�
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
		printf("����ִ�е�һ��!!!");
	SHUTDOWN_TEST_STEP_2()
		printf("����ִ�еڶ���!!!");
	END_SHUTDOWN_TEST()

		return 1;
}

using namespace std;
using namespace FounderXDB::StorageEngineNS;

bool myTest_heap_delete_000()
{
	INTENT("����99��Ԫ�飬����ɾ������һ��Ԫ��\
		   Ԥ�ڽ����ɾ�����ѯ�����ɾ��ǰ��һ��Ԫ��\
		   ��ɾ����Ԫ���ѯ����");
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
	INTENT("�����ظ�ɾ��һ���ѱ�ɾ����Ԫ�飬�������ڵ�Ԫ��");
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
		printf("test failed��\n");
	}
	catch ( StorageEngineExceptionUniversal &ex )
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		if( heap.m_bSuccess )
		{
			printf("test success��\n");
		}
		else
		{
			printf("test failed��\n");
		}
	}

	return heap.m_bSuccess;
}