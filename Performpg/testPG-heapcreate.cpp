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
#include "testPG-heapcreate.h"
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

EntrySetID HEAP_ID;
EntrySetID INDEX_ID;

bool test_performpg_heap_create()
{
	try {
			get_new_transaction();//创建一个事务 
			EntrySet *penry_set = NULL;
			int count;
			
			cout<<"请输入建表的数量:";
			cin>>count;

			int *ID ;
			ID = (int *) malloc (sizeof(int)*count);
			{

				TimerFixture e;
				for (int i=0;i<count;i++)
				{
					HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//创建heap
					ID[i] = HEAP_ID;

				}
			}

			for (int i=0;i<count;i++)
			{
				pStorageEngine->removeEntrySet(pTransaction,ID[i]);//删除heap
			}
					commit_transaction();
	}

	CATCHEXCEPTION;
	return true;
}


		//	penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,0,EID,NULL, NULL));//打开表,transaction传NULL
		//	DataItem data;
		//	int len = sizeof(testdata);		
		//	char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
		//	memcpy(pstr, testdata, len);//构建DataItem 
		//	data.setData((void *)pstr);//pstr中的字符及参数传给data
		//	data.setSize(len);
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

