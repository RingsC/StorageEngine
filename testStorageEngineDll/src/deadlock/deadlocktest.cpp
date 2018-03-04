#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <boost/assign.hpp>
#include <boost/thread.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "deadlock/deadlocktest.h"
using namespace std;
EntrySetID relid1 = 0;
EntrySetID relid2 = 0;


bool wakeUp1 = false;
bool canWakeup1 = false;
bool hardDeadLockDected = false;

extern StorageEngine* pStorageEngine;
void Thread1( void )
{
	Transaction *pTransaction = NULL;
	try
	{
		pStorageEngine->beginThread();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,relid2);

		canWakeup1 = true;
		while (!wakeUp1)
		{
			MySleep(100L);
		}
        pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,relid1);
		pTransaction->commit();
	}
	catch(DeadLockException &ex)
	{
		hardDeadLockDected = true;
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		pTransaction->abort();
	}

	pStorageEngine->endThread();
}

void Thread2( void )
{
	Transaction *pTransaction = NULL;
	try
	{
		pStorageEngine->beginThread();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,relid1);

		while (!canWakeup1)
		{
			MySleep(100L);
		}
		wakeUp1 = true;

		pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,relid2);
		
		pTransaction->commit();
	}
	catch(DeadLockException &ex)
	{
		hardDeadLockDected = true;
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		pTransaction->abort();
	}
	pStorageEngine->endThread();
}

bool DeadLockTest( void )
{
	INTENT("1. 创建两个表:1和2;\n"
		"2. 启动线程1： 打开A->sleep->打开B;\n"
		"3. 启动线程2:  打开B->wakup 线程1->打开A;\n");

	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		Transaction *pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		relid1 = pStorageEngine->createEntrySet(pTransaction,GetSingleColInfo());
		relid2 = pStorageEngine->createEntrySet(pTransaction,GetSingleColInfo());
		pTransaction->commit();

		boost::thread_group thgrp;
		thgrp.create_thread(Thread1);
		thgrp.create_thread(Thread2);

		thgrp.join_all();

		CHECK_BOOL(hardDeadLockDected);

		xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pStorageEngine->removeEntrySet(pTransaction,relid1);
		pStorageEngine->removeEntrySet(pTransaction,relid2);
		pTransaction->commit();

		return hardDeadLockDected;
	}
	CATCHEXCEPTION

	return false;
}
