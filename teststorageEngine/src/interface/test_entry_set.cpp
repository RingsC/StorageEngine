
#include <map>
#include <string>
#include <iostream>
#include <vector>

#include "postgres.h"

#include "DataItem.h"
#include "EntrySet.h"
#include "Transaction.h"
#include "interface/PGEntrySet.h"
#include "interface/test_entry_set.h"
#include "StorageEngine.h"
#include "interface/PGStorageEngine.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/PGTransaction.h"
#include "interface/PGIndexEntrySet.h"



using namespace FounderXDB::StorageEngineNS;
using std::string;
using std::cout;
using std::endl;
using std::vector;

int wfhtest_compare(char *str1, int len1, char *str2, int len2)
{
	int i = 0;
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


//create first and redo creation
bool test_storage_engine_create()
{
// create index
    //StorageEngine  *storage_engine = PGStorageEngine::getInstance();
    //TDTransactionId invalid_transaction = InvalidTransactionId;
    //Transaction *transaction = storage_engine->getTransaction(invalid_transaction, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
    //string name("test");
    //PGIndexEntrySet *pindex = NULL;
    //try {
    //    EntrySet *entryset = static_cast<PGEntrySet *>(storage_engine->openEntrySet(transaction,EntrySet::OPEN_EXCLUSIVE,1001));
    //    pindex = static_cast<PGIndexEntrySet *>(storage_engine->openIndexEntrySet(transaction, entryset, 0, 1002, NULL, NULL));

    //    //storage_engine->closeEntrySet(transaction, pindex);
    //    //Oid id = pindex->getId();
    //    //storage_engine->removeEntrySet(transaction, id);

    //    //storage_engine->closeEntrySet(transaction, pindex);
    //    //IndexEntrySet *penry_set = storage_engine->createIndexEntrySet(transaction, entryset,BTREE_INDEX_ENTRY_SET, name, 0);
    //}
    //catch (StorageEngineExceptionUniversal &ex) {
    //    cout << ex.getErrorNo() << endl;
    //    cout << ex.getErrorMsg() << endl;
    //    return false;
    //}



    //int len = sizeof("testdata_1");
    //char *pstr = (char *)palloc(len);
    //memcpy(pstr, "testdata_1", len);
    //Datum datam = PointerGetDatum(pstr);
    //ScanCondition cond(1, ScanCondition::Equal, datam, len, wfhtest_compare);
    //std::vector<ScanCondition> scankeys;
    //scankeys.push_back(cond);

    //EntrySetScan *pindex_scan = pindex->startEntrySetScan(transaction,0, scankeys);


    //std::vector<std::pair<EntryID, DataItem*> > entry;
    //try {
    //    std::vector<std::pair<EntryID, DataItem*> > eid_entries;
    //    //EntryID eid = 0;
    //    //DataItem *p = new DataItem;
    //    //pindex_scan->getNext(0, eid, *p);
    //    pindex_scan->getNextBatch(6, eid_entries);
    //    cout << eid_entries.size() << endl;
    //} catch (StorageEngineExceptionUniversal &ex) {
    //    cout << ex.getErrorMsg() << endl;
    //    cout << ex.getErrorNo() << endl;
    //}

    //vector<std::pair<EntryID, DataItem*> >::iterator iter;
    //iter = entry.begin();
    //cout << "tid:" << iter->first << "content:" << iter->second->getData() << endl;

   /* if (NULL == penry_set) {
        return false;
    }
    int len = sizeof("testdata_1");
    char *pstr = (char *)palloc(len);
    memcpy(pstr, "testdata_1", len);
    Datum datam = PointerGetDatum(pstr);

    ScanCondition cond(1, ScanCondition::Equal, datam, len, my_compare);
    std::vector<ScanCondition> scankeys;
    scankeys.push_back(cond);

    EntrySetScan *entry_scan = penry_set->startEntrySetScan(transaction, scankeys.size(), scankeys);


    std::vector<std::pair<EntryID, DataItem*> > entry;
    try {
        entry_scan->getNextBatch(6, entry);
    } catch (StorageEngineExceptionUniversal &ex) {
        ex.getErrorMsg();
        ex.getErrorNo();
    }*/

    //cout << entry.size() << endl;


    //create a heap
    StorageEngine  *storage_engine = PGStorageEngine::getInstance();
    TDTransactionId invalid_transaction = InvalidTransactionId;
    Transaction *transaction = storage_engine->getTransaction(invalid_transaction, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
    string name("test");
    EntrySet *penry_set = NULL;
    try {
        
    }
    catch (StorageEngineExceptionUniversal &ex) {
        cout << ex.getErrorNo() << endl;
        cout << ex.getErrorMsg() << endl;
        return false;
    }
    if (NULL == penry_set) {
        return false;
    }
    PGIndexEntrySet *pindex = NULL;
    try {
        pindex = static_cast<PGIndexEntrySet *>(storage_engine->openIndexEntrySet(transaction, penry_set, 0, 1002, NULL, NULL));
    } catch (StorageEngineExceptionUniversal &ex) {
        cout << ex.getErrorNo() << endl;
        cout << ex.getErrorMsg() << endl;
    }

    //int len = sizeof("testdata_1");
    //char *pstr = (char *)palloc(len);
    //memcpy(pstr, "testdata_1", len);
    //Datum datam = PointerGetDatum(pstr);

  /*  ScanCondition cond(1, ScanCondition::Equal, datam, len, my_compare);
    std::vector<ScanCondition> scankeys;
    scankeys.push_back(cond);

    EntrySetScan *entry_scan = penry_set->startEntrySetScan(transaction, scankeys.size(), scankeys);


    std::vector<std::pair<EntryID, DataItem*> > entry;
    try {
        entry_scan->getNextBatch(6, entry);
    } catch (StorageEngineExceptionUniversal &ex) {
        ex.getErrorMsg();
        ex.getErrorNo();
    }

    cout << entry.size() << endl;*/




    char data[20][20] = {"testdata_1", "testdata_2", "testdata_3", "testdata_1",\
    "testdata_2", "testdata_1", "apple", "reboot", "apple"};
    EntrySetID eid;

        for (int cnt = 0; cnt < 10000; ++cnt) {
            for(int i = 0; i < 9; ++i)
            {
                DataItem entry(data[i], strlen(data[i]));
                try {
                    penry_set->insertEntry(transaction, eid, entry);
                } catch (StorageEngineExceptionUniversal &ex) {
                        cout << ex.getErrorNo() << endl;
                        cout << ex.getErrorMsg() << endl;
                }

                try {
                    pindex->insertEntry(transaction, entry, eid);
                } catch (StorageEngineExceptionUniversal &ex) {
                    // how to do this 
                        transaction->abort();
                        cout << ex.getErrorNo() << endl;
                        cout << ex.getErrorMsg() << endl;
                }
            }
        }

    transaction->commit();

    return true;
}



int test_entry_set()
{

  /*  PGEntrySet test_entry_set("test", HEAP_ROW_ENTRY_SET_TYPE, 222222);
    void *pdata = palloc(sizeof("hello"));
    memcpy(pdata, "hello", sizeof("hello"));
    DataItem data(pdata, sizeof("hello"));
    EntryID eid = 0;
    test_entry_set.insertEntry(NULL, eid, data);*/

    return 0;
}
