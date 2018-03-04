/**
* @file test_transaction_index_dll.cpp
* @brief 
* @author 李书淦
* @date 2011-9-23 14:45:53
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
#include "transaction/test_transaction_index_dll.h"
using namespace std;
using namespace boost::assign;
using namespace FounderXDB::StorageEngineNS;
extern Transaction* pTransaction;
static EntrySetID SetUp(std::vector<std::string>& vInsertedData,std::vector<EntryID>* vIds = NULL)
{
    try
	{
		StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetSingleColInfo());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入数据
		vInsertedData += "2","9","7","5";
        InsertData(pTransaction,pEntrySet,vInsertedData,vIds);

		//关闭
        pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	return TableDroper::entryId;
}
bool test_trans_index_InsertRollback(void)
{
	INTENT("1. 创建一个表插入一些数据后提交;"
		   "2. 开启另外一个事务;"
		   "3. 插入另外一些数据后回滚;"
		   "4. 开启一个事务检查原子性.");

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	//创建一个表插入一些数据后提交; 2 5 7 9
	std::vector<std::string> vInserted;
	EntrySetID eid = SetUp(vInserted);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		//插入一些其他数据
		std::vector<std::string> vOtherData;
		vOtherData += "8","6","3";
		InsertData(pTransaction,pSet,vOtherData,NULL);

		//回滚
		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->abort();
	}
	CATCHEXCEPTION

	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pTransaction,pSet);

		//检查原子性
		std::set<std::string> sDesired(vInserted.begin(),vInserted.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());

		CHECK_BOOL(sDesired == sResult);
		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->commit();
	}
	CATCHEXCEPTION
    return true;
}

bool test_trans_index_UpdateRollback(void)
{
	INTENT("1. 创建一个表插入一些数据后提交;"
		"2. 开启另外一个事务;"
		"3. 更新其中的一些数据后回滚;"
		"4. 开启一个事务检查原子性.");

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	//创建一个表插入一些数据后提交; 2 5 7 9
	std::vector<EntryID> vIds;
	std::vector<std::string> vInserted;
	EntrySetID eid = SetUp(vInserted,&vIds);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		//更新数据
		std::map<EntryID,std::string> updateData;
		insert(updateData)(vIds[1],"6")(vIds[2],"3"); //5->6 7->3
        UpdateData(pTransaction,pSet,updateData);

        //回滚
		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->abort();
	}
	CATCHEXCEPTION

	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pTransaction,pSet);

		//检查原子性
		std::set<std::string> sDesired(vInserted.begin(),vInserted.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());

		CHECK_BOOL(sDesired == sResult);
		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	return true;
}

bool test_trans_index_DeleteRollback(void)
{
	INTENT("1. 创建一个表插入一些数据后提交;"
		"2. 开启另外一个事务;"
		"3. 删除其中的一些数据后回滚;"
		"4. 开启一个事务检查原子性.");

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	//创建一个表插入一些数据后提交; 2 5 7 9
	std::vector<EntryID> vIds;
	std::vector<std::string> vInserted;
	EntrySetID eid = SetUp(vInserted,&vIds);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		//删除一些数据
		std::vector<EntryID> vDeleteIds;
		vDeleteIds += vIds[0],vIds[2],vIds[3];
		DeleteData(pTransaction,pSet,vDeleteIds);

		//回滚
		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->abort();
	}
	CATCHEXCEPTION

	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pTransaction,pSet);

		//检查原子性
		std::set<std::string> sDesired(vInserted.begin(),vInserted.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());

		CHECK_BOOL(sDesired == sResult);
		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	return true;
}