#include <iostream>
#include <string>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <cstdarg>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "utils/utils_dll.h"
#include "sequence/utils.h"
#include "Configs.h"
#include "utils/attr_setter.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::setw;
using namespace FounderXDB::StorageEngineNS;

StorageEngine *pStorageEngine = NULL;
extern FounderXDB::StorageEngineNS::Transaction *pTransaction = NULL;

extern uint32 EntryColId = 65535;
extern uint32 IndexId = 65536;
extern uint32 VarColId = 65537;


extern void createTable();
extern void dropTable();
extern void setColInfo(uint32 , Spliti);
extern void my_split();
extern int my_compare_str_index(const char *str1, size_t len1, const char *str2, size_t len2);

int get_col_id()
{
	static int colid = 9105;
	return colid++;
}

void heap_create(EntrySetID &heap_id, int &i)
{
    Transaction* pTransaction = NULL;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        heap_id = pStorageEngine->createEntrySet(pTransaction,1);

        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        i = 0; 
    }

    i = 1;
    
    pStorageEngine->endThread(); 
}

void heap_index_create_insert_data_large(EntrySetID *heap_id, EntrySetID *index_id, EntryID *eid, DataItem *data, int *i)
{
   Transaction* pTransaction = NULL;
    bool suc = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        EntrySetID heapid = pStorageEngine->createEntrySet(pTransaction,1);
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heapid);
		EntrySetID indid = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetSingleColInfo());

		pStorageEngine->endStatement();
		*heap_id = heapid;
        *index_id = indid;
        suc = heap_insert_eid_large(pTransaction,*heap_id, *eid, *data);
		pStorageEngine->endStatement();

		bool inser_flag = index_getNext_large(pTransaction,*heap_id, *index_id, *data, *eid);
		if (suc && inser_flag) {
			*i = 1;
		} else {
			*i = 0;
		}

        pTransaction->commit();
    } catch (StorageEngineException & /*ex*/) {
        //cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        *i = 0; 
    }

    if(suc){
        *i = 1;
    } else {
        *i = 0;
    }
    
    pStorageEngine->endThread(); 
}

void heap_index_create_insert_data(EntrySetID *heap_id, EntrySetID *index_id, EntryID *eid, int *i)
{
   Transaction* pTransaction = NULL;
    bool suc = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        *heap_id = pStorageEngine->createEntrySet(pTransaction,1);
				EntrySet *heap_entry = pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, *heap_id, NULL);
				*index_id = pStorageEngine->createIndexEntrySet(pTransaction,heap_entry,BTREE_INDEX_ENTRY_SET_TYPE,GetSingleColInfo());
		pStorageEngine->endStatement();

        suc = heap_insert_eid(pTransaction,*heap_id, *eid);
		pStorageEngine->endStatement();
		bool inser_flag = index_find(pTransaction,*heap_id, *index_id, *eid);
		if (inser_flag && suc) {
			*i = 1;
		} else {
			*i = 0;
		}

        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        *i = 0; 
    }

   
    pStorageEngine->endThread();  

}

void heap_create_insert_data_large(EntrySetID *heap_id, EntryID *eid, DataItem *data, int *i)
{
    Transaction* pTransaction = NULL;
    bool suc = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        EntrySetID heapid = pStorageEngine->createEntrySet(pTransaction,1);
        *heap_id = heapid;
        suc = heap_insert_eid_large(pTransaction,*heap_id, *eid, *data);

        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        *i = 0; 
    }

    if(suc){
        *i = 1;
    } else {
        *i = 0;
    }
    
    pStorageEngine->endThread();
}

void heap_create_insert_data(EntrySetID *heap_id, EntryID *eid, int *i)
{
    Transaction* pTransaction = NULL;
    bool suc = false;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        *heap_id = pStorageEngine->createEntrySet(pTransaction,1);
        suc = heap_insert_eid(pTransaction,*heap_id, *eid);

        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        *i = 0; 
    }

    if(suc){
        *i = 1;
    } else {
        *i = 0;
    }
    
    pStorageEngine->endThread();     
}



void heap_create_pointer(EntrySetID *heap_id, int *i)
{

    Transaction* pTransaction = NULL;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        EntrySetID heapid = pStorageEngine->createEntrySet(pTransaction,1);
        *heap_id = heapid;

		//pStorageEngine->closeEntrySet(pTransaction,heap_entry);
        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        *i = 0; 
    }

    *i = 1;
    
    pStorageEngine->endThread(); 

}

bool cmpdata(void* first, void* sec)
{
    if(NULL == first || NULL == sec) 
        return false;
    int *pfir = (int *)first;
    int *psec = (int *)sec;
    if(*pfir == *psec){
        return true;
    }
    return false;

}





boost::mutex formDataMutex;
void formDataItem(int len, DataItem &data)
{
	int static loop = 0;
    int data_len = strlen(DATA);
    int len_all = len * data_len + INT_LEN + 1;

    char *pdata = new char[len_all];
	memset((void*)(pdata), 0, len_all);

	//char *pdata = (char *)malloc(len_all * sizeof(char));
    char *cur = pdata;
    for (int i = 0; i < len; i++){
        cur = (char *)memcpy(cur, DATA, data_len);
        cur += data_len;
    }
	//boost::mutex::scoped_lock lock(formDataMutex);
	//++loop;
	//boost::mutex::scoped_lock unlock(formDataMutex);
	sprintf(cur, "%20d", ++loop);

	//memcpy(cur, buf, INT_LEN);


    data.setData((void *)(pdata));
    data.setSize(len_all);
    return ;
}

//void formDataItem(int len, DataItem &data)
//{
//    int data_len = strlen(DATA);
//    int len_all = len * data_len;
//    char *pdata = (char *)malloc(sizeof(len_all));
//   
//    char *cur = pdata;
//    for (int i = 0; i < len; i++){
//        cur = (char *)memcpy(cur, DATA, data_len);
//        cur += data_len;
//    }
//    data.setData(pdata);
//    data.setSize(len_all);
//    return ;
//}

void deformDataItem(DataItem *data)
{
    char *pdata = (char *)(data->getData());
    if(pdata != NULL) {
        delete [] pdata;
        data->setData(NULL);
		data->setSize(0);
    }
}


bool checkDataItem_full(DataItem *data1, DataItem *data2)
{
	int len1 = data1->getSize();
    int len2 = data2->getSize();

	if(len1 != len2) {
		return false;
	}

    void *pdata1 = data1->getData();
    void *pdata2 = data2->getData();

    if ( 0 == memcmp(pdata1,pdata2, len1)) {
        return true;
    }
	else
	{
		char *pData1 = (char*)pdata1;
		char *pData2 = (char*)pdata2;
		for (int i = 0; i < len1; ++i)
		{
			if (((char*)pdata1)[i] != ((char*)pdata2)[i])
			{
				int j = 0;
				++j;
			}
		}
	}
    return false;
}

bool checkDataItem_part(DataItem *data1, DataItem *data2)
{
	int len1 = data1->getSize();
    int len2 = data2->getSize();

    int len = len1 > len2 ? len1 : len2;

    void *pdata1 = data1->getData();
    void *pdata2 = data2->getData();

    if ( 0 == memcmp(pdata1,pdata2, len)) {
        return true;
    }
    return false;
}


bool cheakData(int a, int b)
{
    if (a == b ){
        return true;
    }
    return false;
}
bool cheakEid(EntryID eid, EntryID eid_find)
{
    if(eid.bi_hi == eid_find.bi_hi && 
        eid.bi_lo == eid_find.bi_lo && 
        eid.ip_posid == eid_find.ip_posid){
        return true;
    }
    return false;
}

bool checkeid(EntryID eid)
{
    if(eid.bi_hi == 0 && eid.bi_lo == 0 && eid.ip_posid == 0){
        return false;
    }
    return true;
}

bool heap_insert_eid_large(Transaction* txn,EntrySetID heapId, EntryID &eid, DataItem &data)
{
    EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));
    

    //DataItem data;
	formDataItem(LOOP_TIMES, data);
    try {
        heap_entry->insertEntry(pTransaction, eid, data);

    } catch (StorageEngineException &/*ex*/) {
        //cout << ex.getErrorMsg() << endl;
        deformDataItem(&data);
        pStorageEngine->closeEntrySet(pTransaction, heap_entry);
        
    }

    if(checkeid(eid)){
        return true;
    }
    return false;
}

bool heap_insert_eid(Transaction* txn,EntrySetID heapId, EntryID &eid)
{
    EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));
    
    int len = sizeof(int);
    void *pdata = malloc(len);
    int a = 8;
    memcpy(pdata, &a ,sizeof(a));
    DataItem data;
    data.setData(pdata);
    data.setSize(len);


    try {
        heap_entry->insertEntry(pTransaction, eid, data);

    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        free(pdata);
        pStorageEngine->closeEntrySet(pTransaction, heap_entry);
        
    }

    if(checkeid(eid)){
        return true;
    }
    return false;

}

bool index_getNext_large(Transaction* txn,EntrySetID heapId, EntrySetID indexId, DataItem &data, EntryID eid)
{
        EntrySet *heap_entry = NULL;
        IndexEntrySet *index_entry = NULL;

        heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));

        try {
            index_entry = (pStorageEngine->openIndexEntrySet(txn, heap_entry, EntrySet::OPEN_EXCLUSIVE, indexId, NULL));
        } catch (StorageEngineException &) {
            pStorageEngine->closeEntrySet(pTransaction, heap_entry);
            throw;
        }

		ScanCondition cond(1, ScanCondition::GreaterEqual, (se_uint64)(data.getData()), data.getSize(), wfh_compare);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = index_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
        } catch (StorageEngineException &) {
            pStorageEngine->closeEntrySet(txn, heap_entry);
            pStorageEngine->closeEntrySet(txn, index_entry);
            return false;
        }
        EntryID eid_find = {0};
        DataItem entry;
        se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::PREV_FLAG, eid_find, entry);
            if(NO_DATA_FOUND == ret) {
                index_entry->endEntrySetScan(pscan);
                pStorageEngine->closeEntrySet(txn, heap_entry);
                pStorageEngine->closeEntrySet(txn, index_entry);
                return false;
            }

        } catch (StorageEngineException &/*ex*/) {
            //cout << ex.getErrorMsg() << endl;
            index_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            pStorageEngine->closeEntrySet(txn, index_entry);
            return false;
        }

        index_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        pStorageEngine->closeEntrySet(txn, index_entry);


		if (checkDataItem_full(&data, &entry)){
			return true;
		}
        //if(cheakData(a, b)){
        //    return true;
        //}
        return false;
}

bool index_getNext(Transaction* txn,EntrySetID heapId, EntrySetID indexId, EntryID eid)
{
        EntrySet *heap_entry = NULL;
        IndexEntrySet *index_entry = NULL;

        heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));

        try {
            index_entry = (pStorageEngine->openIndexEntrySet(txn, heap_entry, EntrySet::OPEN_EXCLUSIVE, indexId, NULL));
        } catch (StorageEngineException &) {
            pStorageEngine->closeEntrySet(txn, heap_entry);
            throw;
        }

        int a = 8;
        int *ch = (int*)malloc(sizeof(int));
        *ch = a;
        ScanCondition cond(1, ScanCondition::GreaterEqual, (se_uint64)ch, sizeof(a), wfh_compare);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = index_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
        } catch (StorageEngineException &) {
            free(ch);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            pStorageEngine->closeEntrySet(txn, index_entry);
            return false;
        }
        EntryID eid_find = {0};
        DataItem entry;
        se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::PREV_FLAG, eid_find, entry);
            if(NO_DATA_FOUND == ret) {
                free(ch);
                index_entry->endEntrySetScan(pscan);
                pStorageEngine->closeEntrySet(txn, heap_entry);
                pStorageEngine->closeEntrySet(txn, index_entry);
                return false;
            }

        } catch (StorageEngineException &ex) {
            cout << ex.getErrorMsg() << endl;
            free(ch);
            index_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            pStorageEngine->closeEntrySet(txn, index_entry);
            return false;
        }

        free(ch);
        index_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        pStorageEngine->closeEntrySet(txn, index_entry);

        int b = *(int*)(entry.getData());
        if(cheakData(a, b)){
            return true;
        }
        return false;
}


bool index_find(Transaction* txn,EntrySetID heapId, EntrySetID indexId, EntryID eid)
{
        EntrySet *heap_entry = NULL;
        IndexEntrySet *index_entry = NULL;

        heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));

        try {
            index_entry = (pStorageEngine->openIndexEntrySet(txn, heap_entry, EntrySet::OPEN_EXCLUSIVE, indexId, NULL));
            if(index_entry == NULL)
                return false;
        } catch (StorageEngineException &) {
            pStorageEngine->closeEntrySet(txn, heap_entry);
            throw;
        }

        int a = 8;
        int *ch = (int*)malloc(sizeof(int));
        *ch = a;
        ScanCondition cond(1, ScanCondition::Equal, (se_uint64)ch, sizeof(a), wfh_compare);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = index_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
        } catch (StorageEngineException &) {
            free(ch);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            pStorageEngine->closeEntrySet(txn, index_entry);
            return false;
        }
        EntryID eid_find = {0};
        DataItem entry;
        se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::NEXT_FLAG, eid_find, entry);
            if(NO_DATA_FOUND == ret) {
                free(ch);
                index_entry->endEntrySetScan(pscan);
                pStorageEngine->closeEntrySet(txn, heap_entry);
                pStorageEngine->closeEntrySet(txn, index_entry);
                return false;
            }

        } catch (StorageEngineException &ex) {
            cout << ex.getErrorMsg() << endl;
            free(ch);
            index_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            pStorageEngine->closeEntrySet(txn, index_entry);
            return false;
        }

        free(ch);
        index_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        pStorageEngine->closeEntrySet(txn, index_entry);

        if(cheakEid(eid, eid_find)){
            return true;
        }
        return false;
}

bool heap_getNext_large(Transaction* txn,EntrySetID heapId, DataItem &data, EntryID eid)
{
        EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));


		ScanCondition cond(1, ScanCondition::Equal, (se_uint64)(data.getData()), data.getSize(), wfh_compare_chars);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = heap_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
        } catch (StorageEngineException &) {
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }
        EntryID eid_find = {0};
        DataItem entry;
        se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::PREV_FLAG, eid_find, entry);
            if(NO_DATA_FOUND == ret) {
                heap_entry->endEntrySetScan(pscan);
                pStorageEngine->closeEntrySet(txn, heap_entry);
                return false;
            }

        } catch (StorageEngineException & /*ex*/) {
            //cout << ex.getErrorMsg() << endl;
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }

        heap_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);

		if(checkDataItem_full(&data, &entry)){
			return true;
		}
        return false;
}

bool heap_getNext(Transaction* txn,EntrySetID heapId, EntryID eid)
{
        EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));

        int a = 8;
        int *ch = (int*)malloc(sizeof(int));
        *ch = a;
        ScanCondition cond(1, ScanCondition::GreaterEqual, (se_uint64)ch, sizeof(a), wfh_compare);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = heap_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
			std::cout<<"\nStart Scan\n"<<std::endl;
        } catch (StorageEngineException &) {
            free(ch);
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }
        EntryID eid_find = {0};
        DataItem entry;
        se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::PREV_FLAG, eid_find, entry);
			std::cout<<"\nScan GetNext\n"<<std::endl;
            if(NO_DATA_FOUND == ret) {
                free(ch);
                heap_entry->endEntrySetScan(pscan);
                pStorageEngine->closeEntrySet(txn, heap_entry);
                return false;
            }

        } catch (StorageEngineException &ex) {
            cout << ex.getErrorMsg() << endl;
            free(ch);
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }

        free(ch);
        heap_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        int b = *(int *)(entry.getData());
        if(cheakData(a, b)){
            return true;
        }
        return false;
}

bool heap_find_large(Transaction* txn,EntrySetID heapId, DataItem &data, EntryID eid)
{
        EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));

		ScanCondition cond(1, ScanCondition::GreaterEqual, (se_uint64)(data.getData()), data.getSize(), wfh_compare);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = heap_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
        } catch (StorageEngineException &) {
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }
        EntryID eid_find = {0};
        DataItem entry;
        se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::PREV_FLAG, eid_find, entry);
            if(NO_DATA_FOUND == ret) {
                heap_entry->endEntrySetScan(pscan);
                pStorageEngine->closeEntrySet(txn, heap_entry);
                return false;
            }

        } catch (StorageEngineException &/*ex*/) {
            //cout << ex.getErrorMsg() << endl;
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }

        heap_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);


		if(checkDataItem_part(&data, &entry)){
			return true;
		}
        /*if(cheakEid(eid, eid_find)){
            return true;
        }*/
        return false;
}

bool heap_find_chars(Transaction* txn,EntrySetID heapId, EntryID eid)
{
        EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));

        int len = strlen(DATA);
        char *ch = (char*)malloc(len);
        memcpy(ch, DATA, len);

        ScanCondition cond(1, ScanCondition::GreaterEqual, (se_uint64)ch, len, wfh_compare);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = heap_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
        } catch (StorageEngineException &) {
            free(ch);
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }
        EntryID eid_find = {0};
        DataItem entry;
        se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::PREV_FLAG, eid_find, entry);
            if(NO_DATA_FOUND == ret) {
                free(ch);
                heap_entry->endEntrySetScan(pscan);
                pStorageEngine->closeEntrySet(txn, heap_entry);
                return false;
            }

        } catch (StorageEngineException &ex) {
            cout << ex.getErrorMsg() << endl;
            free(ch);
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }

        free(ch);
        heap_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);

        if(cheakEid(eid, eid_find)){
            return true;
        }
        return false;
}


bool heap_find(Transaction* txn,EntrySetID heapId, EntryID eid)
{
        EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));

		std::cout<<"\nheap_find openEntrySet successfully !\n"<<std::endl;

        int a = 8;
        int *ch = (int*)malloc(sizeof(int));
        *ch = a;
        ScanCondition cond(1, ScanCondition::GreaterEqual, (se_uint64)ch, sizeof(a), wfh_compare);
        std::vector<ScanCondition> vscans;
        vscans.push_back(cond);

        EntrySetScan *pscan = NULL;
        try {
            pscan = heap_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
        } catch (StorageEngineException &) {
            free(ch);
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }
		std::cout<<"\n startEntrySetScan() success !\n"<<std::endl;
        EntryID eid_find = {0};
        DataItem entry;
        se_int64 currentValue = 0;
        int ret = 0;
        try {
            ret = pscan->getNext(EntrySetScan::PREV_FLAG, eid_find, entry);
            if(NO_DATA_FOUND == ret) {
                free(ch);
                heap_entry->endEntrySetScan(pscan);
                pStorageEngine->closeEntrySet(txn, heap_entry);
                return false;
            }

        } catch (StorageEngineException &ex) {
            cout << ex.getErrorMsg() << endl;
            free(ch);
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }

        free(ch);
        heap_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);

		std::cout<<"\nendEntrySetScan() , closeEntrySet() success !\n"<<std::endl;
        if(cheakEid(eid, eid_find)){
            return true;
        }
        return false;

}

bool heap_update_large(Transaction* txn,EntrySetID heapId, int len, EntryID &eid)
{
        EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));

        DataItem data;
        formDataItem(len, data);

        try {
            heap_entry->updateEntry(txn,eid, data);
        } catch (StorageEngineException &ex) {
            cout << ex.getErrorMsg() << endl;
            deformDataItem(&data);
            pStorageEngine->closeEntrySet(txn,heap_entry);
            return false;
        }
        deformDataItem(&data);
        pStorageEngine->closeEntrySet(txn,heap_entry);
        return true;
}

bool heap_update(Transaction* txn,EntrySetID heapId, EntryID eid)
{
        EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));

        int len = sizeof(int);
        void *pdata = malloc(len);
        int a = 6;
        memcpy(pdata, &a ,sizeof(a));
        DataItem data;
        data.setData(pdata);
        data.setSize(len);

        try {
            heap_entry->updateEntry(txn,eid, data);
        } catch (StorageEngineException &ex) {
            cout << ex.getErrorMsg() << endl;
            free(pdata);
            pStorageEngine->closeEntrySet(txn,heap_entry);
            return false;
        }
        free(pdata);
        pStorageEngine->closeEntrySet(txn,heap_entry);
        return true;
}

bool heap_dele(Transaction* txn,EntrySetID heapId, EntryID eid)
{
        EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));
        try {
            heap_entry->deleteEntry(txn, eid);
        } catch (StorageEngineException &/*ex*/) {
            //cout << ex.getErrorMsg() << endl;
            pStorageEngine->closeEntrySet(txn,heap_entry);
            return false;
        }
        pStorageEngine->closeEntrySet(txn,heap_entry);
        return true;
}

bool heap_insert_update_large_find(Transaction* txn,EntrySetID heapId, int len)
{
    EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));
    
    DataItem ientry;
    formDataItem(len, ientry);
    EntryID eid = {0};
    try {
        heap_entry->insertEntry(txn, eid, ientry);

    } catch (StorageEngineException &/*ex*/) {
        //cout << ex.getErrorMsg() << endl;
        deformDataItem(&ientry);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        
    }
	deformDataItem(&ientry);
	pStorageEngine->endStatement();

	DataItem update_entry;
	formDataItem(len, update_entry);
	try {
		heap_entry->updateEntry(txn, eid, update_entry);
	} catch (StorageEngineException &ex) {
		cout << ex.getErrorMsg() << endl;
		deformDataItem(&update_entry);
		pStorageEngine->closeEntrySet(txn, heap_entry);
	}
	
	pStorageEngine->endStatement();

	ScanCondition cond(1, ScanCondition::Equal, (se_uint64)update_entry.getData(), update_entry.getSize(), wfh_compare_chars);
    std::vector<ScanCondition> vscans;
    vscans.push_back(cond);

    EntrySetScan *pscan = NULL;
    try {
       pscan = heap_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
    } catch (StorageEngineException &) {
		deformDataItem(&update_entry);
        heap_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        return false;
    }
    EntryID eid_find = {0};
    DataItem entry;
    se_int64 currentValue = 0;
    int ret = 0;
    try {
        ret = pscan->getNext(EntrySetScan::NEXT_FLAG, eid_find, entry);
        if(NO_DATA_FOUND == ret) {
            deformDataItem(&update_entry);
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }

    } catch (StorageEngineException &/*ex*/) {
            //cout << ex.getErrorMsg() << endl;
        deformDataItem(&update_entry);
        heap_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        return false;
    }

    
    heap_entry->endEntrySetScan(pscan);
    pStorageEngine->closeEntrySet(txn, heap_entry);


    if(checkDataItem_full(&update_entry, &entry)){
		deformDataItem(&update_entry);
        return true;
    }
	deformDataItem(&update_entry);
    return false;
}

boost::mutex pp_mutex;
bool heap_insert_large_find(Transaction* txn,EntrySetID heapId, int len)
{
    EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));
    
    DataItem ientry;
    formDataItem(len, ientry);
    EntryID eid = {0};
    try {
        heap_entry->insertEntry(txn, eid, ientry);
		command_counter_increment();

    } catch (StorageEngineException &/*ex*/) {
        //cout << ex.getErrorMsg() << endl;
        deformDataItem(&ientry);
        
        pStorageEngine->closeEntrySet(txn, heap_entry);
        return false;
    }
	char *pInsert = (char*)(ientry.getData());

	/*{
		boost::lock_guard<boost::mutex> lock(pp_mutex);
		std::cout<<pthread_self()<<": "<<pInsert<<std::endl;
	}*/

	ScanCondition cond(1, ScanCondition::Equal, (se_uint64)ientry.getData(), ientry.getSize(), wfh_compare_chars);
    std::vector<ScanCondition> vscans;
    vscans.push_back(cond);

    EntrySetScan *pscan = NULL;
    try {
       pscan = heap_entry->startEntrySetScan(txn,BaseEntrySet::SnapshotMVCC,vscans);
    } catch (StorageEngineException &) {
		deformDataItem(&ientry);
        heap_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        return false;
    }
    EntryID eid_find = {0};
    DataItem entry;
    se_int64 currentValue = 0;
    int ret = 0;
	char *pGet = NULL;
    try {
        ret = pscan->getNext(EntrySetScan::NEXT_FLAG, eid_find, entry);
		pGet = (char*)(entry.getData());
        if(NO_DATA_FOUND == ret) {
            deformDataItem(&ientry);
            heap_entry->endEntrySetScan(pscan);
            pStorageEngine->closeEntrySet(txn, heap_entry);
            return false;
        }

    } catch (StorageEngineException &/*ex*/) {
            //cout << ex.getErrorMsg() << endl;
        deformDataItem(&ientry);
        heap_entry->endEntrySetScan(pscan);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        return false;
    }

    
    heap_entry->endEntrySetScan(pscan);
    pStorageEngine->closeEntrySet(txn, heap_entry);


    if(checkDataItem_full(&entry, &ientry)){
		deformDataItem(&ientry);
        return true;
    }
	deformDataItem(&ientry);
    return false;
}

bool heap_insert_large(Transaction* txn,EntrySetID heapId, int len)
{
    EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));
    
    DataItem data;
    formDataItem(len, data);
    EntryID eid = {0};
    try {
        heap_entry->insertEntry(txn, eid, data);

    } catch (StorageEngineException &/*ex*/) {
        //cout << ex.getErrorMsg() << endl;
        deformDataItem(&data);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        return false;
    }

    deformDataItem(&data);
    pStorageEngine->closeEntrySet(txn, heap_entry);

    if(checkeid(eid)){
        return true;
    }
    return false;
}

bool heap_insert(Transaction* txn,EntrySetID heapId)
{
    EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(txn,EntrySet::OPEN_EXCLUSIVE,heapId));
    
    int len = sizeof(int);
    void *pdata = malloc(len);
    int a = 8;
    memcpy(pdata, &a ,sizeof(a));
    DataItem data;
    data.setData(pdata);
    data.setSize(len);
    EntryID eid = {0};


    try {
		for (int i = 0; i < LOOP_TIMES; i++){
			heap_entry->insertEntry(txn, eid, data);
		}
    } catch (StorageEngineException &/*ex*/) {
        //cout << ex.getErrorMsg() << endl;
        free(pdata);
        pStorageEngine->closeEntrySet(txn, heap_entry);
        
    }

    free(pdata);
    pStorageEngine->closeEntrySet(txn, heap_entry);

    if(checkeid(eid)){
        return true;
    }
    return false;

}

void index_remove_poi(EntrySetID heapId, EntrySetID indexId, int *i)
{
    Transaction* pTransaction = NULL;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        pStorageEngine->removeIndexEntrySet(pTransaction, heapId, indexId);

        pTransaction->commit();
    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        *i = 0; 
    }

    *i = 1;
    
    pStorageEngine->endThread();     
}

void heap_remove_poi(EntrySetID heapId, int *i)
{
    Transaction* pTransaction = NULL;
    try {
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        pStorageEngine->removeEntrySet(pTransaction, heapId);

        pTransaction->commit();
    } catch (StorageEngineException &/*ex*/) {
        //cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        *i = 0; 
    }

    *i = 1;
    
    pStorageEngine->endThread(); 

}

void thread_del_index(EntrySetID heapid, EntrySetID indexid, int *i)
{
    Transaction* pTransaction = NULL;
    try {
		*i = 1;
        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pStorageEngine->removeIndexEntrySet(pTransaction, heapid, indexid);

        pTransaction->commit();
    } catch (StorageEngineException &/*ex*/) {
        //cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        *i = 0; 

    }
  
    pStorageEngine->endThread(); 
}

void heap_remove(EntrySetID heapId, int *i)
{

    Transaction* pTransaction = NULL;
	
    try {

        pStorageEngine->beginThread();
        pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		std::cout<<"start new transaction success !\n"<<std::endl;
        pStorageEngine->removeEntrySet(pTransaction, heapId);
        pTransaction->commit();

		*i = 1;

    } catch (StorageEngineException &ex) {
        cout << ex.getErrorMsg() << endl;
        if (pTransaction != NULL) {
            pTransaction->abort();
        }
        *i = 0; 
    }

    
    pStorageEngine->endThread(); 

}
extern std::map<std::string,std::string> ProgramOpts;
storage_params *get_param( void )
{
	storage_params* pRet = NULL;
	if (ProgramOpts.find("-conf") != ProgramOpts.end())
	{
		return GetStorageParam(ProgramOpts["-conf"]);
	}
	else if (ProgramOpts.find("-c") != ProgramOpts.end())
	{
		if ("archive" == ProgramOpts["-c"])
		{
			static storage_params para;
			para.doRecovery = false;
			para.XLogArchiveMode = true;	
			//para.XLogArchiveCommand = "python copy.py \"%p\" \"../archive/%f\"";
			para.XLogArchivePath = "../archive";
			pRet = &para;
		}
		else if ("recovery" == ProgramOpts["-c"])
		{
			static storage_params para;
			para.doRecovery = true;
			//para.recoveryRestoreCommand = "python copy.py  \"../archive/%f\" \"%p\"";
            para.recoveryRestorePath = "../archive";
			pRet = &para;
		}
	}
    return pRet;
}
int start_engine_()
{
	extern std::string g_strDataDir;
	pStorageEngine = StorageEngine::getStorageEngine();
//	pStorageEngine->bootstrap("D:\\dataDll");
	pStorageEngine->initialize(const_cast<char*>(g_strDataDir.c_str()), 80, get_param()); //start engine
	return 1;
}

int stop_engine_()
{
	pStorageEngine->shutdown();//stop engine
	return 1;
}

void startEngineAndCreateTable()
{
	start_engine_();
	createTable();
}

void dropTableAndStopEngine()
{
	dropTable();
	stop_engine_();
}

void get_first_transaction()
{	
	FXTransactionId invalid_transaction = ((TransactionId) 0);
	pTransaction = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);//获得一个事务
}

void commit_transaction()
{
	try
	{
		pTransaction->commit();//提交事务	
	}
	catch(StorageEngineException &/*ex*/)
	{
		pTransaction->abort();
		//cout << ex.getErrorMsg() << endl;
		//cout << ex.getErrorNo() << endl;
	}
	delete pTransaction;
	pTransaction = NULL;
}

void user_abort_transaction()
{
	if (NULL != pTransaction)
	{
		pTransaction->abort();//取消事务
		delete pTransaction;
		pTransaction = NULL;
	}
} 

void get_new_transaction()
{	
	FXTransactionId invalid_transaction = ((TransactionId) 0);
	pTransaction = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);//获得一个事务
}

Transaction* start_new_transaction(Transaction::IsolationLevel   level)
{
    FXTransactionId invalid_transaction = ((TransactionId) 0);
	Transaction* pTransaction = pStorageEngine->getTransaction(invalid_transaction, level);//获得一个事务
    return pTransaction;
}

MemoryContext* get_txn_mem_cxt()
{
    return pTransaction ? pTransaction->getAssociatedMemoryContext() : 0;
}

void command_counter_increment()
{
	pStorageEngine->endStatement();
}


std::vector<int> FixSpliter::g_vecOfSplitPos;
FixSpliter::FixSpliter(const std::vector<int> &vec)
{
	g_vecOfSplitPos.clear();
	for (std::vector<int>::const_iterator it = vec.begin();
		it != vec.end();
		++it)
	{
		g_vecOfSplitPos.push_back(*it + (g_vecOfSplitPos.size() > 0 ? g_vecOfSplitPos.back() : 0));
	}
}

void FixSpliter::split(RangeDatai& rangeData, const char *pszNeedSplit,int iIndexOfColumn, size_t len)
{
	pszNeedSplit;
	assert(iIndexOfColumn >= 1 && iIndexOfColumn <= (int)g_vecOfSplitPos.size());

	if (iIndexOfColumn > 1)
	{
		rangeData.start = g_vecOfSplitPos[iIndexOfColumn - 2];
		rangeData.len = g_vecOfSplitPos[iIndexOfColumn - 1] -  g_vecOfSplitPos[iIndexOfColumn - 2];
	}
	else
	{
		rangeData.len = g_vecOfSplitPos[iIndexOfColumn - 1];
	}
}

SearchCondition::SearchCondition():
m_nCount(0)
{

}

SearchCondition::~SearchCondition()
{

}

void SearchCondition::Add(int nColumn,EScanOp op,const char* pszValue,int (*compare)(const char *, size_t , const char *, size_t))
{
	m_vConditions.push_back(ScanCondition(nColumn,GetOption(op),(se_uint64)pszValue,strlen(pszValue),compare));
	++m_nCount;
}

std::ostream& operator<<(std::ostream& os,const SearchCondition& search)
{
	const char aOp[6][16] = {
		"<"
		,"<="
		,"=="
		,">="
		,">"
	};

	BOOST_FOREACH(BOOST_TYPEOF(*search.m_vConditions.begin()) cond,search.m_vConditions)
	{
		cout<<cond.fieldno<<setw(4)<<aOp[cond.compop - 1]
		<<setw(4)<<std::string((const char*)cond.argument,cond.arg_length);
	}
	return os;
}

ScanCondition::CompareOperation SearchCondition::GetOption(EScanOp op)
{
	switch (op)
	{
	case LessThan:
		return ScanCondition::LessThan;
	case LessEqual:
		return ScanCondition::LessEqual;
	case Equal:
		return ScanCondition::Equal;
	case GreaterEqual:
		return ScanCondition::GreaterEqual;
	case GreaterThan:
		return ScanCondition::GreaterThan;
	default:
		return ScanCondition::InvalidOperation;
	}
}

MyColumnInfo::MyColumnInfo(const std::map<int,CompareCallbacki>& mapCompare,Spliti split) : m_columnInfo(false)
{
	m_columnInfo.keys = mapCompare.size();

	if (0 != m_columnInfo.keys)
	{
		m_columnInfo.col_number = new size_t[m_columnInfo.keys];
		m_columnInfo.rd_comfunction = new CompareCallbacki[m_columnInfo.keys];
	}

	m_columnInfo.split_function = split;
	int idx = 0;
	BOOST_FOREACH(BOOST_TYPEOF(*mapCompare.begin()) comp,mapCompare)
	{
		m_columnInfo.col_number[idx] = comp.first;
		m_columnInfo.rd_comfunction[idx++] = comp.second;
	}
}

MyColumnInfo::~MyColumnInfo()
{
	delete []m_columnInfo.col_number;
	m_columnInfo.col_number = NULL;
	delete []m_columnInfo.rd_comfunction;
	m_columnInfo.rd_comfunction = NULL;
}

int str_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
	size_t i = 0;
	while(i < len1 && i < len2)
	{
		if(str1[i] < str2[i])
			return -1;
		else if(str1[i] > str2[i])
			return 1;
		else i++;

	}
	if(len1 == len2)
		return 0;
	if(len1 > len2)
		return 1;
	return -1;
}
static const char g_cSep = '|';
int vstr_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
	if (0 != len1 && str1[len1 - 1] == g_cSep)
	{
		len1 -= 1;
	}

	if (0 != len2 && str2[len2 - 1] == g_cSep)
	{
		len2 -= 1;
	}
	int nret =  str_compare(str1,len1,str2,len2);
#if 0
	char psz1[64] = {0};
	char psz2[64] ={0};
	strncpy(psz1,str1,len1);
	strncpy(psz2,str2,len2);
	printf("%s %d %s\n",psz1,nret,psz2);
#endif
	return nret;
}

void GetEntryId(Transaction *pTrans
				,EntrySet* pEntrySet
				,const std::vector<std::string>& vData
				,std::vector<EntryID>& vIds)
{
	vIds.resize(vData.size());
	std::vector<ScanCondition> vConditions;
	EntrySetScan *pScan = (EntrySetScan*)(pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vConditions));

	DataItem outData;
	EntryID outId;
	while (0 == pScan->getNext(EntrySetScan::NEXT_FLAG,outId,outData))
	{
		for(size_t idx = 0; idx < vData.size(); ++idx)
		{
			if (0 == memcmp(outData.data,vData[idx].c_str(),outData.size))
			{
                vIds[idx] = outId;
			}
		}
	}

	pEntrySet->endEntrySetScan(pScan);
}

void InsertData(Transaction * pTrans 
				,EntrySet *pEntrySet
				,const std::vector<std::string>& vData
				,std::vector<EntryID>* pvIds)
{
	for (size_t iPos = 0; iPos < vData.size(); ++iPos)
	{
		DataItem data;
		data.setData((void*)vData[iPos].c_str());
		data.setSize(vData[iPos].length());
		EntryID eid;
		pEntrySet->insertEntry(pTrans,eid,data);

		if (NULL != pvIds)
		{
			pvIds->push_back(eid);
		}
	}
	command_counter_increment();
}

void UpdateData(Transaction * pTrans
				,EntrySet *pEntrySet
				,const std::map<EntryID
				,std::string>& vData)
{
	BOOST_FOREACH(BOOST_TYPEOF(*vData.begin()) value,vData)
	{
		DataItem data;
		data.setData((void*)value.second.c_str());
		data.setSize(value.second.length());
		pEntrySet->updateEntry(pTrans,value.first,data); 
	}
	command_counter_increment();
}

void DeleteData(Transaction * pTrans
				,EntrySet *pEntrySet
				,const std::vector<EntryID>& vIds)
{
	BOOST_FOREACH(BOOST_TYPEOF(*vIds.begin()) id,vIds)
	{
		pEntrySet->deleteEntry(pTrans,id);
	}
	command_counter_increment();
}

bool operator < (const EntryID& lhs,const EntryID& rhs)
{
	se_uint64 lId = lhs.bi_hi * (1<<32) + lhs.bi_lo * (1<<16) + lhs.ip_posid;
	se_uint64 rId = rhs.bi_hi * (1<<32) + rhs.bi_lo * (1<<16) + rhs.ip_posid;
	return lId < rId;
}

bool CompareVector(std::vector<std::string>& lhs,std::vector<std::string>& rhs)
{
	if (lhs.size() == rhs.size())
	{
		for (size_t iPos = 0; iPos < lhs.size(); ++iPos)
		{
			if(lhs[iPos] != rhs[iPos])
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

void GetDataFromEntrySet(std::vector<std::string>& sResult,Transaction * pTrans,EntrySet *pEntrySet)
{
	std::vector<ScanCondition> vConditions;
	EntrySetScan *pScan = (EntrySetScan*)(pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vConditions));

	DataItem outData;
	EntryID outId;
	while (0 == pScan->getNext(EntrySetScan::NEXT_FLAG,outId,outData))
	{
		sResult.push_back(std::string((char*)outData.getData(),outData.getSize()));
	}

	pEntrySet->endEntrySetScan(pScan);
}

void GetIndexScanResults(std::vector<std::string>& sResult,Transaction * pTrans ,IndexEntrySet * pIndex, std::vector<ScanCondition>& vConditions) 
{
	IndexEntrySetScan *pScan = (IndexEntrySetScan *)(pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vConditions));

	DataItem outData;
	EntryID outId;
	while (0 == pScan->getNext(EntrySetScan::NEXT_FLAG,outId,outData))
	{
		sResult.push_back(std::string((char*)outData.getData(),outData.getSize()));
	}
	pIndex->endEntrySetScan(pScan);
}

uint32 GetSingleColInfo()
{

	static uint32 colid = 0;

	if (0 == colid)
	{
		colid = 1241234;
		using namespace boost::assign;
		std::map<int,CompareCallbacki> mapComp;
		insert(mapComp)(1,str_compare);

		std::vector<int> v;
		v.push_back(1);
		FixSpliter spliter(v);
		static MyColumnInfo columnInfo(mapComp,FixSpliter::split);

		setColumnInfo(colid,&columnInfo.Get());

	}

   
	return EntrySetCollInfo<1>::get();
}

uint32 GetMultiCollInfo()
{
	static uint32 colid = 0;

	if (0 == colid)
	{

		colid = 32589;
		using namespace boost::assign;
		std::map<int,CompareCallbacki> mapComp;
		insert(mapComp)(1,str_compare)(2,str_compare)(3,str_compare);

		std::vector<int> v;
		v += 3,2,1;
		FixSpliter spliter(v);
		static MyColumnInfo columnInfo(mapComp,FixSpliter::split);

		setColumnInfo(colid,&columnInfo.Get());
	}

	return EntrySetCollInfo<3,2,1>::get();
}

bool readfromfile = true;
void RandomGenString(std::string& strLine,size_t nLen)
{
	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::ifstream inFile(szFileName);
		if (inFile.is_open())
		{
			inFile.seekg(0,std::ios::end);
			size_t length = inFile.tellg();
			inFile.seekg(0,std::ios::beg);
			boost::shared_ptr<char> psz(new char[length]);
			inFile.read(psz.get(),length);
			strLine.append(psz.get(),length);
			return;
		}
	}

	std::string s;
	srand((unsigned)time( NULL ));
	size_t nGenerated = 0;
	for (nGenerated = 0;nGenerated < 10;++nGenerated)
	{
		char c = 0;
		s += c;
	}
	for (nGenerated = 10; nGenerated < nLen - 1; ++nGenerated)
	{
		char c = (rand()) % 26;
		s += 'a' + c;
	}
	char c = 0;
	s += c;
	strLine.append(s);

	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::fstream outFile(szFileName,std::ios_base::out);
		if(outFile.is_open())
		{
			outFile<<s;
		}
	}

}


void VarSplit(RangeDatai& rangeData, const char *pszNeedSplit,int iIndexOfColumn, size_t len)
{
	assert(NULL != pszNeedSplit);

	const char* pszStart = pszNeedSplit - 1;
	const char* pszEnd = pszNeedSplit + len,*pEnd = pszEnd;
	for (int iTimes = 1; iTimes < iIndexOfColumn; ++iTimes)
	{
		pszStart = std::find(++pszStart,pszEnd,g_cSep);
		if (pszEnd == pszStart)
		{
			return ;
		}
	}
	pszEnd = std::find(++pszStart,pszEnd,g_cSep);

	rangeData.start = pszStart - pszNeedSplit;
	rangeData.len = pszEnd - pszStart + (pszEnd != pEnd);
#if 0
	char pszPrint[40] = {0},psz[40] = {0};
	strncpy(psz,pszNeedSplit,std::min<int>(39,strlen(pszNeedSplit)));
	strncpy(pszPrint,pszNeedSplit + rangeData.start,rangeData.len);
	printf("%s的第%d列为:%s\n",psz,iIndexOfColumn,pszPrint);
#endif
}


EntrySetID TableDroper::entryId = 0;
EntrySetID TableDroper::indexId = 0;

void TableDroper::Drop(void)
{
	TransactionId xid = 0;
	Transaction *pTrans = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		//if(0 != indexId)
		//{
		//	pStorageEngine->removeIndexEntrySet(pTrans,entryId,indexId);
		//}

		if(0 != entryId)
		{
			pStorageEngine->removeEntrySet(pTrans,entryId);
		}

		pTrans->commit();
	}
	catch(StorageEngineException &ex)
	{
		cout << ex.getErrorMsg() << endl;
		pTrans->abort();
	}
}
void heap_split_to_any(RangeDatai& rd, const char *str, int col, size_t data_len)
{
	while(NULL != str && data_len > 0)
	{
		int len = SpliterGenerater::m_vc_col_array.size();
		for(int i = 1; i <= len; ++i)
		{
			if(col == i)
			{
				int sum = 0;
				for(int j = 0; j < i - 1; ++j)
				{
					sum += SpliterGenerater::m_vc_col_array[j];
				}
				rd.start = sum;
				rd.len = SpliterGenerater::m_vc_col_array[i - 1];
				break;
			}
		}
		break;
	}
}

std::vector<int> SpliterGenerater::m_vc_col_array;

SpliterGenerater::SpliterGenerater()
{
	m_pheap_cf = NULL;
	m_pindex_cf = NULL;
}

SpliterGenerater::~SpliterGenerater()
{
	if(m_pheap_cf && m_pindex_cf)
	{
		free(m_pheap_cf);
		m_pheap_cf = NULL;
	}
	if(m_pindex_cf)
	{
		free(m_pindex_cf->col_number);
		free(m_pindex_cf->rd_comfunction);
		free(m_pindex_cf);
		m_pindex_cf = NULL;
	}
	SpliterGenerater::m_vc_col_array.clear();
}

ColumnInfo* SpliterGenerater::buildHeapColInfo(const int col_count, ...)
{
	m_pheap_cf = (ColumnInfo*)malloc(sizeof(ColumnInfo));
	m_pheap_cf->col_number = 0;
	m_pheap_cf->keys = 0;
	m_pheap_cf->rd_comfunction = NULL;

	va_list arg_ptr;
	va_start(arg_ptr, col_count);
	for(int i = 0; i < col_count; ++i)
	{
		SpliterGenerater::m_vc_col_array.push_back(va_arg(arg_ptr, int));
	}

	m_pheap_cf->split_function = heap_split_to_any;
	return m_pheap_cf;
}

ColumnInfo* SpliterGenerater::buildIndexColInfo(const int col_count, 
																						const int *col_number, 
																						const CompareCallbacki *cmp_func_array,
																						const Spliti split_func)
{
	m_pindex_cf = (ColumnInfo*)malloc(sizeof(ColumnInfo));
	m_pindex_cf->keys = col_count;
	m_pindex_cf->col_number = (size_t*)malloc(sizeof(size_t) * col_count);
	for(int i = 0; i < col_count; ++i)
	{
		m_pindex_cf->col_number[i] = col_number[i];
	}
	m_pindex_cf->rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki) * col_count);
	for(int i = 0; i < col_count; ++i)
	{
		m_pindex_cf->rd_comfunction[i] = cmp_func_array[i];
	}
	m_pindex_cf->split_function = split_func;
	return m_pindex_cf;
}

DataGenerater::DataGenerater(const unsigned int row_num, const unsigned int data_len = DATA_LEN)
{
	int i = 0;
	//如果两次调用构造函数的时间间隔很短，该
	//循环可以避免产生相同的时间
	while(i++ < 1000 * 1000);
	srand((unsigned int)time(NULL));
	this->m_row_num = row_num;
	this->m_data_len = data_len;
	this->m_size = row_num;
	m_rand_data = NULL;
	m_rand_data = (char*)malloc(row_num * data_len);
}

DataGenerater::DataGenerater(const DataGenerater& dg)
{
	m_row_num = dg.m_row_num;
	m_data_len = dg.m_data_len;
	m_rand_data = (char*)malloc(m_row_num * m_data_len);
	memcpy(m_rand_data, dg.m_rand_data, m_row_num * m_data_len);
}

DataGenerater::~DataGenerater()
{
	free(m_rand_data);
	m_rand_data = NULL;
}

void DataGenerater::dataGenerate()
{	
	for(unsigned int j = 0; j < this->m_row_num; ++j)
	{
		char *p = (char *)malloc(m_data_len);
		for(unsigned int i = 0; i < m_data_len - 1; i++)
		{
			p[i] = (char)(rand() % 95 + 32);
		}
		p[m_data_len - 1] = '\0';
		memcpy(m_rand_data + j * m_data_len, p, m_data_len);
		free(p);
	}
}

void DataGenerater::dataGenerate(const char *src_data, const int data_len, int pos)
{
	if(pos >= m_row_num)
	{
		space_increment();
	}
	memcpy(m_rand_data + pos * m_data_len, src_data, data_len);
	++m_size;
}

void DataGenerater::dataToDataArray2D(char data[][DATA_LEN])
{
	memset(data, 0, m_row_num * DATA_LEN);
	for(unsigned int i = 0; i < m_row_num; ++i)
	{
		memcpy(data[i], m_rand_data + i * m_data_len, m_data_len);
	}
}

void DataGenerater::dataToDataArray2D(const int size, char data[][DATA_LEN])
{
	for(unsigned int i = 0; i < size; ++i)
	{
		memcpy(data[i], m_rand_data + i * m_data_len, m_data_len);
	}
}

void DataGenerater::space_increment()
{
	char *tmp_data = NULL;
	tmp_data = (char*)malloc(m_row_num * m_data_len);
	memcpy(tmp_data, m_rand_data, m_row_num * m_data_len);
	free(m_rand_data);
	m_rand_data = (char*)malloc((m_row_num + 10) * m_data_len);
	memcpy(m_rand_data, tmp_data, m_row_num * m_data_len);
	m_row_num += 10;
	free(tmp_data);
}

void MySleep(long millsec)
{
	boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(millsec));
}

/* 该函数用于对单个索引扫描并返回结果，仅限字符串类型的数据 */
void index_get_result(EntrySetID relid, EntrySetID idxid, int colcount, 
											int *column, ScanCondition::CompareOperation *strategynumber,
											vector<string> &v_key, vector<string> &v_data)
{
	vector<ScanCondition> v_sc;

	for (int i = 0; i < colcount; ++i)
	{
		v_sc.push_back(
			ScanCondition(
			column[i], 
			strategynumber[i], 
			(se_uint64)v_key[i].c_str(), 
			v_key[i].length(), 
			my_compare_str_index));
	}

	EntrySet *entryset = pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, relid);
	IndexEntrySet *iEntrySet = pStorageEngine->openIndexEntrySet(pTransaction, entryset, EntrySet::OPEN_SHARED, idxid);

	EntrySetScan *scan = iEntrySet->startEntrySetScan(pTransaction, EntrySet::SnapshotNOW, v_sc);

	EntryID eid;
	DataItem di;
	while((scan->getNext(EntrySetScan::NEXT_FLAG, eid, di)) != NO_DATA_FOUND)
	{
		v_data.push_back((char *)di.getData());
		MemoryContext::deAlloc(di.getData());
	}

	iEntrySet->endEntrySetScan(scan);
	pStorageEngine->closeEntrySet(pTransaction, iEntrySet);
	pStorageEngine->closeEntrySet(pTransaction, entryset);
}

/* 
 * 模拟索引扫描表，使用vector集合和关键字返回vector中的子集
 * 该函数返回的是小于或者等于关键字的集合
 * 一个vector传进来的时候在外头需要使用sort函数排好序
 * 该函数仅适用于字符串类型的数据
 */
static
void vector_index_get_result(vector<string> &v_data, 
														 int *start_pos,
														 vector<string> &key, 
														 vector<string> &result)
{
	vector<string>::iterator it = v_data.begin();
	while(it != v_data.end())
	{
		bool is_equal = true;
		for (int i = 0; i < key.size(); ++i)
		{
			string k  = key[i];
			/* 从v_data[i]中截取出索引列来比较 */
			if (it->substr(start_pos[i], key[i].length()).compare(key[i]) > 0)
			{
				is_equal = false;
				break;
			}
		}
		if (is_equal)
			result.push_back(*it);

		++it;
	}
}

void get_all_tuple_in_heap(EntrySetID relid, vector<string> &v_data)
{
	vector<ScanCondition> v_sc;
	EntrySet *entryset = pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, relid);
	EntrySetScan *scan = entryset->startEntrySetScan(pTransaction, EntrySet::SnapshotNOW, v_sc);

	EntryID eid;
	DataItem di;
	while((scan->getNext(EntrySetScan::NEXT_FLAG, eid, di)) != NO_DATA_FOUND)
	{
		v_data.push_back((char *)di.getData());
		MemoryContext::deAlloc(di.getData());
	}

	entryset->endEntrySetScan(scan);
	pStorageEngine->closeEntrySet(pTransaction, entryset);
}

/* 检测测试结果,v_data需要在外头排序过后再传进来 */
bool check_test_result(vector<string> *v_data, EntrySetID relid, EntrySetID idxid, vector<string> &v_key)
{
	bool return_sta = true;
	vector<string> v_rdata, v_alldata, v_rdata2;
	int *colnum = new int[v_key.size()];
	ScanCondition::CompareOperation *strategynumber = new ScanCondition::CompareOperation[v_key.size()];
	for (int i = 0; i < v_key.size(); ++i)
	{
		colnum[i] = i + 1;
		strategynumber[i] = ScanCondition::LessEqual;
	}

	index_get_result(relid, idxid, v_key.size(), colnum, strategynumber, v_key, v_rdata);
	get_all_tuple_in_heap(relid, v_alldata);

	int start_pos[] = {0, 3};
	vector_index_get_result(*v_data, start_pos, v_key, v_rdata2);
	sort(v_rdata.begin(), v_rdata.end());
	sort(v_alldata.begin(), v_alldata.end());
	sort(v_rdata2.begin(), v_rdata2.end());

	if ((*v_data) != v_alldata || v_rdata2 != v_rdata)
		return_sta = false;

	delete []colnum;
	delete []strategynumber;

	return return_sta;
}