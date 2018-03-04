/**
* @file test_toast_dll.cpp
* @brief 
* @author 李书淦
* @date 2011-9-23 15:58:57
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include "heap/test_toast_dll.h"
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <boost/assign.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "index/test_index_insert_dll.h"
#include <boost/timer.hpp>
#include <boost/thread/thread.hpp>
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
							   ,vector<std::string>& vAfterUpdate
							   ,const std::vector<EntryID>& vIds)
{
	using namespace boost::assign;
    std:string strUpdateData1,strUpdateData2,strUpdateData3;
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
static void GenerateDeleteData(vector<std::string>& vAfterUpdate
							   ,std::vector<EntryID>& vIds)
{
    vIds.erase(vIds.begin() + 4);
	vIds.erase(vIds.begin() + 2);

	vAfterUpdate.erase(vAfterUpdate.begin() + 3);	
	vAfterUpdate.erase(vAfterUpdate.begin() + 1);
	vAfterUpdate.erase(vAfterUpdate.begin());
}

bool test_toast_insert(void)
{
	INTENT("1.创建EntrySet,插入一些大数据;\n"
		"2.查询插入结果是否正确.\n");
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
		std::vector<std::string> vInsertData;
		GenerateInsertData(vInsertData);
		InsertData(pTransaction,pEntrySet,vInsertData);

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

bool test_toast_update(void)
{
	INTENT("1.创建EntrySet,插入一些大数据;\n"
		"2. 更新数据"
		"2.查询更新后的结果是否正确.\n");
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
		std::vector<std::string> vInsertData;
		std::vector<EntryID> vIds;
		GenerateInsertData(vInsertData);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//更新数据
		std::map<EntryID,std::string> mapUpdateData;
		GenerateUpdateData(mapUpdateData,vInsertData,vIds);
		UpdateData(pTransaction,pEntrySet,mapUpdateData);

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

bool test_toast_delete(void)
{
	INTENT("1.创建EntrySet,插入一些大数据;\n"
		"2. 删除数据"
		"2.查询删除后的结果是否正确.\n");
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
		std::vector<std::string> vInsertData;
		std::vector<EntryID> vIds;
		GenerateInsertData(vInsertData);
		InsertData(pTransaction,pEntrySet,vInsertData,&vIds);

		//删除数据
		GenerateDeleteData(vInsertData,vIds);  //vInsertData现在成了删除后的结果，vIds是需要删除的数据项的ids.
		DeleteData(pTransaction,pEntrySet,vIds);

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

static void GenerateOneString(std::string str)
{
	static bool init = false;
	static char s_cData[10000];

	if(!init)
	{
		srand(1);

		for(uint32 i = 0; i < 10000; i++)
		{
			s_cData[i] = rand()%26 + 'a';
		}

		init = true;
	}

	uint32 data_len = rand()%7000 + 3000;
	str.append(s_cData, data_len);
}

static void test_toast_vacuum_heap_split(RangeDatai& range, const char *str, int col, size_t len = 0)
{
	if(col == 1)
	{
		range.start = 0;
		range.len = len;
	}
}

bool test_toast_vacuum(void)
{
	uint32 heap_colid = 565656;
	ColumnInfo *pcol_info = new ColumnInfo;
	pcol_info->keys = 2;
	pcol_info->col_number = NULL;
	pcol_info->rd_comfunction = NULL;
	pcol_info->split_function = test_toast_vacuum_heap_split;
	setColumnInfo(heap_colid, pcol_info);

	StorageEngine* pSE = StorageEngine::getStorageEngine();
	TransactionId xid = 0;
	Transaction *pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	/* create database */
	try
	{
		pSE->createDatabase(pTrans, "testDB", "defaulttablespace");
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	pTrans->commit();

	xid = 0;
	pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	try
	{
		pSE->setCurrentDatabase("testDB");

		EntrySetID heapId;
		/* create heap */
		heapId = pSE->createEntrySet(pTrans, heap_colid);
		/* open heap */
		EntrySet *pEntrySet = pSE->openEntrySet(pTrans, EntrySet::OPEN_EXCLUSIVE, heapId);

		EntryID eid;

		/* insert */
		{
			std::string data_insert;
			GenerateOneString(data_insert);

			DataItem data;
			data.setData((void*)data_insert.c_str());
			data.setSize(data_insert.length());
			pEntrySet->insertEntry(pTrans, eid, data);
			command_counter_increment();
		}

		/* update */
		{
			for(uint32 i = 0; i < 10000; i++)
			{
				std::string data_update;
				GenerateOneString(data_update);

				DataItem data;
				data.setData((void*)data_update.c_str());
				data.setSize(data_update.length());
				pEntrySet->updateEntry(pTrans, eid, data);
				command_counter_increment();

				{
					std::vector<ScanCondition> vConditions;
					EntrySetScan *pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotNOW,vConditions);

					DataItem entry;
					uint32 count = 0;
					while(0 == pScan->getNext(EntrySetScan::NEXT_FLAG, eid, entry))
					{
						count++;
						MemoryContext::deAlloc(entry.getData());
					}
					pEntrySet->endEntrySetScan(pScan);

					if(count != 1)
					{
						throw false;
					}
				}
			}
		}
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		delete pTrans;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	catch(bool& ret)
	{
		pTrans->abort();
		delete pTrans;
		return ret;
	}

	pTrans->commit();

	boost::thread::sleep(boost::get_system_time()
						+ boost::posix_time::milliseconds(360000));

	return true;
}
