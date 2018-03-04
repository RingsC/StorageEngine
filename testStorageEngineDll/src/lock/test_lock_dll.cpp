/**
*@file	:test_lock_dll.cpp
*@brief 	:test lock
*@author	:WangHao
*@date	:2012-4-27
*/

#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <boost/assign.hpp>
#include "boost/thread.hpp" 
#include "PGSETypes.h"
#include "EntrySet.h"
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "lock/test_lock_dll.h"


using boost::detail::spinlock;
using namespace std;
using namespace FounderXDB::StorageEngineNS;


#define THREAD_NUM_3 3
#define TRANSACTION_ROWS 5 

extern StorageEngine *pStorageEngine;
static EntrySetID Entryid = 0;
static int testmutexdata = 10;
//赋值一个常量，检查在共享过程中是否改变
static int testmsharedata = 20;
static int testspindata = 10;



extern void Sleep(int secs);

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
	std::cout << ex.getErrorMsg() << std::endl;\
	if (pTransaction != NULL)\
	{\
		pTransaction->abort();\
	}\
	*i = 0;\
}\
delete pTransaction;

extern char *my_itoa(int value, char *string, int radix);

bool TestSpinLock(void)
{
	int sta[4] = {0};
	int found = 0;

	boost::thread_group tg;
	Transaction * pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	XdbLock* testmutex = XdbLock::createLock(XdbLock::SpinLock,pTransaction->getAssociatedMemoryContext());
	tg.create_thread(boost::bind(&test_Spin_lock1,&sta[0],testmutex));
	tg.create_thread(boost::bind(&test_Spin_lock2,&sta[1],testmutex));
	tg.create_thread(boost::bind(&test_Spin_lock3,&sta[2],testmutex));
	tg.create_thread(boost::bind(&test_Spin_lock4,&sta[3],testmutex));
	tg.join_all();

	if (sta[0])
	{
		++found;
	}
	if (sta[1])
	{
		++found;
	}
	if (sta[2])
	{
		++found;
	}
	if (sta[3])
	{
		++found;
	}
	delete testmutex;
	pTransaction->abort();
	if (4 == found)
		return true;
	return false;
}

void test_Spin_lock1(int * i ,XdbLock* testlock)
{
	INTENT("mutex lock \n");
	try
	{
		testlock->lock(XdbLock::Exclusive);
		testspindata = 10;
		testspindata = testspindata+5;
		testspindata = testspindata-5;
		if (10 == testspindata)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}


void test_Spin_lock2(int * i ,XdbLock* testlock)
{
	INTENT("mutex lock data modify \n");
	try
	{
		testlock->lock(XdbLock::Exclusive);
		testspindata = 10;
		testspindata = testspindata*5;
		testspindata = testspindata/5;
		if (10 == testspindata)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

void test_Spin_lock3(int * i ,XdbLock* testlock)
{
	INTENT("mutex lock data modify \n");
	try
	{
		testlock->lock(XdbLock::Exclusive);
		testspindata = 20;
		testspindata = testspindata-8;
		testspindata = testspindata+8;
		if (20 == testspindata)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

void test_Spin_lock4(int * i ,XdbLock* testlock)
{
	INTENT("mutex lock data modify \n");
	try
	{
		testlock->lock(XdbLock::Exclusive);
		testspindata = 10;
		testspindata = testspindata/5;
		testspindata = testspindata*5;
		if (10 == testspindata)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

bool TestMutexLockExclusive(void)
{
    INTENT("MutexLockExclusive的测试\n");
	int sta[4] = {0};
	int found = 0;

	boost::thread_group tg;
	Transaction * pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	XdbLock* testmutex = XdbLock::createLock(XdbLock::Mutex,pTransaction->getAssociatedMemoryContext());
	tg.create_thread(boost::bind(&test_mutex_lock_exclusive1,&sta[0],testmutex));
	tg.create_thread(boost::bind(&test_mutex_lock_exclusive2,&sta[1],testmutex));
	tg.create_thread(boost::bind(&test_mutex_lock_exclusive3,&sta[2],testmutex));
	tg.create_thread(boost::bind(&test_mutex_lock_exclusive4,&sta[3],testmutex));
	tg.join_all();

	if (sta[0])
	{
		++found;
	}
	if (sta[1])
	{
		++found;
	}
	if (sta[2])
	{
		++found;
	}
	if (sta[3])
	{
		++found;
	}
	delete testmutex;
	pTransaction->abort();
	if (4 == found)
		return true;
	return false;
     
}

void test_mutex_lock_exclusive1(int* i ,XdbLock* testlock)
{
	INTENT("mutex lock exclusive1\n");
	try
	{
		testlock->lock(XdbLock::Exclusive);
		testmutexdata = 10;
		testmutexdata = testmutexdata+1;
		testmutexdata = testmutexdata-1;
		if (10 == testmutexdata)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}


void test_mutex_lock_exclusive2(int* i,XdbLock* testlock)
{
	INTENT("mutex lock exclusive2\n");
	try
	{
		testlock->lock(XdbLock::Exclusive);
		testmutexdata = 10;
		testmutexdata = testmutexdata*4;
		testmutexdata = testmutexdata/4;
		if (10 == testmutexdata)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

void test_mutex_lock_exclusive3(int* i,XdbLock* testlock)
{
	INTENT("mutex lock exclusive2\n");
	try
	{
		testlock->lock(XdbLock::Exclusive);
		testmutexdata = 15;
		testmutexdata = testmutexdata-6;
		testmutexdata = testmutexdata+6;
		if (15 == testmutexdata)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

void test_mutex_lock_exclusive4(int* i,XdbLock* testlock)
{
	INTENT("mutex lock exclusive2\n");
	try
	{
		testlock->lock(XdbLock::Exclusive);
		testmutexdata = 15;
		testmutexdata = testmutexdata/5;
		testmutexdata = testmutexdata*5;
		if (15 == testmutexdata)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

bool TestMutexLockShare(void)
{
	INTENT("MutexLockShare的测试\n");
	int sta[4] = {0};
	int found = 0;

	boost::thread_group tg;
	Transaction * pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	XdbLock* testmutex = XdbLock::createLock(XdbLock::Mutex,pTransaction->getAssociatedMemoryContext());
	tg.create_thread(boost::bind(&test_mutex_lock_share1,&sta[0],testmutex));
	tg.create_thread(boost::bind(&test_mutex_lock_share2,&sta[1],testmutex));
	tg.create_thread(boost::bind(&test_mutex_lock_share3,&sta[2],testmutex));
	tg.create_thread(boost::bind(&test_mutex_lock_share4,&sta[3],testmutex));
	tg.join_all();

	if (sta[0])
	{
		++found;
	}
	if (sta[1])
	{
		++found;
	}
	if (sta[2])
	{
		++found;
	}
	if (sta[3])
	{
		++found;
	}
	delete testmutex;
	pTransaction->abort();
	if (4 == found)
		return true;
	return false;
}

void test_mutex_lock_share1(int* i,XdbLock* testlock)
{
	INTENT("mutex lock share1");
	try
	{
		int value;
		testlock->lock(XdbLock::Shared);
		value = testmsharedata;
		value++;
		if (testmsharedata == 20)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

void test_mutex_lock_share2(int* i,XdbLock* testlock)
{
	INTENT("mutex lock share2");
	try
	{
		int value;
		testlock->lock(XdbLock::Shared);
		value = testmsharedata;
		value--;
		if (testmsharedata == 20)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

void test_mutex_lock_share3(int* i,XdbLock* testlock)
{
	INTENT("mutex lock share2");
	try
	{
		int value;
		testlock->lock(XdbLock::Shared);
		value = testmsharedata;
		value = value-5;
		if (testmsharedata == 20)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

void test_mutex_lock_share4(int* i,XdbLock* testlock)
{
	INTENT("mutex lock share2");
	try
	{
		int value;
		testlock->lock(XdbLock::Shared);
		value = testmsharedata;
		value = value+5;
		if (testmsharedata == 20)
			*i = 1;
		else
			*i = 0;
		testlock->unlock();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		*i = 0;
	}
}

bool TestTransactionLock(void)
{
    INTENT("事务锁的测试\n");

    int sta[3] = {0};
    int found = 0;
	EntrySetCollInfo<9>::get();

    boost::thread_group tg;
    tg.create_thread(boost::bind(&test_first_transaction,&sta[0]));	
	tg.join_all();
	if (sta[0]) 
	{
       ++found; 
    }
	
	tg.create_thread(boost::bind(&test_second_transaction,&sta[1]));
	tg.create_thread(boost::bind(&test_third_transaction,&sta[2]));
	tg.join_all();
	if (sta[1]) 
	{
       ++found; 
    }
	if (sta[2]) 
	{
       ++found; 
    }
	
	if (found == 3) 
	{
        return true;
    }
    return false;     
}

void test_first_transaction(int *i)
{
	INTENT("先用事务插入数据\n");
	Transaction * pTransaction = NULL;
	try
	{
		pStorageEngine->beginThread();
		pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		Entryid = pStorageEngine->createEntrySet(pTransaction,EntrySetCollInfo<9>::get());
		EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,Entryid));
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
		InsertData(trantest);
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

void test_second_transaction(int * i)
{
	INTENT("同步查询数据\n");
	
	Transaction* pTransaction = NULL;
	try
	{
		pStorageEngine->beginThread();
		pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		
		if (XdbLock::transactionTryLock(0,Entryid,0,0, XdbLock::Exclusive))
			XdbLock::transactionLock(0,Entryid,0,0, XdbLock::Exclusive);
		EntrySet* heapentry=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,Entryid));//打开表
		if (NULL==heapentry)
			*i = 0;
		else
			*i = 1;
		pStorageEngine->closeEntrySet(pTransaction,heapentry);
		XdbLock::transactionUnlock(0,Entryid,0,0, XdbLock::Exclusive);
		pTransaction->commit();
	}
	CatchExcepation
	pStorageEngine->endThread(); 

}

void test_third_transaction(int * i)
{
	INTENT("同步查询数据\n");
	
	Transaction* pTransaction = NULL;
	try
	{
		pStorageEngine->beginThread();
		pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		
		if (XdbLock::transactionTryLock(0,Entryid,0,0, XdbLock::Exclusive))
			XdbLock::transactionLock(0,Entryid,0,0, XdbLock::Exclusive);
		EntrySet* heapentry=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,Entryid));//打开表
		if (NULL==heapentry)
			*i = 0;
		else
			*i = 1;
		pStorageEngine->closeEntrySet(pTransaction,heapentry);
		XdbLock::transactionUnlock(0,Entryid,0,0, XdbLock::Exclusive);
		pTransaction->commit();
	}
	CatchExcepation
	pStorageEngine->endThread(); 

}
