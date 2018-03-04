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
#include "heap/test_heap_open.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/utils.h"
#include "DataItem.h"
using namespace std;
using namespace FounderXDB::StorageEngineNS;
 int rid =16665;
 int thread_rid = 40000;
 #define test_data "open disk heap and insert data"
 //����һ�ű�
 void startTest_CreateHeap()
 {
 	try
 	{
 		using namespace FounderXDB::StorageEngineNS;
 		using namespace boost;
 
		SpliterGenerater sg;
 		const int COL_NUM = 2;
 
 		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,2,2);//�������colinfo
 		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//����

 	}
 	catch (StorageEngineExceptionUniversal &ex) {
 		cout << ex.getErrorNo() << endl;
 		cout << ex.getErrorMsg() << endl;
 		user_abort_transaction();
 	}
 }
 
 void finishTest_DropHeap()
 {
 	try
 	{
		int rid=THREAD_RID;
		int index_id=INDEX_ID;
		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//ɾ��
 	}
 	catch (StorageEngineExceptionUniversal &ex) {
 		cout << ex.getErrorNo() << endl;
 		cout << ex.getErrorMsg() << endl;
 		user_abort_transaction();
 	}
 
 }
 
 void ThreadCreateTable(int nTables)
 {
	 try
	 {
		 begin_first_transaction();
		 Oid relspace = DEFAULTTABLESPACE_OID;
		 SAVE_INFO_FOR_DEBUG();
		 for(int rid = thread_rid;rid < thread_rid+nTables;rid++)
		 {
			 FDPG_Heap::fd_heap_create(relspace, rid);
		 }		
		 FDPG_Transaction::fd_CommandCounterIncrement();
		 commit_transaction();
	 }
	 catch (StorageEngineExceptionUniversal &ex) 
	 {
		 user_abort_transaction();
		 cout << ex.getErrorNo() << endl;
		 cout << ex.getErrorMsg() << endl;
	 }	
 }

 void ThreadDropTable(int nTables)
 {
	 try
	 {
		 begin_transaction();
		 for(int rid = thread_rid;rid < thread_rid+nTables;rid++)
		 {
			 FDPG_Heap::fd_heap_drop(rid, MyDatabaseId);
		 }		
		 commit_transaction();
	 }	
	 catch (StorageEngineExceptionUniversal &ex) 
	 {
		 user_abort_transaction();
		 cout << ex.getErrorNo() << endl;
		 cout << ex.getErrorMsg() << endl;
	 }
	 thread_rid+=nTables;
 }
 
 bool test_heap_open()
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
 		INTENT("����FDPG_Heap::fd_heap_open�ܷ��heap��");
  
 		try
 		{	extern Colinfo getColInfo(Oid);
					Colinfo ii = getColInfo(relid);
 		begin_transaction();//��RowExclusiveLock��heap_open���
 		Relation testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
 		CHECK_BOOL(testRelation != NULL);
 		if(testRelation == NULL)
 		{
 			printf("��heapʧ��!\n");
 			user_abort_transaction();
 			return false;
 		}
 		FDPG_Transaction::fd_CommandCounterIncrement();
 		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
 		commit_transaction();
 		}
 		catch (StorageEngineExceptionUniversal &ex) {
 			cout << ex.getErrorNo() << endl;
 			cout << ex.getErrorMsg() << endl;
 			user_abort_transaction();
 			return false;
 		}
 
 		try
 		{	
 		printf("��δ򿪹رյ��������Ͳ�һ�±�����(Nolock����):\n");
 		begin_transaction();//��RowShareLock��heap_open���
 		Relation testRelation2 = FDPG_Heap::fd_heap_open(relid,RowShareLock, MyDatabaseId);
 		CHECK_BOOL(testRelation2 != NULL);
 		if(testRelation2 == NULL)
 		{
 			printf("��heapʧ��!\n");
 			user_abort_transaction();
 			return false;
 		}
 		FDPG_Transaction::fd_CommandCounterIncrement();
 		FDPG_Heap::fd_heap_close(testRelation2, RowShareLock);
 		commit_transaction();
 		}
 		catch (StorageEngineExceptionUniversal &ex) {
 			cout << ex.getErrorNo() << endl;
 			cout << ex.getErrorMsg() << endl;
 			user_abort_transaction();
 			return false;
 		}
 
 		try
 		{	
 		begin_transaction();//��ShareLock��heap_open���
 		Relation testRelation3 = FDPG_Heap::fd_heap_open(relid,ShareLock, MyDatabaseId);
 		CHECK_BOOL(testRelation3 != NULL);
 		if(testRelation3 == NULL)
 		{
 			printf("��heapʧ��!\n");
 			user_abort_transaction();
 			return false;
 		}
 		FDPG_Transaction::fd_CommandCounterIncrement();
 		FDPG_Heap::fd_heap_close(testRelation3, ShareLock);
 		commit_transaction();
 		}
 		catch (StorageEngineExceptionUniversal &ex) {
 			cout << ex.getErrorNo() << endl;
 			cout << ex.getErrorMsg() << endl;
 			user_abort_transaction();
 			return false;
 		}
 
 		try
 		{
 		begin_transaction();//��NoLock��heap_open���
 		Relation testRelation4 = FDPG_Heap::fd_heap_open(relid,NoLock, MyDatabaseId);
 		CHECK_BOOL(testRelation4 != NULL);
 		if(testRelation4 == NULL)
 		{
 			printf("��heapʧ��!\n");
 			user_abort_transaction();
 			return false;
 		}
 		FDPG_Transaction::fd_CommandCounterIncrement();
 		FDPG_Heap::fd_heap_close(testRelation4, NoLock);
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
 
 bool test_heap_open_notexists()
 {
	 INTENT("��FDPG_Heap::fd_heap_openȥ��һ�������ڵı�,Ӧ�÷���NULL");
	 try
	 {	
		 begin_first_transaction();//��RowExclusiveLock��heap_open���
		 Relation testRelation = relation_open(65535, RowExclusiveLock);
		 CHECK_BOOL(testRelation == NULL);
		 commit_transaction();
		 return true;
	 }
	 catch (StorageEngineExceptionUniversal &ex) {
		 cout << ex.getErrorNo() << endl;
		 cout << ex.getErrorMsg() << endl;
		 user_abort_transaction();
		 return false;
	 }
 }

 bool test_heap_open_mult_close()
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
 
 	try
 	{	
 		begin_transaction();
 		Oid reltablespace = DEFAULTTABLESPACE_OID;
 		Relation testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
 		CHECK_BOOL(testRelation != NULL);
 		if(testRelation == NULL)
 		{
 			printf("��heapʧ��!\n");
 			user_abort_transaction();
 			return false;
 		}
 		FDPG_Transaction::fd_CommandCounterIncrement();
 		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);	
 		commit_transaction();
 	}
 	catch (StorageEngineExceptionUniversal &ex) {
 		cout << ex.getErrorNo() << endl;
 		cout << ex.getErrorMsg() << endl;
 		user_abort_transaction();
 		return false;
 	}
 
 	try
 	{	
 		begin_transaction();
 		Oid reltablespace2 = DEFAULTTABLESPACE_OID;
 		Relation testRelation2 = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
 		CHECK_BOOL(testRelation2 != NULL);
 		if(testRelation2 == NULL)
 		{
 			printf("��heapʧ��!\n");
 			user_abort_transaction();
 			return false;
 		}
 		FDPG_Heap::fd_heap_close(testRelation2, RowExclusiveLock);	
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
 
 bool test_heap_open_mult_withoutclose()
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
 	INTENT("����FDPG_Heap::fd_heap_open���رյ�����¶�δ�heap��");
 
 	try
 	{	
 		begin_transaction();
 		Oid reltablespace = DEFAULTTABLESPACE_OID;
 		Relation testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
 		CHECK_BOOL(testRelation != NULL);
 		if(testRelation == NULL)
 		{
 			printf("��heapʧ��!\n");
 			user_abort_transaction();
 			return false;
 		}
 		printf("���ر�heap������´�ͬһ��heap������:\n");
 		commit_transaction();
 	}
 	catch (StorageEngineExceptionUniversal &ex) {
 		cout << ex.getErrorNo() << endl;
 		cout << ex.getErrorMsg() << endl;
 		user_abort_transaction();
 		return false;
 	}
 
 	try
 	{	
 		begin_transaction();
 		Oid reltablespace2 = DEFAULTTABLESPACE_OID;
 		Relation testRelation2 = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
 		CHECK_BOOL(testRelation2 != NULL);
 		if(testRelation2 == NULL)
 		{
 			printf("��heapʧ��!\n");
 			user_abort_transaction();
 			return false;
 		}
 		FDPG_Heap::fd_heap_close(testRelation2, RowExclusiveLock);	
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

#define THREAD_NUMS_10 10
 bool test_thread_heap_open()
 {
	 INTENT("��������̣߳�ÿ���̴߳򿪱���֤");
	 ++THREAD_RID;
	 Oid rid = THREAD_RID;
	 SpliterGenerater sg;
	 Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);//��������������,��Ϊֻ�Ǽ򵥲������ݣ�û�õ�colinfo
	 thread_create_heap_xy(THREAD_NUMS_10,rid,heap_info);
	 boost::thread_group tg;
	 BackendParameters *paramArr[THREAD_NUMS_10];	
	 prepare_param(paramArr,THREAD_NUMS_10);

	 //�����߳�
	 for(int i = 0;i < THREAD_NUMS_10;i++)
	 {
		 tg.create_thread(boost::bind(&thread_simple_heap_open,(void*)paramArr[i],rid+i));
	 }

	 tg.join_all();
	 free_param(paramArr,THREAD_NUMS_10);

	 thread_drop_heap(THREAD_NUMS_10);
	 return true;
 }

static
void thread_open_wait(BackendParameters *GET_PARAM(), Oid relid, Oid index_id, 
											boost::shared_mutex *other_mutex, boost::shared_mutex *my_mutex, bool *sta)
{
	fxdb_SubPostmaster_Main(param);
	*sta = true;

	do 
	{
		try
		{
			begin_transaction();
			/* ���Դ��Ѵ��ڵı�Ŀ���ǽ���ľ�����浽RelationIdCache�� */
			Relation rel = FDPG_Heap::fd_heap_open(relid, AccessShareLock);
			Relation index_rel = FDPG_Index::fd_index_open(index_id, AccessShareLock);
			FDPG_Heap::fd_heap_close(rel, NoLock);
			FDPG_Index::fd_index_close(index_rel, NoLock);
			commit_transaction();
			/* ����ɾ������߳̿���ɾ������ */
			my_mutex->unlock();
			/* �ȴ�ɾ�����߳�ɾ���� */
			other_mutex->lock_shared();
			other_mutex->unlock_shared();
		} catch (FounderXDB::EXCEPTION::XdbBaseException &se)
		{
			*sta = false;
			printf("%s\n", se.what());
			user_abort_transaction();
			break;
		}

		/* ���Ѿ�ɾ������δ򿪱�ᱨ�쳣 */
		try
		{
			begin_transaction();
			Relation rel = FDPG_Heap::fd_heap_open(relid, AccessShareLock);
			*sta = false;
			commit_transaction();
			break;
		} catch (FounderXDB::EXCEPTION::XdbBaseException &se)
		{
			*sta = true;
			printf("%s\n", se.what());
			user_abort_transaction();
		}
		/* ����Ҳ�Ѿ�ɾ����������ʧ�ܲ��ᱨ�쳣 */
		try
		{
			begin_transaction();
			Relation rel = FDPG_Index::fd_index_open(index_id, AccessShareLock);
			if (rel == NULL)
				*sta = true;
			else
				*sta == false;
			commit_transaction();
		} catch (FounderXDB::EXCEPTION::XdbBaseException &se)
		{
			*sta = false;
			printf("%s\n", se.what());
			user_abort_transaction();
		}
	} while (false);

	proc_exit(0);
}

static
void thread_drop_wait(BackendParameters *GET_PARAM(), Oid relid,
											boost::shared_mutex *other_mutex, int lock_num, 
											boost::shared_mutex *my_mutex, bool *sta)
{
	fxdb_SubPostmaster_Main(param);
	*sta = true;

	do 
	{
		/* �ȴ������̻߳����ľ����RelationIdCache�� */
		extern bool wait_for_other_thread(boost::shared_mutex *n_lock, int nlock_len, int sec);
		for (int i = 0; i < lock_num; ++i)
		{
			other_mutex[i].lock();
			other_mutex[i].unlock();
		}
		try
		{
			/* ������ϣ�ɾ���� */
			begin_transaction();
			FDPG_Heap::fd_heap_drop(relid);
			commit_transaction();
			/* ��֪�����߳�ɾ������ */
			my_mutex->unlock();	
		}	catch (FounderXDB::EXCEPTION::XdbBaseException &se)
		{
			*sta = false;
			printf("%s\n", se.what());
			user_abort_transaction();;
		}
	} while (false);

	proc_exit(0);
}

EXTERN_SIMPLE_FUNCTION

int test_thread_heap_drop_and_open()
{
	INTENT("��������߳�ȥ��һ�����ڵı�ʹ�仺���RelationIdCache�С�"
		"���һ���߳�ɾ��������������Ƿ������̶߳��ܽ��ܵ��źŸ���"
		"�Լ���RelationIdCache��Ϣ��");
	using namespace boost;
	PREPARE_TEST();

	int return_sta = 1;
	const int THREADS = 5;
	shared_mutex mutex[THREADS];
	bool sta[THREADS];
	clear_all();
	Oid relid = get_heap_id();
	Oid index_id = get_index_id();

	for (int i = 0; i < THREADS; ++i)
	{
		mutex[i].lock();
	}

	{
		int create_sta = 0;
		Colinfo heap_info = (Colinfo) malloc (sizeof(ColinfoData));
		memset(heap_info, 0, sizeof(ColinfoData));
		SIMPLE_CREATE_HEAP(relid, create_sta);
		SIMPLE_CREATE_INDEX(relid, index_id, heap_info, heap_info, create_sta);
	}

	GET_PARAM() = get_param();
	SAVE_PARAM(GET_PARAM());
	GET_THREAD_GROUP().create_thread(boost::bind(&thread_drop_wait, 
		GET_PARAM(), relid, mutex, THREADS - 1, &mutex[THREADS - 1], &sta[THREADS - 1]));

	for (int i = 0; i < THREADS - 1; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(boost::bind(&thread_open_wait, 
			GET_PARAM(), relid, index_id, &mutex[THREADS - 1], &mutex[i], &sta[i]));
	}
	GET_THREAD_GROUP().join_all();

	FREE_PARAM(BackendParameters *);

	for(int i = 0; i < THREADS; ++i)
	{
		if (!sta[i])
		{
			return_sta = 0;
			break;
		}
	}

	return return_sta;
}

 void thread_simple_heap_open(void *param,Oid rid)
 {
	 fxdb_SubPostmaster_Main(param);
	 INTENT("ͨ�����ص�testRelationȷ��fd_heap_open����ȷִ�С�");
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

 void thread_open(const int relid, BackendParameters *param, int *sta)
 {
	 using namespace FounderXDB::StorageEngineNS;

	 try
	 {
		 extern void* fxdb_SubPostmaster_Main(void*);
		 SAVE_INFO_FOR_DEBUG();
		 fxdb_SubPostmaster_Main((void*)param);

		 begin_transaction();
		 Relation rel = FDPG_Heap::fd_heap_open(relid,AccessShareLock, MyDatabaseId);
		 if(rel == NULL)
		 {
			 *sta = 1;
		 }else
		 {
			 *sta = -1;
		 }
		 FDPG_Heap::fd_heap_close(rel, AccessShareLock);
		 commit_transaction();
	 }catch(StorageEngineExceptionUniversal &se)
	 {
		 printf("%d:%s\n", se.getErrorNo(), se.getErrorMsg());
		 user_abort_transaction();
		 *sta = -1;
	 }
 }
  
