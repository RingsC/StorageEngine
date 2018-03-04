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
using namespace boost;
#define foreach BOOST_FOREACH
StorageData::StorageData(const std::string& strFile,EntrySet* pEntrySet,Transaction* pTrans)
:r_pEntrySet(pEntrySet)
,m_pTrans(pTrans)
{
    Load(strFile);
}

StorageData::~StorageData( void )
{

}

void StorageData::Load(std::string const & strFile)
{
	std::cout<<"load data from file"<<strFile.c_str()<<std::endl;
	std::ifstream inFile(strFile.c_str());
	if (inFile.is_open())
	{
		std::string strLine;
		getline(inFile,strLine);             //The first line is the time stamp
		std::cout<<"first line : "<<strLine.c_str()<<std::endl;
		getline(inFile,strLine);             //The second line is the timeLine
		std::cout<<"second line :"<<strLine.c_str()<<std::endl;
        while(getline(inFile,strLine))
		{
			std::cout<<strLine.c_str()<<std::endl;
            m_setDatas.insert(strLine);
			strLine.clear();
		}
	}
}

void StorageData::Save(std::string const & strFile)
{
	std::cout<<"save data to file "<<strFile.c_str()<<std::endl;
	std::fstream outFile(strFile.c_str(),std::ios_base::out);
	if(outFile.is_open())
	{
		char timestamp[128];

		//sleep 1 second
		MySleep(1000);
		time_t curTime = time(NULL);
		strftime(timestamp,sizeof(timestamp),
			"%Y-%m-%d %H:%M:%S %Z",
			localtime(&curTime));
		outFile<<timestamp<<std::endl;
		outFile<<StorageEngine::getStorageEngine()->getCurrentTimeLine()<<std::endl;
		std::cout<<"save data : "<<std::endl;
		foreach (const std::string& str,m_setDatas)
        {
			std::cout<<str.c_str()<<std::endl;
			outFile<<str<<std::endl;
        }
	}
}

void StorageData::InsertData(const std::string& strData,EntrySet* pEntrySet,Transaction* pTrans)
{
	boost::lock_guard<boost::mutex> lock(m_mutex);
	std::vector<std::string> vInsert;
	vInsert.push_back(strData);

	::InsertData(GetTransaction(pTrans),GetEntrySet(pEntrySet),vInsert);
	m_setDatas.insert(strData);
}

void StorageData::UpdateData(const std::string& strFrom,const std::string& strTo,EntrySet* pEntrySet,Transaction* pTrans)
{
    boost::lock_guard<boost::mutex> lock(m_mutex);
	std::vector<EntryID> vIds;
	std::vector<std::string> vFrom;
	std::map<EntryID,std::string> mapUpdate;
    vFrom.push_back(strFrom);
	::GetEntryId(GetTransaction(pTrans),GetEntrySet(pEntrySet),vFrom,vIds);
   
	if (0 == vIds.size())
	{
		std::cout<<"warning: can not find the data which need be updated!"<<std::endl;
	}

	mapUpdate[vIds[0]] = strTo;
	::UpdateData(GetTransaction(pTrans),GetEntrySet(pEntrySet),mapUpdate);

	if (0 == m_setDatas.count(strFrom))
	{
		throw std::logic_error("the entryset and storage data are not consistently!");
	}
	else
	{
		m_setDatas.erase(strFrom);
		m_setDatas.insert(strTo);
	}
}

void StorageData::DeleteData(const std::string& strDelete,EntrySet* pEntrySet,Transaction* pTrans)
{
    boost::lock_guard<boost::mutex> lock(m_mutex);
	std::vector<std::string> vDelete;
	std::vector<EntryID> vIds;
	vDelete.push_back(strDelete);
	::GetEntryId(GetTransaction(pTrans),GetEntrySet(pEntrySet),vDelete,vIds);
	if (vIds.size() > 0)
	{
		::DeleteData(GetTransaction(pTrans),GetEntrySet(pEntrySet),vIds);
		if (m_setDatas.count(strDelete) == 0)
		{
			throw std::logic_error("the entryset and storage data are not consistently!");
		}
		else
		{
			m_setDatas.erase(strDelete);
		}
	}
}

bool StorageData::CheckIsSame( void )
{
	std::vector<std::string> vResult;
    GetDataFromEntrySet(vResult,m_pTrans,r_pEntrySet);

	std::set<std::string> sResult(vResult.begin(),vResult.end());

	bool bRet = (sResult == m_setDatas);

	if (!bRet)
	{
		std::cout<<"EntrySet Data : "<<std::endl;
		std::set<std::string>::iterator it;
		for (it = sResult.begin(); it != sResult.end(); it ++)
		{
			std::cout<<*it<<std::endl;
		}

		std::cout<<"m_setDatas set record number : "<<m_setDatas.size()<<" ; record list : "<<std::endl;
		std::set<std::string>::iterator it_ex;
		for (it_ex = m_setDatas.begin(); it_ex != m_setDatas.end(); it_ex ++)
		{
			std::cout<<*it_ex<<"\\n"<<std::endl;
		}
	}
	

	return bRet;
}

/////////////////////////////////////////////////////////////////////////

PharseFile::PharseFile(const std::string& strFile)
:m_strPath(strFile)
,m_nPharse(0)
,m_idEntrySet(0)
{
	std::ifstream inFile(strFile.c_str());
	if (inFile.is_open())
	{
		inFile>>m_nPharse;
		m_nPharse %= TOTAL_PHARSE;
		inFile>>m_idEntrySet;
	}
}

PharseFile::~PharseFile( void )
{
    ++m_nPharse;
	std::fstream outFile(m_strPath.c_str(),std::ios_base::out);
	if (outFile.is_open())
	{
		outFile<<m_nPharse<<std::endl;
		outFile<<m_idEntrySet<<std::endl;
	}
}

typedef EntrySetCollInfo<5> EntrySetT;
static const std::string STORAGE_DATA_DIR = "../";
extern Transaction* pTransaction;
extern StorageEngine* pStorageEngine;
extern std::map<std::string,std::string> ProgramOpts;

bool CreateOnTable(EntrySetID& entryId)    //time1
{
    try
    {
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = EntrySetCreator<EntrySetT>::createAndOpen(pTransaction,"test");
        
		StorageData stData(STORAGE_DATA_DIR + "time1",pEntrySet,pTransaction);
		stData.InsertData("12ipw");
		stData.InsertData("22ipn");
		stData.InsertData("w2ino");
		stData.InsertData("ppiew");
		stData.InsertData("iowpi");

		entryId = pEntrySet->getId();
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);

		ShowEntrySetData(pTransaction,pEntrySet);

		pTransaction->commit();
		stData.Save(STORAGE_DATA_DIR + "time1");
		return true;
    }
    CATCHEXCEPTION
}

static void InsertThread(StorageData& stData,EntrySetID entryId)
{
	Transaction* pTrans = NULL;
	try
	{
		pStorageEngine->beginThread();
		TransactionId transId = 0;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		{
			EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,entryId);
			stData.InsertData("wi,wn",pEntrySet,pTrans);
			pStorageEngine->closeEntrySet(pTrans,pEntrySet);
		}
		pTrans->commit();
		pStorageEngine->endThread();
	}
	VOID_CATCHEXCEPTION(pTrans)
}

static void UpdateThread(StorageData& stData,EntrySetID entryId)
{
	Transaction* pTrans = NULL;
	try
	{
		pStorageEngine->beginThread();
		TransactionId transId = 0;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		{
			EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,entryId);
			stData.UpdateData("w2ino","wncuw",pEntrySet,pTrans);
			pStorageEngine->closeEntrySet(pTrans,pEntrySet);
		}
		pTrans->commit();
		pStorageEngine->endThread();
	}
	VOID_CATCHEXCEPTION(pTrans)
}

static void DeleteThread(StorageData& stData,EntrySetID entryId)
{
	Transaction* pTrans = NULL;
	try
	{
		pStorageEngine->beginThread();
		TransactionId transId = 0;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		{
			EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,entryId);
            stData.DeleteData("iowpi",pEntrySet,pTrans);
			pStorageEngine->closeEntrySet(pTrans,pEntrySet);
		}
		pTrans->commit();
		pStorageEngine->endThread();
	}
	VOID_CATCHEXCEPTION(pTrans)
}

bool DoBaseBackup(EntrySetID entryId)     //time2
{
    try
    {
		StorageEngine::getStorageEngine()->startArchive(true);

		std::string strBackup = ProgramOpts["-d"];
		thread_group thrGroup;
		StorageData stData(STORAGE_DATA_DIR + "time1");
		thrGroup.create_thread(boost::bind(BackUpThread,boost::ref(strBackup)));
		thrGroup.create_thread(boost::bind(InsertThread,boost::ref(stData),entryId));
		thrGroup.create_thread(boost::bind(UpdateThread,boost::ref(stData),entryId));
		thrGroup.create_thread(boost::bind(DeleteThread,boost::ref(stData),entryId));
		thrGroup.join_all();

		std::cout<<"base backup end!"<<std::endl;
		//StorageEngine::getStorageEngine()->endArchive(true);

		TransactionId xid = 0;
		Transaction* pTran = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTran,EntrySet::OPEN_EXCLUSIVE,entryId);
		ShowEntrySetData(pTran, pEntrySet);
		pTran->commit();

		stData.Save(STORAGE_DATA_DIR + "time2");
        return true;
    }
    CATCHEXCEPTION
}

bool MakeMoreChanges(EntrySetID entryId)  //time3
{
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,entryId);

		StorageData stData(STORAGE_DATA_DIR + "time2",pEntrySet,pTransaction);

		stData.InsertData("uiwen");
		stData.UpdateData("wi,wn","ioine");
		stData.DeleteData("22ipn");

		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);

		ShowEntrySetData(pTransaction,pEntrySet);

		pTransaction->commit();
		stData.Save(STORAGE_DATA_DIR + "time3");

		return true;
	}
	CATCHEXCEPTION
}

bool Recovery2Time2(EntrySetID entryId)
{
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,entryId);

		ShowEntrySetData(pTransaction,pEntrySet);

		StorageData stData(STORAGE_DATA_DIR + "time2",pEntrySet,pTransaction);
		bool bRet = stData.CheckIsSame();
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
        stData.Save(STORAGE_DATA_DIR + "time4");
		return bRet;
	}
	CATCHEXCEPTION

}



bool MakeSomeChanges(EntrySetID entryId)
{
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		std::cout<<"current timeline id : "<<pStorageEngine->getCurrentTimeLine()<<std::endl;

		std::cout<<"EntryID = "<<entryId<<std::endl;
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,entryId);
		if (NULL == pEntrySet)
		{
			std::cout<<"Open EntrySet failed !"<<std::endl;
		}

		 
		ShowEntrySetData(pTransaction,pEntrySet);

		StorageData stData(STORAGE_DATA_DIR + "time2",pEntrySet,pTransaction);
		
		stData.InsertData("ewion");
        std::cout<<"stData.InsertData(\"ewion\")"<<std::endl;

		stData.DeleteData("12ipw");
        std::cout<<"stData.DeleteData(\"12ipw\");"<<std::endl;
		
		bool bRet = stData.CheckIsSame();
		std::cout<<"stData.CheckIsSame()"<<std::endl;

		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		std::cout<<"pStorageEngine->closeEntrySet(pTransaction,pEntrySet)"<<std::endl;
		pTransaction->commit();
		if (bRet)
		{
			std::cout<<"bRet = true"<<endl;
		}
		else
		{
			std::cout<<"bRet = false"<<endl;
		}
		return bRet;
	}
	CATCHEXCEPTION

}

bool Recovery2Time3(EntrySetID entryId)
{
	try
	{
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,entryId);

		StorageData stData(STORAGE_DATA_DIR + "time3",pEntrySet,pTransaction);
		bool bRet = stData.CheckIsSame();
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();

		return bRet;
	}
	CATCHEXCEPTION

}

bool TestRecovery2Target( void )
{
	if (!(ProgramOpts.end() != ProgramOpts.find("-c") 
		&& ProgramOpts["-c"] == "recovery2target"))
	{
		std::cout<<"TestRecovery2Target havn't run, you can specify -c(recovery) option to run it"<<std::endl;
		return true;		
	}

	static const std::string strPharseFile = "../pharse";
	PharseFile parseFile(strPharseFile);

	switch(parseFile.Get())
	{
	case 0:
	    {
		    std::cout<<">>> Create Table: ";
		    EntrySetID id = 0;
		    bool ret = CreateOnTable(id);
		    parseFile.SetEntrySetId(id);
		    std::cout<<id<<std::endl;
		    return ret;
	    }
	case 1:
	    std::cout<<">>> Do base back up:time2"<<std::endl;
	    return DoBaseBackup(parseFile.GetEntrySetID());
	case 2:
	    std::cout<<">>> Make more changes:time3"<<std::endl;
	    return MakeMoreChanges(parseFile.GetEntrySetID());	
	case 3:
	    std::cout<<">>> Recovery to time2"<<std::endl;
	    return Recovery2Time2(parseFile.GetEntrySetID());
	case 4:
	    std::cout<<">>> Make some changes"<<std::endl;
	    return MakeSomeChanges(parseFile.GetEntrySetID());
	case 5:
	    std::cout<<">>> Recovery to time3"<<std::endl;
	    return Recovery2Time3(parseFile.GetEntrySetID());
	default:
		boost::filesystem::remove(strPharseFile);
		std::cout<<"waring: this test case only 6 pharses"<<std::endl;
	}
    return true;
}

void ShowEntrySetData(Transaction* pTran, EntrySet* pEntrySet)
{
	std::vector<std::string> vResult;
	GetDataFromEntrySet(vResult,pTran,pEntrySet);

	std::cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>EntrySet Data : "<<std::endl;
	std::vector<std::string>::iterator it;
	for (it = vResult.begin(); it != vResult.end(); it ++)
	{
		std::cout<<*it<<std::endl;
	}
}
