/**
* @file test_transWithToast.cpp
* @brief 
* @author ������
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
* Help function ��heap�в���һЩ������ɵĴ�����ֵ�������ز���ֵ�ļ���
* Detailed description.
* @param[in] heap  ���ݽ�����뵽����
* @param[out] sInserted  �������ݵļ���
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
* ����heap,�������в���һЩ������
* Detailed description.
* @param[in] tablespaceId  heap��tablespace  Id
* @param[in] relId  heap��id
* @param[out] sInserted �������ݵļ���
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
	INTENT("1. ����в���һЩ����(�����д�����);"
		   "2. �ύ����;"
		   "3. �ٲ�������һЩ����(����������);"
		   "4. �ع�;"
		   "5  �����е�����.");
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
	INTENT("1. ����в���һЩ����(�����д�����);"
		"2. �ύ����;"
		"3. Update;"
		"4. �ع�"
		"5. �����е�����");
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
	INTENT("1. ����в���һЩ����(�����д�����);"
		"2. �ύ����;"
		"3. ɾ��һЩ����;"
		"4. �ع�;"
		"5  �����е�����.");
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

