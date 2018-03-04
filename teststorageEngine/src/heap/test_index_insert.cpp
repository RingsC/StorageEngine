#include <boost/assign.hpp>
#include <iostream>
#include <vector>
#include <stack>
#include "utils/util.h"
#include "access/xact.h"
#include "test_fram.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "index/test_index_insert.h"
using namespace FounderXDB::StorageEngineNS;
/* 1)
*/
bool testIndexInsert_SingleColumn( void )
{
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
			CommandCounterIncrement();

			//create a index
			std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			SimpleIndex index(heap,indid,vecKeys);

			//insert more data
			heap.Insert("5");
			heap.Insert("6");
			heap.Insert("7");
			CommandCounterIncrement();

			//construct a search
			SearchCondition searchCondition;
			searchCondition.Add(1,LessEqual,"7",str_compare);

			std::set<std::string> sNeedFind;
			sNeedFind += "1","2","3","5","6","7";
			CHECK_BOOL(sNeedFind == index.Find(searchCondition));
		}
		commit_transaction();
	}
	CATCHEXCEPTION

	return true;
}

bool testIndexInsert_SameScakKey( void )
{
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
			CommandCounterIncrement();

			//create a index
			std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			SimpleIndex index(heap,indid,vecKeys);

			//insert more data
			heap.Insert("5");
			heap.Insert("6");
			heap.Insert("7");
			CommandCounterIncrement();

			//construct a search
			SearchCondition searchCondition;
			searchCondition.Add(1,LessEqual,"7",str_compare);
			searchCondition.Add(1,LessEqual,"8",str_compare);

			std::set<std::string> sNeedFind;
			sNeedFind += "1","2","3","5","6","7";
			CHECK_BOOL(sNeedFind == index.Find(searchCondition));
		}
		commit_transaction();
	}
	CATCHEXCEPTION

		return true;
}

bool testIndex_Sort(void)
{
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
			v += 3,2,1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);

			//insert some data in the db
			heap.Insert("123456");
			heap.Insert("123789");
			heap.Insert("012562");
			heap.Insert("012120");
			heap.Insert("012456");

			//create a index
			std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			vecKeys[2] = str_compare;
			SimpleIndex index(heap,indid,vecKeys);


			//construct a search
			SearchCondition searchCondition;
			searchCondition.Add(1,Equal,"012",str_compare);

			std::stack<std::string> sNeedFind;
			sNeedFind += "012562","012456","012120";
			
			std::vector<std::string> &vR = index.Find(searchCondition,1);

			int nCount = 0;
			for (std::vector<std::string>::iterator it = vR.begin();
                 it != vR.end();
				 ++it)
			{
				nCount++;
                string str = sNeedFind.top();
				sNeedFind.pop();
				CHECK_BOOL(str == *it);
			}

			CHECK_BOOL(nCount == vR.size());
		}
		commit_transaction();
	}
	CATCHEXCEPTION
	return true;
}

bool testIndex_InAnotherTrans( void )
{
	Oid reltablespace = DEFAULTTABLESPACE_OID;
	Oid relid = OIDGenerator::instance().GetHeapID();
	Oid indid = OIDGenerator::instance().GetIndexID();

	try
	{
		begin_first_transaction();
		{
			//create heap 
			SimpleHeap heap(reltablespace,relid,FixSpliter::split,true,false);

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
			CommandCounterIncrement();

			//create a index
			std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			SimpleIndex index(heap,indid,vecKeys,true,false);

			//insert more data
			heap.Insert("8");
			heap.Insert("6");
			heap.Insert("7");
			CommandCounterIncrement();
		}
		commit_transaction();
	}
	CATCHEXCEPTION


	try
	{
		begin_transaction();

		{
			//open the heap
			SimpleHeap heap(reltablespace,relid,FixSpliter::split);
			using namespace boost::assign;
			std::vector<int> v;
			v += 1;
			FixSpliter split(v);
			heap.Open(RowExclusiveLock);


			std::map<int,CompareCallback> vecKeys;
			vecKeys[1] = str_compare;
			SimpleIndex index(heap,indid,vecKeys,false);

			//construct a search
			//construct sacn condition
			SearchCondition searchCondition;
			searchCondition.Add(1,LessEqual,"7",str_compare);

			//open the index
			std::set<std::string> sNeedFind;
			sNeedFind += "1","2","3","6","7";
			CHECK_BOOL(sNeedFind == index.Find(searchCondition));
		}
		commit_transaction();
	}
	CATCHEXCEPTION

	return true;
}