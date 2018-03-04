/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.9.18		 测试在一个事务内，相同列上创建多个索引是否正确
001			许彦      创建		  2011.9.22		 测试在一个事务内，不同列上创建多个索引是否正确
002			许彦      创建		  2011.9.22		 测试在一个事务内，创建单个索引，并给多个scankey是否正确
003			许彦      创建		  2011.9.18		 测试在多个事务中，先插入数据，再在相同列上跨事务创建多个索引是否正确
004			许彦      创建		  2011.9.18		 测试在多个事务中，先在相同列上创建多个索引，后插入数据，再新起事务创建索引是否正确


************************************************************************/

#include <iostream>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "PGSETypes.h"
#include "transaction/test_transaction_index_create_dll.h"

using namespace FounderXDB::StorageEngineNS;

extern void initKeyVector(vector<ScanCondition> &keyVec,int col_number, ScanCondition::CompareOperation cmp_op,const char *keydata,int (*compare_func)(const char *str1, size_t len1, const char *str2, size_t len2) );
uint32 index_entry_colid = 501;

int my_compare_str_index(const char *str1, size_t len1, const char *str2, size_t len2)
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

void my_split_index31(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.len = 0;
	rangeData.start = 0;
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
		rangeData.len = 1;
	}
}

void my_split_index21(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.len = 0;
	rangeData.start = 0;
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

void my_split_index13(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	
	rangeData.len = 0;
	rangeData.start = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 1;
	}
	if (col == 2)
	{
		rangeData.start = 1;
		rangeData.len = 3;
	}
}

void my_split_index12(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.len = 0;
	rangeData.start = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 1;
	}
	if (col == 2)
	{
		rangeData.start = 1;
		rangeData.len = 2;
	}
}

void my_split_index1(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.len = 0;
	rangeData.start = 0;
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

ColumnInfo *alloc_index_colinfo(const int nIndexCols, int *heap_col_number, Spliti split_func)
{
	ColumnInfo *colinfo = (ColumnInfo *)malloc(sizeof(ColumnInfo));
	colinfo->keys = nIndexCols;
	colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
	for(int i = 0; i<nIndexCols ; ++i)
	{
		colinfo->col_number[i] = heap_col_number[i];
	}
	colinfo->rd_comfunction = (CompareCallbacki*)malloc(colinfo->keys * sizeof(CompareCallbacki));
	for(int i = 0; i<nIndexCols ; ++i)
	{
		colinfo->rd_comfunction[i] = my_compare_str_index;
	}
	colinfo->split_function = split_func;
	return colinfo;
}

uint32 set_colinfo(ColumnInfo *colinfo)
{
	setColumnInfo(index_entry_colid, colinfo);
	return index_entry_colid++;
}

void index_set_col_info(const int nkeys = 2)
{
	ColumnInfo *colinfo = (ColumnInfo *)malloc(sizeof(ColumnInfo));
	colinfo->keys = nkeys;
//	colinfo->col_number = new int[colinfo->keys];
	colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = 1;
	colinfo->col_number[1] = 3;
//	colinfo->rd_comfunction = new CompareCallbacki[colinfo->keys];
	colinfo->rd_comfunction = (CompareCallbacki*)malloc(colinfo->keys * sizeof(CompareCallbacki));
	colinfo->rd_comfunction[0] = my_compare_str_index;
	colinfo->rd_comfunction[1] = my_compare_str_index;
	colinfo->split_function =  my_split_index31;
	setColumnInfo(30, colinfo);
}

// void create_multi_index(EntrySet *pEntrySet,const int nIndex,IndexEntrySet **pIndexEntry,ColumnInfo **pColinfo)
// {
void create_multi_index(EntrySet *pEntrySet,const int nIndex,EntrySetID *pEntryID,ColumnInfo **pColinfo)
{
	for(int i = 0; i<nIndex ; ++i )
	{
		pEntryID[i] = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE,set_colinfo(pColinfo[i]),0,NULL);
//		pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,pIndexEntry[i],NULL);
	}
// 	for(int i = 0; i<nIndex ; ++i )
// 	{
// 		pIndexEntry[i] = pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,pIndexEntry[i]->getId(),NULL);
// 	}
}

void open_mutil_index(EntrySet *pEntrySet,const int nIndex,IndexEntrySet **pIndexEntry,EntrySetID *pEntryID)
{
	for(int i = 0; i<nIndex ; ++i )
	{
		pIndexEntry[i] = pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,pEntryID[i],NULL);
	}
}

void start_mutil_index_scan(const int nIndex,IndexEntrySetScan **pIndexScan,IndexEntrySet **pIndexEntry,vector<ScanCondition> *keyVec)
{
	for(int i = 0; i<nIndex; ++i)
	{
		pIndexScan[i] = (IndexEntrySetScan *)pIndexEntry[i]->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,keyVec[i]);
	}
}

void end_mutil_index_scan(const int nIndex,IndexEntrySetScan **pIndexScan,IndexEntrySet **pIndexEntry)
{
	for(int i = 0; i<nIndex; ++i)
	{
		pIndexEntry[i]->endEntrySetScan(pIndexScan[i]);
	}	
}

void close_mutil_index(const int nIndex,IndexEntrySet **pIndexEntry)
{
	for(int i = 0; i<nIndex; ++i)
	{
		pStorageEngine->closeEntrySet(pTransaction,pIndexEntry[i]);
	}	
}

void init_mutil_index_key(const int nIndex,const int *nIndexCols,vector<ScanCondition> *keyVec,
						  ScanCondition::CompareOperation cmp_op[][20], char keydata[][20][20])
{
	for(int i = 0; i<nIndex; ++i)
	{
		for(int j = 0; j<nIndexCols[i]; ++j)
		{
			initKeyVector(keyVec[i],j+1,cmp_op[i][j],keydata[i][j],my_compare_str_index);
 		}
 	}
}

void init_mutil_index_key_twice(const int nIndex,const int *nIndexCols,vector<ScanCondition> *keyVec,
						  ScanCondition::CompareOperation cmp_op[][20], char keydata[][20][20])
{
	for(int i = 0; i<nIndex; ++i)
	{
		for(int j = 0; j<nIndexCols[i]; ++j)
		{
			initKeyVector(keyVec[i],j+1,cmp_op[i][j],keydata[i][j],my_compare_str_index);
		}
	}
}

bool checkResult(const int nIndex,IndexEntrySetScan **pIndexScan,char cmpData[][20],const int dataRows)
{
	DataItem *pScanData = new DataItem;
	EntryID scan_tid;
	int counter = 0;
	int data_flag;
	for(int i = 0; i<nIndex; ++i)
	{
		while ( pIndexScan[i]->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{	
			cout << (char*)pScanData->getData() << endl;
			data_flag = strcmp((char*)pScanData->getData(),cmpData[i]);
			CHECK_BOOL(data_flag == 0);
			if(data_flag != 0)
			{
				cout << "插入数据与查询数据内容不一致!" << endl;
				cout << "插入数据为" << cmpData[i] << endl;	
				cout << "查询数据为" << (char*)pScanData->getData() << endl;
				delete pScanData;
				return false;
			}
			++counter;	
		}
	}
	CHECK_BOOL(counter == dataRows);
	if(counter != dataRows)
	{
		cout << "插入数据与查询数据行数不一致!" << endl;
		cout << "查询数据行数counter=" << counter << endl;
		delete pScanData;
		return false;			
	}
	delete pScanData;
	return true;
}

bool check_single_scan_result(IndexEntrySetScan *pIndexScan ,char cmpData[][20],int dataRows)
{
	DataItem *pScanData = new DataItem;
	EntryID scan_tid;
	int counter = 0;
	int data_flag;
	int i = 0;
	while ( pIndexScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
	{		
		cout << (char*)pScanData->getData() << endl;
		data_flag = memcmp((char*)pScanData->getData(),cmpData[i],pScanData->getSize());
		CHECK_BOOL(data_flag == 0);
		if(data_flag != 0)
		{
			cout << "插入数据与查询数据内容不一致!" << endl;
			cout << "插入数据为" << cmpData[i] << endl;	
			cout << "查询数据为" << (char*)pScanData->getData() << endl;
			delete pScanData;
			return false;
		}
		++counter;
		++i;
	}
	CHECK_BOOL(counter == dataRows );
	if(counter != dataRows)
	{
		cout << "插入数据与查询数据行数不一致!" << endl;
		cout << "查询数据行数counter=" << counter << endl;
		delete pScanData;
		return false;		
	}
	delete pScanData;
	return true;
}

void end_scan_and_close_entry(const int nIndex,IndexEntrySetScan **pIndexScan,IndexEntrySet **pIndexEntry,EntrySet *pEntrySet)
{
	end_mutil_index_scan(nIndex,pIndexScan,pIndexEntry);
	close_mutil_index(nIndex,pIndexEntry);
	pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
}

void free_memory(char *insert_str,vector<DataItem*> dvec)
{
	if( NULL != insert_str )
	{
		free(insert_str);
//		cout << "del str suc" << endl;
	}
		
	for(vector<DataItem*>::size_type ix = 0; ix != dvec.size(); ++ix)
	{
		if( NULL != dvec[ix] )
		delete dvec[ix];
//		cout << "del dvec suc" << endl;
	}
}

void fill_info_array(const int nColinfo,ColumnInfo **pColinfoArr,int heapNumberArr[][20],int *nIndexColsArr,Spliti *splitFuncArr)
{
	for(int i = 0; i<nColinfo; ++i)
	{
		pColinfoArr[i] = alloc_index_colinfo(nIndexColsArr[i],heapNumberArr[i],splitFuncArr[i]);
	}
}

void close_entry_set(EntrySet *pEntrySet)
{
	pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
}	

#define DATAROWS_INDEX 10
#define DATA_INDEX "abcd_"
#define NINDEX 5
#define L ScanCondition::LessThan
#define LE ScanCondition::LessEqual
#define E ScanCondition::Equal
#define GE ScanCondition::GreaterEqual
#define G ScanCondition::GreaterThan

bool test_index_transaction_create_dll_000()
{
	INTENT("测试在一个事务内，相同列上创建多个索引是否正确");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(DATAROWS_INDEX,DATA_INDEX);
		vector<DataItem*> dvec;
		char copy_insert_data[DATAROWS_INDEX][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,DATAROWS_INDEX,DATA_INDEX);
		command_counter_increment();

		//在1,3列上建立索引
		const int nIndex = NINDEX;
		const int nIndexCols = 2;
		int heap_col_number[nIndexCols] = {1,3};
		ColumnInfo *pcolinfo = alloc_index_colinfo(nIndexCols,heap_col_number,my_split_index31);
			
		EntrySetID pIndexEntryID[nIndex];
		IndexEntrySet *pIndexEntry[nIndex];
		ColumnInfo *pColinfoArray[nIndex] = {pcolinfo,pcolinfo,pcolinfo,pcolinfo,pcolinfo};

		//创建index
		create_multi_index(pEntrySet,nIndex,pIndexEntryID,pColinfoArray);
		open_mutil_index(pEntrySet,nIndex,pIndexEntry,pIndexEntryID);
		command_counter_increment();
		
		vector<ScanCondition> keyVecArray[nIndex];
		const int nIndexColsArr[nIndex] = {2,2,2,2,2};
		ScanCondition::CompareOperation cmp_op[nIndex][20] = {{GE,E},{LE,G},{E,L},{G,E},{L,E}};
		char keydata[nIndex][20][20] = { { "abc","2" },{"abc","8"},{"abc","1"},{"abb","4"},{"abd","3"}};
 		init_mutil_index_key(nIndex,nIndexColsArr,keyVecArray,cmp_op,keydata);
		
		IndexEntrySetScan *pIndexScan[nIndex];
		start_mutil_index_scan(nIndex,pIndexScan,pIndexEntry,keyVecArray);
		char cmpData[][20] = {"abcd_2","abcd_9","abcd_0","abcd_4","abcd_3"};
		const int dataRows = 5;
		if( false == checkResult(nIndex,pIndexScan,cmpData,dataRows) )
		{
			end_scan_and_close_entry(nIndex,pIndexScan,pIndexEntry,pEntrySet);
			user_abort_transaction();
			return false;
		}
		free_memory(insert_str,dvec);
		end_scan_and_close_entry(nIndex,pIndexScan,pIndexEntry,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_index_transaction_create_dll_001()
{
	INTENT("测试在一个事务内，不同列上创建多个索引是否正确");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(DATAROWS_INDEX,DATA_INDEX);
		vector<DataItem*> dvec;
		char copy_insert_data[DATAROWS_INDEX][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,DATAROWS_INDEX,DATA_INDEX);
		command_counter_increment();

		//在1,3列上建立索引
		const int nIndex = NINDEX;
		const int nColinfo = 5;
		ColumnInfo *pColinfoArr[nIndex];
		int heapNumberArr[][20] = {{1,3},{2,3},{3,1},{3,2},{3}};
		int nIndexColsArr[nColinfo] = {2,2,2,2,1};
		Spliti splitFuncArr[nColinfo] = {my_split_index31,my_split_index21,my_split_index13,my_split_index12,my_split_index1};
		fill_info_array(nColinfo,pColinfoArr,heapNumberArr,nIndexColsArr,splitFuncArr);
		
		//创建index
		EntrySetID pIndexEntryID[nIndex];
		IndexEntrySet *pIndexEntry[nIndex];
		create_multi_index(pEntrySet,nIndex,pIndexEntryID,pColinfoArr);
		open_mutil_index(pEntrySet,nIndex,pIndexEntry,pIndexEntryID);
		command_counter_increment();

		vector<ScanCondition> keyVecArray[nIndex];
//		const int nIndexColsArr[nIndex] = {2,2,2,2,1};
		ScanCondition::CompareOperation cmp_op[nIndex][20] = {{GE,E},{LE,G},{L,E},{G,E},{L}};
		char keydata[nIndex][20][20] = { { "abc","2" },{"d_","8"},{"1","abc"},{"8","d_"},{"1"}};
		init_mutil_index_key(nIndex,nIndexColsArr,keyVecArray,cmp_op,keydata);

		IndexEntrySetScan *pIndexScan[nIndex];
		start_mutil_index_scan(nIndex,pIndexScan,pIndexEntry,keyVecArray);
		char cmpData[][20] = {"abcd_2","abcd_9","abcd_0","abcd_9","abcd_0"};
		const int dataRows = 5;
		if( false == checkResult(nIndex,pIndexScan,cmpData,dataRows) )
		{
			end_scan_and_close_entry(nIndex,pIndexScan,pIndexEntry,pEntrySet);
			user_abort_transaction();
			return false;
		}
		free_memory(insert_str,dvec);
		end_scan_and_close_entry(nIndex,pIndexScan,pIndexEntry,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_index_transaction_create_dll_002()
{
	INTENT("测试在一个事务内，创建单个索引，并给多个scankey是否正确");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(DATAROWS_INDEX,DATA_INDEX);
		vector<DataItem*> dvec;
		char copy_insert_data[DATAROWS_INDEX][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,DATAROWS_INDEX,DATA_INDEX);
		command_counter_increment();

		//在1,3列上建立索引
//		const int nIndex = 1; //创建单列

		const int nIndexCols = 1;
		int heap_col_number[nIndexCols] = {3};
		ColumnInfo *pcolinfo = alloc_index_colinfo(nIndexCols,heap_col_number,my_split_index1);
		
		IndexEntrySet *pIndexEntry = NULL;
		EntrySetID ieid = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE,set_colinfo(pcolinfo),0,NULL);
		pIndexEntry = pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,ieid,NULL);
		command_counter_increment();

		vector<ScanCondition> keyVec;
		initKeyVector(keyVec,1,ScanCondition::GreaterThan,"0",my_compare_str_index);
		initKeyVector(keyVec,1,ScanCondition::LessThan,"2",my_compare_str_index);

		IndexEntrySetScan *pIndexScan = (IndexEntrySetScan *)pIndexEntry->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,keyVec);
		char cmpData[][20] = {"abcd_1"};
		int dataRows = 1;
		if( false == check_single_scan_result(pIndexScan ,cmpData,dataRows) )
		{
			free_memory(insert_str,dvec);
			pIndexEntry->endEntrySetScan(pIndexScan);
			pStorageEngine->closeEntrySet(pTransaction,pIndexEntry);
			pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
			user_abort_transaction();
			return false;
		}
		free_memory(insert_str,dvec);
		pIndexEntry->endEntrySetScan(pIndexScan);
		pStorageEngine->closeEntrySet(pTransaction,pIndexEntry);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_index_transaction_create_dll_003()
{
	INTENT("测试在多个事务中，相同列上创建多个索引是否正确");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(DATAROWS_INDEX,DATA_INDEX);
		vector<DataItem*> dvec;
		char copy_insert_data[DATAROWS_INDEX][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,DATAROWS_INDEX,DATA_INDEX);
		close_entry_set(pEntrySet);
		commit_transaction();

		//在1,3列上建立索引
		get_new_transaction();
		pEntrySet = open_entry_set();
		const int nIndex = NINDEX;
		const int nIndexCols = 2;
		int heap_col_number[nIndexCols] = {1,3};
		ColumnInfo *pcolinfo = alloc_index_colinfo(nIndexCols,heap_col_number,my_split_index31);

		EntrySetID pIndexEntryID[nIndex+1];
		IndexEntrySet *pIndexEntry[nIndex+1];
		ColumnInfo *pColinfoArray[nIndex] = {pcolinfo,pcolinfo,pcolinfo,pcolinfo,pcolinfo};

		//创建5个index
		create_multi_index(pEntrySet,nIndex,pIndexEntryID,pColinfoArray);
		close_entry_set(pEntrySet);
		commit_transaction();
		
		//另一个事务，再创建1个index
		get_new_transaction();
		pEntrySet = open_entry_set();
		IndexEntrySet *pIndex = NULL;
		pIndexEntryID[nIndex] = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE,set_colinfo(pcolinfo),0,NULL);
		close_entry_set(pEntrySet);
		commit_transaction();
		
		get_new_transaction();
		pEntrySet = open_entry_set();
		pIndexEntry[nIndex] = pIndex;
//		IndexEntrySet *pIndexEntryArr[nIndex+1];

		open_mutil_index(pEntrySet,nIndex+1,pIndexEntry,pIndexEntryID);

		vector<ScanCondition> keyVecArray[nIndex+1];
		const int nIndexColsArr[nIndex+1] = {2,2,2,2,2,2};
		ScanCondition::CompareOperation cmp_op[nIndex+1][20] = {{GE,E},{LE,G},{E,L},{G,E},{L,E},{L,E}};
		char keydata[nIndex+1][20][20] = { { "abc","2" },{"abc","8"},{"abc","1"},{"abb","4"},{"abd","3"},{"abd","5"}};
		init_mutil_index_key(nIndex+1,nIndexColsArr,keyVecArray,cmp_op,keydata);

		IndexEntrySetScan *pIndexScan[nIndex+1];
		start_mutil_index_scan(nIndex+1,pIndexScan,pIndexEntry,keyVecArray);
		char cmpData[][20] = {"abcd_2","abcd_9","abcd_0","abcd_4","abcd_3","abcd_5"};
		const int dataRows = 6;
		if( false == checkResult(nIndex+1,pIndexScan,cmpData,dataRows) )
		{
			end_scan_and_close_entry(nIndex+1,pIndexScan,pIndexEntry,pEntrySet);
			user_abort_transaction();
			return false;
		}
		free_memory(insert_str,dvec);
		end_scan_and_close_entry(nIndex+1,pIndexScan,pIndexEntry,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_index_transaction_create_dll_004()
{
	INTENT("测试在多个事务中，先在相同列上创建多个索引，后插入数据，再新起事务创建索引是否正确");
	try 
	{
		//在1,3列上建立索引
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		const int nIndex = NINDEX;
		const int nIndexCols = 2;
		int heap_col_number[nIndexCols] = {1,3};
		ColumnInfo *pcolinfo = alloc_index_colinfo(nIndexCols,heap_col_number,my_split_index31);

		EntrySetID pIndexEntryID[nIndex+1];
		IndexEntrySet *pIndexEntry[nIndex+1];
		ColumnInfo *pColinfoArray[nIndex] = {pcolinfo,pcolinfo,pcolinfo,pcolinfo,pcolinfo};

		//创建5个index
		create_multi_index(pEntrySet,nIndex,pIndexEntryID,pColinfoArray);
		close_entry_set(pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		char *insert_str = formData(DATAROWS_INDEX,DATA_INDEX);
		vector<DataItem*> dvec;
		char copy_insert_data[DATAROWS_INDEX][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,DATAROWS_INDEX,DATA_INDEX);
		close_entry_set(pEntrySet);
		commit_transaction();

		//另一个事务，再创建1个index
		get_new_transaction();
		pEntrySet = open_entry_set();
		pIndexEntryID[nIndex] = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE,set_colinfo(pcolinfo),0,NULL);
		close_entry_set(pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
//		pIndexEntry[nIndex] = pIndex;
//		IndexEntrySet *pIndexEntryArr[nIndex+1];

		open_mutil_index(pEntrySet,nIndex+1,pIndexEntry,pIndexEntryID);
// 		for(int i = 0; i<nIndex ; ++i )
// 		{
// 			pIndexEntry[i] = pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,pIndexEntry[i]->getId(),NULL);
// 		}

		vector<ScanCondition> keyVecArray[nIndex+1];
		const int nIndexColsArr[nIndex+1] = {2,2,2,2,2,2};
		ScanCondition::CompareOperation cmp_op[nIndex+1][20] = {{GE,E},{LE,G},{E,L},{G,E},{L,E},{L,E}};
		char keydata[nIndex+1][20][20] = { { "abc","2" },{"abc","8"},{"abc","1"},{"abb","4"},{"abd","3"},{"abd","5"}};
		init_mutil_index_key(nIndex+1,nIndexColsArr,keyVecArray,cmp_op,keydata);

		IndexEntrySetScan *pIndexScan[nIndex+1];
		start_mutil_index_scan(nIndex+1,pIndexScan,pIndexEntry,keyVecArray);
		char cmpData[][20] = {"abcd_2","abcd_9","abcd_0","abcd_4","abcd_3","abcd_5"};
		const int dataRows = 6;
		if( false == checkResult(nIndex+1,pIndexScan,cmpData,dataRows) )
		{
			end_scan_and_close_entry(nIndex+1,pIndexScan,pIndexEntry,pEntrySet);
			user_abort_transaction();
			return false;
		}
		free_memory(insert_str,dvec);
		end_scan_and_close_entry(nIndex+1,pIndexScan,pIndexEntry,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}










