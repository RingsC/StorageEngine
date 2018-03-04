/**
* @file test_toast_index_dll.cpp
* @brief 
* @author ������
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
* �������һЩ���ݲ����أ�ͬʱ����ǰ3��С��"123",�ҽ��������ַ�����45������ֵ
* Detailed description.
* @param[out] vDesiredResults 
**************************************************************************/
static void GenerateInsertData(std::vector<std::string>& vInserted
							   ,std::vector<std::string>& vDesiredResults)
{
	using namespace boost::assign;
	//�������Բ��������
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
* ������Ҫɾ���������id��ɾ���Ժ�Ľ��
* Detailed description.
* @param[in/out] vAfterUpdate �������������ݵ�ȫ��������ɾ���Ժ�Ľ��
* @param[in/out] vIds �����������������id,������Ҫɾ�����ݵ�id
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
	INTENT("1. �����ݿ��в���һЩ����\n"
		"2. ����һ����ѯ;\n"
		"3. ���ʵ�ʽ��������ֵ�Ƿ����\n");
	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetMultiCollInfo());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//����һЩ����
		std::vector<std::string> vInsertData;
		std::vector<std::string> vDesiredData;
		GenerateInsertData(vInsertData,vDesiredData);
		InsertData(pTransaction,pEntrySet,vInsertData);

		//����index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetMultiCollInfo());
		IndexEntrySet *pIndex  = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

		//����һЩ����������
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


		//�����
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
	INTENT("1.����EntrySet,����һЩ������;\n"
		"2. ��������"
		"2.��ѯ���º�Ľ���Ƿ���ȷ.\n");
	try
	{
		typedef EntrySetCollInfo<3,2,1> EntrySetT;
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//����һЩ����
		std::vector<std::string> vInsertData;
		std::vector<std::string> vDesiredResult;
		std::vector<EntryID> vIds;
		GenerateInsertData(vInsertData,vDesiredResult);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//����index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,IndexCollInfo<EntrySetT,1,2>::get());
		IndexEntrySet *pIndex  = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

		//��������
		std::map<EntryID,std::string> mapUpdateData;
		GenerateUpdateData(mapUpdateData,vInsertData,vIds,vDesiredResult);
		UpdateData(pTransaction,pEntrySet,mapUpdateData);

		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,searchCondition.Keys());


		//�����
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
	INTENT("1.����EntrySet,����һЩ������;\n"
		"2. ɾ������"
		"2.��ѯɾ����Ľ���Ƿ���ȷ.\n");
	try
	{
		typedef EntrySetCollInfo<3,2,1> EntrySetT;
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//����һЩ����
		std::vector<std::string> vInsertData;
		std::vector<std::string> vDesiredResult;
		std::vector<EntryID> vIds;
		GenerateInsertData(vInsertData,vDesiredResult);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//����index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,IndexCollInfo<EntrySetT,1,2>::get());
		IndexEntrySet * pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);


		//ɾ������
		GenerateDeleteData(vInsertData,vIds,vDesiredResult);  //vInsertData���ڳ���ɾ����Ľ����vIds����Ҫɾ�����������ids.
		DeleteData(pTransaction,pEntrySet,vIds);

		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vvResult;
		GetDataFromEntrySet(vvResult,pTransaction,pEntrySet);
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,pTransaction,pIndex,searchCondition.Keys());


		//�����
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
	INTENT("1.����EntrySet,����һЩ������;"
		"2.�����������ϴ���IndexEntrySet;"
		"3.��������һЩ������;"
		"4.����һ����ѯ��(���һ�������)"
		"5.����ѯ����Ƿ񰴵ڶ���������.");
	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetMultiCollInfo());
		EntrySet* pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//����һЩ����
		std::vector<std::string> vInsertData;
		std::vector<std::string> vDesired;
		std::vector<EntryID> vIds;
		GenerateInsertData(vInsertData,vDesired);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);
		vDesired.push_back(vInsertData[3]);

		//����index
		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,GetMultiCollInfo());
		IndexEntrySet *pIndex = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);

        //����һЩ����������
		std::string strData1("012472"),strData2("033410");
		RandomGenString(strData1,1<<13);
		RandomGenString(strData2,1<<15);
		vDesired.push_back(strData1);

		std::vector<std::string> v;
		v += strData1,strData2;
		InsertData(pTransaction,pEntrySet,v,&vIds);

		//����һ����ѯ
		SearchCondition scanCondition;
		scanCondition.Add(1,Equal,"012",str_compare);

		//������Ƿ��ǰ��յڶ��ؽ�������
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
