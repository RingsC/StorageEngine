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
		INTENT("openEntrySet��transaction��NULL,���ִ�NULL�봫�õ���transactionû������...");

		get_new_transaction();//����һ������ 
		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�,transaction��NULL
		DataItem data;
		int len = sizeof(testdata);		
		char *pstr = (char *)malloc(len);//�ǵ��ͷſռ�...free(pstr);
		memcpy(pstr, testdata, len);//����DataItem 
		data.setData((void *)pstr);//pstr�е��ַ�����������data
		data.setSize(len);

		EntryID tid;
		penry_set->insertEntry(pTransaction, tid, data);//��������
		command_counter_increment(); 		
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);//��ʼɨ��

		DataItem getdata;
		char *str = (char*)malloc(sizeof(testdata));//����һ���ռ��str...free(str);
		unsigned int length;
		entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getdata);//��ʱͨ��getdata�жϺ����Ƿ������ݣ��Ժ����ͨ�����ص�int�Ƿ�ΪNULL�ж�
		str=(char*)getdata.getData();//getdata�е����ݸ�str
		length=getdata.getSize();//��С

		CHECK_BOOL(length!=0);
		if (length==0)
		{
			cout<<"��ѯ�����ݳ���Ϊ0������!"<<endl;
			HEAP_RETURN_FALSE_HS;
		}
		int flag=0;
		flag=memcmp(testdata,str,sizeof(str));
		CHECK_BOOL(flag==0);
		if (flag!=0)
		{
			cout<<"��ѯ��������Ԥ��Ĳ�ͬ������!"<<endl;
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
		INTENT("������openEntrySet��ͬһ���������Է��֣��򿪶��ٴα����رն��ٴα���Ȼ�ᱨ��"
			   "��������������ģ���ΪopenEntrySet�����ü����Ĺ��ܣ��򿪱����Ϊ��ʹ����");

		get_new_transaction();//����һ������ 
		EntrySet *penry_set = NULL;
		int max=10;
		int i=0;
		for (;i<max;i++)
		{
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));//�򿪱�
		
		pStorageEngine->closeEntrySet(pTransaction,penry_set);//�رձ�
		}
		commit_transaction();//�ύ����,�����ύ�������ɾ����

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
    INTENT("����߳�ͬʱ��ͬһ�����ڵı�");

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
    INTENT("����߳�ͬʱ��ͬһ��Ԫ���ݱ�");
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
    INTENT("����߳�ͬʱ��0��");
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
    INTENT("����߳�ͬʱ��һ�������ڵı�");
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
    INTENT("���̴߳�һ�����ڱ�ɾ���ı�");

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
    INTENT("���߳�ͬʱʹ��ͬһ��������ͬһ������");

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
    INTENT("���߳�ͬʱʹ��ͬһ��������ͬһ������");

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
    INTENT("���߳�ͬʱʹ��ͬһ��������ͬһ������");

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
    INTENT("���߳�ͬʱʹ��ͬһ��������ͬһ������");

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
    INTENT("���߳�ͬʱ����ͬһ�ű��е�ͬһ��large���ݣ�ÿ���̶߳�ִ�гɹ���");

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
    INTENT("���߳�ͬʱ����ͬһ�ű��е�ͬһ�����ݣ�ÿ���̶߳�ִ�гɹ���");

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
    INTENT("���߳�ͬʱ����ͬһ�ű��е�ͬһ��large���ݣ�ÿ���̶߳�ִ�гɹ���");

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
    INTENT("���߳�ͬʱ����ͬһ�ű��е�ͬһ�����ݣ�ÿ���̶߳�ִ�гɹ���");

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
    INTENT("���߳�ͬʱ����ͬһ�������ڵ�����, ÿ���̶߳�Ӧ����ʾ����ʧ��");

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
    INTENT("���߳�ͬʱ���Ҳ����ظ��º�Ĵ���������ÿ���̶߳����ҳɹ�");

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
    INTENT("һ�̲߳�������,��һ�߳��ڲ�������ݸ���Ϊ�����ݣ����³ɹ���");

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
    INTENT("���߳�ͬʱ����ͬһ������ͬһ������,ֻ��һ���߳�ִ�гɹ�");

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
     INTENT("���߳�ͬʱ�����Ѿ�ɾ��������,����߳̾�δ���ҡ�");

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
    INTENT("���߳�ͬʱɾ�������ڵ�����,�߳̾�δִ�гɹ�");

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
    INTENT("���߳�ͬʱ��ͬһ������ɾ������,ֻ��һ���߳�ִ�гɹ�");

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
    INTENT("���߳�ͬʱ��ͬһ������ɾ������,ֻ��һ���߳�ִ�гɹ�");

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
    INTENT("���߳�ͬʱ��һ�����в�������");

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
    INTENT("���߳�ͬʱ��һ�����в�������");

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
   INTENT("���߳�ͬʱ��һ�����в�������");

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

    INTENT("���߳�ͬʱɾ��ͬһ������ֻ��һ���߳�ִ�гɹ�");
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
    INTENT("���߳�ͬʱɾ��ͬһ�ű�");

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
    INTENT("���߳�ͬʱɾ��ͬһ�ű�");

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
    INTENT("���߳�ͬʱɾ��ͬһ�����");

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
    INTENT("���߳�ͬʱɾ��ͬһ����");

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
    INTENT("���߳�ͬʱ�ر�ͬһ����");

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
    INTENT("���߳�ͬʱ�ر�ͬһ����");

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
    INTENT("���߳�ͬʱ�ر�һ�������ڵı�");

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
    INTENT("���߳�ͬʱ�ر�һ�������ڵı�");

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