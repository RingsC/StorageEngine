/************************************************************************


���Ե��б�
���		��д��      �޸�����      �޸�ʱ��		�޸�����		
000			����      ����		  2011.8.4		 ��scankey������ǰɨ��
001			����	  ����		  2011.8.8		 ��scankey������ǰɨ�裬ʹ�ô��ڡ�С�ں͵��ڲ���
002			����	  ����		  2011.8.11		 ��scankey�������ɨ��
003			����	  ����		  2011.8.11		 ��scankey�������ɨ�裬ʹ�ô��ڡ�С�ں͵��ڲ���


************************************************************************/
#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"

#include "heap/test_select.h"
#include "utils/util.h"
#include "test_fram.h"
#include "utils/fmgroids.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
using namespace std;
using namespace FounderXDB::StorageEngineNS;

extern TupleDesc single_attibute_tupDesc;
extern Datum fdxdb_string_formdatum(const char *p, size_t len);
extern char *my_itoa(int value, char *string, int radix);

int my_compare_select(const char *str1, size_t len1, const char *str2, size_t len2)
{
	int i = 0;
	while(i < len1 && i < len2)
	{
		if(str1[i] < str2[i])
			return -1;
		else if(str1[i] > str2[i])
			return 1;
		else i++;
	}
	if(len1 == len2)
		return 0;
	if(len1 > len2)
		return 1;
	return -1;
}


void my_split_select(RangeData& data, const char *str,int col, size_t len = 0)
{
	memset(&data, 0, sizeof(data));
	data.len = 0;
	data.start = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		data.start = 0;
		data.len = 3;
	}
	if (col == 2)
	{
		data.start = 3;
		data.len = 2;
	}
	if (col == 3)
	{
		data.start = 5;
		data.len = 1;
	}
}

bool test_heap_sqscan_000()
{
	INTENT("��scankey������ǰɨ��");
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
		
		char insertData[50] = test_select_no_scankey;
		char string[5];
		int i = 0;
		for(;i < ROWS ;i++)
		{
			my_itoa(i,string,10);
			strcat(insertData,string);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);
			//printf("��������Ϊ��	%s\n",insertData);
			insertData[strlen(test_select_no_scankey)] = '\0'; //Ϊ�����ӳ�ʼ���ַ�������Ҫ�ڸ�λ������һ��'\0'
		}
		
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select without scankey
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		char * temp;
		int counter = 0;
		int flag = 1;
		i = 0;
		SAVE_INFO_FOR_DEBUG();
		//printf("\n*********�±�Ϊ��ѯ���*********\n\n");
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			my_itoa(i,string,10);
			strcat(insertData,string);
			flag = memcmp(temp,insertData,sizeof(insertData));
			i++;
			insertData[strlen(test_select_no_scankey)] = '\0';
			//CHECK_BOOL( flag == 0);
			if(flag != 0)
			{
				printf("�����������ѯ���������ݲ�һ�£�����ʧ�ܣ�\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			else
			{
				//printf("��ѯ����Ϊ��	%s ����Ϊ%d ������%d\n",temp,strlen(temp),counter);	
			}				
		}

		CHECK_BOOL(counter == ROWS);
		if(counter != ROWS)
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

bool test_heap_sqscan_001()
{
	INTENT("��scankey������ǰɨ�裬ʹ�ô��ڡ�С�ں͵��ڲ���");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
//		Oid reltablespace = MyDatabaseTableSpace;	
		Relation testRelation;
		HeapTuple tuple;

// 		SAVE_INFO_FOR_DEBUG();
// 		ColinfoData colinfo;
// 		colinfo.col_number = NULL;
// 		colinfo.rd_comfunction = NULL;
// 		colinfo.split_function =  my_split_select;

		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		//testRelation = heap_open(relid, reltablespace, RowExclusiveLock, &colinfo);

		char data[20][20] = {"jim70m", "jim60m", "jon30f", "jan60m"};
		for(int i=0;i<4;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = FDPG_Heap::fd_heap_form_tuple(data[i], sizeof(data[i]));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		char *temp;
		HeapScanDesc scan;
		ScanKeyData key[3];
		int flag = 1;
		
		//��ʼ��scankey
		Datum * values1 = (Datum *) palloc(sizeof(Datum));
		values1[0] = fdxdb_string_formdatum("jim", 3);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTEqualStrategyNumber, my_compare_select, values1[0]);
		Datum * values2 = (Datum *) palloc(sizeof(Datum));
		values2[0] = fdxdb_string_formdatum("70", 2);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],2,BTLessStrategyNumber, my_compare_select, values2[0]);
		Datum * values3 = (Datum *) palloc(sizeof(Datum));
		values3[0] = fdxdb_string_formdatum("f", 1);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[2],3,BTGreaterStrategyNumber, my_compare_select, values3[0]);

		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 2, key);
		while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			temp=fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp, "jim60m",sizeof("jim60m"));
			printf("%s\n",temp);
		}

		CHECK_BOOL(flag == 0);
		if(flag != 0)
		{
			printf("��scankey����ʧ��!\n");
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

bool test_heap_sqscan_002()
{
	INTENT("��scankey�������ɨ��");
	int relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
//		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Relation testRelation;
		HeapTuple tuple;
		//open heap and insert some tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		char insertData[50] = test_select_no_scankey;
		char string[5];
		int i = 0;
		for(;i < ROWS ;i++)
		{
			my_itoa(i,string,10);
			strcat(insertData,string);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);			
			//printf("��������Ϊ��	%s\n", insertData);
			insertData[strlen(test_select_no_scankey)] = '\0';
		}

		FDPG_Transaction::fd_CommandCounterIncrement();

		//select without scankey
		HeapScanDesc scan;
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		char * temp;
		int counter = 0;
		int flag = 1;
		i = ROWS-1;

		SAVE_INFO_FOR_DEBUG();
		//printf("\n*********�±�Ϊ��ѯ���*********\n\n");
		while ((tuple = FDPG_Heap::fd_heap_getnext(scan, BackwardScanDirection)) != NULL )
		{        
			++counter;
			temp=fxdb_tuple_to_chars(tuple);
			my_itoa(i,string,10);
			strcat(insertData,string);
			flag = memcmp(temp,insertData,sizeof(insertData));
			i--;
			insertData[strlen(test_select_no_scankey)] = '\0';
			//CHECK_BOOL( flag == 0);
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
		}

		CHECK_BOOL(counter == ROWS);
		if(counter != ROWS)
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

bool test_heap_sqscan_003()
{
	INTENT("��scankey�������ɨ�裬ʹ�ô��ڡ�С�ں͵��ڲ���");
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

		SAVE_INFO_FOR_DEBUG();
		ColinfoData colinfo;
		colinfo.col_number = NULL;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function =  my_split_select;
		
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		//testRelation = heap_open(relid, reltablespace, RowExclusiveLock, &colinfo);

		char data[20][20] = {"jim70m", "jim60m", "jon30f", "jan50m"};
		for(int i=0;i<4;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = FDPG_Heap::fd_heap_form_tuple(data[i], sizeof(data[i]));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		char *temp;
		HeapScanDesc scan;
		ScanKeyData key[3];
		int flag = 1;

		//��ʼ��scankey
		Datum * values1 = (Datum *) palloc(sizeof(Datum));
		values1[0] = fdxdb_string_formdatum("jim", 3);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTEqualStrategyNumber, my_compare_select, values1[0]);
		Datum * values2 = (Datum *) palloc(sizeof(Datum));
		values2[0] = fdxdb_string_formdatum("70", 2);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],2,BTLessStrategyNumber, my_compare_select, values2[0]);
		Datum * values3 = (Datum *) palloc(sizeof(Datum));
		values3[0] = fdxdb_string_formdatum("f", 1);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[2],3,BTGreaterStrategyNumber, my_compare_select, values3[0]);

		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 2, key);
		while((tuple = FDPG_Heap::fd_heap_getnext(scan, BackwardScanDirection)) != NULL)
		{
			temp=fxdb_tuple_to_chars(tuple);
			flag = memcmp(temp, "jim60m",sizeof("jim60m"));
			printf("%s\n",temp);
		}

		CHECK_BOOL(flag == 0);
		if(flag != 0)
		{
			printf("��scankey����ʧ��!\n");
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

bool myTest_test_heap_sqscan_000()
{
 	INTENT("�� scankey ��ǰɨ��");

	try
	{
		//create heap
		HeapFuncTest heap;
		heap.buildHeapColInfo();
		begin_transaction();
		
		//open heap
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );

		//insert data with head "test_select_without_scankey_"
		const int startRow = 0;
		const int endRow = 100;
		char testData[] = "test_select_without_scankey_";
		const int dataLen = sizeof( testData );
		heap.insertRangeTuples( startRow, endRow, testData, dataLen );
 
		//scan tuples inserted
		heap.scanRangeTuplesWithHeader( endRow, testData, dataLen );

		//drop heap
		heap.dropHeap();

		if( heap.m_bSuccess == true )
			END_TRANSACTION
		return heap.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}

bool myTest_test_heap_sqscan_001()
// 3�� scankey ɨ�裬�������ݲ�ȡ����ɨ����ԣ�
// 1~4 �ַ����� "test"��5~8�ַ�С��"2012"�����3���ַ�����"100"
{
	INTENT("��� scankey ��ǰ���ɨ�裬ɨ���������������ڣ����ڣ�С��");
	try
	{
		const int COLUMN = 3;
		const int colLen[COLUMN] = {4, 4, 3};
		const int dataLen = 20;
		char testData[][dataLen] = {"test2012100", "tess2012100", "tesu2012100", "test2011101", "test2011099",
						   "test2011100", "test2013100", "test1996100", "test3213100", "test2011", "test200",
						   "test2012099", "test2012101", "test201299", "test2012250", "test2011abc","test1000"};
		int amount = sizeof(testData) / dataLen;

		//test environment initialization
		HeapFuncTest heap;
		heap.buildHeapColInfo( COLUMN, colLen );
		begin_transaction();

		//open heap
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );

		//insert data
		heap.insertTuples( testData, amount );

		//construct scan keys
		struct ScanKeyInfo scanKey[] = { 
			{ 1, "test", BTEqualStrategyNumber },
			{ 2, "2012", BTLessStrategyNumber },
			{ 3, "100", BTGreaterStrategyNumber }
			//{ 3, "\0", BTGreaterStrategyNumber }
		};
		struct ScanKeyInfo *pScanKey = scanKey;
		const int nkeys = sizeof(scanKey)/sizeof(ScanKeyInfo);
		heap.constructKeys( pScanKey, nkeys );

		//begin scan
		heap.scanWithKey( pScanKey, nkeys );

		//drop heap
		heap.dropHeap();

		if( heap.m_bSuccess == true )
			END_TRANSACTION
		return heap.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}

bool myTest_test_heap_sqscan_002()
//¼������0��9999
//ɨ����򣺵�1���ַ�����"3"����3���ַ�С��"7"����4���ַ�����"4"
{
	INTENT("��� scankey ɨ���ѯ�����е����ݣ��Ӻ���ǰɨ�裬ɨ�������������ڣ����ڣ�С��");
	try
	{
		const int COLUMN = 4;
		const int colLen[COLUMN] = { 1, 1, 1, 1 };

		//test environment initialization
		HeapFuncTest heap;
		heap.buildHeapColInfo( COLUMN, colLen );
		begin_transaction();

		//open heap and insert batch data
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );
		char emptyData[] = "";
		int dataLen = sizeof(emptyData);
		const int endRow = 10000;
		const int startRow = 0;
		heap.insertRangeTuples( startRow, endRow, emptyData, dataLen );

		//initial scan keys
		struct ScanKeyInfo scanKey[] = { 
			{ 1, "3", BTEqualStrategyNumber },
			{ 2, "7", BTLessStrategyNumber },
			{ 4, "4", BTGreaterStrategyNumber }
		};
		struct ScanKeyInfo *pScanKey = scanKey;
		const int nkeys = sizeof(scanKey)/sizeof(ScanKeyInfo);
		heap.constructKeys( pScanKey, nkeys );

		//begin scan
		heap.scanWithKey( pScanKey, nkeys );

		//drop heap
		heap.dropHeap();

		if( heap.m_bSuccess == true )
			END_TRANSACTION
		return heap.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}

bool myTest_test_heap_sqscan_003()
//ͬһ�����ö���ؼ���ɨ������
//ע�⣺������һ���ؼ���ɨ��������С�ڣ��ǵ��ڹؼ����������ƥ������ޣ����򱨴�
{
	INTENT("ͬһ�����ö�� scankey ��ɨ���������������ڣ����ڣ�С��");
	try
	{
		const int COLUMN = 5;
		const int colLen[COLUMN] = { 1, 2, 1, 1 };

		//test environment initialization
		HeapFuncTest heap;
		heap.buildHeapColInfo( COLUMN, colLen );
		begin_transaction();

		//open heap and insert batch data
		SAVE_INFO_FOR_DEBUG();
		heap.openHeap( RowExclusiveLock );
		char emptyData[] = "\0";
		int dataLen = sizeof(emptyData);
		const int startRow = 0;
		const int endRow = 5000;
		heap.insertRangeTuples( startRow, endRow, emptyData, dataLen );


		//initial scan keys
		struct ScanKeyInfo scanKey[] = { 
			{ 1, "1", BTEqualStrategyNumber },
			{ 2, "20", BTLessStrategyNumber },
			{ 2, "15", BTGreaterEqualStrategyNumber },
			{ 3, "\0", BTGreaterStrategyNumber },
			{ 3, "2", BTLessStrategyNumber }
		};
		struct ScanKeyInfo *pScanKey = scanKey;
		const int nkeys = sizeof(scanKey)/sizeof(ScanKeyInfo);
		heap.constructKeys( pScanKey, nkeys );

		//begin scan
		heap.scanWithKey( pScanKey, nkeys );

		//drop heap
		heap.dropHeap();

		if( heap.m_bSuccess == true )
			END_TRANSACTION
		return heap.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}