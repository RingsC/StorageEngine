#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"

#include "test_fram.h"
#include "utils/util.h"
#include "transaction/test_transaction_delete.h"
#include "utils/fmgroids.h"
#include "interface/FDPGAdapter.h"
#include "iostream"
#include "interface/StorageEngineExceptionUniversal.h"
using namespace std;
using namespace FounderXDB::StorageEngineNS;
extern char *my_itoa(int value, char *string, int radix);

#define data  "insert_transaction_data"
#define data_rows 10000

bool test_transaction_delete_atom()
{
	INTENT("����ɾ������ԭ���ԣ�ɾ��ȫ��������ɺ���һ�������ѯ�Ƿ���ȷɾ����");
	try
	{
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//����

		begin_transaction();//������һ�����񣬲���һЩԪ��
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = THREAD_RID;
		Oid ret = 0;
		Relation testRelation;
		HeapTuple tuple;
		HeapScanDesc scan;
		char insertData[50] = data;
		ItemPointer tid;
		char string[5];
		int i = 0;

		//open heap and insert some tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//��heap
		for(;i < data_rows ;i++)
		{
			my_itoa(i,string,10);
			strcat(insertData,string);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//����Ԫ��
			if (tuple== NULL)
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			ret=FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);//����Ԫ��
			if(ret!=0)
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			insertData[strlen(data)] = '\0'; //Ϊ�����ӳ�ʼ���ַ�������Ҫ�ڸ�λ������һ��'\0'
		}
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//�ر�heap
			commit_transaction();//�ύ����

			begin_transaction();//�����ڶ�������ɾ�����е�heap
			testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//��heap
			scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);//������һ��ɨ��
			while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )//fetch
			{
			tid = &(tuple->t_self);
			FDPG_Heap::fd_simple_heap_delete(testRelation, tid);//ɾ��Ԫ��
			}			
			FDPG_Heap::fd_heap_endscan(scan);//����ɨ��
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//�ر�heap
			commit_transaction();//�ύ����

			begin_transaction();//��������������
								//���������ȷִ�У�Ӧ�ò�ѯ�����в������ݶ���ɾ��
			testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//��heap
			scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
			if ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) == NULL )//fetch
			{
				printf("������ȷִ�У��������ݶ����ɹ�ɾ��!\n");
			}
			else //������
			{
				printf("����û����ȫɾ��,����ʧ��!\n");
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			FDPG_Heap::fd_heap_endscan(scan);//����ɨ��
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//�ر�heap
			commit_transaction();

			remove_heap(THREAD_RID);//ɾ��

	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	return true;
}

bool test_transaction_delete_atom_halfdelete()
{
	INTENT("����ɾ������ԭ���ԣ�ɾ������������ɺ������жϣ���һ�������ѯɾ���Ƿ�ع���");
	try
	{
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//����

		begin_transaction();//������һ��������heap�����Ԫ��
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = THREAD_RID;
		Oid ret = 0;
		Relation testRelation;
		HeapTuple tuple;
		HeapScanDesc scan;
		char * temp;
		char insertData[50] = data;
		ItemPointer tid;//deleteָ��
		char string[5];
		int i = 0;
		char storedata[data_rows][50];//��������Ԫ��
		int cmp=0;//memcpy�Ƚϵķ��ؽ��
		int counter=0;//����storedata�м���

		//open heap and insert some tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//��heap
		for(;i < data_rows ;i++)
		{
			my_itoa(i,string,10);
			strcat(insertData,string);
			memcpy(storedata[i],insertData,sizeof(insertData));
			//strcpy(storedata[i][50],insertData);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//����Ԫ��
			if (tuple== NULL)
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			ret=FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);//����Ԫ��
			if(ret!=0)
			{				
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			insertData[strlen(data)] = '\0'; //Ϊ�����ӳ�ʼ���ַ�������Ҫ�ڸ�λ������һ��'\0'
		}
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//�ر�heap
		commit_transaction();//�ύ����


		begin_transaction();//�����ڶ�������ɾ��һЩheap��Ȼ���ж�
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//��heap
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);//ɨ��
		for (i=0;i < data_rows ;i++)
		{
			if(i == data_rows/2)
			{
				user_abort_transaction();
				break;
			}
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);//fetch
			tid = &(tuple->t_self);
			FDPG_Heap::fd_simple_heap_delete(testRelation, tid);//ɾ��Ԫ��
		}

		begin_transaction();//��������������
		//�����ȷ�ع���Ӧ�ò�ѯ�����в������ݶ�û�б�ɾ��
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//��heap
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )//fetch
		{	
			temp=fxdb_tuple_to_chars(tuple);//Ԫ��ת��Ϊ�ַ�
			cmp=memcmp(storedata[counter],temp,sizeof(temp));//�����������ݺʹ洢�����ݽ��бȽ�
			if(cmp != 0)
			{
				printf("ɾ������ִ��֮�����ݲ�һ�£�����ʧ�ܣ�\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				return false;
			}
			else
			{
				counter++;
			}	
		}
			CHECK_BOOL(counter == data_rows);//�����Ƚ�
			if(counter != data_rows)
			{
				printf("�����뷵����������ȣ�\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			else
			{
				printf("ɾ������ع��ɹ�!\n");
			}
		FDPG_Heap::fd_heap_endscan(scan);//����ɨ��
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//�ر�heap
		commit_transaction();

		remove_heap(THREAD_RID);//ɾ��

	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	return true;
}