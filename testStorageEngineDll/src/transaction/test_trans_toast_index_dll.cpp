/**
* @file test_trans_toast_index_dll.cpp
* @brief 
* @author 李书淦
* @date 2011-9-26 15:44:13
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
#include <boost/tuple/tuple.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "transaction/test_trans_toast_index_dll.h"
extern Transaction* pTransaction;
using namespace std;
using namespace boost::assign;
using namespace FounderXDB::StorageEngineNS;
/************************************************************************** 
* @brief PrepareData 
* 随时生成并返回需要插入的数据
* Detailed description.
* @param[out] sInserted  返回需要插入数据的集合
* @param[out] sResult 返回使用index查询的结果
**************************************************************************/
static void PrepareData(std::vector<std::string>& sInserted,std::set<std::string>& sResult)
{
	using namespace boost::assign;
	std::string strInsertData1 ;
	std::string strInsertData2 ;
	std::string strInsertData3 ;
	std::string strInsertData4 ;
	std::string strInsertData5 ;

	strInsertData1 += "123456";
	strInsertData2 +="123789";
	strInsertData3 +="012562";
	strInsertData4 +="012120";
	strInsertData5 +="012856";
    
	RandomGenString(strInsertData1,1<<20);
	RandomGenString(strInsertData2,1<<8);
	RandomGenString(strInsertData3,1<<21);
	RandomGenString(strInsertData4,1<<17);
	RandomGenString(strInsertData5,1<<19);
    sInserted += strInsertData1,strInsertData2,strInsertData3,strInsertData4,strInsertData5;
    sResult += strInsertData3,strInsertData5;

}

/************************************************************************** 
* @brief PrepareDBAndIndex 
* 创建数据库，并在其上建立索引，返回索引查询的结果
* Detailed description.
* @param[in] idxId 索引标识
* @param[out] sResult 索引查询结果
**************************************************************************/
static std::pair<EntrySetID,EntrySetID> PrepareDBAndIndex(std::set<std::string>& sResult
														  ,std::vector<std::string>& vInserted
														  ,std::vector<EntryID>* vIds = NULL)
{
	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId xid = 0;
	pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
    //EntrySetID eId,eIdxId;
	try
	{

		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetMultiCollInfo());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//insert some toast data
		PrepareData(vInserted,sResult);
		InsertData(pTransaction,pEntrySet,vInserted,vIds);

		//创建index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetMultiCollInfo());
		IndexEntrySet *pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		//check the result
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,searchCondition.Keys());
		std::set<std::string> setResult(vResult.begin(),vResult.end());
		CHECK_BOOL(setResult == sResult);

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

	}
	catch(StorageEngineException &ex)
	{
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		pStorageEngine->removeEntrySet(pTransaction,TableDroper::entryId);
		pTransaction->abort();
		TableDroper::entryId = TableDroper::indexId = 0;
	}
	return make_pair(TableDroper::entryId,TableDroper::indexId);
}


bool test_toastIndex_InsertRollabck_DLL(void)
{
	INTENT( "1.创建表并插入一些大数据;"
			"2.创建索引再插入另外一些大数据;"
			"3.插入另外一些大数值并回滚;"
			"4.构建查询查检结果是否正确");

	SAVE_INFO_FOR_DEBUG();
	//创建表并插入一些大数据
	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	std::vector<std::string> vInserted;
	std::set<std::string> sDesiredResult;
	EntrySetID eId,idxId;
	boost::tie(eId,idxId) = PrepareDBAndIndex(sDesiredResult,vInserted);
    try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eId);

		//插入另外一些大数据
		std::string strData1,strData2;
		strData1 += "101784";
		strData2 += "182329";
		RandomGenString(strData1,1<<18);
		RandomGenString(strData2,1<<9);

		std::vector<std::string> vOtherData;
		vOtherData += strData1,strData2;
		InsertData(pTransaction,pSet,vOtherData);

		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->abort();
	}
	CATCHEXCEPTION

	try
	{
		//打开表和index
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eId);
		IndexEntrySet *pIndex = (IndexEntrySet*)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,idxId);


		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		//检查原子性
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,searchCondition.Keys());

		std::set<std::string> sResult(vResult.begin(),vResult.end());
		CHECK_BOOL(sResult == sDesiredResult);

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION
	return true;
}

bool test_toastIndex_UpdateRollback_DLL(void)
{
	INTENT("1.创建heap并插入一些大数据;\n"
		   "2.创建索引再插入另外一些大数据;\n"
		   "3.更新heap中的值并回滚;\n"
		   "4. 构建查询查检结果是否正确\n");
	SAVE_INFO_FOR_DEBUG();
	//创建表并插入一些大数据
	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	std::vector<std::string> vInserted;
	std::set<std::string> sDesiredResult;
	std::vector<EntryID> vIds;
	EntrySetID eId,idxId;
	boost::tie(eId,idxId) = PrepareDBAndIndex(sDesiredResult,vInserted,&vIds);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eId);

		//更新其中的一些数据
		std::string strData1,strData2;
		strData1 += "101784";
		strData2 += "182329";
		RandomGenString(strData1,1<<18);
		RandomGenString(strData2,1<<9);

		std::map<EntryID,std::string> updateData;
		insert(updateData)(vIds[1],strData1)(vIds[3],strData2);
		UpdateData(pTransaction,pSet,updateData);

		//回滚
		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->abort();
	}
	CATCHEXCEPTION

	try
	{
		//打开表和index
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eId);
		IndexEntrySet *pIndex = (IndexEntrySet*)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,idxId);


		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		//检查原子性
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,searchCondition.Keys());

		std::set<std::string> sResult(vResult.begin(),vResult.end());
		CHECK_BOOL(sResult == sDesiredResult);

		//回滚
		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION
	return true;
}

bool test_toastIndex_DeleteRollback_DLL(void)
{
	INTENT("1.创建heap并插入一些大数据;"
		"2.创建索引再插入另外一些大数据;"
		"3.删除heap中的一些值并回滚;"
		"4. 构建查询查检结果是否正确");
	SAVE_INFO_FOR_DEBUG();
	//创建表并插入一些大数据
	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	std::vector<std::string> vInserted;
	std::set<std::string> sDesiredResult;
	std::vector<EntryID> vIds;
	EntrySetID eId,idxId;
	boost::tie(eId,idxId) = PrepareDBAndIndex(sDesiredResult,vInserted,&vIds);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eId);

		//更新其中的一些数据
		std::vector<EntryID> vDeleteId;
        vDeleteId += vIds[1],vIds[4];
		DeleteData(pTransaction,pSet,vDeleteId);

		//回滚
		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->abort();
	}
	CATCHEXCEPTION

	try
	{
		//打开表和index
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eId);
		IndexEntrySet *pIndex = (IndexEntrySet*)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,idxId);


		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		//检查原子性
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,searchCondition.Keys());

		std::set<std::string> sResult(vResult.begin(),vResult.end());
		CHECK_BOOL(sResult == sDesiredResult);

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION
	return true;
}

