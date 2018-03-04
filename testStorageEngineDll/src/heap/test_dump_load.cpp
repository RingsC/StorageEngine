/**
* @file test_dump_load.cpp
* @brief 
* @author 李书淦
* @date 2012-2-8 14:23:44
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include "heap/test_dump_load.h"
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
extern Transaction *pTransaction;
using namespace std;
using namespace FounderXDB::StorageEngineNS;
/************************************************************************** 
* @brief GenerateInsertData 
* 随机生成待插入的数据
* Detailed description.
* @param[out] vInserted  返回生成的数据
**************************************************************************/
static void GenerateInsertData(std::vector<std::string>& vInserted)
{
	using namespace boost::assign;
    std:string strInsertData1,strInsertData2,strInsertData3,strInsertData4,strInsertData5;
	RandomGenString(strInsertData1,1<<1);
	RandomGenString(strInsertData2,(1<<13)-4);
	RandomGenString(strInsertData3,(1<<20) - 44);
	RandomGenString(strInsertData4,(1<<20) - 43);
	RandomGenString(strInsertData5,(1<<20) - 45);
	//RandomGenString(strInsertData1,1<<13);
	//RandomGenString(strInsertData2,1<<14);
	//RandomGenString(strInsertData3,1<<12);
	//RandomGenString(strInsertData4,1<<8);
	//RandomGenString(strInsertData5,1<<7);
	vInserted += strInsertData1,strInsertData2,strInsertData3,strInsertData4, strInsertData5;
}

bool test_dump_load( void )
{
	INTENT("1.创建EntrySet,插入一些数据;\n"
		"2. dump到一数据目录中\n"
		"3. 删除entryset\n"
        "4. 重新创建这个entryset\n"
		"5. 从test.dump中load数据\n"
        "6.查询插入结果是否正确.\n");

	std::vector<EntryID> vIds;
	std::vector<std::string> vInsertData;
	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetSingleColInfo());

		//打开EntrySet
		EntrySet* pEntrySet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//插入一些数据
		GenerateInsertData(vInsertData);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pTransaction,pEntrySet);


		//检查结果
		std::set<std::string> sDiredResult(vInsertData.begin(),vInsertData.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		CHECK_BOOL(sDiredResult == sResult);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	using namespace boost::filesystem;
	path fullpath(initial_path());
	fullpath = system_complete(path(".")); 
	fullpath /= "dump";
	
	std::string strDumpFile = fullpath.string() + "/eee";
	//dump to dump directory
	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		//打开EntrySet
		EntrySet* pEntrySet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		if (!boost::filesystem::exists(fullpath))
		{
			boost::filesystem::create_directory(fullpath);
		}
		pEntrySet->dump(strDumpFile.c_str(),pTransaction);
		std::list<EntryID> list;
		for(int i = 0; i < vIds.size(); ++i)
		{
			list.push_back(vIds[i]);
		}
		pEntrySet->deleteEntries(pTransaction,list);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION


	//load to an entryset
	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		//打开EntrySet
		EntrySet* pEntrySet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);
         
		pEntrySet->load(strDumpFile.c_str(),pTransaction);
		pStorageEngine->endStatement();

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pTransaction,pEntrySet);

		//检查结果
		std::set<std::string> sDiredResult(vInsertData.begin(),vInsertData.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		CHECK_BOOL(sDiredResult == sResult);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION
	return true;
}
