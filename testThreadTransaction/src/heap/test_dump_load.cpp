#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include "TestFrame/TestFrame.h"
#include "TestFrame/TestTask.h"
#include "TestFrameCommon/TestFrameCommon.h"
#include "heap/test_dump_load.h"
#include "test_utils/test_utils.h"

using namespace FounderXDB::StorageEngineNS;
class dump_load : public ParamBase
{
public:
	EntrySetID entrysetId;
	std::vector<EntryID> vIds;
	std::vector<std::string> vInsertData;
	std::string strDumpFile;
};

/************************************************************************** 
* @brief GenerateInsertData 
* 随机生成待插入的数据
* Detailed description.
* @param[out] vInserted  返回生成的数据
**************************************************************************/
static void GenerateInsertData(std::vector<std::string>& vInserted)
{
	using namespace boost::assign;
	std::string strInsertData1,strInsertData2,strInsertData3,strInsertData4,strInsertData5;
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

bool dump_load_create_entryset(ParamBase* arg)
{
	dump_load *pArgs = (dump_load*)arg;

	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		pArgs->entrysetId = pStorageEngine->createEntrySet(pArgs->GetTransaction() ,GetSingleColInfo());
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool dump_load_insert_data(ParamBase* arg)
{
	dump_load *pArgs = (dump_load*)arg;

	try
	{
		//打开EntrySet
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		//插入一些数据
		GenerateInsertData(pArgs->vInsertData);
		InsertData(pArgs->GetTransaction(),pEntrySet,pArgs->vInsertData,&pArgs->vIds);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(), pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool dump_load_check_insert_result(ParamBase* arg)
{
	dump_load *pArgs = (dump_load*)arg;

	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(arg->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);
		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);
		std::set<std::string> sDiredResult(pArgs->vInsertData.begin(),pArgs->vInsertData.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		pArgs->SetSuccessFlag(sDiredResult == sResult);
		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}


bool dump_load_dump(ParamBase* arg)
{
	dump_load *pArgs = (dump_load*)arg;

	try
	{
		using namespace boost::filesystem;
		path fullpath(initial_path());
		fullpath = system_complete(path("."));
		fullpath /= "dump";
		std::string strDumpFile = fullpath.string() + "/eee";

		//dump to dump directory
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();

		//打开EntrySet
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		if (!boost::filesystem::exists(fullpath))
		{
			boost::filesystem::create_directory(fullpath);
		}

		pEntrySet->dump(strDumpFile.c_str(),pArgs->GetTransaction());

		pArgs->strDumpFile = strDumpFile;

		pStorageEngine->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool dump_load_delete_entryset(ParamBase* arg)
{
	dump_load *pArgs = (dump_load*)arg;

	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		std::list<EntryID> list;
		for(unsigned int i = 0; i < pArgs->vIds.size(); ++i)
		{
			list.push_back(pArgs->vIds[i]);
		}

		pEntrySet->deleteEntries(pArgs->GetTransaction(),list);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}


bool dump_load_load(ParamBase* arg)
{
	dump_load *pArgs = (dump_load*)arg;

	try
	{
		using namespace boost::assign;
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();

		//打开EntrySet
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		pEntrySet->load(pArgs->strDumpFile.c_str(),pArgs->GetTransaction());

		pStorageEngine->endStatement();
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool dump_load_check_load_result(ParamBase* arg)
{
	dump_load *pArgs = (dump_load*)arg;

	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);

		//检查结果
		std::set<std::string> sDiredResult(pArgs->vInsertData.begin(),pArgs->vInsertData.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		pArgs->SetSuccessFlag(sDiredResult == sResult);
		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}


bool test_dump_load()
{
	//INTENT("1.创建EntrySet,插入一些数据;\n"
	//	"2. dump到一数据目录中\n"
	//	"3. 删除entryset\n"
	//	"4. 重新创建这个entryset\n"
	//	"5. 从test.dump中load数据\n"
	//	"6.查询插入结果是否正确.\n");

	//INITRANID();

	int TestTranId = IncreaseTransId();
	dump_load *pArgs = new dump_load;

	REGTASK(dump_load_create_entryset, pArgs);
	REGTASK(dump_load_insert_data, pArgs);
	REGTASK(dump_load_check_insert_result, pArgs);

	//    INITRANID();
	//TestTranId = IncreaseTransId();
	//dump entryset
	REGTASK(dump_load_dump, pArgs);
	REGTASK(dump_load_delete_entryset, pArgs);

	//	INITRANID();
	//TestTranId = IncreaseTransId();
	//load to an entryset
	REGTASK(dump_load_load, pArgs);
	REGTASK(dump_load_check_load_result, pArgs);

	return true;
}