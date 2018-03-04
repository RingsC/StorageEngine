/**
* @file test_dump_load.cpp
* @brief 
* @author ������
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
* ������ɴ����������
* Detailed description.
* @param[out] vInserted  �������ɵ�����
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
	INTENT("1.����EntrySet,����һЩ����;\n"
		"2. dump��һ����Ŀ¼��\n"
		"3. ɾ��entryset\n"
        "4. ���´������entryset\n"
		"5. ��test.dump��load����\n"
        "6.��ѯ�������Ƿ���ȷ.\n");

	std::vector<EntryID> vIds;
	std::vector<std::string> vInsertData;
	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		pTransaction = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,GetSingleColInfo());

		//��EntrySet
		EntrySet* pEntrySet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);

		//����һЩ����
		GenerateInsertData(vInsertData);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pTransaction,pEntrySet);


		//�����
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

		//��EntrySet
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

		//��EntrySet
		EntrySet* pEntrySet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);
         
		pEntrySet->load(strDumpFile.c_str(),pTransaction);
		pStorageEngine->endStatement();

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pTransaction,pEntrySet);

		//�����
		std::set<std::string> sDiredResult(vInsertData.begin(),vInsertData.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		CHECK_BOOL(sDiredResult == sResult);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	CATCHEXCEPTION
	return true;
}
