/**
* @file test_index_insert.cpp
* @brief 
* @author 李书淦
* @date 2011-9-21 15:49:40
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <boost/assign.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "index/test_index_insert_dll.h"
using namespace std;
using namespace FounderXDB::StorageEngineNS;
extern Transaction* pTransaction;
typedef EntrySetCollInfo<1> SingleEntrySetT;
typedef EntrySetCollInfo<3,2,1> MultiEntrySetT;
bool testIndexInsert_SingleColumn( void )
{
	INTENT("1.创建EntrySet,插入一些数据;\n"
		"2.创建IndexEntrySet;\n"
		"3.插入其他一些数据;\n"
		"4.查询结果是否正确.\n");
	try
	{
		//创建表
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,SingleEntrySetT::get());

		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		std::vector<std::string> vInsertData;
		vInsertData += "1","2","3";
		InsertData(pTransaction,pEntrySet,vInsertData);

        //创建index并插入一些其他的数据
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,SingleEntrySetT::get());
		IndexEntrySet *pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);
		vInsertData.clear();
		vInsertData += "5","6","7";
		InsertData(pTransaction,pEntrySet,vInsertData);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessEqual,"7",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired += "1","2","3","5","6","7";



		CHECK_BOOL(sResult == sDesired);
		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

	}
	CATCHEXCEPTION

	return true;
}

bool testIndexInsert_SameScaKey( void )
{
	INTENT("1.创建EntrySet,插入一些数据;\n"
		"2.创建IndexEntrySet;\n"
		"3.插入其他一些数据;\n"
		"4. 构造一个查询，两个条件完全相同;\n"
		"4.查询结果是否正确.\n");
	try
	{
		//创建表
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,SingleEntrySetT::get());

	   EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		std::vector<std::string> vInsertData;
		vInsertData += "1","2","3";
		InsertData(pTransaction,pEntrySet,vInsertData);

		//创建index并插入一些其他的数据
		TableDroper::indexId =  pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,SingleEntrySetT::get());
		IndexEntrySet *pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);
		vInsertData.clear();
		vInsertData += "5","6","7";
		InsertData(pTransaction,pEntrySet,vInsertData);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessEqual,"7",str_compare);
		scanCondition.Add(1,LessEqual,"8",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired += "1","2","3","5","6","7";
		CHECK_BOOL(sResult == sDesired);
		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

	}
	CATCHEXCEPTION

		return true;
}

bool testIndex_InAnotherTrans(void)
{
	INTENT("1.创建EntrySet,插入一些数据;"
		"2.创建IndexEntrySet;"
		"3.插入其他一些数据;"
		"4.Commit;"
		"5.开启另外一个事务并查询结果");

	using namespace boost::assign;
	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,SingleEntrySetT::get());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		std::vector<std::string> vInsertData;
		vInsertData += "1","2","3";
		InsertData(pTransaction,pEntrySet,vInsertData);

		//创建index并插入一些其他的数据
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,SingleEntrySetT::get());
		IndexEntrySet* pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

		vInsertData.clear();
		vInsertData += "5","6","7";
		InsertData(pTransaction,pEntrySet,vInsertData);
		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = static_cast<EntrySet*>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId,NULL));
		IndexEntrySet *pIndex = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId));

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessEqual,"7",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired += "1","2","3","5","6","7";
		CHECK_BOOL(sResult == sDesired);
		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

	}
	CATCHEXCEPTION
	return true;
}


bool testIndex_Sort()
{
	INTENT("1.创建EntrySet,插入一些数据;"
		"2.在其中两列上创建IndexEntrySet;"
		"3.插入其他一些数据;"
		"4.构造一个查询：(与第一个键相等)"
		"5.检查查询结果是否按第二个键排序.");
	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,MultiEntrySetT::get());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		std::vector<std::string> vInsertData;
		vInsertData += "123456","123789","012562","012120","012456";
		InsertData(pTransaction,pEntrySet,vInsertData);

		//创建index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,IndexCollInfo<MultiEntrySetT,1,2>::get());
		IndexEntrySet *pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,Equal,"012",str_compare);

		//检查结果是否是按照第二关健字排序
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,scanCondition.Keys());

		std::vector<std::string> vDesired;
		vDesired += "012120","012456","012562";
		CHECK_BOOL(CompareVector(vResult, vDesired));

		ostream_iterator<std::string> out(std::cout," ");
		std::copy(vResult.begin(),vResult.end(),out);
		std::cout<<endl;

		std::copy(vDesired.begin(),vDesired.end(),out);
		std::cout<<endl;

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION
    return true;
}



static void cluster_index_intstr_split(RangeDatai &rangeData,const char*str, int coli, size_t charlen)
{
    assert(coli == 1 && charlen == sizeof(int));
    
    rangeData.start = 0;
	rangeData.len = sizeof(int);
	return;
}

static
int intcmp(const char *str1, size_t len1, const char *str2, size_t len2)
{
    assert(len1 == sizeof(int) && len2 == sizeof(int));
    int a = *(int*)str1;
    int b = *(int*)str2;
    return a > b ? 1 : (a < b ? -1 : 0);
}

bool test_cluster_index()
{
    int arr[10];
    
    INTENT("test cluster index, insert key-value pairs, and then scan.");
    StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId xid = 0;
	int counter = 0;
	ColumnInfo colinfo;
	uint32 mycolinfoid = 65217;
	size_t colnums[2] = {1, 2};
	CompareCallbacki ccb[2] = {intcmp, 0};
	colinfo.col_number = colnums;
	colinfo.keys = 1;
	colinfo.rd_comfunction = ccb;
	colinfo.split_function = cluster_index_intstr_split;
	setColumnInfo(mycolinfoid, &colinfo);
    try {	
	    pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	    TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction, 0, 
	        FounderXDB::StorageEngineNS::CLUSTER_BTREE_INDEX_ENTRY_SET_TYPE, mycolinfoid);// we don't need this FounderXDB::StorageEngineNS::BTREE_COLINFO_ID
    	
	    IndexEntrySet* ies = (IndexEntrySet*)pStorageEngine->openIndexEntrySet(pTransaction, 0, EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);
    	
	    DataItem key, data;
    	int i, j;
	    for (  i = 0; i < 10; i++) {
	        key.setData(&i);
	        key.setSize(sizeof(i));
	        j = 100 + i;
	        arr[i] = j;
	        data.setData(&j);
	        data.setSize(sizeof(j));
	        ies->insertEntry(pTransaction, key, data);
        }
        j = 5;
        ScanCondition scancond(1,ScanCondition::GreaterEqual, (se_uint64)(&j),sizeof(j), intcmp);
        std::vector<ScanCondition> scs;
        scs.push_back(scancond);
        EntrySetScan *iess = ies->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, scs);
        int ret = 0;
        DataItem dataent;
        EntryID eid;
        counter = 0;
        do {
            ret = iess->getNext(EntrySetScan::NEXT_FLAG, eid, dataent);
            assert(dataent.getSize() == sizeof(int)*2);
            int keyi = *(int *)dataent.getData();
            int datai = *(int *)((char *)dataent.getData() + sizeof(int));
            counter++;
            assert(arr[keyi] == datai);
        } while (ret == 0);
        ies->endEntrySetScan(iess);
        assert(counter == 6);
        
        counter = 0;
        iess = ies->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, scs);
        do {
            ret = iess->getNext(EntrySetScan::PREV_FLAG, eid, dataent);
            assert(dataent.getSize() == sizeof(int)*2);
            int keyi = *(int *)dataent.getData();
            int datai = *(int *)((char *)dataent.getData() + sizeof(int));
            counter++;
            assert(arr[keyi] == datai);
        } while (ret == 0);
        ies->endEntrySetScan(iess);
        assert(counter == 6);
        
        i = 8;
        counter = 0;
        ScanCondition scancond2(1,ScanCondition::LessEqual, (se_uint64)(&i),sizeof(i), intcmp);
        scs.push_back(scancond2);
        iess = ies->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, scs);
        do {
            ret = iess->getNext(EntrySetScan::PREV_FLAG, eid, dataent);
            assert(dataent.getSize() == sizeof(int)*2);
            int keyi = *(int *)dataent.getData();
            int datai = *(int *)((char *)dataent.getData() + sizeof(int));
            counter++;
            assert(arr[keyi] == datai);
        } while (ret == 0);
        ies->endEntrySetScan(iess);
        assert(counter == 5);
        
        scs.clear();// whole table scan
        counter = 0;
        iess = ies->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, scs);
        do {
            ret = iess->getNext(EntrySetScan::PREV_FLAG, eid, dataent);
            assert(dataent.getSize() == sizeof(int)*2);
            int keyi = *(int *)dataent.getData();
            int datai = *(int *)((char *)dataent.getData() + sizeof(int));
            counter++;
            assert(arr[keyi] == datai);
        } while (ret == 0);
        assert(counter == 11);
        
        ies->endEntrySetScan(iess);
        pStorageEngine->closeEntrySet(pTransaction,ies);
        
        
	    EntrySetID entsid = pStorageEngine->createIndexEntrySet(pTransaction, 0, 
	        FounderXDB::StorageEngineNS::CLUSTER_UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE, mycolinfoid);// we don't need this FounderXDB::StorageEngineNS::BTREE_COLINFO_ID
    	
	    ies = (IndexEntrySet*)pStorageEngine->openIndexEntrySet(pTransaction, 0, EntrySet::OPEN_EXCLUSIVE, entsid);
          
        char somebytes[256];
        time_t starttime, endtime;
        time(&starttime);
        for (i = 0; i < 10000; i++) {
	        key.setData(&i);
	        key.setSize(sizeof(i));
	        data.setData(somebytes);
	        data.setSize(sizeof(somebytes));
	        ies->insertEntry(pTransaction, key, data);
        }
        time(&endtime);
        cout<<"\nCluster index 10000 entries (2.6M) insert taken(s):"<<endtime-starttime;
        
        counter = 0;
        time(&starttime);
        iess = ies->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, scs);
        do {
            ret = iess->getNext(EntrySetScan::NEXT_FLAG, eid, dataent);
            assert(dataent.getSize() == sizeof(int) + sizeof(somebytes));
            int keyi = *(int *)dataent.getData();
            if (ret == 0) {
                assert(keyi == counter);
                assert(dataent.getSize() - sizeof(int) == sizeof(somebytes) && 
                    memcmp(somebytes, (char *)dataent.getData() + sizeof(int), sizeof(somebytes)) == 0);
            }
            counter++;
        } while (ret == 0);

        assert(counter == 10001);
        ies->endEntrySetScan(iess);
        time(&endtime);
        cout<<"\nCluster index 10000 entries (2.6M) scan taken(s):"<<endtime-starttime;

        // mark&restore test
        iess = ies->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, scs);
        bool marked = false, restored = false;
        counter = 0;
        do {
            ret = iess->getNext(EntrySetScan::NEXT_FLAG, eid, dataent);
            assert(dataent.getSize() == sizeof(int) + sizeof(somebytes));
            int keyi = *(int *)dataent.getData();
            if (ret == 0) {
                assert(keyi == counter);
                assert(dataent.getSize() - sizeof(int) == sizeof(somebytes) && 
                    memcmp(somebytes, (char *)dataent.getData() + sizeof(int), sizeof(somebytes)) == 0);
            }
            counter++;
            if (!marked && counter == 500) {
                iess->markPosition();
                marked = true;
            } else if (marked && !restored && counter == 9000) {
                iess->restorePosition();
                restored = true;
                counter = 500;
            }
        } while (ret == 0);

        assert(counter == 10001);
        ies->endEntrySetScan(iess);
        
                        
                // uniqueness check test
        try {
            i = 5234; // dup
            key.setData(&i);
	        key.setSize(sizeof(i));
	        data.setData(somebytes);
	        data.setSize(sizeof(somebytes));
	        ies->insertEntry(pTransaction, key, data);
        } catch (StorageEngineException &e) {
            // ignore it, proceed as usual.
            cout<<"\nCluster index uniqueness check OK.";
        }
 
        
        
        
        // big data insert tes
        try {
        
            i = 50001; //  
            key.setData(&i);
	        key.setSize(sizeof(i));
	        char *dbytes = (char *)malloc(8192);//too big
	        DataItem di(dbytes, 8192, free);// deallocator.
	        
	        data.setData(dbytes);
	        data.setSize(8192);
	        ies->insertEntry(pTransaction, key, data);
            
            counter = 0;
            ScanCondition scancond3(1,ScanCondition::Equal, (se_uint64)(&i), sizeof(i), intcmp);
            scs.push_back(scancond3);
            iess = ies->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, scs);
            do {
                ret = iess->getNext(EntrySetScan::NEXT_FLAG, eid, dataent);

                int keyi = *(int *)dataent.getData();
                if (ret == 0) {
                    assert(dataent.getSize() == sizeof(int) + 8192);// NEEDS uncompression.
                    assert(keyi == 50001);
                    assert(memcmp(dbytes, (char *)dataent.getData() + sizeof(int), 8192) == 0);
                }
                counter++;
            } while (ret == 0);
            assert(counter == 2);
            ies->endEntrySetScan(iess);	        
	        
	        
            i = 50000; //  
            key.setData(&i);
	        key.setSize(sizeof(i));
	        dbytes = (char *)malloc(819200);//too big
	        DataItem di2(dbytes, 819200, free);// deallocator.
	        
	        data.setData(dbytes);
	        data.setSize(819200);
	        ies->insertEntry(pTransaction, key, data);//should fail
        } catch (StorageEngineException&e) {
            cout<<"\nCluster index, big data rejected.";
        }
        
        pStorageEngine->closeEntrySet(pTransaction,ies);
		pTransaction->commit();
        colinfo.rd_comfunction = 0;
     	colinfo.col_number = 0;
	}
	CATCHEXCEPTION
    return true;

    
}
