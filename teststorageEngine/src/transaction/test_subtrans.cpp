/**
* @file test_subtrans.cpp
* @brief 
* @author ������
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
	INTENT("����������:\n"
		"1. ����һ������,����һ����������һЩ����;\n"
		"2. ����һ������������в�������һЩ����;\n"
		"3. �ύ������󣬼���Ƿ����ɹ�.\n");
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
	INTENT("����������:\n"
		"1. ����һ������,����һ����������һЩ����;\n"
		"2. ����һ������������в�������һЩ����;\n"
		"3. �ع�.\n"
		"3. ����������ԭ����.\n");
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
	INTENT("����������:\n"
		"1. ����һ������,����һ����������һЩ����;\n"
		"2. ����һ������������в�������һЩ���ݺ��ύ;\n"
		"3. ������һ���������ٲ���һЩ����\n"
		"4. �ύ������󣬼���Ƿ����ɹ�.\n");
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
	INTENT("����������:\n"
		"1. ����һ������,����һ����������һЩ����;\n"
		"2. ����һ������������в�������һЩ���ݺ��ύ;\n"
		"3. ������һ���������ٲ���һЩ���ݺ�ع�\n"
		"4. �ύ������󣬼���Ƿ����ɹ�.\n");
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
	INTENT("����������:\n"
		"1. ����һ������,����һ����������һЩ����;\n"
		"2. ����һ��������1������в�������һЩ����\n"
		"3. ������һ��������2���ٲ���һЩ����\n"
		"4. �ύ������2\n"
		"5. �ύ������1\n"
		"6. ����Ƿ����ɹ�\n");
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
	INTENT("����������:\n"
		"1. ����һ������,����һ����������һЩ����;\n"
		"2. ����һ��������1������в�������һЩ����\n"
		"3. ������һ��������2���ٲ���һЩ���ݺ�ع�\n"
		"4. �ύ������1\n"
		"5. ����Ƿ����ɹ�\n");
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