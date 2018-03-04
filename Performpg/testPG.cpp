#include <iostream>
#include <vector>
//#include "boost/thread/thread.hpp"
//#include "boost/thread.hpp" 
//#include  "boost/thread/xtime.hpp" 
//#include  "boost/thread/tss.hpp"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
//#include "heap/test_heap_create_dll.h"
#include "pg_utils_dll.h"
#include "testPG.h"
//#include "test_fram.h"
//#include "sequence/utils.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace FounderXDB::StorageEngineNS;
using namespace boost;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
//extern EntrySetID EID;
//EntrySetID indexIdCheck = 0;
//bool flag = false;
//bool insert = false;

int start_engine_();
int stop_engine_();

int main()
{
	try {
			start_engine_();
			//test_performpg_heap_create();

			//test_performpg_heap_insert();
			//test_performpg_heap_insert_createHeapFisrt_thenIndex();//fix data
			//test_performpg_heap_insert_createIndexFirst_thenHeap();//fix data
			//test_performpg_heap_insert_large_data();
			//test_performpg_heap_insertentries_indexfisrt();
			//test_performpg_heap_insertentries_indexlater();
			//test_performpg_heap_insertentries_large_data();
			//test_performpg_heap_insert_special_indexfirst();//random data
			//test_performpg_heap_insert_special_indexlater();//random data

			//test_performpg_heap_delete();
			//test_performpg_heap_delete_special();
			//test_performpg_heap_delete_large_data();

			//test_performpg_heap_update();
			//test_performpg_heap_update_special();
			//test_performpg_heap_update_large_data();

			//test_performpg_indexscan();
			//test_performpg_indexscan_special();
			//test_performpg_indexscan_special_random();
			//test_performpg_indexscan_large_data();

			//test_performpg_thread_insert();
			//test_performpg_thread_delete();
			test_performpg_thread_update();
			//test_performpg_thread_scan();

			

			stop_engine_();

		}
			CATCHEXCEPTION;
			return true;

}

int start_engine_()
{
//	std::string g_strDataDir;
	pStorageEngine = StorageEngine::getStorageEngine();
//	pStorageEngine->bootstrap("D:\\dataDll");
//  pStorageEngine->bootstrap("C:\\storageEngine\\dataDll");
	pStorageEngine->initialize("C:\\storageEngine\\dataDll", 80, false); //start engine
//	pStorageEngine->initialize(const_cast<char*>(g_strDataDir.c_str()), 80); //start engine
	return 1;
}

int stop_engine_()
{
	pStorageEngine->shutdown();//stop engine
	return 1;
}

//get_new_transaction();//����һ������ 
//EntrySet *penry_set = NULL;
//EntrySetID EID =0;
//penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,0,EID,NULL, NULL));//�򿪱�,transaction��NULL
//DataItem data;
//int len = sizeof(testdata);		
//char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
//memcpy(pstr, testdata, len);//����DataItem 
//data.setData((void *)pstr);//pstr�е��ַ�����������data
//data.setSize(len);
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
//}