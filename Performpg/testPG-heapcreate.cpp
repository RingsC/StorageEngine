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
			get_new_transaction();//����һ������ 
			EntrySet *penry_set = NULL;
			int count;
			
			cout<<"�����뽨�������:";
			cin>>count;

			int *ID ;
			ID = (int *) malloc (sizeof(int)*count);
			{

				TimerFixture e;
				for (int i=0;i<count;i++)
				{
					HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<4>::get());//����heap
					ID[i] = HEAP_ID;

				}
			}

			for (int i=0;i<count;i++)
			{
				pStorageEngine->removeEntrySet(pTransaction,ID[i]);//ɾ��heap
			}
					commit_transaction();
	}

	CATCHEXCEPTION;
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

