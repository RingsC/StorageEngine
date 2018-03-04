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
		INTENT("����closeEntrySet֮���Ƿ��������в�������,���Է��ֻ��ܲ�������"
			   "�����ܹ������,����ʱ��Ҳ����ҪcloseEntrySet");

		get_new_transaction();//����һ������ 
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
		DataItem data;
		int len = sizeof(testdata);		
		char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
		memcpy(pstr, testdata, len);//����DataItem 
		data.setData((void *)pstr);//pstr�е��ַ�����������data
		data.setSize(len);

		EntryID tid;
		penry_set->insertEntry(pTransaction, tid, data);//��������
		command_counter_increment();		
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
		 
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata));//����һ���ռ��str...free(str);
		int length;
		int flag;
		flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
		if (flag==NO_DATA_FOUND)
		{
			cout<<"����������������!"<<endl;
			HEAP_RETURN_FALSE_HS;	
		}
		str=(char*)getdata.getData();//getdata�е����ݸ�str
		length=getdata.getSize();//��С
		flag=memcmp(testdata,str,sizeof(testdata));
		if (flag!=0)
		{
			cout<<"�Ƚϵ����ݲ�һ��!"<<endl;
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
 		INTENT("����closeEntrySet��transaction�����ΪNULL,���Կ���ͨ��");
 
 		get_new_transaction();//����һ������ 
 		EntrySet *penry_set = NULL;
 		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
 
 		pStorageEngine->closeEntrySet(pTransaction,penry_set);//�رձ�
 		commit_transaction();//�ύ����,�����ύ�������ɾ����
 
 	}
	CATCHEXCEPTION;
 	return true;
 }


bool test_heap_close_withoutopen_dll()
{
	try {
		INTENT("����û��openEntrySet�������ֱ��close,penry_setΪNULL,����ͨ��");

		get_new_transaction();//����һ������ 
		EntrySet *penry_set = NULL;
		pStorageEngine->closeEntrySet(pTransaction,penry_set);//�رձ�
		commit_transaction();//�ύ����,�����ύ�������ɾ����

	}
	CATCHEXCEPTION;
	return true;
}


