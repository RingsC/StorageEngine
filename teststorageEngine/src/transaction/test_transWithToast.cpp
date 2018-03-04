/**
* @file test_transWithToast.cpp
* @brief 
* @author 李书淦
* @date 2011-9-20 14:13:52
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <vector>
#include <string>
#include <set>
#include <boost/assign.hpp>
#include "transaction/test_transactionWithToast.h"
#include "utils/util.h"
#include "utils/toast_test_utils.h"
#include "test_fram.h"
#include "interface/FDPGAdapter.h"

/************************************************************************** 
* @brief InsertData2Heap 
* Help function 向heap中插入一些随机生成的大数据值，并返回插入值的集合
* Detailed description.
* @param[in] heap  数据将会插入到其中
* @param[out] sInserted  插入数据的集合
**************************************************************************/
static void InsertData2Heap(SimpleHeap& heap,std::set<std::string>& sInserted)
{
	std::string strData;
	RandomGenString(strData,1<<20);
	heap.Insert(strData);
	sInserted.insert(strData);

	strData.clear();
	RandomGenString(strData,1<<23);
	heap.Insert(strData);
	sInserted.insert(strData);

	strData.clear();
	RandomGenString(strData,1<<17);
	heap.Insert(strData);
	sInserted.insert(strData);


	strData.clear();
	RandomGenString(strData,1<<11);
	heap.Insert(strData);
	sInserted.insert(strData);
}


/************************************************************************** 
* @brief PrepareDB 
* 创建heap,并向其中插入一些大数据
* Detailed description.
* @param[in] tablespaceId  heap的tablespace  Id
* @param[in] relId  heap的id
* @param[out] sInserted 插入数据的集合
**************************************************************************/
static void PrepareDB(Oid tablespaceId,Oid relId,std::set<std::string>& sInserted)
{
	begin_first_transaction();
	{
		SimpleHeap heap(tablespaceId,relId,FixSpliter::split,true,false);
		//open the db
		using namespace boost::assign;
		std::vector<int> v;
		v += 1;
		FixSpliter split(v);
		heap.Open(RowExclusiveLock);

		//insert some toast data
		InsertData2Heap(heap,sInserted);
		FDPG_Transaction::fd_CommandCounterIncrement();
		CHECK_BOOL(sInserted == heap.GetAll());	
	}
	commit_transaction();
}

bool testToastTrans_InsertRollback( void )
{
	INTENT("1. 向表中插入一些数据(包含有大数据);"
		   "2. 提交事务;"
		   "3. 再插入其他一些数据(包含大数据);"
		   "4. 回滚;"
		   "5  检查表中的数据.");
    try
    {
		Oid tablespaceId = OIDGenerator::instance().GetTableSpaceID();
		Oid relId = OIDGenerator::instance().GetHeapID();
		std::set<std::string> sInserted;
        PrepareDB(tablespaceId,relId,sInserted);

		begin_transaction();			
		{		
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split,false,false);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);
			//Insert some other data
			std::set<std::string> sInserted1;
			InsertData2Heap(heap,sInserted1);
		}
		user_abort_transaction();

		begin_transaction();			
		{		
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);
			//Insert some other data
			std::set<std::string>& sResult = heap.GetAll();
			bool bResult = sInserted == heap.GetAll();
			CHECK_BOOL(bResult);
			if (!bResult)
			{
				OutPutToastError(sInserted,sResult);
			}
		}
		commit_transaction();
		return true;

    }
    CATCHEXCEPTION
}

bool testToastTrans_UpdateRollback( void )
{
	INTENT("1. 向表中插入一些数据(包含有大数据);"
		"2. 提交事务;"
		"3. Update;"
		"4. 回滚"
		"5. 检查表中的数据");
	try
	{
		Oid tablespaceId = OIDGenerator::instance().GetTableSpaceID();
		Oid relId = OIDGenerator::instance().GetHeapID();
		std::set<std::string> sInserted;
		PrepareDB(tablespaceId,relId,sInserted);

		begin_transaction();			
		{		
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split,false,false);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);
			

			//update 
			std::set<std::string> sInserted1;
			std::string strUpdateData1;   
			std::string strUpdateData2;
			std::string strUpdateData3;

			std::vector<std::string> vInserted(sInserted.begin(),sInserted.end());
			heap.Update(vInserted[0],strUpdateData1);
			heap.Update(vInserted[2],strUpdateData2);
			heap.Update(vInserted[1],strUpdateData3);
		}
		user_abort_transaction();

		begin_transaction();			
		{		
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);
			//Insert some other data
			std::set<std::string>& sResult = heap.GetAll();
			bool bResult = sInserted == heap.GetAll();
			CHECK_BOOL(bResult);
			if (!bResult)
			{
				OutPutToastError(sInserted,sResult);
			}
		}
		commit_transaction();
		return true;

	}
	CATCHEXCEPTION
}

bool testToastTrans_DeleteRollback( void )
{
	INTENT("1. 向表中插入一些数据(包含有大数据);"
		"2. 提交事务;"
		"3. 删除一些数据;"
		"4. 回滚;"
		"5  检查表中的数据.");
	try
	{
		Oid tablespaceId = OIDGenerator::instance().GetTableSpaceID();
		Oid relId = OIDGenerator::instance().GetHeapID();
		std::set<std::string> sInserted;
		PrepareDB(tablespaceId,relId,sInserted);

		begin_transaction();			
		{		
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split,false,false);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);
			

			//delete some data
			std::vector<std::string> vInserted(sInserted.begin(),sInserted.end());
			heap.Delete(vInserted[0]);
			heap.Delete(vInserted[2]);
		}
		user_abort_transaction();

		begin_transaction();			
		{		
			SimpleHeap heap(tablespaceId,relId,FixSpliter::split);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);
			//Insert some other data
			std::set<std::string>& sResult = heap.GetAll();
			bool bResult = sInserted == heap.GetAll();
			CHECK_BOOL(bResult);
			if (!bResult)
			{
				OutPutToastError(sInserted,sResult);
			}
		}
		commit_transaction();
		return true;

	}
	CATCHEXCEPTION
}

