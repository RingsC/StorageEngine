/************************************************************************


���Ե��б�
���		��д��      �޸�����      �޸�ʱ��		�޸�����		
000			����      ����		  2011.8.12		 ��������ԭ���ԣ����벿������ʱȡ����������һ���������Ƿ���ȷ�ع�
001			����      ����		  2011.8.15		 ��������ԭ���ԣ�����ȫ��������ɺ���һ�������ѯ�Ƿ���ȷ����
002			����      ����		  2011.8.17		 ��������ԭ���ԣ����벿������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�(δ���)

************************************************************************/

#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"

#include "test_fram.h"
#include "utils/util.h"
#include "transaction/test_transaction_insert.h"
#include "utils/fmgroids.h"
#include "interface/FDPGAdapter.h"
#include "iostream"
#include "interface/StorageEngineExceptionUniversal.h"
using namespace std;
using namespace FounderXDB::StorageEngineNS;
extern char *my_itoa(int value, char *string, int radix);

void mutil_begin_test()
{
	
}

void mutil_end_test()
{

}

bool test_transaction_insert_000()
{
	INTENT("��������ԭ���ԣ����벿������ʱȡ����������һ���������Ƿ���ȷ�ع�");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = RID;
		Relation testRelation;
		HeapTuple tuple;

		//open heap and insert some tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		char insertData[50] = insert_transaction_data;
		char string[5];
		int i = 0;
		for(;i < insert_transaction_rows ;i++)
		{
			//���벿�����ݺ���ֹ��ǰ����
			if(i == insert_transaction_rows/2)
			{
				FDPG_Transaction::fd_CommandCounterIncrement();
				user_abort_transaction();
				break;
			}
			my_itoa(i,string,10);
			strcat(insertData,string);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);			
			//printf("��������Ϊ��	%s\n",insertData);
			insertData[strlen(insert_transaction_data)] = '\0'; //Ϊ�����ӳ�ʼ���ַ�������Ҫ�ڸ�λ������һ��'\0'
		}

		//����һ��������в�ѯ
		begin_transaction();

		HeapScanDesc scan;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);

		SAVE_INFO_FOR_DEBUG();
		//�����ȷ�ع���Ӧ�ò�ѯ��������
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple == NULL);
		if(tuple == NULL)
		{
			printf("\n��ѯ���Ϊ�գ�����ع��ɹ������Գɹ�!\n");
		}
		else
		{
			printf("����ع���������ʧ�ܣ�\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}
	dropTable();
	return true;
}

bool test_transaction_insert_001()
{
	INTENT("��������ԭ���ԣ�����ȫ��������ɺ���һ�������ѯ�Ƿ���ȷ����");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = RID;
		Relation testRelation;
		HeapTuple tuple;

		//open heap and insert some tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		
		//����������ȫ�����ݣ���ȡ��
		char insertData[50] = insert_transaction_data;
		char copyData[insert_transaction_rows][50]; //��Ų������ݣ���߲�ѯ��ʱ����бȽ�
		char string[5];
		int i = 0;
		for(;i < insert_transaction_rows ;i++)
		{
			my_itoa(i,string,10);
			strcat(insertData,string);
			memcpy(copyData[i],insertData,strlen(insertData)+1);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);			
			//printf("��������Ϊ��	%s\n",insertData);
			insertData[strlen(insert_transaction_data)] = '\0'; //Ϊ�����ӳ�ʼ���ַ�������Ҫ�ڸ�λ������һ��'\0'
		}

		FDPG_Transaction::fd_CommandCounterIncrement();
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
		
		//����һ��������в�ѯ
		begin_transaction();

		HeapScanDesc scan;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);

		//���������ȷִ�У�Ӧ�ò�ѯ�����в�������
		char * temp;
		int counter = 0;
		int flag = 1;
		i = 0;
		SAVE_INFO_FOR_DEBUG();
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )
		{        
			temp = fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp,copyData[i],strlen(temp)+1);		
			CHECK_BOOL( flag == 0);
			if(flag != 0)
			{
				printf("�����������ѯ���������ݲ�һ�£�����ʧ�ܣ�\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				dropTable();
				return false;
			}
			else
			{
				//printf("��ѯ����Ϊ��	%s ����Ϊ%d ������%d\n",temp,strlen(temp),counter);	
			}	
			++counter;
			++i;
		}

		CHECK_BOOL(counter == insert_transaction_rows);
		if(counter != insert_transaction_rows)
		{
			printf("�������������뷵����������ȣ�\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			dropTable();
			return false;
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		dropTable();
		return false;
	}
	dropTable();
	return true;
}

bool test_transaction_insert_002_step1()
{
	INTENT("��������ԭ���ԣ����벿������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�");
	try
	{
		begin_first_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = 250000;
		Relation testRelation;
		HeapTuple tuple;

		FDPG_Heap::fd_heap_create(reltablespace, relid);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//open heap and insert some tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		char insertData[50] = insert_transaction_data;
		char string[5];
		int i = 0;
		for(;i < insert_transaction_rows ;i++)
		{
			//���벿�����ݺ�ǿ��ͣ����ǰ����
			if(i == insert_transaction_rows/2)
			{
				FDPG_Transaction::fd_CommandCounterIncrement();
				exit(1);
			}
			my_itoa(i,string,10);
			strcat(insertData,string);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);			
			//printf("��������Ϊ��	%s\n",insertData);
			insertData[strlen(insert_transaction_data)] = '\0'; //Ϊ�����ӳ�ʼ���ַ�������Ҫ�ڸ�λ������һ��'\0'
		}

		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
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

bool test_transaction_insert_002_step2()
{
	INTENT("��������ԭ���ԣ����벿������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�");
	try
	{
		begin_first_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = 250000;
		Relation testRelation;
		HeapTuple tuple;

		SAVE_INFO_FOR_DEBUG();
		HeapScanDesc scan;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);

		SAVE_INFO_FOR_DEBUG();
		//�����ȷ�ع���Ӧ�ò�ѯ��������
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple == NULL);
		if(tuple == NULL)
		{
			printf("\n��ѯ���Ϊ�գ�����ع��ɹ������Գɹ�!\n");
		}
		else
		{
			printf("����ع���������ʧ�ܣ�\n");
			FDPG_Heap::fd_heap_endscan(scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			user_abort_transaction();
			return false;
		}

		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
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