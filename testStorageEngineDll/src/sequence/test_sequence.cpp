#include <utility>
#include <iostream>
#include <vector>
#include <boost/assign.hpp>
#include "boost/thread.hpp" 
#include "PGSETypes.h"
#include "StorageEngine.h"
#include "Transaction.h"
#include "XdbSequence.h"
#include "sequence/utils.h"
#include "utils/utils_dll.h"
#include "test_fram.h"


using std::cout;
using std::endl;

using namespace FounderXDB::StorageEngineNS;

bool testseq_DeleteSequence(SeqID seqId)
{
	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	FXTransactionId xid = 0;
	Transaction *pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	try
	{
		SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();
		seqFactory->deleteSequence(seqId);
		pTrans->commit();
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}

	return true;
}

bool testseq_DeleteSequence(const char *seqName)
{
	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	FXTransactionId xid = 0;
	Transaction *pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	try
	{
		SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();
		seqFactory->deleteSequence(seqName);
		pTrans->commit();
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}

	return true;
}

bool testseq_CreateSequence(const char *seqName, xdb_seq_t initValue, uint32 flags,
	bool hasRange, xdb_seq_t minValue, xdb_seq_t maxValue, SeqID& seqId)
{
	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	FXTransactionId xid = 0;
	Transaction *pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	try
	{
		SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();

		if(hasRange)
		{
			std::pair<xdb_seq_t, xdb_seq_t> range = std::make_pair(minValue, maxValue);
			seqId = seqFactory->createSequence(initValue, range, seqName,flags);
		}
		else
		{
			seqId = seqFactory->createSequence(initValue, seqName,flags);
		}

		pTrans->commit();
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}

	return true;
}

bool testseq_CheckSequenceInfo(std::string& expSeqName, sequenceInfo& expSeqInfo, bool openByName)
{
	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	FXTransactionId xid = 0;
	Transaction *pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	std::string seqName;
	sequenceInfo seqInfo;

	try
	{
		SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();
		XdbSequence *sequence;
		if(openByName){
			sequence = seqFactory->openSequence(expSeqName.c_str());
		}else{
			sequence = seqFactory->openSequence(expSeqInfo.seqId);
		}
		seqInfo = sequence->getSequenceInfo();
		seqName = std::string(sequence->getName());
		pTrans->commit();
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}

	if((expSeqInfo.range.first == 0) && (expSeqInfo.range.second == 0))
	{
		seqInfo.range = std::make_pair(0, 0);
	}

	return ((seqName == expSeqName)
		&& (seqInfo.value == expSeqInfo.value)
		&& (seqInfo.seqId == expSeqInfo.seqId)
		&& (seqInfo.flags == expSeqInfo.flags)
		&& (seqInfo.range == expSeqInfo.range));
}

bool __testseq_SeqFactoryMethod(const char *seqName, xdb_seq_t initValue, uint32 flags,
	bool hasRange, xdb_seq_t minValue, xdb_seq_t maxValue, bool deleteByName, bool openByName)
{
	sequenceInfo seqInfo;
	std::string stringSeqName;
	SeqID seqId;

	if(!testseq_CreateSequence(seqName, initValue, flags, hasRange, minValue, maxValue, seqId))
	{
		cout << "create sequence failed" << endl;
		return false;
	}

	if(seqName == NULL)
	{
		char cName[32];
		snprintf(cName, 32, "%d", seqId);
		stringSeqName = std::string(cName);
	}
	else
	{
		stringSeqName = std::string(seqName);
	}

	seqInfo.seqId = seqId;
	seqInfo.value = initValue;
	seqInfo.flags = flags;
	if(hasRange)
	{
		seqInfo.range = std::make_pair(minValue, maxValue);
	}

	bool ret = true;
	if(!testseq_CheckSequenceInfo(stringSeqName, seqInfo, openByName))
	{
		ret = false;
		cout << "sequence info not match" << endl;
	}

	if(deleteByName)
	{
		ret = testseq_DeleteSequence(stringSeqName.c_str());
	}
	else
	{
		ret = testseq_DeleteSequence(seqId);
	}

	return ret;
}

bool testseq_SeqFactoryMethod()
{
	return (__testseq_SeqFactoryMethod(NULL, 1, SequenceFactory::INCREMENTAL, false, 0, 0, true, true)
			&& __testseq_SeqFactoryMethod(NULL, 1, SequenceFactory::INCREMENTAL, false, 0, 0, false, false)
			&& __testseq_SeqFactoryMethod("testSeq1", 12, SequenceFactory::DECREMENTAL, false, 0, 0, true, true)
			&& __testseq_SeqFactoryMethod("testSeq2", 22, SequenceFactory::DECREMENTAL, false, 0, 0, false, false)
			&& __testseq_SeqFactoryMethod("testSeq3", 23, SequenceFactory::ALLOW_WRAP, true, 5, 100, true, true)
			&& __testseq_SeqFactoryMethod("testSeq4", 23, SequenceFactory::ALLOW_WRAP, true, 5, 100, false, false));
}

bool testseq_SeqMethod()
{
	bool ret = true;

	xdb_seq_t initValue = 10;
	xdb_seq_t currValue = initValue;
	std::pair<xdb_seq_t, xdb_seq_t> range = std::make_pair(50, 100);
	uint32 flags = SequenceFactory::INCREMENTAL;
	const char *seqName = "testseq";
	SeqID seqId;
	if(!testseq_CreateSequence(seqName, initValue, flags, true, range.first, range.second, seqId))
	{
		cout << "create sequence failed." << endl;
		return false;
	}

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	FXTransactionId xid = 0;
	Transaction *pTrans = NULL;

	/* check read functions */
	xid = 0;
	pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();
		XdbSequence *sequence = seqFactory->openSequence(seqId);

		bool ret = (sequence->getRange() == range) && (sequence->getId() == seqId)
			&& (sequence->getFlags() == flags) && (strcmp(seqName, sequence->getName()) == 0);
		pTrans->commit();
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}

	if(!ret)
	{
		cout << "errors in read functions" << endl;
		return false;
	}

	/* check getValue function */
	xid = 0;
	pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();
		XdbSequence *sequence = seqFactory->openSequence(seqId);

		int32 delta = 50;
		xdb_seq_t oldValue = sequence->getValue(50);
		xdb_seq_t newValue = sequence->getSequenceInfo().value;
		if((currValue != oldValue) || (newValue != oldValue + delta))
		{
			cout << "getValue failed." << endl;
			ret = false;
		}
		pTrans->commit();

		currValue = newValue; /* for last case */
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	if(!ret)
	{
		cout << "errors in getValue functions" << endl;
		return false;
	}

	/* check reset function */
	xid = 0;
	pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();
		XdbSequence *sequence = seqFactory->openSequence(seqId);

		int32 newInitValue = 80;
		sequence->reset(newInitValue);
		xdb_seq_t newValue = sequence->getSequenceInfo().value;
		if(newValue != newInitValue)
		{
			cout << "reset failed." << endl;
			ret = false;
		}

		currValue = newInitValue; /* for last case */
		pTrans->commit();
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}

	if(!ret)
	{
		cout << "errors in reset functions" << endl;
		return false;
	}

	return testseq_DeleteSequence(seqId);
}

/* concurrency test */
void testseq_CreateSeqThread(SeqID *pSeqId, uint32 sleepMs, bool *pRet)
{
	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	pStorageEngine->beginThread();

	FXTransactionId xid = 0;
	Transaction *pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	try
	{
		SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();
		*pSeqId = seqFactory->createSequence(1);

		if(sleepMs != 0)
		{
			boost::thread::sleep(boost::get_system_time()
				+ boost::posix_time::milliseconds(sleepMs));
		}

		pTrans->commit();
		*pRet = true;
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		*pRet = false;
	}

	pStorageEngine->endThread();
}

bool testseq_CreateSeqConcurrency()
{
	static const uint32 threadCount = 20;
	bool threadRet[threadCount];
	SeqID seqId[threadCount];

	boost::thread_group thgrp;
	for(uint32 i = 0; i < threadCount; i++)
	{
		thgrp.create_thread(boost::bind(&testseq_CreateSeqThread, &seqId[i], 200, &threadRet[i]));
	}
	thgrp.join_all();

	bool ret = true;
	std::set<xdb_seq_t> seqIdSet;
	for(uint32 i = 0; i < threadCount; i++)
	{
		cout << "ret[" << i << "]: " << threadRet[i]
			<< "seqId[" << i << "]: " << seqId[i] << endl;
		seqIdSet.insert(seqId[i]);
		ret = (ret && threadRet[i]);
	}

	std::set<xdb_seq_t>::iterator it = seqIdSet.begin();
	while(it != seqIdSet.end())
	{
		cout << *it << " ";
		++it;
	}
	cout << endl;

	return ret && (seqIdSet.size() == threadCount);
}

void testseq_GetValueThread(SeqID seqId, int32 delta, uint32 sleepMs,
	xdb_seq_t *pValue, bool *pRet)
{
	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	pStorageEngine->beginThread();

	/* check read functions */
	FXTransactionId xid = 0;
	Transaction *pTrans = NULL;
	pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();
		XdbSequence *sequence = seqFactory->openSequence(seqId);
		*pValue = sequence->getValue(delta);
		if(sleepMs != 0)
		{
			boost::thread::sleep(boost::get_system_time()
				+ boost::posix_time::milliseconds(sleepMs));
		}

		pTrans->commit();

		*pRet = true;
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		*pRet = false;
	}

	pStorageEngine->endThread();
}

bool testseq_GetValueConcurrency()
{
	SeqID seqId;

	xdb_seq_t initValue = 1;
	if(!testseq_CreateSequence(NULL, initValue, SequenceFactory::INCREMENTAL, false, 0, 0, seqId))
	{
		cout << "create sequence failed." << endl;
		return false;
	}

	cout << "seqId: " << seqId << endl;

	static const uint32 threadCount = 20;
	bool threadRet[threadCount];
	xdb_seq_t value[threadCount];
	int32 delta = 1;
	boost::thread_group thgrp;
	for(uint32 i = 0; i < threadCount; i++)
	{
		thgrp.create_thread(boost::bind(&testseq_GetValueThread, seqId, delta, 0, &value[i], &threadRet[i]));
	}
	thgrp.join_all();

	bool ret = true;
	std::set<xdb_seq_t> expectedValueSet;
	std::set<xdb_seq_t> valueSet;
	for(uint32 i = 0; i < threadCount; i++)
	{
		cout << "threadRet[" << i << "]: " << threadRet[i]
			<< ", value[" << i << "]: " << value[i] << endl;
		expectedValueSet.insert(initValue + i * delta);
		valueSet.insert(value[i]);
		ret = (ret && threadRet[i]);
	}

	return (ret && (valueSet == expectedValueSet));
}
