#include <iostream>

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "heap/test_heap_close_dll.h"
#include "utils/utils_dll.h"
#include "test_fram.h"


using std::cout;
using std::endl;
using std::string;
using namespace FounderXDB::StorageEngineNS;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
extern EntrySetID EID;

#define testdata "testdata_1"
bool test_heap_close_Theninsert_dll()
{
	try {
		INTENT("测试closeEntrySet之后是否还能往表中插入数据,测试发现还能插入数据"
			   "而且能够查出来,最后的时候也不需要closeEntrySet");

		get_new_transaction();//创建一个事务 
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
		DataItem data;
		int len = sizeof(testdata);		
		char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
		memcpy(pstr, testdata, len);//构建DataItem 
		data.setData((void *)pstr);//pstr中的字符及参数传给data
		data.setSize(len);

		EntryID tid;
		penry_set->insertEntry(pTransaction, tid, data);//插入数据
		command_counter_increment();		
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		 
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata));//分配一个空间给str...free(str);
		int length;
		int flag;
		flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
		if (flag==NO_DATA_FOUND)
		{
			cout<<"检索至最后的数据了!"<<endl;
			HEAP_RETURN_FALSE_HS;	
		}
		str=(char*)getdata.getData();//getdata中的数据给str
		length=getdata.getSize();//大小
		flag=memcmp(testdata,str,sizeof(testdata));
		if (flag!=0)
		{
			cout<<"比较的数据不一致!"<<endl;
			HEAP_RETURN_FALSE_HS;
		}
		ENDSCAN_CLOSE_COMMIT;
	}
	CATCHEXCEPTION;
	return true;
}

bool test_heap_close_dll()
 {
 	try {
 		INTENT("测试closeEntrySet的transaction传入的为NULL,测试可以通过");
 
 		get_new_transaction();//创建一个事务 
 		EntrySet *penry_set = NULL;
 		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
 
 		pStorageEngine->closeEntrySet(pTransaction,penry_set);//关闭表
 		commit_transaction();//提交事务,必须提交事务才能删除表
 
 	}
	CATCHEXCEPTION;
 	return true;
 }


bool test_heap_close_withoutopen_dll()
{
	try {
		INTENT("测试没有openEntrySet的情况下直接close,penry_set为NULL,测试通过");

		get_new_transaction();//创建一个事务 
		EntrySet *penry_set = NULL;
		pStorageEngine->closeEntrySet(pTransaction,penry_set);//关闭表
		commit_transaction();//提交事务,必须提交事务才能删除表

	}
	CATCHEXCEPTION;
	return true;
}


