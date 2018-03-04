#include <iostream>
#include <vector>
#include <math.h>
#include "boost/thread/thread.hpp"
#include "boost/thread.hpp" 
#include  "boost/thread/xtime.hpp" 
#include  "boost/thread/tss.hpp"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "pg_utils_dll.h"
#include "testPG-heapdelete.h"
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
#define MAX_RAM_STR_SIZE 2000
bool test_performpg_heap_delete()
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

		cout<<"PGɾ����������..��������:";
		cin>>count;
		for (int i=0;i<count;i++)
		{
			penry_set->insertEntry(pTransaction, tid, data);//��������
		}

		command_counter_increment();

	
		{
			TimerFixture e;
			EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
			DataItem getdata;
			char *str = (char*)malloc(sizeof(TESTDATA));//����һ���ռ��str...free(str);
			unsigned int length;

			while (true)
			{
				int flag;
				flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
				if (flag==NO_DATA_FOUND)
				{
					break;
				}
				str=(char*)getdata.getData();//getdata�е����ݸ�str
				length=getdata.getSize();//��С			
				if(memcmp(str,TESTDATA,sizeof(TESTDATA)) == 0)
				{
					penry_set->deleteEntry(pTransaction,tid);//ɾ��Ԫ��
				}
			}
			penry_set->endEntrySetScan(entry_scan);
	
		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

		commit_transaction();
		}

	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_delete_special()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(pTransaction,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));
		//IndexEntrySet *pIndexEntry = NULL;
		//pIndexEntry =	IndexCreator<EntrySetCollInfo<4>,1>::createAndOpen(pTransaction,penry_set,"");

		EntryID tid ;
		int count;

		cout<<"PGɾ������������ݣ�������󳤶�2000��...����:";
		cin>>count;

		srand((unsigned)time( NULL ));

		vector<std::string>pstr_find;
		for (int i=0;i<count;i++)
		{

			std::string temp_data;
			RandomGenString(temp_data,(rand()%MAX_RAM_STR_SIZE+1));

			pstr_find.push_back(temp_data);

			DataItem data;
			int len = temp_data.length();		
			char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
			memcpy(pstr, temp_data.c_str(), len);//����DataItem 
			data.setData((void *)pstr);//pstr�е��ַ�����������data
			data.setSize(len);

			penry_set->insertEntry(pTransaction, tid, data);//��������
		}

		command_counter_increment();
		cout << "INSERT DATA FINISH!"<< endl;

		{
			TimerFixture e;

			//for (int i = 0 ; i < count ; i++)
			//{

			//SearchCondition search;
			//search.Add(1,Equal,string(pstr_find[i].begin(),pstr_find[i].begin() + 4),compare_str);

			EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
			DataItem getdata;
			char *str = (char*)malloc(sizeof(TESTDATA));//����һ���ռ��str...free(str);

				while (true)
				{
					int flag;
					flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}
					penry_set->deleteEntry(pTransaction,tid);//ɾ��Ԫ��
				}

			penry_set->endEntrySetScan(entry_scan);
			//command_counter_increment();

			//}

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();
		}

	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_heap_delete_large_data()
{
	try {
		get_new_transaction();//����һ������ 

		EntrySet *penry_set = NULL;
		HEAP_ID = pStorageEngine->createEntrySet(pTransaction,EntrySetCollInfo<4>::get());//����heap
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));

		EntryID tid ;
		int count,data_size;

		cout<<"PGɾ�������������(���ݴ�С������)...����:";
		cin>>count;

		cout<<endl<<"�������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;

		//srand((unsigned)time( NULL ));

		vector<std::string>pstr_find;
		for (int i=0;i<count;i++)
		{

			std::string temp_data;
			RandomGenString(temp_data,1<<data_size);

			pstr_find.push_back(temp_data);

			DataItem data;
			int len = temp_data.length();		
			char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
			memcpy(pstr, temp_data.c_str(), len);//����DataItem 
			data.setData((void *)pstr);//pstr�е��ַ�����������data
			data.setSize(len);

			penry_set->insertEntry(pTransaction, tid, data);//��������
		}

		command_counter_increment();
		cout << "INSERT DATA FINISH!"<< endl;

		{
			TimerFixture e;

			EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
			DataItem getdata;
			char *str = (char*)malloc(sizeof(TESTDATA));//����һ���ռ��str...free(str);

			while (true)
			{
				int flag;
				flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
				if (flag==NO_DATA_FOUND)
				{
					break;
				}
				penry_set->deleteEntry(pTransaction,tid);//ɾ��Ԫ��
			}

			penry_set->endEntrySetScan(entry_scan);

			pStorageEngine->closeEntrySet(pTransaction,penry_set);
			pStorageEngine->removeEntrySet(pTransaction,HEAP_ID);//ɾ��heap

			commit_transaction();
		}
		cout << "DELETE DATA FINISH!"<< endl;

	}

	CATCHEXCEPTION;
	return true;
}

bool test_performpg_thread_delete()
{
	int count,data_size,thread_nums,status = 0;
	vector<EntryID>data_eid;
	vector<vector<EntryID>> temp_data_eid;

	cout<<"PG���߳�ɾ�����е��������...������:";
	cin>>count;

	cout<<endl<<"����������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
	cin>>data_size;

	cout<<endl<<"ɾ���������ݵ��߳���:";
	cin>>thread_nums;

	thread_group tg;
	tg.create_thread(bind(&thread_heap_create, &status));
	tg.join_all();

	tg.create_thread(bind(&thread_heap_insert, HEAP_ID , count , data_size , &status, boost::ref(data_eid)));
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
			tg.create_thread(bind(&thread_heap_delete , &status , i, boost::ref(temp_data_eid)));
		} 
			tg.join_all();
	}
	cout<<"DELETE DATA FINISH!"<<endl;

	tg.create_thread(bind(&thread_heap_remove, HEAP_ID, &status));
	tg.join_all();

	check_status(status);

	return true;
}


//	penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,0,EID,NULL, NULL));//�򿪱�,transaction��NULL
//	DataItem data;
//	int len = sizeof(testdata);		
//	char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
//	memcpy(pstr, testdata, len);//����DataItem 
//	data.setData((void *)pstr);//pstr�е��ַ�����������data
//	data.setSize(len);
//	
//EntryID tid;
//penry_set->insertEntry(pTransaction, tid, data);//��������
//command_counter_increment();		
//EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, NULL, (std::vector<ScanCondition>)NULL);//��ʼɨ��
//DataItem getdata;
//char *str = (char*)malloc(sizeof(testdata));//����һ���ռ��str...free(str);
//unsigned int length;
//entry_scan->getNext(NEXT_FLAG,tid,getdata);//��ʱͨ��getdata�жϺ����Ƿ������ݣ��Ժ����ͨ�����ص�int�Ƿ�ΪNULL�ж�
//str=(char*)getdata.getData();//getdata�е����ݸ�str
//length=getdata.getSize();//��С
//
////CHECK_BOOL(length!=0);
//if (length==0)
//{
//	cout<<"��ѯ�����ݳ���Ϊ0������!"<<endl;
//	HEAP_RETURN_FALSE;
//}
//int flag=0;
//flag=memcmp(testdata,str,sizeof(str));
////CHECK_BOOL(flag==0);
//if (flag!=0)
//{
//	cout<<"��ѯ��������Ԥ��Ĳ�ͬ������!"<<endl;
//	HEAP_RETURN_FALSE;
//}
//
//ENDSCAN_CLOSE_COMMIT;

