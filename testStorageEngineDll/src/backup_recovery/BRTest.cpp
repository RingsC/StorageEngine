/**
* @file BRTest.cpp
* @brief 
* @author 李书淦
* @date 2012-2-14 9:24:57
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
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
extern Transaction* pTransaction;
extern StorageEngine *pStorageEngine;
typedef EntrySetCollInfo<2,3,8,2> EntrySet1;
typedef EntrySetCreator<EntrySet1> EntrySetCreator1;
typedef IndexCreator<EntrySet1,2,3> IndexCreator1;

typedef EntrySetCollInfo<7,2> EntrySet2;
typedef EntrySetCreator<EntrySet2> EntrySetCreator2;
typedef IndexCreator<EntrySet2,1> IndexCreator2;

typedef EntrySetCollInfo<2,2,1> EntrySet3;
typedef EntrySetCreator<EntrySet3> EntrySetCreator3;
typedef IndexCreator<EntrySet3,1,3> IndexCreator3;

using namespace boost;

extern std::map<std::string,std::string> ProgramOpts;

enum RunMode
{
	Prepare,
	Backup,
	Recovery

}g_eRunMode;
/************************************************************************** 
* @brief NeedRun 
* 
* Detailed description. 判断是否在执行backup 和 recovery的测试用例
* @param[in] void 
* @return bool  
**************************************************************************/
static bool NeedRun( void )
{
	if (ProgramOpts.end() != ProgramOpts.find("-c"))
	{
		using namespace boost::assign;
		std::string option = ProgramOpts["-c"];
		std::set<std::string> options;
		options += "preparebk","archive","recovery";
		if (options.find(option) != options.end())
		{
			return true;
		}
	}
	return false;
}

static void PrepareCollinfo( void )
{
	EntrySet1::get();
	IndexCollInfo<EntrySet1,2,3>::get();

	EntrySet2::get();
	IndexCollInfo<EntrySet2,1>::get();

	EntrySet3::get();
	IndexCollInfo<EntrySet3,1,3>::get();

}
namespace bf = boost::filesystem;
/************************************************************************** 
* @brief CopyFiles 
* 
* Detailed description 递归地拷贝from到to
* @param[in] from 被拷贝的文件夹或者文件
* @param[in] to   目的文件夹或者文件
**************************************************************************/
static void CopyFiles( const bf::path& from, const bf::path& to )
{
	if( !bf::exists(to) )
	{
		bf::create_directories( to );
	}    

	bf::directory_iterator end;

	for( bf::directory_iterator it(from); it != end; ++it )
	{
		bf::path newFrom = from;
		newFrom /= ((boost::filesystem::path)(*it)).filename();

		bf::path newTo = to;
		newTo /= ((boost::filesystem::path)(*it)).filename();

		if( bf::is_directory( newFrom ) )
		{
			CopyFiles( newFrom, newTo );
		}
		else if( bf::is_regular( newFrom ) )
		{
			bf::copy_file( newFrom, newTo, bf::copy_option::overwrite_if_exists );
		}
	}
}

vector<string> vInsertEntry1_1;
vector<string> vInsertEntry1_2;
vector<string> vUpdateEntry1_1;

vector<string> vInsertEntry2_1;
vector<string> vInsertEntry2_2;
vector<string> vDeleteEntry2_1;

vector<string> vInsertEntry3_1;
vector<string> vInsertEntry3_2;
vector<string> vUpdateEntry3_1;
static void PrepareData( void )
{
	using namespace boost::assign;
	vInsertEntry1_1 += "womwowlit,seive","oiwcnuw,xiwlsvr","ewweoi92342wnmi","oiwnc,seo8knsle";
	vInsertEntry1_2 += "uiwoubc,sedwwbi","aonxowqopiwinxc";
	vUpdateEntry1_1 += "oiwcnuw,xiwlsvr";  //update to iowoiwuiwenxoek

	vInsertEntry2_1 += "wewwwe2","iowewin","hiowncm","pinowio";
	vInsertEntry2_2 += "uuiwewu";
	vDeleteEntry2_1 += "wewwwe2";

	vInsertEntry3_1 += "iuwet","nhwib","powni","yhqoi";
	vInsertEntry3_2 += "8w,we","w32iw";
	vUpdateEntry3_1 += "nhwib"; //update to iowno
}

static void GetDesiredData(std::string const& strEntrySet,std::set<std::string>& vOut)
{
	if (Prepare == g_eRunMode)
	{
		if (strEntrySet == "EntrySet1")
		{
			vOut.insert(vInsertEntry1_1.begin(),vInsertEntry1_1.end());
			vOut.insert(vInsertEntry1_2.begin(),vInsertEntry1_2.end());
		}
		else if (strEntrySet == "EntrySet2")
		{
			vOut.insert(vInsertEntry2_1.begin(),vInsertEntry2_1.end());
			//vOut.insert(vOut.end(),vInsertEntry1_2.begin(),vInsertEntry1_2.end());	
		}
		else if (strEntrySet == "EntrySet3")
		{
			vOut.insert(vInsertEntry3_1.begin(),vInsertEntry3_1.end());
			//vOut.insert(vOut.end(),vInsertEntry1_2.begin(),vInsertEntry1_2.end());
		}
	}
	else if (Backup == g_eRunMode)
	{
		if (strEntrySet == "EntrySet1")
		{
			vOut.insert(vInsertEntry1_1.begin(),vInsertEntry1_1.end());
			vOut.insert(vInsertEntry1_2.begin(),vInsertEntry1_2.end());
			vOut.erase(std::find(vOut.begin(),vOut.end(),vUpdateEntry1_1[0]));
			vOut.insert("iowoiwuiwenxoek");
		}
		else if (strEntrySet == "EntrySet2")
		{
			vOut.insert(vInsertEntry2_1.begin(),vInsertEntry2_1.end());
			vOut.insert(vInsertEntry2_2.begin(),vInsertEntry2_2.end());
			vOut.erase(std::find(vOut.begin(),vOut.end(),vDeleteEntry2_1[0]));
		}
		else if (strEntrySet == "EntrySet3")
		{
			vOut.insert(vInsertEntry3_1.begin(),vInsertEntry3_1.end());
			vOut.erase(std::find(vOut.begin(),vOut.end(),vUpdateEntry3_1[0]));
			vOut.insert("iowno");
		}
	}
	else
	{
		if (strEntrySet == "EntrySet1")
		{
			vOut.insert(vInsertEntry1_1.begin(),vInsertEntry1_1.end());
			vOut.insert(vInsertEntry1_2.begin(),vInsertEntry1_2.end());
			vOut.erase(std::find(vOut.begin(),vOut.end(),vUpdateEntry1_1[0]));
			vOut.insert("iowoiwuiwenxoek");
		}
		else if (strEntrySet == "EntrySet2")
		{
			vOut.insert(vInsertEntry2_1.begin(),vInsertEntry2_1.end());
			vOut.insert(vInsertEntry2_2.begin(),vInsertEntry2_2.end());
			vOut.erase(std::find(vOut.begin(),vOut.end(),vDeleteEntry2_1[0]));	
		}
		else if (strEntrySet == "EntrySet3")
		{
			vOut.insert(vInsertEntry3_1.begin(),vInsertEntry3_1.end());
			vOut.erase(std::find(vOut.begin(),vOut.end(),vUpdateEntry3_1[0]));
			vOut.insert("iowno");
		}
	}
}

/************************************************************************** 
* @brief CheckBackResult 
* 
* Detailed description.  检查结果是否正确
* @param[in] pTrans 
* @param[in] pEntrySet1 
* @param[in] pIndex1 
* @param[in] pEntryset2 
* @param[in] pIndex2 
* @param[in] pEntryset3 
* @param[in] pIndex3 
* @return bool    如果结果正确，返回true,否则返回false
**************************************************************************/
static bool CheckBackResult(Transaction* pTrans,EntrySet* pEntrySet1,IndexEntrySet* pIndex1
							,EntrySet* pEntrySet2,IndexEntrySet* pIndex2
							,EntrySet* pEntrySet3,IndexEntrySet* pIndex3)
{
	//Check EntrySet1,Index1
	std::vector<string> vResult1;
	if(NULL != pEntrySet1)
	{
		GetDataFromEntrySet(vResult1,pTrans,pEntrySet1);
		std::set<string> sResult(vResult1.begin(),vResult1.end());
		std::set<std::string> sDesire1;
		GetDesiredData("EntrySet1",sDesire1);
		if (sResult != sDesire1)
		{
			return false;
		}
	}


	//Check EntrySet2,Index2
	std::vector<string> vResult2;
	if(NULL != pEntrySet2)
	{
		GetDataFromEntrySet(vResult2,pTrans,pEntrySet2);
		std::set<string> sResult(vResult2.begin(),vResult2.end());
		std::set<std::string> sDesire2 ;
		GetDesiredData("EntrySet2",sDesire2);
		if (sResult != sDesire2)
		{
			return false;
		}
	}

	//Check EntrySet3,Index3
	std::vector<string> vResult3;
	if (NULL != pEntrySet3)
	{
		GetDataFromEntrySet(vResult3,pTrans,pEntrySet3);
		std::set<std::string> sResult(vResult3.begin(),vResult3.end());
		std::set<std::string> sDesire3 ;
		GetDesiredData("EntrySet3",sDesire3);
		if (sResult != sDesire3)
		{
			return false;
		}
	}

	return true;
}

/************************************************************************** 
* @brief BackUpThread 
* 
* Detailed description. 这个函数是线程函数，用来做一个基础备份
* @param[in] baseBackDir 通过这个参数指定base backup要存入的目录
**************************************************************************/
bool BackUpThread(const std::string& baseBackDir)
{
	INTENT("1. 在数据目录下创建一个叫做backup_in_progress的文件用来确认backup已经开始;\n"
		"2. 调用startBackup,这相当于pg中的select pg_start_backup(label,fast);\n"
		"3. 将数据目录中的所有文件拷贝到指定目录;\n"
		"4. 调用endBackup,这相当pg中的select pg_stop_backup();\n"
		"5. 删除数据目录中的文件backup_in_progress\n");
	try
	{
		pStorageEngine->beginThread();
		TransactionId transId = 0;
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		{

			extern std::string g_strDataDir;
			if (!boost::filesystem::exists(g_strDataDir + "/backup_in_progress"))
			{
				std::fstream back_in_process((g_strDataDir + "/backup_in_progress").c_str());
				//boost::filesystem::create_directory(g_strDataDir + "/backup_in_progress");
			}

			pStorageEngine->startBackup(baseBackDir.c_str(),true);

			if (boost::filesystem::exists(baseBackDir))
			{
				boost::filesystem::remove_all(baseBackDir);
			}
			CopyFiles(g_strDataDir,baseBackDir);
			pStorageEngine->endBackup();

			boost::filesystem::remove(g_strDataDir + "/backup_in_progress");

		}
		pTransaction->commit();
		pStorageEngine->endThread();
		return false;
	}
	CATCHEXCEPTION
}


static void SaveIds(const std::map<std::string,EntrySetID>& ids)
{
	std::fstream idFile("ids",std::ios_base::out);
	BOOST_FOREACH(BOOST_TYPEOF(*ids.begin()) id,ids)
	{
		idFile<<id.first<<" "<<id.second<<std::endl;
	}
}

static void ParseIds(std::map<std::string,EntrySetID>& ids)
{
	std::fstream idFile("ids");
	std::string strLine;
	while (getline(idFile,strLine))
	{
		std::stringstream sstream(strLine);
		std::string strName;
		EntrySetID id;
		sstream>>strName>>id;
		ids[strName] = id;
	}
};

/************************************************************************** 
* @brief WorkingThread 
* 
* Detailed description. 工作线程，和备份线程并行工作
* @param[in] void 
**************************************************************************/
static bool WorkingThread( void )
{
	Transaction *pTrans = NULL;
	try
	{
		std::map<std::string,EntrySetID> Ids;
		ParseIds(Ids);
		pStorageEngine->beginThread();
		TransactionId transId = 0;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		{
			EntrySet* pEntrySet2 = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,Ids["EntrySet2"]);
			IndexEntrySet* pIndex2 = IndexCreator2::createAndOpen(pTrans,pEntrySet2,"");

			InsertData(pTrans,pEntrySet2,vInsertEntry2_2);

			std::vector<EntryID> vIds;
			GetEntryId(pTrans,pEntrySet2,vDeleteEntry2_1,vIds);
			DeleteData(pTrans,pEntrySet2,vIds);
			pStorageEngine->closeEntrySet(pTrans,pEntrySet2);
			pStorageEngine->closeEntrySet(pTrans,pIndex2);

			if(!CheckBackResult(pTrans,NULL,NULL,pEntrySet2,pIndex2,NULL,NULL))
			{
				std::cout<<"Working thread check failed"<<std::endl;
			}
		}
		pTrans->commit();
		pStorageEngine->endThread();
		return true;
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
}

static bool WorkingThread1( void )
{
	Transaction *pTrans = NULL;
	try
	{
		std::map<std::string,EntrySetID> Ids;
		ParseIds(Ids);
		pStorageEngine->beginThread();
		TransactionId transId = 0;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		{
			EntrySet* pEntrySet1 = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,Ids["EntrySet1"]);
			IndexEntrySet* pIndex1 = IndexCreator2::createAndOpen(pTrans,pEntrySet1,"");

			std::vector<EntryID> vIds;
			GetEntryId(pTrans,pEntrySet1,vUpdateEntry1_1,vIds);
			std::map<EntryID,std::string> vUpdateData;
			vUpdateData[vIds[0]] = "iowoiwuiwenxoek";
			UpdateData(pTrans,pEntrySet1,vUpdateData);


			pStorageEngine->closeEntrySet(pTrans,pEntrySet1);
			pStorageEngine->closeEntrySet(pTrans,pIndex1);

			if(!CheckBackResult(pTrans,pEntrySet1,pIndex1,NULL,NULL,NULL,NULL))
			{
				std::cout<<"Working thread check failed"<<std::endl;
			}
		}
		pTrans->commit();
		pStorageEngine->endThread();
		return true;
	}
	catch(StorageEngineException &ex)
	{
		pTrans->abort();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
}

static void CloseAll(Transaction* pTrans,EntrySet* pEntrySet1,IndexEntrySet* pIndex1
					 ,EntrySet* pEntryset2,IndexEntrySet* pIndex2
					 ,EntrySet* pEntryset3,IndexEntrySet* pIndex3)
{
	if(NULL != pIndex1)
	{
		pStorageEngine->closeEntrySet(pTrans,pIndex1);
	}

	if(NULL != pIndex2)
	{
		pStorageEngine->closeEntrySet(pTrans,pIndex2);
	}

	if(NULL != pIndex3)
	{
		pStorageEngine->closeEntrySet(pTrans,pIndex3);
	}

	if (NULL != pEntrySet1)
	{
		pStorageEngine->closeEntrySet(pTrans,pEntrySet1);
	}

	if(NULL != pEntryset2)
	{
		pStorageEngine->closeEntrySet(pTrans,pEntryset2);
	}

	if(NULL != pEntryset3)
	{
		pStorageEngine->closeEntrySet(pTrans,pEntryset3);
	}
}

//================================下面是测试用例的实现=====================================================//
bool prepareBackup( void )
{
	INTENT("1. 创建3个的表;"
		"2. 分别在其上创建3个索引;"
		"3. 确认表和索引的创建是正确的"
		"4. 提交事务");
	g_eRunMode = Prepare;
	if(!NeedRun())
	{
		return true;
	}
	PrepareCollinfo();
	try
	{
		bool result = true;
		PrepareData();
		std::map<std::string,EntrySetID> Ids;
		//创建一个第一个EntrySet并插入一些数据
		TransactionId transId = 0;
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet1 = EntrySetCreator1::createAndOpen(pTransaction,"test");
		Ids["EntrySet1"] = pEntrySet1->getId();

		InsertData(pTransaction,pEntrySet1,vInsertEntry1_1);

		IndexEntrySet* pIndex1 = IndexCreator1::createAndOpen(pTransaction,pEntrySet1,"index");
		Ids["Index1"] = pIndex1->getId();
		InsertData(pTransaction,pEntrySet1,vInsertEntry1_2);


		//创建第二个EntrySet并插入一些数据
		EntrySet *pEntrySet2 = EntrySetCreator2::createAndOpen(pTransaction,"test");
		Ids["EntrySet2"] = pEntrySet2->getId();
		InsertData(pTransaction,pEntrySet2,vInsertEntry2_1);


		//创建第三个EntrySet并插入一些数据
		EntrySet *pEntrySet3 = EntrySetCreator3::createAndOpen(pTransaction,"test");
		Ids["EntrySet3"] = pEntrySet3->getId();
		InsertData(pTransaction,pEntrySet3,vInsertEntry3_1);
		IndexEntrySet* pIndex3 = IndexCreator3::createAndOpen(pTransaction,pEntrySet3,"index");
		Ids["Index3"] = pIndex3->getId();
		if(!CheckBackResult(pTransaction,pEntrySet1,pIndex1,pEntrySet2,NULL,pEntrySet3,pIndex3))
		{
			result = false;
		}
		CloseAll(pTransaction,pEntrySet1,pIndex1,pEntrySet2,NULL,pEntrySet3,pIndex3);
		pTransaction->commit();

		SaveIds(Ids);

		return result;
	}
	CATCHEXCEPTION

		return true;
}



bool test_backup( void )
{
	g_eRunMode = Backup;
	if(!NeedRun())
	{
		return true;
	}
	PrepareCollinfo();
	PrepareData();
	std::string strBackup = ProgramOpts["-d"];

	if (0 == strBackup.length())
	{
		std::cout<<"Please specify a directory to store backup"<<std::endl;
		exit(0);
	}

	StorageEngine::getStorageEngine()->startArchive(true);

	thread_group thrGroup;
	thrGroup.create_thread(boost::bind(BackUpThread,boost::ref(strBackup)));
	thrGroup.create_thread(WorkingThread);
	thrGroup.create_thread(WorkingThread1);
	thrGroup.join_all();

	std::cout<<"base backup end!"<<std::endl;

	try
	{
		std::map<std::string,EntrySetID> Ids;
		ParseIds(Ids);
		TransactionId transId = 0;
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet3 = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,Ids["EntrySet3"]);
		std::vector<EntryID> vIds;
		GetEntryId(pTransaction,pEntrySet3,vUpdateEntry3_1,vIds);
		std::map<EntryID,std::string> mapData;
		mapData[vIds[0]] = "iowno";
		UpdateData(pTransaction,pEntrySet3,mapData);
		CheckBackResult(pTransaction,NULL,NULL,NULL,NULL,pEntrySet3,NULL);
		CloseAll(pTransaction,NULL,NULL,NULL,NULL,pEntrySet3,NULL);
		pTransaction->commit();
	}
	CATCHEXCEPTION

		try
	{
		std::map<std::string,EntrySetID> Ids;
		ParseIds(Ids);
		TransactionId transId = 0;
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet3 = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,Ids["EntrySet3"]);
		InsertData(pTransaction,pEntrySet3,vInsertEntry3_2);
		StorageEngineFixture::bEnded = true;
		extern void on_exit_reset(void);
		pStorageEngine->resetOnExit();
		exit(0);
	}
	CATCHEXCEPTION

		StorageEngine::getStorageEngine()->endArchive(true);
}

bool test_Recovery( void )
{
	g_eRunMode = Recovery;
	if(!NeedRun())
	{
		return true;
	}
	PrepareCollinfo();
	PrepareData();
	std::map<std::string,EntrySetID> Ids;
	ParseIds(Ids);
	EntrySetID idEntrySet1 = Ids["EntrySet1"],idEntrySet2 = Ids["EntrySet2"],idEntrySet3 = Ids["EntrySet3"];
	EntrySetID idIndex1 = Ids["Index1"],idIndex2 = Ids["Index2"],idIndex3 = Ids["Index3"];
	try
	{
		TransactionId transId = 0;
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet1 = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,idEntrySet1);
		IndexEntrySet* pIndex1 = pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet1,EntrySet::OPEN_EXCLUSIVE,idIndex1);
		EntrySet* pEntrySet2 = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,idEntrySet2);
		IndexEntrySet* pIndex2 = pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet1,EntrySet::OPEN_EXCLUSIVE,idIndex2); 
		EntrySet* pEntrySet3 = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,idEntrySet3);;
		IndexEntrySet *pIndex3 = pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet1,EntrySet::OPEN_EXCLUSIVE,idIndex3);
		bool bResult = CheckBackResult(pTransaction,pEntrySet1,pIndex1,pEntrySet2,pIndex2,pEntrySet3,pIndex3);
		CloseAll(pTransaction,pEntrySet1,pIndex1,pEntrySet2,NULL,pEntrySet3,pIndex3);

		pTransaction->commit();
		return bResult;
	}
	CATCHEXCEPTION

		return false;
}
