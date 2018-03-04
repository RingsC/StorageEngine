/**
* @file test_toast_index.cpp
* @brief TestIndexWithToast的实现文件
* @author 李书淦
* @date 2011-9-15 13:34:14
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <iostream>
#include <boost/assign.hpp>
#include "index/test_toast_index.h"
#include "test_fram.h"
#include "access/xact.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"

using namespace std;
static std::map<int,CompareCallback>& GetIndexKeys(SimpleHeap& heap)
{
	//open the db
	using namespace boost::assign;
	std::vector<int> v ;
	v += 3,2;
	FixSpliter spliter(v);
	heap.Open(RowExclusiveLock);

	FDPG_Transaction::fd_CommandCounterIncrement();

	static std::map<int,CompareCallback> vecKeys;
	vecKeys[1] = str_compare;
	vecKeys[2] = str_compare;
	return vecKeys;
}

TestIndexWithToast* TestIndexWithToast::g_pInstance = NULL;
TestIndexWithToast* TestIndexWithToast::g_pTempInstance = NULL;

TestIndexWithToast::TestIndexWithToast(bool temp = false):
m_heap(OIDGenerator::instance().GetTableSpaceID(),test_toast_get_heap_id(temp),FixSpliter::split,true)
,m_index(m_heap,test_toast_get_index_id(temp),GetIndexKeys(m_heap))
{
}

void TestIndexWithToast::setUp(void)
{
	INTENT("准备测试所需要的数据");

	using namespace boost::assign;
	//生成用以插入的数据
	m_strInsertData1 += "123456";
	m_strInsertData2 +=	"123789";
	m_strInsertData3 +=	"012562";
	m_strInsertData4 +=	"012120";
	m_strInsertData5 += "012856";
	RandomGenString(m_strInsertData1,1<<20);
	RandomGenString(m_strInsertData2,1<<8);
	RandomGenString(m_strInsertData3,1<<24);
	RandomGenString(m_strInsertData4,1<<17);
	RandomGenString(m_strInsertData5,1<<19);
	m_setInsert += m_strInsertData3,m_strInsertData5;
	m_stackSort += m_strInsertData5,m_strInsertData3,m_strInsertData4;

	//generate the data used to update
	m_strUpdateData1 += "102830";
	m_strUpdateData2 +=	"125239";
	m_strUpdateData3 +=	"138735";

	RandomGenString(m_strUpdateData1,1<<8);   //m_strInsertData1 update to m_strUpdateData1
	RandomGenString(m_strUpdateData2,1<<25);  //m_strInsertData2 update to m_strUpdateData2
	RandomGenString(m_strUpdateData3,1<<13);  //m_strInsertData4 update to m_strUpdateData3
	m_setAfterUpdate += m_strUpdateData1,m_strInsertData3,m_strInsertData5;

	//generate the data set after delete: delete  m_strUpdateData1 m_strUpdateData2 m_strUpdateData3
	m_setAfterDelete += m_strInsertData3,m_strInsertData5;




}

bool TestIndexWithToast::testInsert(void)
{
	INTENT("1. 向数据库中插入一些数据"
		   "2. 构造一个查询;"
		   "3. 检查实际结果与期望值是否相等");
	try
	{
		//insert some data to the db
		m_heap.Insert(m_strInsertData1);
		m_heap.Insert(m_strInsertData2);
		m_heap.Insert(m_strInsertData3);
		m_heap.Insert(m_strInsertData4);
		m_heap.Insert(m_strInsertData5);
		FDPG_Transaction::fd_CommandCounterIncrement();

		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,LessThan,"123",str_compare);
		searchCondition.Add(2,GreaterThan,"45",str_compare);

		//check the result
		std::set<std::string>& setResult = m_index.Find(searchCondition);
		bool bResult = setResult == m_setInsert;
		CHECK_BOOL(bResult);
		if(!bResult)
		{
			OutPutToastError(m_setInsert,setResult);
		}
	}
	CATCHEXCEPTION

	return true;
}

bool TestIndexWithToast::testMultiOrder(void)
{
	INTENT("1. 构造一个有序的查询,等于某个第一关键字;"
		   "2. 查询结果;"
		   "3. 检查结果是否是按照第二关键字排序")
    try
    {
		//construct a search
		SearchCondition searchCondition;
		searchCondition.Add(1,Equal,"012",str_compare);

		std::vector<std::string> vResult = m_index.Find(searchCondition,1);
		int nCount = 0;
		for (std::vector<std::string>::iterator it = vResult.begin();
			it != vResult.end();
			++it)
		{
			nCount++;
			string str = m_stackSort.top();
			m_stackSort.pop();
			CHECK_BOOL(str == *it);
		}

		CHECK_BOOL(nCount == vResult.size());
    }
    CATCHEXCEPTION
	return true;
}

bool TestIndexWithToast::testUpdate(void)
{
	INTENT("1. 将符合条件的项更新为不合条件的项;"
		   "2. 将不符合条件的项更新为符合条件的项;"
		   "3. 构造一个查询，检查结果是否也更新了")
	try
	{	
		//begin_transaction();
		{
			//update the data
			m_heap.Update(m_strInsertData1,m_strUpdateData1);
			m_heap.Update(m_strInsertData2,m_strUpdateData2);
			m_heap.Update(m_strInsertData4,m_strUpdateData3);
			FDPG_Transaction::fd_CommandCounterIncrement();

			//construct a search
			SearchCondition searchCondition;
			searchCondition.Add(1,LessThan,"123",str_compare);
			searchCondition.Add(2,GreaterThan,"45",str_compare);

			//check the result
			std::set<std::string>& setResult = m_index.Find(searchCondition);
			bool bResult = setResult == m_setAfterUpdate;
			CHECK_BOOL(bResult);
			if (!bResult)
			{
				OutPutToastError(m_setAfterUpdate,setResult);
			}

		}
		//commit_transaction();
		return true;
	}
	CATCHEXCEPTION
	return true;
}

bool TestIndexWithToast::testDelete(void)
{
	INTENT("1. 删除一些满足条件的数据"
		   "2. 删除一些不满足条件的数据;"
		   "3. 构造一个查询;"
		   "4. 检查删除是否正确地反映在结果之中")
	try
	{
		//begin_transaction();
		{
			//update the data
			m_heap.Delete(m_strUpdateData1);
			m_heap.Delete(m_strUpdateData2);
			m_heap.Delete(m_strUpdateData3);
			FDPG_Transaction::fd_CommandCounterIncrement();

			//construct a search
			SearchCondition searchCondition;
			searchCondition.Add(1,LessThan,"123",str_compare);
			searchCondition.Add(2,GreaterThan,"45",str_compare);

			//check the result
			std::set<std::string>& setResult = m_index.Find(searchCondition);
			bool bResult = setResult == m_setAfterDelete;
			CHECK_BOOL(bResult);
			if (!bResult)
			{
				OutPutToastError(m_setAfterDelete,setResult);
			}
		}
		//commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}

