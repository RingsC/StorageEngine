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
#include "testPG-indexscan.h"
#include "pg_utils_dll.h"
#include<time.h>
#define random(x) (rand()%x)
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

#define TESTDATA "test"
#define FINDTIMES 100
#define MAX_RAM_STR_SIZE 2000

bool test_performpg_indexscan_special()
{
	try {
			get_new_transaction();//����һ������ 

			EntrySet *penry_set = NULL;
			HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
			penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

			EntryID tid ;
			int count;

			cout<<"PGͨ�������Ӻ��������в���100������...����:";
			cin>>count;

			INDEX_ID = pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());

			std::string testdata ("test");
			
			for (int i=0;i<count;i++)
			{
				std::stringstream pstr;
				DataItem data;
				pstr << testdata << i;
				data.setData((void*)pstr.str().c_str());//pstr�е��ַ�����������data
				data.setSize(pstr.str().length());

				penry_set->insertEntry(pTransaction, tid, data);//��������
			}
			cout<< "INSERT DATA FINISH!" << endl;

			command_counter_increment();
			{
				TimerFixture d;
				IndexEntrySet *pIndexEntry = NULL;
				pIndexEntry = pStorageEngine->openIndexEntrySet(pTransaction,penry_set,EntrySet::OPEN_EXCLUSIVE,INDEX_ID,NULL);
					srand((unsigned)time( NULL ));
				for (int i = 0; i< FINDTIMES ; i++)
				{
					std::stringstream pstr_find;
					pstr_find << testdata << random(count);

					vector<ScanCondition> keyVec;
					initKeyVector(keyVec,1,ScanCondition::Equal,pstr_find.str().c_str(),compare_str);

					IndexEntrySetScan *pIndexScan  = (IndexEntrySetScan *)pIndexEntry->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, keyVec);//��ʼɨ��

					DataItem *pScanData = new DataItem;
					EntryID scan_tid;
					while ( pIndexScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
					{		
						break;
					}
					pIndexEntry->endEntrySetScan(pIndexScan);
				}
				pStorageEngine->closeEntrySet(pTransaction,pIndexEntry);

				pStorageEngine->closeEntrySet(pTransaction,penry_set);
				pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

				commit_transaction();

		}
			cout<< "SCAN DATA FINISH!" << endl;
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_indexscan_special_random()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		EntryID tid ;
		int count;

		cout<<"PGͨ�������ҵ����е�������ݣ�������󳤶�Ϊ2000��...����:";
		cin>>count;
		IndexEntrySet *pIndexEntry = NULL;
		pIndexEntry =	IndexCreator<EntrySetCollInfo<4>,1>::createAndOpen(pTransaction,penry_set,"");

		srand((unsigned)time( NULL ));

		vector<std::string>pstr_find;

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

			pstr_find.push_back(temp_data);

			penry_set->insertEntry(pTransaction, tid, data);//��������
		}

			command_counter_increment();
			cout<<"INSERT DATA FINISH!"<<endl;
		{
			TimerFixture d;
			srand((unsigned)time( NULL ));
			for (int i = 0; i< count ; i++)
			{
				SearchCondition search;
				search.Add(1,Equal,string(pstr_find[i].begin(),pstr_find[i].begin() + 4),compare_str);

				IndexEntrySetScan *pIndexScan  = (IndexEntrySetScan *)pIndexEntry->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, search.Keys());//��ʼɨ��

				DataItem *pScanData = new DataItem;
				EntryID scan_tid;
				if( pIndexScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
				{		
				}
				else
				{
					cout<<"error! testdata no the same!";
					break;
				}
				pIndexEntry->endEntrySetScan(pIndexScan);
			}
			pStorageEngine->closeEntrySet(pTransaction,pIndexEntry);

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();

		}
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_indexscan()
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

		cout<<"PGͨ��������������...����:";
		cin>>count;

		INDEX_ID = pStorageEngine->createIndexEntrySet(pTransaction,penry_set,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetCollInfo<4>::get());

		for (int i=0;i<count;i++)
		{
			penry_set->insertEntry(pTransaction, tid, data);//��������
		}

		command_counter_increment();
		{
			TimerFixture d;
			IndexEntrySet *pIndexEntry = NULL;
			pIndexEntry = pStorageEngine->openIndexEntrySet(pTransaction,penry_set,EntrySet::OPEN_EXCLUSIVE,INDEX_ID,NULL);


			vector<ScanCondition> keyVec;
			initKeyVector(keyVec,1,ScanCondition::Equal,"test",compare_str);

			IndexEntrySetScan *pIndexScan  = (IndexEntrySetScan *)pIndexEntry->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, keyVec);//��ʼɨ��

			DataItem *pScanData = new DataItem;
			EntryID scan_tid;
			if ( pIndexScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
			{		
				//if(memcmp((char*)pScanData->getData(),TESTDATA,sizeof(TESTDATA)) != 0)
				//{
				//	cout<<"error! testdata no the same!";
				//	break;
				//}	

			}
			else
			{
				cout<<"error! testdata no the same!";
			}
			pIndexEntry->endEntrySetScan(pIndexScan);
			pStorageEngine->closeEntrySet(pTransaction,pIndexEntry);

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();

		}
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_indexscan_large_data()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		EntryID tid ;
		int count,data_size;

		cout<<"PGͨ�������ҵ����е��������...����:";
		cin>>count;

		cout<<endl<<"�������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;

		IndexEntrySet *pIndexEntry = NULL;
		pIndexEntry =	IndexCreator<EntrySetCollInfo<4>,1>::createAndOpen(pTransaction,penry_set,"");

		srand((unsigned)time( NULL ));

		vector<std::string>pstr_find;

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

			pstr_find.push_back(temp_data);

			penry_set->insertEntry(pTransaction, tid, data);//��������
		}

		command_counter_increment();
		cout<<"INSERT DATA FINISH!"<<endl;
		{
			TimerFixture d;
			//srand((unsigned)time( NULL ));
			for (int i = 0; i< count ; i++)
			{
				SearchCondition search;
				search.Add(1,Equal,string(pstr_find[i].begin(),pstr_find[i].begin() + 4),compare_str);

				IndexEntrySetScan *pIndexScan  = (IndexEntrySetScan *)pIndexEntry->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, search.Keys());//��ʼɨ��

				DataItem *pScanData = new DataItem;
				EntryID scan_tid;
				if( pIndexScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
				{		
				}
				else
				{
					cout<<"error! testdata no the same!";
					break;
				}
				pIndexEntry->endEntrySetScan(pIndexScan);
			}
			pStorageEngine->closeEntrySet(pTransaction,pIndexEntry);

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();

		}
	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_thread_scan()
{
	//int count,data_size,thread_nums,status = 0;
	//vector<EntryID>data_eid;
	//vector<vector<EntryID>> temp_data_eid;

	//cout<<"PG���߳�ɨ����е��������...������:";
	//cin>>count;

	//cout<<endl<<"����������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
	//cin>>data_size;

	//cout<<endl<<"ɨ��������ݵ��߳���:";
	//cin>>thread_nums;

	//thread_group tg;
	//tg.create_thread(bind(&thread_heap_create, &status));
	//tg.join_all();

	//tg.create_thread(bind(&thread_heap_insert, HEAP_ID , count , data_size , &status, boost::ref(data_eid)));
	//tg.join_all();

	//cout<<"INSERT DATA FINISH!"<<endl;

	//for (int j=0 ; j<thread_nums ; j++)
	//{
	//	temp_data_eid.push_back(vector<EntryID>(data_eid.begin()+j*(count/thread_nums) , data_eid.begin()+(j+1)*(count/thread_nums)));

	//}

	//{
	//	TimerFixture e;
	//	for (int i=0 ; i<thread_nums ; i++)
	//	{
	//		tg.create_thread(bind(&thread_heap_scan , &status , i, boost::ref(temp_data_eid)));
	//	} 
	//	tg.join_all();
	//}
	//cout<<"DELETE DATA FINISH!"<<endl;

	//tg.create_thread(bind(&thread_heap_remove, HEAP_ID, &status));
	//tg.join_all();

	//check_status(status);

	return true;
}