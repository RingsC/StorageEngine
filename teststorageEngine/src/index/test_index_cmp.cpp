#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "stdlib.h"
#include "catalog/metaData.h"
#include "test_fram.h"
#include "utils/util.h"
#include "index/test_index_cmp.h"
#include "utils/fmgroids.h"
#include "interface/FDPGAdapter.h"
#include "iostream"
#include "interface/StorageEngineExceptionUniversal.h"
#define startnum 100000
#define endnum 150000
using namespace std;
using namespace FounderXDB::StorageEngineNS;
extern char *my_itoa(int value, char *string, int radix);
int my_compare_index(const char *str1, size_t len1, const char *str2, size_t len2)
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

void my_split_index_OneCol_MultMethod(RangeData& rangeData, const char *str, int col, size_t len = 0)
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
		rangeData.len = 3;
	}
}

void my_split_index(RangeData& rangeData, const char *str, int col, size_t len=0)
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

void my_split_index_number(RangeData& rangeData, const char *str, int col, size_t len = 0)
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
		rangeData.len = 2;
	}
}
bool test_indexscan_NumberCmp()
{
	INTENT("大量数值数据，index检索比较,int内存区中的数据直接拷入char内存区,"
		"前后2个字符分别作为key，通过检测是否与索引检索出的数据相等的比较策略比较"
		"输入的数据为100000至150000，预期的查询出的数据为123456。");

	try
	{	
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,2,2);//创建表的colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//建表

		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;
		begin_transaction();
		Relation testRelation;
		HeapTuple tup;
		int flag;//比较标志
		
		int s =123456;//预估数字
		char basechar[50];
		char cpychar1[2];
		char cpychar2[2];
		memcpy(basechar,&s,sizeof(s));//int内存区中的数据直接拷入char内存区，int32位，char8位，占用4个char
		memcpy(cpychar1,&s,2);//前两个字符赋给cpychar1
		memcpy(cpychar2,&basechar[2],2);//后两个字符赋给cpychar2

		Datum * values = (Datum *) palloc(sizeof(Datum)*2);//用几个datum就乘几
		values[0] = fdxdb_string_formdatum(cpychar1, 2);//构建两个keys
		values[1] = fdxdb_string_formdatum(cpychar2, 2);
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId); //打开heap
			
		unsigned int i;
		for (i=startnum;i<endnum;i++)//插入大量的tuple，从100000至150000
		{
		char insertData[50];
		memcpy(insertData, &i, sizeof(i));//i存入insertData中
		tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//构建元组
		FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//插入元组
		}
		CommandCounterIncrement();
 		Relation indexrelation;
 		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
 		colinfo->keys = 2;
 		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
 		colinfo->col_number[0] = 1;
 		colinfo->col_number[1] = 2;
 		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
 		colinfo->rd_comfunction[0] = my_compare_index;
 		colinfo->rd_comfunction[1] = my_compare_index;//比较函数
 		colinfo->split_function =  my_split_index_number;//index的split
		setColInfo(indid,colinfo);;
 		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//创建索引
 
 		IndexScanDesc index_scan;
 		ScanKeyData key[2];
 		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTEqualStrategyNumber, my_compare_index,values[0]); //初始化scankey
 		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],2,BTEqualStrategyNumber, my_compare_index,values[1]);

		indexrelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//打开索引

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexrelation, SnapshotNow, 2, 0);//开始索引扫描
		FDPG_Index::fd_index_rescan(index_scan, key, 2, NULL, 0);//扫描索引

 		int ss;
		char *tt;
 		while((tup = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
 		{
 			tt=fxdb_tuple_to_chars(tup);
 			ss=*(int*)tt;//转换成数字
			flag=(ss-s);//s为预估数字
			if (flag==0)
			{
				printf("检索正确,数据为:%d\n",ss);
			}
			else
			{
				printf("检索出错!\n");
				printf("检索的数字为d%\n",ss);
				user_abort_transaction();
				remove_index(rid,INDEX_ID);//++index_id
				remove_heap(THREAD_RID);//删表
				return false;
			}
 		}
		
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexrelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//删表
		if (colinfo->col_number)
 			free(colinfo->col_number);
 		if (colinfo->rd_comfunction)
 			free(colinfo->rd_comfunction);
 		free(colinfo);
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

bool test_indexscan_LargeMount()
{
	INTENT("扫描大量数据，进行字符串比较并确认数据正确"
			"比较的策略为前三个字符比123小，紧接着两个字符比45大"
			"输入的数据为100000至150000，预期输出100460至100999,101460至101999以此类推");
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

		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;//索引号
		Relation testRelation;
		HeapTuple tuple;
		int flag = 0;

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*2); //用几个datum就乘几
		values[0] = fdxdb_string_formdatum("123", 3);
		values[1] = fdxdb_string_formdatum("45", 2);
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));//初始化colinfo空间
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_index;//heap的split
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		int i;
		int datacols=0;//预估数据存放行号
		char insertData[20]={0};
		char baseData[20]={0};
		char (*storeData)[20] = new char[50000][20];
		char string[20] = {0};
		for (i=startnum;i<endnum;i++)//存储预测数据
		{
			my_itoa(i,baseData,10);
			char baseData_3[3]={0};
			char baseData_2[2]={0};
			memcpy(baseData_3,baseData,sizeof(baseData_3));//前三个字符放在baseData_3中
			memcpy(baseData_2,&baseData[3],sizeof(baseData_2));//接着两个字符放在baseData_2中
			char *cpychar1="123";
			flag=memcmp(baseData_3,cpychar1,sizeof(baseData_3));//比较前三个字符
			if (flag<0)
			{
				char *cpychar2="45";
				flag=memcmp(baseData_2,cpychar2,sizeof(baseData_2));//比较后两个字符
				if (flag>0)
				{
					memcpy(storeData[datacols],baseData,sizeof(baseData));//预估数据存在storeData中
					datacols++;
				}
			}		
		}
					
		for (i=startnum;i<endnum;i++)//构建大量的字符元组
		{
			my_itoa(i,string,10);
			memcpy(insertData,string,sizeof(string));
			tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//构建元组
			FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//插入元组
		}
		CommandCounterIncrement();

		Relation indexrelation;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));//初始化colinfo空间
		colinfo->keys = 2;//在两列上建立索引
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->col_number[1] = 2;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_index;
		colinfo->rd_comfunction[1] = my_compare_index;//比较函数
		colinfo->split_function =  my_split_index;//index的split
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//创建索引

		IndexScanDesc index_scan;
		ScanKeyData key[2];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTLessStrategyNumber, my_compare_index,values[0]);	 //初始化scankey
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],2,BTGreaterStrategyNumber, my_compare_index,values[1]);
		indexrelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//打开索引

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexrelation, SnapshotNow, 2, 0);//开始索引扫描
		FDPG_Index::fd_index_rescan(index_scan, key, 2, NULL, 0);//扫描索引
		char *tt;//存储提取scan的char
		int storeRow=0;
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			int tup_len = 0;
			tt = fdxdb_tuple_to_chars_with_len(tuple,tup_len);
			flag=memcmp(storeData[storeRow],tt,tup_len);
			storeRow++;
			if (flag!=0)
			{
				printf("检索出错!\n");
				printf("出错的地方的字符为:%s\n",tt);
				FDPG_Index::fd_index_endscan(index_scan);
				FDPG_Index::fd_index_close(indexrelation,AccessShareLock);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				remove_index(rid,INDEX_ID);//++index_id
				remove_heap(THREAD_RID);//删表
				user_abort_transaction();
				delete[] storeData;
				return false;
			}
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexrelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//删表
    delete[] storeData;

		if (colinfo->col_number)
			free(colinfo->col_number); 	
		if (colinfo->rd_comfunction)
			free(colinfo->rd_comfunction);
 		free(colinfo);
		free(colinfo0);
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

bool test_indexscan_OneCol_MultMethod()
{
	INTENT("扫描大量数据，进行字符串比较并确认数据正确"
			"在一列上建立多个索引策略,策略为前三个字符大于123，并且小于134"
			"输入数据为100000至150000，预期输出数字为124000至133999");
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

		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;//索引号
		Relation testRelation;
		HeapTuple tuple;
		int flag = 0;

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*2); //用几个datum就乘几
		values[0] = fdxdb_string_formdatum("123", 3);
		values[1] = fdxdb_string_formdatum("134", 3);
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));//初始化colinfo空间
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_index_OneCol_MultMethod;//heap的split
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		int i,storeRow=0;
		int begintry=124000;//预估开始数据
		int endtry=134000;//预估结束数据
		char insertData[10]={"\0"};
		char storeData[50000][10]={"\0"};
		char string[10] = {"\0"};
		for (i=begintry;i<endtry;i++) //保存预估数据
		{
			my_itoa(i,string,10);
			memcpy(storeData[storeRow],string,sizeof(string));
			storeRow++;
		}

		for (i=startnum;i<endnum;i++)//构建大量的字符元组
		{
			my_itoa(i,string,10);
			memcpy(insertData,string,sizeof(string));
			tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//构建元组
			FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//插入元组
		}
		CommandCounterIncrement();

		Relation indexrelation;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));//初始化colinfo空间
		colinfo->keys = 2;//在两列上建立索引
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->col_number[1] = 1;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_index;
		colinfo->rd_comfunction[1] = my_compare_index;//比较函数
		colinfo->split_function =  my_split_index_OneCol_MultMethod;//index的split
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//创建索引

		IndexScanDesc index_scan;
		ScanKeyData key[2];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTGreaterStrategyNumber, my_compare_index,values[0]);	 //初始化scankey
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],1,BTLessStrategyNumber, my_compare_index,values[1]);
		indexrelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//打开索引

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexrelation, SnapshotNow, 2, 0);//开始索引扫描
		FDPG_Index::fd_index_rescan(index_scan, key, 2, NULL, 0);//扫描索引
		char *tt;//存储提取scan的char
		storeRow=0;
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			int tup_len = 0;
			tt = fdxdb_tuple_to_chars_with_len(tuple,tup_len);
			flag=memcmp(storeData[storeRow],tt,tup_len);
			storeRow++;
			if (flag!=0)
			{
				printf("检索出错!\n");
				printf("出错的地方的字符为:%s\n",tt);
				FDPG_Index::fd_index_endscan(index_scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				FDPG_Index::fd_index_close(indexrelation, AccessShareLock);
				remove_index(rid,INDEX_ID);//++index_id
				remove_heap(THREAD_RID);//删表
				user_abort_transaction();
				return false;
			}
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexrelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//删表
		
		if (colinfo->col_number)
			free(colinfo->col_number); 	
		if (colinfo->rd_comfunction)
			free(colinfo->rd_comfunction);
		free(colinfo);
		free(colinfo0);//释放申请的colinfo0空间
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

bool test_indexscan_MultCol_MultMethod()
{
	INTENT("扫描大量数据，进行字符串比较并确认数据正确"
		"在两列上分别建立两个索引策略,策略为前三个字符大于123，"
		"并且小于134，接着两个字符大于45，小于78"
		"输入数据为100000至150000，"
		"预期输出数字为124460至124779,125469至125460以此类推至133779");
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

		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;//索引号
		Relation testRelation;
		HeapTuple tuple;
		int flag = 0;

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*4); //用几个datum就乘几
		values[0] = fdxdb_string_formdatum("123", 3);
		values[1] = fdxdb_string_formdatum("134", 3);
		values[2] = fdxdb_string_formdatum("45", 2);
		values[3] = fdxdb_string_formdatum("78", 2);
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));//初始化colinfo空间
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_index;//heap的split
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		int i;
		int k=0;
		int datacols=0;
		char insertData[20]={"\0"};
		char baseData[20]={"\0"};
		char storeData[20000][20]={"\0"};
		char string[20] = {"\0"};
		for (i=startnum;i<endnum;i++)//存储预测数据
		{
			my_itoa(i,baseData,10);
			char baseData_3[3]={"\0"};
			char baseData_2[2]={"\0"};
			memcpy(baseData_3,baseData,sizeof(baseData_3));//前三个字符放在baseData_3中
			memcpy(baseData_2,&baseData[3],sizeof(baseData_2));//接着两个字符放在baseData_2中
			char *cpychar1_1="123";
			flag=memcmp(baseData_3,cpychar1_1,sizeof(baseData_3));//比较前三个字符
			if (flag>0)
			{
				char *cpychar1_2="134";
				flag=memcmp(baseData_3,cpychar1_2,sizeof(baseData_3));
				if (flag<0)
				{	
					char *cpychar2_1="45";
					flag=memcmp(baseData_2,cpychar2_1,sizeof(baseData_2));//比较后两个字符
					if (flag>0)
					{
						char *cpychar2_2="78";
						flag=memcmp(baseData_2,cpychar2_2,sizeof(baseData_2));
						if (flag<0)
						{
							memcpy(storeData[datacols],baseData,sizeof(baseData));//预估数据存在storeData中
							datacols++;
						}
					}
				}

			}		
			k++;
		}

		for (i=startnum;i<endnum;i++)//构建大量的字符元组
		{
			my_itoa(i,string,10);
			memcpy(insertData,string,sizeof(string));
			tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//构建元组
			FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//插入元组
		}
		CommandCounterIncrement();

		Relation indexRelation;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));//初始化colinfo空间
		colinfo->keys = 2;//在列上建立索引
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->col_number[1] = 2;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_index;
		colinfo->rd_comfunction[1] = my_compare_index;
		colinfo->split_function =  my_split_index;//index的split
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//创建索引
		CommandCounterIncrement();

		IndexScanDesc index_scan;
		ScanKeyData key[4];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTGreaterStrategyNumber, my_compare_index,values[0]);//初始化scankey
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],1,BTLessStrategyNumber, my_compare_index,values[1]);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[2],2,BTGreaterStrategyNumber, my_compare_index,values[2]);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[3],2,BTLessStrategyNumber, my_compare_index,values[3]);
		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//打开索引

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, 4, 0);//开始索引扫描
		FDPG_Index::fd_index_rescan(index_scan, key, 4, NULL, 0);//扫描索引
		char *tt;//存储提取scan的char
		int storeRow=0;
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			int tup_len = 0;
			tt = fdxdb_tuple_to_chars_with_len(tuple,tup_len);
			flag=memcmp(storeData[storeRow],tt,tup_len);
			storeRow++;
 			if (flag!=0)
 			{
 				printf("检索出错!\n");
 				printf("出错的地方的字符为:%s\n",tt);
 				FDPG_Index::fd_index_endscan(index_scan);
 				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
 				FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
				remove_index(rid,INDEX_ID);//++index_id
				remove_heap(THREAD_RID);//删表
 				user_abort_transaction();
 				return false;
 			}
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//删表

		if (colinfo->col_number)
			free(colinfo->col_number); 	
		if (colinfo->rd_comfunction)
			free(colinfo->rd_comfunction);
		free(colinfo);
		free(colinfo0);//释放申请的colinfo0空间
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

bool test_indexscan_MultCol_Col1OneMethod_Col2MultMethod()
														  //多列索引不一定有单列索引效率高，单列索引再利用优化器可能更快
{
	INTENT("扫描大量数据，进行字符串比较并确认数据正确"
		"在两列上建立索引策略,策略为前三个字符等于123"
		",接着的两个字符大于23，小于78"
		"输入数据为100000至150000"
		"预期输出数字为123240至123779");
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

		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;//索引号
		Relation testRelation;
		HeapTuple tuple;
		int flag = 0;
		int beginnum=123240;
		int finishnum=123780;

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*3); //用几个datum就乘几
		values[0] = fdxdb_string_formdatum("123", 3);
		values[1] = fdxdb_string_formdatum("23", 2);
		values[2] = fdxdb_string_formdatum("78", 2);
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));//初始化colinfo空间
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_index;//heap的split
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		int i;
		int storeRow=0;
		char insertData[20]={"\0"};
		char storeData[1000][20]={"\0"};
		char string[10] = {"\0"};
		for (i=beginnum;i<finishnum;i++)//存储预测数据
		{
			my_itoa(i,string,10);
			memcpy(storeData[storeRow],string,sizeof(string));//预期输出数字为123240至123779"
			storeRow++;
		}

		for (i=startnum;i<endnum;i++)//构建大量的字符元组
		{
			my_itoa(i,string,10);
			memcpy(insertData,string,sizeof(string));
			tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//构建元组
			FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//插入元组

		}

		Relation indexRelation;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));//初始化colinfo空间
		colinfo->keys = 2;//在列上建立索引
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->col_number[1] = 2;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_index;
		colinfo->rd_comfunction[1] = my_compare_index;
		colinfo->split_function =  my_split_index;//index的split
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//创建索引
		CommandCounterIncrement();

		IndexScanDesc index_scan;
		ScanKeyData key[3];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTEqualStrategyNumber, my_compare_index,values[0]);//初始化scankey
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],2,BTGreaterStrategyNumber, my_compare_index,values[1]);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[2],2,BTLessStrategyNumber, my_compare_index,values[2]);

		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//打开索引

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, 3, 0);//开始索引扫描
		FDPG_Index::fd_index_rescan(index_scan, key, 3, NULL, 0);//扫描索引
		char *tt;//存储提取scan的char
		storeRow=0;//初始化存储数据行数
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			int tup_len = 0;
			tt = fdxdb_tuple_to_chars_with_len(tuple, tup_len);
			flag=memcmp(storeData[storeRow],tt,tup_len);//index扫描出来的数据与预测数据比较
			storeRow++;
			if (flag!=0)
			{
			 	printf("检索出错!\n");
			 	printf("出错的地方的字符为:%s\n",tt);
				FDPG_Index::fd_index_endscan(index_scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
				FDPG_Index::fd_index_drop(rid, reltablespace, indid, MyDatabaseId);
				user_abort_transaction();
				return false;
			}
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//删表

		if (colinfo->col_number)
			free(colinfo->col_number); 	
		if (colinfo->rd_comfunction)
			free(colinfo->rd_comfunction);
		free(colinfo);
		free(colinfo0);//释放申请的colinfo0空间
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

bool test_index_uniqe_01()
{
	INTENT("测试唯一索引，向表中插入多行相同的数据，建索引的过程传入" 
		"UNIQUE_CHECK_YES进行表唯一检测，报错，会被catch到.");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;

	SpliterGenerater sg;
	const int COL_NUM = 3;

	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//创建表的colinfo
	THREAD_RID += 10;
	int rid=THREAD_RID;
	INDEX_ID += 10;
	int index_id=INDEX_ID;
	setColInfo(rid, heap_info);
	create_heap(rid,heap_info);//建表

	const unsigned int data_row = 10;
	const unsigned int data_len = 20;
	DataGenerater dg(data_row, data_len);
	dg.dataGenerate();

	char data[][DATA_LEN] = {"aqwevadr", "aqwevadr","aqwevadr"};//初始化插入表的数据
	insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN,rid);//向表中插入数据

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//构建index的colinfo
	try
	{
		create_index(rid, heap_info,index_info,index_id, UNIQUE_CHECK_YES);//建索引
	}
	catch(...)
	{
		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//删表
		return TRUE;
	}
	return false;
}

bool test_index_uniqe_02()
{
	INTENT("测试唯一索引，向表中插入多行相同的数据并建索引，在更新索引的过程中，"
		"调用的index_insert传入UNIQUE_CHECK_YES 报错，会被catch住。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	char *update_src_1 =  "aqwevadr"; 
	char *update_det_1 =   "aqwevadr"; 
	SpliterGenerater sg;
	const int COL_NUM = 3;

	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//创建表的colinfo
	++THREAD_RID;
	int rid=THREAD_RID;
	++INDEX_ID;
	int index_id=INDEX_ID;
	setColInfo(rid, heap_info);
	create_heap(rid,heap_info);//建表

	const unsigned int data_row = 10;
	const unsigned int data_len = 20;
	DataGenerater dg(data_row, data_len);
	dg.dataGenerate();

	char data[][DATA_LEN] = {"aqwevadr", "aqwevadr","aqwevadr"};//初始化插入表的数据

	insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN,rid);//向表中插入数据

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//构建index的colinfo
	create_index(rid, heap_info,index_info,index_id);//建索引

	Relation rel = NULL;
	begin_transaction();
	int count = 0;
	int found[DATA_LEN];
	Datum		values[1];
	bool		isnull[1];

	Relation indrel1 = NULL;
	rel = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);
	ItemPointerData ipd1 = findTuple(update_src_1, rel, found[1], count);//找到src
	HeapTuple tuple1 = fdxdb_heap_formtuple(update_det_1, sizeof("aqwevadr"));

	FDPG_Heap::fd_simple_heap_update(rel, &ipd1, tuple1);//更新表
	indrel1 = FDPG_Index::fd_index_open(index_id,AccessShareLock, MyDatabaseId);//打开索引表
	values[0] = fdxdb_form_index_datum(rel, indrel1, tuple1);
	isnull[0] = false;
	try
	{
		FDPG_Index::fd_index_insert(indrel1, values, isnull, &(tuple1->t_self), rel, UNIQUE_CHECK_YES);//更新索引
	}
	catch(...)
	{
		FDPG_Index::fd_index_close(indrel1,AccessShareLock);//关闭索引
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		CommandCounterIncrement();
		commit_transaction();
		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//删表
		return TRUE;
	}
	commit_transaction();
	remove_index(rid,INDEX_ID);//++index_id
	remove_heap(THREAD_RID);//删表
	return TRUE;
}
#define COUNT 100
#define ARRAY_IN "aqwev"
bool test_index_uniqe_03()
{
	INTENT("在一个表中进行多次插入同一条数据，同时更新索引，"
		"测试能否正确执行,在这个过程中不检测uniqe，最后index_scan扫描出结果。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	SpliterGenerater sg;
	int i=0;
	const int COL_NUM = 2;
	int data_len=sizeof(ARRAY_IN);
	char predict_data[COUNT][DATA_LEN]={"\0"};

	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//创建表的colinfo
	++THREAD_RID;
	int rid=THREAD_RID;
	++INDEX_ID;
	int index_id=INDEX_ID;
	setColInfo(rid, heap_info);
	create_heap(rid,heap_info);//建表

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//构建index的colinfo
	create_index(rid, heap_info,index_info,index_id);//建索引

	HeapIndexRelation hir;
	hir.index_array_len = 1;
	hir.heap_id = rid;
	hir.heap_info = heap_info;
	hir.index_id = (Oid*)malloc(hir.index_array_len * sizeof(Oid));
	hir.index_id[0] = index_id;
	hir.index_info = (Colinfo*)malloc(hir.index_array_len * sizeof(Colinfo));
	hir.index_info[0] = index_info;

	for (i=0;i<COUNT;i++)
	{
		memcpy(predict_data[i],ARRAY_IN,data_len);//预测数据
	}

	char data[][DATA_LEN] = {ARRAY_IN};//初始化插入表的数据
	for (i=0;i<COUNT;i++)
	{
		insert_with_index(data,data_len,1,&hir,UNIQUE_CHECK_NO);//插入数据并更新索引
	}

	IndexScanInfo isi;
	alloc_scan_space(INDEX_COL_NUM, isi);
	isi.cmp_values[0] = fdxdb_string_formdatum("aqw", strlen("aqw"));//初始化scankey
	isi.cmp_values[1] = fdxdb_string_formdatum("ev", strlen("ev"));
	isi.cmp_func[0] = my_compare_str;
	isi.cmp_func[1] = my_compare_str;
	isi.cmp_strategy[0] = BTGreaterEqualStrategyNumber;
	isi.cmp_strategy[1] = BTGreaterEqualStrategyNumber;
	isi.col_array[0] = 1;
	isi.col_array[1] = 2;
	init_scan_key(isi);

	unsigned int array_len=0;
	char result_data[COUNT][DATA_LEN];
	DataGenerater found_dg(100, DATA_LEN);
	index_scan_hs(index_id, index_info, rid, heap_info,&isi, &found_dg,&array_len);
	//生成二维字符串数组
	found_dg.dataToDataArray2D(array_len, result_data);
	bool sta = check_array_equal(predict_data, result_data, data_len, ARRAY_LEN_CALC(data));
	assert(sta == TRUE);

	remove_index(rid,INDEX_ID);//++index_id
	remove_heap(THREAD_RID);//删表

	return TRUE;
}

bool test_index_update_multi()
{
	INTENT("在一个表中进行多次插入一条随机数据，同时更新索引，测试能否正确执行。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	SpliterGenerater sg;
	int i=0;
	int count=100;
	const int COL_NUM = 3;

	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//创建表的colinfo
	++THREAD_RID;
	int rid=THREAD_RID;
	++INDEX_ID;
	int index_id=INDEX_ID;
	setColInfo(rid, heap_info);
	create_heap(rid,heap_info);//建表

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//构建index的colinfo
	create_index(rid, heap_info,index_info,index_id);//建索引

	for (i=0;i<count;i++)
	{
		const unsigned int data_row = 1;
		const unsigned int data_len = 6;
		DataGenerater dg(data_row, data_len);
		dg.dataGenerate();
		char data[data_row][DATA_LEN];
		dg.dataToDataArray2D(data);//初始化插入表的数据

		insert_data(data, 1, DATA_LEN,rid);//向表中插入数据
		form_index(data, 1, data_len, rid, index_id, heap_info, index_info, UNIQUE_CHECK_NO);//更新索引
	}
	remove_index(rid,INDEX_ID);//++index_id
	remove_heap(THREAD_RID);//删表
	
	return TRUE;
}

bool test_debug_001()
{
	try
	{

	INTENT("debug 001");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	SpliterGenerater sg;
	int i=0;
	const int COL_NUM = 3;
	HeapTuple tuple;
	++THREAD_RID;
	++INDEX_ID;
	int rid=THREAD_RID;
	int index_id=INDEX_ID;

	begin_transaction();

	Oid relspace = MyDatabaseTableSpace;
	SAVE_INFO_FOR_DEBUG();
	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//创建表的colinfo
	setColInfo(rid, heap_info);
	rid = FDPG_Heap::fd_heap_create(relspace, 0,0,rid);

	Relation heap_relation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock, MyDatabaseId);

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//构建index的colinfo
	setColInfo(index_id , index_info);

	FDPG_Index::fd_index_create(heap_relation, BTREE_UNIQUE_TYPE,index_id,index_id);

	Relation indexRelation = FDPG_Index::fd_index_open(index_id,AccessShareLock, MyDatabaseId);//打开索引

	//CommandCounterIncrement();
	const unsigned int data_row = 10;
	const unsigned int data_len = 20;
	DataGenerater dg(data_row, data_len);
	dg.dataGenerate();

	char data[][DATA_LEN] = {"abcdefse","e3erefas","fasdferera","fefsdfe4543"
							 "aqwevadr","rer6u7y6","afhth656wgf","r3rasfe4wea"
							 "aqwdeeadr","sdfet45fsd","dfr3fsdaaf","ef3radsfee"};//初始化插入表的数据

	for(int i = 0; i <  ARRAY_LEN_CALC(data); ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(data[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(heap_relation, tuple);
		}
		CommandCounterIncrement();
	//commit_transaction();

	StorageEngineFixture::bEnded = true;
	on_exit_reset();
	exit(0);
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

bool test_debug_002()
{
	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	SpliterGenerater sg;
	try
	{
		INTENT("debug 002");
		begin_transaction();
		HeapTuple tuple;
		int rid=513;
		++INDEX_ID;
		int index_id=INDEX_ID;
		Relation indexRelation;
		Relation testRelation;

		const int COL_NUM = 3;
		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//创建表的colinfo
		setColInfo(rid, heap_info);

		const int INDEX_COL_NUM = 2;
		int col_number[INDEX_COL_NUM] = {1,2};
		CompareCallback cmp_func[INDEX_COL_NUM] = 
		{
			my_compare_str,
			my_compare_str
		};
		Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//构建index的colinfo

		setColInfo(index_id , index_info);
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		indexRelation = FDPG_Index::fd_index_open(index_id,AccessShareLock, MyDatabaseId);//打开索引

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*1); //用几个datum就乘几
		values[0] = fdxdb_string_formdatum("a", 1);

		IndexScanDesc index_scan;
		ScanKeyData key[1];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTGreaterEqualStrategyNumber, my_compare_index,values[0]);//初始化scankey

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, 1, 0);//开始索引扫描
		FDPG_Index::fd_index_rescan(index_scan, key, 1, NULL, 0);//扫描索引
		char *tt;//存储提取scan的char

		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			tt=fxdb_tuple_to_chars(tuple);
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//删表 ++heap_id
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