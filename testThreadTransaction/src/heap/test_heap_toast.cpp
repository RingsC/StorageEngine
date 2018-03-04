#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <boost/assign.hpp>
#include "StorageEngine.h"
#include "index/test_index_insert_dll.h"
#include "TestFrameCommon/TestFrameCommon.h"
#include "TestFrame/TestFrame.h"
#include "TestFrame/TestTask.h"
#include "heap/test_toast_dll.h"
#include "test_utils/test_utils.h"
#include "heap/test_heap_create.h"

//using namespace std;
using namespace FounderXDB::StorageEngineNS;

class test_toast : public HeapBase
{
public:
	EntrySetID entrysetId;
	std::vector<std::string> vInsertData;
	std::vector<EntryID> vIds;
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
	RandomGenString(strInsertData1,1<<20);
	RandomGenString(strInsertData2,1<<8);
	RandomGenString(strInsertData3,1<<24);
	RandomGenString(strInsertData4,1<<17);
	RandomGenString(strInsertData5,1<<7);
	vInserted += strInsertData1,strInsertData2,strInsertData3,strInsertData4,strInsertData5;

}

/************************************************************************** 
* @brief GenerateUpdateData 
* 生成用于update的数据
* Detailed description. 
* @param[out] mapUpdateData  将id更新为data
* @param[in/out] vAfterUpdate 传入以前插入的数据，返回update以后的值
* @param[in] vIds 
**************************************************************************/
static void GenerateUpdateData(std::map<EntryID,std::string>& mapUpdateData
							   ,std::vector<std::string>& vAfterUpdate
							   ,const std::vector<EntryID>& vIds)
{
	using namespace boost::assign;
	std::string strUpdateData1,strUpdateData2,strUpdateData3;
	RandomGenString(strUpdateData1,1<<8);   //strInsertData1 update to strUpdateData1
	RandomGenString(strUpdateData2,1<<25);  //strInsertData2 update to strUpdateData2
	RandomGenString(strUpdateData3,1<<13);  //strInsertData4 update to strUpdateData3

	vAfterUpdate[0].clear();
	vAfterUpdate[1].clear();
	vAfterUpdate[3].clear();

	vAfterUpdate[0] += strUpdateData1;
	vAfterUpdate[1] += strUpdateData2;
	vAfterUpdate[3] += strUpdateData3;  


	insert(mapUpdateData)(vIds[0],strUpdateData1)(vIds[1],strUpdateData2)(vIds[3],strUpdateData3);
}


/************************************************************************** 
* @brief GenerateDeleteData 
* 返回需要删除数据项的id和删除以后的结果
* Detailed description.
* @param[in/out] vAfterUpdate 输入是所有数据的全集，返回删除以后的结果
* @param[in/out] vIds 输入是所有数据项的id,返回需要删除数据的id
**************************************************************************/
static void GenerateDeleteData(std::vector<std::string>& vAfterUpdate
							   ,std::vector<EntryID>& vIds)
{
	vIds.erase(vIds.begin() + 4);
	vIds.erase(vIds.begin() + 2);

	vAfterUpdate.erase(vAfterUpdate.begin() + 3);	
	vAfterUpdate.erase(vAfterUpdate.begin() + 1);
	vAfterUpdate.erase(vAfterUpdate.begin());
}

bool test_toast_insert_create_entryset(ParamBase* arg)
{
	test_toast *pArgs = (test_toast*)arg;

	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		pArgs->entrysetId = pStorageEngine->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());

		//打开EntrySet
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		//插入一些数据
		GenerateInsertData(pArgs->vInsertData);
		InsertData(pArgs->GetTransaction(),pEntrySet,pArgs->vInsertData);

		pStorageEngine->closeEntrySet(pArgs->GetTransaction(), pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_toast_insert_check_result(ParamBase* arg)
{
	test_toast *pArgs = (test_toast*)arg;
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
bool test_toast_insert()
{
	INITRANID();
	test_toast *pArgs = new test_toast;
	pArgs->strCaseName = __FUNCTION__;
	REGTASK(test_toast_insert_create_entryset,pArgs);
	REGTASK(test_toast_insert_check_result,pArgs);
	REGTASK(test_heap_create_drop_task,pArgs);

	return true;
}


bool test_toast_update_create(ParamBase* arg)
{
	test_toast* pArgs = (test_toast*)arg;
	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		pArgs->entrysetId = pStorageEngine->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());

		//打开EntrySet
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		//插入一些数据
		GenerateInsertData(pArgs->vInsertData);
		InsertData(pArgs->GetTransaction(),pEntrySet,pArgs->vInsertData,&pArgs->vIds);
		pStorageEngine->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)


	return true;
}
bool test_toast_update_modify(ParamBase* arg)
{
	test_toast* pArgs = (test_toast*)arg;
	try
	{
		EntrySet *pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId); 
		//更新数据
		std::map<EntryID,std::string> mapUpdateData;
		GenerateUpdateData(mapUpdateData,pArgs->vInsertData,pArgs->vIds);
		UpdateData(pArgs->GetTransaction(),pEntrySet,mapUpdateData);
		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(), pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_toast_update_check_result(ParamBase* arg)
{
	test_toast* pArgs = (test_toast*)arg;
	try
	{
		EntrySet *pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId); 

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);

		//检查结果
		std::set<std::string> sDiredResult(pArgs->vInsertData.begin(),pArgs->vInsertData.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		pArgs->SetSuccessFlag(sDiredResult == sResult);

		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(), pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;

}
bool test_toast_update(void)
{
	//INTENT("1.创建EntrySet,插入一些大数据;\n"
	//	"2. 更新数据"
	//	"2.查询更新后的结果是否正确.\n");

	INITRANID();
	test_toast *pArgs = new test_toast;
	pArgs->strCaseName = __FUNCTION__;
	REGTASK(test_toast_update_create, pArgs);
	REGTASK(test_toast_update_modify, pArgs);
	REGTASK(test_toast_update_check_result, pArgs);
	REGTASK(test_heap_create_drop_task,pArgs);

	return true;
}


bool test_toast_delete_create(ParamBase* arg)
{
	test_toast *pArgs = (test_toast*)arg;
	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();

		pArgs->entrysetId = pStorageEngine->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());

		//打开EntrySet
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		//插入一些数据
		GenerateInsertData(pArgs->vInsertData);
		InsertData(pArgs->GetTransaction(),pEntrySet,pArgs->vInsertData,&pArgs->vIds);
		pStorageEngine->closeEntrySet(pArgs->GetTransaction(), pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)


	return true;
}
bool test_toast_delete_remove(ParamBase* arg)
{
	test_toast *pArgs = (test_toast*)arg;
	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();

		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		//删除数据
		GenerateDeleteData(pArgs->vInsertData,pArgs->vIds);  //vInsertData现在成了删除后的结果，vIds是需要删除的数据项的ids.
		DeleteData(pArgs->GetTransaction(),pEntrySet,pArgs->vIds);

		pStorageEngine->closeEntrySet(pArgs->GetTransaction(), pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)


	return true;
}
bool test_toast_delete_check(ParamBase* arg)
{
	test_toast *pArgs = (test_toast*)arg;
	try
	{
		StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		std::vector<std::string> vResult;
		GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);

		//检查结果
		std::set<std::string> sDiredResult(pArgs->vInsertData.begin(),pArgs->vInsertData.end());
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		pArgs->SetSuccessFlag(sDiredResult == sResult);

		pStorageEngine->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)


	return true;
}
bool test_toast_delete(void)
{
	INITRANID();
	test_toast *pArgs = new test_toast;
	pArgs->strCaseName = __FUNCTION__;
	REGTASK(test_toast_delete_create, pArgs);
	REGTASK(test_toast_delete_remove, pArgs);
	REGTASK(test_toast_delete_check, pArgs);
	REGTASK(test_heap_create_drop_task, pArgs);

	return true;
}