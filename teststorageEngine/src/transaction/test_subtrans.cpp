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
#include <map>
#include <vector>
#include <set>
#include <boost/assign.hpp>
#include "utils/util.h"
#include "test_fram.h"
#include "interface/FDPGAdapter.h"
#include "utils/toast_test_utils.h"
#include "transaction/test_subtrans.h"
bool TestSubTransaction( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务，向表中插入另外一些数据;\n"
		"3. 提交子事务后，检测是否插入成功.\n");
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	try
	{
		begin_first_transaction();
		{
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("1");
			heap.Insert("2");
			heap.Insert("3");
			FDPG_Transaction::fd_CommandCounterIncrement();

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			CHECK_BOOL(heap.GetAll() == sDesired);

			begin_subtransaction();
			{
				heap.Insert("8");
				heap.Insert("9");
				FDPG_Transaction::fd_CommandCounterIncrement();
			}
			commit_subtransaction();
            sDesired += "8","9";
			CHECK_BOOL(heap.GetAll() == sDesired);
		}
		commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}

bool TestSubTransactionAbort( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务，向表中插入另外一些数据;\n"
		"3. 回滚.\n"
		"3. 检测子事务的原子性.\n");
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	try
	{
		begin_first_transaction();
		{
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("1");
			heap.Insert("2");
			heap.Insert("3");
			FDPG_Transaction::fd_CommandCounterIncrement();

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			CHECK_BOOL(heap.GetAll() == sDesired);

			begin_subtransaction();
			{
				heap.Insert("8");
				heap.Insert("9");
				FDPG_Transaction::fd_CommandCounterIncrement();
			}
			abort_subtransaction();
			CHECK_BOOL(heap.GetAll() == sDesired);

		}
		commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}

bool TestMultiSubTransaction( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务，向表中插入另外一些数据后提交;\n"
		"3. 启动另一个子事务，再插入一些数据\n"
		"4. 提交子事务后，检测是否插入成功.\n");
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	try
	{
		begin_first_transaction();
		{
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("1");
			heap.Insert("2");
			heap.Insert("3");
			FDPG_Transaction::fd_CommandCounterIncrement();

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			CHECK_BOOL(heap.GetAll() == sDesired);

			begin_subtransaction();
			{
				heap.Insert("8");
				heap.Insert("9");
				FDPG_Transaction::fd_CommandCounterIncrement();
			}
			commit_subtransaction();
			sDesired += "8","9";
			CHECK_BOOL(heap.GetAll() == sDesired);

			begin_subtransaction();
			{
				heap.Insert("4");
				heap.Insert("6");
				FDPG_Transaction::fd_CommandCounterIncrement();
			}
			commit_subtransaction();

			sDesired += "4","6";
			CHECK_BOOL(heap.GetAll() == sDesired);

		}
		commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}

bool TestMultiSubTransactionAbort( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务，向表中插入另外一些数据后提交;\n"
		"3. 启动另一个子事务，再插入一些数据后回滚\n"
		"4. 提交子事务后，检测是否插入成功.\n");
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	try
	{
		begin_first_transaction();
		{
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("1");
			heap.Insert("2");
			heap.Insert("3");
			FDPG_Transaction::fd_CommandCounterIncrement();

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			CHECK_BOOL(heap.GetAll() == sDesired);

			begin_subtransaction();
			{
				heap.Insert("8");
				heap.Insert("9");
				FDPG_Transaction::fd_CommandCounterIncrement();
			}
			commit_subtransaction();
			sDesired += "8","9";
			CHECK_BOOL(heap.GetAll() == sDesired);

			begin_subtransaction();
			{
				heap.Insert("4");
				heap.Insert("6");
				FDPG_Transaction::fd_CommandCounterIncrement();
			}
			abort_subtransaction();

			CHECK_BOOL(heap.GetAll() == sDesired);

		}
		commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}

bool TestMultiLevelTransaction( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务1，向表中插入另外一些数据\n"
		"3. 启动另一个子事务2，再插入一些数据\n"
		"4. 提交子事务2\n"
		"5. 提交子事务1\n"
		"6. 检测是否插入成功\n");
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	try
	{
		begin_first_transaction();
		{
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("1");
			heap.Insert("2");
			heap.Insert("3");
			FDPG_Transaction::fd_CommandCounterIncrement();

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			CHECK_BOOL(heap.GetAll() == sDesired);

			begin_subtransaction();
			{
				heap.Insert("8");
				heap.Insert("9");
				FDPG_Transaction::fd_CommandCounterIncrement();
				sDesired += "8","9";
				CHECK_BOOL(heap.GetAll() == sDesired);

				begin_subtransaction();
				{
					heap.Insert("4");
					heap.Insert("6");
					FDPG_Transaction::fd_CommandCounterIncrement();
				}
				commit_subtransaction();

			}
			commit_subtransaction();

			sDesired += "4","6";
			CHECK_BOOL(heap.GetAll() == sDesired);

		}
		commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}

bool TestMultiLevelTransactionAbort( void )
{
	INTENT("测试子事务:\n"
		"1. 启动一个事务,创建一个表，并插入一些数据;\n"
		"2. 启动一个子事务1，向表中插入另外一些数据\n"
		"3. 启动另一个子事务2，再插入一些数据后回滚\n"
		"4. 提交子事务1\n"
		"5. 检测是否插入成功\n");
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	try
	{
		begin_first_transaction();
		{
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true);

			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("1");
			heap.Insert("2");
			heap.Insert("3");
			FDPG_Transaction::fd_CommandCounterIncrement();

			std::set<std::string> sDesired;
			sDesired += "1","2","3";
			CHECK_BOOL(heap.GetAll() == sDesired);

			begin_subtransaction();
			{
				heap.Insert("8");
				heap.Insert("9");
				FDPG_Transaction::fd_CommandCounterIncrement();
				sDesired += "8","9";
				CHECK_BOOL(heap.GetAll() == sDesired);

				begin_subtransaction();
				{
					heap.Insert("4");
					heap.Insert("6");
					FDPG_Transaction::fd_CommandCounterIncrement();
				}
				abort_subtransaction();

			}
			commit_subtransaction();

			CHECK_BOOL(heap.GetAll() == sDesired);

		}
		commit_transaction();
		return true;
	}
	CATCHEXCEPTION
}