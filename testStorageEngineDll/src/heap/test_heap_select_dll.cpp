/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.9.23		 单列单scankey，仅向前扫描
001			许彦	  创建		  2011.9.23		 多列单scankey，仅向前扫描，使用大于、小于和等于策略
001			许彦	  创建		  2011.9.23		 单列多scankey，仅向前扫描，使用大于、小于和等于策略
001			许彦	  创建		  2011.9.23		 多列多scankey，仅向前扫描，使用大于、小于和等于策略


************************************************************************/

#include <iostream>

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "PGSETypes.h"
#include "heap/test_heap_select_dll.h"

using namespace FounderXDB::StorageEngineNS;

extern void insertData(EntrySet *pEntrySet,char *insert_str,vector<DataItem*> &dvec,char copy_insert_data[][20],uint32 nData,char *str);
extern void free_memory(char *insert_str,vector<DataItem*> dvec);

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

void my_spilt_select(RangeDatai &rangeData, char* str, int col)
{

	rangeData.len = 0;
	rangeData.start = 0;
	if(str == NULL)
	{
		return;
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
	return;
}

void initKeyVector(vector<ScanCondition> &keyVec,int col_number, ScanCondition::CompareOperation cmp_op,const char *keydata,int (*compare_func)(const char *, size_t , const char *, size_t))
{
	int key_len = strlen(keydata); 
	char *tKeydata = (char*)malloc(key_len+1);
	memcpy(tKeydata,keydata,key_len+1);
	ScanCondition key(col_number, cmp_op, (se_uint64)tKeydata, key_len, compare_func);
	keyVec.push_back(key);
}

void end_heap_scan_and_close_entry(EntrySet *pEntrySet,EntrySetScan *pHeapScan)
{
	pEntrySet->endEntrySetScan(pHeapScan);
	pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
}

bool check_result(EntrySetScan *pHeapScan ,char cmpData[][20],int dataRows)
{
	DataItem *pScanData = new DataItem;
	EntryID scan_tid;
	int counter = 0;
	int data_flag;
	int i = 0;
	while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
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

#define DATAROWS_SELECT 10
#define DATA_SELECT "abcd_"
bool test_heap_select_dll_000()
{
	INTENT("单scankey，仅向前扫描");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(DATAROWS_SELECT,DATA_SELECT);
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char copy_insert_data[DATAROWS_SELECT][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,DATAROWS_SELECT,DATA_SELECT);
		command_counter_increment();

		vector<ScanCondition> keyVec;
		initKeyVector(keyVec,3,ScanCondition::LessThan,"2",my_compare_select);

		EntrySetScan *pHeapScan = pEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,keyVec);
		char cmpData[][20] = {"abcd_0","abcd_1"};
		int dataRows = 2;
		if(false == check_result(pHeapScan,cmpData,dataRows))
		{
			end_heap_scan_and_close_entry(pEntrySet,pHeapScan);
			user_abort_transaction();
			return false;
		}		
		end_heap_scan_and_close_entry(pEntrySet,pHeapScan);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_heap_select_dll_001()
{
	INTENT("多列单scankey，仅向前扫描，使用大于、小于和等于策略");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(DATAROWS_SELECT,DATA_SELECT);
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char copy_insert_data[DATAROWS_SELECT][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,DATAROWS_SELECT,DATA_SELECT);

		DataItem *pInsertData = new DataItem;
		int len = sizeof("bbcd_9");
		char *pinsert_str = (char *)malloc(len);
		memcpy(pinsert_str, "bbcd_9", len);
		pInsertData->setData((void *)pinsert_str);
		pInsertData->setSize(len);

		EntryID insert_tid;
		pEntrySet->insertEntry(pTransaction, insert_tid, *pInsertData);//data为const dataitem类型，输入参数
		command_counter_increment();

		vector<ScanCondition> keyVec;
		initKeyVector(keyVec,1,ScanCondition::LessThan,"bbb",my_compare_select);
		initKeyVector(keyVec,2,ScanCondition::Equal,"d_",my_compare_select);
		initKeyVector(keyVec,3,ScanCondition::GreaterThan,"7",my_compare_select);

		EntrySetScan *pHeapScan = pEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,keyVec);
		char cmpData[][20] = {"abcd_8","abcd_9"};
		int dataRows = 2;
		if(false == check_result(pHeapScan,cmpData,dataRows))
		{
			end_heap_scan_and_close_entry(pEntrySet,pHeapScan);
			user_abort_transaction();
			return false;
		}		
		end_heap_scan_and_close_entry(pEntrySet,pHeapScan);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_heap_select_dll_002()
{
	INTENT("单列多scankey，仅向前扫描");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(DATAROWS_SELECT,DATA_SELECT);
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char copy_insert_data[DATAROWS_SELECT][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,DATAROWS_SELECT,DATA_SELECT);
		command_counter_increment();

		vector<ScanCondition> keyVec;
		initKeyVector(keyVec,3,ScanCondition::GreaterThan,"0",my_compare_select);
		initKeyVector(keyVec,3,ScanCondition::LessThan,"3",my_compare_select);

		EntrySetScan *pHeapScan = pEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,keyVec);
		char cmpData[][20] = {"abcd_1","abcd_2"};
		int dataRows = 2;
		if(false == check_result(pHeapScan,cmpData,dataRows))
		{
			end_heap_scan_and_close_entry(pEntrySet,pHeapScan);
			user_abort_transaction();
			return false;
		}		
		end_heap_scan_and_close_entry(pEntrySet,pHeapScan);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_heap_select_dll_003()
{
	INTENT("多列多scankey，仅向前扫描，使用大于、小于和等于策略");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(DATAROWS_SELECT,DATA_SELECT);
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char copy_insert_data[DATAROWS_SELECT][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,DATAROWS_SELECT,DATA_SELECT);

		DataItem *pInsertData = new DataItem;
		int len = sizeof("bbcd_8");
		char *pinsert_str = (char *)malloc(len);
		memcpy(pinsert_str, "bbcd_8", len);
		pInsertData->setData((void *)pinsert_str);
		pInsertData->setSize(len);

		EntryID insert_tid;
		pEntrySet->insertEntry(pTransaction, insert_tid, *pInsertData);//data为const dataitem类型，输入参数
		command_counter_increment();

		vector<ScanCondition> keyVec;
		initKeyVector(keyVec,1,ScanCondition::LessThan,"bbd",my_compare_select);
		initKeyVector(keyVec,1,ScanCondition::GreaterThan,"abb",my_compare_select);
		initKeyVector(keyVec,2,ScanCondition::Equal,"d_",my_compare_select);
		initKeyVector(keyVec,3,ScanCondition::GreaterThan,"5",my_compare_select);
		initKeyVector(keyVec,3,ScanCondition::LessThan,"9",my_compare_select);
		
		EntrySetScan *pHeapScan = pEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,keyVec);
		char cmpData[][20] = {"abcd_6","abcd_7","abcd_8","bbcd_8"};
		int dataRows = 4;
		if(false == check_result(pHeapScan,cmpData,dataRows))
		{
			end_heap_scan_and_close_entry(pEntrySet,pHeapScan);
			user_abort_transaction();
			return false;
		}		
		end_heap_scan_and_close_entry(pEntrySet,pHeapScan);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

extern void insert_data(char data[][200], 
												const int data_len,
												EntrySet **pEntrySet,
												Transaction *transaction = pTransaction);

bool test_heap_mark_restore()
{
	INTENT("测试entry set的mark/restore position功能函数是否正确工作。");

	bool return_sta = true;

	try
	{
		get_new_transaction();
		const uint32 INSERT_ROW = 100;
		EntrySet *pEntrySet = open_entry_set();
		/* 构造测试数据 */
		DataGenerater dg(INSERT_ROW, DATA_LEN);
		dg.dataGenerate();
		char copy_insert_data[INSERT_ROW][DATA_LEN];
		dg.dataToDataArray2D(copy_insert_data);
		insert_data(copy_insert_data, ARRAY_LEN_CALC(copy_insert_data), &pEntrySet);
		pStorageEngine->endStatement();

		/* 开始扫描表，使用mark做记号并使用restore读取记号 */
		std::vector<ScanCondition> v_sc;
		EntrySetScan *ess = pEntrySet->startEntrySetScan(pTransaction, EntrySet::SnapshotNOW, v_sc);
		/* 插入INSERT_ROW行数据，这里扫描的时候在最后MARK_ROW行做标记 */
		const uint32 MARK_ROW = 23;
		int index_pos = INSERT_ROW - MARK_ROW;
		DataItem di;
		EntryID ei;
		vector<string> v_cmp1, v_cmp2;
		while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
		{
			--index_pos;
			if(index_pos == 0)
			{
				ess->markPosition();
			}
			if(index_pos < 0)
			{
				v_cmp1.push_back((char*)di.getData());
			}
		}
		/* 读取标记重新扫描 */
		ess->restorePosition();
		while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
		{
			v_cmp2.push_back((char*)di.getData());
		}
		pEntrySet->endEntrySetScan(ess);
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		commit_transaction();

		sort(v_cmp1.begin(), v_cmp1.end());
		sort(v_cmp2.begin(), v_cmp2.end());

		return_sta = (v_cmp1 == v_cmp2) ? true : false;
	}catch(StorageEngineException &se)
	{
		user_abort_transaction();
		printf("%d : %s\n", se.getErrorNo(), se.getErrorMsg());
		return_sta = false;
	}

	return return_sta;
}