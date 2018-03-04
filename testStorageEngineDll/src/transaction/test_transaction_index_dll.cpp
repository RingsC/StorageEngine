/**
* @file test_transaction_index_dll.cpp
* @brief 
* @author ������
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

		//��������
		vInsertedData += "2","9","7","5";
        InsertData(pTransaction,pEntrySet,vInsertedData,vIds);

		//�ر�
        pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	return TableDroper::entryId;
}
bool test_trans_index_InsertRollback(void)
{
	INTENT("1. ����һ�������һЩ���ݺ��ύ;"
		   "2. ��������һ������;"
		   "3. ��������һЩ���ݺ�ع�;"
		   "4. ����һ��������ԭ����.");

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	//����һ�������һЩ���ݺ��ύ; 2 5 7 9
	std::vector<std::string> vInserted;
	EntrySetID eid = SetUp(vInserted);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		//����һЩ��������
		std::vector<std::string> vOtherData;
		vOtherData += "8","6","3";
		InsertData(pTransaction,pSet,vOtherData,NULL);

		//�ع�
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

		//���ԭ����
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
	INTENT("1. ����һ�������һЩ���ݺ��ύ;"
		"2. ��������һ������;"
		"3. �������е�һЩ���ݺ�ع�;"
		"4. ����һ��������ԭ����.");

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	//����һ�������һЩ���ݺ��ύ; 2 5 7 9
	std::vector<EntryID> vIds;
	std::vector<std::string> vInserted;
	EntrySetID eid = SetUp(vInserted,&vIds);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		//��������
		std::map<EntryID,std::string> updateData;
		insert(updateData)(vIds[1],"6")(vIds[2],"3"); //5->6 7->3
        UpdateData(pTransaction,pSet,updateData);

        //�ع�
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

		//���ԭ����
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
	INTENT("1. ����һ�������һЩ���ݺ��ύ;"
		"2. ��������һ������;"
		"3. ɾ�����е�һЩ���ݺ�ع�;"
		"4. ����һ��������ԭ����.");

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	//����һ�������һЩ���ݺ��ύ; 2 5 7 9
	std::vector<EntryID> vIds;
	std::vector<std::string> vInserted;
	EntrySetID eid = SetUp(vInserted,&vIds);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pSet = (EntrySet*)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,eid);

		//ɾ��һЩ����
		std::vector<EntryID> vDeleteIds;
		vDeleteIds += vIds[0],vIds[2],vIds[3];
		DeleteData(pTransaction,pSet,vDeleteIds);

		//�ع�
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

		//���ԭ����
		std::set<std::string> sDesired(vInserted.begin(),vInserted.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());

		CHECK_BOOL(sDesired == sResult);
		pStorageEngine->closeEntrySet(pTransaction,pSet);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	return true;
}