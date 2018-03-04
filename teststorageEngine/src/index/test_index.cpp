/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.8.18		 两字段索引，仅向前扫描，索引列为第1列和第2列
001			许彦      创建		  2011.8.22		 两字段索引，仅向后扫描，改变索引列为第2列和第3列
002			许彦      创建		  2011.8.22		 两字段索引，仅向前扫描，改变索引列顺序为先后列再前列，如索引列为先第2列后第1列
003			许彦      创建		  2011.8.24		 五字段索引，仅向前扫描，索引列为2,3,5,4,1,使用5种不同策略
004			许彦      创建		  2011.8.24		 32字段索引，仅向前扫描，索引列为1-32，插入大量数据

005			许彦      创建		  2011.11.14	 多线程测试，先插入数据再建索引，起多个线程向前扫描，索引列为第1列和第2列
006			许彦      创建		  2011.11.14	 多线程测试，先建立索引再插数据，再更新索引，起多个线程向前扫描，索引列为第1列和第2列
007			许彦      创建		  2011.11.14	 多线程测试，先插入数据再建索引，再插数据并更新索引，多线程扫描，索引列为第1列和第2列
008			许彦      创建		  2011.11.22	 多线程测试，大数据量，先插入数据再建索引，多个线程扫描，索引列为最后一个变长列
009			许彦      创建		  2011.11.23	 多线程测试，大数据量，先建索引再插入数据，多个线程扫描，索引列为最后一个变长列
010			许彦      创建		  2011.11.30	 先插入数据,多线程同时创建多个索引，再起多个线程同时进行查询


************************************************************************/
#include "utils/SafeMap.h"
#include "boost/thread/thread.hpp"
#include <boost/assign.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include "utils/util.h"
#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "catalog/metaData.h"
#include "catalog/pg_database.h"
#include "commands/tablespace.h"
#include "test_fram.h"
#include "utils/rel.h"
#include "index/test_index.h"
#include "utils/fmgroids.h"
#include "interface/FDPGAdapter.h"
#include "iostream"
#include "interface/StorageEngineExceptionUniversal.h"
#include "Macros.h"
#include <time.h>
#include <vector>
#include <string>
using namespace std;
using namespace FounderXDB::StorageEngineNS;
using boost::detail::spinlock;


int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2)
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

int my_compare_int(const char *str1, size_t len1, const char *str2, size_t len2)
{
// 	int i = 0;
// 	while(i < len1 && i < len2)
// 	{
// 		if(str1[i] < str2[i])
// 			return -1;
// 		else if(str1[i] > str2[i])
// 			return 1;
// 		else i++;
// 
// 	}
// 	if(len1 == len2)
// 		return 0;
// 	if(len1 > len2)
// 		return 1;
// 	return -1;
	int a = atoi(str1);
	int b = atoi(str2);
	if(a < b)
		return -1;
	else if(a == b)
		return 0;
	else return 1;
}

void my_split_heap_321(RangeData& rangeData,const char *str, int col, size_t len = 0)
{

	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 1;
	}
}

void my_split_heap_321321(RangeData& rangeData,const char *str, int col, size_t len = 0)
{

	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 1;
	}
	if (col == 4)
	{
		rangeData.start = 6;
		rangeData.len = 3;
	}
	if (col == 5)
	{
		rangeData.start = 9;
		rangeData.len = 2;
	}
	if (col == 6)
	{
		rangeData.start = 11;
		rangeData.len = 1;
	}
}

void my_split_heap_100(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	for(int i = 0;i<100;i++)
	{
		if (col == i+1)
		{
			rangeData.start = i;
			rangeData.len = 1;
		}
	}
}

void my_split_index_12(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
}

void my_split_index_23(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 1;
	}
}

void my_split_index_21(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 3;
	}
}

void my_split_index_23541(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 1;
	}
	if (col == 3)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 4)
	{
		rangeData.start = 5;
		rangeData.len = 3;
	}
	if (col == 5)
	{
		rangeData.start = 8;
		rangeData.len = 3;
	}
}

void my_split_index_100(RangeData& rangeData,const char *str, int col , size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	for(int i = 0;i<100;i++)
	{
		if (col == i+1)
		{
			rangeData.start = i;
			rangeData.len = 1;
		}
	}
}

void my_form_datum(Datum *values,int nvalues,char datum_data[][20])
{
	for(int i = 0;i<nvalues;i++)
	{
		values[i] = fdxdb_string_formdatum(datum_data[i], strlen(datum_data[i]));
	}
}

void my_scankey_init(ScanKeyData *key,StrategyNumber *strategyArray,AttrNumber *colArray,Datum * values,int ncols,CompareCallback compareFunction)
{
	for(int i = 0;i<ncols;i++)
	{
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[i],colArray[i],strategyArray[i],compareFunction,values[i]);	
	}	
}

bool test_indexscan_000()
{
	INTENT("两字段索引，仅向前扫描，索引列为第1列和第2列");
	int relid = RID;
	try
	{	
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;
		HeapTuple tuple;
		const int nkeys = 2;
		
		//open heap and insert some tuple
// 		Colinfo colinfo0 = (Colinfo)palloc(sizeof(ColinfoData));
// 		colinfo0->col_number = NULL;
// 		colinfo0->rd_comfunction = NULL;
// 		colinfo0->split_function =  my_split_heap_321;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
//		pfree(colinfo0);

		char data[20][20] = {"123456", "123789", "012562", "012120", "012456"};
		for(int i=0;i<5;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(data[i], sizeof(data[i]))) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(testRelation, tuple) == 0);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//create a index on testRelation
		Relation indexRelation;
		++INDEX_ID;
		Oid indid = INDEX_ID;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo->keys = nkeys;
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->col_number[1] = 2;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_str;
		colinfo->rd_comfunction[1] = my_compare_str;
		colinfo->split_function =  my_split_index_12;
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select use scankey
		IndexScanDesc index_scan;
		ScanKeyData key[nkeys];
		char *temp;
		int flag = 1;

		//form datum
		char datum_data[20][20] = {"123", "45"};
		Datum * values = (Datum *) palloc(sizeof(Datum)*nkeys); //用几个datum就分配多大的空间
		my_form_datum(values,nkeys,datum_data);

		//init scankey
		StrategyNumber strategyArray[nkeys] = {BTLessStrategyNumber,BTGreaterStrategyNumber};
		AttrNumber colArray[nkeys] = {1,2};
		my_scankey_init(key,strategyArray,colArray,values,nkeys,my_compare_str);
		pfree(values);
		//open index and begin scan
		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);
		FreeColiInfo(colinfo);

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, nkeys, 0);
		FDPG_Index::fd_index_rescan(index_scan, key, nkeys, NULL, 0);

		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			temp = fxdb_tuple_to_chars(tuple);
			flag = strcmp(temp, "012562");
			printf("%s\n",temp);
		}
		CHECK_BOOL(flag == 0);
		if(flag != 0)
		{
			printf("测试失败！\n");
			FDPG_Index::fd_index_endscan(index_scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
			FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
			user_abort_transaction();
			dropTable();
			return false;
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
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

bool test_indexscan_001()
{
	INTENT("两字段索引，仅向前扫描，改变索引列为第2列和第3列");
	int relid = RID;
	try
	{	
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;
		HeapTuple tuple;
		const int nkeys = 2;

		//open heap and insert some tuple
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_heap_321;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		FreeColiInfo(colinfo0);

		char data[20][20] = {"123456", "123789", "012562", "012120", "012456"};
		for(int i=0;i<5;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(data[i], sizeof(data[i]))) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(testRelation, tuple) == 0);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//create a index on testRelation
		Relation indexRelation;
		++INDEX_ID;
		Oid indid = INDEX_ID;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo->keys = nkeys;
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 2;
		colinfo->col_number[1] = 3;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_str;
		colinfo->rd_comfunction[1] = my_compare_str;
		colinfo->split_function =  my_split_index_23;
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select use scankey
		IndexScanDesc index_scan;
		ScanKeyData key[nkeys];
		char *temp;
		int flag = 1;

		//form datum
		char datum_data[20][20] = {"45", "2"};
		Datum * values = (Datum *) palloc(sizeof(Datum)*nkeys); //用几个datum就分配多大的空间
		my_form_datum(values,nkeys,datum_data);

		//init scankey
		StrategyNumber strategyArray[nkeys] = {BTLessStrategyNumber,BTLessStrategyNumber};
		AttrNumber colArray[nkeys] = {1,2};
		my_scankey_init(key,strategyArray,colArray,values,nkeys,my_compare_str);
		pfree(values);
		//open index and begin scan
		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);
		FreeColiInfo(colinfo);
		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, nkeys, 0);
		FDPG_Index::fd_index_rescan(index_scan, key, nkeys, NULL, 0);
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, BackwardScanDirection)) != NULL)
		{
			temp = fxdb_tuple_to_chars(tuple);
			flag = strcmp(temp, "012120");
			printf("%s\n",temp);
		}
		CHECK_BOOL(flag == 0);
		if(flag != 0)
		{
			printf("测试失败！\n");
			FDPG_Index::fd_index_endscan(index_scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
			FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
			user_abort_transaction();
			dropTable();
			return false;
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
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

bool test_indexscan_002()
{
	INTENT("两字段索引，仅向前扫描，改变索引列顺序为先后列再前列，如索引列为先第2列后第1列");
	int relid = RID;
	try
	{	
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		createTable(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;
		HeapTuple tuple;
		const int nkeys = 2;

		//open heap and insert some tuple
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_heap_321;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		FreeColiInfo(colinfo0);

		char data[20][20] = {"123456", "123789", "012562", "012120", "012456"};
		for(int i=0;i<5;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(data[i], sizeof(data[i]))) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(testRelation, tuple) == 0);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//create a index on testRelation
		Relation indexRelation;
		++INDEX_ID;
		Oid indid = INDEX_ID;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo->keys = nkeys;
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 2;
		colinfo->col_number[1] = 1;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_str;
		colinfo->rd_comfunction[1] = my_compare_str;
		colinfo->split_function =  my_split_index_21;
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select use scankey
		IndexScanDesc index_scan;
		ScanKeyData key[nkeys];
		char *temp;
		int flag = 1;

		//form datum
		char datum_data[20][20] = {"45", "123"};   //必须与索引列顺序保持一致
		Datum * values = (Datum *) palloc(sizeof(Datum)*nkeys); //用几个datum就分配多大的空间
		my_form_datum(values,nkeys,datum_data);

		//init scankey
		StrategyNumber strategyArray[nkeys] = {BTGreaterStrategyNumber,BTLessStrategyNumber}; //必须与索引列顺序保持一致
		AttrNumber colArray[nkeys] = {1,2}; //必须是按列升序排列
		my_scankey_init(key,strategyArray,colArray,values,nkeys,my_compare_str);	
		pfree(values);
		//open index and begin scan
		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);
		FreeColiInfo(colinfo);
		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, nkeys, 0);
		FDPG_Index::fd_index_rescan(index_scan, key, nkeys, NULL, 0);
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			temp = fxdb_tuple_to_chars(tuple);
			flag = strcmp(temp, "012562");
			printf("%s\n",temp);
		}
		CHECK_BOOL(flag == 0);
		if(flag != 0)
		{
			printf("测试失败！\n");
			FDPG_Index::fd_index_endscan(index_scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
			FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
			user_abort_transaction();
			dropTable();
			return false;
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
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

bool test_indexscan_003()
{
	INTENT("五字段索引，仅向前扫描，索引列为2,3,5,4,1，使用5种不同策略");
	int relid = RID;
	try
	{	
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(6, 3, 2, 1,3,2,1);
		createTable(relid,heap_info);
 		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;
		HeapTuple tuple;
		const int nkeys = 5;

		//open heap and insert some tuple
// 		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));
// 		colinfo0->col_number = NULL;
// 		colinfo0->rd_comfunction = NULL;
// 		colinfo0->keys = 0;
// 		colinfo0->split_function =  my_split_heap_321321;
// 		createTable(relid,colinfo0);
//		free(colinfo0);
//		begin_transaction();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
//		pfree(colinfo0);

		char data[20][20] = {"123206jim456", "123789jim789", "012562tom562", "012120tom120",
			"255456ann456","123456ann458","255206ann568"};
		//2 3 5 4 1
		//"123 20 6 jim 45 6" 
		//"123 78 9 jim 78 9"
		//"012 56 2 tom 56 2"
		//"012 12 0 tom 12 0" 
		//"255 45 6 ann 45 6"
		//"123 45 6 ann 45 8"
		//"255 20 6 ann 56 8
		for(int i=0;i<7;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(data[i], sizeof(data[i]))) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(testRelation, tuple) == 0);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//create a index on testRelation
		Relation indexRelation;
		++INDEX_ID;
		Oid indid = INDEX_ID;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo->keys = nkeys;
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 2;
		colinfo->col_number[1] = 3;
		colinfo->col_number[2] = 5;
		colinfo->col_number[3] = 4;
		colinfo->col_number[4] = 1;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_str;
		colinfo->rd_comfunction[1] = my_compare_str;
		colinfo->rd_comfunction[2] = my_compare_str;
		colinfo->rd_comfunction[3] = my_compare_str;
		colinfo->rd_comfunction[4] = my_compare_str;
		colinfo->split_function =  my_split_index_23541;
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select use scankey
		IndexScanDesc index_scan;
		ScanKeyData key[nkeys];
		char *temp;
		int flag = 1;

		//form datum
		char datum_data[20][20] = {"20", "6", "56", "ann", "255"};   //必须与索引列顺序保持一致
		Datum * values = (Datum *) palloc(sizeof(Datum)*nkeys); //用几个datum就分配多大的空间
		my_form_datum(values,nkeys,datum_data);

		//init scankey
		StrategyNumber strategyArray[nkeys] = {BTGreaterStrategyNumber,BTLessEqualStrategyNumber,
			BTLessStrategyNumber,BTEqualStrategyNumber,BTGreaterEqualStrategyNumber}; //必须与索引列顺序保持一致
		AttrNumber colArray[nkeys] = {1,2,3,4,5}; //必须是按列升序排列
		my_scankey_init(key,strategyArray,colArray,values,nkeys,my_compare_str);
		pfree(values);
		//open index and begin scan
		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);
		FreeColiInfo(colinfo);
		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, nkeys, 0);
		FDPG_Index::fd_index_rescan(index_scan, key, nkeys, NULL, 0);
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			temp = fxdb_tuple_to_chars(tuple);
			flag = strcmp(temp, "255456ann456"); //255206ann568
			printf("%s\n",temp);
		}
		CHECK_BOOL(flag == 0);
		if(flag != 0)
		{
			printf("测试失败！\n");
			FDPG_Index::fd_index_endscan(index_scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
			FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
			user_abort_transaction();
			dropTable();
			return false;
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
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

#define index_cols 32
bool test_indexscan_004()
{
	INTENT("32字段索引，仅向前扫描，索引列为1-32，插入大量数据");
	int relid = RID;
	try
	{	
//		SpliterGenerater sg;
//		Colinfo heap_info = sg.buildHeapColInfo(6, 3, 2, 1,3,2,1);
//		createTable(relid,heap_info);
//		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;
		HeapTuple tuple;
		const int nkeys = index_cols;

		//open heap and insert some tuple
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->keys = 0;
		colinfo0->split_function =  my_split_heap_100;
		createTable(relid,colinfo0);
//		free(colinfo0);
		begin_transaction();
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);

		const int rows = 10000;
		const int cols = index_cols;
		char data[rows][cols+1];
		char data_row1[cols+1];
		memset(data_row1,'a',sizeof(data_row1));
		char data_row2[cols+1];
		memset(data_row2,'b',sizeof(data_row2));
		char data_row3[cols+1];
		memset(data_row3,'c',sizeof(data_row3));

		data_row1[cols] = '\0';
		data_row2[cols] = '\0';
		data_row3[cols] = '\0';

		for(int i = 0;i<rows;i++)
		{
			if((i%3)==0)
			{
				memcpy(data[i],data_row1,sizeof(data_row1));
			}
			if((i%3)==1)
			{
				memcpy(data[i],data_row2,sizeof(data_row2));
			}
			if((i%3)==2)
			{
				memcpy(data[i],data_row3,sizeof(data_row3));
			}
			//printf("data[i] = %s\n",data[i]);
		}
		
		int time_begin,time_end;
		time_begin = clock();
		for(int i = 0;i<rows;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = FDPG_Heap::fd_heap_form_tuple(data[i], sizeof(data[i]));
			FDPG_Heap::fd_simple_heap_insert(testRelation, tuple);
		}
		time_end = clock();
		printf("插入%d行数据耗时%dms\n",rows,time_end - time_begin);
		//printf("计算耗时:%dms\n",clock()-i);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//create a index on testRelation
		Relation indexRelation;
		++INDEX_ID;
		Oid indid = INDEX_ID;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo->keys = nkeys;
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		for(int i = 0;i<nkeys;i++)
		{
			colinfo->col_number[i] = i+1;
		}

		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		for(int i = 0;i<nkeys;i++)
		{
			colinfo->rd_comfunction[i] = my_compare_str;
		}
		colinfo->split_function =  my_split_index_100;
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//select use scankey
		IndexScanDesc index_scan;
		ScanKeyData key[nkeys];
		char *temp;
		int flag = 1;
		int counter = 0;

		//form datum
	    //必须与索引列顺序保持一致
		char datum_data[nkeys][20];
		for(int i = 0;i<nkeys;i++)
		{
			for(int j = 0;j<1;j++)
			{
				datum_data[i][j] = 'b';
				datum_data[i][j+1] = '\0';		
			}
		}

		Datum * values = (Datum *) palloc(sizeof(Datum)*nkeys); //用几个datum就分配多大的空间
		my_form_datum(values,nkeys,datum_data);

		//init scankey
		StrategyNumber strategyArray[nkeys];  	
		for(int i = 0;i<nkeys;i++)
		{
			strategyArray[i] = BTEqualStrategyNumber;
		}

	    //必须是按列升序排列
		AttrNumber colArray[nkeys];
		for(AttrNumber i = 0;i<nkeys;i++)
		{
			colArray[i] = i+1;
		}

		my_scankey_init(key,strategyArray,colArray,values,nkeys,my_compare_str);
		pfree(values);
		//open index and begin scan
		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);
		FreeColiInfo(colinfo);
		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, nkeys, 0);
		FDPG_Index::fd_index_rescan(index_scan, key, nkeys, NULL, 0);
		time_begin = clock();
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			counter++;
			temp = fxdb_tuple_to_chars(tuple);
			flag = strcmp(temp, data_row2); 
			//printf("%s\n",temp);
		}
		time_end = clock();
		printf("索引列为32字段，查询%d行数据耗时%dms\n",rows,time_end);
		CHECK_BOOL(flag==0 && counter==rows/3);
		if(flag!=0 || counter!=rows/3)
		{
			printf("测试失败！\n");
			FDPG_Index::fd_index_endscan(index_scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
			FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
			user_abort_transaction();
			dropTable();
			return false;
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
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

void thread_index_scan(BackendParameters *param,
											 const Oid index_id, 
											 const Colinfo ind_info, 
											 const Oid heap_id, 
											 const Colinfo heap_info,
											 const IndexScanInfo *isi, 
											 DataGenerater *dg,
											unsigned int *array_len)
{
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(param);

	using namespace FounderXDB::StorageEngineNS;

	try
	{
		begin_transaction();
		Relation heap = FDPG_Heap::fd_heap_open(heap_id,AccessShareLock, MyDatabaseId);
		Relation index = FDPG_Index::fd_index_open(index_id,AccessShareLock, MyDatabaseId);

		IndexScanDesc isd = FDPG_Index::fd_index_beginscan(heap, index, SnapshotNow, isi->ncol, 0);
		FDPG_Index::fd_index_rescan(isd, isi->scan_key, isi->ncol, NULL, 0);
		HeapTuple tuple = NULL;
		unsigned int count = 0;
		while((tuple = FDPG_Index::fd_index_getnext(isd, ForwardScanDirection)) != NULL)
		{
			std::pair<char*, int> pci = tuple_get_string_and_len(tuple);
			dg->dataGenerate(pci.first, pci.second, count++);
			++(*array_len);
			pfree(pci.first);
		}
		dg->setSize(*array_len);
		FDPG_Index::fd_index_endscan(isd);
		FDPG_Index::fd_index_close(index, AccessShareLock);
		FDPG_Heap::fd_heap_close(heap, AccessShareLock);
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		cout << se.getErrorNo() << endl;
		cout << se.getErrorMsg() << endl;
	}
	proc_exit(0);
}


void thread_index_scan_map(BackendParameters *param,
					   const Oid index_id, 
					   const Colinfo ind_info, 
					   const Oid heap_id, 
					   const Colinfo heap_info,
					   IndexScanInfo *isi,
					   const int thread_ID, 
//					   SafeMap<int,std::set<string> > &map_result)
					   std::map<int,std::set<string> > &map_result)
{
//	printf("thread map: %p \n",&map_result);
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(param);

	using namespace FounderXDB::StorageEngineNS;

	try
	{
		begin_transaction();
		Relation heap = FDPG_Heap::fd_heap_open(heap_id,AccessShareLock, MyDatabaseId);
		Relation index = FDPG_Index::fd_index_open(index_id,AccessShareLock, MyDatabaseId);

		IndexScanDesc isd = FDPG_Index::fd_index_beginscan(heap, index, SnapshotNow, isi->ncol, 0);
		FDPG_Index::fd_index_rescan(isd, isi->scan_key, isi->ncol, NULL, 0);
		HeapTuple tuple = NULL;
		unsigned int count = 0;
		std::set<string> setResult;
		std::set<string>::iterator set_it;
		while((tuple = FDPG_Index::fd_index_getnext(isd, ForwardScanDirection)) != NULL)
		{
			char *tmp = fxdb_tuple_to_chars(tuple);
			setResult.insert(tmp);
			pfree(tmp);
//			setResult.insert(string(fxdb_tuple_to_chars(tuple)));
		}
		{
			//boost::lock_guard<boost::mutex> lock(MAP_MUTEX2);
			//map_result.insert(make_pair(thread_ID,setResult));
			map_insert_safe(thread_ID,map_result,setResult);
		}
		FDPG_Index::fd_index_endscan(isd);
		FDPG_Index::fd_index_close(index, AccessShareLock);
		FDPG_Heap::fd_heap_close(heap, AccessShareLock);
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		cout << se.getErrorNo() << endl;
		cout << se.getErrorMsg() << endl;
	}
	proc_exit(0);
}

void thread_index_create(const int heap_id, 
												 const Colinfo heap_colinfo, 
												 const Colinfo ind_colinfo, 
												 const int ind_id, 
												 const bool unique,
												 BackendParameters *param,
												 int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(param);

	try
	{
		create_index(heap_id, heap_colinfo, ind_colinfo, ind_id, unique);
		*sta = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*sta = -1;
	}
	proc_exit(0);
}

void thread_index_drop(const int heap_id,
											 int ind_id,
											 BackendParameters *param,
											 int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(param);

	try
	{
		remove_index(heap_id, ind_id);
		*sta = 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		*sta = -1;
	}
	proc_exit(0);
}

EXTERN_SIMPLE_FUNCTION

#define THREAD_NUM_1 50

int test_thread_indexscan_000()
{
	++INDEX_ID;
	INTENT("创建表和在表上创建索引，然后插入若干数据。"
		"启动线程利用索引查询数据。测试成功的条件是"
		"能查询到所有插入的数据。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	SpliterGenerater sg;

	const int COL_NUM = 3;
	//创建一个3列的表，各列长度为5 10 3
	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM, 5, 10, 3);
	{
		int create_sta = 0;
		SIMPLE_CREATE_HEAP(RELID, create_sta);
	}

	const unsigned int data_row = 10;
	const unsigned int data_len = 20;
	DataGenerater dg(data_row, data_len);
	dg.dataGenerate();

	char data[data_row][DATA_LEN];
	dg.dataToDataArray2D(data);

	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(RELID, data, DATA_LEN, insert_sta);
	}
	CommandCounterIncrement();

	//创建一个两列的索引，索引列为1、3列
	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1, 3};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1, 3>);
	{
		int create_sta = 0;
		SIMPLE_CREATE_INDEX(RELID, INDEX_ID, heap_info, index_info, create_sta);
	}
	CommandCounterIncrement();

	/*
	* 初始化scankey
	*/
	IndexScanInfo isi[THREAD_NUM_1];
	for(int i = 0; i < THREAD_NUM_1; ++i)
	{
	alloc_scan_space(INDEX_COL_NUM, isi[i]);
	isi[i].cmp_values[0] = fdxdb_string_formdatum("AAAAA", strlen("AAAAA"));
	isi[i].cmp_values[1] = fdxdb_string_formdatum("zzz", strlen("zzz"));
	isi[i].cmp_func[0] = my_compare_str;
	isi[i].cmp_func[1] = my_compare_str;
	isi[i].cmp_strategy[0] = BTGreaterEqualStrategyNumber;
	isi[i].cmp_strategy[1] = BTLessStrategyNumber;
	isi[i].col_array[0] = 1;
	isi[i].col_array[1] = 2;
	init_scan_key(isi[i]);
	}
	/*
	* 创建一个线程利用索引查询数据
	*/
	char found_array[data_row][DATA_LEN];

	VectorMgr<DataGenerater *> vm_dg;
	VectorMgr<unsigned int *> vm_ui;
	for(int i = 0; i < THREAD_NUM_1; ++i)
	{
		unsigned int *array_len = new unsigned int(0);
		vm_ui.vPush(array_len);
		DataGenerater *found_dg = new DataGenerater(10, DATA_LEN);
		vm_dg.vPush(found_dg);
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		tg.create_thread(bind(&thread_index_scan, GET_PARAM(), INDEX_ID, index_info, RELID, heap_info, &isi[i], found_dg, array_len));
	}
	tg.join_all();
	FREE_PARAM(BackendParameters *);
	//free_scan_key(isi);

	CheckResultInfo cri;
	cri.ncol = 2;
	cri.col_array = (unsigned int *)malloc(COL_NUM * sizeof(unsigned int));
	cri.col_array[0] = 5;
	cri.col_array[1] = 10;
	cri.col_array[2] = 3;
	char *c[] = {"AAAAA", "zzz"};
	cri.cmp_data = c;
	cri.cmp_stn = (StrategyNumber *)malloc(INDEX_COL_NUM * sizeof(StrategyNumber));
	cri.cmp_stn[0] = BTGreaterEqualStrategyNumber;
	cri.cmp_stn[1] = BTLessStrategyNumber;
	cri.col_num = (unsigned int *)malloc(INDEX_COL_NUM * sizeof(unsigned int));
	cri.col_num[0] = 1;
	cri.col_num[1] = 3;
	char det_data[data_row][DATA_LEN];
	memset(det_data, 0, data_row * DATA_LEN);
	unsigned int det_data_len = 0;
	get_result_in_array(data, ARRAY_LEN_CALC(data), cri, det_data, det_data_len);

	free(cri.cmp_stn);
	free(cri.col_array);
	free(cri.col_num);

	for(int i = 0; i < THREAD_NUM_1; ++i)
	{
		memset(found_array, 0, data_row * DATA_LEN);
		vm_dg.getVMgr()[i]->dataToDataArray2D(*vm_ui.getVMgr()[i], found_array);
		//检查索引查找出来的结果都在表中
		bool sta = check_all_in_array(data, found_array, ARRAY_LEN_CALC(data), *vm_ui.getVMgr()[i], DATA_LEN);

		if(sta != 1)
		{
			int drop_sta = 0;
			SIMPLE_DROP_INDEX(RELID, INDEX_ID, drop_sta);
			SIMPLE_DROP_HEAP(RELID, drop_sta);
			return 0;
		}
		if(!check_array_equal(det_data, det_data_len, found_array, *vm_ui.getVMgr()[i]))
		{
			int drop_sta = 0;
			SIMPLE_DROP_INDEX(RELID, INDEX_ID, drop_sta);
			SIMPLE_DROP_HEAP(RELID, drop_sta);
			return 0;
		}
	}

	int drop_sta = 0;
	SIMPLE_DROP_INDEX(RELID, INDEX_ID, drop_sta);
	SIMPLE_DROP_HEAP(RELID, drop_sta);
	return 1;
}

int test_thread_indexscan_001()
{
	++INDEX_ID;
	INTENT("建表建索引，随后插入大量相同的数据。测试利用索引是否能找出"
				 "所有数据。");

	using namespace boost;
	using namespace FounderXDB::StorageEngineNS;
	PREPARE_TEST();

	SpliterGenerater sg;
	//构建4列的表
	const int HEAP_NUMS = 4;
	Colinfo heap_info = sg.buildHeapColInfo(HEAP_NUMS, 1, 2, 3, 4);
	{
		int creste_sta = 0;
		SIMPLE_CREATE_HEAP(RELID, creste_sta);
	}
	//构建3列索引，索引列为2 3 4列
	int col_number[] = {2, 3, 4};
	CompareCallback ccb[] = {my_compare_str, my_compare_str, my_compare_str};
	const int INDEX_NUMS = 3;
	Colinfo index_info = sg.buildIndexColInfo(INDEX_NUMS, col_number, ccb, SpliterGenerater::index_split_to_any<2, 3, 4>);

	{
		int create_sta = 0;
		SIMPLE_CREATE_INDEX(RELID, INDEX_ID, heap_info, index_info, create_sta);
	}

#undef SAME_DATA
#define SAME_DATA "asdfjalkwfjaiosfn!osjdlfkj~jslkdjf\0"
#undef ROW
#define ROW 100
	char data[ROW][DATA_LEN];
	memset(data, 0, ROW * DATA_LEN);
	for(int i = 0; i < ROW; ++i)
	{
		memcpy(data[i], SAME_DATA, strlen(SAME_DATA));
	}

	{
		//插入100行相同的测试数据
		HeapIndexRelation hir;
		hir.index_array_len = 1;
		hir.heap_id = RELID;
		hir.heap_info = heap_info;
		hir.index_id = (Oid*)malloc(hir.index_array_len * sizeof(Oid));
		hir.index_id[0] = INDEX_ID;
		hir.index_info = (Colinfo*)malloc(hir.index_array_len * sizeof(Colinfo));
		hir.index_info[0] = index_info;
		SIMPLE_INSERT_DATA_WITH_INDEX(hir, UNIQUE_CHECK_NO, data, DATA_LEN);
		free(hir.index_id);
		free(hir.index_info);
	}
	const char *COL2_DATA = "sd";
	const char *COL3_DATA = "fja";
	const char *COL4_DATA = "lkwf";
	IndexScanInfo isi[THREAD_NUM_1];
	for(int i = 0; i < THREAD_NUM_1; ++i)
	{
	alloc_scan_space(INDEX_NUMS, isi[i]);
	isi[i].cmp_values[0] = fdxdb_string_formdatum(COL2_DATA, strlen(COL2_DATA));
	isi[i].cmp_values[1] = fdxdb_string_formdatum(COL3_DATA, strlen(COL3_DATA));
	isi[i].cmp_values[2] = fdxdb_string_formdatum(COL4_DATA, strlen(COL4_DATA));
	isi[i].cmp_func[0] = my_compare_str;
	isi[i].cmp_func[1] = my_compare_str;
	isi[i].cmp_func[2] = my_compare_str;
	isi[i].cmp_strategy[0] = BTEqualStrategyNumber;
	isi[i].cmp_strategy[1] = BTEqualStrategyNumber;
	isi[i].cmp_strategy[2] = BTEqualStrategyNumber;
	isi[i].col_array[0] = 1;
	isi[i].col_array[1] = 2;
	isi[i].col_array[2] = 3;
	init_scan_key(isi[i]);
	}
	VectorMgr<DataGenerater *> vm_dg;
	VectorMgr<unsigned int *> vm_ui;

	for(int i = 0; i < THREAD_NUM_1; ++i)
	{
		DataGenerater *found_dg = new DataGenerater(ROW, DATA_LEN);
		vm_dg.vPush(found_dg);
		unsigned int *array_len = new unsigned int(0);
		vm_ui.vPush(array_len);
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(&thread_index_scan, 
			GET_PARAM(), 
			INDEX_ID, 
			index_info, 
			RELID, 
			heap_info, 
			&isi[i], 
			found_dg, 
			array_len));
	}
	GET_THREAD_GROUP().join_all();
	//free_scan_key(isi);
	FREE_PARAM(BackendParameters *);

	int test_success = true;

	//索引应该找出所有的数据，因为所有的数据都是相同的
	for(int i = 0; i < THREAD_NUM_1; ++i)
	{
		int num = *vm_ui.getVMgr()[i];
		if(*vm_ui.getVMgr()[i] != ROW)
		{
			test_success = false;
			break;
		}
	}

	if(test_success)
	{
		for(int i = 0; i < THREAD_NUM_1; ++i)
		{
			char found_array[ROW][DATA_LEN];
			vm_dg.getVMgr()[i]->dataToDataArray2D(found_array);
			for(int i = 0; i < ROW; ++i)
			{
				if(memcmp(found_array[i], data[i], DATA_LEN) != 0)
				{
					test_success = false;
					break;
				}
			}
			if(test_success == false)
			{
				break;
			}
		}
	}

	int drop_sta = 0;
	SIMPLE_DROP_INDEX(RELID, INDEX_ID, drop_sta);
	SIMPLE_DROP_HEAP(RELID, drop_sta);
	return test_success;
}


int test_thread_indexscan_002()
{
	++INDEX_ID;
	INTENT("创建多个表和多个索引，每一张表对应一个索引。"
				 "每张表插入大量数据，利用索引扫描所有表。");

	using namespace boost;
	using namespace FounderXDB::StorageEngineNS;
	using namespace std;
	PREPARE_TEST();

#undef CREATE_NUM
#define CREATE_NUM 10
	clear_all();
	unsigned int rel_id[CREATE_NUM];
	unsigned int ind_id[CREATE_NUM];
	for(int i = 0; i < CREATE_NUM; ++i)
	{
		rel_id[i] = get_heap_id();
		ind_id[i] = get_index_id();
	}

	//为每张表建索引，使用5个索引列
	vector<DataGenerater> v_dg;
#undef ROW
#define ROW 100
	SpliterGenerater sg;
	Colinfo heap_info = sg.buildHeapColInfo(6, 1, 3, 1, 5, 2, 10);
	for(int i = 0; i < CREATE_NUM; ++i)
	{
		int create_sta = 0;
		//建CREATE_NUM张表,表划分为6列
		SIMPLE_CREATE_HEAP(rel_id[i], create_sta);
		int col_number[] = {1, 2, 4, 5, 6};
		CompareCallback ccb[] = {my_compare_str, my_compare_str, my_compare_str, my_compare_str, my_compare_str};
		Colinfo index_info = sg.buildIndexColInfo(5, col_number, ccb, SpliterGenerater::index_split_to_any<1, 2, 4, 5, 6>);
		SIMPLE_CREATE_INDEX(rel_id[i], ind_id[i], heap_info, index_info, create_sta);
		//构建测试数据
		DataGenerater dg(ROW, DATA_LEN);
		dg.dataGenerate();
		v_dg.push_back(dg);
		char tmp_data[ROW][DATA_LEN];
		dg.dataToDataArray2D(tmp_data);
		int insert_sta = 0;
		HeapIndexRelation hir;
		hir.index_array_len = 1;
		hir.heap_id = rel_id[i];
		hir.heap_info = heap_info;
		hir.index_id = (Oid*)malloc(hir.index_array_len * sizeof(Oid));
		hir.index_id[0] = ind_id[i];
		hir.index_info = (Colinfo*)malloc(hir.index_array_len * sizeof(Colinfo));
		hir.index_info[0] = index_info;
		SIMPLE_INSERT_DATA_WITH_INDEX(hir, UNIQUE_CHECK_NO, tmp_data, DATA_LEN);
		free(hir.index_id);
		free(hir.index_info);
	}

	//构建scankey
	const char *COL1_DATA = "A";
	const char *COL2_DATA = "zzz";
	const char *COL4_DATA = "ccccc";
	const char *COL5_DATA = "DD";
	const char *COL6_DATA = "7777774635";
	const int INDEX_NUMS = 5;

	vector<pair<unsigned int *, DataGenerater *> > v_found_dg;
	//使用索引扫描
	int col_number[] = {1, 2, 4, 5, 6};
	CompareCallback ccb[] = {my_compare_str, my_compare_str, my_compare_str, my_compare_str, my_compare_str};
	Colinfo index_info = sg.buildIndexColInfo(5, col_number, ccb, SpliterGenerater::index_split_to_any<1, 2, 4, 5, 6>);
	IndexScanInfo isi[CREATE_NUM];
		for(int i = 0; i < CREATE_NUM; ++i)
	{
	alloc_scan_space(INDEX_NUMS, isi[i]);
	isi[i].cmp_values[0] = fdxdb_string_formdatum(COL1_DATA, strlen(COL1_DATA));
	isi[i].cmp_values[1] = fdxdb_string_formdatum(COL2_DATA, strlen(COL2_DATA));
	isi[i].cmp_values[2] = fdxdb_string_formdatum(COL4_DATA, strlen(COL4_DATA));
	isi[i].cmp_values[3] = fdxdb_string_formdatum(COL5_DATA, strlen(COL5_DATA));
	isi[i].cmp_values[4] = fdxdb_string_formdatum(COL6_DATA, strlen(COL6_DATA));
	for(int j = 0; j < INDEX_NUMS; ++j)
	{
		isi[i].cmp_func[j] = my_compare_str;
	}
	isi[i].cmp_strategy[0] = BTGreaterStrategyNumber;//第一列大于"A"
	isi[i].cmp_strategy[1] = BTLessStrategyNumber;		//第二列小于"zzz"
	isi[i].cmp_strategy[2] = BTGreaterEqualStrategyNumber;	//第四列大于等于"ccccc"
	isi[i].cmp_strategy[3] = BTGreaterStrategyNumber; //第五列大于"DD"
	isi[i].cmp_strategy[4] = BTGreaterStrategyNumber; //第六列大于"7777774635"
	isi[i].col_array[0] = 1;
	isi[i].col_array[1] = 2;
	isi[i].col_array[2] = 3;
	isi[i].col_array[3] = 4;
	isi[i].col_array[4] = 5;
	init_scan_key(isi[i]);
	}
	for(int i = 0; i < CREATE_NUM; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		DataGenerater *found_dg = new DataGenerater(ROW, DATA_LEN);
		unsigned int *array_len = new unsigned int(0);
		GET_THREAD_GROUP().create_thread(bind(&thread_index_scan, 
			GET_PARAM(), 
			ind_id[i], 
			index_info, 
			rel_id[i], 
			heap_info, 
			&isi[i], 
			found_dg, 
			array_len));
		v_found_dg.push_back(pair<unsigned int *, DataGenerater *>(array_len, found_dg));
		GET_THREAD_GROUP().join_all();
	}
	//free_scan_key(isi);

	//检查结果
	CheckResultInfo cri;
	cri.ncol = INDEX_NUMS;
	cri.col_array = (unsigned int *)malloc(6 * sizeof(unsigned int));
	cri.col_array[0] = 1;
	cri.col_array[1] = 3;
	cri.col_array[2] = 1;
	cri.col_array[3] = 5;
	cri.col_array[4] = 2;
	cri.col_array[5] = 10;
	char *c[] = {"A", "zzz", "ccccc", "DD", "7777774635"};
	cri.cmp_data = c;
	cri.cmp_stn = (StrategyNumber *)malloc(INDEX_NUMS * sizeof(StrategyNumber));
	cri.cmp_stn[0] = BTGreaterStrategyNumber;
	cri.cmp_stn[1] = BTLessStrategyNumber;
	cri.cmp_stn[2] = BTGreaterEqualStrategyNumber;
	cri.cmp_stn[3] = BTGreaterStrategyNumber;
	cri.cmp_stn[4] = BTGreaterStrategyNumber;
	cri.col_num = (unsigned int *)malloc(INDEX_NUMS * sizeof(unsigned int));
	cri.col_num[0] = 1;
	cri.col_num[1] = 2;
	cri.col_num[2] = 4;
	cri.col_num[3] = 5;
	cri.col_num[4] = 6;
	int test_success = true;
	for(int i = 0; i < v_dg.size(); ++i)
	{
		char det_data[ROW][DATA_LEN];
		char data[ROW][DATA_LEN];
		v_dg[i].dataToDataArray2D(data);
		memset(det_data, 0, ROW * DATA_LEN);
		unsigned int det_data_len = 0;
		get_result_in_array(data, ARRAY_LEN_CALC(data), cri, det_data, det_data_len);
		char found_data[ROW][DATA_LEN];
		v_found_dg[i].second->dataToDataArray2D(found_data);
		if(!check_array_equal(det_data, det_data_len, found_data, *v_found_dg[i].first))
		{
			test_success = false;
			break;
		}
	}
	for(int i = 0; i < v_found_dg.size(); ++i)
	{
		v_found_dg[i].second->~DataGenerater();
		delete v_found_dg[i].first;
	}
	free(cri.cmp_stn);
	free(cri.col_array);
	free(cri.col_num);
	FREE_PARAM(BackendParameters *);
	for(int i = 0; i < CREATE_NUM; ++i)
	{
		int drop_sta = 0;
		SIMPLE_DROP_INDEX(rel_id[i], ind_id[i], drop_sta);
		SIMPLE_DROP_HEAP(rel_id[i], drop_sta);
	}

	return test_success;
}

int test_thread_indexscan_003()
{
	++INDEX_ID;
	INTENT("创建多张表，每张表上创建多个索引（非多列索引）。其中部分索引在"
				 "插入数据前创建，部分索引在插入数据后创建。测试每张表的索引扫描"
				 "是否正确。");

	using namespace boost;
	using namespace std;
	PREPARE_TEST();

#undef HEAP_NUM
#define HEAP_NUM 2
	int rel_id[HEAP_NUM];
	clear_all();
	for(int i = 0; i < HEAP_NUM; ++i)
	{
		rel_id[i] = get_heap_id();
	}

#undef INDEX_NUM
#define INDEX_NUM (2 * HEAP_NUM)
	int ind_id[INDEX_NUM];
	for(int i = 0; i < INDEX_NUM; ++i)
	{
		ind_id[i] = get_index_id();
	}

#undef ROW
#define ROW 100
	vector<DataGenerater *> v_dg;
	vector<SpliterGenerater *> v_tmp_sg;
	{
		//创建HEAP_NUM张表并在每张表上创建INDEX_NUM/HEAP_NUM个索引
		int tmp_j = 0;
		SpliterGenerater *sg = new SpliterGenerater;
		Colinfo heap_info = sg->buildHeapColInfo(4, 3, 3, 3, 3);
		for(int i = 0; i < HEAP_NUM; ++i)
		{
			int create_sta = 0;
			SIMPLE_CREATE_HEAP(rel_id[i], create_sta);

			/*
			* 如果是双数表，则为先插数据后建索引，否者先建
			* 索引后插数据。
			*/
			if(i % 2 == 0)
			{
				DataGenerater *dg = new DataGenerater(ROW, DATA_LEN);
				dg->dataGenerate();
				char data[ROW][DATA_LEN];
				dg->dataToDataArray2D(data);
				int insert_sta = 0;
				SIMPLE_INSERT_DATA(rel_id[i], data, DATA_LEN, insert_sta);
				v_dg.push_back(dg);
			}
			//索引在第一和第三列上创建
			HeapIndexRelation hir;
			hir.heap_id = rel_id[i];
			hir.index_array_len = INDEX_NUM/HEAP_NUM;
			hir.index_id = (Oid *)malloc(hir.index_array_len * sizeof(Oid));
			hir.index_info = (Colinfo *)malloc(hir.index_array_len * sizeof(Colinfo));
			for(int j = 0; j < INDEX_NUM/HEAP_NUM; ++j)
			{
				//动态创建SpliterGenerater防止超出作用域的析构调用
				sg = new SpliterGenerater;
				v_tmp_sg.push_back(sg);
				//表分割为等长的4列
				heap_info = sg->buildHeapColInfo(4, 3, 3, 3, 3);
				hir.heap_info = heap_info;
				Colinfo index_info = NULL;
				int col_number[1];
				CompareCallback ccb[] = {my_compare_str};
				if(j % 2 == 0)
				{
					col_number[0] = 1;
					index_info = sg->buildIndexColInfo(1, col_number, ccb, SpliterGenerater::index_split_to_any<1>);
				}else
				{
					col_number[0] = 3;
					index_info = sg->buildIndexColInfo(1, col_number, ccb, SpliterGenerater::index_split_to_any<3>);
				}
				SIMPLE_CREATE_INDEX(rel_id[i], ind_id[tmp_j], heap_info, index_info, create_sta);
				hir.index_id[j] = ind_id[tmp_j];
				hir.index_info[j] = index_info;
				++tmp_j;
			}
			//单数表使用先建索引后插数据
			if(!(i % 2 == 0))
			{
				DataGenerater *dg = new DataGenerater(ROW, DATA_LEN);
				dg->dataGenerate();
				char data[ROW][DATA_LEN];
				dg->dataToDataArray2D(data);
				SIMPLE_INSERT_DATA_WITH_INDEX(hir, UNIQUE_CHECK_NO, data, DATA_LEN);
				v_dg.push_back(dg);
			}
			free(hir.index_id);
			free(hir.index_info);
		}
	}

	//创建多个线程用索引扫描表
	vector<pair<unsigned int *, DataGenerater *> > v_scan_on1, v_scan_on3;
#undef THREAD_SCAN_NUM
#define THREAD_SCAN_NUM 2

	const int INDEX_NUMS = 1;
	const char *SCAN_ON_1 = "[bc";//第一列上的索引比较字符串
	const char *SCAN_ON_3 = "!$@";//第三列上的索引比较字符串

	//构建扫描用的Colinfo
	SpliterGenerater sg1;
	Colinfo heap_info = sg1.buildHeapColInfo(4, 3, 3, 3, 3);
	int col_number[1];
	CompareCallback ccb[] = {my_compare_str};
	col_number[0] = 1;
	Colinfo index1_info = sg1.buildIndexColInfo(1, col_number, ccb, SpliterGenerater::index_split_to_any<1>);
	SpliterGenerater sg2;
	sg2.buildHeapColInfo(4, 3, 3, 3, 3);
	col_number[0] = 3;
	Colinfo index3_info = sg2.buildIndexColInfo(1, col_number, ccb, SpliterGenerater::index_split_to_any<3>);

	int tmp_ind = 0;
	vector<IndexScanInfo *> v_isi;
	for(int i = 0; i < HEAP_NUM; ++i)
	{
		//使用第一列扫描
		IndexScanInfo isi1[THREAD_SCAN_NUM] ;
		for(int j = 0; j < THREAD_SCAN_NUM; ++j)
		{
		alloc_scan_space(INDEX_NUMS, isi1[j]);
		isi1[j].cmp_func[0] = my_compare_str;
		isi1[j].col_array[0] = 1;
		isi1[j].cmp_values[0] = fdxdb_string_formdatum(SCAN_ON_1, strlen(SCAN_ON_1));
		isi1[j].cmp_strategy[0] = BTGreaterStrategyNumber;//第一列大于"[bc"
		init_scan_key(isi1[j]);
		}
		for(int j = 0; j < THREAD_SCAN_NUM; ++j)
		{
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());

			DataGenerater *found_dg = new DataGenerater(ROW, DATA_LEN);
			unsigned int *array_len = new unsigned int(0);
			GET_THREAD_GROUP().create_thread(bind(&thread_index_scan, 
				GET_PARAM(), 
				ind_id[tmp_ind], 
				index1_info, 
				rel_id[i], 
				heap_info, 
				&isi1[j], 
				found_dg, 
				array_len));
			v_scan_on1.push_back(pair<unsigned int *, DataGenerater *>(array_len, found_dg));
		}
		++tmp_ind;
		GET_THREAD_GROUP().join_all();
		//使用第三列扫描
		IndexScanInfo isi2[THREAD_SCAN_NUM];	
		for(int j = 0; j < THREAD_SCAN_NUM; ++j)
		{
		alloc_scan_space(INDEX_NUMS, isi2[j]);
		isi2[j].cmp_func[0] = my_compare_str;
		isi2[j].col_array[0] = 1;
		isi2[j].cmp_values[0] = fdxdb_string_formdatum(SCAN_ON_3, strlen(SCAN_ON_3));
		isi2[j].cmp_strategy[0] = BTGreaterStrategyNumber;//第一列大于"[bc"
		init_scan_key(isi2[j]);
		}
		for(int j = 0; j < THREAD_SCAN_NUM; ++j)
		{
			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());

			DataGenerater *found_dg1 = new DataGenerater(ROW, DATA_LEN);
			unsigned int *array_len1 = new unsigned int(0);
			GET_THREAD_GROUP().create_thread(bind(&thread_index_scan, 
				GET_PARAM(), 
				ind_id[tmp_ind], 
				index3_info, 
				rel_id[i], 
				heap_info, 
				&isi2[j], 
				found_dg1, 
				array_len1));
			v_scan_on3.push_back(pair<unsigned int *, DataGenerater *>(array_len1, found_dg1));
		}
		++tmp_ind;
	}
	GET_THREAD_GROUP().join_all();

	int vc_tmp_pos = 0;
	//获取扫描结果
	vector<pair<unsigned int *, DataGenerater *> >::iterator it_on1 = v_scan_on1.begin();
	vector<pair<unsigned int *, DataGenerater *> >::iterator it_on3 = v_scan_on3.begin();
	int test_success = true;
	for(int i = 0; i < v_dg.size(); ++i)
	{
		//获取第一列的查询结果
		CheckResultInfo cri;
		cri.ncol = INDEX_NUMS;
		cri.col_array = (unsigned int *)malloc(4 * sizeof(unsigned int));
		cri.col_array[0] = 3;
		cri.col_array[1] = 3;
		cri.col_array[2] = 3;
		cri.col_array[3] = 3;
		char *c[] = {"[bc"};
		cri.cmp_data = c;
		cri.cmp_stn = (StrategyNumber *)malloc(INDEX_NUMS * sizeof(StrategyNumber));
		cri.cmp_stn[0] = BTGreaterStrategyNumber;
		cri.col_num = (unsigned int *)malloc(INDEX_NUMS * sizeof(unsigned int));
		cri.col_num[0] = 1;
		char data[ROW][DATA_LEN];
		char det_data[ROW][DATA_LEN];
		memset(det_data, 0, ROW * DATA_LEN);
		unsigned int return_len = 0;
		v_dg[i]->dataToDataArray2D(data);
		get_result_in_array(data, ROW, cri, det_data, return_len);
		for(int j = 0; j < THREAD_SCAN_NUM; ++j)
		{
			char tmp_det_data[ROW][DATA_LEN];
			it_on1->second->dataToDataArray2D(tmp_det_data);
			if(!check_array_equal(det_data, return_len, tmp_det_data, *it_on1->first))
			{
				test_success = false;
				break;
			}
			++it_on1;
		}
		if(test_success == false)
		{
			break;
		}

		//获取第三列的查询结果
		char *c1[] = {"!$@"};
		cri.cmp_data = c1;
		cri.cmp_stn[0] = BTGreaterStrategyNumber;
		cri.col_num[0] = 3;
		return_len = 0;
		memset(det_data, 0, ROW * DATA_LEN);
		get_result_in_array(data, ROW, cri, det_data, return_len);
		for(int j = 0; j < THREAD_SCAN_NUM; ++j)
		{
			char tmp_det_data[ROW][DATA_LEN];
			memset(tmp_det_data, 0, ROW * DATA_LEN);
			it_on3->second->dataToDataArray2D(tmp_det_data);
			if(!check_array_equal(det_data, return_len, tmp_det_data, *it_on3->first))
			{
				test_success = false;
				break;
			}
			++it_on3;
		}
		if(test_success == false)
		{
			break;
		}
	}

	for(int i = 0; i < v_dg.size(); ++i)
	{
		delete v_dg[i];
	}
	for(int i = 0; i < v_scan_on1.size(); ++i)
	{
		delete v_scan_on1[i].first;
		delete v_scan_on1[i].second;
	}
	for(int i = 0; i < v_scan_on3.size(); ++i)
	{
		delete v_scan_on3[i].first;
		delete v_scan_on3[i].second;
	}	
	int drop_pos = 0;
	for(int j = 0; j < HEAP_NUM; ++j)
	{
		int drop_sta = 0;
		for(int i = 0; i < INDEX_NUM/HEAP_NUM; ++i)
		{
			SIMPLE_DROP_INDEX(rel_id[j], ind_id[drop_pos], drop_sta);
			++drop_pos;
		}
		SIMPLE_DROP_HEAP(rel_id[j], drop_sta);
	}
	FREE_PARAM(BackendParameters *);
	for(int i = 0; i < v_tmp_sg.size(); ++i)
	{
		delete v_tmp_sg[i];
	}

	return test_success;
}

int test_thread_indexscan_004()
{
	++INDEX_ID;
	INTENT("建表，插入测试数据，建索引。使用变长字段为索引列。"
				 "测试变长字段能否正确索引");

	using namespace boost;
	using namespace std;
	PREPARE_TEST();

	SpliterGenerater sg;
	//表分割为二列,使用第二列作为变长字段
	Colinfo heap_info = sg.buildHeapColInfo(2, 10, 5);
	int col_number[] = {2};
	CompareCallback ccb[] = {my_compare_str};
	Colinfo index_info = sg.buildIndexColInfo(1, col_number, ccb, SpliterGenerater::index_split_to_any<2>);
	{
		int *create_sta = new int(0);
		SIMPLE_CREATE_HEAP(RELID, (*create_sta));
		delete create_sta;
	}

#undef ROW
#define ROW 99
	const int D_LEN = 11;
	//构建测试数据
	DataGenerater *dg = new DataGenerater(ROW, D_LEN);
	dg->dataGenerate();
	char data[ROW][DATA_LEN];
	memset(data, 0, ROW * DATA_LEN);
	dg->dataToDataArray2D(data);

	//构建3种长度的数据
	const char *CAT_DATA1 = "ABCDE";
	const char *CAT_DATA2 = "ABC";
	const char *CAT_DATA3 = "d";
	for(int i = 0; i < ROW; i += 3)
	{
		/* 这里由于最后一个位置被\0取代，所以要从D_LEN - 1处开始拷贝 */
		memcpy(&data[i][D_LEN - 1], CAT_DATA1, strlen(CAT_DATA1));
		memcpy(&data[i + 1][D_LEN - 1], CAT_DATA2, strlen(CAT_DATA2));
		memcpy(&data[i + 2][D_LEN - 1], CAT_DATA3, strlen(CAT_DATA3));
	}
	{
		int insert_sta = 0;
		SIMPLE_INSERT_DATA(RELID, data, DATA_LEN, insert_sta);
	}
	delete dg;

	{
		int create_sta = 0;
		SIMPLE_CREATE_INDEX(RELID, INDEX_ID, heap_info, index_info, create_sta);
	}

	VectorMgr<DataGenerater *> vm_dg1, vm_dg2;
#undef THREAD_SCAN_NUM
#define THREAD_SCAN_NUM 10
	/* 使用两种种不同的扫描策略 */
	VectorMgr<IndexScanInfo *> *vm_isi = new VectorMgr<IndexScanInfo *>;
	VectorMgr<unsigned int *> *vm_ui = new VectorMgr<unsigned int *>;
	for(int i = 0; i < THREAD_SCAN_NUM; ++i)
	{
		{
			IndexScanInfo *isi = new IndexScanInfo;
			vm_isi->vPush(isi);
			alloc_scan_space(1, *isi);
			isi->cmp_func[0] = my_compare_str;
			isi->cmp_strategy[0] = BTGreaterEqualStrategyNumber;//小于等于"ABC"
			isi->cmp_values[0] = fdxdb_string_formdatum(CAT_DATA2, strlen(CAT_DATA2));
			isi->col_array[0] = 1;
			init_scan_key(*isi);
			DataGenerater *dg = new DataGenerater(0, DATA_LEN);
			vm_dg1.vPush(dg);
			unsigned int *array_len = new unsigned int(0);
			vm_ui->vPush(array_len);

			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(bind(thread_index_scan, GET_PARAM(), INDEX_ID, index_info, RELID, heap_info, isi, dg, array_len));
			GET_THREAD_GROUP().join_all();
		}

		{
			IndexScanInfo *isi1 = new IndexScanInfo;
			vm_isi->vPush(isi1);
			alloc_scan_space(1, *isi1);
			isi1->cmp_func[0] = my_compare_str;
			isi1->cmp_strategy[0] = BTGreaterEqualStrategyNumber;//大于等于"ABCDE"
			isi1->cmp_values[0] = fdxdb_string_formdatum(CAT_DATA1, strlen(CAT_DATA1));
			isi1->col_array[0] = 1;
			init_scan_key(*isi1);
			DataGenerater *dg1 = new DataGenerater(0, DATA_LEN);
			vm_dg2.vPush(dg1);
			unsigned int *array_len1 = new unsigned int(0);
			vm_ui->vPush(array_len1);

			GET_PARAM() = get_param();
			SAVE_PARAM(GET_PARAM());
			GET_THREAD_GROUP().create_thread(bind(thread_index_scan, GET_PARAM(), INDEX_ID, index_info, RELID, heap_info, isi1, dg1, array_len1));
			GET_THREAD_GROUP().join_all();
		}
	}
	for(int i = 0; i < vm_isi->getVMgr().size(); ++i)
	{
		free_scan_key(*(vm_isi->getVMgr()[i]));
	}

	/* 检查结果 */
	CheckResultInfo cri;
	char *c[] = {const_cast<char*>(CAT_DATA2)};
	cri.ncol = 1;
	cri.cmp_data = c;
	cri.cmp_stn = (StrategyNumber*)malloc(sizeof(StrategyNumber) * cri.ncol);
	cri.cmp_stn[0] = BTGreaterEqualStrategyNumber;
	cri.col_array = (unsigned int*)malloc(sizeof(unsigned int) * 2);
	cri.col_array[0] = 10;
	cri.col_array[1] = 5;
	cri.col_num = (unsigned int*)malloc(sizeof(unsigned int) * cri.ncol);
	cri.col_num[0] = 2;
	char det_data1[ROW][DATA_LEN];
	unsigned int det_array1 = 0;
	memset(det_data1, 0, ROW * DATA_LEN);
	get_result_in_array(data, ROW, cri, det_data1, det_array1);

	char *c1[] = {const_cast<char*>(CAT_DATA1)};
	cri.cmp_data = c1;
	cri.cmp_stn[0] = BTGreaterEqualStrategyNumber;
	char det_data2[ROW][DATA_LEN];
	unsigned int det_array2 = 0;
	memset(det_data2, 0, ROW * DATA_LEN);
	get_result_in_array(data, ROW, cri, det_data2, det_array2);

	/* 检查第一个索引条件的结果 */
	int test_success = true;
	int nSize = vm_dg1.getVMgr().size();
	for(int i = 0; i < nSize; ++i)
	{
		char tmp_data[ROW][DATA_LEN];
		memset(tmp_data, 0, ROW * DATA_LEN);
		vm_dg1.getVMgr()[i]->dataToDataArray2D(tmp_data);
		if(!check_array_equal(det_data1, det_array1, tmp_data, vm_dg1.getVMgr()[i]->getSize()))
		{
			test_success = false;
			break;
		}
	}

	/* 检查第二个索引条件的结果 */
	if(test_success)
	{
		for(int i = 0; i < vm_dg2.getVMgr().size(); ++i)
		{
			char tmp_data[ROW][DATA_LEN];
			memset(tmp_data, 0, ROW * DATA_LEN);
			vm_dg2.getVMgr()[i]->dataToDataArray2D(tmp_data);
			if(!check_array_equal(det_data2, det_array2, tmp_data, vm_dg2.getVMgr()[i]->getSize()))
			{
				test_success = false;
				break;
			}
		}
	}

	delete vm_isi;
	delete vm_ui;
	FREE_PARAM(BackendParameters *);

	{
		int drop_sta = 0;
		SIMPLE_DROP_INDEX(RELID, INDEX_ID, drop_sta);
		SIMPLE_DROP_HEAP(RELID, drop_sta);
	}

	return test_success;
}

static void threadCreate_Heap(void *param,int rel_id, const Colinfo heap_info)
{
	fxdb_SubPostmaster_Main(param);
	create_heap(rel_id,heap_info);
	proc_exit(0);
}

extern void prepare_param(BackendParameters *paramArr[],const int nThread);
extern void free_param(BackendParameters *paramArr[],const int nThread);
void threadCreateHeap(int rel_id, const Colinfo heap_info)
{
	boost::thread_group tg;
	BackendParameters *param[1];
	prepare_param(param,1);
	setColInfo(rel_id, heap_info);
	tg.create_thread(boost::bind(&threadCreate_Heap,param[0],rel_id,heap_info));
	tg.join_all();
	free_param(param,1);
}

static void threadinsert_data(void *param,
							  const char data[][DATA_LEN], 
							  const int array_len, 
							  const int data_len,
							  const int rel_id)
{
	fxdb_SubPostmaster_Main(param);
	insert_data(data,array_len,data_len,rel_id);
	proc_exit(0);
}

void thread_insert_data(const char data[][DATA_LEN], 
						const int array_len, 
						const int data_len,
						const int rel_id = RELID)
{
	boost::thread_group tg;
	BackendParameters *param[1];
	prepare_param(param,1);
	tg.create_thread(boost::bind(&threadinsert_data,param[0],data,array_len,data_len,rel_id));
	tg.join_all();
	free_param(param,1);
}

static void create_index_not_set_colinfo(int heap_id, Colinfo heap_colinfo, Colinfo ind_colinfo, int ind_id, bool unique)
{
	using namespace FounderXDB::StorageEngineNS;

	try
	{
//		setColInfo(ind_id,ind_colinfo);
		begin_transaction();
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		Relation heap_relation = FDPG_Heap::fd_heap_open(heap_id,RowExclusiveLock);

		if(unique)
		{
			FDPG_Index::fd_index_create(heap_relation, BTREE_UNIQUE_TYPE,ind_id,ind_id);
		}
		else
		{
			FDPG_Index::fd_index_create(heap_relation,BTREE_TYPE,ind_id,ind_id);
		}
		FDPG_Heap::fd_heap_close(heap_relation, RowExclusiveLock);
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
}

static void threadcreate_index_not_set_colinfo(void *param,int heap_id, Colinfo heap_colinfo, Colinfo ind_colinfo, int ind_id, bool unique)
{
	fxdb_SubPostmaster_Main(param);
	create_index_not_set_colinfo(heap_id,heap_colinfo,ind_colinfo,ind_id,unique);
	proc_exit(0);
}

static void threadcreate_index(void *param,int heap_id, Colinfo heap_colinfo, Colinfo ind_colinfo, int ind_id, bool unique)
{
	fxdb_SubPostmaster_Main(param);
	create_index(heap_id,heap_colinfo,ind_colinfo,ind_id,unique);
	proc_exit(0);
}

void thread_create_index(int heap_id, Colinfo heap_colinfo, Colinfo ind_colinfo, int ind_id = INDEX_ID, bool unique = false)
{
	boost::thread_group tg;
	BackendParameters *param[1];
	prepare_param(param,1);
	tg.create_thread(boost::bind(&threadcreate_index,param[0],heap_id,heap_colinfo,ind_colinfo,ind_id,unique));
	tg.join_all();
	free_param(param,1);
}

void thread_create_index_n(BackendParameters *param[],const int thread_num,const int index_num,int heap_id, Colinfo heap_colinfo, Colinfo ind_colinfo, int ind_id = INDEX_ID, bool unique = false)
{

	boost::thread_group tg;

	for(int i = 0; i < thread_num; ++i)
	{
		for(int j = 0; j < index_num; ++j)
		{
//			setColInfo(heap_id,heap_colinfo);
			setColInfo(ind_id+i*index_num+j,ind_colinfo);
		}
	}

	for(int i = 0; i < thread_num; ++i)
	{
		
		for(int j = 0; j < index_num; ++j)
		{
//			setColInfo(heap_id,heap_colinfo);
//			setColInfo(ind_id+i*index_num+j,ind_colinfo);
			tg.create_thread(boost::bind(&threadcreate_index_not_set_colinfo,param[i*index_num+j],heap_id,heap_colinfo,ind_colinfo,ind_id+i*index_num+j,unique));
			++INDEX_ID;
		}

	}	
	tg.join_all();
//	free_param(param,index_num * thread_num);
}

static void threadremove_index(void *param,const int heap_id, int ind_id)
{
	fxdb_SubPostmaster_Main(param);
	remove_index(heap_id,ind_id);
	proc_exit(0);
}

void thread_remove_index(const int heap_id, int ind_id = INDEX_ID)
{
	++INDEX_ID;
	boost::thread_group tg;
	BackendParameters *param[1];
	prepare_param(param,1);
	tg.create_thread(boost::bind(&threadremove_index,param[0],heap_id,ind_id));
	tg.join_all();
	free_param(param,1);
}

static void threadremove_heap(void *param,int &rel_id)
{
	fxdb_SubPostmaster_Main(param);
	remove_heap(rel_id);
	proc_exit(0);
}

void thread_remove_heap(int &rel_id = RELID)
{
	boost::thread_group tg;
	BackendParameters *param[1];
	prepare_param(param,1);
	tg.create_thread(boost::bind(&threadremove_heap,param[0],boost::ref(rel_id)));
	tg.join_all();
	free_param(param,1);
}

#define THREAD_NUMS_2 2
#define THREAD_NUMS_1 1
#define THREAD_NUMS_50 50
bool test_thread_index_scan_005()
{
	++INDEX_ID;
	INTENT("多线程测试，先插入数据再建索引，起多个线程向前扫描，索引列为第1列和第2列");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	
	const unsigned int thread_num = 80;
	const unsigned int data_row = 5;
	SpliterGenerater sg;

	//创建一个3列的表，各列长度为4 2 1
	Colinfo heap_info = sg.buildHeapColInfo(3, 4, 2, 1);
//	create_heap(RELID, heap_info);
	threadCreateHeap(RELID, heap_info);

	char data[data_row][DATA_LEN] = 
	{
		"dddd33m",
		"cccc22f",
		"eeee11m",
		"aaaa44f",
		"ffff33m"
	};

//	insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN);
	thread_insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN);
	
	//创建一个两列的索引，索引列为1、2列
	const int index_col_num = 2;
	int col_number[index_col_num] = {1, 2};
	CompareCallback cmp_func[index_col_num] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1, 2>);
//	create_index(RELID, heap_info,index_info);
	setColInfo(RELID, heap_info);
	setColInfo(INDEX_ID,index_info);
	thread_create_index(RELID, heap_info,index_info);
	/*
	* 初始化scankey
	*/
	IndexScanInfo isi[thread_num];
	for(int i = 0; i < thread_num; ++i)
	{
	alloc_scan_space(index_col_num, isi[i]);
	isi[i].cmp_values[0] = fdxdb_string_formdatum("dddd", strlen("dddd"));
	isi[i].cmp_values[1] = fdxdb_string_formdatum("33", strlen("33"));
	isi[i].cmp_func[0] = my_compare_str;
	isi[i].cmp_func[1] = my_compare_str;
	isi[i].cmp_strategy[0] = BTGreaterEqualStrategyNumber;
	isi[i].cmp_strategy[1] = BTLessStrategyNumber;
	isi[i].col_array[0] = 1;
	isi[i].col_array[1] = 2;
	init_scan_key(isi[i]);
	}
	/*
	* 创建一个线程利用索引查询数据
	*/
	PREPARE_PARAM(BackendParameters *);
	thread_group tg;	
	std::map<int,std::set<string> > map_result;
	for(int i = 0; i < thread_num; ++i)
	{
		BackendParameters *param = get_param();
		SAVE_PARAM(param);
		setColInfo(RELID,heap_info);
		setColInfo(INDEX_ID,index_info);
		tg.create_thread(bind(&thread_index_scan_map, param, INDEX_ID, index_info, RELID, heap_info, &isi[i],i+1, boost::ref(map_result)));
	}
	tg.join_all();
	FREE_PARAM(BackendParameters *);
	//free_scan_key(isi);

	std::set<string> rset;
	rset.insert("eeee11m");
 	int found = 0;
 	int notfound = 0;

	/*  检查结果，map_result.count(i)判断线程i有没有查询到数据,
	rset==map_result[i]判断线程i查询到得结果集和预期的结果集是否一致*/
	for(int i = 1; i <= thread_num; ++i)
	{
		if( map_result.count(i)==1 && rset==map_result[i])			
			++found;
		else
			++notfound;
	}
// 	remove_index(RELID);
// 	remove_heap();
	thread_remove_heap();
	cout << "查询到正确结果的线程数：  found=" << found << endl;
	cout << "未查询到正确结果的线程数: notfound=" << notfound << endl;
	if(found != thread_num)
		return false;
	return true;
}

static void myIndexInsert(HeapTuple updatedTuple,Relation heapRelation,Colinfo index_info)
{
	using namespace FounderXDB::StorageEngineNS;
	try
	{
		Datum		values[1];
		bool		isnull[1];
		isnull[0] = false;
//		Relation heapRelation = FDPG_Heap::fd_heap_open(heap_id,RowExclusiveLock, MyDatabaseId);
		Relation indexRelation = FDPG_Index::fd_index_open(INDEX_ID,AccessShareLock, MyDatabaseId);
		values[0] = fdxdb_form_index_datum(heapRelation, indexRelation, updatedTuple);

		FDPG_Index::fd_index_insert(indexRelation, /* index relation */
			values,	/* array of index Datums */
			isnull,	/* null flags */
			&(updatedTuple->t_self),		/* tid of heap tuple */
			heapRelation,	/* heap relation */
			UNIQUE_CHECK_NO);	/* type of uniqueness check to do */
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		throw StorageEngineExceptionUniversal(se);
	}
}

void insert_index(const char insert_data[][DATA_LEN], 
				 const int array_len, 
				 const int data_len,
				 const int rel_id,
				 Colinfo heap_colinfo,
				 Colinfo index_info)
{
	using namespace FounderXDB::StorageEngineNS;
	begin_transaction();
	Relation heapRelation = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
	HeapTuple tuple = NULL;
	try
	{
		for(int i = 0; i < array_len; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(insert_data[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple);
			myIndexInsert(tuple,heapRelation,index_info);
			CommandCounterIncrement();
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	heap_close(heapRelation, RowExclusiveLock);
	commit_transaction();
}

static void threadindex_insert(void *param,
							   const char data[][DATA_LEN], 
							   const int array_len, 
							   const int data_len,
							   const int rel_id,
							   Colinfo heap_colinfo,
							   Colinfo index_info)
{
	fxdb_SubPostmaster_Main(param);
	insert_index( data, array_len, data_len, rel_id,heap_colinfo,index_info);
	proc_exit(0);
}

void thread_index_insert(const char insert_data[][DATA_LEN], 
						 const int array_len, 
						 const int data_len,
						 const int rel_id,
						 Colinfo heap_colinfo,
						 Colinfo index_info)
{
	boost::thread_group tg;
	BackendParameters *param[1];
	prepare_param(param,1);
	tg.create_thread(boost::bind(&threadindex_insert,param[0],insert_data, array_len, data_len, rel_id,heap_colinfo,index_info));
	tg.join_all();
	free_param(param,1);
}

bool test_thread_index_scan_006()
{
	++INDEX_ID;
	INTENT("多线程测试，先建立索引再插数据，再更新索引，起多个线程向前扫描，索引列为第1列和第2列");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;

	const unsigned int thread_num = 80;//THREAD_NUMS_50;
	const unsigned int data_row = 5;
	SpliterGenerater sg;

	//创建一个3列的表，各列长度为4 2 1
	Colinfo heap_info = sg.buildHeapColInfo(3, 4, 2, 1);
	setColInfo(RELID, heap_info);
	threadCreateHeap(RELID, heap_info);

	//创建一个两列的索引，索引列为1、2列
	const int index_col_num = 2;
	int col_number[index_col_num] = {1, 2};
	CompareCallback cmp_func[index_col_num] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1, 2>);
	//	create_index(RELID, heap_info,index_info);
	setColInfo(INDEX_ID, index_info);
	thread_create_index(RELID, heap_info,index_info);

	char data[data_row][DATA_LEN] = 
	{
		"dddd33m",
		"cccc22f",
		"eeee11m",
		"aaaa44f",
		"ffff33m"
	};
	//插入数据，并更新索引

	thread_index_insert(data, ARRAY_LEN_CALC(data),DATA_LEN, RELID, heap_info,index_info);

	/*
	* 初始化scankey
	*/
	IndexScanInfo isi[thread_num];
	for(int i = 0; i < thread_num; ++i){
	alloc_scan_space(index_col_num, isi[i]);
	isi[i].cmp_values[0] = fdxdb_string_formdatum("dddd", strlen("dddd"));
	isi[i].cmp_values[1] = fdxdb_string_formdatum("33", strlen("33"));
	isi[i].cmp_func[0] = my_compare_str;
	isi[i].cmp_func[1] = my_compare_str;
	isi[i].cmp_strategy[0] = BTGreaterEqualStrategyNumber;
	isi[i].cmp_strategy[1] = BTLessStrategyNumber;
	isi[i].col_array[0] = 1;
	isi[i].col_array[1] = 2;
	init_scan_key(isi[i]);
	}
	/*
	* 创建一个线程利用索引查询数据
	*/
	PREPARE_PARAM(BackendParameters *);
	thread_group tg;	
	std::map<int,std::set<string> > map_result;
	for(int i = 0; i < thread_num; ++i)
	{
		BackendParameters *param = get_param();
		SAVE_PARAM(param);
		setColInfo(RELID, heap_info);
		setColInfo(INDEX_ID, index_info);
		tg.create_thread(bind(&thread_index_scan_map, param, INDEX_ID, index_info, RELID, heap_info, &isi[i],i+1, boost::ref(map_result)));
	}
	tg.join_all();
	FREE_PARAM(BackendParameters *);
	//free_scan_key(isi);

	std::set<string> rset;
	rset.insert("eeee11m");
	int found = 0;
	int notfound = 0;

	/*  检查结果，map_result.count(i)判断线程i有没有查询到数据,
	rset==map_result[i]判断线程i查询到得结果集和预期的结果集是否一致*/
	for(int i = 1; i <= thread_num; ++i)
	{
		if( map_result.count(i)==1 && rset==map_result[i])			
			++found;
		else
			++notfound;
	}
	// 	remove_index(RELID);
	// 	remove_heap();
	thread_remove_heap();
	cout << "查询到正确结果的线程数：  found=" << found << endl;
	cout << "未查询到正确结果的线程数: notfound=" << notfound << endl;
	if(found != thread_num)
		return false;
	return true;
}

bool test_thread_index_scan_007()
{
	++INDEX_ID;
	INTENT("多线程测试，先插入数据再建索引，再插数据并更新索引，起多个线程向前扫描，索引列为第1列和第2列");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	using namespace boost::assign;


	const unsigned int thread_num = 80;//THREAD_NUMS_50;
	const unsigned int data_row = 5;
	SpliterGenerater sg;

	//创建一个3列的表，各列长度为4 2 1
	Colinfo heap_info = sg.buildHeapColInfo(3, 4, 2, 1);
	setColInfo(RELID,heap_info);
	threadCreateHeap(RELID, heap_info);

	char data[data_row][DATA_LEN] = 
	{
		"dddd33m",
		"cccc22f",
		"eeee11m",
		"aaaa44f",
		"ffff33m"
	};
	thread_insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN);

	//创建一个两列的索引，索引列为1、2列
	const int index_col_num = 2;
	int col_number[index_col_num] = {1, 2};
	CompareCallback cmp_func[index_col_num] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1, 2>);
	setColInfo(INDEX_ID,index_info);
	thread_create_index(RELID, heap_info,index_info);

	char data2[data_row][DATA_LEN] = 
	{
		"dddd22m",
		"cccc22f",
		"eeee11m",
		"aaaa44f",
		"ffff11m"
	};
	//插入数据，并更新索引
	thread_index_insert(data2, ARRAY_LEN_CALC(data2),DATA_LEN, RELID, heap_info,index_info);

	/*
	* 初始化scankey
	*/
	IndexScanInfo isi[thread_num];
		for(int i = 0; i < thread_num; ++i)
	{
	alloc_scan_space(index_col_num, isi[i]);
	isi[i].cmp_values[0] = fdxdb_string_formdatum("dddd", strlen("dddd"));
	isi[i].cmp_values[1] = fdxdb_string_formdatum("33", strlen("33"));
	isi[i].cmp_func[0] = my_compare_str;
	isi[i].cmp_func[1] = my_compare_str;
	isi[i].cmp_strategy[0] = BTGreaterEqualStrategyNumber;
	isi[i].cmp_strategy[1] = BTLessStrategyNumber;
	isi[i].col_array[0] = 1;
	isi[i].col_array[1] = 2;
	init_scan_key(isi[i]);
	}
	/*
	* 创建一个线程利用索引查询数据
	*/
	PREPARE_PARAM(BackendParameters *);
	thread_group tg;	
	std::map<int,std::set<string> > map_result;
	for(int i = 0; i < thread_num; ++i)
	{
		BackendParameters *param = get_param();
		SAVE_PARAM(param);
		tg.create_thread(bind(&thread_index_scan_map, param, INDEX_ID, index_info, RELID, heap_info, &isi[i],i+1, boost::ref(map_result)));
	}
	tg.join_all();
	FREE_PARAM(BackendParameters *);
	//free_scan_key(isi);

	std::set<string> rset;
	rset+="eeee11m","dddd22m","ffff11m";
	int found = 0;
	int notfound = 0;

	/*  检查结果，map_result.count(i)判断线程i有没有查询到数据,
	rset==map_result[i]判断线程i查询到得结果集和预期的结果集是否一致*/
	for(int i = 1; i <= thread_num; ++i)
	{
		if( map_result.count(i)==1 && rset==map_result[i])			
			++found;
		else
			++notfound;
	}
	thread_remove_heap();
	cout << "查询到正确结果的线程数：  found=" << found << endl;
	cout << "未查询到正确结果的线程数: notfound=" << notfound << endl;
	if(found != thread_num)
		return false;
	return true;
}

//计算数字位数，为了分配合适的列大小
int numLenth(int num)
{
	int num_len = 0;
	for(int i = num ; i!=0 ; i/=10)
	{
		++num_len;
	}
	return num_len;
}

extern void thread_insert_large_data(char *pdataArr[], 
									 const int array_len, 
									 const int data_len, 
									 const Oid rel_id, 
									 BackendParameters *param, 
									 int *sta);

void thread_index_insert_large_data(char *pdataArr[], 
									const int array_len, 
									const int data_len, 
									const Oid rel_id,
									int *sta)
{
	int num_len = numLenth(array_len);
	boost::thread_group tg;
	BackendParameters *param[1];
	prepare_param(param,1);
	tg.create_thread(boost::bind(&thread_insert_large_data,pdataArr, array_len, data_len+num_len, rel_id,param[0],sta));
	tg.join_all();
	free_param(param,1);
}

void insert_large_data_update_index(char *pdataArr[],
									const int array_len, 
									const int data_len,
									const int rel_id,
									Colinfo heap_colinfo,
									Colinfo index_info,
									BackendParameters *param)
{
	using namespace FounderXDB::StorageEngineNS;
	fxdb_SubPostmaster_Main(param);
	begin_transaction();
	Relation heapRelation = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
	HeapTuple tuple = NULL;
	try
	{
		for(int i = 0; i < array_len; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(pdataArr[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple);
			myIndexInsert(tuple,heapRelation,index_info);
			CommandCounterIncrement();
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	heap_close(heapRelation, RowExclusiveLock);
	commit_transaction();
	proc_exit(0);
}

extern char *my_itoa(int value, char *string, int radix);

void thread_insert_large_data_update_index(char *pdataArr[], 
									const int array_len, 
									const int data_len, 
									const Oid rel_id,
									Colinfo heap_colinfo,
									Colinfo index_info)
{
	int num_len = numLenth(array_len);
	boost::thread_group tg;
	BackendParameters *param[1];
	prepare_param(param,1);
	tg.create_thread(boost::bind(&insert_large_data_update_index,pdataArr, 
		array_len, data_len+num_len, rel_id,heap_colinfo,index_info,param[0]));
	tg.join_all();
	free_param(param,1);
}



bool test_thread_index_scan_008()
{
	++INDEX_ID;
	INTENT("多线程测试，大数据量，先插入数据再建索引，多个线程扫描，索引列为最后一个变长列");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	using namespace boost::assign;

	const unsigned int thread_num = 80;
	const unsigned int data_row = 1000;
	const unsigned int data_size = 10;
	const unsigned int num_lenth = numLenth(data_row);
	SpliterGenerater sg;
	int rid = RELID;

	//创建一个2列的表,第1列为data_size，第2列为变长，长度取决于最大行号的位数
	Colinfo heap_info = sg.buildHeapColInfo(2, data_size,num_lenth);
	setColInfo(rid,heap_info);
	threadCreateHeap(rid, heap_info);

	char *pDataArr[data_row] = {NULL};
	srand((unsigned) time(NULL)); /*播种子*/ 
	for(int i = 0; i < data_row; ++i)
	{
		pDataArr[i] = make_unique_data(data_size,i+1);
//		printf("%s\n",pDataArr[i]);
	}
	int sta[thread_num] = {0};
	thread_index_insert_large_data(pDataArr, data_row, data_size, rid,sta);

	//创建一个1列的索引，索引列为第2列
	const int index_col_num = 1;
	int col_number[index_col_num] = {2};
	CompareCallback cmp_func[index_col_num] = 
	{
		my_compare_int
	};
	Colinfo index_info = sg.buildIndexColInfo(1, col_number, cmp_func, SpliterGenerater::index_split_to_any<2>);
	setColInfo(INDEX_ID,index_info);
	thread_create_index(rid, heap_info,index_info);

	/*
	* 初始化scankey
	*/
	
	IndexScanInfo isi[thread_num];	
	for(int i = 0; i < thread_num; ++i)
	{
		alloc_scan_space(index_col_num, isi[i]);
		char str[20];
		my_itoa(data_row,str,10);//比较条件为第2列小于列号
		isi[i].cmp_values[0] = fdxdb_string_formdatum(str, strlen(str));
		isi[i].cmp_func[0] = my_compare_int;
		isi[i].cmp_strategy[0] = BTLessEqualStrategyNumber;
		isi[i].col_array[0] = 1;
		init_scan_key(isi[i]);
	}
	/*
	* 创建一个线程利用索引查询数据
	*/
	PREPARE_PARAM(BackendParameters *);
	thread_group tg;	
	std::map<int,std::set<string> > map_result;
	for(int i = 0; i < thread_num; ++i)
	{
		BackendParameters *param = get_param();
		SAVE_PARAM(param);
		tg.create_thread(bind(&thread_index_scan_map, param, INDEX_ID, index_info, RELID, heap_info, &isi[i],i+1, boost::ref(map_result)));
	}
	tg.join_all();
	FREE_PARAM(BackendParameters *);
	//free_scan_key(isi);

	std::set<string> rset;
	for(int i = 0; i < data_row; ++i)
	{
		rset += pDataArr[i];
	}
	int found = 0;
	int notfound = 0;

	/*  检查结果，map_result.count(i)判断线程i有没有查询到数据,
	rset==map_result[i]判断线程i查询到得结果集和预期的结果集是否一致*/
	for(int i = 1; i <= thread_num; ++i)
	{
		if( map_result.count(i)==1 && rset==map_result[i])			
			++found;
		else
			++notfound;
	}
	for(int i = 0; i < data_row; ++i)
	{
		free(pDataArr[i]);
	}
	thread_remove_heap();
	cout << "查询到正确结果的线程数：  found=" << found << endl;
	cout << "未查询到正确结果的线程数: notfound=" << notfound << endl;
	if(found != thread_num)
		return false;
	return true;
}

bool test_create_index_abort()
{
	INTENT("测试在一个事务中为一个表创建多个索引，然后回滚事务。"
				 "启动新的事务，对表handle上的索引数目执行检查.");

	using namespace FounderXDB::StorageEngineNS;

	bool return_sta = true;

	++INDEX_ID;
	Oid idx_id1 = INDEX_ID;
	++INDEX_ID;
	Oid idx_id2 = INDEX_ID;
	++RELID;

	SpliterGenerater sg;

	Colinfo heap_info = sg.buildHeapColInfo(1, 3);
	int col_num[] = {1};
	CompareCallback cmp_func[] = {my_compare_str};
	Colinfo index_info = sg.buildIndexColInfo(1, col_num, cmp_func, SpliterGenerater::index_split_to_any<1>);
	setColInfo(RELID, heap_info);
	setColInfo(idx_id1, index_info);
	setColInfo(idx_id2, index_info);

	try
	{
		begin_transaction();

		FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, RELID, MyDatabaseId, RELID);

		Relation rel = FDPG_Heap::fd_heap_open(RELID, ShareLock, MyDatabaseId);

		FDPG_Index::fd_index_create(rel, BTREE_TYPE, idx_id1, idx_id1);
		FDPG_Index::fd_index_create(rel, BTREE_TYPE, idx_id2, idx_id2);

		FDPG_Heap::fd_heap_close(rel, ShareLock);

		commit_transaction();

		begin_transaction();

		rel = FDPG_Heap::fd_heap_open(RELID, ShareLock, MyDatabaseId);

		/* 此时该结构体上应该有两个索引 */
		return_sta = (return_sta ? (rel->mt_info.indinfo.index_num == 2) : false);

		FDPG_Index::fd_index_drop(RELID, MyDatabaseTableSpace, idx_id1, MyDatabaseId);

		/* 删除了一个索引，此时handle上有一个索引 */
		return_sta = (return_sta ? (rel->mt_info.indinfo.index_num == 1) : false);

		FDPG_Heap::fd_heap_close(rel, ShareLock);

		/* 这里回滚事务 */
		user_abort_transaction();

		begin_transaction();

		/* 此时打开表，handle应该是需要重新构造，handle上依然有两个索引 */
		rel = FDPG_Heap::fd_heap_open(RELID, AccessShareLock, MyDatabaseId);

		return_sta = (return_sta ? (rel->mt_info.indinfo.index_num == 2) : false);

		FDPG_Heap::fd_heap_close(rel, AccessShareLock);

		commit_transaction();
	} catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		return_sta = false;
	}

	return return_sta;
}

bool test_thread_index_scan_009() 
{
	++INDEX_ID;
	INTENT("多线程测试，大数据量，先建索引再插入数据，多个线程扫描，索引列为最后一个变长列");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	using namespace boost::assign;

	const unsigned int thread_num = 80;
	const unsigned int data_row = 1000;
	const unsigned int data_size = 10;
	const unsigned int num_lenth = numLenth(data_row);
	SpliterGenerater sg;
	int rid = RELID;

	//创建一个2列的表,第1列为data_size，第2列为变长，长度取决于最大行号的位数
	Colinfo heap_info = sg.buildHeapColInfo(2, data_size,num_lenth);
	setColInfo(rid,heap_info);
	threadCreateHeap(rid, heap_info);

	char *pDataArr[data_row] = {NULL};
	srand((unsigned) time(NULL)); /*播种子*/ 
	for(int i = 0; i < data_row; ++i)
	{
		pDataArr[i] = make_unique_data(data_size,i+1);
//		printf("%s\n",pDataArr[i]);
	}

	//创建一个1列的索引，索引列为第2列
	const int index_col_num = 1;
	int col_number[index_col_num] = {2};
	CompareCallback cmp_func[index_col_num] = 
	{
		my_compare_int
	};
	Colinfo index_info = sg.buildIndexColInfo(1, col_number, cmp_func, SpliterGenerater::index_split_to_any<2>);
	setColInfo(INDEX_ID,index_info);
	thread_create_index(rid, heap_info,index_info);
	//插入数据并更新索引
	thread_insert_large_data_update_index(pDataArr, data_row, data_size, rid,heap_info,index_info);

	/*
	* 初始化scankey
	*/
	IndexScanInfo isi[thread_num];
	for(int i = 0; i < thread_num; ++i)
	{
	alloc_scan_space(index_col_num, isi[i]);
	char str[20];
	my_itoa(data_row,str,10);//比较条件为第2列小于列号
	isi[i].cmp_values[0] = fdxdb_string_formdatum(str, strlen(str));
	isi[i].cmp_func[0] = my_compare_int;
	isi[i].cmp_strategy[0] = BTLessEqualStrategyNumber;
	isi[i].col_array[0] = 1;
	init_scan_key(isi[i]);
	}
	/*
	* 创建一个线程利用索引查询数据
	*/
	PREPARE_PARAM(BackendParameters *);
	thread_group tg;	
	std::map<int,std::set<string> > map_result;
	for(int i = 0; i < thread_num; ++i)
	{
		BackendParameters *param = get_param();
		SAVE_PARAM(param);
		tg.create_thread(bind(&thread_index_scan_map, param, INDEX_ID, index_info, RELID, heap_info, &isi[i],i+1, boost::ref(map_result)));
	}
	tg.join_all();
	FREE_PARAM(BackendParameters *);
	//free_scan_key(isi);

	std::set<string> rset;
	for(int i = 0; i < data_row; ++i)
	{
		rset += pDataArr[i];
	}
	int found = 0;
	int notfound = 0;

	/*  检查结果，map_result.count(i)判断线程i有没有查询到数据,
	rset==map_result[i]判断线程i查询到得结果集和预期的结果集是否一致*/
	for(int i = 1; i <= thread_num; ++i)
	{
		if( map_result.count(i)==1 && rset==map_result[i])			
			++found;
		else
			++notfound;
	}
	for(int i = 0; i < data_row; ++i)
	{
		free(pDataArr[i]);
	}
	thread_remove_heap();
	cout << "查询到正确结果的线程数：  found=" << found << endl;
	cout << "未查询到正确结果的线程数: notfound=" << notfound << endl;
	if(found != thread_num)
		return false;
	return true;
}

bool test_thread_index_create_010()
{
	++INDEX_ID;
	INTENT("先插入数据,多线程同时创建多个索引，再起多个线程同时进行查询");
	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	using namespace boost::assign;

	const unsigned int thread_num = 10;//THREAD_NUMS_50;
	const unsigned int index_num = 8;
	const unsigned int data_row = 5;
	const Oid index_id = INDEX_ID;
	
	const Oid rel_id = RELID;
	SpliterGenerater sg;


	//创建一个3列的表，各列长度为4 2 1
	Colinfo heap_info = sg.buildHeapColInfo(3, 4, 2, 1);
	setColInfo(rel_id,heap_info);
	threadCreateHeap(rel_id, heap_info);

	char data[data_row][DATA_LEN] = 
	{
		"dddd33m",
		"cccc22f",
		"eeee11m",
		"aaaa44f",
		"ffff33m"
	};
	thread_insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN);

	//创建一个两列的索引，索引列为1、2列
	const int index_col_num = 2;
	int col_number[index_col_num] = {1, 2};
	CompareCallback cmp_func[index_col_num] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1, 2>);
    setColInfo(INDEX_ID,index_info);
	BackendParameters *param[index_num * thread_num];
	prepare_param(param,index_num * thread_num);
	//起多个线程创建多个索引
	thread_create_index_n(param,thread_num,index_num,rel_id, heap_info, index_info);

	free_param(param,index_num * thread_num);

	/*
	* 初始化scankey
	*/
	IndexScanInfo isi[thread_num];
	for(int i = 0; i < thread_num; ++i)
	{
	alloc_scan_space(index_col_num, isi[i]);
	isi[i].cmp_values[0] = fdxdb_string_formdatum("dddd", strlen("dddd"));
	isi[i].cmp_values[1] = fdxdb_string_formdatum("33", strlen("33"));
	isi[i].cmp_func[0] = my_compare_str;
	isi[i].cmp_func[1] = my_compare_str;
	isi[i].cmp_strategy[0] = BTGreaterEqualStrategyNumber;
	isi[i].cmp_strategy[1] = BTLessStrategyNumber;
	isi[i].col_array[0] = 1;
	isi[i].col_array[1] = 2;
	init_scan_key(isi[i]);
	}
	/*
	* 创建一个线程利用索引查询数据
	*/
	PREPARE_PARAM(BackendParameters *);
	thread_group tg;	
	std::map<int,std::set<string> > map_result;
	for(int i = 0; i < thread_num; ++i)
	{
		BackendParameters *param = get_param();
		SAVE_PARAM(param);
		setColInfo(index_id + i,index_info);
		tg.create_thread(bind(&thread_index_scan_map, param, index_id+i, index_info, rel_id, heap_info, &isi[i],i+1, boost::ref(map_result)));
	}
	tg.join_all();
	FREE_PARAM(BackendParameters *);
	//free_scan_key(isi);

	std::set<string> rset;
	rset.insert("eeee11m");
	int found = 0;
	int notfound = 0;

	/*  检查结果，map_result.count(i)判断线程i有没有查询到数据,
	rset==map_result[i]判断线程i查询到得结果集和预期的结果集是否一致*/
	for(int i = 1; i <= thread_num; ++i)
	{
		if( map_result.count(i)==1 && rset==map_result[i])			
			++found;
		else
			++notfound;
	}
	INDEX_ID += thread_num*index_num;
	thread_remove_heap();
	cout << "查询到正确结果的线程数：  found=" << found << endl;
	cout << "未查询到正确结果的线程数: notfound=" << notfound << endl;
	if(found != thread_num)
		return false;
	return true;
}

bool myTest_test_indexscan_000()
//**任意列字段索引查询测试**
//参数说明：
//	COLUMN：创建表的字段数
//	colLen[]：各字段长度
//	scanKey[]：索引条件的结构体。成员包含：索引列号，索引键值，查询条件
//	start, end：要插入的数据范围
//测例：输入0到2000的数字，表分四个字段，每个字段占1位，查询第2个字段小于5的数据
{
	INTENT("单字段索引测试，索引列为第2列，查询条件：第二列小于5。\
		   可扩展为任意多字段以及任一列上多字段的索引查询");
	try
	{
		const int COLUMN = 4;
		const int colLen[COLUMN] = { 1, 1, 1, 1 };
		struct ScanKeyInfo scanKey[] = { 
			//{ 1, "1", BTEqualStrategyNumber },
			{ 2, "5", BTLessStrategyNumber },
			{ 2, "0", BTGreaterStrategyNumber },
			//{ 3, "5", BTLessStrategyNumber }
		};
		const int nkeys = sizeof(scanKey)/sizeof(ScanKeyInfo);
		ScanKeyInfo *pScanKey = scanKey;
		const int start = 0;
		const int end = 2000;

		//create heap
		IndexFuncTest index;
		index.buildHeapColInfo( COLUMN, colLen );
		begin_transaction();

		//open heap and insert batch data
		SAVE_INFO_FOR_DEBUG();
		char emptyData[] = "";
		int dataLen = sizeof( emptyData );
		index.openHeap( RowExclusiveLock );
		index.insertRangeTuples( start, end, emptyData, dataLen );

		//create index on heap
		index.buildIndexColInfo( nkeys, colLen, pScanKey );
		FDPG_Transaction::fd_CommandCounterIncrement();

		//inital scan keys
		index.constructKeys( pScanKey, nkeys );
		
		// open index and begin scan
		index.scanIndex( pScanKey, nkeys );

		//drop heap
		index.dropHeap();

		if( index.m_bSuccess == true )
			END_TRANSACTION
		return index.m_bSuccess;
	}
	CATCH_EXCEPTION_AND_ABORT
}

void reindex_relation_split_index_0(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
}
void reindex_relation_split_index_1(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
}

void reindex_relation_split_index_2(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
}

void reindex_relation_split_index_3(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 4;
	}
}

void reindex_relation_split_index_4(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 1;
	}
}

void reindex_index_split_index(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
}

void reindex_relation_split_heap(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 3;
	}
	if (col == 4)
	{
		rangeData.start = 8;
		rangeData.len = 12;
	}
	if (col == 5)
	{
		rangeData.start = 12;
		rangeData.len = 1;
	}
	if (col == 6)
	{
		rangeData.start = 13;
		rangeData.len = 5;
	}
	if (col == 7)
	{
		rangeData.start = 18;
		rangeData.len = 4;
	}
		
}

void reindex_index_split_heap(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 2;
	}
}

void create_heap_index_table(const Oid& heapId,const Oid& indexId,
								const size_t& nkey,const bool bflag)
{
	if (NULL == getColInfo(heapId))
	{
		Colinfo heap_info = (Colinfo)malloc(sizeof(ColinfoData));
		heap_info->col_number = 0;
		heap_info->keys = 0;
		heap_info->rd_comfunction = NULL;
		if (bflag)
			heap_info->split_function =  reindex_relation_split_heap;
		else
			heap_info->split_function =  reindex_index_split_heap;
		setColInfo(heapId,heap_info);
	}

	if (!bflag)
	{
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo->keys = nkey;
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		for (int i=0; i<nkey; ++i)
		{
			colinfo->col_number[i] = i+1;
			colinfo->rd_comfunction[i] = my_compare_str;
		}
		colinfo->split_function =  reindex_index_split_index;
		setColInfo(indexId,colinfo);
	}
	else
	{
		for(int i=0; i<nkey; i++)
		{
			Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
			colinfo->keys = 1;
			colinfo->col_number = (size_t*)malloc(sizeof(size_t));
			colinfo->rd_comfunction = (CompareCallback*)malloc(sizeof(CompareCallback));
			colinfo->col_number[0] = i+1;
			colinfo->rd_comfunction[0] = my_compare_str;
			if (0 == i)
				colinfo->split_function = reindex_relation_split_index_0;
			else if (1 == i)
				colinfo->split_function = reindex_relation_split_index_1;
			else if (2 == i)
				colinfo->split_function = reindex_relation_split_index_2;
			else if (3 == i)
				colinfo->split_function = reindex_relation_split_index_3;
			else if (4 == i)
				colinfo->split_function = reindex_relation_split_index_4;
			setColInfo(indexId+i,colinfo);
		}
	}
}

void test_index_scan(Relation& heap,Relation& index,const Oid& heapid,const Oid& indexid,const std::vector<std::string>& Scanstr,
					 StrategyNumber* ScanStrage,const int nCol,ScanKeyData* Scankey,Datum* value,std::vector<std::string>& Resultstr)
{
	Resultstr.clear();
	heap = FDPG_Heap::fd_heap_open(heapid,RowExclusiveLock, MyDatabaseId);
	index = FDPG_Index::fd_index_open(indexid,RowExclusiveLock, MyDatabaseId);

	IndexScanDesc index_scan;

	for(int i=0; i<nCol; i++)
	{
		value[i] = fdxdb_string_formdatum(Scanstr[i].c_str(), Scanstr[i].size());
		Fdxdb_ScanKeyInitWithCallbackInfo(&Scankey[i],(AttrNumber)(i+1),ScanStrage[i],my_compare_str,value[i]);	
	}
	
	index_scan = FDPG_Index::fd_index_beginscan(heap, index, SnapshotNow, nCol, 0);
	FDPG_Index::fd_index_rescan(index_scan, Scankey, nCol, NULL, 0);

	char *temp = NULL;
	int Count = 0;
	HeapTuple tuple;
	while((tuple = FDPG_Index::fd_index_getnext(index_scan, BackwardScanDirection)) != NULL)
	{
		int len = 0;
		temp = fdxdb_tuple_to_chars_with_len(tuple,len);
		Resultstr.push_back(string(temp,len));
		pfree(temp);
	}

	FDPG_Index::fd_index_endscan(index_scan);
	FDPG_Heap::fd_heap_close(heap, RowExclusiveLock);
	FDPG_Index::fd_index_close(index, AccessShareLock);
}
int test_reindex_index()
{
	int rel_id = 80010;
	int index_id = 85010;
	try
	{
		Oid relspace = MyDatabaseTableSpace;
		create_heap_index_table(rel_id,index_id,2,false);
		
		begin_transaction();
		FDPG_Heap::fd_heap_create(relspace, rel_id, 0, rel_id);
		Relation heapRelation = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(heapRelation,BTREE_TYPE,index_id,index_id);
		Relation indexRelation = FDPG_Index::fd_index_open(index_id,RowExclusiveLock, MyDatabaseId);
	
		HeapTuple tuple;
		std::vector<std::string> vString;
		vString.push_back("123456a");
		vString.push_back("123789b");
		vString.push_back("012562c");
		vString.push_back("012120d");
		vString.push_back("012456e");
		for (int i=0; i<150; ++i)
			vString.push_back("124896e");
		for (int i=0;i<155;i++)
		{
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(vString[i].c_str(), vString[i].size())) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		}

		FDPG_Heap::fd_heap_close(heapRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Transaction::fd_CommandCounterIncrement();
		
		//重建索引
		FDPG_Index::fd_reindex_index(rel_id, index_id, MyDatabaseId);

		const int nkey = 2;
		ScanKeyData Sacnkey[nkey];
		Datum value[nkey];
		std::vector<std::string> vScanStr;
		std::vector<std::string> vEndResult;
		vScanStr.push_back("011");
		vScanStr.push_back("12");
		StrategyNumber ScanStrage[nkey] = {BTGreaterStrategyNumber,BTGreaterEqualStrategyNumber};
		test_index_scan(heapRelation,indexRelation,rel_id,index_id,vScanStr,ScanStrage,nkey,Sacnkey,value,vEndResult);

		FDPG_Index::fd_index_drop(rel_id, relspace,index_id, MyDatabaseId);
		FDPG_Heap::fd_heap_drop(rel_id, MyDatabaseId);
		
		std::sort(vEndResult.begin(),vEndResult.end());
		std::sort(vString.begin(),vString.end());

		if(vEndResult == vString)
			commit_transaction();
		else
		{
			user_abort_transaction();
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	catch(...)
	{
		user_abort_transaction();
		return false;
	}
	return true;
}

int test_reindex_index_Unique_delete()
{
	int rel_id = 80410;
	int index_id = 88810;
	try
	{
		Oid relspace = MyDatabaseTableSpace;
		create_heap_index_table(rel_id,index_id,2,false);
		
		begin_transaction();
		FDPG_Heap::fd_heap_create(relspace, rel_id, 0, rel_id);
		Relation heapRelation = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(heapRelation,BTREE_UNIQUE_TYPE,index_id,index_id);
		Relation indexRelation = FDPG_Index::fd_index_open(index_id,RowExclusiveLock, MyDatabaseId);
	
		HeapTuple tuple;
		std::vector<std::string> vString;
		vString.push_back("123456a");
		vString.push_back("123789b");
		vString.push_back("012562c");
		vString.push_back("012120d");
		vString.push_back("012456e");
		vString.push_back("125ab89");
		for (int i=0;i<6;i++)
		{
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(vString[i].c_str(), vString[i].size())) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();
		
		HeapScanDesc heapScan = FDPG_Heap::fd_heap_beginscan(heapRelation,SnapshotNow, 0, NULL);
		HeapTuple heaptuple;
		int  Count = 0;
		while((heaptuple = FDPG_Heap::fd_heap_getnext(heapScan,ForwardScanDirection)) != NULL)
		{
			if (++Count > 1)
				continue;
			char* temp = fxdb_tuple_to_chars(heaptuple);
			for (int j=0; j<6; ++j)
			{
				if (0 == vString[j].compare(temp))
				{
					vString.erase(vString.begin()+j);
					FDPG_Heap::fd_simple_heap_delete(heapRelation,&heaptuple->t_self);
					break;
				}
			}
		};
		
		FDPG_Heap::fd_heap_endscan(heapScan);
		
		FDPG_Heap::fd_heap_close(heapRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Transaction::fd_CommandCounterIncrement();
		
		//重建索引
		FDPG_Index::fd_reindex_index(rel_id, index_id, MyDatabaseId);

		const int nkey = 2;
		ScanKeyData Sacnkey[nkey];
		Datum value[nkey];
		std::vector<std::string> vScanStr;
		std::vector<std::string> vEndResult;
		vScanStr.push_back("011");
		vScanStr.push_back("12");
		StrategyNumber ScanStrage[nkey] = {BTGreaterStrategyNumber,BTGreaterEqualStrategyNumber};
		test_index_scan(heapRelation,indexRelation,rel_id,index_id,vScanStr,ScanStrage,nkey,Sacnkey,value,vEndResult);

		FDPG_Index::fd_index_drop(rel_id, relspace,index_id, MyDatabaseId);
		FDPG_Heap::fd_heap_drop(rel_id, MyDatabaseId);
		
		std::sort(vEndResult.begin(),vEndResult.end());
		std::sort(vString.begin(),vString.end());

		if(vEndResult == vString)
			commit_transaction();
		else
		{
			user_abort_transaction();
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	catch(...)
	{
		user_abort_transaction();
		return false;
	}
	return true;
}

int test_reindex_index_Unique_Update()
{
	int rel_id = 90010;
	int index_id = 90110;
	try
	{
		Oid relspace = MyDatabaseTableSpace;
		create_heap_index_table(rel_id,index_id,2,false);
		
		begin_transaction();
		FDPG_Heap::fd_heap_create(relspace, rel_id, 0, rel_id);
		Relation heapRelation = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(heapRelation,BTREE_UNIQUE_TYPE,index_id,index_id);
		Relation indexRelation = FDPG_Index::fd_index_open(index_id,RowExclusiveLock, MyDatabaseId);
	
		HeapTuple tuple;
		std::vector<std::string> vString;
		vString.push_back("123456a");
		vString.push_back("123789b");
		vString.push_back("012562c");
		vString.push_back("012120d");
		vString.push_back("012456e");
		vString.push_back("125ab89");
		for (int i=0;i<6;i++)
		{
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(vString[i].c_str(), vString[i].size())) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();
		
		HeapScanDesc heapScan = FDPG_Heap::fd_heap_beginscan(heapRelation,SnapshotNow, 0, NULL);
		HeapTuple heaptuple;
		int  Count = 0;
		while((heaptuple = FDPG_Heap::fd_heap_getnext(heapScan,ForwardScanDirection)) != NULL)
		{
			if (++Count > 1)
				continue;
			char* temp = fxdb_tuple_to_chars(heaptuple);
			for (int j=0; j<6; ++j)
			{
				if (0 == vString[j].compare(temp))
				{
					vString.erase(vString.begin()+j);
					HeapTuple UpdateTuple = fdxdb_heap_formtuple("128bcad", 7);
					FDPG_Heap::fd_simple_heap_update(heapRelation,&heaptuple->t_self,UpdateTuple);
					vString.push_back("128bcad");
					break;
				}
			}
		};
		
		FDPG_Heap::fd_heap_endscan(heapScan);
		
		FDPG_Heap::fd_heap_close(heapRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Transaction::fd_CommandCounterIncrement();
		
		//重建索引
		FDPG_Index::fd_reindex_index(rel_id, index_id, MyDatabaseId);

		const int nkey = 2;
		ScanKeyData Sacnkey[nkey];
		Datum value[nkey];
		std::vector<std::string> vScanStr;
		std::vector<std::string> vEndResult;
		vScanStr.push_back("011");
		vScanStr.push_back("12");
		StrategyNumber ScanStrage[nkey] = {BTGreaterStrategyNumber,BTGreaterEqualStrategyNumber};
		test_index_scan(heapRelation,indexRelation,rel_id,index_id,vScanStr,ScanStrage,nkey,Sacnkey,value,vEndResult);

		FDPG_Index::fd_index_drop(rel_id, relspace,index_id, MyDatabaseId);
		FDPG_Heap::fd_heap_drop(rel_id, MyDatabaseId);
		
		std::sort(vEndResult.begin(),vEndResult.end());
		std::sort(vString.begin(),vString.end());

		if(vEndResult == vString)
			commit_transaction();
		else
		{
			user_abort_transaction();
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	catch(...)
	{
		user_abort_transaction();
		return false;
	}
	return true;
}

void get_toast_data(std::string& strLine,size_t nLen)
{
	std::string s;
	srand((unsigned)time( NULL ));
	size_t nGenerated = 0;
	for (nGenerated = 0;nGenerated < 10;++nGenerated)
	{
		char c = 5;
		s += c;
	}
	for (nGenerated = 10; nGenerated < nLen; ++nGenerated)
	{
		char c = (rand()) % 26;
		s += 'a' + c;
	}
	char c = 0;
	s += c;
	strLine.append(s);
}

int test_reindex_relation()
{
	int rel_id = 80012;
	int index_id0 = 85612;
	int index_id1 = 85613;
	int index_id2 = 85614;
	int index_id3 = 85615;
	int index_id4 = 85616;
	int i = 0;
	try
	{
		std::vector<int> vIndexId;
		std::vector<Relation> vIndexRelation;
		for (i=0; i<5; i++)
			vIndexId.push_back(index_id0+i);
		create_heap_index_table(rel_id,index_id0,5,true);
			
		begin_transaction();
		Oid relspace = MyDatabaseTableSpace;
		FDPG_Heap::fd_heap_create(relspace, rel_id, 0, rel_id);
		Relation heapRelation = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		for (i=0; i<5; i++)
		{
			FDPG_Index::fd_index_create(heapRelation,BTREE_TYPE,vIndexId[i],vIndexId[i]);
			Relation indexRelation = FDPG_Index::fd_index_open(vIndexId[i],RowExclusiveLock, MyDatabaseId);
			vIndexRelation.push_back(indexRelation);
		}
		
		HeapTuple tuple;
		std::vector<std::string> vString;
		vString.push_back(string("12334abcabcdb12345abcd",22));
		vString.push_back(string("23445bcdbcdec12345abcd",22));
		vString.push_back(string("34556cdecdefd12345abcd",22));
		vString.push_back(string("45667edfdefge12345abcd",22));
		vString.push_back(string("56778dfgefghf12345abcd",22));
		for (i=0; i<150; ++i)
			vString.push_back("56a78dfgefghf12345abcd");
		for(i=0;i<155;i++)
		{
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(vString[i].c_str(), vString[i].size())) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		}

		for (i=0; i<5; i++)
			FDPG_Index::fd_index_close(vIndexRelation[i], AccessShareLock);
		FDPG_Heap::fd_heap_close(heapRelation, RowExclusiveLock);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//重建Relation
		bool ReturnVal = FDPG_Index::fd_reindex_relation(rel_id, MyDatabaseId,REINDEX_REL_PROCESS_TOAST);

		if (ReturnVal)
		{
			const int nkey = 1;
			ScanKeyData Sacnkey[5];
			Datum value[5];
			std::vector<std::vector<std::string> >vScanStr;
			std::vector<std::vector<std::string> >vEndResult;
			vScanStr.push_back(std::vector<std::string>(1,string("123",3)));
			vScanStr.push_back(std::vector<std::string>(1,string("24",2)));
			vScanStr.push_back(std::vector<std::string>(1,string("abc",3)));
			vScanStr.push_back(std::vector<std::string>(1,string("abcd",4)));
			vScanStr.push_back(std::vector<std::string>(1,string("b",1)));
			vector<string> temp;
			for (int j=0; j<5; ++j)
			{
				vEndResult.push_back(temp);
			}
			StrategyNumber ScanStrage[5] = {BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber};
			
			for (i=0; i<5; i++)
				test_index_scan(heapRelation,vIndexRelation[i],rel_id,vIndexId[i],vScanStr[i],&ScanStrage[i],nkey,&Sacnkey[i],&value[i],vEndResult[i]);

			for (i=0; i<5; i++)
				FDPG_Index::fd_index_drop(rel_id, relspace,vIndexId[i], MyDatabaseId);
			FDPG_Heap::fd_heap_drop(rel_id, MyDatabaseId);

			for (i=0; i<5; i++)
				std::sort(vEndResult[i].begin(),vEndResult[i].end());
			std::sort(vString.begin(),vString.end());

			int Count = 0;
			for (i=0; i<5; i++)
			{
				if(vEndResult[i] == vString)
					++Count;
			}
			if(5 == Count)
				commit_transaction();
			else
			{
				user_abort_transaction();
				return false;
			}
		}
		else
		{
			FDPG_Heap::fd_heap_close(heapRelation, RowExclusiveLock);
			for (int i=0; i<5; i++)
			{
				FDPG_Index::fd_index_close(vIndexRelation[i], AccessShareLock);
				FDPG_Index::fd_index_drop(rel_id, relspace,vIndexId[i], MyDatabaseId);
			}
			FDPG_Heap::fd_heap_drop(rel_id, MyDatabaseId);
			user_abort_transaction();
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	catch(...)
	{
		user_abort_transaction();
		return false;
	}
	return true;
}

int test_reindex_relation_toast()
{
	int rel_id = 89012;
	int index_id0 = 89612;
	int index_id1 = 89613;
	int index_id2 = 89614;
	int index_id3 = 89615;
	int index_id4 = 89616;
	int i = 0;
	try
	{
		std::vector<int> vIndexId;
		std::vector<Relation> vIndexRelation;
		for (i=0; i<5; i++)
		{
			vIndexId.push_back(index_id0+i);
			create_heap_index_table(rel_id,index_id0+i,2,false);
		}
			
		begin_transaction();
		Oid relspace = MyDatabaseTableSpace;
		FDPG_Heap::fd_heap_create(relspace, rel_id, 0, rel_id);
		Relation heapRelation = FDPG_Heap::fd_heap_open(rel_id,RowExclusiveLock, MyDatabaseId);
		for (i=0; i<5; i++)
		{
			FDPG_Index::fd_index_create(heapRelation,BTREE_TYPE,vIndexId[i],vIndexId[i]);
			Relation indexRelation = FDPG_Index::fd_index_open(vIndexId[i],RowExclusiveLock, MyDatabaseId);
			vIndexRelation.push_back(indexRelation);
		}
		std::vector<std::string> vString;
		HeapTuple tuple;
		string strinsert1("123456a");
		string strinsert2("123789b");
		string strinsert3("012562c");
		string strinsert4("012120d");
		string strinsert5("012456e");
		vString.push_back("123456a");
		vString.push_back("123789b");
		vString.push_back("012562c");
		vString.push_back("012120d");
		vString.push_back("012456e");
		get_toast_data(strinsert1,1<<16);
		get_toast_data(strinsert2,1<<16);
		get_toast_data(strinsert3,1<<16);
		get_toast_data(strinsert4,1<<16);
		get_toast_data(strinsert5,1<<16);
		CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(strinsert1.c_str(), strinsert1.size())) !=NULL);
		CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(strinsert2.c_str(), strinsert2.size())) !=NULL);
		CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(strinsert3.c_str(), strinsert3.size())) !=NULL);
		CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(strinsert4.c_str(), strinsert4.size())) !=NULL);
		CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(strinsert5.c_str(), strinsert5.size())) !=NULL);
		CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);


		for (i=0; i<5; i++)
			FDPG_Index::fd_index_close(vIndexRelation[i], AccessShareLock);
		FDPG_Heap::fd_heap_close(heapRelation, RowExclusiveLock);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//重建Relation
		bool ReturnVal = FDPG_Index::fd_reindex_relation(rel_id, MyDatabaseId,REINDEX_REL_PROCESS_TOAST);

		if (ReturnVal)
		{
			const int nkey = 2;
			ScanKeyData Sacnkey[10];
			Datum value[10];
			std::vector<std::vector<std::string> >vScanStr;
			std::vector<std::vector<std::string> >vEndResult;
			for (int j=0; j<5; ++j)
			{
				std::vector<std::string> vtemp(1,string("011",3));
				vtemp.push_back(string("12",2));
				vScanStr.push_back(vtemp);
				vEndResult.push_back(std::vector<std::string>(1,string("a",1)));
			}
			StrategyNumber ScanStrage[10] = {BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber};
			
			for (i=0; i<5; i++)
				test_index_scan(heapRelation,vIndexRelation[i],rel_id,vIndexId[i],vScanStr[i],&ScanStrage[i],nkey,&Sacnkey[i],&value[i],vEndResult[i]);

			for (i=0; i<5; i++)
				FDPG_Index::fd_index_drop(rel_id, relspace,vIndexId[i], MyDatabaseId);
			FDPG_Heap::fd_heap_drop(rel_id, MyDatabaseId);

			for (i=0; i<5; i++)
				std::sort(vEndResult[i].begin(),vEndResult[i].end());
			std::sort(vString.begin(),vString.end());

			int Count = 0;
			for (i=0; i<5; ++i)
			{
				for (int j=0; j<5; ++j)
				{
					if(0 == vEndResult[i][j].compare(0,7,vString[j]))
						++Count;
				}
			}
			if(25 == Count)
				commit_transaction();
			else
			{
				user_abort_transaction();
				return false;
			}
		}
		else
		{
			FDPG_Heap::fd_heap_close(heapRelation, RowExclusiveLock);
			for (int i=0; i<5; i++)
			{
				FDPG_Index::fd_index_close(vIndexRelation[i], AccessShareLock);
				FDPG_Index::fd_index_drop(rel_id, relspace,vIndexId[i], MyDatabaseId);
			}
			FDPG_Heap::fd_heap_drop(rel_id, MyDatabaseId);
			user_abort_transaction();
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	catch(...)
	{
		user_abort_transaction();
		return false;
	}
	return true;
}

void create_relation(Relation& heap,Relation& index, const Oid& dbid, const Oid& Rel_Id, 
						const Oid& Index_Id, const vector<string>& vString)
{
	try
	{
		Oid relspace = MyDatabaseTableSpace;
		create_heap_index_table(Rel_Id,Index_Id,2,false);
		
		FDPG_Heap::fd_heap_create(relspace, Rel_Id, dbid, Rel_Id);
		heap = FDPG_Heap::fd_heap_open(Rel_Id,RowExclusiveLock, dbid);
		FDPG_Index::fd_index_create(heap,BTREE_TYPE,Index_Id,Index_Id);
		index = FDPG_Index::fd_index_open(Index_Id,AccessShareLock, dbid);
	
		HeapTuple tuple;
		for(int i=0;i<155;i++)
		{
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(vString[i].c_str(), vString[i].size())) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heap, tuple) == 0);
		}
		FDPG_Heap::fd_heap_close(heap, RowExclusiveLock);
		FDPG_Index::fd_index_close(index, AccessShareLock);
		FDPG_Transaction::fd_CommandCounterIncrement();
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
	}
	catch(...)
	{
		user_abort_transaction();
	}
}
bool test_reindex_database()
{
	int rel_id = 80100;
	int index_id = 88800;
	int i = 0;
	try
	{
		vector<Oid> vRelationId;
		vector<Oid> vRelIndexId;
		vector<Relation> vRelheapRel;
		vector<Relation> vRelIndexRel;
		Relation temp;
		for (i=0; i<5; ++i)
		{
			vRelationId.push_back(rel_id+i);
			vRelIndexId.push_back(index_id+i);
			vRelheapRel.push_back(temp);
			vRelIndexRel.push_back(temp);
		}

		vector<std::string> vString;
		vString.push_back("45678ab");
		vString.push_back("56789bc");
		vString.push_back("6789acd");
		vString.push_back("789abde");
		vString.push_back("89abcef");
		for (i=0; i<150; ++i)
			vString.push_back("9abcdfg");
		
		begin_transaction();
		TransactionId txId = GetCurrentTransactionId();
		Oid dbId = InvalidOid;
		THROW_CALL(dbId = CreateDatabase,txId,"test_redinex",DEFAULT_TABLESPACE_NAME);
		CommandCounterIncrement();

		for (i=0; i<5; ++i)
			create_relation(vRelheapRel[i],vRelIndexRel[i],dbId,vRelationId[i],vRelIndexId[i],vString);

		//重建Database中的所有Relation
		FDPG_Index::fd_reindex_database(dbId);

		ScanKeyData Sacnkey[10];
		Datum value[10];
		StrategyNumber ScanStrage[10] = {BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber};
		std::vector<std::vector<std::string> >vScanStr;
		std::vector<std::vector<std::string> >vEndResult;
		for (i=0; i<5; ++i)
		{	
			std::vector<std::string> vtemp(1,string("456",2));
			vtemp.push_back(string("78",2));
			vScanStr.push_back(vtemp);
			vEndResult.push_back(std::vector<std::string>(1,string("a",1)));
		}
		
		for (i=0; i<5; i++)
			test_index_scan(vRelheapRel[i],vRelIndexRel[i],vRelationId[i],vRelIndexId[i],vScanStr[i],&ScanStrage[i*2],2,&Sacnkey[i*2],&value[i*2],vEndResult[i]);

		for (i=0; i<5; i++)
		{
			FDPG_Index::fd_index_drop(vRelationId[i], MyDatabaseTableSpace,vRelIndexId[i], dbId);
			FDPG_Heap::fd_heap_drop(vRelationId[i], dbId);
		}

		for (i=0; i<5; i++)
			std::sort(vEndResult[i].begin(),vEndResult[i].end());
		std::sort(vString.begin(),vString.end());

		int Count = 0;
		for (i=0; i<5; i++)
		{
			if(vEndResult[i] == vString)
				++Count;
		}
		if(5 == Count)
			commit_transaction();
		else
		{
			user_abort_transaction();
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	catch(...)
	{
		user_abort_transaction();
		return false;
	}
	return true;
}

void test_reindex_index_insert(int* relid, int* index_id, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		free(param);

		begin_transaction();
		FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, *relid, 0, *index_id);
		Relation heapRelation = FDPG_Heap::fd_heap_open(*relid,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(heapRelation,BTREE_TYPE,*index_id,*index_id);
		Relation indexRelation = FDPG_Index::fd_index_open(*index_id,AccessShareLock, MyDatabaseId);
		
		HeapTuple tuple;
		std::vector<std::string> vString;
		vString.push_back("123456a");
		vString.push_back("123789b");
		vString.push_back("012562c");
		vString.push_back("012120d");
		vString.push_back("012456e");
		for (int j=0; j<150; ++j)
			vString.push_back("124896e");
		for(int j=0;j<155;j++)
		{
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(vString[j].c_str(), vString[j].size())) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		}
		
		FDPG_Heap::fd_heap_close(heapRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void test_reindex_index_reindex(int* rel_id, int* index_id, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		//重建索引
		FDPG_Index::fd_reindex_index(*rel_id, *index_id, MyDatabaseId);
		
		commit_transaction();
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void test_reindex_index_insert2(int* relid, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);

		begin_transaction();
		Relation heapRelation = FDPG_Heap::fd_heap_open(*relid,RowExclusiveLock, MyDatabaseId);
		HeapTuple tuple;
		std::vector<std::string> vString;
		vString.push_back("01378ab");
		CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(vString[0].c_str(), vString[0].size())) !=NULL);
		CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		
		FDPG_Heap::fd_heap_close(heapRelation, AccessShareLock);
		commit_transaction();
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void test_reindex_index_scan(int* rel_id, int* index_id, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);

		begin_transaction();
		Relation heapRelation;
		Relation indexRelation;
		
		const int nkey = 2;
		ScanKeyData Sacnkey[nkey];
		Datum value[nkey];
		std::vector<std::string> vScanStr;
		std::vector<std::string> vEndResult;
		vScanStr.push_back("011");
		vScanStr.push_back("12");
		StrategyNumber ScanStrage[nkey] = {BTGreaterStrategyNumber,BTGreaterEqualStrategyNumber};
		test_index_scan(heapRelation,indexRelation,*rel_id,*index_id,vScanStr,ScanStrage,nkey,Sacnkey,value,vEndResult);

		FDPG_Index::fd_index_drop(*rel_id, MyDatabaseTableSpace,*index_id, MyDatabaseId);
		FDPG_Heap::fd_heap_drop(*rel_id, MyDatabaseId);
		
		std::sort(vEndResult.begin(),vEndResult.end());
		
		std::vector<std::string> vString;
		vString.push_back("123456a");
		vString.push_back("123789b");
		vString.push_back("012562c");
		vString.push_back("012120d");
		vString.push_back("012456e");
		vString.push_back("01378ab");
		for (int j=0; j<150; ++j)
			vString.push_back("124896e");
		std::sort(vString.begin(),vString.end());
		
		if(vEndResult == vString)
		{
			commit_transaction();
			*i = 1;
		}
		else
		{
			user_abort_transaction();
			*i = 0;
		}
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

bool test_reindex_index_mult_thread()
{
	int sta[4] = {0};
	
	int rel_id = 80150;
	int index_id = 88500;
	Oid relspace = MyDatabaseTableSpace;
	create_heap_index_table(rel_id,index_id,2,false);
	
	boost::thread_group tg;
	tg.create_thread(boost::bind(&test_reindex_index_insert,&rel_id,&index_id,&sta[0]));
	tg.join_all();

	tg.create_thread(boost::bind(&test_reindex_index_reindex,&rel_id, &index_id, &sta[1]));
	tg.create_thread(boost::bind(&test_reindex_index_insert2,&rel_id, &sta[2]));
	tg.join_all();
	
	tg.create_thread(boost::bind(&test_reindex_index_scan,&rel_id,&index_id,&sta[3]));
	tg.join_all();

	int found = 0;
	for (int i=0; i<4; ++i)
	{
		if (1 == sta[i])
			++found;
	}

	if (4 == found)
		return true;
	else
		return false;
}

void test_reindex_relation_insert(int* relid, int* index_id, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);

		std::vector<int> vIndexId;
		std::vector<Relation> vIndexRelation;
		for (int j=0; j<5; j++)
			vIndexId.push_back(*index_id+j);
		
		begin_transaction();

		Oid relspace = MyDatabaseTableSpace;
		FDPG_Heap::fd_heap_create(relspace, *relid, 0, *relid);
		Relation heapRelation = FDPG_Heap::fd_heap_open(*relid,RowShareLock, MyDatabaseId);
		for (int j=0; j<5; j++)
		{
			FDPG_Index::fd_index_create(heapRelation,BTREE_TYPE,vIndexId[j],vIndexId[j]);
			Relation indexRelation = FDPG_Index::fd_index_open(vIndexId[j],RowShareLock, MyDatabaseId);
			vIndexRelation.push_back(indexRelation);
		}
		
		HeapTuple tuple;
		std::vector<std::string> vString;
		vString.push_back(string("12334abcabcdb12345abcd",22));
		vString.push_back(string("23445bcdbcdec12345abcd",22));
		vString.push_back(string("34556cdecdefd12345abcd",22));
		vString.push_back(string("45667edfdefge12345abcd",22));
		vString.push_back(string("56778dfgefghf12345abcd",22));
		for (int j=0; j<150; ++j)
			vString.push_back("56a78dfgefghf12345abcd");
		for(int j=0;j<155;j++)
		{
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(vString[j].c_str(), vString[j].size())) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		}

		for (int j=0; j<5; j++)
			FDPG_Index::fd_index_close(vIndexRelation[j], AccessShareLock);
		FDPG_Heap::fd_heap_close(heapRelation, RowExclusiveLock);

		commit_transaction();
		free(param);
		*i = 1;
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void test_reindex_relation_reindex(int* rel_id, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		//重建Relation
		FDPG_Index::fd_reindex_relation(*rel_id, MyDatabaseId,REINDEX_REL_PROCESS_TOAST);
		
		commit_transaction();
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void test_reindex_relation_insert2(int* relid, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);

		begin_transaction();

		Oid relspace = MyDatabaseTableSpace;
		Relation heapRelation = FDPG_Heap::fd_heap_open(*relid,RowShareLock, MyDatabaseId);
		
		HeapTuple tuple;
		std::vector<std::string> vString;
		vString.push_back(string("67889fghfghjh12356ebcd",22));
		vString.push_back(string("7899aghjghjkj14589fdec",22));
		
		for(int j=0;j<2;j++)
		{
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(vString[j].c_str(), vString[j].size())) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(heapRelation, tuple) == 0);
		}
		
		FDPG_Heap::fd_heap_close(heapRelation, RowShareLock);
		
		commit_transaction();
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void test_reindex_relation_scan(int* rel_id, int* index_id, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);

		std::vector<int> vIndexId;
		std::vector<Relation> vIndexRelation;
		Relation tempRel;
		for (int j=0; j<5; ++j)
		{
			vIndexId.push_back(*index_id+j);
			vIndexRelation.push_back(tempRel);
		}
		
		begin_transaction();
		
		const int nkey = 1;
		ScanKeyData Sacnkey[5];
		Datum value[5];
		vector<vector<string> >vScanStr;
		vector<vector<string> >vEndResult;
		vScanStr.push_back(vector<string>(1,string("123",3)));
		vScanStr.push_back(vector<string>(1,string("24",2)));
		vScanStr.push_back(vector<string>(1,string("abc",3)));
		vScanStr.push_back(vector<string>(1,string("abcd",4)));
		vScanStr.push_back(vector<string>(1,string("b",1)));

		vector<string> temp;
		for (int j=0; j<5; ++j)
			vEndResult.push_back(temp);
	
		StrategyNumber ScanStrage[5] = {BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
			BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber};

		Relation heapRelation;
		for (int j=0; j<5; ++j)
			test_index_scan(heapRelation,vIndexRelation[j],*rel_id,vIndexId[j],vScanStr[j],&ScanStrage[j],nkey,&Sacnkey[j],&value[j],vEndResult[j]);

		for (int j=0; j<5; j++)
			FDPG_Index::fd_index_drop(*rel_id, MyDatabaseTableSpace,vIndexId[j], MyDatabaseId);
		FDPG_Heap::fd_heap_drop(*rel_id, MyDatabaseId);

		for (int j=0; j<5; j++)
			std::sort(vEndResult[j].begin(),vEndResult[j].end());

		std::vector<std::string> vString;
		vString.push_back(string("12334abcabcdb12345abcd",22));
		vString.push_back(string("23445bcdbcdec12345abcd",22));
		vString.push_back(string("34556cdecdefd12345abcd",22));
		vString.push_back(string("45667edfdefge12345abcd",22));
		vString.push_back(string("56778dfgefghf12345abcd",22));
		vString.push_back(string("67889fghfghjh12356ebcd",22));
		vString.push_back(string("7899aghjghjkj14589fdec",22));
		for (int j=0; j<150; ++j)
			vString.push_back("56a78dfgefghf12345abcd");
		std::sort(vString.begin(),vString.end());

		int Count = 0;
		for (int j=0; j<5; ++j)
		{
			if(vEndResult[j] == vString)
				++Count;
		}
		if(5 == Count)
			commit_transaction();
		else
		{
			*i = 0;
			user_abort_transaction();
		}
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

bool test_reindex_relation_mult_thread()
{
	int sta[4] = {0};
	int rel_id = 80200;
	int index_id0 = 88600;
	int index_id1 = 88601;
	int index_id2 = 88602;
	int index_id3 = 88603;
	int index_id4 = 88604;
	Oid relspace = MyDatabaseTableSpace;
	
	std::vector<int> vIndexId;
	for (int i=0; i<5; i++)
		vIndexId.push_back(index_id0+i);
	
	create_heap_index_table(rel_id,index_id0,5,true);
	
	boost::thread_group tg;
	tg.create_thread(boost::bind(&test_reindex_relation_insert,&rel_id,&index_id0,&sta[0]));
	tg.join_all();

	tg.create_thread(boost::bind(&test_reindex_relation_reindex,&rel_id,&sta[1]));
	tg.create_thread(boost::bind(&test_reindex_relation_insert2,&rel_id,&sta[2]));
	tg.join_all();
	
	tg.create_thread(boost::bind(&test_reindex_relation_scan,&rel_id,&index_id0,&sta[3]));
	tg.join_all();

	int found = 0;
	for (int i=0; i<4; ++i)
	{
		if (1 == sta[i])
			++found;
	}

	if (4 == found)
		return true;
	else
		return false;
	return true;
}

void test_reindex_database_insert(int* relid, int* index_id, int* dbid, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		

		begin_transaction();
		vector<Oid> vRelationId;
		vector<Oid> vRelIndexId;
		vector<Relation> vRelheapRel;
		vector<Relation> vRelIndexRel;
		Relation temp;
		int j = 0;
		for (j=0; j<5; ++j)
		{
			vRelationId.push_back(*relid+j);
			vRelIndexId.push_back(*index_id+j);
			vRelheapRel.push_back(temp);
			vRelIndexRel.push_back(temp);
		}

		vector<std::string> vString;
		vString.push_back("45678ab");
		vString.push_back("56789bc");
		vString.push_back("6789acd");
		vString.push_back("789abde");
		vString.push_back("89abcef");
		for (int j=0; j<150; ++j)
			vString.push_back("9abcdfg");
		TransactionId txId = GetCurrentTransactionId();
		THROW_CALL(*dbid = CreateDatabase,txId,"test_redinex_mult",DEFAULT_TABLESPACE_NAME);
		CommandCounterIncrement();

		for (j=0; j<5; ++j)
			create_relation(vRelheapRel[j],vRelIndexRel[j],*dbid,vRelationId[j],vRelIndexId[j],vString);

		commit_transaction();
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void test_reindex_database_reindex(int* dbid, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		//重建Relation
		FDPG_Index::fd_reindex_database(*dbid);
		
		commit_transaction();
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void test_reindex_database_insert2(int* relid, int* index_id, int* dbid, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);

		begin_transaction();

		vector<std::string> vString;
		vString.push_back("abcdefg");
		vString.push_back("bcdefgh");
		vString.push_back("cdefghj");
		vString.push_back("defghjk");
		vString.push_back("efghjkl");
		for (int j=0; j<150; ++j)
			vString.push_back("9abcdfg");
		Relation heapRel;
		Relation IndexRel;
		create_relation(heapRel, IndexRel, *dbid, *relid, *index_id,vString);
		
		commit_transaction();
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void test_reindex_database_scan(int* rel_id, int* index_id, int* dbid, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);

		vector<Oid> vRelationId;
		vector<Oid> vRelIndexId;
		vector<Relation> vRelheapRel;
		vector<Relation> vRelIndexRel;
		Relation temp;
		int j = 0;
		for (j=0; j<6; ++j)
		{
			vRelationId.push_back(*rel_id+j);
			vRelIndexId.push_back(*index_id+j);
			vRelheapRel.push_back(temp);
			vRelIndexRel.push_back(temp);
		}
		
		begin_transaction();
		
		ScanKeyData Sacnkey[12];
		Datum value[12];
		StrategyNumber ScanStrage[12] = {BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,BTGreaterEqualStrategyNumber,
				BTGreaterEqualStrategyNumber};
		std::vector<std::vector<std::string> >vScanStr;
		std::vector<std::vector<std::string> >vEndResult;
		for (j=0; j<6; ++j)
		{	
			std::vector<std::string> vtemp(1,string("456",2));
			vtemp.push_back(string("78",2));
			vScanStr.push_back(vtemp);
			vEndResult.push_back(std::vector<std::string>(1,string("a",1)));
		}
		
		for (j=0; j<6; j++)
			test_index_scan(vRelheapRel[j],vRelIndexRel[j],vRelationId[j],vRelIndexId[j],vScanStr[j],&ScanStrage[j*2],2,&Sacnkey[j*2],&value[j*2],vEndResult[j]);

		for (j=0; j<6; j++)
		{
			FDPG_Index::fd_index_drop(vRelationId[j], MyDatabaseTableSpace,vRelIndexId[j], *dbid);
			FDPG_Heap::fd_heap_drop(vRelationId[j], *dbid);
		}

		vector<std::string> vString;
		vString.push_back("45678ab");
		vString.push_back("56789bc");
		vString.push_back("6789acd");
		vString.push_back("789abde");
		vString.push_back("89abcef");
		vString.push_back("abcdefg");
		vString.push_back("bcdefgh");
		vString.push_back("cdefghj");
		vString.push_back("defghjk");
		vString.push_back("efghjkl");
		for (int j=0; j<150; ++j)
			vString.push_back("9abcdfg");
		for (j=0; j<6; j++)
			std::sort(vEndResult[j].begin(),vEndResult[j].end());
		std::sort(vString.begin(),vString.end());

		int Count = 0;
		for (j=0; j<6; ++j)
		{
			if(vEndResult[j] == vString)
				++Count;
		}
		if(6 == Count)
			commit_transaction();
		else
		{
			user_abort_transaction();
			*i = 0;
		}
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	catch(...)
	{
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

bool test_reindex_database_mult_thread()
{
	int sta[4];
	int i = 0;
	int rel_id0 = 80300;
	int rel_id5 = 80305;
	int index_id0 = 88700;
	int index_id1 = 88701;
	int index_id2 = 88702;
	int index_id3 = 88703;
	int index_id4 = 88704;
	int index_id5 = 88705;
	int dbid = 0;
	Oid relspace = MyDatabaseTableSpace;
	
	boost::thread_group tg;
	tg.create_thread(boost::bind(&test_reindex_database_insert,&rel_id0,&index_id0,&dbid, &sta[0]));
	tg.join_all();

	tg.create_thread(boost::bind(&test_reindex_database_reindex,&dbid,&sta[1]));
	tg.create_thread(boost::bind(&test_reindex_database_insert2,&rel_id5,&index_id5,&dbid,&sta[2]));
	tg.join_all();
	
	tg.create_thread(boost::bind(&test_reindex_database_scan,&rel_id0,&index_id0,&dbid,&sta[3]));
	tg.join_all();
	int found = 0;
	for(i=0; i<4; ++i)
	{
		if (1 == sta[i])
			++found;
	}
	
	if (4 == found)
		return true;
	else
		return false;
}

/*hash index*/
void HashIndex_Heap(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 3;
	}
	if (col == 4)
	{
		rangeData.start = 8;
		rangeData.len = 4;
	}	
}

void HashIndex_Index(RangeData& rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 4;
	}	
}

void CreateHashIndexTable(const Oid& HeapId, const Oid& IndexId)
{
	Colinfo heap_info = (Colinfo)malloc(sizeof(ColinfoData));
	heap_info->col_number = NULL;
	heap_info->keys = 0;
	heap_info->rd_comfunction = NULL;
	heap_info->split_function =  HashIndex_Heap;
	heap_info->tuple_size = 12;
	setColInfo(HeapId,heap_info);

	Colinfo index_info = (Colinfo)malloc(sizeof(ColinfoData));
	index_info->keys = 2;
	index_info->col_number = (size_t*)malloc(sizeof(size_t)*index_info->keys);
	index_info->col_number[0] = 1;
	index_info->col_number[1] = 4;
	index_info->rd_comfunction = (CompareCallback*)malloc(sizeof(CompareCallback)*index_info->keys);
	index_info->rd_comfunction[0] = my_compare_str;
	index_info->rd_comfunction[1] = my_compare_str;
	index_info->tuple_size = 7;
	index_info->split_function = HashIndex_Index;
	setColInfo(IndexId,index_info);
}

void Hash_Index_Scan(Relation& heap,Relation& index,const Oid& heapid,const Oid& indexid,const std::vector<std::string>& Scanstr,
					 StrategyNumber* ScanStrage,const int nCol,ScanKeyData* Scankey,Datum* value,std::vector<std::string>& Resultstr)
{
	Resultstr.clear();
	heap = FDPG_Heap::fd_heap_open(heapid,RowShareLock, MyDatabaseId);
	index = FDPG_Index::fd_index_open(indexid,RowShareLock, MyDatabaseId);

	IndexScanDesc index_scan;

	for(int i=0; i<nCol; i++)
	{
		value[i] = fdxdb_string_formdatum(Scanstr[i].c_str(), Scanstr[i].size());
		Fdxdb_ScanKeyInitWithCallbackInfo(&Scankey[i],(AttrNumber)(i+1),ScanStrage[i],my_compare_str,value[i]);	
	}
	
	index_scan = FDPG_Index::fd_index_beginscan(heap, index, SnapshotNow, nCol, 0);
	FDPG_Index::fd_index_rescan(index_scan, Scankey, nCol, NULL, 0);

	char *temp = NULL;
	int Count = 0;
	HeapTuple tuple;
	while((tuple = FDPG_Index::fd_index_getnext(index_scan, BackwardScanDirection)) != NULL)
	{
		int len = 0;
		temp = fdxdb_tuple_to_chars_with_len(tuple,len);
		Resultstr.push_back(string(temp,len));
		pfree(temp);
	}

	FDPG_Index::fd_index_endscan(index_scan);
	FDPG_Heap::fd_heap_close(heap, RowExclusiveLock);
	FDPG_Index::fd_index_close(index, AccessShareLock);
}

bool HashIndexTest()
{
	Oid HeapId = 90210;
	Oid IndexId = 90310;
	try
	{
		Oid relspace = MyDatabaseTableSpace;
		CreateHashIndexTable(HeapId,IndexId);
		begin_transaction();
		
		FDPG_Heap::fd_heap_create(relspace, HeapId, MyDatabaseId, HeapId);
		Relation HeapRel = FDPG_Heap::fd_heap_open(HeapId,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(HeapRel,HASH_TYPE,IndexId,IndexId);
		Relation IndexRel = FDPG_Index::fd_index_open(IndexId,RowExclusiveLock, MyDatabaseId);

		vector<string> vString;
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01345678bcde");
		vString.push_back("01456789cdef");
		vString.push_back("0156789adefg");
		vString.push_back("016789abefgh");
		HeapTuple tuple;
		int i =0;
		for(; i<8; ++i)
		{
			tuple = FDPG_Heap::fd_heap_form_tuple(vString[i].c_str(),vString[i].length());
			FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();
		
		/*heap delete*/
		HeapScanDesc heapScan = FDPG_Heap::fd_heap_beginscan(HeapRel,SnapshotNow, 0, NULL);
		while((tuple = FDPG_Heap::fd_heap_getnext(heapScan,ForwardScanDirection)) != NULL)
		{
			char* temp = fxdb_tuple_to_chars(tuple);
			if (0 == vString[5].compare(temp))
				FDPG_Heap::fd_simple_heap_delete(HeapRel,&tuple->t_self);
		}
		FDPG_Heap::fd_heap_endscan(heapScan);
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);
		FDPG_Transaction::fd_CommandCounterIncrement();

		/*index*/
		ScanKeyData nScankey[2];
		Datum value[2];
		StrategyNumber ScanStrage[10] = {HTEqualStrategyNumber,HTEqualStrategyNumber};
		vector<string> vScanStr;
		vector<string> vEndResult;	
		vScanStr.push_back(string("016efgh",7));
		vScanStr.push_back(string("016efgh",7));
		Hash_Index_Scan(HeapRel,IndexRel,HeapId,IndexId,vScanStr,ScanStrage,2,nScankey,value,vEndResult);

		bool found = false;
		for(i=0; i<vEndResult.size(); ++i)
		{
			if (0 == vEndResult[i].compare("016789abefgh"))
			{
				found = true;
				break;
			}
		}

		if (!found)
			return false;

		vScanStr.clear();
		vEndResult.clear();
		vScanStr.push_back(string("012abcd",7));
		vScanStr.push_back(string("012abcd",7));
		Hash_Index_Scan(HeapRel,IndexRel,HeapId,IndexId,vScanStr,ScanStrage,2,nScankey,value,vEndResult);

		int count = 0;
		for(i=0; i<vEndResult.size(); ++i)
		{
			if (0 == vEndResult[i].compare("01234567abcd"))
				++count;
		}

		if (4 != count)
			return false;
		
		FDPG_Index::fd_index_drop(HeapId, MyDatabaseTableSpace,IndexId, MyDatabaseId);
		FDPG_Heap::fd_heap_drop(HeapId, MyDatabaseId);
		commit_transaction();
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	return true;
}

bool ReIndexHashIndexTest()
{
	Oid HeapId = 90212;
	Oid IndexId = 90312;
	try
	{
		Oid relspace = MyDatabaseTableSpace;
		CreateHashIndexTable(HeapId,IndexId);
		begin_transaction();
		
		FDPG_Heap::fd_heap_create(relspace, HeapId, MyDatabaseId, HeapId);
		Relation HeapRel = FDPG_Heap::fd_heap_open(HeapId,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(HeapRel,HASH_TYPE,IndexId,IndexId);
		Relation IndexRel = FDPG_Index::fd_index_open(IndexId,RowExclusiveLock, MyDatabaseId);

		vector<string> vString;
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01345678bcde");
		vString.push_back("01456789cdef");
		vString.push_back("0156789adefg");
		vString.push_back("016789abefgh");
		HeapTuple tuple;
		int i =0;
		for(; i<8; ++i)
		{
			tuple = FDPG_Heap::fd_heap_form_tuple(vString[i].c_str(),vString[i].length());
			FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();
		/*heap delete*/
		HeapScanDesc heapScan = FDPG_Heap::fd_heap_beginscan(HeapRel,SnapshotNow, 0, NULL);
		while((tuple = FDPG_Heap::fd_heap_getnext(heapScan,ForwardScanDirection)) != NULL)
		{
			char* temp = fxdb_tuple_to_chars(tuple);
			if (0 == vString[5].compare(temp))
				FDPG_Heap::fd_simple_heap_delete(HeapRel,&tuple->t_self);
		}
		FDPG_Heap::fd_heap_endscan(heapScan);
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);
		FDPG_Transaction::fd_CommandCounterIncrement();

		/*reindex */
		FDPG_Index::fd_reindex_index(HeapId, IndexId, MyDatabaseId);
		
		/*index*/
		ScanKeyData nScankey[2];
		Datum value[2];
		StrategyNumber ScanStrage[10] = {HTEqualStrategyNumber,HTEqualStrategyNumber};
		vector<string> vScanStr;
		vector<string> vEndResult;	
		
		vScanStr.push_back(string("012abcd",7));
		vScanStr.push_back(string("012abcd",7));
		Hash_Index_Scan(HeapRel,IndexRel,HeapId,IndexId,vScanStr,ScanStrage,2,nScankey,value,vEndResult);

		int count = 0;
		for(i=0; i<vEndResult.size(); ++i)
		{
			if (0 == vEndResult[i].compare("01234567abcd"))
				++count;
		}

		if (4 != count)
			return false;

		FDPG_Index::fd_index_drop(HeapId, MyDatabaseTableSpace,IndexId, MyDatabaseId);
		FDPG_Heap::fd_heap_drop(HeapId, MyDatabaseId);
		commit_transaction();
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	return true;
}

bool HashIndexToastTest()
{
	Oid HeapId = 90215;
	Oid IndexId = 90315;
	try
	{
		Oid relspace = MyDatabaseTableSpace;
		CreateHashIndexTable(HeapId,IndexId);
		begin_transaction();
		
		FDPG_Heap::fd_heap_create(relspace, HeapId, MyDatabaseId, HeapId);
		Relation HeapRel = FDPG_Heap::fd_heap_open(HeapId,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(HeapRel,HASH_TYPE,IndexId,IndexId);
		Relation IndexRel = FDPG_Index::fd_index_open(IndexId,RowExclusiveLock, MyDatabaseId);

		string string1("01234567abcd");
		string string2("01345678bcde");
		string string3("01456789cdef");
		string string4("0156789adefg");
		string string5("016789abefgh");
		get_toast_data(string1,1<<16);
		get_toast_data(string2,1<<16);
		get_toast_data(string3,1<<16);
		get_toast_data(string4,1<<16);
		get_toast_data(string5,1<<16);
		HeapTuple tuple;
		tuple = FDPG_Heap::fd_heap_form_tuple(string1.c_str(),string1.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		tuple = FDPG_Heap::fd_heap_form_tuple(string2.c_str(),string1.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		tuple = FDPG_Heap::fd_heap_form_tuple(string3.c_str(),string1.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		tuple = FDPG_Heap::fd_heap_form_tuple(string4.c_str(),string1.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		tuple = FDPG_Heap::fd_heap_form_tuple(string5.c_str(),string1.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);
		FDPG_Transaction::fd_CommandCounterIncrement();
		
		/*index*/
		ScanKeyData nScankey[2];
		Datum value[2];
		StrategyNumber ScanStrage[10] = {HTEqualStrategyNumber,HTEqualStrategyNumber};
		vector<string> vScanStr;
		vector<string> vEndResult;	
		
		vScanStr.push_back(string("016efgh",7));
		vScanStr.push_back(string("016efgh",7));
		Hash_Index_Scan(HeapRel,IndexRel,HeapId,IndexId,vScanStr,ScanStrage,2,nScankey,value,vEndResult);

		bool bfound = false;
		int i = 0;
		for(;i<vEndResult.size(); ++i)
		{
			if (0 == vEndResult[i].compare(string5))
				bfound = true;
		}

		if (!bfound)
			return false;

		FDPG_Index::fd_index_drop(HeapId, MyDatabaseTableSpace,IndexId, MyDatabaseId);
		FDPG_Heap::fd_heap_drop(HeapId, MyDatabaseId);
		commit_transaction();
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	return true;
}

bool HashIndexBitmapTest()
{
	Oid HeapId = 90220;
	Oid IndexId = 90320;
	try
	{
		Oid relspace = MyDatabaseTableSpace;
		CreateHashIndexTable(HeapId,IndexId);
		begin_transaction();
		
		FDPG_Heap::fd_heap_create(relspace, HeapId, MyDatabaseId, HeapId);
		Relation HeapRel = FDPG_Heap::fd_heap_open(HeapId,RowExclusiveLock, MyDatabaseId);
		int i=0; 
		HeapTuple tuple;
		string str("01234567abcd");
		for (; i<2000; ++ i)
		{
			tuple = FDPG_Heap::fd_heap_form_tuple(str.c_str(),str.size());
			FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		}

		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Transaction::fd_CommandCounterIncrement();

		FDPG_Index::fd_index_create(HeapRel,HASH_TYPE,IndexId,IndexId);
		Relation IndexRel = FDPG_Index::fd_index_open(IndexId,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_close(IndexRel, AccessShareLock);
		

		/*index*/
		ScanKeyData nScankey[2];
		Datum value[2];
		StrategyNumber ScanStrage[10] = {HTEqualStrategyNumber,HTEqualStrategyNumber};
		vector<string> vScanStr;
		vector<string> vEndResult;	
		
		vScanStr.push_back(string("012abcd",7));
		vScanStr.push_back(string("012abcd",7));
		
		HeapRel = FDPG_Heap::fd_heap_open(HeapId,RowShareLock, MyDatabaseId);
		IndexRel = FDPG_Index::fd_index_open(IndexId,RowShareLock, MyDatabaseId);

		IndexScanDesc index_scan;
		for(int i=0; i<2; i++)
		{
			value[i] = fdxdb_string_formdatum(vScanStr[i].c_str(), vScanStr[i].size());
			Fdxdb_ScanKeyInitWithCallbackInfo(&nScankey[i],(AttrNumber)(i+1),ScanStrage[i],my_compare_str,value[i]);	
		}
	
		index_scan = FDPG_Index::fd_index_beginscan(HeapRel, IndexRel, FDPG_Transaction::fd_GetTransactionSnapshot(), 2, 0);
		FDPG_Index::fd_index_rescan(index_scan, nScankey, 2, NULL, 0);

		TIDBitmap* pBitmap = NULL;
		int64 num;
		THROW_CALL(pBitmap = tbm_create,work_mem * 1024L);
		if (NULL != index_scan)
		{
			THROW_CALL(num = index_getbitmap,index_scan, pBitmap);
		}
		BitmapHeapScanState* bmpScanState = NULL;
		THROW_CALL(bmpScanState = ExecInitBitmapHeapScan,index_scan->heapRelation,pBitmap,index_scan->xs_snapshot,index_scan->numberOfKeys,index_scan->keyData);

		char* Temp = NULL;
		vEndResult.clear();
		while((tuple = BitmapHeapNext(bmpScanState)) != NULL)
		{
			int len = 0;
			Temp = fdxdb_tuple_to_chars_with_len(tuple,len);
			vEndResult.push_back(string(Temp,len));
			pfree(Temp);
		}
				
		THROW_CALL(ExecEndBitmapHeapScan,bmpScanState);
		THROW_CALL(tbm_free,pBitmap);

		if (2000 != vEndResult.size())
		{
			user_abort_transaction();
			return false;
		}
		
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(HeapRel, RowShareLock);
		FDPG_Index::fd_index_close(IndexRel, RowShareLock);

		FDPG_Index::fd_index_drop(HeapId, MyDatabaseTableSpace,IndexId, MyDatabaseId);
		FDPG_Heap::fd_heap_drop(HeapId, MyDatabaseId);
		commit_transaction();
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		return false;
	}
	return true;
}

void HashIndexMultInsert(int* HeapId, int* IndexId, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		
		FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, *HeapId, MyDatabaseId, *HeapId);
		Relation HeapRel = FDPG_Heap::fd_heap_open(*HeapId,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(HeapRel,HASH_TYPE,*IndexId,*IndexId);
		Relation IndexRel = FDPG_Index::fd_index_open(*IndexId,RowExclusiveLock, MyDatabaseId);

		vector<string> vString;
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01345678bcde");
		vString.push_back("01456789cdef");
		vString.push_back("0156789adefg");
		vString.push_back("016789abefgh");
		HeapTuple tuple;
		int j =0;
		for(; j<8; ++j)
		{
			tuple = FDPG_Heap::fd_heap_form_tuple(vString[j].c_str(),vString[j].length());
			FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		}
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);
		
		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0 ;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void HashIndexMultUpdate(int* HeapId, int* IndexId, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		
		Relation HeapRel = FDPG_Heap::fd_heap_open(*HeapId,RowShareLock, MyDatabaseId);
		Relation IndexRel = FDPG_Index::fd_index_open(*IndexId,RowShareLock, MyDatabaseId);
		
		/*heap update*/
		HeapTuple tuple;
		HeapScanDesc heapScan = FDPG_Heap::fd_heap_beginscan(HeapRel,SnapshotNow, 0, NULL);
		while((tuple = FDPG_Heap::fd_heap_getnext(heapScan,ForwardScanDirection)) != NULL)
		{
			char* temp = fxdb_tuple_to_chars(tuple);
			if (0 == memcmp(temp,"016789abefgh",12))
			{
				HeapTuple UpdateTuple = FDPG_Heap::fd_heap_form_tuple("018abcdeghjk",12);
				FDPG_Heap::fd_simple_heap_update(HeapRel,&tuple->t_self,UpdateTuple);
			}
		}
		FDPG_Heap::fd_heap_endscan(heapScan);
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);

		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0 ;
	}
	FDPG_Heap::fd_proc_exit(0);
}
void HashIndexMultScan(int* HeapId, int* IndexId,int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		/*index*/
		ScanKeyData nScankey[2];
		Datum value[2];
		StrategyNumber ScanStrage[10] = {HTEqualStrategyNumber,HTEqualStrategyNumber};
		vector<string> vScanStr;
		vector<string> vEndResult;	
		vScanStr.push_back(string("015defg",7));
		vScanStr.push_back(string("015defg",7));
		Relation HeapRel,IndexRel;
		Hash_Index_Scan(HeapRel,IndexRel,*HeapId,*IndexId,vScanStr,ScanStrage,2,nScankey,value,vEndResult);

		bool found = false;
		int j = 0;
		for(; j<vEndResult.size(); ++j)
		{
			if (0 == vEndResult[j].compare("0156789adefg"))
			{
				found = true;
				break;
			}
		}
		
		if (!found)
		{
			*i = 0;
			return;
		}
		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}
bool HashIndexMultTest()
{
	int sta[3] = {0};
	
	int Heap_Id = 90217;
	int Index_Id = 90317;
	Oid relspace = MyDatabaseTableSpace;
	CreateHashIndexTable(Heap_Id,Index_Id);

	boost::thread_group tg;
	tg.create_thread(boost::bind(&HashIndexMultInsert,&Heap_Id,&Index_Id,&sta[0]));
	tg.join_all();

	tg.create_thread(boost::bind(&HashIndexMultUpdate,&Heap_Id, &Index_Id, &sta[1]));
	tg.create_thread(boost::bind(&HashIndexMultScan,&Heap_Id, &Index_Id,&sta[2]));
	tg.join_all();

	int found = 0;
	for (int i=0; i<3; ++i)
	{
		if (1 == sta[i])
			++found;
	}

	if (3 == found)
		return true;
	else
		return false;
}

void ReIndexHashIndexMultInsert(int* HeapId, int* IndexId, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		
		FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, *HeapId, MyDatabaseId, *HeapId);
		Relation HeapRel = FDPG_Heap::fd_heap_open(*HeapId,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(HeapRel,HASH_TYPE,*IndexId,*IndexId);
		Relation IndexRel = FDPG_Index::fd_index_open(*IndexId,RowExclusiveLock, MyDatabaseId);

		vector<string> vString;
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01234567abcd");
		vString.push_back("01345678bcde");
		vString.push_back("01456789cdef");
		vString.push_back("0156789adefg");
		vString.push_back("016789abefgh");
		HeapTuple tuple;
		int j =0;
		for(; j<8; ++j)
		{
			tuple = FDPG_Heap::fd_heap_form_tuple(vString[j].c_str(),vString[j].length());
			FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		}
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);
		
		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0 ;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void ReIndexHashIndexMultUpdate(int* HeapId, int* IndexId, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		
		Relation HeapRel = FDPG_Heap::fd_heap_open(*HeapId,RowShareLock, MyDatabaseId);
		Relation IndexRel = FDPG_Index::fd_index_open(*IndexId,RowShareLock, MyDatabaseId);
		
		/*heap update*/
		HeapTuple tuple;
		HeapScanDesc heapScan = FDPG_Heap::fd_heap_beginscan(HeapRel,SnapshotNow, 0, NULL);
		while((tuple = FDPG_Heap::fd_heap_getnext(heapScan,ForwardScanDirection)) != NULL)
		{
			char* temp = fxdb_tuple_to_chars(tuple);
			if (0 == memcmp(temp,"016789abefgh",12))
			{
				HeapTuple UpdateTuple = FDPG_Heap::fd_heap_form_tuple("018abcdeghjk",12);
				FDPG_Heap::fd_simple_heap_update(HeapRel,&tuple->t_self,UpdateTuple);
			}
		}
		FDPG_Heap::fd_heap_endscan(heapScan);
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);

		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0 ;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void ReIndexHashIndexMultReIndex(int* HeapId, int* IndexId, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		//重建索引
		FDPG_Index::fd_reindex_index(*HeapId, *IndexId, MyDatabaseId);
		
		commit_transaction();
		*i = 1;
		free(param);
	}
	catch(StorageEngineException &ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void ReIndexHashIndexMultScan(int* HeapId, int* IndexId,int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		/*index*/
		ScanKeyData nScankey[2];
		Datum value[2];
		StrategyNumber ScanStrage[10] = {HTEqualStrategyNumber,HTEqualStrategyNumber};
		vector<string> vScanStr;
		vector<string> vEndResult;	
		vScanStr.push_back(string("018ghjk",7));
		vScanStr.push_back(string("018ghjk",7));
		Relation HeapRel,IndexRel;
		Hash_Index_Scan(HeapRel,IndexRel,*HeapId,*IndexId,vScanStr,ScanStrage,2,nScankey,value,vEndResult);

		bool found = false;
		int j = 0;
		for(; j<vEndResult.size(); ++j)
		{
			if (0 == vEndResult[j].compare("018abcdeghjk"))
			{
				found = true;
				break;
			}
		}
		
		if (!found)
		{
			*i = 0;
			return;
		}
		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

bool ReInexHashIndexMultTest()
{
	int sta[4] = {0};
	
	int Heap_Id = 90218;
	int Index_Id = 90318;
	Oid relspace = MyDatabaseTableSpace;
	CreateHashIndexTable(Heap_Id,Index_Id);

	boost::thread_group tg;
	tg.create_thread(boost::bind(&ReIndexHashIndexMultInsert,&Heap_Id,&Index_Id,&sta[0]));
	tg.join_all();

	tg.create_thread(boost::bind(&ReIndexHashIndexMultUpdate,&Heap_Id, &Index_Id, &sta[1]));
	tg.create_thread(boost::bind(&ReIndexHashIndexMultReIndex,&Heap_Id, &Index_Id,&sta[2]));
	tg.join_all();

	tg.create_thread(boost::bind(&ReIndexHashIndexMultScan,&Heap_Id,&Index_Id,&sta[3]));
	tg.join_all();
	
	int found = 0;
	for (int i=0; i<4; ++i)
	{
		if (1 == sta[i])
			++found;
	}

	if (4 == found)
		return true;
	else
		return false;
}

spinlock SetLock;
void ToastHashIndexMultInsert(int* HeapId, int* IndexId, std::set<std::string>* SetStr, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		
		FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, *HeapId, MyDatabaseId, *HeapId);
		Relation HeapRel = FDPG_Heap::fd_heap_open(*HeapId,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(HeapRel,HASH_TYPE,*IndexId,*IndexId);
		Relation IndexRel = FDPG_Index::fd_index_open(*IndexId,RowExclusiveLock, MyDatabaseId);

		string string1("01234567abcd");
		string string2("01345678bcde");
		string string3("01456789cdef");
		string string4("0156789adefg");
		string string5("016789abefgh");
		get_toast_data(string1,1<<16);
		get_toast_data(string2,1<<16);
		get_toast_data(string3,1<<16);
		get_toast_data(string4,1<<16);
		get_toast_data(string5,1<<16);
		SetStr->insert(string1);
		SetStr->insert(string2);
		SetStr->insert(string3);
		SetStr->insert(string4);
		SetStr->insert(string5);
		HeapTuple tuple;
		tuple = FDPG_Heap::fd_heap_form_tuple(string1.c_str(),string1.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		tuple = FDPG_Heap::fd_heap_form_tuple(string2.c_str(),string2.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		tuple = FDPG_Heap::fd_heap_form_tuple(string3.c_str(),string3.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		tuple = FDPG_Heap::fd_heap_form_tuple(string4.c_str(),string4.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		tuple = FDPG_Heap::fd_heap_form_tuple(string5.c_str(),string5.length());
		FDPG_Heap::fd_simple_heap_insert(HeapRel,tuple);
		
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);
		
		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0 ;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void ToastHashIndexMultUpdate(int* HeapId, int* IndexId, std::set<std::string>* SetStr, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		
		Relation HeapRel = FDPG_Heap::fd_heap_open(*HeapId,RowShareLock, MyDatabaseId);
		Relation IndexRel = FDPG_Index::fd_index_open(*IndexId,RowShareLock, MyDatabaseId);
		
		/*heap update*/
		int	len = 0;
		int	Count = 0;	
		HeapTuple tuple;
		HeapScanDesc heapScan = FDPG_Heap::fd_heap_beginscan(HeapRel,SnapshotNow, 0, NULL);
		while((tuple = FDPG_Heap::fd_heap_getnext(heapScan,ForwardScanDirection)) != NULL)
		{
			if (++Count < 3)
				continue;
			char* temp = fdxdb_tuple_to_chars_with_len(tuple,len);
			string str1(temp,len);
			SetLock.lock();
			if (SetStr->end() != SetStr->find(str1))
			{	
				SetStr->erase(str1);
				string str0("018abcdeghjk");
				get_toast_data(str0,1<<16);
				SetStr->insert(str0);
				HeapTuple UpdateTuple = FDPG_Heap::fd_heap_form_tuple(str0.c_str(),str0.size());
				FDPG_Heap::fd_simple_heap_update(HeapRel,&tuple->t_self,UpdateTuple);
				SetLock.unlock();
				break;
			}
			SetLock.unlock();
		}
		FDPG_Heap::fd_heap_endscan(heapScan);
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);

		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0 ;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void ToastHashIndexMultDelete(int* HeapId, int* IndexId, std::set<std::string>* SetStr, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		
		Relation HeapRel = FDPG_Heap::fd_heap_open(*HeapId,RowShareLock, MyDatabaseId);
		Relation IndexRel = FDPG_Index::fd_index_open(*IndexId,RowShareLock, MyDatabaseId);
		
		/*heap delete*/
		int	   len = 0;
		HeapTuple tuple;
		HeapScanDesc heapScan = FDPG_Heap::fd_heap_beginscan(HeapRel,SnapshotNow, 0, NULL);
		while((tuple = FDPG_Heap::fd_heap_getnext(heapScan,ForwardScanDirection)) != NULL)
		{
			char* temp = fdxdb_tuple_to_chars_with_len(tuple,len);
			string str1(temp,len);
			SetLock.lock();
			if (SetStr->end() != SetStr->find(str1))
			{
				FDPG_Heap::fd_simple_heap_delete(HeapRel,&tuple->t_self);
				SetLock.unlock();
				break;
			}
			SetLock.unlock();
		}
		FDPG_Heap::fd_heap_endscan(heapScan);
		FDPG_Heap::fd_heap_close(HeapRel, RowExclusiveLock);
		FDPG_Index::fd_index_close(IndexRel, RowExclusiveLock);

		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0 ;
	}
	FDPG_Heap::fd_proc_exit(0);
}

void ToastHashIndexMultScan(int* HeapId, int* IndexId, std::set<std::string>* SetStr, int* i)
{
	try
	{
		BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
    	param->MyThreadType = backend;
    	FDPG_Heap::fd_fxdb_SubPostmaster_Main((void*)param);
		
		begin_transaction();
		/*index*/
		ScanKeyData nScankey[2];
		Datum value[2];
		StrategyNumber ScanStrage[10] = {HTEqualStrategyNumber,HTEqualStrategyNumber};
		vector<string> vScanStr;
		vector<string> vEndResult;	
		int Count = 0;
		std::set<std::string>::iterator it;
		char buffer[10];
		char buffer2[10];
		int len = 0;
		int len2 = 0;
		string str;
		for(it=SetStr->begin(); it!=SetStr->end(); ++it)
		{
			str.clear();
			len = (*it).copy(buffer,3,0);
			if (3 == len)
			{
				str.append(buffer,len);
				len2 = (*it).copy(buffer2,4,8);
				if (4 == len2)
				{
					buffer2[4] = '\0';
					str.append(buffer2);
				}
			}

			if (7 == str.size())
			{
				vScanStr.clear();
				vScanStr.push_back(str);
				vScanStr.push_back(str);
				Relation HeapRel,IndexRel;
				Hash_Index_Scan(HeapRel,IndexRel,*HeapId,*IndexId,vScanStr,ScanStrage,2,nScankey,value,vEndResult);
				if (vEndResult.size()>0 && (SetStr->end() != SetStr->find(vEndResult[0])))
					++Count;
			}
		}
		
		if (4 != Count)
		{
			*i = 0;
			return;
		}
		commit_transaction();
		*i = 1;
	}
	catch(StorageEngineException& ex)
	{
		printf("ErrorMsg %s   ErrorNO %s\n", ex.getErrorMsg(),ex.getErrorNo());
		user_abort_transaction();
		*i = 0;
	}
	FDPG_Heap::fd_proc_exit(0);
}

bool ToastHashIndexMultTest()
{
	int sta[4] = {0};
	
	int Heap_Id = 90219;
	int Index_Id = 90319;
	Oid relspace = MyDatabaseTableSpace;
	CreateHashIndexTable(Heap_Id,Index_Id);
	set<string> SetStr;
	boost::thread_group tg;
	tg.create_thread(boost::bind(&ToastHashIndexMultInsert,&Heap_Id,&Index_Id,&SetStr,&sta[0]));
	tg.join_all();

	tg.create_thread(boost::bind(&ToastHashIndexMultUpdate,&Heap_Id, &Index_Id,&SetStr,&sta[1]));
	tg.create_thread(boost::bind(&ToastHashIndexMultDelete,&Heap_Id, &Index_Id,&SetStr,&sta[2]));
	tg.join_all();

	tg.create_thread(boost::bind(&ToastHashIndexMultScan,&Heap_Id,&Index_Id,&SetStr,&sta[3]));
	tg.join_all();
	
	int found = 0;
	for (int i=0; i<4; ++i)
	{
		if (1 == sta[i])
			++found;
	}

	if (4 == found)
		return true;
	else
		return false;
}
