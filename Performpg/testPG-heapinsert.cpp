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
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		DataItem data;
		int len = sizeof(TESTDATA);		
		char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
		memcpy(pstr, TESTDATA, len);//����DataItem 
		data.setData((void *)pstr);//pstr�е��ַ�����������data
		data.setSize(len);

		EntryID tid ;
		int count;

		cout<<"PG�����в�������..��������:";
		cin>>count;
		TimerFixture e;
		{
			
			for (int i=0;i<count;i++)
			{
			penry_set->insertEntry(pTransaction, tid, data);//��������
			}
			//pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());
		}

		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

		commit_transaction();
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_createIndexFirst_thenHeap()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		DataItem data;
		int len = sizeof(TESTDATA);		
		char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
		memcpy(pstr, TESTDATA, len);//����DataItem 
		data.setData((void *)pstr);//pstr�е��ַ�����������data
		data.setSize(len);

		EntryID tid ;
		int count;

		cout<<"PG�Ƚ������ٲ�������...����:";
		cin>>count;
		//cout<<"��������ʱ��:"<<endl;
		
		{
			TimerFixture e;
			INDEX_ID = pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());
		}
		//cout<<"��������ʱ��:"<<endl;
		
		{
			TimerFixture d;
			for (int i=0;i<count;i++)
			{
				penry_set->insertEntry(pTransaction, tid, data);//��������
			}
		
		command_counter_increment();
		//IndexEntrySet *pIndexEntry = NULL;
		//pIndexEntry = pStorageEngine->openIndexEntrySet(pTransaction,penry_set,EntrySet::OPEN_EXCLUSIVE,INDEX_ID,NULL);


		//vector<ScanCondition> keyVec;
		//initKeyVector(keyVec,1,ScanCondition::Equal,"test",compare_str);

		//IndexEntrySetScan *pIndexScan  = (IndexEntrySetScan *)pIndexEntry->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, keyVec);//��ʼɨ��

		//DataItem *pScanData = new DataItem;
		//EntryID scan_tid;
		//while ( pIndexScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
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
		pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

		commit_transaction();

		}
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_createHeapFisrt_thenIndex()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		DataItem data;
		int len = sizeof(TESTDATA);		
		char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
		memcpy(pstr, TESTDATA, len);//����DataItem 
		data.setData((void *)pstr);//pstr�е��ַ�����������data
		data.setSize(len);

		EntryID tid ;
		int count;

		cout<<"PG�Ȳ��������ٽ�����...����:";
		cin>>count;
		
		{
			TimerFixture e;

			for (int i=0;i<count;i++)
			{
				penry_set->insertEntry(pTransaction, tid, data);//��������
			}
		}

		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());
		
		
		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

		commit_transaction();
		}
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_special_indexfirst()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	
		}


		EntryID tid ;
		int count;

		cout<<"PG�Ƚ������ٲ���������ݣ�������󳤶�2000��...����:";
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
				char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//����DataItem 
				data.setData((void *)pstr);//pstr�е��ַ�����������data
				data.setSize(len);

				penry_set->insertEntry(pTransaction, tid, data);//��������
			}

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();
		}

	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_special_indexlater()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		EntryID tid ;
		int count;

		cout<<"PG�Ȳ���������ݣ�������󳤶�2000���ٽ�����...����:";
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
				char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//����DataItem 
				data.setData((void *)pstr);//pstr�е��ַ�����������data
				data.setSize(len);

				penry_set->insertEntry(pTransaction, tid, data);//��������
			}
		}
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();
		}


	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insertentries_indexfisrt()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	
		}

		int count;

		cout<<"PGͨ��insertentries�����������(�Ƚ�����)��������󳤶�2000��...����:";
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
				char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//����DataItem 
				data.setData((void *)pstr);//pstr�е��ַ�����������data
				data.setSize(len);

				insertdata.push_back(data);
			}

			penry_set->insertEntries(pTransaction,insertdata);//��������

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();
		}

	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insertentries_indexlater()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		int count;

		cout<<"PGͨ��insertentries�����������(������)��������󳤶�2000��...����:";
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
				char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//����DataItem 
				data.setData((void *)pstr);//pstr�е��ַ�����������data
				data.setSize(len);

				insertdata.push_back(data);
			}
			penry_set->insertEntries(pTransaction,insertdata);//��������
		}
			{
				TimerFixture d;
				pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	


			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();
			}


	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insert_large_data()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		EntryID tid ;
		int count,data_size;

		cout<<"PG�Ȳ�����������ٽ�����...����:";
		cin>>count;

		cout<<endl<<"�������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
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
				char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//����DataItem 
				data.setData((void *)pstr);//pstr�е��ַ�����������data
				data.setSize(len);

				penry_set->insertEntry(pTransaction, tid, data);//��������
			}
		}
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();
		}


	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_insertentries_large_data()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		int count,data_size;

		cout<<"PGͨ��insertentries�����������(������)...����:";
		cin>>count;

		cout<<endl<<"�������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
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
				char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//����DataItem 
				data.setData((void *)pstr);//pstr�е��ַ�����������data
				data.setSize(len);

				insertdata.push_back(data);
			}
			penry_set->insertEntries(pTransaction,insertdata);//��������
		}
		{
			TimerFixture d;
			pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());	


			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();
		}


	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_thread_insert()
{	
	int thread_num,count,data_size,status = 0;
	

	cout<<"PG���߳���һ�����в����������(������)...�߳�����:";
	cin>>thread_num;

	cout<<endl<<"ÿ���̲߳���������:";
	cin>>count;

	cout<<endl<<"����������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
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

