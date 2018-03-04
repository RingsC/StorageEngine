/**************************************************************************
* @file test_toast.cpp
* @brief TestToast的实现文件
* @author 李书淦
* @date 2011-9-15 14:11:06
* @version 1.0
**************************************************************************/
#include <boost/assign.hpp>
#include <vector>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <fstream>
#include "utils/util.h"
#include "access/xact.h"
#include "test_fram.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "index/test_index_insert.h"
#include  "transaction/test_transactionWithIndex.h"
#include "index/test_index_update.h"
#include "heap/test_toast.h"





using namespace FounderXDB::StorageEngineNS;
TestToast* TestToast::g_pInstance = NULL;
TestToast* TestToast::g_pTempInstance = NULL;

TestToast::TestToast(bool temp = false):
m_heap(OIDGenerator::instance().GetTableSpaceID(),test_toast_get_heap_id(temp),FixSpliter::split,true)
{
}

void TestToast::setUp()
{
	INTENT("生成数据")

	using namespace boost::assign;
	//generate the data for insert
    RandomGenString(m_strInsertData1,1<<20);
	RandomGenString(m_strInsertData2,1<<8);
	RandomGenString(m_strInsertData3,1<<24);
	RandomGenString(m_strInsertData4,1<<17);
	RandomGenString(m_strInsertData5,1<<7);
	m_setInsert += m_strInsertData1,m_strInsertData2,m_strInsertData3,m_strInsertData4,m_strInsertData5;

	//generate the data used to update
	RandomGenString(m_strUpdateData1,1<<9);   //m_strInsertData1 update to m_strUpdateData1
	RandomGenString(m_strUpdateData2,1<<25);  //m_strInsertData2 update to m_strUpdateData2
	RandomGenString(m_strUpdateData3,1<<13);  //m_strInsertData4 update to m_strUpdateData3
	m_setAfterUpdate += m_strUpdateData1,m_strUpdateData2,m_strInsertData3,m_strUpdateData3,m_strInsertData5;

	//generate the data set after delete: delete  m_strUpdateData1 m_strUpdateData2 m_strUpdateData3
	m_setAfterDelete += m_strInsertData3,m_strInsertData5;


	//open the db
	std::vector<int> v ;
	v += 1;
	FixSpliter spliter(v);
	m_heap.Open(RowExclusiveLock);

	CommandCounterIncrement();
}


bool TestToast::testInsert()
{
	INTENT("向表中插入一些大数据，然后取出它们，比较插入的值和取出的值是否相同")
	try
	{
		//begin_transaction();
		{
			//insert some data in the db
			for (std::set<std::string>::const_iterator it = m_setInsert.begin();
				it != m_setInsert.end();
				++it)
			{
				m_heap.Insert(*it);
			}
			FDPG_Transaction::fd_CommandCounterIncrement();

			//check whether the insert operation is success or not
//			CHECK_BOOL(m_heap.GetAll() == m_setInsert);
            typedef std::set<std::string> ResultT;
            ResultT& sResult = m_heap.GetAll();
            bool result = sResult == m_setInsert;
			CHECK_BOOL(result);
            if(!result)
            {
                OutPutToastError(m_setInsert,sResult);
            }

		}
		//commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}


bool TestToast::testUpdate()
{
	INTENT("对表中的数据作一些update操作，然后取出它们，比较期望值与实际值是否相等")
	try
	{
		//begin_transaction();
		{
			//update the data
			m_heap.Update(m_strInsertData1,m_strUpdateData1);
			m_heap.Update(m_strInsertData2,m_strUpdateData2);
			m_heap.Update(m_strInsertData4,m_strUpdateData3);
			FDPG_Transaction::fd_CommandCounterIncrement();

			//check whether the insert operation is success or not
            typedef std::set<std::string> ResultT;
            ResultT& sResult = m_heap.GetAll();
            bool result = sResult == m_setAfterUpdate;
			CHECK_BOOL(result);
            if(!result)
            {
                OutPutToastError(m_setAfterUpdate,sResult);
			}
		}
		//commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}


bool TestToast::testDelete()
{
	INTENT("删除一些数据，然后检查是否真的已经删除")
	try
	{
		//begin_transaction();
		{
			//update the data
			m_heap.Delete(m_strUpdateData1);
			m_heap.Delete(m_strUpdateData2);
			m_heap.Delete(m_strUpdateData3);
			FDPG_Transaction::fd_CommandCounterIncrement();

			//check whether the insert operation is success or not
            typedef std::set<std::string> ResultT;
            ResultT& sResult = m_heap.GetAll();
            bool result = sResult == m_setAfterDelete;
			CHECK_BOOL(result);
            if(!result)
            {
                OutPutToastError(m_setAfterDelete,sResult);
            }

		}
		//commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}


