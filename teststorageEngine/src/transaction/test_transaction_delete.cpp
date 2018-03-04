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
	INTENT("测试删除事务原子性，删除全部数据完成后，另一个事务查询是否正确删除。");
	try
	{
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//创建表的colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//建表

		begin_transaction();//启动第一个事务，插入一些元组
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
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//打开heap
		for(;i < data_rows ;i++)
		{
			my_itoa(i,string,10);
			strcat(insertData,string);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//构建元组
			if (tuple== NULL)
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			ret=FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);//插入元组
			if(ret!=0)
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			insertData[strlen(data)] = '\0'; //为了连接初始的字符串，需要在该位置设置一个'\0'
		}
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//关闭heap
			commit_transaction();//提交事务

			begin_transaction();//启动第二个事务，删除所有的heap
			testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//打开heap
			scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);//新启动一个扫描
			while ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )//fetch
			{
			tid = &(tuple->t_self);
			FDPG_Heap::fd_simple_heap_delete(testRelation, tid);//删除元组
			}			
			FDPG_Heap::fd_heap_endscan(scan);//结束扫描
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//关闭heap
			commit_transaction();//提交事务

			begin_transaction();//启动第三个事务
								//如果事务正确执行，应该查询到所有插入数据都被删除
			testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//打开heap
			scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
			if ((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) == NULL )//fetch
			{
				printf("事务正确执行，所有数据都被成功删除!\n");
			}
			else //出错处理
			{
				printf("数据没有完全删除,测试失败!\n");
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			FDPG_Heap::fd_heap_endscan(scan);//结束扫描
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//关闭heap
			commit_transaction();

			remove_heap(THREAD_RID);//删表

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
	INTENT("测试删除事务原子性，删除部分数据完成后，事务中断，另一个事务查询删除是否回滚。");
	try
	{
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//创建表的colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//建表

		begin_transaction();//启动第一个事务，往heap中添加元组
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		Oid relid = THREAD_RID;
		Oid ret = 0;
		Relation testRelation;
		HeapTuple tuple;
		HeapScanDesc scan;
		char * temp;
		char insertData[50] = data;
		ItemPointer tid;//delete指针
		char string[5];
		int i = 0;
		char storedata[data_rows][50];//保存插入的元组
		int cmp=0;//memcpy比较的返回结果
		int counter=0;//用于storedata行计数

		//open heap and insert some tuple
		SAVE_INFO_FOR_DEBUG();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//打开heap
		for(;i < data_rows ;i++)
		{
			my_itoa(i,string,10);
			strcat(insertData,string);
			memcpy(storedata[i],insertData,sizeof(insertData));
			//strcpy(storedata[i][50],insertData);
			tuple = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//构建元组
			if (tuple== NULL)
			{
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			ret=FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);//插入元组
			if(ret!=0)
			{				
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			insertData[strlen(data)] = '\0'; //为了连接初始的字符串，需要在该位置设置一个'\0'
		}
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//关闭heap
		commit_transaction();//提交事务


		begin_transaction();//启动第二个事务，删除一些heap，然后中断
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//打开heap
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);//扫描
		for (i=0;i < data_rows ;i++)
		{
			if(i == data_rows/2)
			{
				user_abort_transaction();
				break;
			}
			tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection);//fetch
			tid = &(tuple->t_self);
			FDPG_Heap::fd_simple_heap_delete(testRelation, tid);//删除元组
		}

		begin_transaction();//启动第三个事务
		//如果正确回滚，应该查询到所有插入数据都没有被删除
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);//打开heap
		scan = FDPG_Heap::fd_heap_beginscan(testRelation, SnapshotNow, 0, NULL);
		while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL )//fetch
		{	
			temp=fxdb_tuple_to_chars(tuple);//元组转换为字符
			cmp=memcmp(storedata[counter],temp,sizeof(temp));//读出来的数据和存储的数据进行比较
			if(cmp != 0)
			{
				printf("删除事务执行之后数据不一致！测试失败！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				return false;
			}
			else
			{
				counter++;
			}	
		}
			CHECK_BOOL(counter == data_rows);//行数比较
			if(counter != data_rows)
			{
				printf("行数与返回行数不相等！\n");
				FDPG_Heap::fd_heap_endscan(scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				user_abort_transaction();
				return false;
			}
			else
			{
				printf("删除事务回滚成功!\n");
			}
		FDPG_Heap::fd_heap_endscan(scan);//结束扫描
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);//关闭heap
		commit_transaction();

		remove_heap(THREAD_RID);//删表

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