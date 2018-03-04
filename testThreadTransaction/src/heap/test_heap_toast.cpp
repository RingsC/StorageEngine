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
* ������ɴ����������
* Detailed description.
* @param[out] vInserted  �������ɵ�����
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
* ��������update������
* Detailed description. 
* @param[out] mapUpdateData  ��id����Ϊdata
* @param[in/out] vAfterUpdate ������ǰ��������ݣ�����update�Ժ��ֵ
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
* ������Ҫɾ���������id��ɾ���Ժ�Ľ��
* Detailed description.
* @param[in/out] vAfterUpdate �������������ݵ�ȫ��������ɾ���Ժ�Ľ��
* @param[in/out] vIds �����������������id,������Ҫɾ�����ݵ�id
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

		//��EntrySet
		EntrySet *pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		//����һЩ����
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

		//�����
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

		//��EntrySet
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		//����һЩ����
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
		//��������
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

		//�����
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
	//INTENT("1.����EntrySet,����һЩ������;\n"
	//	"2. ��������"
	//	"2.��ѯ���º�Ľ���Ƿ���ȷ.\n");

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

		//��EntrySet
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);

		//����һЩ����
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

		//ɾ������
		GenerateDeleteData(pArgs->vInsertData,pArgs->vIds);  //vInsertData���ڳ���ɾ����Ľ����vIds����Ҫɾ�����������ids.
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

		//�����
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