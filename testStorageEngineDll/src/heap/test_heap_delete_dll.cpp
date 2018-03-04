#include <iostream>

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "heap/test_heap_delete_dll.h"
#include "utils/utils_dll.h"
#include "test_fram.h"
#include "EntryIDBitmap.h"

using std::cout;
using std::endl;
using std::string;
using namespace FounderXDB::StorageEngineNS;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
extern EntrySetID EID;
extern void my_spilt_heap_shot(RangeDatai& rangeData, const char* str, int col, size_t len = 0);
extern void my_split_index_shot(RangeDatai& rangeData, const char *str, int col, size_t len = 0);
extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);
#define testdata  "testdata"
bool beginTest()
{
	try {
		get_new_transaction();
		string entrySetName("test");
		EntrySet *pEntrySet = NULL;
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = NULL;
		uint32 colid = 5;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		EID = pStorageEngine->createEntrySet(pTransaction, colid, 0,NULL);//����һ��heap
		
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
		EntryID tid;
		char data[20][20] = {"testdata_1", "testdata_2", "testdata_3", "testdata_1",\
			"testdata_2", "testdata_1", "apple", "reboot", "apple"};
			for (int cnt = 0; cnt < 100; ++cnt) 
			{
				for(int i = 0; i < 9; ++i)
				{
					DataItem entry(data[i], strlen(data[i]));
					penry_set->insertEntry(pTransaction, tid, entry);
				}
			}
		pStorageEngine->closeEntrySet(pTransaction,penry_set);//�رձ�
		commit_transaction();//�ύ����
		
	} 
	CATCHEXCEPTION
	return true;
}

#define testdata1 "testdata_1"
#define testdata2 "testdata_2"
#define testdata3 "testdata_3"
bool test_Delete_FirstTupleFromHeap_dll()
{
	try {
		INTENT("����һ������Ԫ�飬����ɾ�����ݱ��е�һ��Ԫ��,"
				"Ԥ�Ʋ���������ݿ�������һ��Ԫ�飬���ǵڶ���Ԫ��");
		get_new_transaction();
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
		EntryID tid;
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata1));//����һ���ռ��str
		int length;
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);//Ԫ���ڵ�һ�У���getNextһ��
		str=(char*)getdata.getData();//getdata�е����ݸ�str
		length=getdata.getSize();//��С
		penry_set->deleteEntry(pTransaction,tid);//ɾ��
		penry_set->endEntrySetScan(entry_scan);

		command_counter_increment();

		entry_scan=penry_set->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//��ʼɨ��
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);//Ԫ���ڵ�һ������getNextһ��
		str=(char*)getdata.getData();//getdata�е����ݸ�str
		length=getdata.getSize();//��С
		int flag;
		flag=memcmp(testdata2,str,sizeof(testdata2));
		if (flag==0)//�����һ��Ԫ�鲻����testdata_1����˵��deleteʹ��һ����¼�������ˣ�getnext������Ӧ����
					//ԭ���ĵڶ���Ԫ�飬��"testdata_2".
		{
			ENDSCAN_CLOSE_COMMIT;
		}
		else
		{
			flag=memcmp(testdata1,str,sizeof(testdata1));
			if (flag==0)
			{
				cout<<"δ�ܳɹ�ɾ����һ��Ԫ��!"<<endl;
				HEAP_RETURN_FALSE_HS;
			}
			else
			{
				cout<<"�ó���������ڶ���Ԫ�鲻��ͬ!"<<endl;
				HEAP_RETURN_FALSE_HS;
			}
		}
	}
	CATCHEXCEPTION
	return true;
}

bool test_Delete_OneTupleFromManyTuple_dll()
{
	try {
		INTENT("������Ԫ�飬����ɾ������һ��Ԫ�顣"
			"���ݸ���������ɨ������ҵ���Ӧ����Ȼ��ɾ����"
			"Ԥ�ڰ����е�testdata_3ɾ��������ѯ���е�Ԫ���е�testdata_3�����ɼ�");
		get_new_transaction();
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
		EntryID tid;
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata1));//����һ���ռ��str...free(str);
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
			if(memcmp(str,testdata3,sizeof(testdata3)) == 0)
			{
				penry_set->deleteEntry(pTransaction,tid);//ɾ��Ԫ��
			}
		}
		penry_set->endEntrySetScan(entry_scan);

		command_counter_increment();

		entry_scan=penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
		int flag=0;
		while (flag==0)
		{
			flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
			str=(char*)getdata.getData();//getdata�е����ݸ�str
			length=getdata.getSize();//��С

			if(memcmp(str,testdata3,sizeof(testdata3)) == 0)
			{
				cout<<"testdata3ɾ�����ɹ�!"<<endl;
				HEAP_RETURN_FALSE_HS;//"testdata_3"�����ڣ�˵������ʧ��
			}
		}
		ENDSCAN_CLOSE_COMMIT;

	} 
	CATCHEXCEPTION
	return true;
}

bool test_Delete_FromGiveSteps_dll()
{
	try {
		INTENT("���Ը��ݸ����Ĳ���ɾ��һ��Ԫ�飬Ԥ��ɾ�����ĸ�Ԫ��,"
				"��ѯ������Ӧ����ԭ���ĵ����Ԫ��");
		get_new_transaction();
		int steps=4;//��������Ϊ4����ɾ�����ĸ������Ԫ��
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
		EntryID tid;
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata1));//����һ���ռ��str...free(str);
		int length;
		int flag;
		while (steps>0)
		{
		
			flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
			if (flag==NO_DATA_FOUND)
			{
				cout<<"�ѵ�����ĩβ!"<<endl;
				HEAP_RETURN_FALSE_HS;
			}
			steps--;
		}
		penry_set->deleteEntry(pTransaction,tid);
		penry_set->endEntrySetScan(entry_scan);
		command_counter_increment();
		entry_scan=penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
		steps=4;
		while (steps>0)
		{
			flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
			steps--;
		}
			str=(char*)getdata.getData();//getdata�е����ݸ�str
			length=getdata.getSize();//��С
			flag=memcmp(str,testdata2,sizeof(testdata2));
			if(flag!=0)
			{
				flag=memcmp(str,testdata1,sizeof(testdata1));
				if (flag==0)
				{
					cout<<"δ��ɾ��Ԫ��testdata_1!"<<endl;
					HEAP_RETURN_FALSE_HS;
				}
				else
				{
					cout<<"��ѯ�����ݲ���ȷ!"<<endl;
					HEAP_RETURN_FALSE_HS;
				}
			}
			ENDSCAN_CLOSE_COMMIT;
	}
	CATCHEXCEPTION;
	return true;
}

bool test_Delete_NoExistTuple_dll()
{
		INTENT("�ֶ�����һ��tid����delteteEntryȥɾ����"
		"�൱��ɾ��һ�������ڵı�����ᱻcatch��");
		get_new_transaction();
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
	try {
		EntryID tid;
		tid.bi_hi=65535;
		tid.bi_lo=65535;
		tid.ip_posid=65535;
		penry_set->deleteEntry(pTransaction,tid);//ɾ�� ������ᱻcatch��
		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		commit_transaction();

		} 
	catch (StorageEngineException &ex) 
	{
		cout << ex.getErrorMsg() << endl;
		pStorageEngine->closeEntrySet(pTransaction,penry_set);//�رձ�
		commit_transaction();//�ύ����,�����ύ�������ɾ����
		return true;
	}
	return false;
}

bool test_DeleteTheSameTuple_Without_commit_dll()
{
	try {
		INTENT("ɾ�������һ��Ԫ�飬��ɾ����command_counter_increment"
			"��ʹ�ú����Ĳ�����������ɾ�������Ǻ�����ɾ��ͬһ�ű����ͬ"
			"һ��Ԫ�顣��������º�����ɾ��Ӧ���ǳɹ��ġ�");
		get_new_transaction();
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
		EntryID tid;
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata1));//����һ���ռ��str
		int length;
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);//Ԫ���ڵ�һ�У���getNextһ��
		str=(char*)getdata.getData();//getdata�е����ݸ�str
		length=getdata.getSize();//��С
		penry_set->deleteEntry(pTransaction,tid);//ɾ��
		penry_set->endEntrySetScan(entry_scan);//ֹͣɨ�裬��ֹͣɨ���޷�ɾ��
		entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
		str=(char*)getdata.getData();//getdata�е����ݸ�str
		length=getdata.getSize();//��С
		int flag;
		flag=memcmp(testdata1,str,sizeof(testdata1));
		if (flag==0)//����Ӧ�ÿ�����ǰ���ɾ�����������Ե�һ��Ԫ����Ȼ��testdata_1
		{
			ENDSCAN_CLOSE_COMMIT;
		}
		else
		{
			flag=memcmp(testdata1,str,sizeof(testdata1));
			if (flag==0)
			{
				cout<<"δ�ܳɹ�ɾ����һ��Ԫ��!"<<endl;
				HEAP_RETURN_FALSE_HS;
			}
			else
			{
				cout<<"�ó���������ڶ���Ԫ�鲻��ͬ!"<<endl;
				HEAP_RETURN_FALSE_HS;
			}
		}
	}
	CATCHEXCEPTION
	return true;
}

int test_DeleteByTupleInsert_GetBlockAndOffset_dll()
{
	try {
		INTENT("����һ��Ψһ��Ԫ�飬�����Ƿ��ڲ���ɹ��󣬸�Ԫ����"
			"�Ѿ���������Ԫ���ڴ����ϵĿ���Ϣ���ò��Լٶ��Ѿ���"
			"���п���Ϣ��Ȼ����ݿ���Ϣ��ȥɾ������Ԫ�顣");
		get_new_transaction();
		EntrySet *penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
		EntryID tid;
		int len = sizeof(testdata1);//�ó����Ѱ���'\0'
		char *insert_str = (char*)malloc(len);
		DataItem entry(insert_str,len);
		penry_set->insertEntry(pTransaction, tid, entry);//���������Ԫ��
		command_counter_increment();
		penry_set->deleteEntry(pTransaction,tid);//ɾ��
		command_counter_increment();
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//��ʼɨ��

		DataItem getdata;
		int flag=0;
		flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
		if (flag==NO_DATA_FOUND)//Ԫ�鱻ɾ���ˣ�û��������
		{
			ENDSCAN_CLOSE_COMMIT;
		}
		else
		{
			cout<<"δ�ܳɹ�ɾ��Ԫ��!"<<endl;
			HEAP_RETURN_FALSE_HS;
		}
	}
	CATCHEXCEPTION
	return true;

}

void delete_data_dll(EntrySet *es, Transaction *trans, const char *det_data, const int det_len)
{
	vector<ScanCondition> vc;
	EntrySetScan *ess = es->startEntrySetScan(trans,BaseEntrySet::SnapshotMVCC,vc);
	DataItem di;
	EntryID ei;
	while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
	{
		if(memcmp(det_data, di.getData(), det_len) == 0)
		{
			es->deleteEntry(trans, ei);
		}
	}
	es->endEntrySetScan(ess);
}

void thread_delete_dll(const EntrySetID eid, const char *det_data, const int det_len, int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	pStorageEngine->beginThread();
	Transaction* trans = NULL;
	EntrySet *es = NULL;
	bool fail = false;
	try {
		trans = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *es = (EntrySet*)pStorageEngine->openEntrySet(trans,EntrySet::OPEN_EXCLUSIVE,eid);
		delete_data_dll(es, trans, det_data, det_len);
		pStorageEngine->closeEntrySet(trans, es);
		trans->commit();
		*sta = 1;
	} catch (StorageEngineException &ex) {
		cout << ex.getErrorMsg() << endl;
		trans->abort();
		*sta = -1;
	}
	pStorageEngine->endThread();
}

void update_data_dll(EntrySet *es, Transaction *trans, const char *det_data, 
										 const int det_len, const char *src_data, const int src_len)
{
	vector<ScanCondition> vc;
	EntrySetScan *ess = es->startEntrySetScan(trans,BaseEntrySet::SnapshotMVCC,vc);
	DataItem di;
	EntryID ei;
	while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
	{
		if(memcmp(src_data, di.getData(), src_len) == 0)
		{
			DataItem tmp_di;
			tmp_di.setSize(det_len);
			tmp_di.setData((void*)det_data);
			es->updateEntry(trans, ei, tmp_di);
		}
	}
	es->endEntrySetScan(ess);
}

void thread_update_dll(const EntrySetID eid, const char *det_data, const int det_len, 
											 const char *src_data, const int src_len, int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	pStorageEngine->beginThread();
	Transaction* trans = NULL;
	EntrySet *es = NULL;
	bool fail = false;
	try {
		trans = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *es = (EntrySet*)pStorageEngine->openEntrySet(trans,EntrySet::OPEN_EXCLUSIVE,eid);
		update_data_dll(es, trans, det_data, det_len, src_data, src_len);
		pStorageEngine->closeEntrySet(trans, es);
		trans->commit();
		*sta = 1;
	} catch (StorageEngineException &ex) {
		cout << ex.getErrorMsg() << endl;
		trans->abort();
		*sta = -1;
	}
	pStorageEngine->endThread();
}

/*
*author :wanghao	
*date:	2012-12-13
*/
//������������д���
bool test_Delete_Shot_Dll()
{
	try
	{
		get_new_transaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt_heap_shot;
		uint32 colid = 82000;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		EntrySetID HeapEntryID= pStorageEngine->createEntrySet(pTransaction,colid);
		EntrySet *pEntry = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HeapEntryID);
		
		ColumnInfo *pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys*sizeof(size_t));
		pIndexCol->col_number[0] = 1;
		pIndexCol->col_number[1] = 2;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys*sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_index_shot;
		uint32 Col_Id = 82030;
		setColumnInfo(Col_Id,pIndexCol);
		EntrySetID m_nIndexEntryID = pStorageEngine->createIndexEntrySet(pTransaction,pEntry,
						BTREE_INDEX_ENTRY_SET_TYPE,Col_Id);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(pTransaction,
								pEntry, EntrySet::OPEN_EXCLUSIVE,
								m_nIndexEntryID);
		
		char strdata[15];
		EntryID  eid;
		vector<EntryID> vInsert;
		for(int i=0; i<10; i++)
		{
			DataItem data;
			memset(strdata,0,15);
			snprintf(strdata,15,"%s-%d",testdata,i);
			data.setData((void*)strdata);
			data.setSize(11);
			pEntry->insertEntry(pTransaction,eid,data);
			vInsert.push_back(eid);
		}
		command_counter_increment();
		
		DataItem Updatedata;
		memset(strdata,0,15);
		snprintf(strdata,15,"%s","teetdata-8");
		Updatedata.setData((void*)strdata);
		Updatedata.setSize(11);
			
		pEntry->updateEntry(pTransaction,vInsert[5],Updatedata);
		memset(strdata,0,15);
		snprintf(strdata,15,"%s","teetdata-2");
		Updatedata.setData((void*)strdata);
		Updatedata.setSize(11);
		pEntry->updateEntry(pTransaction,vInsert[2],Updatedata);
		command_counter_increment();

	
		DataItem Scandata;
		bool find = false;
		SearchCondition cond;
		cond.Add(1,Equal,"te",str_compare);
		
		IndexEntrySetScan *entry_scan = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(pTransaction, 
			BaseEntrySet::SnapshotMVCC, cond.Keys());
		
		EntryIDBitmap* pBitmap = entry_scan->getBitmap(pTransaction);
		EntryIDBitmapIterator* pIter = pBitmap->beginIterate();

		EntryID Eid;
		DataItem DI;
		char* str;
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
			str = NULL;
			str = (char*)DI.getData();
			pEntry->deleteEntry(pTransaction,Eid);
		}
		
		pBitmap->endIterate(pIter);
		entry_scan->deleteBitmap(pBitmap);
		pIndexSet->endEntrySetScan(entry_scan);
		pStorageEngine->closeEntrySet(pTransaction, pIndexSet);
		pStorageEngine->closeEntrySet(pTransaction, pEntry);
		pStorageEngine->removeIndexEntrySet(pTransaction,HeapEntryID,m_nIndexEntryID);
		pStorageEngine->removeEntrySet(pTransaction, HeapEntryID);
		
		commit_transaction();
	}
	catch(StorageEngineException &e)
	{
		//��Ϊ����bitmapɾ����ȷ�Ļ����϶��ᷢ���쳣���������ﷵ��true
		printf("%s", e.getErrorMsg());
		user_abort_transaction();
		return true;
	}
	return false;
}