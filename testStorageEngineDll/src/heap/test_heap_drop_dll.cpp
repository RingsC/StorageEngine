#include <iostream>

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "heap/test_heap_drop_dll.h"
#include "test_fram.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include <boost/thread.hpp>

using std::cout;
using std::endl;
using std::string;
using namespace FounderXDB::StorageEngineNS;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
extern EntrySetID EID;

bool test_heap_create_drop_dll()
{
	INTENT("����̲��ԣ�������������������ȴ�����");
	try
	{
		createTable();
	}
	CATCHEXCEPTION;
	return true;
}

bool test_heap_drop_dll()
{
	INTENT("����̲��ԣ������������������ɾ����");
	try {
		get_new_transaction();		
		pStorageEngine->removeEntrySet(pTransaction,EID);//ɾ��heap
		commit_transaction();
		} 
	CATCHEXCEPTION;
	return true;
}

void thread_func_get_current_transaction(bool& bFlag)
{
	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();

	pStorageEngine->beginThread();

	TransactionId tid = InvalidTransactionID;
	Transaction* pTrans = pStorageEngine->getTransaction(tid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	Transaction* pCurTrans = pStorageEngine->GetCurrentTransaction();

	if (pTrans != pCurTrans)
		bFlag = false;
	
	std::cout<<pCurTrans<<std::endl;
	pTrans->commit();

	pStorageEngine->endThread();
}

bool bThread1 = false;
bool bThread2 = false;
void current_transaction_thread1(bool& bFlag)
{
	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();

	pStorageEngine->beginThread();

	if (NULL != pStorageEngine->GetCurrentTransaction())
		bFlag = false;
	
	TransactionId tid = InvalidTransactionID;
	Transaction* pTrans = pStorageEngine->getTransaction(tid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	Transaction* pCurTrans = pStorageEngine->GetCurrentTransaction();

	if (pTrans != pCurTrans)
		bFlag = false;

	std::cout<<pCurTrans<<std::endl;
	pTrans->commit();

	//thread2 wait for this flag
	bThread1 = true;

	//wait for thread2
	while (!bThread2)

	pStorageEngine->endThread();
}
void current_transaction_thread2(bool& bFlag)
{
	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();

	pStorageEngine->beginThread();

	//wait for thread1
	while(!bThread1)

	if (NULL != pStorageEngine->GetCurrentTransaction())
		bFlag = false;

	TransactionId tid = InvalidTransactionID;
	Transaction* pTrans = pStorageEngine->getTransaction(tid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	Transaction* pCurTrans = pStorageEngine->GetCurrentTransaction();

	if (pTrans != pCurTrans)
		bFlag = false;

	std::cout<<pCurTrans<<std::endl;
	pTrans->commit();

	//thread1 wait for this flag to exit
	bThread2 = true;

	pStorageEngine->endThread();
}
bool test_get_current_transaction()
{
	bool bRet = true;
	StorageEngine* pStorageEngine = NULL;

	try
	{
		//ʮ���߳�ͬʱ�������쳣
		bool arrFlag[10] = {true};

		boost::thread_group g;
		for (uint32 i = 0; i < 10; i ++)
		{
			g.create_thread(boost::bind(thread_func_get_current_transaction,arrFlag[i]));
		}
		g.join_all();

		//thread1��Ӱ��thread2��current_transactonָ��ֵ
		bool bFlag1 = true;
		bool bFlag2 = true;

		g.create_thread(boost::bind(current_transaction_thread1,bFlag1));
		g.create_thread(boost::bind(current_transaction_thread2, bFlag2));
		g.join_all();

		for (uint32 i = 0; i < 10; i ++)
		{
			if (!arrFlag[i])
			{
				bRet = false;
				break;
			}
		}

		bRet = bFlag1&&bFlag2;
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		return false;
	}

	return bRet;
}