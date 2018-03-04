#include <iostream>
#include <boost/thread/thread.hpp>

#include "utils/util.h"
#include "interface/StorageEngineExceptionUniversal.h"

using std::cout;
using std::endl;
using namespace FounderXDB::StorageEngineNS;


void update_same_tuple_first(Oid table_space_id, Oid heap_id,
	int isolation_level, std::string old_data, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	std::string new_data("2222");

	*p_ret = false;

	cout << "************update_first start*************" << endl;

	begin_transaction();
	setXactIsoLevel(isolation_level);
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, false);
		heap.Open(AccessShareLock);
		heap.Update(old_data, new_data);
	}
	CATCHNORETURN

	cout << "after update, sleep 30s, let upadte later run" << endl;
	boost::thread::sleep(boost::get_system_time()
					+ boost::posix_time::milliseconds(30000));
	cout << "after sleep 30s, we commit first" << endl;
	commit_transaction();

	*p_ret = true;

	proc_exit(0);

	return;
}

void update_same_tuple_later(Oid table_space_id, Oid heap_id,
	int isolation_level, std::string old_data, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	std::string new_data("3333");

	*p_ret = false;

	cout << "*************update later start**************" << endl;

	begin_transaction();
	setXactIsoLevel(isolation_level);
	{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, false);
		try{
			heap.Open(AccessShareLock);
			heap.Update(old_data, new_data);
		}catch(TupleUpdateConcurrent &/*ex*/){
			cout << "update concurrently" << endl;
			std::set<std::string> all_data = heap.GetAll();
			cout << "==================Data===============" << endl;
			for(std::set<std::string>::iterator it = all_data.begin();
			it != all_data.end();++it){
				cout << "[" << *it << "]" << endl;
			}
			cout << "=====================================" << endl;
			if(all_data.find(new_data) == all_data.end()){
				*p_ret = true;
			}
		}
	}
	commit_transaction();

	proc_exit(0);

	return;
}

bool test_update_same_tuple(int isolation_level1, int isolation_level2)
{
	std::string test_data("1111");
	Oid table_space_id = OIDGenerator::instance().GetTableSpaceID();
	Oid heap_id = OIDGenerator::instance().GetHeapID();

	cout << "heap_id: " << heap_id << endl;

	cout << OIDGenerator::instance().GetHeapID() << endl;

	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, true, false);
		heap.Open(AccessShareLock);
		heap.Insert(test_data);
		
	}
	CATCHEXCEPTION
	commit_transaction();

	boost::thread_group tg;
	bool ret1;
	bool ret2;

	tg.create_thread(boost::bind(&update_same_tuple_first,
		table_space_id, heap_id, isolation_level1, test_data, &ret1));

	/* sleep 10s, let update_later thread do first*/
	boost::thread::sleep(boost::get_system_time()
					+ boost::posix_time::milliseconds(10000));

	tg.create_thread(boost::bind(&update_same_tuple_later,
		table_space_id, heap_id, isolation_level2, test_data, &ret2));

	tg.join_all();

	cout << "ret1: " << ret1 << endl;
	cout << "ret2: " << ret2 << endl;

	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, true);
		heap.Open(ExclusiveLock);
	}
	CATCHEXCEPTION
	commit_transaction();

	return (ret1 && ret2);
}

bool test_ust_readcommited_readcommited(void)
{
	return test_update_same_tuple(XACT_READ_COMMITTED, XACT_READ_COMMITTED);
}

bool test_ust_readcommited_serializable(void)
{
	return test_update_same_tuple(XACT_READ_COMMITTED, XACT_SERIALIZABLE);
}
