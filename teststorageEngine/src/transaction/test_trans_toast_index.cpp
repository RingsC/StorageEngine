/**
* @file test_trans_toast_index.cpp
* @brief 
* @author 李书淦
* @date 2011-9-20 16:33:50
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <map>
#include <vector>
#include <set>
#include <boost/assign.hpp>
#include "utils/util.h"
#include "test_fram.h"
#include "interface/FDPGAdapter.h"
#include "utils/toast_test_utils.h"
#include "transaction/test_trans_toast_index.h"

/************************************************************************** 
* @brief PrepareData 
* 随时生成并返回需要插入的数据
* Detailed description.
* @param[out] sInserted  返回需要插入数据的集合
* @param[out] sResult 返回使用index查询的结果
**************************************************************************/
static void PrepareData(std::vector<std::string>& sInserted,std::set<std::string>& sResult)
{
	using namespace boost::assign;
	std::string strInsertData1 ;
	std::string strInsertData2 ;
	std::string strInsertData3 ;
	std::string strInsertData4 ;
	std::string strInsertData5 ;

	strInsertData1 += "123456";
	strInsertData2 +=	"123789";
	strInsertData3 +=	"012562";
	strInsertData4 +=	"012120";
	strInsertData5 += "012856";
    
	RandomGenString(strInsertData1,1<<20);
	RandomGenString(strInsertData2,1<<8);
	RandomGenString(strInsertData3,1<<24);
	RandomGenString(strInsertData4,1<<17);
	RandomGenString(strInsertData5,1<<19);
    sInserted += strInsertData1,strInsertData2,strInsertData3,strInsertData4,strInsertData5;
    sResult += strInsertData3,strInsertData5;

}

/************************************************************************** 
* @brief PrepareDBAndIndex 
* 创建数据库，并在其上建立索引，返回索引查询的结果
* Detailed description.
* @param[in] tablespaceId  
* @param[in] relId 关系标识
* @param[in] idxId 索引标识
* @param[out] sResult 索引查询结果
**************************************************************************/
static void PrepareDBAndIndex(Oid tablespaceId,Oid relId,Oid idxId,std::set<std::string>& sResult,std::vector<std::string>& vInserted)
{
	begin_first_transaction();
	{
		SimpleHeap heap(tablespaceId,relId,FixSpliter::split,true,false);
		//open the db
		using namespace boost::assign;
		std::vector<int> v;
		v += 3,2,1;
		FixSpliter split(v);
		heap.Open(RowExclusiveLock);

		//insert some toast data
		PrepareData(vInserted,sResult);
		heap.Insert(vInserted[0]);
		heap.Insert(vInserted[1]);
		heap.Insert(vInserted[2]);
		FDPG_Transaction::fd_CommandCounterIncrement();

		std::map<int,CompareCallback> vecKeys;
		vecKeys[1] = str_compare;
		vecKeys[2] = str_compare;
        SimpleIndex index(heap,idxId,vecKeys,true,false);
		heap.Insert(vInserted[3]);
		heap.Insert(vInserted[4]);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		//check the result
		std::set<std::string> setResult = index.Find(searchCondition);
		CHECK_BOOL(setResult == sResult);
	}
	commit_transaction();
}


bool test_toastIndex_InsertRollabck(void)
{
	INTENT("1.创建heap并插入一些大数据;"
		"2.创建索引再插入另外一些大数据;"
		"3.插入另外一些大数值并回滚;"
		"4. 构建查询查检结果是否正确");
    try
	{
		Oid tablespaceId = OIDGenerator::instance().GetTableSpaceID();
		Oid relId = OIDGenerator::instance().GetHeapID();
		Oid idxId = OIDGenerator::instance().GetIndexID();

		std::set<std::string> sResult;
		std::vector<std::string> vInserted;
        PrepareDBAndIndex(tablespaceId,relId,idxId,sResult,vInserted);

		begin_transaction();
		{
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split,false,false);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//open the index
			static std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			vecKeys[2] = str_compare;
			SimpleIndex index(heap,idxId,vecKeys,false,false);

			std::string strData1,strData2;
			strData1 += "101784";
			strData2 += "182329";
			RandomGenString(strData1,1<<18);
			RandomGenString(strData2,1<<9);

			heap.Insert(strData1);
			heap.Insert(strData2);
		}
		user_abort_transaction();

		begin_transaction();
		{
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split,false,true);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//open the index
			static std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			vecKeys[2] = str_compare;
			SimpleIndex index(heap,idxId,vecKeys,false,true);


			//construct a search
			SearchCondition searchCondition;
			searchCondition.Add(1,LessThan,"123",str_compare);
			searchCondition.Add(2,GreaterThan,"45",str_compare);

			//check the result
			std::set<std::string>& setResult = index.Find(searchCondition);
			bool bResult = setResult == sResult;
			CHECK_BOOL(bResult);
			if (!bResult)
			{
				OutPutToastError(sResult,setResult);
			}
		}
		commit_transaction();
		return true;

	}
	CATCHEXCEPTION
}

bool test_toastIndex_UpdateRollback(void)
{
	INTENT("1.创建heap并插入一些大数据;"
		   "2.创建索引再插入另外一些大数据;"
		   "3.更新heap中的值并回滚;"
		   "4. 构建查询查检结果是否正确");
	try
	{
		Oid tablespaceId = OIDGenerator::instance().GetTableSpaceID();
		Oid relId = OIDGenerator::instance().GetHeapID();
		Oid idxId = OIDGenerator::instance().GetIndexID();

		std::set<std::string> sResult;
		std::vector<std::string> vInserted;
		PrepareDBAndIndex(tablespaceId,relId,idxId,sResult,vInserted);

		begin_transaction();
		{
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split,false,false);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//open the index
			static std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			vecKeys[2] = str_compare;
			SimpleIndex index(heap,idxId,vecKeys,false,false);

			std::string strUpdateData1;
			std::string strUpdateData2;
			std::string strUpdateData3;
			strUpdateData1 += "102830";
			strUpdateData2 +=	"125239";
			strUpdateData3 +=	"138735";

			RandomGenString(strUpdateData1,1<<9);  
			RandomGenString(strUpdateData2,1<<25); 
			RandomGenString(strUpdateData3,1<<13); 

			heap.Update(vInserted[0],strUpdateData1);
			heap.Update(vInserted[1],strUpdateData2);
			heap.Update(vInserted[3],strUpdateData3);
		}
		user_abort_transaction();

		begin_transaction();
		{
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split,false,true);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//open the index
			static std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			vecKeys[2] = str_compare;
			SimpleIndex index(heap,idxId,vecKeys,false,true);


			//construct a search
			SearchCondition searchCondition;
			searchCondition.Add(1,LessThan,"123",str_compare);
			searchCondition.Add(2,GreaterThan,"45",str_compare);

			//check the result
			std::set<std::string> setResult = index.Find(searchCondition);
			bool bResult = setResult == sResult;
			CHECK_BOOL(bResult);
			if (!bResult)
			{
				OutPutToastError(sResult,setResult);
			}
		}
		commit_transaction();
		return true;

	}
	CATCHEXCEPTION
}

bool test_toastIndex_DeleteRollback(void)
{
	INTENT("1.创建heap并插入一些大数据;"
		"2.创建索引再插入另外一些大数据;"
		"3.删除heap中的一些值并回滚;"
		"4. 构建查询查检结果是否正确");
	try
	{
		Oid tablespaceId = OIDGenerator::instance().GetTableSpaceID();
		Oid relId = OIDGenerator::instance().GetHeapID();
		Oid idxId = OIDGenerator::instance().GetIndexID();

		std::set<std::string> sResult;
		std::vector<std::string> vInserted;
		PrepareDBAndIndex(tablespaceId,relId,idxId,sResult,vInserted);

		begin_transaction();
		{
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split,false,false);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//open the index
			static std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			vecKeys[2] = str_compare;
			SimpleIndex index(heap,idxId,vecKeys,false,false);

			heap.Delete(vInserted[0]);
			heap.Delete(vInserted[2]);
		}
		user_abort_transaction();

		begin_transaction();
		{
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split,false,true);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//open the index
			static std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			vecKeys[2] = str_compare;
			SimpleIndex index(heap,idxId,vecKeys,false,true);


			//construct a search
			SearchCondition searchCondition;
			searchCondition.Add(1,LessThan,"123",str_compare);
			searchCondition.Add(2,GreaterThan,"45",str_compare);

			//check the result
			std::set<std::string> setResult = index.Find(searchCondition);
			bool bResult = setResult == sResult;
			CHECK_BOOL(bResult);
			if (!bResult)
			{
				OutPutToastError(sResult,setResult);
			}
		}
		commit_transaction();
		return true;

	}
	CATCHEXCEPTION
}
