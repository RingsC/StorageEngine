/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.8.12		 测试事务原子性，插入部分数据时取消事务，另起一个事务检查是否正确回滚
001			许彦      创建		  2011.8.15		 测试事务原子性，插入全部数据完成后，另一个事务查询是否正确插入
002			许彦      创建		  2011.8.17		 测试事务原子性，插入部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚(未完成)

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
	INTENT("测试事务原子性，插入部分数据时取消事务，另起一个事务检查是否正确回滚");
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
			//插入部分数据后，中止当前事务
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
			//printf("插入数据为：	%s\n",insertData);
			insertData[strlen(insert_transaction_data)] = '\0'; //为了连接初始的字符串，需要在该位置设置一个'\0'
		}

		//新起一个事务进行查询
		begin_transaction();

		HeapScanDesc scan;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);

		SAVE_INFO_FOR_DEBUG();
		//如果正确回滚，应该查询不到数据
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple == NULL);
		if(tuple == NULL)
		{
			printf("\n查询结果为空，事务回滚成功，测试成功!\n");
		}
		else
		{
			printf("事务回滚出错，测试失败！\n");
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
	INTENT("测试事务原子性，插入全部数据完成后，另一个事务查询是否正确插入");
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
		
		//完成事务插入全部数据，不取消
		char insertData[50] = insert_transaction_data;
		char copyData[insert_transaction_rows][50]; //存放插入数据，后边查询的时候进行比较
		char string[5];
		int i = 0;
		for(;i < insert_transaction_rows ;i++)
		{
			my_itoa(i,string,10);
			strcat(insertData,string);
			memcpy(copyData[i],insertData,strlen(insertData)+1);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);			
			//printf("插入数据为：	%s\n",insertData);
			insertData[strlen(insert_transaction_data)] = '\0'; //为了连接初始的字符串，需要在该位置设置一个'\0'
		}

		FDPG_Transaction::fd_CommandCounterIncrement();
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		commit_transaction();
		
		//新起一个事务进行查询
		begin_transaction();

		HeapScanDesc scan;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);

		//如果事务正确执行，应该查询到所有插入数据
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
				printf("插入数据与查询出来的数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				dropTable();
				return false;
			}
			else
			{
				//printf("查询数据为：	%s 长度为%d 行数：%d\n",temp,strlen(temp),counter);	
			}	
			++counter;
			++i;
		}

		CHECK_BOOL(counter == insert_transaction_rows);
		if(counter != insert_transaction_rows)
		{
			printf("插入数据行数与返回行数不相等！\n");
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
	INTENT("测试事务原子性，插入部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚");
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
			//插入部分数据后，强制停掉当前进程
			if(i == insert_transaction_rows/2)
			{
				FDPG_Transaction::fd_CommandCounterIncrement();
				exit(1);
			}
			my_itoa(i,string,10);
			strcat(insertData,string);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);			
			//printf("插入数据为：	%s\n",insertData);
			insertData[strlen(insert_transaction_data)] = '\0'; //为了连接初始的字符串，需要在该位置设置一个'\0'
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
	INTENT("测试事务原子性，插入部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚");
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
		//如果正确回滚，应该查询不到数据
		tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);
		CHECK_BOOL(tuple == NULL);
		if(tuple == NULL)
		{
			printf("\n查询结果为空，事务回滚成功，测试成功!\n");
		}
		else
		{
			printf("事务回滚出错，测试失败！\n");
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