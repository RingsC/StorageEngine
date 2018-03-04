/*
����heap_drop���ö���̵ķ���������һ���ӽ��̣���test1��startEngineȻ��heap_create 
��heap_drop ��stopEngine,ͬʱ�����˸��ӽ��̡��������У���test2������startEngine����
��heap_open��򿪲����ڵ�heap��heap_drop�ɹ���
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
	INTENT("����heap_create��heap_drop�Ĺ����Ƿ����ơ�");
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
	INTENT("����heap_drop�ܷ�ɾ��heap��");
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

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//����
		Oid relid= THREAD_RID;
 		INTENT("����FDPG_Heap::fd_heap_open�ܷ��δ򿪹ر�heap��");
 		begin_transaction();
 		Oid reltablespace = DEFAULTTABLESPACE_OID;
 
 		FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		printf("û�йر�heap�������drop��heap������:\n");
 		commit_transaction();
		remove_heap(THREAD_RID);//ɾ��
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
	INTENT("��������߳�ȥɾ��ͬһ�ű����Ե���ȷ���"
				 "��ȫ�����̶߳�ɾ���ɹ���");

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
	INTENT("��������̣߳����в����̶߳Ա�����ѯ����������"
				 "�̶߳Ա���ɾ�������������̲߳���ͬһ�ű�����"
				 "����ȷ����ǲ�ѯ�߳��Ƿ���ҵ������������Ƿ���"
				 "��ɾ���߳���ס��");

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
	*����ɾ���Ͳ�ѯ�߳�
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

	//���Գɹ�
	if(drop_sta[0] == 1 && drop_sta[0] == 1 && drop_sta[0] == 1)
	{
		return 1;
	}
	return 0;
}

int test_thread_heap_drop_002()
{
	INTENT("��������̣߳����в����߳���drop table����������"
				 "�߳�������������������ɹ��ļ�������������߳���"
				 "��drop table�߳�ִ�У���ô���������������ִ�У�"
				 "���߻ᱨ�쳣��(�������������������е�drop������"
				 "insert�������ǳɶԲ���ͬһ�ű�");

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
		* �������drop�߳�
		*/
		for(int i = 0; i < THREAD_NUM_3; ++i)
		{
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			tg.create_thread(bind(&thread_heap_drop, rel_id[i], GET_PARAM(), &drop_sta[i]));
		}

		/*
		* ��������������߳�
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
	* ���������ȷ���Ӧ���ǲ����߳��������ɾ���̻߳�ȡ��
	* ���������ô�����̻߳�����ɾ���̵߳�ִ�У�ֱ�������߳�
	* ���������ݶ�����ɹ���Ż�ִ��ɾ������������heap���ǿ�
	* �ģ�û���κ����ݾͱ�ɾ���ˡ�
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
	* �򿪱�ɨ�裬��ʱ���Ѿ���ɾ��
	*/
	if(!(drop_sta[0] == 1 && drop_sta[1] == 1 && drop_sta[2] == 1))
	{
		test_success = false;
	}
#undef ROW
	return test_success;
}