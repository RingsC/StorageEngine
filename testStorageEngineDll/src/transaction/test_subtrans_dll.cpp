/**
* @file test_subtrans.cpp
* @brief 
* @author 李书淦
* @date 2011-11-28 15:27:50
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <boost/assign.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
using namespace boost::assign;
using std::cout;
using std::endl;
extern Transaction* pTransaction;
#define CATCHSUBEXCEPTION \
	catch(StorageEngineException &ex)\
{\
	pSubTrans->abort();\
    std::cout << ex.getErrorNo() << std::endl;\
	std::cout << ex.getErrorMsg() << std::endl;\
	delete pSubTrans;\
	pSubTrans = NULL;\
	return false;\
}
bool TestSubTransactionDLL( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务，向表中插入另外一些数据;\n"
		"3. 提交子事务后，检测是否插入成功.\n");
	//申明一个有1列的表
	typedef EntrySetCollInfo<1> EntrySetT;

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;

	//1. 创建表和index，并插入一些数据
	try
	{
		//创建一个EntrySet并插入一些数据
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);
		{
			//insert some data in the db
			std::vector<std::string> vInserted;
			vInserted += "1","2","3";
			InsertData(pTransaction,pEntrySet,vInserted);

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			std::vector<std::string> vResult;
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult == sDesired);

			Transaction *pSubTrans = pTransaction->startSubTransaction();
			try
			{
				vInserted.clear();
				vInserted += "8","9";
				InsertData(pSubTrans,pEntrySet,vInserted);
				pSubTrans->commit();
				delete pSubTrans;
			}
			CATCHSUBEXCEPTION

			sDesired += "8","9";
			vResult.clear();
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult1(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult1 == sDesired);
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		return true;
	}
	CATCHEXCEPTION
}

bool TestSubTransactionAbortDLL( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务，向表中插入另外一些数据;\n"
		"3. 回滚.\n"
		"3. 检测子事务的原子性.\n");

	//申明一个有1列的表
	typedef EntrySetCollInfo<1> EntrySetT;

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;

	//1. 创建表和index，并插入一些数据
	try
	{
		//创建一个EntrySet并插入一些数据
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);
		{
			//insert some data in the db
			std::vector<std::string> vInserted;
			vInserted += "1","2","3";
				InsertData(pTransaction,pEntrySet,vInserted);

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			std::vector<std::string> vResult;
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult == sDesired);

			Transaction *pSubTrans = pTransaction->startSubTransaction();
			try
			{
				vInserted.clear();
				vInserted += "8","9";
				InsertData(pSubTrans,pEntrySet,vInserted);
				pSubTrans->abort();
				delete pSubTrans;
			}
			CATCHSUBEXCEPTION

			vResult.clear();
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult1(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult1 == sDesired);
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		return true;
	}
	CATCHEXCEPTION
}

bool TestMultiSubTransactionDLL( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务，向表中插入另外一些数据后提交;\n"
		"3. 启动另一个子事务，再插入一些数据\n"
		"4. 提交子事务后，检测是否插入成功.\n");
	//申明一个有1列的表
	typedef EntrySetCollInfo<1> EntrySetT;

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;

	//1. 创建表和index，并插入一些数据
	try
	{
		//创建一个EntrySet并插入一些数据
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);
		{
			//insert some data in the db
			std::vector<std::string> vInserted;
			vInserted += "1","2","3";
			InsertData(pTransaction,pEntrySet,vInserted);

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			std::vector<std::string> vResult;
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult == sDesired);

			Transaction *pSubTrans = pTransaction->startSubTransaction();
			try
			{
				vInserted.clear();
				vInserted += "8","9";
				InsertData(pSubTrans,pEntrySet,vInserted);
				pSubTrans->commit();
				delete pSubTrans;
			}
			CATCHSUBEXCEPTION

			pSubTrans = pTransaction->startSubTransaction();
			try
			{
				vInserted.clear();
				vInserted += "4","5";
				InsertData(pSubTrans,pEntrySet,vInserted);
				pSubTrans->commit();
				delete pSubTrans;
			}
			CATCHSUBEXCEPTION

			sDesired += "4","5","8","9";
			vResult.clear();
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult1(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult1 == sDesired);
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		return true;
	}
	CATCHEXCEPTION
}

bool TestMultiSubTransactionAbortDLL( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务，向表中插入另外一些数据后提交;\n"
		"3. 启动另一个子事务，再插入一些数据后回滚\n"
		"4. 提交子事务后，检测是否插入成功.\n");
	//申明一个有1列的表
	typedef EntrySetCollInfo<1> EntrySetT;

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;

	//1. 创建表和index，并插入一些数据
	try
	{
		//创建一个EntrySet并插入一些数据
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);
		{
			//insert some data in the db
			std::vector<std::string> vInserted;
			vInserted += "1","2","3";
			InsertData(pTransaction,pEntrySet,vInserted);

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			std::vector<std::string> vResult;
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult == sDesired);

			Transaction *pSubTrans = pTransaction->startSubTransaction();
			try
			{
				vInserted.clear();
				vInserted += "8","9";
				InsertData(pSubTrans,pEntrySet,vInserted);
				pSubTrans->commit();
				delete pSubTrans;
			}
			CATCHSUBEXCEPTION

				pSubTrans = pTransaction->startSubTransaction();
			try
			{
				vInserted.clear();
				vInserted += "4","5";
				InsertData(pSubTrans,pEntrySet,vInserted);
				pSubTrans->abort();
				delete pSubTrans;
			}
			CATCHSUBEXCEPTION

			sDesired += "8","9";
			vResult.clear();
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult1(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult1 == sDesired);
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		return true;
	}
	CATCHEXCEPTION
}

bool TestMultiLevelTransactionDLL( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务1，向表中插入另外一些数据\n"
		"3. 启动另一个子事务2，再插入一些数据\n"
		"4. 提交子事务2\n"
		"5. 提交子事务1\n"
		"6. 检测是否插入成功\n");
	//申明一个有1列的表
	typedef EntrySetCollInfo<1> EntrySetT;

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;

	//1. 创建表和index，并插入一些数据
	try
	{
		//创建一个EntrySet并插入一些数据
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);
		{
			//insert some data in the db
			std::vector<std::string> vInserted;
			vInserted += "1","2","3";
			InsertData(pTransaction,pEntrySet,vInserted);

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			std::vector<std::string> vResult;
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult == sDesired);

			Transaction *pSubTrans = pTransaction->startSubTransaction();
			try
			{
				vInserted.clear();
				vInserted += "8","9";
				InsertData(pSubTrans,pEntrySet,vInserted);

				Transaction *pOldTrans = pSubTrans;
				pSubTrans = pOldTrans->startSubTransaction();
				try
				{
					vInserted.clear();
					vInserted += "4","5";
					InsertData(pSubTrans,pEntrySet,vInserted);
					pSubTrans->commit();
					delete pSubTrans;
				}
				CATCHSUBEXCEPTION
				pSubTrans = pOldTrans;

				pSubTrans->commit();
				delete pSubTrans;
			}
			CATCHSUBEXCEPTION

			sDesired += "4","5","8","9";
			vResult.clear();
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult1(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult1 == sDesired);
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		return true;
	}
	CATCHEXCEPTION
}


bool TestMultiLevelTransactionAbortDLL( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务1，向表中插入另外一些数据\n"
		"3. 启动另一个子事务2，再插入一些数据后回滚\n"
		"4. 提交子事务1\n"
		"5. 检测是否插入成功\n");
	typedef EntrySetCollInfo<1> EntrySetT;

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;
	//EntrySetID  entryId,index1Id,index2Id,index3Id;

	//1. 创建表和index，并插入一些数据
	try
	{
		//创建一个EntrySet并插入一些数据
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		TableDroper::entryId = pStorageEngine->createEntrySet(pTransaction,EntrySetT::get());
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);
		{
			//insert some data in the db
			std::vector<std::string> vInserted;
			vInserted += "1","2","3";
			InsertData(pTransaction,pEntrySet,vInserted);

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			std::vector<std::string> vResult;
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult == sDesired);

			Transaction *pSubTrans = pTransaction->startSubTransaction();
			try
			{
				vInserted.clear();
				vInserted += "8","9";
				InsertData(pSubTrans,pEntrySet,vInserted);

				Transaction *pOldTrans = pSubTrans;
				pSubTrans = pOldTrans->startSubTransaction();
				try
				{
					vInserted.clear();
					vInserted += "4","5";
					InsertData(pSubTrans,pEntrySet,vInserted);
					pSubTrans->abort();
					delete pSubTrans;
				}
				CATCHSUBEXCEPTION
				pSubTrans = pOldTrans;

				pSubTrans->commit();
				delete pSubTrans;
			}
			CATCHSUBEXCEPTION

			sDesired += "8","9";
			vResult.clear();
			GetDataFromEntrySet(vResult,pTransaction,pEntrySet);
			std::set<std::string> sResult1(vResult.begin(),vResult.end());
			CHECK_BOOL(sResult1 == sDesired);
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
		return true;
	}
	CATCHEXCEPTION
}

static void simple_entry_split(RangeDatai& rd, const char *data, int col, size_t charlen)
{
	rd.len = 0;
	rd.start = 0;

	if(col == 1)
	{
		rd.start = 0;
		rd.len = 4;
	}
	else if(col == 2)
	{
		rd.start = 4;
		rd.len = 4;
	}
}

static void simple_i_entry_split(RangeDatai& rd, const char *data, int col, size_t charlen)
{
	rd.len = 0;
	rd.start = 0;

	if(col == 1)
	{
		rd.start = 0;
		rd.len = 4;
	}
}

static ColumnInfo* init_entry_colinfo()
{
	ColumnInfo *info = (ColumnInfo*)malloc(sizeof(ColumnInfo));
	info->keys = 2;
	info->col_number = NULL;
	info->rd_comfunction = NULL;
	info->split_function = simple_entry_split;
	return info;
}

extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);

static ColumnInfo* init_i_entry_colinfoA()
{
    std::map<int,CompareCallbacki> mapComp;
	insert(mapComp)(1,my_compare_str);
	static MyColumnInfo columnInfo(mapComp,simple_i_entry_split);
	return &(columnInfo.Get());
}

static ColumnInfo* init_i_entry_colinfoB()
{
	ColumnInfo *info = init_i_entry_colinfoA();
	info->col_number[0] = 2;
	return info;
}

extern void insert_data(char data[][200], 
												const int data_len,
												EntrySet **pEntrySet,
												Transaction *transaction);

bool TestMultiSubTransCreateIndex()
{
	INTENT("测试使用子事务在一个表上创建多个索引和插入数据。"
				 "测试跨子事务的创建索引和插入操作是否正常。");

	int return_sta = true;

	StorageEngine *pStorageEngine = NULL;
	TransactionId tran_id = 0;
	EntrySetID entry_id = 0, i_entry_idA = 0, i_entry_idB = 0;
	EntrySet *entry_set = NULL;
	IndexEntrySet *i_entry_setA = NULL, *i_entry_setB = NULL;
	int32 entry_col_id = get_col_id(),
				i_A_entry_col_id = get_col_id(),
				i_B_entry_col_id = get_col_id();
	try
	{
		pStorageEngine = StorageEngine::getStorageEngine();
		pTransaction = pStorageEngine->getTransaction(tran_id,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	}catch (StorageEngineException &ex)
	{
		printf("%s\n", ex.getErrorMsg());
		return false;
	}

	char data[][200] = 
	{
		"123",
		"456",
		"789"
	};

	try
	{
		ColumnInfo *entry_info = init_entry_colinfo();
		setColumnInfo(entry_col_id, entry_info);
		entry_id = pStorageEngine->createEntrySet(pTransaction, entry_col_id);
		entry_set = pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,entry_id);
		entry_id = entry_set->getId();
		insert_data(data, ARRAY_LEN_CALC(data), &entry_set, pTransaction);

		/* 这里开始启动子事务创建索引和插入数据 */
		{
			Transaction *sub_tran = pTransaction->startSubTransaction();
			ColumnInfo *i_entry_info = init_i_entry_colinfoA();
			setColumnInfo(i_A_entry_col_id, i_entry_info);
			i_entry_idA = pStorageEngine->createIndexEntrySet(sub_tran, entry_set, BTREE_INDEX_ENTRY_SET_TYPE, i_A_entry_col_id);
			insert_data(data, ARRAY_LEN_CALC(data), &entry_set, pTransaction);
			sub_tran->commit();
		}

		{
			Transaction *sub_tran = pTransaction->startSubTransaction();
			insert_data(data, ARRAY_LEN_CALC(data), &entry_set, pTransaction);
			ColumnInfo *i_entry_info = init_i_entry_colinfoB();
			setColumnInfo(i_B_entry_col_id, i_entry_info);
			i_entry_idB = pStorageEngine->createIndexEntrySet(sub_tran, entry_set, BTREE_INDEX_ENTRY_SET_TYPE, i_B_entry_col_id);
			sub_tran->commit();
		}

		/* 这里开始启动子事务用索引A和B扫描 */
		{
			Transaction *sub_tran = pTransaction->startSubTransaction();
			i_entry_setA = pStorageEngine->openIndexEntrySet(sub_tran, entry_set, EntrySet::OPEN_EXCLUSIVE, i_entry_idA);
			/* 此时表中有9条数据分别是重复3次的123 456 789 */
			ScanCondition sc(1, ScanCondition::LessThan, (se_uint64)("789"), 3, my_compare_str);
			vector<ScanCondition> v_sc_a;
			v_sc_a.push_back(sc);
			EntrySetScan *ess = i_entry_setA->startEntrySetScan(sub_tran,BaseEntrySet::SnapshotMVCC,v_sc_a);
			EntryID ei;
			DataItem di;
			int32 count = 0;
			vector<std::string> v_str;
			while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
			{
				v_str.push_back((char*)di.getData());
				++count;
			}
			i_entry_setA->endEntrySetScan(ess);
			pStorageEngine->closeEntrySet(sub_tran, i_entry_setA);
			sub_tran->commit();
			pStorageEngine->closeEntrySet(pTransaction, entry_set);
			pStorageEngine->removeEntrySet(pTransaction, entry_id);
			pTransaction->commit();
			sort(v_str.begin(), v_str.end());
			if(count == 6)
			{
				for(int i = 0; i < v_str.size(); i += 3)
				{
					if(!(v_str[i] == v_str[i + 1] && v_str[i + 1] == v_str[i + 2]))
					{
						return_sta = false;
						break;
					}
				}
			}else
			{
				return_sta = false;
			}
		}

	}catch (StorageEngineException &ex)
	{
		printf("%s\n", ex.getErrorMsg());
		return false;
	}

	return return_sta;
}
