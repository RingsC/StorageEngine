#include <boost/assign.hpp>
#include <iostream>
#include "utils/util.h"
#include "access/xact.h"
#include "test_fram.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "index/test_index_insert.h"
#include  "transaction/test_transactionWithIndex.h"

using namespace FounderXDB::StorageEngineNS;
Oid idTableSpace = OIDGenerator::instance().GetTableSpaceID();
Oid  idHeap = OIDGenerator::instance().GetHeapID();
Oid  idIndex = OIDGenerator::instance().GetIndexID();
std::set<std::string> setData;
  //construct sacn condition
#define CONSTRUCT_SEARCH_CONDITION SearchCondition scanCond;\
  scanCond.Add(1,LessThan,"6",str_compare);

void  testTransWidthIndexSetUp(void)
{
	INTENT("创建数据库，并插入一些数据，然后在其上创建索引，为下面的增删改作铺垫.")
	try
	{
		begin_first_transaction();
		{
			//create heap
			SimpleHeap heap(idTableSpace,idHeap,FixSpliter::split,true,false);
			//open the db
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("2");
			heap.Insert("6");
			heap.Insert("9");
			CommandCounterIncrement();


			//create a index
			std::map<int,CompareCallback> vecKeys;
			insert(vecKeys)(1,str_compare);
			SimpleIndex index(heap,idIndex,vecKeys,true,false);
			heap.Insert("1");
			CommandCounterIncrement();

            CONSTRUCT_SEARCH_CONDITION

			std::set<std::string> sNeedFind;
			sNeedFind += "1","2";
			CHECK_BOOL(sNeedFind == index.Find(scanCond));

			setData += "1","2","6","9";
		}
		commit_transaction();
	}
	CATCHNORETURN
}

//open the db
#define OPEN_DB_AND_INDEX SimpleHeap heap(idTableSpace,idHeap,FixSpliter::split,false,false); \
heap.Open(RowExclusiveLock);\
std::map<int,CompareCallback> vecKeys;\
insert(vecKeys)(1,str_compare);\
SimpleIndex index(heap,idIndex,vecKeys,false,false);

bool  testTransWidthIndexInsert(void)
{
	INTENT("插入一些数据后回滚，查看index是否同步地回滚")
    try
	{
		using namespace boost::assign;

		begin_transaction();
		{
			OPEN_DB_AND_INDEX

            heap.Insert("3");
			heap.Insert("4");

            CommandCounterIncrement();
		}
		user_abort_transaction();

        begin_transaction();
		{
			OPEN_DB_AND_INDEX
			CONSTRUCT_SEARCH_CONDITION

			CHECK_BOOL(setData == heap.GetAll());

			std::set<std::string> sNeedFind;
			sNeedFind += "1","2";

			CHECK_BOOL(sNeedFind == index.Find(scanCond));
		}
		commit_transaction();

	}
	CATCHEXCEPTION

	return true;
}

bool  testTransWidthIndexUpdate(void)
{
	INTENT("修改一些数据后回滚，查看index是否同步地回滚")
	try
	{
		using namespace boost::assign;
		begin_transaction();
		{
            OPEN_DB_AND_INDEX

			heap.Update("2","8");
			heap.Update("9","3");
		}
		user_abort_transaction();

		begin_transaction();
		{
			//open the db and index
			OPEN_DB_AND_INDEX
            CONSTRUCT_SEARCH_CONDITION
	
			CHECK_BOOL(setData == heap.GetAll());

			std::set<std::string> sNeedFind;
			sNeedFind += "1","2";
			CHECK_BOOL(sNeedFind == index.Find(scanCond));
		}
		commit_transaction();
	}
	CATCHEXCEPTION

	return true;
}

bool  testTransWidthIndexDelete(void)
{
	INTENT("删除一些数据后回滚，查看index是否同步地回滚")
	try
	{
		using namespace boost::assign;
		begin_transaction();
		{
			//open the db
			OPEN_DB_AND_INDEX

			heap.Delete("2");
			heap.Delete("9");
		}
		user_abort_transaction();

		begin_transaction();
		{
			//open the db and index
			OPEN_DB_AND_INDEX
			CONSTRUCT_SEARCH_CONDITION

			CHECK_BOOL(setData == heap.GetAll());

			std::set<std::string> sNeedFind;
			sNeedFind += "1","2";
			CHECK_BOOL(sNeedFind == index.Find(scanCond));
		}
		commit_transaction();

	}
	CATCHEXCEPTION

	return true;
}

void testTransWidthIndexClearUp(void)
{
	begin_transaction();
	FDPG_Heap::fd_heap_drop(idHeap, MyDatabaseId);
	commit_transaction();
}