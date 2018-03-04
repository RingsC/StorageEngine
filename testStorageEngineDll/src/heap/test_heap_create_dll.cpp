#include <iostream>
#include <vector>
//#include "boost/thread/thread.hpp"
#include "boost/thread.hpp" 
#include  "boost/thread/xtime.hpp" 
#include  "boost/thread/tss.hpp"

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "heap/test_heap_create_dll.h"
#include "utils/utils_dll.h"
#include "test_fram.h"
#include "sequence/utils.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace FounderXDB::StorageEngineNS;
using namespace boost;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
extern EntrySetID EID;
EntrySetID indexIdCheck = 0;
bool flag = false;
bool insert = false;

#define testdata "testdata_1"
bool test_heap_create_dll()
{
	try {
		INTENT("通过插入数据并查出来确认表已经创建成功");

		get_new_transaction();//创建一个事务 
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表,transaction传NULL
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
		unsigned int length;
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);//暂时通过getdata判断后面是否还有数据，以后可以通过返回的int是否为NULL判断
		str=(char*)getdata.getData();//getdata中的数据给str
		length=getdata.getSize();//大小

		CHECK_BOOL(length!=0);
		if (length==0)
		{
			cout<<"查询的数据长度为0，出错!"<<endl;
			HEAP_RETURN_FALSE_HS;
		}
		int flag=0;
		flag=memcmp(testdata,str,sizeof(str));
		CHECK_BOOL(flag==0);
		if (flag!=0)
		{
			cout<<"查询的数据与预测的不同，出错!"<<endl;
			HEAP_RETURN_FALSE_HS;
		}

		ENDSCAN_CLOSE_COMMIT;
	}
	CATCHEXCEPTION;
	return true;
}


/////多线程创建heap///////
vector<EntrySetID> entrySetIdVec;
//#define THREAD_NUM_10 10

bool test_thread_create_index_check_valid_1()
{
    INTENT("多个线程同时进行index创建操作");
    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);
    int sta = 0;
	int sta1 = 0;
	int sta2 = 0;
	
	EntrySetID heapId = {0};
	//EntrySetID indexId = {0};
	int found = 0;
	thread_group tg;

	tg.create_thread(bind(&thread_heap_create, &heapId, &sta));
	tg.join_all();
	if (sta) {
		++found;
	}

	tg.create_thread(bind(&thread_index_create_check_unique_1, heapId, &sta1));

	tg.create_thread(bind(&thread_index_create_check_valid_1, heapId, &sta2));

	tg.join_all();

	if (sta1) {
		++found;
	}
	if (sta2){
		++found;
		sta2 = 0;
	}
	tg.create_thread(bind(&thread_heap_del, heapId, &sta2));
	tg.join_all();

	if (sta2) {
		++found;
	}
	if (found == 4) {
		return true;
	}
	return false;
}

bool thrfun_heap_create(Transaction* txn)
{

    EntrySetID heap_id = pStorageEngine->createEntrySet(txn,1);
    //EntrySetID heap_id = heap_entry->getId();
    //heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id));
    //pStorageEngine->closeEntrySet(pTransaction, heap_entry);
    //entrySetIdVec.push_back(heap_entry->getId());
    //cout << "heap id:" << heap_entry->getId() << endl;
    pStorageEngine->endStatement();

    pStorageEngine->removeEntrySet(txn, heap_id);

    return true;
}

bool thrfun_heap_close(Transaction* txn,EntrySetID heap_id)
{
	std::cout<<"\ntransaction id : "<<txn->getTransactionId()<<"\n"<<std::endl;

    EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heap_id));

	std::cout<<"thrfun_heap_close(Transaction* txn,EntrySetID heap_id) openEntrySet success !\n"<<std::endl;

    pStorageEngine->closeEntrySet(txn, heap_entry);

	std::cout<<"pStorageEngine->closeEntrySet(txn, heap_entry) !\n"<<std::endl;

    return true;   
}

bool thrfun_index_open_indid_heapid(EntrySetID &heapid, EntrySetID &indid)
{
	EntrySet *heap_entry = NULL;

	heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heapid));

    IndexEntrySet *index_entry = NULL;
	index_entry = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(pTransaction,heap_entry,EntrySet::OPEN_EXCLUSIVE,indid));
    return true;
}

bool thrfun_index_open_indid(EntrySetID &indid)
{

    IndexEntrySet *index_entry = NULL;
	index_entry = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(pTransaction,NULL,EntrySet::OPEN_EXCLUSIVE,indid));
    return true;
}

bool thrfun_index_create_indid_heapid(EntrySetID &heapid, EntrySetID &indid)
{
    EntrySetID heap_id = pStorageEngine->createEntrySet(pTransaction,1);
	heapid = heap_id;
	EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id);
    indid = pStorageEngine->createIndexEntrySet(pTransaction, pEntrySet, BTREE_INDEX_ENTRY_SET_TYPE,GetSingleColInfo());

    return true;
}

bool thrfun_index_create_indid(EntrySetID &indid)
{
    EntrySetID heap_id = pStorageEngine->createEntrySet(pTransaction,1);
    //cout << "heap id:" << heap_entry->getId() << endl;
	EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id);

	indid = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,GetSingleColInfo());

    return true;
}


bool thrfun_index_create_del()
{
    EntrySetID heap_id = pStorageEngine->createEntrySet(pTransaction,1);
    //cout << "heap id:" << heap_entry->getId() << endl;
	EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id);
   
    EntrySetID indid = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,GetSingleColInfo());

	pStorageEngine->removeIndexEntrySet(pTransaction, heap_id, indid);
    pStorageEngine->removeEntrySet(pTransaction, heap_id);
    return true;
}

bool thrfun_index_create(Transaction* pTransaction)
{
    EntrySetID heap_id = pStorageEngine->createEntrySet(pTransaction,1);
    cout << "\nheap id:" << heap_id<<"\n"<<endl;
	EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id);

    for (int i = 0; i < 10; i++){
         EntrySetID indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,GetSingleColInfo());
		 cout << "\nindex id_" <<i<<" : "<< indexId <<"\n"<< endl;
    }
	pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
    pStorageEngine->removeEntrySet(pTransaction, heap_id);
    return true;
}

void thread_index_getNext_large(EntrySetID heap_id,EntrySetID index_id, EntryID eid, DataItem *data, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        inser_flag = index_getNext_large(pTransaction,heap_id, index_id, *data, eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread();  
}

void thread_index_getNext(EntrySetID heap_id,EntrySetID index_id, EntryID eid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        inser_flag = index_getNext(pTransaction,heap_id, index_id, eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread();     
}


void thread_index_find(EntrySetID heap_id,EntrySetID index_id, EntryID eid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        inser_flag = index_find(pTransaction,heap_id, index_id, eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread(); 
}

void thread_heap_getNext_large(EntrySetID heap_id,EntryID eid, DataItem *data, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        inser_flag = heap_getNext_large(pTransaction,heap_id, *data, eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread();
}

void thread_heap_getNext(EntrySetID heap_id,EntryID eid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        inser_flag = heap_getNext(pTransaction,heap_id, eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread(); 
}


void thread_heap_find_large(EntrySetID heap_id,EntryID eid, DataItem *data, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        //inser_flag = heap_find(heap_id, eid);
        inser_flag = heap_find_large(pTransaction,heap_id, *data, eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread(); 
}
void thread_heap_find_chars(EntrySetID heap_id,EntryID eid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        //inser_flag = heap_find(heap_id, eid);
        inser_flag = heap_find_chars(pTransaction,heap_id, eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread(); 
}

void thread_heap_find(EntrySetID heap_id,EntryID eid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		std::cout<<"\nStart new transaction success !\n"<<std::endl;
        inser_flag = heap_find(pTransaction,heap_id, eid);
        if(!inser_flag) {
            fail = true;
        }
		std::cout<<"\nheap_find() excute successfully !\n"<<std::endl;
        pTransaction->commit();
		std::cout<<"\nTransaction commit !\n"<<std::endl;
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread(); 
}

void thread_heap_update_large(EntrySetID heap_id,int len, EntryID *eid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        inser_flag = heap_update_large(pTransaction,heap_id, len, *eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread();     
}

void thread_heap_update(EntrySetID heap_id,EntryID eid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        inser_flag = heap_update(pTransaction,heap_id, eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread(); 
}

void thread_heap_dele(EntrySetID heap_id,EntryID eid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        inser_flag = heap_dele(pTransaction,heap_id, eid);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread(); 
}

void thread_heap_update_large_find(EntrySetID heap_id, int len, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		//for (int i = 0; i < HEAP_CNT; i++) {
		for (int i = 0; i < 1; i++) {
			inser_flag = heap_insert_update_large_find(pTransaction,heap_id, len);
			if(!inser_flag) {
				fail = true;
				break;
			}
		} //for
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread();
}


void thread_heap_insert_large_find(EntrySetID heap_id, int len, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		for (int i = 0; i < HEAP_CNT; i++) {
			inser_flag = heap_insert_large_find(pTransaction,heap_id, len);
			if(!inser_flag) {
				fail = true;
				break;
			}
		} //for
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
	else
	{
		*i = 0;
	}
    
    pStorageEngine->endThread();  
}

void thread_heap_insert_large(EntrySetID heap_id, int len, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		for (int i = 0; i < LOOP_TIMES; i++) {
			inser_flag = heap_insert_large(pTransaction,heap_id, len);
			if(!inser_flag) {
				fail = true;
				break;
			}
		} //for
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread();  
}

void thread_heap_insert(EntrySetID heap_id, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    bool inser_flag = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        inser_flag = heap_insert(pTransaction,heap_id);
        if(!inser_flag) {
            fail = true;
        }
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;   
    }
    if(!fail){
        *i = 1;
    }
    
    pStorageEngine->endThread();  
}


void thread_heap_del(EntrySetID heap_id, int *i)
{
    Transaction* pTransaction = NULL;
    bool success = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		pStorageEngine->removeEntrySet(pTransaction, heap_id);


        pTransaction->commit();
    } catch (StorageEngineException &ex) {
       cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        success = true;   
    }
    if(!success){
        *i = 1;
    } else {
        *i = 0;
    } 
    
    pStorageEngine->endThread();    
}


void thread_heap_close(EntrySetID heap_id, int *i)
{
    Transaction* pTransaction = NULL;
    bool success = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		std::cout<<"start new transaction success !\n"<<std::endl;
        thrfun_heap_close(pTransaction,heap_id);
        
		std::cout<<"\npTransaction->Id = "<<pTransaction->getTransactionId()<<std::endl;
        pTransaction->commit();
		std::cout<<"transaction commit !\n"<<std::endl;
    } catch (StorageEngineException &ex) {
		std::cout<<"\nthread_heap_close(EntrySetID heap_id, int *i) exception !\n"<<std::endl;
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
			std::cout<<"transaction abort !\n"<<std::endl;
        }
        success = true;   
    }
    if(!success){
        *i = 1;
    } else {
        *i = 0;
    }
    
    pStorageEngine->endThread();
}


void thread_heap_create( EntrySetID *heapId, int *i)
{
    Transaction* pTransaction = NULL;
    bool success = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		*heapId = pStorageEngine->createEntrySet(pTransaction,1);

		//pStorageEngine->removeEntrySet(pTransaction, *heapId );
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        success = true; 
    }

    if(success) {
        *i = 0;
    } else {
        *i = 1;
    }
    
    pStorageEngine->endThread();  
}

void thread_heap_create( int *i)
{
    Transaction* pTransaction = NULL;
    bool success = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		for (int i = 0; i < HEAP_CNT; i ++) {
			thrfun_heap_create(pTransaction);
		}
        
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        success = true; 
    }

    if(success) {
        *i = 0;
    } else {
        *i = 1;
    }
    
    pStorageEngine->endThread();    
}

//void thread_heap_create(int &i)
//{
//    Transaction* pTransaction = NULL;
//    bool success = false;
//    try {
//        pStorageEngine->beginThread();
//        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
//        thrfun_heap_create();
//        
//        pTransaction->commit();
//    } catch (StorageEngineException &ex) {
//        cout << ex.getErrorMsg() << endl;
//        if (pTransaction != NULL) {
//            pTransaction->abort();
//        }
//        success = true; 
//    }
//
//    if(success) {
//        i = 0;
//    } else {
//        i = 1;
//    }
//    
//    pStorageEngine->endThread();
//
//}

void thread_index_create_open_heapid(EntrySetID &heapid, EntrySetID &indid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        thrfun_index_open_indid_heapid(heapid, indid);
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void thread_index_create_open(EntrySetID &indid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        thrfun_index_open_indid(indid);
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
       cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
		throw;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void thread_index_create_indid_heapid(EntrySetID *heapid, EntrySetID *indid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        thrfun_index_create_indid_heapid(*heapid, *indid);
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}


void thread_index_create_indid(EntrySetID *indid, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        thrfun_index_create_indid(*indid);
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void thread_index_create_del(int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        thrfun_index_create_del();
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void thread_index_del(EntrySetID heapId, EntrySetID indexId, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		pStorageEngine->removeIndexEntrySet(pTransaction, heapId, indexId);
		pStorageEngine->removeEntrySet(pTransaction, heapId);
 
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void   Sleep(int   secs) 
{ 
	boost::xtime   xt; 
	boost::xtime_get(&xt,   boost::TIME_UTC_); //得到当前时间点xt. 
	xt.sec   +=   secs; //在当前时间点上加上secs秒,得到一个新的时间点, 
	//然后使线程休眠到指定的时间点xt. 
	boost::thread::sleep(xt);   //   sleep   for   secs   second; 
} 


void thread_index_create_check_unique_1(EntrySetID heapId,int *i)
{
	Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		///insert dataItem
		EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,heapId));
    

		DataItem data;
		formDataItem(LOOP_TIMES, data);
		EntryID eid_one = {0};
		EntryID eid_sec = {0};
		try {
			heap_entry->insertEntry(pTransaction, eid_one, data);
			heap_entry->insertEntry(pTransaction, eid_sec, data);
			insert = true;

		} catch (StorageEngineException &ex) {
			cout << ex.getErrorMsg() << endl;
			deformDataItem(&data);
			pStorageEngine->closeEntrySet(pTransaction, heap_entry);
			throw;
        
		}
		

		pStorageEngine->closeEntrySet(pTransaction, heap_entry);
		pStorageEngine->endStatement();

		while(!flag){
			continue;
		}

		//使用索引查找data
		
		//EntrySet *heap_entry = NULL;
        IndexEntrySet *index_entry = NULL;

		heap_entry = (pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heapId));

		EntrySetID indexId ;

        try {
            index_entry = (pStorageEngine->openIndexEntrySet(pTransaction,heap_entry,EntrySet::OPEN_EXCLUSIVE,indexIdCheck));
        } catch (StorageEngineException &) {
            pStorageEngine->closeEntrySet(pTransaction, heap_entry);
            throw;
        }

		ScanCondition cond(1, ScanCondition::Equal, (se_uint64)(data.getData()), data.getSize(), wfh_compare);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = index_entry->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vscans);
        } catch (StorageEngineException &) {
            pStorageEngine->closeEntrySet(pTransaction, heap_entry);
            pStorageEngine->closeEntrySet(pTransaction, index_entry);
            fail = true;
        }
        EntryID eid_find = {0};
        DataItem entry;
        se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::PREV_FLAG, eid_find, entry);
            if(NO_DATA_FOUND == ret) {

                fail = true;
            }

        } catch (StorageEngineException &ex) {
            cout << ex.getErrorMsg() << endl;
            index_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(pTransaction, heap_entry);
            pStorageEngine->closeEntrySet(pTransaction, index_entry);
            throw;
        }

        index_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(pTransaction, heap_entry);
        pStorageEngine->closeEntrySet(pTransaction, index_entry);


		if (checkDataItem_full(&data, &entry)){
			fail = false;
		}

        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void thread_index_create_check_unique(EntrySetID *heapId,int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

        EntrySetID heap_id = pStorageEngine->createEntrySet(pTransaction,1);
 		//IndexEntrySet *index_entry = index_entry = pStorageEngine->createIndexEntrySet(pTransaction, heap_entry, UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE,"index", 0, 2);
		//*heapId = heap_entry->getId();

		///insert dataItem
		EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id));
    

		DataItem data;
		formDataItem(LOOP_TIMES, data);
		EntryID eid_one = {0};
		EntryID eid_sec = {0};
		try {
			heap_entry->insertEntry(pTransaction, eid_one, data);
			heap_entry->insertEntry(pTransaction, eid_sec, data);

		} catch (StorageEngineException &ex) {
			cout << ex.getErrorMsg() << endl;
			deformDataItem(&data);
			pStorageEngine->closeEntrySet(pTransaction, heap_entry);
			throw;
        
		}
		pStorageEngine->closeEntrySet(pTransaction, heap_entry);
		pStorageEngine->endStatement();

		//此处使线程睡眠


		while(!flag){
			continue;
		}
		//Sleep(100);

		//使用索引查找data
		
		//EntrySet *heap_entry = NULL;
        IndexEntrySet *index_entry = NULL;

		heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id));

        try {
            index_entry = (pStorageEngine->openIndexEntrySet(pTransaction,heap_entry,EntrySet::OPEN_EXCLUSIVE,indexIdCheck));
        } catch (StorageEngineException &) {
            pStorageEngine->closeEntrySet(pTransaction, heap_entry);
            throw;
        }

		ScanCondition cond(1, ScanCondition::Equal, (se_uint64)(data.getData()), data.getSize(), wfh_compare);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = index_entry->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vscans);
        } catch (StorageEngineException &) {
            pStorageEngine->closeEntrySet(pTransaction, heap_entry);
            pStorageEngine->closeEntrySet(pTransaction, index_entry);
            fail = true;
        }
        EntryID eid_find = {0};
        DataItem entry;
        //se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::PREV_FLAG, eid_find, entry);
            if(NO_DATA_FOUND == ret) {
                index_entry->endEntrySetScan(pscan);
                pStorageEngine->closeEntrySet(pTransaction, heap_entry);
                pStorageEngine->closeEntrySet(pTransaction, index_entry);
                fail = true;
            }

        } catch (StorageEngineException &ex) {
            cout << ex.getErrorMsg() << endl;
            index_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(pTransaction, heap_entry);
            pStorageEngine->closeEntrySet(pTransaction, index_entry);
            throw;
        }

        index_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(pTransaction, heap_entry);
        pStorageEngine->closeEntrySet(pTransaction, index_entry);


		if (checkDataItem_full(&data, &entry)){
			fail = false;
		}

        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void thread_index_create_unique(int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

        EntrySetID heap_id = pStorageEngine->createEntrySet(pTransaction,1);
		EntrySet* heap_entry = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id);
		EntrySetID index_id = pStorageEngine->createIndexEntrySet(pTransaction,heap_entry
			,UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE
			,GetSingleColInfo());
		
		
		pStorageEngine->endStatement();
		///insert dataItem
		heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id));
    

		DataItem data;
		formDataItem(LOOP_TIMES, data);
		EntryID eid_one = {0};
		EntryID eid_sec = {0};
		try {
			heap_entry->insertEntry(pTransaction, eid_one, data);
			heap_entry->insertEntry(pTransaction, eid_sec, data);

		} catch (StorageEngineException &ex) {
			cout << ex.getErrorMsg() << endl;
			deformDataItem(&data);
			pStorageEngine->closeEntrySet(pTransaction, heap_entry);
			fail = true;
			throw;
        }

		pStorageEngine->removeEntrySet(pTransaction, heap_entry->getId());
        pTransaction->commit();

    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void thread_index_create_check_valid_1(EntrySetID heapId, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;

	while(!insert){
			continue;
		}

    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_SHARED,heapId));

		EntrySetID index_id = pStorageEngine->createIndexEntrySet(pTransaction,heap_entry
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetMultiCollInfo());

		//*heapId = heap_entry->getId();
		indexIdCheck = index_id;

        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
		flag = true;
    }
    pStorageEngine->endThread();
}

void thread_index_create_check_valid(EntrySetID heapId, EntrySetID *indexId, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heapId));

		EntrySetID index_id = pStorageEngine->createIndexEntrySet(pTransaction,heap_entry
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetMultiCollInfo());

		//*heapId = heap_entry->getId();
		*indexId =index_id;

        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
		flag = true;
    }
    pStorageEngine->endThread();
}

void thread_index_create(EntrySetID *heapId, EntrySetID *indexId, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

        EntrySetID heap_id = pStorageEngine->createEntrySet(pTransaction,1);
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heap_id);
		EntrySetID index_id = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetSingleColInfo());

		*heapId = heap_id;
		*indexId = index_id;

        pTransaction->commit();
   } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void thread_index_create(int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		std::cout<<"\nstart transaction success !\n"<<std::endl;
        thrfun_index_create(pTransaction);
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
		std::cout<<"\nthread_index_create(int* i) error\n"<<std::endl;
        cout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(fail) {
        *i = 0;
    } else {
        *i =1;
    }
    pStorageEngine->endThread();
}

void thread_transaction_create_noncommit(int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        //cout << "transaction id:" << pTransaction->getTransactionId() << endl;
        //pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
            *i = 0;
        }
        fail = true;
    }
    
    if(fail) {
        *i = 0;
    }else {
        *i = 1;
    }
    pStorageEngine->endThread();
}

void thread_transaction_create(int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        //cout << "transaction id:" << pTransaction->getTransactionId() << endl;
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
            *i = 0;
        }
        fail = true;
    }
    
    if(fail) {
        *i = 0;
    }else {
        *i = 1;
    }
    pStorageEngine->endThread();
}

//void thread_transaction_create(int &i)
//{
//    Transaction* pTransaction = NULL;
//    bool fail = false;
//    try {
//        pStorageEngine->beginThread();
//        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
//        //cout << "transaction id:" << pTransaction->getTransactionId() << endl;
//        pTransaction->commit();
//    } catch (StorageEngineException &ex) {
//        cout << ex.getErrorMsg() << endl;
//        if (pTransaction != NULL) {
//            pTransaction->abort();
//            i = 0;
//        }
//        fail = true;
//    }
//    
//    if(fail) {
//        i = 0;
//    }else {
//        i = 1;
//    }
//    pStorageEngine->endThread();
//}

bool test_thread_index_create()
{
    INTENT("多个线程同时创建同一个表上的多个相同索引");
    int sta[THREAD_NUM_10] = {0};
    try
    {
        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_index_create, &sta[i]));
	    }
	    tg.join_all();
    } catch (StorageEngineException &ex) {
      cout << ex.getErrorNo() << endl;
		//cout << ex.getErrorMsg() << endl;
    }

    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10) {
        return true;
    }
    return false;
}



bool test_thread_create_heap()
{
    INTENT("多个线程同时进行heap创建操作");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};

        thread_group tg;
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_create, &sta[i]));
	    }
	    tg.join_all();

    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10) {
        return true;
    }
    return false;
}


bool test_thread_create_transaction_noncommit()
{
    INTENT("多个线程同时进行transaction创建操作");
    int sta[THREAD_NUM_10] = {0};

        thread_group tg;
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_transaction_create_noncommit, &sta[i]));
	    }
	    tg.join_all();


    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10) {
        return true;
    }
    return false;
}

bool test_thread_create_transaction()
{
    INTENT("多个线程同时进行transaction创建操作");
    int sta[THREAD_NUM_10] = {0};

        thread_group tg;
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_transaction_create, &sta[i]));
	    }
	    tg.join_all();

    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10) {
        return true;
    }
    return false;
}

bool test_thread_open_heap_index_normal()
{
    INTENT("多个线程同时进行transaction创建操作");
    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);

	EntrySetID indexID = 0;
	EntrySetID heapID = 0;
	int sta[THREAD_NUM_10] = {0};
	int i = 0;
	int found = 0;

	

		thread_group tg;
		tg.create_thread(bind(&thread_index_create_indid_heapid, &heapID, &indexID, &i));
	    tg.join_all();

		if(0 == heapID || 0 == indexID) {
			return false;
		}

		if (i) {
			++found;
			i = 0;
		}
		for(int i = 0; i < THREAD_NUM_10; ++i) {
			tg.create_thread(bind(&thread_index_create_open_heapid, heapID, indexID, &sta[i]));
		}
		tg.join_all();
		

		tg.create_thread(bind(&heap_remove, heapID, &i));
		tg.join_all();
		if(i) {
			++found;
			i = 0;
		}

	
	for (int  i = 0; i < THREAD_NUM_10; i++) {
		if(sta[i]){
			++found;
		}
	}

    if (found == THREAD_NUM_10 + 2) {
        return true;
    } 
    return false;
}

bool test_thread_open_heap_index()
{
    INTENT("多个线程同时进行transaction创建操作");
    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);

	EntrySetID indexID = 0;
	int i = 0;
	int found = 0;

	

		thread_group tg;
		tg.create_thread(bind(&thread_index_create_indid, &indexID, &i));
	    tg.join_all();

		if (i) {
			++found;
			i = 0;
		}

		tg.create_thread(bind(&thread_index_create_open, indexID, &i));
		tg.join_all();

		if (i) {
			++found;
			i = 0;
		}

	/*	tg.create_thread(bind(&heap_remove, heapID, &i));
		tg.join_all();*/
		if(i) {
			++found;
			//i + 0;
		}

    if (found == 2) {
        return true;
    } 
    return false;
}

bool test_thread_create_index_del_spr()
{
    INTENT("多个线程同时进行index创建和删除操作");
    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);
    int sta[THREAD_NUM_10] = {0};
	int sta_del[THREAD_NUM_10] = {0};
	struct IDSET{
		EntrySetID heapId;
		EntrySetID indexId;
	};
	IDSET ids[THREAD_NUM_10] = {0};

        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_index_create, &ids[i].heapId, &ids[i].indexId, &sta[i]));
	    }
	    tg.join_all();

	    for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_index_del, ids[i].heapId, ids[i].indexId, &sta_del[i]));
	    }
	    tg.join_all();

    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }

	for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta_del[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10 + THREAD_NUM_10) {
        return true;
    } 
    return false;
}

bool test_thread_create_index_del()
{
    INTENT("多个线程同时进行index创建和删除操作");
    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);
    int sta[THREAD_NUM_10] = {0};

        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_index_create_del, &sta[i]));
	    }
	    tg.join_all();


    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10) {
        return true;
    } 
    return false;
}



//bool test_thread_create_index_check_valid()
//{
//    INTENT("多个线程同时进行index创建操作");
//    form_heap_colinfo(heap_colinfo);
//    form_index_colinfo(index_colinfo);
//
//    setColumnInfo(1, &heap_colinfo);
//    setColumnInfo(2, &index_colinfo);
//    int sta = {0};
//	
//	EntrySetID heapId = {0};
//	
//	    int found = 0;
//
//
//        thread_group tg;
//
//		tg.create_thread(bind(&thread_index_create_check_unique, &heapId, &sta));
//		if (sta) {
//			++found;
//		}
//	   // tg.join_all();
//		tg.create_thread(bind(&thread_index_create_check_valid, heapId, &indexId, &sta));
//
//
//
//        if(sta){
//            ++found;
//        }
//
//    if (found == 2) {
//        return true;
//    } 
//    return false;
//}

bool test_thread_create_index_insert_unique()
{
    INTENT("多个线程同时进行index创建操作");
    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);
    int sta[THREAD_NUM_10] = {0};

        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_index_create_unique, &sta[i]));
	    }
	    tg.join_all();


    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == 0) {
        return true;
    } 
    return false;
}

bool test_thread_create_index()
{
    INTENT("多个线程同时进行index创建操作");
    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);
    int sta[THREAD_NUM_10] = {0};

        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_index_create, &sta[i]));
	    }
	    tg.join_all();


    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10) {
        return true;
    } 
	std::cout<<"\ntest_thread_create_index error, found value : "<<found<<"\n"<<std::endl;
    return false;
}

void thread_create_heap_dll(const int col_id, EntrySetID *id)
{
	using namespace FounderXDB::StorageEngineNS;

	pStorageEngine->beginThread();
	Transaction* trans = NULL;
	try {
		trans = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySetID heap_id= pStorageEngine->createEntrySet(trans,GetSingleColInfo());
		*id = heap_id;
		trans->commit();
	} catch (StorageEngineException &ex) {
		cout << ex.getErrorMsg() << endl;
		trans->abort();
	}
	pStorageEngine->endThread();
}

void create_heap_thread(const int col_id, EntrySetID *id)
{
	using namespace FounderXDB::StorageEngineNS;

	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try 
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySetID heap_id= pStorageEngine->createEntrySet(pTrans,GetSingleColInfo());
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heap_id);
		*id = heap_id;
		pTrans->commit();
	}
	catch (StorageEngineException &ex)
	{
		cout << ex.getErrorMsg() << endl;
		pTrans->abort();
	}
}
void thread_func_create_and_open_heap()
{
	StorageEngine::getStorageEngine()->beginThread();
    try
	{
		uint32 colid = 123456;
		int heap_count = 30;
		for (int i = 0; i < heap_count; i ++)
		{
			EntrySetID entrySetId = InvalidTransactionID;

			create_heap_thread(colid + i,&entrySetId);
		}
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
	}
	StorageEngine::getStorageEngine()->endThread();
}
bool test_thread_create_and_open_heap()
{
	int thread_count = 5;
	boost::thread_group g;
	for (int i = 0; i < thread_count; i ++)
	{
		g.create_thread(thread_func_create_and_open_heap);
	}
	g.join_all();

	return true;
}

typedef struct TMPTRANS
{
	typedef void(*handle_func)(volatile TMPTRANS*, bool *sta);
	FXTransactionId tid;
	EntrySetID eid, indexeid;
	handle_func hfunc;
	bool canQuit;
} TMPTRANS;

static
void thread_stepN(boost::mutex *mymt, boost::mutex *othermt, volatile TMPTRANS *transinfo, bool *sta)
{
	*sta = true;
	StorageEngine *se = StorageEngine::getStorageEngine();
	se->beginThread();

	do 
	{
		if (transinfo->canQuit)
			break;
		othermt->lock();
		if (transinfo->hfunc)
			transinfo->hfunc(transinfo, sta);
		mymt->unlock();
	} while (true);

	se->endThread();
}

static
void step3(volatile TMPTRANS *transinfo, bool *sta)
{
	/* A线程往表T中插入若干数据并使用索引I扫描这些数据 */
	try
	{
		StorageEngine *se = StorageEngine::getStorageEngine();
		FXTransactionId xid = transinfo->tid;
		Transaction *trans = se->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		se->beginStatement(trans);
		EntrySet *entryset = se->openEntrySet(trans, EntrySet::OPEN_SHARED, transinfo->eid);

		/* 构建测试数据 */
		const int DATA_ROW_ = 10;
		DataGenerater dg(1, DATA_LEN);
		dg.dataGenerate();
		vector<DataItem> v_di;
		for(int i = 0; i < DATA_ROW_; ++i)
		{
			v_di.push_back(DataItem(dg[0], strlen(dg[0]) + 1));
		}
		entryset->insertEntries(trans, v_di);
		se->endStatement();

		/* 使用索引扫描这些数据 */
		IndexEntrySet *indexentryset = se->openIndexEntrySet(trans, entryset, EntrySet::OPEN_SHARED, transinfo->indexeid);
		char *c = (char *)malloc(2);
		memcpy(c, dg[0], 1);
		c[1] = '\0';
		SearchCondition scanCondition;
		scanCondition.Add(1, LessEqual, c, str_compare);

		vector<string> vResult;
		GetIndexScanResults(vResult, trans, indexentryset, scanCondition.Keys());
		free(c);

		/* 检查结果 */
		if (vResult.size() != DATA_ROW_)
			*sta = false;

		se->closeEntrySet(trans, indexentryset);
		se->closeEntrySet(trans, entryset);
		se->removeEntrySet(trans, transinfo->eid);
		se->endStatement(trans);
		trans->commit();
		se->detachThread();
		transinfo->hfunc = NULL;
	} catch (StorageEngineException& e)
	{
		std::cout<<e.getErrorMsg()<<std::endl;
		*sta = false;
	}

	transinfo->canQuit = true;
}

static
void step2(volatile TMPTRANS *transinfo, bool *sta)
{
	/* B线程为表T创建索引I */
	try
	{
		StorageEngine *se = StorageEngine::getStorageEngine();
		FXTransactionId xid = transinfo->tid;
		Transaction *trans = se->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		se->beginStatement(trans);
		EntrySet *entryset = se->openEntrySet(trans, EntrySet::OPEN_SHARED, transinfo->eid);
		EntrySetID indexeid = se->createIndexEntrySet(trans, entryset, BTREE_INDEX_ENTRY_SET_TYPE, GetSingleColInfo());
		se->closeEntrySet(trans, entryset);
		se->endStatement(trans);
		se->detachThread();
		transinfo->indexeid = indexeid;
		transinfo->hfunc = step3;
	} catch (StorageEngineException& e)
	{
		std::cout<<e.getErrorMsg()<<std::endl;
		*sta = false;
	}
}

static
void step1(volatile TMPTRANS *transinfo, bool *sta)
{
	/* A线程创建表T */
	try
	{
		StorageEngine *se = StorageEngine::getStorageEngine();
		FXTransactionId xid = InvalidTransactionID;
		Transaction *trans = se->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		transinfo->tid = trans->getTransactionId();
		transinfo->eid = se->createEntrySet(trans, GetSingleColInfo());
		se->endStatement(trans);
		se->detachThread();
		transinfo->hfunc = step2;
	} catch (StorageEngineException& e)
	{
		std::cout<<e.getErrorMsg()<<std::endl;
		*sta = false;
	}
}

/*
 * 1--A线程创建表T
 * 2--切到B线程为表T创建索引I
 * 3--切到A线程插入若干数据
 * 4--在A线程上使用索引I扫描数据
 */
bool test_thread_create_insert_index_dll()
{
	INTENT("模拟线程切换，测试不同线程创建的索引是否能在插入数据的同时正确更新索引。");
	using namespace boost;

	bool return_sta = true;
	mutex mt[2];
	bool sta[2];
	TMPTRANS transinfo;
	memset(&transinfo, 0, sizeof(transinfo));
	transinfo.hfunc = step1;
	thread_group tg;
	/* 启动两个线程交替执行任务 */
	mt[0].lock();
	tg.create_thread(bind(thread_stepN, &mt[0], &mt[1], &transinfo, &sta[0]));
	tg.create_thread(bind(thread_stepN, &mt[1], &mt[0], &transinfo, &sta[1]));
	tg.join_all();

	/* 检测结果 */
	if (!sta[0] || !sta[1])
		return_sta = false;

	memset(&mt, 0 , sizeof(mutex)*2);
	return return_sta;
}

static
void step2_2(volatile TMPTRANS *transinfo, bool *sta)
{
	/* B线程为表T创建索引I并提交事务,然后启动事务 */
	try
	{
		StorageEngine *se = StorageEngine::getStorageEngine();
		FXTransactionId xid = transinfo->tid;
		Transaction *trans = se->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		se->beginStatement(trans);
		EntrySet *entryset = se->openEntrySet(trans, EntrySet::OPEN_SHARED, transinfo->eid);
		EntrySetID indexeid = se->createIndexEntrySet(trans, entryset, BTREE_INDEX_ENTRY_SET_TYPE, GetSingleColInfo());
		se->closeEntrySet(trans, entryset);
		se->endStatement(trans);
		trans->commit();
		xid = InvalidTransactionID;
		trans = se->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		entryset = se->openEntrySet(trans, EntrySet::OPEN_SHARED, transinfo->eid);
		se->closeEntrySet(trans, entryset);
		transinfo->tid = trans->getTransactionId();
		se->endStatement(trans);
		se->detachThread();
		transinfo->indexeid = indexeid;
		transinfo->hfunc = step3;
	} catch (StorageEngineException& e)
	{
		std::cout<<e.getErrorMsg()<<std::endl;
		*sta = false;
	}
}

static
void step2_1(volatile TMPTRANS *transinfo, bool *sta)
{
	/* A线程创建表T */
	step1(transinfo, sta);
	transinfo->hfunc = step2_2;
}

/*
 * 1--A线程创建表T
 * 2--切到B线程为表T创建索引I并提交事务
 * 3--B线程启动事务并打开表
 * 4--切换到A线程插入数据并用索引I查询这些数据
 */
bool test_thread_create_insert_index_dll2()
{
	INTENT("模拟线程切换，测试不同线程创建的索引是否能在插入数据的同时正确更新索引。");
	using namespace boost;

	bool return_sta = true;
	mutex mt[2];
	bool sta[2];
	TMPTRANS transinfo;
	memset(&transinfo, 0, sizeof(transinfo));
	transinfo.hfunc = step2_1;
	thread_group tg;
	/* 启动两个线程交替执行任务 */
	mt[0].lock();
	tg.create_thread(bind(thread_stepN, &mt[0], &mt[1], &transinfo, &sta[0]));
	tg.create_thread(bind(thread_stepN, &mt[1], &mt[0], &transinfo, &sta[1]));
	tg.join_all();

	/* 检测结果 */
	if (!sta[0] || !sta[1])
		return_sta = false;

	memset(&mt, 0 , sizeof(mutex)*2);
	return return_sta;
}

