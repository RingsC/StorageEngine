/**
*@file	:test_transaction_isolation_dll.cpp
*@brief 	:test transaction isolation level
*@author	:WangHao
*@date	:2012-4-5
*/

#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <boost/assign.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include "boost/thread.hpp" 
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "sequence/utils.h"
#include "transaction/test_transaction_isolation_dll.h"

using namespace boost;
using namespace std;
using namespace FounderXDB::StorageEngineNS;
using boost::detail::spinlock;
extern StorageEngine *pStorageEngine;

#define TRANSACTION_ROWS 10 
#define THREAD_NUM_3 3
#define testdate3 		"testdate_3"
#define testdata3 		"testdata_3"
#define Repeattestdate3 "Repeattestdate_3"
#define Repeattestdata3 "Repeattestdata_3"
#define Serialtestdate3 "Serialtestdate_3"
#define Serialtestdata3 "Serialtestdata_3"


static EntrySetID heap_id = 0;
static int CommitUpdateFlag = 0;
static int UpdatedFlag = 0;
static int WaitUpdateFlag = 0;
static spinlock ReadsWaitLock;
static spinlock ReadsUpdateLock;
static spinlock ReadsCommitLock;


static EntrySetID Rheap_id = 0;
static int RepeatCommitUpdateFlag = 0;
static int RepeatUpdatedFlag = 0;
static StorageEngine *pReStorageEngine = NULL;
static int ReWaitUpdateFlag = 0;
static spinlock RepeatWaitLock;
static spinlock RepeatUpdateLock;
static spinlock RepeatCommitLock;


static EntrySetID Sheap_id = 0;
static int SerialCommitUpdateFlag = 0;
static int SerialUpdatedFlag = 0;
static StorageEngine *pSeStorageEngine = NULL;
static int SeWaitUpdateFlag = 0;
static spinlock SerialWaitLock;
static spinlock SerialUpdateLock;
static spinlock SerialCommitLock;

#define SearchData(num,key_len,str) \
do{\
vector<ScanCondition> keyVec##num;\
key_len = strlen(#str);\
char *tKeydata##num = (char*)malloc(key_len+1);\
memcpy(tKeydata##num,#str,key_len+1);\
ScanCondition key##num(1, ScanCondition::Equal, (se_uint64)tKeydata##num, key_len, str_compare_select);\
keyVec##num.push_back(key##num);\
pHeapScan = heap_entry->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,keyVec##num);\
}while(0)

#define SearchUpdateData(strData) \
do{\
while(flag==0)\
{\
flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);\
if (flag==NO_DATA_FOUND)\
{\
	cout<<"查询至最后的数据!\n"<<endl;\
	break;\
}\
str=(char*)getdata.getData();\
int cmp;\
cmp=memcmp(str,#strData,sizeof(#strData));\
if(cmp == 0)\
{\
	status = 1;\
	break;\
}\
}\
}\
while(0)

#define UpdateData(strData) \
do{\
DataItem data;\
char *pstr = (char *)malloc(len);\
memcpy(pstr, #strData, len);\
data.setData((void *)pstr);\
data.setSize(len);\
heap_entry->updateEntry(pTransaction,tid,data);\
free(pstr);\
cout<<"数据已更新!"<<endl;\
}while(0)

#define InsertData(str)\
do{\
int len = sizeof(#str);\
char *insert_str = (char*)std::malloc(len+num);\
for (ix = 0; ix != TRANSACTION_ROWS; ++ix)\
{\
	memcpy(insert_str,#str, sizeof(#str));\
	my_itoa(ix,string,10);\
	strcat(insert_str,string);\
	memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);\
	DataItem *data = new DataItem;\
	data->setData((void*)copy_insert_data[ix]);\
	len = strlen((char*)insert_str);\
	data->setSize(len+1);\
	dvec.push_back(data);\
	heap_entry->insertEntry(pTransaction, insert_tid, *dvec[ix]);\
}\
std::free(insert_str);\
}while(0)

#define CatchExcepation \
catch(StorageEngineException &ex)\
{\
	std::cout << ex.getErrorMsg() << endl;\
	if (pTransaction != NULL)\
	{\
		pTransaction->abort();\
	}\
	*i = 0;\
}\
delete pTransaction;

extern char *my_itoa(int value, char *string, int radix);
extern void Sleep(int secs);

int str_compare_select(const char *str1, size_t len1, const char *str2, size_t len2)
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


void check_result_data(EntrySetScan *pHeapScan ,char* cmpData,int& counter)
{
	DataItem *pScanData = new DataItem;
	EntryID scan_tid;
	int data_flag;
	pHeapScan->restorePosition();
	while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
	{		
		data_flag = memcmp((char*)pScanData->getData(),cmpData,pScanData->getSize());
		CHECK_BOOL(data_flag == 0);
		if(data_flag == 0)
		{
			counter++;	
		}
	}
	delete pScanData;
}


bool TestTransactionReadIsolation(void)
{
    INTENT("读已提交事务隔离级别的测试\n");

	//SpliterGenerater sg;
	//ColumnInfo *colinfo = sg.buildHeapColInfo(1, 10);
    //setColumnInfo(1, colinfo);
    int sta[THREAD_NUM_3] = {0};
    int found = 0;
	EntrySetCollInfo<10>::get();

    thread_group tg;
    tg.create_thread(bind(&test_reads_heap_insert_data,&sta[0]));
	tg.join_all();

    if(sta[0]) 
	{
       ++found; 
    }
    
    tg.create_thread(bind(&test_reads_heap_update_data,&sta[1]));
    tg.create_thread(bind(&test_reads_heap_select_data,&sta[2]));
	tg.join_all();
	
	if(sta[1]) 
	{
       ++found; 
    }
	if(sta[2]) 
	{
       ++found; 
    }
	
    if (found == 3) 
	{
        return true;
    }
    return false;     
}


void test_reads_heap_insert_data(int *i)
{
    Transaction* pTransaction = NULL;
    try
	{
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        heap_id = pStorageEngine->createEntrySet(pTransaction,EntrySetCollInfo<10>::get());
		EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id));
		
		int num = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int j = TRANSACTION_ROWS; j!=0; j/=10)
		{
			++num;
		}
		
		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		//插入的每个数据都具有唯一性
		InsertData(testdata_);
      	for(ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			delete dvec[ix];
		}
		pStorageEngine->closeEntrySet(pTransaction,heap_entry);
        pTransaction->commit();
		*i = 1;
    }
	CatchExcepation

    pStorageEngine->endThread();     
}


void test_reads_heap_select_data(int *i)
{
	INTENT("事务隔离级别测试时数据查询\n");
	
    Transaction* pTransaction = NULL;
	//0000111
	//第一位为1时已脏读
	//第二位为1时不可重复读
	//第三位为1时已幻读
	char a = 0;
    try 
	{
        pStorageEngine->beginThread();
		EntrySetScan* pHeapScan = NULL;
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *heap_entry=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,heap_id));//打开表

		int key_len = 0;
		SearchData(0,key_len,testdata_3);
		
		char cmpData[20] = {testdata3};
		int count0 = 0;
		check_result_data(pHeapScan,cmpData,count0);
		
		heap_entry->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,heap_entry);

		ReadsWaitLock.lock();
		WaitUpdateFlag = 1;
		ReadsWaitLock.unlock();
		while(1)
		{
			Sleep(1);
			ReadsUpdateLock.lock();
			if (1 == UpdatedFlag)
			{
				UpdatedFlag = 0;
				ReadsUpdateLock.unlock();
				break;
			}
			ReadsUpdateLock.unlock();
		}
		
		heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,heap_id));//打开表
		SearchData(1,key_len,testdata_3);
		int count1 = 0;
		//插入的数据具有唯一性，cout为0时数据被改变
		check_result_data(pHeapScan,cmpData,count1);

		ReadsCommitLock.lock();
		if (0==CommitUpdateFlag &&(0==count1 && count0!=count1))
		{
			cout<<"数据已脏读"<<endl;
			//READ_COMMITTED_ISOLATION_LEVEL 脏读，测试失败
			a=a&0x1;
		}
		ReadsCommitLock.unlock();

		ReadsCommitLock.lock();
		if (1==CommitUpdateFlag && (0==count1 && count0!=count1))
		{
			cout<<"数据不可重复读"<<endl;
		}
		ReadsCommitLock.unlock();

		ReadsCommitLock.lock();
		//因为在存储引擎层，幻读测试是不准确的
		if (1==CommitUpdateFlag && count0!=count1)
		{
			cout<<"数据可幻读"<<endl;
		}
		ReadsCommitLock.unlock();
		
		heap_entry->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,heap_entry);
		pTransaction->commit();
		cout<<"数据查询的事务已Commit!\n"<<endl;
		if ((a&0x1)==0x1)
		{
			*i = 0;
		}
		else
		{
			*i = 1;
		}
    } 
	CatchExcepation

    pStorageEngine->endThread(); 

}


void test_reads_heap_update_data(int *i)
{
	INTENT("事务隔离级别测试时数据更新\n");

    Transaction* pTransaction = NULL;
    try 
	{
		while(1)
		{
			Sleep(1);
			ReadsWaitLock.lock();
			if (1 == WaitUpdateFlag)
			{
				WaitUpdateFlag = 0;
				ReadsWaitLock.unlock();
				break;
			}
			ReadsWaitLock.unlock();
		}
		
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *heap_entry=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,heap_id));//打开表
		EntrySetScan *entry_scan = heap_entry->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//开始扫描

		EntryID tid;//用于接收getNext得到的结构体，这个结构体中存放了元组的位置和偏移量
		int flag=0;
		int status = 0;
		int len = sizeof(testdate3);
		char *str = (char *)malloc(len);
		DataItem getdata;
		SearchUpdateData(testdata_3);
		
		if (1 == status)
		{
			UpdateData(testdate_3);
			ReadsUpdateLock.lock();
			UpdatedFlag = 1;
			ReadsUpdateLock.unlock();
		}
		//free(str);
		heap_entry->endEntrySetScan(entry_scan);
		pStorageEngine->closeEntrySet(pTransaction,heap_entry);
		pTransaction->commit();

		ReadsCommitLock.lock();
		CommitUpdateFlag = 1;
		ReadsCommitLock.unlock();
		
		cout<<"数据更新的事务已Commit!"<<endl;
		*i = 1;

    } 
	CatchExcepation
    pStorageEngine->endThread(); 

}

bool TestTransactionRepeatableIsolation(void)
{
    INTENT("可重复读事务隔离级别的测试\n");

    //form_heap_colinfo(heap_colinfo);
    //setColumnInfo(1, &heap_colinfo);
    EntrySetCollInfo<16>::get();
    int sta[THREAD_NUM_3] = {0};
    int found = 0;
	
	pReStorageEngine = StorageEngine::getStorageEngine();
	
    thread_group tg;
    tg.create_thread(bind(&test_Repeatable_heap_insert_data,&sta[0]));
	tg.join_all();
    if(sta[0]) 
	{
       ++found; 
    }
    
    tg.create_thread(bind(&test_Repeatable_heap_update_data,&sta[1]));
    tg.create_thread(bind(&test_Repeatable_heap_select_data,&sta[2]));
	tg.join_all();
	
	if(sta[1]) 
	{
       ++found; 
    }
	if(sta[2]) 
	{
       ++found; 
    }
    
    if (found == 3) 
	{
        return true;
    }
    return false;     
}

void test_Repeatable_heap_insert_data(int *i)
{
    Transaction* pTransaction = NULL;
    try
	{
        pReStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::REPEATABLE_READ_ISOLATION_LEVEL);
        Rheap_id = pReStorageEngine->createEntrySet(pTransaction,EntrySetCollInfo<16>::get());
		EntrySet *heap_entry = static_cast<EntrySet *>(pReStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,Rheap_id));
		
		int num = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int j = TRANSACTION_ROWS; j!=0; j/=10)
		{
			++num;
		}

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		//插入的每个数据都具有唯一性
		InsertData(Repeattestdata_);
      	for(ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			delete dvec[ix];
		}
        pTransaction->commit();
		*i = 1;
    }
	CatchExcepation
		
    pReStorageEngine->endThread();  

}

void test_Repeatable_heap_select_data(int *i)
{
	INTENT("事务隔离级别测试时数据查询\n");
	
    Transaction* pTransaction = NULL;
	//0000111
	//第一位为1时已脏读
	//第二位为1时不可重复读
	//第三位为1时已幻读
	char a = 0;
    try 
	{
        pReStorageEngine->beginThread();
		EntrySetScan* pHeapScan = NULL;
        pTransaction = start_new_transaction(Transaction::REPEATABLE_READ_ISOLATION_LEVEL);
		EntrySet *heap_entry=static_cast<EntrySet *>(pReStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,Rheap_id));//打开表
		
		int key_len = 0;
		SearchData(0,key_len,Repeattestdata_3);
		
		char cmpData[20] = {Repeattestdata3};
		int count0 = 0;
		check_result_data(pHeapScan,cmpData,count0);
		
		heap_entry->endEntrySetScan(pHeapScan);
		pReStorageEngine->closeEntrySet(pTransaction,heap_entry);

		RepeatWaitLock.lock();
		ReWaitUpdateFlag = 1;
		RepeatWaitLock.unlock();
		while(1)
		{
			Sleep(1);
			RepeatUpdateLock.lock();
			if (1 == RepeatUpdatedFlag)
			{
				RepeatUpdatedFlag = 0;
				RepeatUpdateLock.unlock();
				break;
			}
			RepeatUpdateLock.unlock();
		}
		//RepeatLock.unlock();

		heap_entry=static_cast<EntrySet *>(pReStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,Rheap_id));//打开表
		SearchData(1,key_len,Repeattestdata_3);
		int count1 = 0;
		check_result_data(pHeapScan,cmpData,count1);
		RepeatCommitLock.lock();
		if (0==RepeatCommitUpdateFlag && (0==count1 && count0!=count1))
		{
			cout<<"数据已脏读"<<endl;
			//REPEATABLE_READ_ISOLATION_LEVEL 脏读，测试失败
			a=a&0x1;
		}
		RepeatCommitLock.unlock();
		
		RepeatCommitLock.lock();
		if (1==RepeatCommitUpdateFlag && (0==count1 && count0!=count1))
		{
			cout<<"数据不可重复读"<<endl;
			//REPEATABLE_READ_ISOLATION_LEVEL 不可重复读，测试失败
			a=a&0x2;
		}
		RepeatCommitLock.unlock();

		RepeatCommitLock.lock();
		//因为在存储引擎层，幻读测试是不准确的
		if (1==RepeatCommitUpdateFlag && count0!=count1)
		{
			cout<<"数据可幻读"<<endl;
		}
		RepeatCommitLock.unlock();
		
		heap_entry->endEntrySetScan(pHeapScan);
		pReStorageEngine->closeEntrySet(pTransaction,heap_entry);
		pTransaction->commit();
		cout<<"数据查询的事务已Commit!\n"<<endl;
		if (a > 0x0)
		{
			*i = 0;
		}
		else
		{
			*i = 1;
		}
    } 
	CatchExcepation
		
    pReStorageEngine->endThread(); 

}
void test_Repeatable_heap_update_data(int *i)
{
	INTENT("事务隔离级别测试时数据更新\n");
    Transaction* pTransaction = NULL;
    try 
	{
		while(1)
		{
			Sleep(1);
			RepeatWaitLock.lock();
			if (1 == ReWaitUpdateFlag)
			{
				ReWaitUpdateFlag = 0;
				RepeatWaitLock.unlock();
				break;
			}
			RepeatWaitLock.unlock();
		}
		
        pReStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::REPEATABLE_READ_ISOLATION_LEVEL);
		EntrySet *heap_entry=static_cast<EntrySet *>(pReStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,Rheap_id));//打开表
		EntrySetScan *entry_scan = heap_entry->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//开始扫描

		EntryID tid;//用于接收getNext得到的结构体，这个结构体中存放了元组的位置和偏移量
		int flag=0;
		int status = 0;
		int len = sizeof(Repeattestdate3);
		char *str = (char *)malloc(len);
		DataItem getdata;
		SearchUpdateData(Repeattestdata_3);
		if (1 == status)
		{
			UpdateData(Repeattestdate_3);
			RepeatUpdateLock.lock();
			RepeatUpdatedFlag = 1;
			RepeatUpdateLock.unlock();
		}
		//free(str);
		heap_entry->endEntrySetScan(entry_scan);
		pReStorageEngine->closeEntrySet(pTransaction,heap_entry);
		pTransaction->commit();
		RepeatCommitLock.lock();
		RepeatCommitUpdateFlag = 1;
		RepeatCommitLock.unlock();
		cout<<"数据更新的事务已Commit!"<<endl;
		*i = 1;

    } 
	CatchExcepation
		
    pReStorageEngine->endThread(); 

}

bool TestTransactionSerializableIsolation(void)
{
    INTENT("可串行化事务隔离级别的测试\n");

    //form_heap_colinfo(heap_colinfo);
    //setColumnInfo(1, &heap_colinfo);
    EntrySetCollInfo<16>::get();
    int sta[THREAD_NUM_3] = {0};
    int found = 0;
	
	pSeStorageEngine = StorageEngine::getStorageEngine();
	
    thread_group tg;
    tg.create_thread(bind(&test_Serializable_heap_insert_data,&sta[0]));
	tg.join_all();
    if(sta[0]) 
	{
       ++found; 
    }
    
    tg.create_thread(bind(&test_Serializable_heap_update_data,&sta[1]));
    tg.create_thread(bind(&test_Serializable_heap_select_data,&sta[2]));
	tg.join_all();
	
	if(sta[1]) 
	{
       ++found; 
    }
    if(sta[2]) 
	{
       ++found; 
    }
	
    if (found == 3) 
	{
        return true;
    }
    return false;     

}

void test_Serializable_heap_insert_data(int *i)
{
    Transaction* pTransaction = NULL;
    try
	{
        pSeStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::SERIALIZABLE_ISOLATION_LEVEL);
        Sheap_id = pSeStorageEngine->createEntrySet(pTransaction,EntrySetCollInfo<16>::get());
		EntrySet *heap_entry = static_cast<EntrySet *>(pSeStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,Sheap_id));
		
		int num = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int j = TRANSACTION_ROWS; j!=0; j/=10)
		{
			++num;
		}
		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		//插入的每个数据都具有唯一性
		InsertData(Serialtestdata_);
      	for(ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			delete dvec[ix];
		}
        pTransaction->commit();
		*i = 1;
    }
	CatchExcepation
		
    pSeStorageEngine->endThread();  

}

void test_Serializable_heap_select_data(int *i)
{
	INTENT("事务隔离级别测试时数据查询\n");
	
    Transaction* pTransaction = NULL;
	//0000111
	//第一位为1时已脏读
	//第二位为1时不可重复读
	//第三位为1时已幻读
	char a = 0;
    try 
	{
        pSeStorageEngine->beginThread();
		EntrySetScan* pHeapScan = NULL;
        pTransaction = start_new_transaction(Transaction::SERIALIZABLE_ISOLATION_LEVEL);
		EntrySet *heap_entry=static_cast<EntrySet *>(pSeStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,Sheap_id));//打开表

		int key_len = 0;
		SearchData(0,key_len,Serialtestdata_3);
		
		char cmpData[20] = {Serialtestdata3};
		int count = 0;
		check_result_data(pHeapScan,cmpData,count);
		heap_entry->endEntrySetScan(pHeapScan);
		pSeStorageEngine->closeEntrySet(pTransaction,heap_entry);

		SerialWaitLock.lock();
		SeWaitUpdateFlag = 1;
		SerialWaitLock.unlock();
		while(1)
		{
			Sleep(1);
			SerialUpdateLock.lock();
			if (1 == SerialUpdatedFlag)
			{
				SerialUpdatedFlag = 0;
				SerialUpdateLock.unlock();
				break;
			}
			SerialUpdateLock.unlock();
		}
		

		heap_entry=static_cast<EntrySet *>(pSeStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,Sheap_id));//打开表
		SearchData(1,key_len,Serialtestdata_3);
		
		int count1 = 0;
		check_result_data(pHeapScan,cmpData,count1);
		SerialCommitLock.lock();
		if (0==SerialCommitUpdateFlag && (0==count1 && count1!=count))
		{
			cout<<"数据已脏读"<<endl;
			//SERIALIZABLE_ISOLATION_LEVEL 脏读，测试失败
			a=a&0x1;
		}
		SerialCommitLock.unlock();

		SerialCommitLock.lock();
		if (1==SerialCommitUpdateFlag && (0==count1 && count1!=count))
		{
			cout<<"数据不可重复读"<<endl;
			//SERIALIZABLE_ISOLATION_LEVEL 不可重复读，测试失败
			a=a&0x2;
		}
		SerialCommitLock.unlock();

		SerialCommitLock.lock();
		//因为在存储引擎层，幻读测试是不准确的
		if (1==SerialCommitUpdateFlag && count!=count1)
		{
			cout<<"数据可幻读"<<endl;
			//SERIALIZABLE_ISOLATION_LEVEL幻读，测试失败
			a=a&0x4;
		}
		SerialCommitLock.unlock();
		
		heap_entry->endEntrySetScan(pHeapScan);
		pSeStorageEngine->closeEntrySet(pTransaction,heap_entry);
		pTransaction->commit();
		cout<<"数据查询的事务已Commit!\n"<<endl;
		if (a > 0x0)
		{
			*i = 0;
		}
		else
		{
			*i = 1;
		}
    } 
	CatchExcepation
		
    pSeStorageEngine->endThread(); 

}
void test_Serializable_heap_update_data(int *i)
{
	INTENT("事务隔离级别测试时数据更新\n");
    Transaction* pTransaction = NULL;
    try 
	{
		while(1)
		{
			Sleep(1);
			SerialWaitLock.lock();
			if (1 == SeWaitUpdateFlag)
			{
				SeWaitUpdateFlag = 0;
				SerialWaitLock.unlock();
				break;
			}
			SerialWaitLock.unlock();
		}
		
        pSeStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::SERIALIZABLE_ISOLATION_LEVEL);
		EntrySet *heap_entry=static_cast<EntrySet *>(pSeStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,Sheap_id));//打开表
		EntrySetScan *entry_scan = heap_entry->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//开始扫描

		EntryID tid;//用于接收getNext得到的结构体，这个结构体中存放了元组的位置和偏移量
		int flag=0;
		int status = 0;
		int len = sizeof(Serialtestdate3);
		char *str = (char *)malloc(len);
		DataItem getdata;
		SearchUpdateData(Serialtestdata_3);
		
		if (1 == status)
		{
			UpdateData(Serialtestdate_3);
			SerialUpdateLock.lock();
			SerialUpdatedFlag = 1;
			SerialUpdateLock.unlock();
		}
		//free(str);
		heap_entry->endEntrySetScan(entry_scan);
		pSeStorageEngine->closeEntrySet(pTransaction,heap_entry);
		pTransaction->commit();
		SerialCommitLock.lock();
		SerialCommitUpdateFlag = 1;
		SerialCommitLock.unlock();
		cout<<"数据更新的事务已Commit!"<<endl;
		*i = 1;

    } 
	CatchExcepation
		
    pSeStorageEngine->endThread(); 

}

