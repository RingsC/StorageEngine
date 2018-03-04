#include "backup_recovery/Recovery2Recently.h"
#include "backup_recovery/Recovery2Target.h"
#include "backup_recovery/BRTest.h"
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <boost/assign.hpp>
#include <boost/thread/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
using namespace std;
typedef EntrySetCollInfo<5> EntrySetT;
static const std::string STORAGE_DATA_DIR = "../";
extern Transaction* pTransaction;
extern StorageEngine* pStorageEngine;
extern std::map<std::string,std::string> ProgramOpts;
static const std::string strPharseFile = "../pharse1";

bool NeedRun1( void )
{
	if ((ProgramOpts.end() != ProgramOpts.find("-c") 
		&& ProgramOpts["-c"] == "recovery2recently"))
	{
		return true;
	}
	return false;
}

bool PrepareBasebackup( void )
{
	if (!NeedRun1())
	{
		return true;
	}

	PharseFile parseFile(strPharseFile);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = EntrySetCreator<EntrySetT>::createAndOpen(pTransaction,"test");

		StorageData stData(STORAGE_DATA_DIR + "timeR0",pEntrySet,pTransaction);
		stData.InsertData("12ipw");
		stData.InsertData("22ipn");
		stData.InsertData("w2ino");
		stData.InsertData("ppiew");
		stData.InsertData("iowpi");

		parseFile.SetEntrySetId(pEntrySet->getId());
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		stData.Save(STORAGE_DATA_DIR + "timeR0");
	}
	CATCHEXCEPTION

	try
	{
		StorageEngine::getStorageEngine()->startArchive(true);

		std::string strBackup = ProgramOpts["-d"];
		boost::thread_group thrGroup;
		StorageData stData(STORAGE_DATA_DIR + "timeR0");
		thrGroup.create_thread(boost::bind(BackUpThread,boost::ref(strBackup)));
		thrGroup.join_all();

		std::cout<<"base backup end!"<<std::endl;
		//StorageEngine::getStorageEngine()->endArchive(true);
		stData.Save(STORAGE_DATA_DIR + "timeR1");
		return true;
	}
	CATCHEXCEPTION
}

bool RecoveryFirst( void )
{
	if (!NeedRun1())
	{
		return true;
	}
	PharseFile parseFile(strPharseFile);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,parseFile.GetEntrySetID());

		StorageData stData(STORAGE_DATA_DIR + "timeR1",pEntrySet,pTransaction);
		bool bRet = stData.CheckIsSame();
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		stData.Save(STORAGE_DATA_DIR + "timeR2");
		return bRet;
	}
	CATCHEXCEPTION
}

bool MakeMoreChanges1( void )
{
	if (!NeedRun1())
	{
		return true;
	}
	PharseFile parseFile(strPharseFile);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,parseFile.GetEntrySetID());

		StorageData stData(STORAGE_DATA_DIR + "timeR2",pEntrySet,pTransaction);
		stData.InsertData("uuuw,");
		stData.InsertData("i8892");
		bool bRet = stData.CheckIsSame();
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		stData.Save(STORAGE_DATA_DIR + "timeR3");
		return true;
	}
	CATCHEXCEPTION
}

bool Recovery2Recently( void )
{
	if (!NeedRun1())
	{
		return true;
	}
	PharseFile parseFile(strPharseFile);
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,parseFile.GetEntrySetID());

		StorageData stData(STORAGE_DATA_DIR + "timeR3",pEntrySet,pTransaction);
		bool bRet = stData.CheckIsSame();
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		stData.Save(STORAGE_DATA_DIR + "timeR4");
		return bRet;
	}
	CATCHEXCEPTION
}