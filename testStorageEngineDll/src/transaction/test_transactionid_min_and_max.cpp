#include <iostream>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "transaction/test_transactionid_min_and_max.h"

using namespace std;
using namespace FounderXDB::StorageEngineNS;

extern StorageEngine* pStorageEngine;

bool test_transactionid_max()
{
	Transaction* pTransaction = NULL;
	try
	{
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		pTransaction = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);//获得一个事务
		
		FXTransactionId MaxTranId = pTransaction->getTransactionId();
		FXTransactionId MaxGetTranID = pStorageEngine->getMaxTransactionId();

		if (MaxTranId != MaxGetTranID)
		{
			delete pTransaction;
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << endl;
		if (pTransaction != NULL)
		{
			pTransaction->abort();
			delete pTransaction;
		}
		return false;
	}
	return true;
}

bool test_transactionid_max_second()
{
	Transaction* pTransaction = NULL;
	try
	{
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		pTransaction = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);//获得一个事务

		if (NULL == pTransaction)
			return false;

		FXTransactionId MaxTranId = pTransaction->getTransactionId();
		delete pTransaction;
		
		FXTransactionId MaxId = pStorageEngine->getMaxTransactionId();
		FXTransactionId MaxTId = 3;
			
		for (FXTransactionId i=1; i<MaxTranId; i++)
		{
			pTransaction = pStorageEngine->getTransaction(i,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			if (pTransaction != NULL)
			{
				if (pTransaction->getTransactionId() > MaxTId)
					MaxTId = pTransaction->getTransactionId();
			}
		}
		if (MaxTId != MaxId)
			return false;
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << endl;
		return false;
	}
	return true;
}

bool test_transactionid_min()
{
	Transaction* pTransaction = NULL;
	try
	{
		FXTransactionId MaxTranId = pStorageEngine->getMaxTransactionId();
		FXTransactionId MinTranID = pStorageEngine->getMinTransactionId();
		FXTransactionId MinId = MaxTranId;
		for (FXTransactionId i=1; i<MaxTranId; i++)
		{
			pTransaction = pStorageEngine->getTransaction(i,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			if (pTransaction != NULL)
			{
				if (pTransaction->getTransactionId() < MinId)
					MinId = pTransaction->getTransactionId();
			}
		}
		if (MinTranID != MinId)
			return false;
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << endl;
		return false;
	}
	return true;
}


bool test_transactionid_next()
{
	Transaction* pTransaction = NULL;
	try
	{
		FXTransactionId NextTranId = pStorageEngine->getNextTransactionId();
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		pTransaction = pStorageEngine->getTransaction(invalid_transaction,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		if (NULL == pTransaction)
			return false;

		FXTransactionId CurrTranId = pTransaction->getTransactionId();
		if (NextTranId == CurrTranId)
			return true;
		else
			return false;
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << endl;
		return false;
	}
	return true;
}

bool test_transaction_del()
{
	Transaction* pTransaction = NULL;
	Transaction* pTranSe = NULL;
	try
	{
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		pTransaction = pStorageEngine->getTransaction(invalid_transaction,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		FXTransactionId validTranId = pTransaction->getTransactionId();
		pStorageEngine->deleteTransaction(validTranId);
		pTranSe = pStorageEngine->getTransaction(validTranId, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << endl;
		return true;
	}
	return false;
}

