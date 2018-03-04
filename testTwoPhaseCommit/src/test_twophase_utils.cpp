#include "test_twophase_utils.h"
#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <time.h>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/preprocessor/repeat.hpp>
#include "attr_setter.h"
#include "boost/timer.hpp"
#include "StorageEngineException.h"
#include "StorageEngine.h"
//#include "Transaction.h"
#include "MemoryContext.h"
#include "boost/thread.hpp"
#include "boost/format.hpp"


using namespace FounderXDB::StorageEngineNS;

int start_engine_()
{
	extern std::string g_strDataDir;
	StorageEngine::getStorageEngine()->initialize(const_cast<char*>(g_strDataDir.c_str()), 80, get_param());
	return 1;
}

int stop_engine_()
{
	StorageEngine::getStorageEngine()->shutdown();
	return 1;
}


extern std::map<std::string,std::string> ProgramOpts;
storage_params *get_param( void )
{
	storage_params* pRet = NULL;
	if (ProgramOpts.find("-conf") != ProgramOpts.end())
	{
		return GetStorageParam(ProgramOpts["-conf"]);
	}
	else if (ProgramOpts.find("-c") != ProgramOpts.end())
	{
		if ("archive" == ProgramOpts["-c"])
		{
			static storage_params para;
			para.doRecovery = false;
			para.XLogArchiveMode = true;	
			//para.XLogArchiveCommand = "python copy.py \"%p\" \"../archive/%f\"";
			para.XLogArchivePath = "../archive";
			pRet = &para;
		}
		else if ("recovery" == ProgramOpts["-c"])
		{
			static storage_params para;
			para.doRecovery = true;
			//para.recoveryRestoreCommand = "python copy.py  \"../archive/%f\" \"%p\"";
			para.recoveryRestorePath = "../archive";
			pRet = &para;
		}
	}
	return pRet;
}
int str_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
	int i = 0;
	while(i < len1 && i < len2)
	{
		if(str1[i] < str2[i])
			return -1;
		else if(str1[i] > str2[i])
			return 1;
		else i++;

	}
	if(len1 == len2)
		return 0;
	if(len1 > len2)
		return 1;
	return -1;
}

FXGlobalTransactionId GetGlobalTxid()
{
	static FXGlobalTransactionId gxid = 1;
	return ++gxid;
}

bool TwophaseDropEntrySet(const EntrySetID& entrySetId)
{
	bool bRet = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		pSE->removeEntrySet(pTrans,entrySetId);

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}

	return bRet;
}
