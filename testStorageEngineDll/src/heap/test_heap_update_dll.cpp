#include <iostream>
#include <list>
#include <algorithm>
#include <boost/assign.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include "boost/thread.hpp"

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "heap/test_heap_update_dll.h"
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

#define testdata1 "testdata_1"
#define testdata2 "testdata_2"
#define testdata3 "testdata_3"
#define testdata  "testdata"

void my_spilt_heap_shot(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.len = 0;
	rangeData.start = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 3;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 5;
	}
}


void my_split_index_shot(RangeDatai& rangeData, const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 3;
	}
}
extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);

bool test_Update_simple_dll()
{
	try {
		INTENT("扫描testdata_3，并将其更新为testdata_1");
		get_new_transaction();
		EntrySet *penry_set = NULL;	
		EntryID tid;//用于接收getNext得到的结构体，这个结构体中存放了元组的位置和偏移量，然后传给Entryupdate
		unsigned int length;
		int status=0;
		int flag=0;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		int count=0;
		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata1));//分配一个空间给str
		while (flag==0)
		{
			flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
			count++;//count记录getNext第几次找到testdata3
			if (flag==NO_DATA_FOUND)
			{
				cout<<"检索至最后的数据了!"<<endl;
				HEAP_RETURN_FALSE_HS;
			}
			str=(char*)getdata.getData();//getdata中的数据给str
			length=getdata.getSize();//大小
			int cmp;
			cmp=memcmp(str,testdata3,sizeof(testdata3));
			if(cmp == 0)
			{
				status=1;
				break;
			}
		}
		
		if(status==0)
		{
			cout<<"未能找到对比的元组!"<<endl;
			HEAP_RETURN_FALSE_HS;
		}
		DataItem data;
		int len = sizeof(testdata1);		
		char *pstr = (char *)malloc(len);//记得释放空间...free(pstr)
		memcpy(pstr, testdata1, len);//构建DataItem
		data.setData((void *)pstr);//pstr中的字符及参数传给data
		data.setSize(len);
		penry_set->updateEntry(pTransaction,tid,data);//更新EntrySet
		penry_set->endEntrySetScan(entry_scan);
		command_counter_increment();
		entry_scan=penry_set->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//开始扫描																							  //entry_scan必须重新赋值，在endEntrySet 中被释放了
		int i;
		for (i=0;i<count;i++)//执行getNext count次，找到准备update元组的位置
		{
			flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
			if (flag==NO_DATA_FOUND)
			{
				cout<<"检索至最后的数据了"<<endl;
				break;
			}
		}
		str=(char*)getdata.getData();//getdata中的数据给str
		length=getdata.getSize();//大小
		flag=memcmp(testdata3,str,sizeof(str));
		if (flag!=0)
		{
			cout<<"数据不相等!update失败!"<<endl;
			HEAP_RETURN_FALSE_HS;
		}
		ENDSCAN_CLOSE_COMMIT;
	}
	CATCHEXCEPTION
	return true;
}

bool test_Update_OnNoExistBlock_dll()
{	
		INTENT("手动构建tid，尝试更新不存在的表");
		get_new_transaction();
		EntrySet *penry_set = NULL;	
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
	try {
		DataItem data;
		EntryID tid;
		tid.bi_hi=65535;
		tid.bi_lo=65535;
		tid.ip_posid=65535;
		penry_set->updateEntry(pTransaction,tid,data);//更新EntrySet
		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		commit_transaction();

	} catch (StorageEngineException &ex) {
		cout << ex.getErrorMsg() << endl;
		pStorageEngine->closeEntrySet(pTransaction,penry_set);
		commit_transaction();
		return true;
	}
	return false;
}

bool test_Update_WithoutCommitManyTimes_dll()
{
	INTENT("多次更新同一个元组，但都不command_counter_increment,"
		"只在最后一次掉用一次。然后查看最后的结果。该测试会报异常。"
		"因为每次更新过后的数据将产生一个新的版本，对于旧的版本，如"
		"果没有调用command_counter_increment则使得旧的版本的更新产生多"
		"个分支，这是不允许的。如果报异常说明测试成功，不报异常则说明"
		"测试失败。");
	get_new_transaction();
	EntrySet *penry_set = NULL;
	penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
	EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//开始扫描
	try
	{
	EntryID tid;//用于接收getNext得到的结构体，这个结构体中存放了元组的位置和偏移量，然后传给Entryupdate
	int flag=0;
	DataItem data;
	int len = sizeof(testdata1);		
	char *pstr = (char *)malloc(len);//记得释放空间...free(pstr)
	memcpy(pstr, testdata1, len);//构建DataItem
	data.setData((void *)pstr);//pstr中的字符及参数传给data
	data.setSize(len);
	DataItem getdata;

		flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
		if (flag==NO_DATA_FOUND)
		{
			cout<<"查询至最后的数据!"<<endl;
			HEAP_RETURN_FALSE_HS;
		}
		int count =10000;
		for (int i=0;i<count;i++)
		{
			penry_set->updateEntry(pTransaction,tid,data);//更新EntrySet,这里的会报错并被catch到
		}
		ENDSCAN_CLOSE_COMMIT;
	} 
	HEAP_RETURN_TRUE_HS;
	return false;	
}

bool test_Update_TupleManyTimes_dll()
{	
	INTENT("多次更新同一个元组，且调用CommandCounterIncrement函数."
			   "。然后查看最后的结果。该测试会报异常。该测试将产生多"
			   "个版本的元组，使得数据文件比逻辑上的大小大很多。");
		get_new_transaction();
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//开始扫描
	try
	{
		EntryID tid;//用于接收getNext得到的结构体，这个结构体中存放了元组的位置和偏移量，然后传给Entryupdate
		int flag=0;
		DataItem data;
		int len = sizeof(testdata1);		
		char *pstr = (char *)malloc(len);//记得释放空间...free(pstr)
		memcpy(pstr, testdata1, len);//构建DataItem
		data.setData((void *)pstr);//pstr中的字符及参数传给data
		data.setSize(len);
		DataItem getdata;

		flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);
		if (flag==NO_DATA_FOUND)
		{
			cout<<"查询至最后的数据!"<<endl;
			HEAP_RETURN_FALSE_HS;
		}
		int count =10000;
		for (int i=0;i<count;i++)
		{
			penry_set->updateEntry(pTransaction,tid,data);//更新EntrySet
			command_counter_increment();
		}
		ENDSCAN_CLOSE_COMMIT;
	} 
	HEAP_RETURN_TRUE_HS;
	return false;
}

static
void rand_get_update(EntrySet *pentry, 
										 Transaction *ptrans,
										 vector<std::pair<EntryID, DataItem> > &v_update,
										 const char *update_data, 
										 bool is_delete = false)
{
	vector<ScanCondition> v_condition;
	EntryID tid;
	DataItem item;
	const int CONDITION_DELETE = 10;
	uint32 count = 0;

	EntrySetScan *esc = pentry->startEntrySetScan(ptrans, EntrySet::SnapshotMVCC, v_condition);
	while(esc->getNext(EntrySetScan::NEXT_FLAG, tid, item) == SUCCESS)
	{
		if(!is_delete)
		{
			if(count++ % CONDITION_DELETE == 0)
			{
				v_update.push_back(std::pair<EntryID, DataItem>(tid, DataItem(NULL, strlen(update_data) + 1, NULL)));
			}
		} else {
			if(strcmp((char*)item.getData(), update_data) == 0)
			{
				v_update.push_back(std::pair<EntryID, DataItem>(tid, DataItem(NULL, strlen(update_data) + 1, NULL)));
			}
		}
	}
	pentry->endEntrySetScan(esc);

	/* 打乱v_update中的数据 */
	random_shuffle(v_update.begin(), v_update.end());

	/* 给v_update赋值 */
	for(int i = 0; i < v_update.size(); ++i)
	{
		v_update[i].second.setData((void*)update_data);
	}
}

static
uint32 check_result_vector(EntrySet *pentry, Transaction *ptrans, vector<std::pair<EntryID, DataItem> > &v_update)
{
	vector<ScanCondition> v_condition;
	EntryID tid;
	DataItem item;
	uint32 count = 0;

	EntrySetScan *esc = pentry->startEntrySetScan(ptrans, EntrySet::SnapshotMVCC, v_condition);
	while(esc->getNext(EntrySetScan::NEXT_FLAG, tid, item) == SUCCESS)
	{
		if(strcmp((char*)item.getData(), (char*)v_update[0].second.getData()) == 0)
		{
			count++;
			item.setData(NULL);
			item.setSize(0);
		}
	}
	pentry->endEntrySetScan(esc);

	return count;
}

bool test_Update_Delete_Insert()
{
	INTENT("测试优化过的EntrySet的update、delete和insert函数。");

	const int ROW_NUM = 1024;
	const char *ROW_DATA = "test_data";
	const char *UPDATE_DATA = "update_data";

	EntrySetID eid = 0;
	EntrySet *penry_set = NULL;
	bool retval = true;
	vector<DataItem> v_data;

	try
	{
		get_new_transaction();
		eid = pStorageEngine->createEntrySet(pTransaction, 0);
		commit_transaction();

		DataItem di;
		di.setSize(strlen(ROW_DATA) + 1);
		di.setData((void*)ROW_DATA);

		/* 构建插入数据 */
		for(int i = 0; i < ROW_NUM; ++i)
		{
			v_data.push_back(di);
		}

		/* 批量插入数据 */
		get_new_transaction();
		penry_set = static_cast<EntrySet *>
			(pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, eid));
		penry_set->insertEntries(pTransaction, v_data);
		pStorageEngine->closeEntrySet(pTransaction, penry_set);
		commit_transaction();

		/* 接下来扫描表，找出需要更新的元组并批量更新 */
		get_new_transaction();

		std::list<std::pair<EntryID, DataItem> > l_update;
		vector<std::pair<EntryID, DataItem> > v_update;

		penry_set = static_cast<EntrySet *>
			(pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, eid));
		rand_get_update(penry_set, pTransaction, v_update, UPDATE_DATA);
		pStorageEngine->closeEntrySet(pTransaction, penry_set);
		commit_transaction();
		
		/* 构造一个list用于批量更新 */
		l_update.assign(v_update.begin(), v_update.end());

		/* 批量更新 */
		get_new_transaction();
		penry_set = static_cast<EntrySet *>
			(pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, eid));
		penry_set->updateEntries(pTransaction, l_update, false);
		pStorageEngine->closeEntrySet(pTransaction, penry_set);

		/* 检测表中是否有等量的更新后的元组 */
		pStorageEngine->endStatement();
		penry_set = static_cast<EntrySet *>
			(pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, eid));
		retval = (check_result_vector(penry_set, pTransaction, v_update) == v_update.size());
		pStorageEngine->closeEntrySet(pTransaction, penry_set);
		commit_transaction();

		if(true == retval)
		{
			/* 这里原先的TID容器已经不可用，需要重新构造 */
			v_update.clear();

			std::list<EntryID> l_delete;

			get_new_transaction();
			penry_set = static_cast<EntrySet *>
				(pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, eid));

			/* 重新获取更新后数据的tid */
			rand_get_update(penry_set, pTransaction, v_update, UPDATE_DATA, true);
			/* 构造新的list */
			for(int i = 0; i < v_update.size(); ++i)
			{
				l_delete.push_back(v_update[i].first);
			}
			penry_set->deleteEntries(pTransaction, l_delete);
			pStorageEngine->closeEntrySet(pTransaction, penry_set);
			commit_transaction();

			get_new_transaction();
			penry_set = static_cast<EntrySet *>
				(pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, eid));
			/* 所有更新的元组已经被删除 */
			retval = (check_result_vector(penry_set, pTransaction, v_update) == 0);
			pStorageEngine->closeEntrySet(pTransaction, penry_set);
			commit_transaction();
		}

	} catch(StorageEngineException &se)
	{
		printf("%s", se.getErrorMsg());
		retval = false;
	}
	
	get_new_transaction();
	pStorageEngine->removeEntrySet(pTransaction, eid);
	commit_transaction();

	return retval;
}

/*
*author :wanghao	
*date:	2012-12-13
*/
using namespace boost;
using boost::detail::spinlock;
static spinlock ShotLock;
static spinlock ShotWaitLock;
EntrySetID HeapEntryID;
EntrySetID m_nIndexEntryID;
bool RunFlag = false;
bool WaitFlag = false;
EntryID GloalEid;

void test_Update_simple_shot_thread03_dll(int *i)
{
	try
	{
		pStorageEngine->beginThread();
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
		HeapEntryID= pStorageEngine->createEntrySet(pTransaction,colid);
		EntrySet *pEntry = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,HeapEntryID);
		
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
		m_nIndexEntryID = pStorageEngine->createIndexEntrySet(pTransaction,pEntry,
						BTREE_INDEX_ENTRY_SET_TYPE,Col_Id);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(pTransaction,
								pEntry, EntrySet::OPEN_SHARED,
								m_nIndexEntryID);
		
		char strdata[15];
		DataItem data;
		EntryID  eid;
		for(int i=0; i<10; i++)
		{
			memset(strdata,0,15);
			snprintf(strdata,15,"%s-%d",testdata,i);
			strdata[10] = 0;
			data.setData((void*)strdata);
			data.setSize(11);
			pEntry->insertEntry(pTransaction,eid,data);
			//if (5 == i)
			//	GloalEid = eid;
		}
		pStorageEngine->closeEntrySet(pTransaction, pIndexSet);
		pStorageEngine->closeEntrySet(pTransaction, pEntry);
		commit_transaction();
	}
	catch(StorageEngineException &e)
	{
		//因为上面bitmap更新正确的话，肯定会发生异常，所以这里返回true
		printf("%s", e.getErrorMsg());
		user_abort_transaction();
		*i = 0;
	}
	*i = 1;
	pStorageEngine->endThread();
}
void test_Update_simple_shot_thread01_dll(int *i)
{
	Transaction *pTrans = NULL;
	try
	{
		pStorageEngine->beginThread();
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		pTrans = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);//获得一个事务
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,HeapEntryID);
		IndexEntrySet* pIndexSetSecond = pStorageEngine->openIndexEntrySet(pTrans,
								pEntrySet, EntrySet::OPEN_SHARED,
								m_nIndexEntryID);
		DataItem Updatedata;
		char strdata[15];
		EntryID  eid;
		memset(strdata,0,15);
		snprintf(strdata,15,"%s","teetnata-8");
		strdata[10] = 0;
		Updatedata.setData((void*)strdata);
		Updatedata.setSize(11);
		pEntrySet->insertEntry(pTrans,GloalEid,Updatedata);
		command_counter_increment();
		
		/*DataItem Scandata;
		bool find = false;
		SearchCondition cond;
		cond.Add(1,Equal,"te",str_compare);
		
		IndexEntrySetScan *entry_scan = (IndexEntrySetScan*)pIndexSetSecond->startEntrySetScan(pTrans, 
			BaseEntrySet::SnapshotMVCC, cond.Keys());
		
		EntryIDBitmap* pBitmap = entry_scan->getBitmap(pTrans);
		EntryIDBitmapIterator* pIter = pBitmap->beginIterate();
		EntryIDBitmapIterationResult* pResult = 0;
		BitmapFetch BitpFetch = pEntrySet->startBitmapFetch(BaseEntrySet::SnapshotMVCC);

		EntryID Eid;
		DataItem DI;
		char* str;
		while(pResult = pIter->next())
		{
			for (int i = 0; i < pResult->offsets.size(); i++)
			{
				pEntrySet->fetchEntry(BitpFetch,pResult->blockno,
							pResult->offsets[i],Eid,DI);
				str = NULL;
				str = (char*)DI.getData();
				if (0 == memcmp(str,"teetnata-8",10))
				{
					GloalEid = Eid;
				}
			}
		}
		pEntrySet->endBitmapFetch(BitpFetch);
		pBitmap->endIterate(pIter);
		entry_scan->deleteBitmap(pBitmap);
		pIndexSetSecond->endEntrySetScan(entry_scan);*/

		pStorageEngine->closeEntrySet(pTrans, pIndexSetSecond);
		pStorageEngine->closeEntrySet(pTrans, pEntrySet);
		pTrans->commit();//提交事务
		
		ShotLock.lock();
		RunFlag = true;
		ShotLock.unlock();

		/*while(1)
		{
			ShotWaitLock.lock();
			if (WaitFlag)
			{
				WaitFlag = false;
				ShotWaitLock.unlock();
				break;
			}
			else
			{
				ShotWaitLock.unlock();
				MySleep(500);
			}
		}*/
	}
	catch(StorageEngineException &e)
	{
		//因为上面bitmap更新正确的话，肯定会发生异常，所以这里返回true
		printf("%s", e.getErrorMsg());
		user_abort_transaction();
		*i = 0;
	}
	*i = 1;
	pStorageEngine->endThread();
}

void test_Update_simple_shot_thread02_dll(int *i)
{
	Transaction *pTrans = NULL;
	try
	{
		pStorageEngine->beginThread();
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		pTrans = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);//获得一个事务
		
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,HeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(pTrans,
								pEntrySet, EntrySet::OPEN_SHARED,
								m_nIndexEntryID);
		DataItem Scandata;
		bool find = false;
		SearchCondition cond;
		cond.Add(1,Equal,"te",str_compare);
		
		IndexEntrySetScan *entry_scan = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(pTrans, 
			BaseEntrySet::SnapshotMVCC, cond.Keys());
		
		EntryIDBitmap* pBitmap = entry_scan->getBitmap(pTrans);
		EntryIDBitmapIterator* pIter = pBitmap->beginIterate();

		while(1)
		{
			ShotLock.lock();
			if (RunFlag)
			{
				RunFlag = false;
				ShotLock.unlock();
				break;
			}
			else
			{
				ShotLock.unlock();
				MySleep(500);
			}
		}
		
		DataItem Updatedata;
		EntryID Eid;
		DataItem DI;
		char* str;
		char strdata[15];
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
			str = NULL;
			str = (char*)DI.getData();
			if (0 == memcmp(str,"teetnata-8",10))
			{
				memset(strdata,0,15);
				snprintf(strdata,15,"%s","teetffff-8");
				Updatedata.setData((void*)strdata);
				Updatedata.setSize(11);
				pEntrySet->updateEntry(pTrans,Eid,Updatedata,false);
			}
		}
		
		pBitmap->endIterate(pIter);
		entry_scan->deleteBitmap(pBitmap);
		pIndexSet->endEntrySetScan(entry_scan);

		/*DataItem Updatedata;
		char strdata[15];
		memset(strdata,0,15);
		snprintf(strdata,15,"%s","teetgggg-8");
		Updatedata.setData((void*)strdata);
		Updatedata.setSize(11);
		pEntrySet->updateEntry(pTrans,GloalEid,Updatedata,false,true);*/
					
		pStorageEngine->closeEntrySet(pTrans, pIndexSet);
		pStorageEngine->closeEntrySet(pTrans, pEntrySet);
		//pStorageEngine->removeIndexEntrySet(pTrans,HeapEntryID,m_nIndexEntryID);
		//pStorageEngine->removeEntrySet(pTrans, HeapEntryID);
		pTrans->commit();//提交事务
	}
	catch(StorageEngineException &e)
	{
		//因为上面bitmap更新正确的话，肯定会发生异常，所以这里返回true
		printf("%s", e.getErrorMsg());
		pTrans->abort();
		/*ShotWaitLock.lock();
		WaitFlag = true;
		ShotWaitLock.unlock();*/
		*i = 1;
	}
	*i = 1;
	pStorageEngine->endThread(); 
}

#define THREAD_NUM_2 3
//这个测试用例有错误
bool test_Update_simple_shot_dll()
{
    INTENT("多个线程同时进行index创建操作");
    int sta[THREAD_NUM_2] = {0};

	thread_group tg;
	tg.create_thread(bind(&test_Update_simple_shot_thread03_dll, &sta[2]));
	tg.join_all();

	tg.create_thread(bind(&test_Update_simple_shot_thread02_dll, &sta[1]));
	tg.create_thread(bind(&test_Update_simple_shot_thread01_dll, &sta[0]));
	tg.join_all();


    int found = 0;
    for (int i = 0; i < THREAD_NUM_2; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_2) {
        return true;
    } 
    return false;
}

