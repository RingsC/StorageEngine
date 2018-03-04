/**
* @file test_index_update_dll.cpp
* @brief 
* @author 李书淦
* @date 2011-9-23 10:08:35
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
#include "index/test_index_update_dll.h"
extern Transaction* pTransaction;
using namespace std;
using namespace FounderXDB::StorageEngineNS;

bool testIndexUpdate_SingleColumn_DLL()
{
	INTENT("1.创建EntrySet,插入一些数据;\n"
		"2.创建IndexEntrySet;\n"
		"3.更新其中的一些数据;\n"
		"4.查询结果是否正确.\n");
	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetSingleColInfo());
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		std::vector<std::string> vInsertData;
		std::vector<EntryID> vIds;
		vInsertData += "3","4","2","6","9";
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//创建index并插入一些其他的数据
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetSingleColInfo());
		IndexEntrySet *pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);
		vInsertData.clear();
		vInsertData += "1";
		InsertData(pTransaction,pEntrySet,vInsertData);

		//update data
		std::map<EntryID,std::string> mapUpdateData;
		insert(mapUpdateData)(vIds[2],"8")(vIds[4],"0");   //update "2"->"8" "9"->"0"
		UpdateData(pTransaction,pEntrySet,mapUpdateData);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessEqual,"6",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired += "0","1","3","4","6";
		CHECK_BOOL(sResult == sDesired);
		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

	}
	CATCHEXCEPTION
	return true;
}

bool testIndexUpdate_MultiColumn_DLL()
{
	INTENT("1.创建EntrySet,插入一些数据;\n"
		"2.创建IndexEntrySet;\n"
		"3.更新其中的一些数据;\n"
		"4.查询结果是否正确.\n");
	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetMultiCollInfo());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		std::vector<std::string> vInsertData;
		std::vector<EntryID> vIds;
		vInsertData += "123456","123789","012562","012120","012456";
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//创建index并插入一些其他的数据
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetMultiCollInfo());
		IndexEntrySet *pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

		vInsertData.clear();
		vInsertData += "012506";
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//update data
		std::map<EntryID,std::string> mapUpdateData;
		insert(mapUpdateData)(vIds[5],string("124889"))(vIds[0],string("121671"));   //update "012506"->"124889" "123456"->"121671"
		UpdateData(pTransaction,pEntrySet,mapUpdateData);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessThan,"123",str_compare);
		scanCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired += "012562","121671";
		CHECK_BOOL(sResult == sDesired);

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

	}
	CATCHEXCEPTION
	return true;
}

bool testIndexUpdate_InAnotherTrans_DLL()
{
	INTENT("1.创建EntrySet,插入一些数据;\n"
		"2.创建IndexEntrySet;\n"
		"3.更新其中的一些数据;\n"
		"4.提交事务并开启另外一个事务"
		"5.构造一个查询并检查结果是否正确.\n");
	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	using namespace boost::assign;
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetMultiCollInfo());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		std::vector<std::string> vInsertData;
		std::vector<EntryID> vIds;
		vInsertData += "123456","123789","012562","012120","012456";
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//创建index并插入一些其他的数据
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetMultiCollInfo());
		IndexEntrySet *pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);
		vInsertData.clear();
		vInsertData += "012506";
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//update data
		std::map<EntryID,std::string> mapUpdateData;
		insert(mapUpdateData)(vIds[5],string("124889"))(vIds[0],string("121671"));   //update "012506"->"124889" "123456"->"121671"
		UpdateData(pTransaction,pEntrySet,mapUpdateData);

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

	}
	CATCHEXCEPTION

	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = static_cast<EntrySet*>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId));
		IndexEntrySet *pIndex = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId));

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessThan,"123",str_compare);
		scanCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired += "012562","121671";
		CHECK_BOOL(sResult == sDesired);

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION
	return true;
}
