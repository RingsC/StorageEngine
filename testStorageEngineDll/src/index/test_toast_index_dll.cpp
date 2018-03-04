/**
* @file test_toast_index_dll.cpp
* @brief 
* @author 李书淦
* @date 2011-9-23 17:34:00
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <boost/assign.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "index/test_toast_index_dll.h"
extern Transaction *pTransaction;
using namespace std;
using namespace FounderXDB::StorageEngineNS;
/************************************************************************** 
* @brief GenerateInsertData 
* 随机产生一些数据并返回，同时返回前3个小于"123",且接着两个字符大于45的所有值
* Detailed description.
* @param[out] vDesiredResults 
**************************************************************************/
static void GenerateInsertData(std::vector<std::string>& vInserted
							   ,std::vector<std::string>& vDesiredResults)
{
	using namespace boost::assign;
	//生成用以插入的数据
	std::string strInsertData1;
	std::string strInsertData2;
	std::string strInsertData3;
	std::string strInsertData4;
	std::string strInsertData5;

	strInsertData1 += "123456";
	strInsertData2 += "123789";
	strInsertData3 += "012562";
	strInsertData4 += "012120";
	strInsertData5 += "012856";

	RandomGenString(strInsertData1,1<<20);
	RandomGenString(strInsertData2,1<<8);
	RandomGenString(strInsertData3,1<<24);
	RandomGenString(strInsertData4,1<<17);
	RandomGenString(strInsertData5,1<<19);

	vInserted += strInsertData1,strInsertData2,strInsertData3,strInsertData4,strInsertData5;
	vDesiredResults += strInsertData3,strInsertData5;

}

static void GenerateUpdateData(std::map<EntryID,std::string>& mapUpdateData
							   ,vector<std::string>& vAfterUpdate
							   ,const std::vector<EntryID>& vIds
							   ,vector<std::string>& vDesiredResult)
{
	using namespace boost::assign;
	std::string strUpdateData1,strUpdateData2,strUpdateData3;

	strUpdateData1 += "102830";
	strUpdateData2 += "125239";
	strUpdateData3 += "138735";

	RandomGenString(strUpdateData1,1<<9);   //strInsertData1 update to strUpdateData1
	RandomGenString(strUpdateData2,1<<25);  //strInsertData2 update to strUpdateData2
	RandomGenString(strUpdateData3,1<<13);  //strInsertData4 update to strUpdateData3

	vAfterUpdate[0].clear();
	vAfterUpdate[1].clear();
	vAfterUpdate[3].clear();

	vAfterUpdate[0] += strUpdateData1;
	vAfterUpdate[1] += strUpdateData2;
	vAfterUpdate[3] += strUpdateData3;  


	insert(mapUpdateData)(vIds[0],strUpdateData1)(vIds[1],strUpdateData2)(vIds[3],strUpdateData3);
	vDesiredResult += strUpdateData1;
}


/************************************************************************** 
* @brief GenerateDeleteData 
* 返回需要删除数据项的id和删除以后的结果
* Detailed description.
* @param[in/out] vAfterUpdate 输入是所有数据的全集，返回删除以后的结果
* @param[in/out] vIds 输入是所有数据项的id,返回需要删除数据的id
**************************************************************************/
static void GenerateDeleteData(vector<std::string>& vAfterDelete
							   ,std::vector<EntryID>& vIds
							   ,std::vector<std::string>& /*vDesiredResult*/)
{
    vIds.erase(vIds.begin() + 4);
	vIds.erase(vIds.begin() + 2);

	vAfterDelete.erase(vAfterDelete.begin()) + 3;	
	vAfterDelete.erase(vAfterDelete.begin() + 1);
	vAfterDelete.erase(vAfterDelete.begin());
}


bool test_toast_index_insert(void)
{
	INTENT("1. 向数据库中插入一些数据\n"
		"2. 构造一个查询;\n"
		"3. 检查实际结果与期望值是否相等\n");
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
		std::vector<std::string> vDesiredData;
		GenerateInsertData(vInsertData,vDesiredData);
		InsertData(pTransaction,pEntrySet,vInsertData);

		//创建index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetMultiCollInfo());
		IndexEntrySet *pIndex  = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

		//插入一些其他的数据
		std::string strData1("100518");
		std::string strData2("234882");
		RandomGenString(strData1,1<<14);
		RandomGenString(strData2,1<<12);
		std::vector<std::string> v;
		v += strData1,strData2;
		InsertData(pTransaction,pEntrySet,v);
		vDesiredData.push_back(strData1);



		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,searchCondition.Keys());


		//检查结果
		std::set<std::string> sDesired(vDesiredData.begin(),vDesiredData.end()),sResult(vResult.begin(),vResult.end());
		CHECK_BOOL(sDesired == sResult);

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

	}
	CATCHEXCEPTION

	return true;
}

bool test_toast_index_update(void)
{
	INTENT("1.创建EntrySet,插入一些大数据;\n"
		"2. 更新数据"
		"2.查询更新后的结果是否正确.\n");
	try
	{
		typedef EntrySetCollInfo<3,2,1> EntrySetT;
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		std::vector<std::string> vInsertData;
		std::vector<std::string> vDesiredResult;
		std::vector<EntryID> vIds;
		GenerateInsertData(vInsertData,vDesiredResult);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//创建index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,IndexCollInfo<EntrySetT,1,2>::get());
		IndexEntrySet *pIndex  = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

		//更新数据
		std::map<EntryID,std::string> mapUpdateData;
		GenerateUpdateData(mapUpdateData,vInsertData,vIds,vDesiredResult);
		UpdateData(pTransaction,pEntrySet,mapUpdateData);

		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,searchCondition.Keys());


		//检查结果
		std::set<std::string> sDesired(vDesiredResult.begin(),vDesiredResult.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		CHECK_BOOL(sDesired == sResult);

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

	}
	CATCHEXCEPTION
	return true;

}

bool test_toast_index_delete(void)
{
	INTENT("1.创建EntrySet,插入一些大数据;\n"
		"2. 删除数据"
		"2.查询删除后的结果是否正确.\n");
	try
	{
		typedef EntrySetCollInfo<3,2,1> EntrySetT;
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		std::vector<std::string> vInsertData;
		std::vector<std::string> vDesiredResult;
		std::vector<EntryID> vIds;
		GenerateInsertData(vInsertData,vDesiredResult);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//创建index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,IndexCollInfo<EntrySetT,1,2>::get());
		IndexEntrySet * pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);


		//删除数据
		GenerateDeleteData(vInsertData,vIds,vDesiredResult);  //vInsertData现在成了删除后的结果，vIds是需要删除的数据项的ids.
		DeleteData(pTransaction,pEntrySet,vIds);

		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vvResult;
		GetDataFromEntrySet(vvResult,pTransaction,pEntrySet);
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,searchCondition.Keys());


		//检查结果
		std::set<std::string> sDesired(vDesiredResult.begin(),vDesiredResult.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		CHECK_BOOL(sDesired == sResult);


		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	return true;
}

bool test_toast_index_order(void)
{
	INTENT("1.创建EntrySet,插入一些大数据;"
		"2.在其中两列上创建IndexEntrySet;"
		"3.插入其他一些大数据;"
		"4.构造一个查询：(与第一个键相等)"
		"5.检查查询结果是否按第二个键排序.");
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
		std::vector<std::string> vDesired;
		std::vector<EntryID> vIds;
		GenerateInsertData(vInsertData,vDesired);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);
		vDesired.push_back(vInsertData[3]);

		//创建index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetMultiCollInfo());
		IndexEntrySet *pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

        //插入一些其他的数据
		std::string strData1("012472"),strData2("033410");
		RandomGenString(strData1,1<<13);
		RandomGenString(strData2,1<<15);
		vDesired.push_back(strData1);

		std::vector<std::string> v;
		v += strData1,strData2;
		InsertData(pTransaction,pEntrySet,v,&vIds);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,Equal,"012",str_compare);

		//检查结果是否是按照第二关健字排序
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,scanCondition.Keys());

		std::sort(vDesired.begin(),vDesired.end());
		CHECK_BOOL(CompareVector(vResult, vDesired));

		pStorageEngine->closeEntrySet(pTransaction,pIndex);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION
	return true;
}
