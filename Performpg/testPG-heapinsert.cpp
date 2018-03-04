#include <iostream>
#include <vector>
#include <sstream>
#include "boost/thread/thread.hpp"
#include "boost/thread.hpp" 
#include  "boost/thread/xtime.hpp" 
#include  "boost/thread/tss.hpp"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "testPG-heapinsert.h"
#include "pg_utils_dll.h"
#include "testPG-heapinsert.h"
//#include "test_fram.h"
//#include "sequence/utils.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;
using std::stringstream;

using namespace FounderXDB::StorageEngineNS;
using namespace boost;

extern EntrySetID HEAP_ID;
extern EntrySetID INDEX_ID;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;

#define MAX_RAM_STR_SIZE 2000
#define TESTDATA "test"
bool test_performpg_heap_insert()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		DataItem data;
		int len = sizeof(TESTDATA);		
		char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
		memcpy(pstr, TESTDATA, len);//构建DataItem 
		data.setData((void *)pstr);//pstr中的字符及参数传给data
		data.setSize(len);

		EntryID tid ;
		int count;

		cout<<"PG往表中插入数据..数据行数:";
		cin>>count;
		TimerFixture e;
		{
			
			for (int i=0;i<count;i++)
			{
			penry_set->insertEntry(pTransaction, tid, data);//插入数据
			}
			//pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());
		}

		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

		commit_transaction();
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_createIndexFirst_thenHeap()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		DataItem data;
		int len = sizeof(TESTDATA);		
		char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
		memcpy(pstr, TESTDATA, len);//构建DataItem 
		data.setData((void *)pstr);//pstr中的字符及参数传给data
		data.setSize(len);

		EntryID tid ;
		int count;

		cout<<"PG先建索引再插入数据...行数:";
		cin>>count;
		//cout<<"建立索引时间:"<<endl;
		
		{
			TimerFixture e;
			INDEX_ID = pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());
		}
		//cout<<"插入数据时间:"<<endl;
		
		{
			TimerFixture d;
			for (int i=0;i<count;i++)
			{
				penry_set->insertEntry(pTransaction, tid, data);//插入数据
			}
		
		command_counter_increment();
		//IndexEntrySet *pIndexEntry = NULL;
		//pIndexEntry = pStorageEngine->openIndexEntrySet(pTransaction,penry_set,EntrySet::OPEN_EXCLUSIVE,INDEX_ID,NULL);


		//vector<ScanCondition> keyVec;
		//initKeyVector(keyVec,1,ScanCondition::Equal,"test",compare_str);

		//IndexEntrySetScan *pIndexScan  = (IndexEntrySetScan *)pIndexEntry->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, keyVec);//开始扫描

		//DataItem *pScanData = new DataItem;
		//EntryID scan_tid;
		//while ( pIndexScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		//{		
		//	if(memcmp((char*)pScanData->getData(),TESTDATA,sizeof(TESTDATA)) != 0)
		//	{
		//		cout<<"error! testdata no the same!";
		//		break;
		//	}	

		//}
		//pIndexEntry->endEntrySetScan(pIndexScan);
		//pStorageEngine->closeEntrySet(pTransaction,pIndexEntry);

		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

		commit_transaction();

		}
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_createHeapFisrt_thenIndex()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		DataItem data;
		int len = sizeof(TESTDATA);		
		char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
		memcpy(pstr, TESTDATA, len);//构建DataItem 
		data.setData((void *)pstr);//pstr中的字符及参数传给data
		data.setSize(len);

		EntryID tid ;
		int count;

		cout<<"PG先插入数据再建索引...行数:";
		cin>>count;
		
		{
			TimerFixture e;

			for (int i=0;i<count;i++)
			{
				penry_set->insertEntry(pTransaction, tid, data);//插入数据
			}
		}

		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());
		
		
		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

		commit_transaction();
		}
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_special_indexfirst()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	
		}


		EntryID tid ;
		int count;

		cout<<"PG先建索引再插入随机数据（数据最大长度2000）...行数:";
		cin>>count;

		{	
			TimerFixture e;
			srand((unsigned)time( NULL ));

			for (int i=0;i<count;i++)
			{

				std::string temp_data;
				RandomGenString(temp_data,(rand()%MAX_RAM_STR_SIZE+1));

				DataItem data;
				int len = temp_data.length();		
				char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//构建DataItem 
				data.setData((void *)pstr);//pstr中的字符及参数传给data
				data.setSize(len);

				penry_set->insertEntry(pTransaction, tid, data);//插入数据
			}

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

			commit_transaction();
		}

	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_special_indexlater()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		EntryID tid ;
		int count;

		cout<<"PG先插入随机数据（数据最大长度2000）再建索引...行数:";
		cin>>count;

		{	
			TimerFixture e;
			srand((unsigned)time( NULL ));

			for (int i=0;i<count;i++)
			{

				std::string temp_data;
				RandomGenString(temp_data,(rand()%MAX_RAM_STR_SIZE+1));

				DataItem data;
				int len = temp_data.length();		
				char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//构建DataItem 
				data.setData((void *)pstr);//pstr中的字符及参数传给data
				data.setSize(len);

				penry_set->insertEntry(pTransaction, tid, data);//插入数据
			}
		}
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

			commit_transaction();
		}


	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insertentries_indexfisrt()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	
		}

		int count;

		cout<<"PG通过insertentries插入随机数据(先建索引)（数据最大长度2000）...行数:";
		cin>>count;

		{	
			TimerFixture e;
			srand((unsigned)time( NULL ));

			vector<DataItem> insertdata;
			for (int i=0;i<count;i++)
			{

				std::string temp_data;
				RandomGenString(temp_data,(rand()%MAX_RAM_STR_SIZE+1));

				DataItem data;
				int len = temp_data.length();		
				char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//构建DataItem 
				data.setData((void *)pstr);//pstr中的字符及参数传给data
				data.setSize(len);

				insertdata.push_back(data);
			}

			penry_set->insertEntries(pTransaction,insertdata);//插入数据

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

			commit_transaction();
		}

	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insertentries_indexlater()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		int count;

		cout<<"PG通过insertentries插入随机数据(后建索引)（数据最大长度2000）...行数:";
		cin>>count;

		{	
			TimerFixture e;
			srand((unsigned)time( NULL ));

			vector<DataItem> insertdata;
			for (int i=0;i<count;i++)
			{

				std::string temp_data;
				RandomGenString(temp_data,(rand()%MAX_RAM_STR_SIZE+1));

				DataItem data;
				int len = temp_data.length();		
				char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//构建DataItem 
				data.setData((void *)pstr);//pstr中的字符及参数传给data
				data.setSize(len);

				insertdata.push_back(data);
			}
			penry_set->insertEntries(pTransaction,insertdata);//插入数据
		}
			{
				TimerFixture d;
				pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	


			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

			commit_transaction();
			}


	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_large_data()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		EntryID tid ;
		int count,data_size;

		cout<<"PG先插入随机数据再建索引...行数:";
		cin>>count;

		cout<<endl<<"插入数据的大小(生成数据为2的N次方字节)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;

		{	
			TimerFixture e;
			//srand((unsigned)time( NULL ));

			for (int i=0;i<count;i++)
			{

				std::string temp_data;
				RandomGenString(temp_data,1<<data_size);

				DataItem data;
				int len = temp_data.length();		
				char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//构建DataItem 
				data.setData((void *)pstr);//pstr中的字符及参数传给data
				data.setSize(len);

				penry_set->insertEntry(pTransaction, tid, data);//插入数据
			}
		}
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

			commit_transaction();
		}


	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insertentries_large_data()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		int count,data_size;

		cout<<"PG通过insertentries插入随机数据(后建索引)...行数:";
		cin>>count;

		cout<<endl<<"插入数据的大小(生成数据为2的N次方字节)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;


		{	
			TimerFixture e;
			//srand((unsigned)time( NULL ));

			vector<DataItem> insertdata;
			for (int i=0;i<count;i++)
			{

				std::string temp_data;
				RandomGenString(temp_data,1<<data_size);

				DataItem data;
				int len = temp_data.length();		
				char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//构建DataItem 
				data.setData((void *)pstr);//pstr中的字符及参数传给data
				data.setSize(len);

				insertdata.push_back(data);
			}
			penry_set->insertEntries(pTransaction,insertdata);//插入数据
		}
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	


			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

			commit_transaction();
		}


	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_thread_insert()
{	
	int thread_num,count,data_size,status = 0;
	

	cout<<"PG多线程向一个表中插入随机数据(后建索引)...线程数量:";
	cin>>thread_num;

	cout<<endl<<"每个线程插入数据量:";
	cin>>count;

	cout<<endl<<"插入随机数据的大小(生成数据为2的N次方字节)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
	cin>>data_size;

	vector<EntryID>data_eid;
	thread_group tg;
	tg.create_thread(bind(&thread_heap_create, &status));
	tg.join_all();

	{
		TimerFixture e;
		for(int i = 0; i < thread_num; ++i)
		{
			tg.create_thread(bind(&thread_heap_insert, HEAP_ID , count , data_size , &status , boost::ref(data_eid)));
		}
		tg.join_all();
	}

	tg.create_thread(bind(&thread_heap_remove, HEAP_ID, &status));
	tg.join_all();

	check_status(status);

	return true;
}

