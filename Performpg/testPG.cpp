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

//get_new_transaction();//创建一个事务 
//EntrySet *penry_set = NULL;
//EntrySetID EID =0;
//penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,0,EID,NULL, NULL));//打开表,transaction传NULL
//DataItem data;
//int len = sizeof(testdata);		
//char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
//memcpy(pstr, testdata, len);//构建DataItem 
//data.setData((void *)pstr);//pstr中的字符及参数传给data
//data.setSize(len);
//
//EntryID tid;
//penry_set->insertEntry(pTransaction, tid, data);//插入数据
//command_counter_increment();		
//EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, NULL, (std::vector<ScanCondition>)NULL);//开始扫描
//DataItem getdata;
//char *str = (char*)malloc(sizeof(testdata));//分配一个空间给str...free(str);
//unsigned int length;
//entry_scan->getNext(NEXT_FLAG,tid,getdata);//暂时通过getdata判断后面是否还有数据，以后可以通过返回的int是否为NULL判断
//str=(char*)getdata.getData();//getdata中的数据给str
//length=getdata.getSize();//大小
//
////CHECK_BOOL(length!=0);
//if (length==0)
//{
//	cout<<"查询的数据长度为0，出错!"<<endl;
//	HEAP_RETURN_FALSE;
//}
//int flag=0;
//flag=memcmp(testdata,str,sizeof(str));
////CHECK_BOOL(flag==0);
//if (flag!=0)
//{
//	cout<<"查询的数据与预测的不同，出错!"<<endl;
//	HEAP_RETURN_FALSE;
//}
//
//ENDSCAN_CLOSE_COMMIT;
//}