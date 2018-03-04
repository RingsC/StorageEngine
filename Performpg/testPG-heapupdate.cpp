#include <iostream>
#include <vector>
#include "boost/thread/thread.hpp"
#include "boost/thread.hpp" 
#include  "boost/thread/xtime.hpp" 
#include  "boost/thread/tss.hpp"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "pg_utils_dll.h"
#include "testPG-heapupdate.h"
//#include "test_fram.h"
//#include "sequence/utils.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;

using namespace FounderXDB::StorageEngineNS;
using namespace boost;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;

extern EntrySetID HEAP_ID;
extern EntrySetID INDEX_ID;

#define TESTDATA "test"
#define TESTDATA_NEW "test_new"
#define MAX_RAM_STR_SIZE 2000
bool test_performpg_heap_update()
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

		cout<<"PG更新表中数据..数据行数:";
		cin>>count;
		for (int i=0;i<count;i++)
		{
			penry_set->insertEntry(pTransaction, tid, data);//插入数据
		}

		command_counter_increment();

		DataItem newdata;
		int newlen = sizeof(TESTDATA_NEW);		
		char *newpstr = (char *)malloc(newlen);//记得释放空间...free(pstr)
		memcpy(newpstr, TESTDATA_NEW, newlen);//构建DataItem
		newdata.setData((void *)newpstr);//pstr中的字符及参数传给data
		newdata.setSize(newlen);

			{
				TimerFixture e;
				EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
				DataItem getdata;
				char *str = (char*)malloc(sizeof(TESTDATA));//分配一个空间给str...free(str);

				while (true)
				{
					int flag;
					flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}
					penry_set->updateEntry(pTransaction,tid,newdata);//更新EntrySet
				}
			
				penry_set->endEntrySetScan(entry_scan);

				pStorageEngine->closeEntrySet(pTransaction,penry_set);
				pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

				commit_transaction();
			}
		}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_update_special()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		EntryID tid ;
		int count;

		cout<<"PG更新表中所有随机数据为test（数据最大长度2000）...行数:";
		cin>>count;

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

		command_counter_increment();
		
		cout<< "INSERT DATA FINISH!" << endl;
		DataItem newdata;
		int newlen = sizeof(TESTDATA_NEW);		
		char *newpstr = (char *)malloc(newlen);//记得释放空间...free(pstr)
		memcpy(newpstr, TESTDATA_NEW, newlen);//构建DataItem
		newdata.setData((void *)newpstr);//pstr中的字符及参数传给data
		newdata.setSize(newlen);
		{
			TimerFixture e;
			EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
			DataItem getdata;
			char *str = (char*)malloc(sizeof(TESTDATA));//分配一个空间给str...free(str);

			while (true)
			{
				int flag;
				flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
				if (flag==NO_DATA_FOUND)
				{
					break;
				}
				penry_set->updateEntry(pTransaction,tid,newdata);//更新EntrySet
			}

			penry_set->endEntrySetScan(entry_scan);

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

			commit_transaction();
		}

	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_update_large_data()
{
	try {
		get_new_transaction();//创建一个事务 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		EntryID tid ;
		int count,data_size;

		cout<<"PG更新表中随机数据(数据大小可配置)...行数:";
		cin>>count;

		cout<<endl<<"插入数据的大小(生成数据为2的N次方字节)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;

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

		command_counter_increment();

		cout<< "INSERT DATA FINISH!" << endl;
		DataItem newdata;
		int newlen = sizeof(TESTDATA_NEW);		
		char *newpstr = (char *)malloc(newlen);//记得释放空间...free(pstr)
		memcpy(newpstr, TESTDATA_NEW, newlen);//构建DataItem
		newdata.setData((void *)newpstr);//pstr中的字符及参数传给data
		newdata.setSize(newlen);
		{
			TimerFixture e;
			EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
			DataItem getdata;
			char *str = (char*)malloc(sizeof(TESTDATA));//分配一个空间给str...free(str);

			while (true)
			{
				int flag;
				flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
				if (flag==NO_DATA_FOUND)
				{
					break;
				}
				penry_set->updateEntry(pTransaction,tid,newdata);//更新EntrySet
			}

			penry_set->endEntrySetScan(entry_scan);

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//删除heap

			commit_transaction();
		}
			cout<< "UPDATE DATA FINISH!" << endl;
	}

	CATCHEXCEPTION;
	return true;
}


bool test_performpg_thread_update()
{
	int thread_nums,count,data_size,status = 0;


	cout<<"PG多线程更新表中的随机数据...数据量:";
	cin>>count;

	cout<<endl<<"插入随机数据的大小(生成数据为2的N次方字节)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
	cin>>data_size;

	cout<<endl<<"分多个线程更新数据:";
	cin>>thread_nums;

	vector<EntryID>data_eid;
	vector<vector<EntryID>> temp_data_eid;

	thread_group tg;
	tg.create_thread(bind(&thread_heap_create, &status));
	tg.join_all();

	tg.create_thread(bind(&thread_heap_insert, HEAP_ID , count , data_size , &status , boost::ref(data_eid)));
	tg.join_all();
	cout<<"INSERT DATA FINISH!"<<endl;


	for (int j=0 ; j<thread_nums ; j++)
	{
		temp_data_eid.push_back(vector<EntryID>(data_eid.begin()+j*(count/thread_nums) , data_eid.begin()+(j+1)*(count/thread_nums)));
	}

	{
		TimerFixture e;
		for (int i=0 ; i<thread_nums ; i++)
		{
			tg.create_thread(bind(&thread_heap_update , &status , i, boost::ref(temp_data_eid)));
		} 
		tg.join_all();
	}
	cout<<"UPDATE DATA FINISH!"<<endl;

	tg.create_thread(bind(&thread_heap_remove, HEAP_ID, &status));
	tg.join_all();

	check_status(status);

	return true;
}
