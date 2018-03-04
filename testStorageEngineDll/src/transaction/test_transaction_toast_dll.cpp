/**
* @file test_transaction_toast_dll.cpp
* @brief 
* @author 李书淦
* @date 2011-9-26 15:11:36
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
#include "transaction/test_transaction_toast_dll.h"
using namespace std;
using namespace boost::assign;
using namespace FounderXDB::StorageEngineNS;
extern Transaction* pTransaction;
/************************************************************************** 
* @brief SetUp 
* 创建一个表，并向其中插入一些大数据，同时返回插入的数据，数据对应的id以及表的id
* Detailed description.
* @param[out] vInserted  插入的数据
* @param[out] vIds 数据对应的Id
* @return EntrySetID  表的id
**************************************************************************/
static EntrySetID SetUp(std::vector<std::string>& vInserted,std::vector<EntryID>* vIds = NULL)
{
	try
	{
		StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetMultiCollInfo());
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);
		
		//随机生成将要插入的数据
		std::string strData1,strData2,strData3,strData4;
		RandomGenString(strData1,1<<14);
		RandomGenString(strData2,1<<16);
		RandomGenString(strData3,1<<13);
		RandomGenString(strData4,1<<11);

		vInserted += strData1,strData2,strData3,strData4;

		InsertData(pTransaction,pEntrySet,vInserted,vIds);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	return TableDroper::entryId;
}
bool testToastTrans_InsertRollback_DLL( void )
{
	INTENT("1. 创建一个表插入一些大数据后提交;"
		"2. 开启另外一个事务;"
		"3. 插入另外一些大数据后回滚;"
		"4. 开启一个事务检查原子性.");

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	//创建一个表插入一些数据后提交
	std::vector<std::string> vInserted;
	EntrySetID eid = SetUp(vInserted);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		//随机生成将要插入的数据
		std::vector<std::string> vOtherData;
		std::string strData1,strData2;
		RandomGenString(strData1,1<<17);
		RandomGenString(strData2,1<<12);
		vOtherData += strData1,strData2;

		//插入并回滚
		InsertData(pTransaction,pSet,vOtherData);

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

bool testToastTrans_UpdateRollback_DLL(void)
{
	INTENT("1. 创建一个表插入一些大数据后提交;"
		"2. 开启另外一个事务;"
		"3. 更新其中一些大数据后回滚;"
		"4. 开启一个事务检查原子性.");

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	//创建一个表插入一些数据后提交
	std::vector<EntryID> vIds;
	std::vector<std::string> vInserted;
	EntrySetID eid = SetUp(vInserted,&vIds);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		//随机生成用于update的数据
		std::string strUpdateData1,strUpdateData2;
		RandomGenString(strUpdateData1,1<<17);
		RandomGenString(strUpdateData2,1<<12);

		std::map<EntryID,std::string> updateData;
		insert(updateData)(vIds[0],strUpdateData1)(vIds[3],strUpdateData2);
        UpdateData(pTransaction,pSet,updateData);   //


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

bool testToastTrans_DeleteRollback_DLL(void)
{
	INTENT("1. 创建一个表插入一些大数据后提交;"
		"2. 开启另外一个事务;"
		"3.删除一些大数据后回滚;"
		"4. 开启一个事务检查原子性.");

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	//创建一个表插入一些数据后提交
	std::vector<EntryID> vIds;
	std::vector<std::string> vInserted;
	EntrySetID eid = SetUp(vInserted,&vIds);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

        //删除其中一些数据
		std::vector<EntryID> vDeleteIds;
		vDeleteIds.push_back(vIds[1]);
		vDeleteIds.push_back(vIds[3]);

		DeleteData(pTransaction,pSet,vIds);

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

