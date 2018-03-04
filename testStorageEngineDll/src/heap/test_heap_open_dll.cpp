#include <iostream>
#include "boost/thread.hpp"


#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "heap/test_heap_open_dll.h"
#include "utils/utils_dll.h"
#include "test_fram.h"
#include "sequence/utils.h"


using std::cout;
using std::endl;
using std::string;
using namespace boost;

using namespace FounderXDB::StorageEngineNS;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
extern EntrySetID EID;

#define testdata "testdata_1"


bool test_heap_open_dll()
{
	try {
		INTENT("openEntrySet的transaction传NULL,发现传NULL与传得到的transaction没有区别...");

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
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//开始扫描

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


bool test_heap_multopen_dll()
{
	try {
		INTENT("连续用openEntrySet打开同一个表，经测试发现，打开多少次表必须关闭多少次表，不然会报错"
			   "这个现象是正常的，因为openEntrySet有引用计数的功能，打开表就是为了使用它");

		get_new_transaction();//创建一个事务 
		EntrySet *penry_set = NULL;
		int max=10;
		int i=0;
		for (;i<max;i++)
		{
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//打开表
		
		pStorageEngine->closeEntrySet(pTransaction,penry_set);//关闭表
		}
		commit_transaction();//提交事务,必须提交事务才能删除表

	}
	CATCHEXCEPTION;
	return true;
}



////multi thread test /////
void thread_open_heap(EntrySetID heapId, int *i)
{
    Transaction* pTransaction = NULL;
    bool fail = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

      
		pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heapId);
 
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        //ccout << ex.getErrorMsg() << endl;

        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        fail = true;
    }

    if(!fail){
        *i = 1;
    } else {
        *i = 0;
    }
    pStorageEngine->endThread();
    return;
}


bool test_thread_open_heap()
{
    INTENT("多个线程同时打开同一个存在的表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
	int found = 0;

		EntrySetID heap_id = 0;
        int i = 0;

        thread_group tg;
        tg.create_thread(bind(&heap_create_pointer, &heap_id, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }

    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_open_heap, heap_id, &sta[i]));
	    }
	    tg.join_all();

		tg.create_thread(bind(&thread_heap_del, heap_id, &i));
        tg.join_all();

		if (i) {
			++found;
			i = 0;
		}

    
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
	
    if (found == THREAD_NUM_10  + 2) {
        return true;
    }
    return false;


}

bool test_thread_open_heap_meta()
{
    INTENT("多个线程同时打开同一个元数据表");
    int sta[THREAD_NUM_10] = {0};

        EntrySetID heapId = 254;
        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_open_heap, heapId, &sta[i]));
	    }
	    tg.join_all();


    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (0 == found) {
        return true;
    }
    return false;
}

bool test_thread_open_heap_nonexist_zero()
{
    INTENT("多个线程同时打开0表");
    int sta[THREAD_NUM_10] = {0};

	EntrySetID heapId = 0;
        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_open_heap, heapId, &sta[i]));
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

bool test_thread_open_heap_nonexist()
{
    INTENT("多个线程同时打开一个不存在的表");
    int sta[THREAD_NUM_10] = {0};

        EntrySetID heapId = 3;
        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_open_heap, heapId, &sta[i]));
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


bool test_thread_open_heap_del()
{
    INTENT("多线程打开一个正在被删除的表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};

    try
    {
        thread_group tg;
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_create, &sta[i]));
	    }
	    tg.join_all();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
        throw;
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

bool test_thread_index_getNext_large()
{
    INTENT("多线程同时使用同一索引查找同一条数据");

    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);

    int found = 0;
	DataItem data;

        EntrySetID heap_id = 0;
        EntrySetID index_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_index_create_insert_data_large, &heap_id, &index_id, &eid, &data, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        


        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

	deformDataItem(&data);
    free_colinfo(heap_colinfo);
    free_colinfo(index_colinfo);


    if (found == 2) {
        return true;
    }
    return false; 
}

bool test_thread_index_getNext()
{
    INTENT("多线程同时使用同一索引查找同一条数据");

    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);

    int found = 0;

        EntrySetID heap_id = 0;
        EntrySetID index_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_index_create_insert_data, &heap_id, &index_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

    free_colinfo(heap_colinfo);
    free_colinfo(index_colinfo);

    if (found == 2) {
        return true;
    }
    return false; 
}

bool test_thread_index_find_del()
{
    INTENT("多线程同时使用同一索引查找同一条数据");

    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);

    int found = 0;

        EntrySetID heap_id = 0;
        EntrySetID index_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_index_create_insert_data, &heap_id, &index_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }


        tg.create_thread(bind(&index_remove_poi, heap_id, index_id, &i));
        tg.join_all();
        if(i) {
           ++found; 
           i = 0;
        }

        tg.create_thread(bind(&thread_index_find, heap_id, index_id, eid, &i));
        tg.join_all();
        if(i) {
           ++found; 
           i = 0;
        }

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

    free_colinfo(heap_colinfo);
    free_colinfo(index_colinfo);


    if (found == 3) {
        return true;
    }
    return false;             
}

bool test_thread_index_find()
{
    INTENT("多线程同时使用同一索引查找同一条数据");

    form_heap_colinfo(heap_colinfo);
    form_index_colinfo(index_colinfo);

    setColumnInfo(1, &heap_colinfo);
    setColumnInfo(2, &index_colinfo);


    int found = 0;

        EntrySetID heap_id = 0;
        EntrySetID index_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_index_create_insert_data, &heap_id, &index_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

    free_colinfo(heap_colinfo);
    free_colinfo(index_colinfo);


    if (found == 2) {
        return true;
    }
    return false;         
}

bool test_thread_heap_getNext_large()
{
    INTENT("多线程同时查找同一张表中的同一条large数据，每个线程都执行成功。");

    form_heap_colinfo_chars(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;
	DataItem data;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data_large, &heap_id, &eid, &data, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
        //for(int i = 0; i < 1; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_getNext_large, heap_id, eid, &data, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

    free_colinfo(heap_colinfo);
    free_colinfo(index_colinfo);

    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10 + 2) {
        return true;
    }
    return false;   
}

bool test_thread_heap_getNext()
{
    INTENT("多线程同时查找同一张表中的同一条数据，每个线程都执行成功。");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data, &heap_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }

		std::cout<<"\ncreate heap found value : "<<found<<"\n"<<std::endl;
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_getNext, heap_id, eid, &sta[i]));
	    }
	    tg.join_all();



        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

		std::cout<<"\nremove heap found value : "<<found<<"\n"<<std::endl;

    free_colinfo(heap_colinfo);
    free_colinfo(index_colinfo);

    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10 + 2) {
        return true;
    }

	std::cout<<"\nfinally found value : "<<found<<"\n"<<std::endl;
    return false;     
}

bool test_thread_heap_find_large()
{
    INTENT("多线程同时查找同一张表中的同一条large数据，每个线程都执行成功。");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;
	DataItem data;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};


        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data_large, &heap_id, &eid, &data, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
 	    {
		    tg.create_thread(bind(&thread_heap_find_large, heap_id, eid, &data, &sta[i]));
	    }
	    tg.join_all();


		
        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

	deformDataItem(&data);
    free_colinfo(heap_colinfo);
    free_colinfo(index_colinfo);

    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10 + 2) {
        return true;
    }
    return false; 
}

bool test_thread_heap_find()
{
    INTENT("多线程同时查找同一张表中的同一条数据，每个线程都执行成功。");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data, &heap_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
 	    {
		    tg.create_thread(bind(&thread_heap_find, heap_id, eid, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }


    free_colinfo(heap_colinfo);
    free_colinfo(index_colinfo);

    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10 + 2) {
        return true;
    }
    return false;         
}

bool test_thread_heap_update_nonexist()
{
    INTENT("多线程同时更新同一个表不存在的数据, 每个线程都应该提示更新失败");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data, &heap_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }

        eid.ip_posid = 0;        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_update, heap_id, eid, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == 2) {
        return true;
    }
    return false;         
}

bool test_thread_heap_update_large_find()
{
    INTENT("多线程同时查找并返回更新后的大数据量，每个线程都查找成功");

    form_heap_colinfo_chars(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;

        thread_group tg;
        tg.create_thread(bind(&heap_create_pointer, &heap_id, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }

		for(int i = 0; i < THREAD_NUM_10; ++i)
		//for(int i = 0; i < 1; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_update_large_find, heap_id, LOOP_TIMES, &sta[i]));
	    }
	    tg.join_all();


        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

	for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }


    if (found == 2 + THREAD_NUM_10) {
        return true;
    }
    return false;
}

bool test_thread_heap_update_large()
{
    INTENT("一线程插入数据,另一线程在插入的数据更新为大数据，更新成功。");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data, &heap_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
		tg.create_thread(bind(&thread_heap_update_large, heap_id, DATA_LEN, &eid, &i));
	    tg.join_all();
        if(i) {
           ++found; 
           i = 0;
        }

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }


    if (3 == found) {
        return true;
    }
    return false;     
}

bool test_thread_heap_update()
{
    INTENT("多线程同时更新同一个表中同一条数据,只有一个线程执行成功");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data, &heap_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_update, heap_id, eid, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == 3) {
        return true;
    }
    return false;         
}

bool test_thread_heap_delete_find()
{
     INTENT("多线程同时查找已经删除的数据,多个线程均未查找。");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data, &heap_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }

        tg.create_thread(bind(&thread_heap_dele, heap_id, eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }


    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
            tg.create_thread(bind(&thread_heap_find, heap_id, eid, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }


    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == 3) {
        return true;
    }
    return false;         
}

bool test_thread_heap_delete_nonexist()
{
    INTENT("多线程同时删除不存在的数据,线程均未执行成功");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data, &heap_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
        eid.ip_posid = 0;  //set invalid eid
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_dele, heap_id, eid, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == 2) {
        return true;
    }
    return false; 
}

bool test_thread_heap_delete_large()
{
    INTENT("多线程同时向同一个表中删除数据,只有一个线程执行成功");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

		DataItem data;
        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data_large, &heap_id, &eid, &data, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_dele, heap_id, eid, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == 3) {
        return true;
    }
    return false;     
}

bool test_thread_heap_delete()
{
    INTENT("多线程同时向同一个表中删除数据,只有一个线程执行成功");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;
        EntryID eid = {0};

        thread_group tg;
        tg.create_thread(bind(&heap_create_insert_data, &heap_id, &eid, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
        //for(int i = 0; i < 1; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_dele, heap_id, eid, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }


    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == 3) {
        return true;
    }
    return false;     
}

bool test_thread_heap_insert_large_find()
{
    INTENT("多线程同时向一个表中插入数据");

    form_heap_colinfo_chars(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;

        thread_group tg;
        tg.create_thread(bind(&heap_create_pointer, &heap_id, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }

		for(int i = 0; i < THREAD_NUM_10; ++i)
		{
			tg.create_thread(bind(&thread_heap_insert_large_find, heap_id, LOOP_TIMES, &sta[i]));
		}
		tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

     for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10 + 2) {
        return true;
    }
    return false; 
}

bool test_thread_heap_insert_large()
{
    INTENT("多线程同时向一个表中插入数据");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;

        thread_group tg;
        tg.create_thread(bind(&heap_create_pointer, &heap_id, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }

        //int len = 200;
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
        //for(int i = 0; i < 1; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_insert_large, heap_id, DATA_LEN, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }


    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10 + 2) {
        return true;
    }
    return false;    
}


bool test_thread_heap_insert()
{
   INTENT("多线程同时向一个表中插入数据");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;


        EntrySetID heap_id = 0;
        int i = 0;

        thread_group tg;
        tg.create_thread(bind(&heap_create_pointer, &heap_id, &i));
        tg.join_all();

        if(i) {
           ++found; 
           i = 0;
        }
        
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_insert, heap_id, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove_poi, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
           i = 0;
        }

 
    for (int i = 0; i < THREAD_NUM_10 ; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10 + 2) {
        return true;
    }
    return false;    
}

bool test_thread_del_heap_index()
{

    INTENT("多线程同时删除同一索引表，只有一个线程执行成功");
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
			tg.create_thread(bind(&thread_del_index, heapID, indexID, &sta[i]));
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

    if (found == 1 + 2) {
        return true;
    } 
    return false;
}

bool test_thread_del_heap_meta()
{
    INTENT("多线程同时删除同一张表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 254;
        int i = 0;

        thread_group tg;

    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&heap_remove, heap_id, &sta[i]));
	    }
	    tg.join_all();

  
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (0 == found) {
        return true;
    }
    return false;
}


bool test_thread_del_heap_nonexist()
{
    INTENT("多线程同时删除同一张表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 3;
        int i = 0;

        thread_group tg;

    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&heap_remove, heap_id, &sta[i]));
	    }
	    tg.join_all();

  
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (THREAD_NUM_10 == found) {
        return true;
    }
    return false;
}


bool test_thread_del_heap_nonexist_zero()
{
    INTENT("多线程同时删除同一张零表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;

        thread_group tg;

    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&heap_remove, heap_id, &sta[i]));
	    }
	    tg.join_all();

  
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (THREAD_NUM_10 == found) {
        return true;
    }
    return false;
}

bool test_thread_del_heap()
{
    INTENT("多线程同时删除同一个表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;

        thread_group tg;
        tg.create_thread(bind(&heap_create_pointer, &heap_id, &i));
        tg.join_all();
        if(i) {
           ++found; 
           i = 0;
        }

    	for(int i = 0; i < THREAD_NUM_10; ++i)
		//for(int i = 0; i < 1; ++i)
	    {
		    tg.create_thread(bind(&heap_remove, heap_id, &sta[i]));
	    }
	    tg.join_all();

        //heap_remove(heap_id);

 
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (found == THREAD_NUM_10 + 1) {
        return true;
    }
    return false;
}

bool test_thread_close_heap_meta()
{
    INTENT("多线程同时关闭同一个表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;


        EntrySetID heap_id = 254;
        int i = 0;

        thread_group tg;

    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_close, heap_id, &sta[i]));
	    }
	    tg.join_all();

        tg.create_thread(bind(&heap_remove, heap_id, &i));
        tg.join_all();


        if(i) {
           ++found; 
        }

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


bool test_thread_close_heap_exist()
{
    INTENT("多线程同时关闭同一个表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    int found = 0;

        EntrySetID heap_id = 0;
        int i = 0;

        thread_group tg;
        tg.create_thread(bind(&heap_create_pointer, &heap_id, &i));
        tg.join_all();
		std::cout<<"\ncreate heap success, heap id : "<<heap_id<<" !\n"<<std::endl;
        if(i) {
           ++found; 
           i = 0;
        }

    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_close, heap_id, &sta[i]));
	    }
	    tg.join_all();

		std::cout<<"\nten thread finish!\n"<<std::endl;

        tg.create_thread(bind(&heap_remove, heap_id, &i));
        tg.join_all();

		std::cout<<"\nremove heap success !\n"<<std::endl;


        if(i) {
           ++found; 
        }


    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }

	std::cout<<"found value : "<<found<<" !\n"<<std::endl;
    if (found == (THREAD_NUM_10 + 2)) {
        return true;
    }
    return false;
}

bool test_thread_close_heap_nonexist_nonzero()
{
    INTENT("多线程同时关闭一个不存在的表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    EntrySetID heap_id = 100;

        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_close, heap_id, &sta[i]));
	    }
	    tg.join_all();


    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (0 == found) {
        return true;
    }
    return false;
}

bool test_thread_close_heap_nonexist()
{
    INTENT("多线程同时关闭一个不存在的表");

    form_heap_colinfo(heap_colinfo);
    setColumnInfo(1, &heap_colinfo);
    int sta[THREAD_NUM_10] = {0};
    EntrySetID heap_id = 0;

        thread_group tg;
    	for(int i = 0; i < THREAD_NUM_10; ++i)
	    {
		    tg.create_thread(bind(&thread_heap_close, heap_id, &sta[i]));
	    }
	    tg.join_all();

    int found = 0;
    for (int i = 0; i < THREAD_NUM_10; i++){
        if(sta[i]){
            ++found;
        }
    }
    if (0 == found) {
        return true;
    }
    return false;
}


bool thrfun_index_open(EntrySetID heapId)
{

    EntrySet *heap_entry = NULL;
    if (0 == heapId) {
    
        EntrySetID heapId = pStorageEngine->createEntrySet(NULL,1);
        heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(NULL,EntrySet::OPEN_EXCLUSIVE,heapId));
        //cout << "heap id:" << heap_entry->getId() << endl;
        pStorageEngine->closeEntrySet(NULL, heap_entry);

        pStorageEngine->removeEntrySet(NULL, heapId);

    } else {
        heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(NULL,EntrySet::OPEN_EXCLUSIVE,heapId));
        //cout << "heap id:" << heap_entry->getId() << endl;
        pStorageEngine->closeEntrySet(NULL, heap_entry);
    }
    return true;
}

bool test_replication_check()
{
    FILE *fp;
    int flag;
    std::string file_site = get_my_arg("test_replication_check");
    cout<< file_site <<endl;

    const char *p=file_site.c_str();
    fp = fopen(p,"r");  //open file for svn read
    
	if (NULL != fp)
	{
		fscanf(fp,"%d",&flag);
		if (flag == 1)
		{
			printf("replication check data error!\n");
			fclose(fp);
			remove(p);
			return false;
		}
		remove(p);
		fclose(fp);	
	}

    return true;
}