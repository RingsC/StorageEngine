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
		EID = pStorageEngine->createEntrySet(pTransaction, colid, 0,NULL);//创建一个heap
		
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
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
		pStorageEngine->closeEntrySet(pTransaction,penry_set);//关闭表
		commit_transaction();//提交事务
		
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
		INTENT("插入一个单列元组，测试删除数据表中第一个元组,"
				"预计查出来的数据看不到第一个元组，而是第二个元组");
		get_new_transaction();
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		EntryID tid;
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata1));//分配一个空间给str
		int length;
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);//元组在第一列，故getNext一次
		str=(char*)getdata.getData();//getdata中的数据给str
		length=getdata.getSize();//大小
		penry_set->deleteEntry(pTransaction,tid);//删表
		penry_set->endEntrySetScan(entry_scan);

		command_counter_increment();

		entry_scan=penry_set->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//开始扫描
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);//元组在第一个，故getNext一次
		str=(char*)getdata.getData();//getdata中的数据给str
		length=getdata.getSize();//大小
		int flag;
		flag=memcmp(testdata2,str,sizeof(testdata2));
		if (flag==0)//如果第一个元组不再是testdata_1，则说明delete使第一条记录看不到了，getnext看到的应该是
					//原来的第二条元组，即"testdata_2".
		{
			ENDSCAN_CLOSE_COMMIT;
		}
		else
		{
			flag=memcmp(testdata1,str,sizeof(testdata1));
			if (flag==0)
			{
				cout<<"未能成功删除第一个元组!"<<endl;
				HEAP_RETURN_FALSE_HS;
			}
			else
			{
				cout<<"得出的数据与第二个元组不相同!"<<endl;
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
		INTENT("插入多个元组，测试删除其中一种元组。"
			"根据给定的数据扫描表，查找到相应数据然后删除。"
			"预期把所有的testdata_3删除，最后查询所有的元组中的testdata_3都不可见");
		get_new_transaction();
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		EntryID tid;
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata1));//分配一个空间给str...free(str);
		unsigned int length;
		while (true)
		{
			int flag;
			flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
			if (flag==NO_DATA_FOUND)
			{
				break;
			}
			str=(char*)getdata.getData();//getdata中的数据给str
			length=getdata.getSize();//大小			
			if(memcmp(str,testdata3,sizeof(testdata3)) == 0)
			{
				penry_set->deleteEntry(pTransaction,tid);//删除元组
			}
		}
		penry_set->endEntrySetScan(entry_scan);

		command_counter_increment();

		entry_scan=penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		int flag=0;
		while (flag==0)
		{
			flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
			str=(char*)getdata.getData();//getdata中的数据给str
			length=getdata.getSize();//大小

			if(memcmp(str,testdata3,sizeof(testdata3)) == 0)
			{
				cout<<"testdata3删除不成功!"<<endl;
				HEAP_RETURN_FALSE_HS;//"testdata_3"还存在，说明测试失败
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
		INTENT("测试根据给定的步数删除一个元组，预期删除第四个元组,"
				"查询出来的应该是原来的第五个元组");
		get_new_transaction();
		int steps=4;//给定步数为4，即删除第四个插入的元组
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		EntryID tid;
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata1));//分配一个空间给str...free(str);
		int length;
		int flag;
		while (steps>0)
		{
		
			flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
			if (flag==NO_DATA_FOUND)
			{
				cout<<"已到达表的末尾!"<<endl;
				HEAP_RETURN_FALSE_HS;
			}
			steps--;
		}
		penry_set->deleteEntry(pTransaction,tid);
		penry_set->endEntrySetScan(entry_scan);
		command_counter_increment();
		entry_scan=penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		steps=4;
		while (steps>0)
		{
			flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
			steps--;
		}
			str=(char*)getdata.getData();//getdata中的数据给str
			length=getdata.getSize();//大小
			flag=memcmp(str,testdata2,sizeof(testdata2));
			if(flag!=0)
			{
				flag=memcmp(str,testdata1,sizeof(testdata1));
				if (flag==0)
				{
					cout<<"未能删除元组testdata_1!"<<endl;
					HEAP_RETURN_FALSE_HS;
				}
				else
				{
					cout<<"查询的数据不正确!"<<endl;
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
		INTENT("手动构建一个tid，用delteteEntry去删除它"
		"相当于删除一个不存在的表，出错会被catch到");
		get_new_transaction();
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
	try {
		EntryID tid;
		tid.bi_hi=65535;
		tid.bi_lo=65535;
		tid.ip_posid=65535;
		penry_set->deleteEntry(pTransaction,tid);//删表 在这里会被catch到
		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		commit_transaction();

		} 
	catch (StorageEngineException &ex) 
	{
		cout << ex.getErrorMsg() << endl;
		pStorageEngine->closeEntrySet(pTransaction,penry_set);//关闭表
		commit_transaction();//提交事务,必须提交事务才能删除表
		return true;
	}
	return false;
}

bool test_DeleteTheSameTuple_Without_commit_dll()
{
	try {
		INTENT("删除表里的一个元组，但删除后不command_counter_increment"
			"，使得后续的操作看不到该删除。但是后面再删除同一张表里的同"
			"一个元组。正常情况下后续的删除应该是成功的。");
		get_new_transaction();
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		EntryID tid;
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata1));//分配一个空间给str
		int length;
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);//元组在第一列，故getNext一次
		str=(char*)getdata.getData();//getdata中的数据给str
		length=getdata.getSize();//大小
		penry_set->deleteEntry(pTransaction,tid);//删表
		penry_set->endEntrySetScan(entry_scan);//停止扫描，不停止扫描无法删表
		entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
		str=(char*)getdata.getData();//getdata中的数据给str
		length=getdata.getSize();//大小
		int flag;
		flag=memcmp(testdata1,str,sizeof(testdata1));
		if (flag==0)//现在应该看不到前面的删除操作，所以第一个元组依然是testdata_1
		{
			ENDSCAN_CLOSE_COMMIT;
		}
		else
		{
			flag=memcmp(testdata1,str,sizeof(testdata1));
			if (flag==0)
			{
				cout<<"未能成功删除第一个元组!"<<endl;
				HEAP_RETURN_FALSE_HS;
			}
			else
			{
				cout<<"得出的数据与第二个元组不相同!"<<endl;
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
		INTENT("插入一个唯一的元组，测试是否在插入成功后，该元组中"
			"已经保存有新元组在磁盘上的块信息。该测试假定已经保"
			"存有块信息。然后根据块信息再去删除掉该元组。");
		get_new_transaction();
		EntrySet *penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
		EntryID tid;
		int len = sizeof(testdata1);//该长度已包含'\0'
		char *insert_str = (char*)malloc(len);
		DataItem entry(insert_str,len);
		penry_set->insertEntry(pTransaction, tid, entry);//构建插入的元组
		command_counter_increment();
		penry_set->deleteEntry(pTransaction,tid);//删表
		command_counter_increment();
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描

		DataItem getdata;
		int flag=0;
		flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
		if (flag==NO_DATA_FOUND)//元组被删除了，没有数据了
		{
			ENDSCAN_CLOSE_COMMIT;
		}
		else
		{
			cout<<"未能成功删除元组!"<<endl;
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
//这个测试用例有错误
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
		//因为上面bitmap删除正确的话，肯定会发生异常，所以这里返回true
		printf("%s", e.getErrorMsg());
		user_abort_transaction();
		return true;
	}
	return false;
}