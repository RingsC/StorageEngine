#include <iostream>
#include <boost/thread/thread.hpp>

#include "utils/util.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/FDPGAdapter.h"

extern void FinishPreparedTransaction(const char *gid, bool isCommit);


using std::cout;
using std::endl;
using std::set;
using std::string;
using namespace FounderXDB::StorageEngineNS;

#define TEST_TRANSP_PREPARE_ID "prepare"

static void test_TransPrepare_DropHeap(Oid table_space_id, Oid heap_id, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, true);
		heap.Open(ExclusiveLock);
	}
	catch(StorageEngineExceptionUniversal &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		user_abort_transaction();
		*p_ret = false;
	}
	commit_transaction();

    *p_ret = true;
    
	proc_exit(0);
}

static bool test_TransPrepare_DropHeapInThread(Oid table_space_id, Oid heap_id)
{
   	boost::thread_group tg;
	bool ret = false;

	tg.create_thread(boost::bind(&test_TransPrepare_DropHeap, table_space_id, heap_id, &ret));

	tg.join_all();

    return ret;
}

static bool test_TransPrepare_HeapEqual(Oid table_space_id, Oid heap_id,
	set<string> data_except)
{
	bool ret;
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, false);
		heap.Open(AccessShareLock);
		set<string> data_get = heap.GetAll();
		cout << "====================Data from heap=================" << endl;
		for(set<string>::iterator it = data_get.begin(); it != data_get.end(); ++it){
			cout << "[" << *it << "]" << endl;
		}
		cout << "===================================================" << endl;
		ret = (data_except == data_get);
	}
	CATCHEXCEPTION
	commit_transaction();

	return ret;
}

static void test_TransPrepare_GetOids(Oid& table_space_id, Oid& heap_id,
	bool genNewOids)
{
	static Oid s_table_space_id = 0;
	static Oid s_heap_id = 0;

	if((s_table_space_id == 0) || (s_heap_id == 0) || (genNewOids)){
		s_table_space_id = OIDGenerator::instance().GetTableSpaceID();
		s_heap_id = OIDGenerator::instance().GetHeapID();
	}

	table_space_id = s_table_space_id;
	heap_id = s_heap_id;
}

static void test_TransPrepare_BuildInsertTestData(set<string>& test_data)
{
	test_data.insert(string("1111"));
	test_data.insert(string("abcd"));
}

bool test_TransPrepare_Insert_P1(bool isCommit)
{
	set<string> test_data;
	Oid table_space_id;
	Oid heap_id;
	
	test_TransPrepare_BuildInsertTestData(test_data);
	test_TransPrepare_GetOids(table_space_id, heap_id, true);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	/* create heap */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, true, false);
		heap.Open(AccessShareLock);
	}
	CATCHEXCEPTION
	commit_transaction();

	/* insert data */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, false);
		heap.Open(AccessShareLock);
		for(set<string>::iterator it = test_data.begin(); it != test_data.end(); ++it){
			heap.Insert(*it);
		}
	}
	CATCHEXCEPTION
	
	/* prepare */
	StartTransactionCommand();
	PrepareTransactionBlock(TEST_TRANSP_PREPARE_ID);
	CommitTransactionCommand();
	
	return true;
}

bool test_TransPrepare_Insert_P2(bool isCommit)
{
	set<string> test_data;
	Oid table_space_id;
	Oid heap_id;
	bool ret = false;

	/* commit prepare */
	cout << "start finish......" << endl;
	StartTransactionCommand();
	FinishPreparedTransaction(TEST_TRANSP_PREPARE_ID, isCommit);
	CommitTransactionCommand();
	cout << "after finish......" << endl;

	if(isCommit){
		test_TransPrepare_BuildInsertTestData(test_data);
	}
	test_TransPrepare_GetOids(table_space_id, heap_id, false);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;
	
	/* check data */
	ret = test_TransPrepare_HeapEqual(table_space_id, heap_id, test_data);

	/* drop heap */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, true);
		heap.Open(ExclusiveLock);
	}
	CATCHEXCEPTION
	commit_transaction();

	return ret;
}

bool test_TransPrepare_Insert_Commit_P1(void)
{
	return test_TransPrepare_Insert_P1(true);
}

bool test_TransPrepare_Insert_Commit_P2(void)
{
	return test_TransPrepare_Insert_P2(true);
}

bool test_TransPrepare_Insert_Abort_P1(void)
{
	return test_TransPrepare_Insert_P1(false);
}

bool test_TransPrepare_Insert_Abort_P2(void)
{
	return test_TransPrepare_Insert_P2(false);
}











/*******************************************************************************
**						test create heap
*******************************************************************************/
bool test_TransPrepare_CreateHeap_P1(bool isCommit)
{
	Oid table_space_id;
	Oid heap_id;

	test_TransPrepare_GetOids(table_space_id, heap_id, true);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	/* create heap */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, true, false);
		heap.Open(AccessShareLock);
	}
	CATCHEXCEPTION

	/* prepare */
	StartTransactionCommand();
	PrepareTransactionBlock(TEST_TRANSP_PREPARE_ID);
	CommitTransactionCommand();

	return true;
}

bool test_TransPrepare_CreateHeap_P2(bool isCommit)
{
	Oid table_space_id;
	Oid heap_id;
	bool ret = false;

	/* commit prepare */
	cout << "start finish......" << endl;
	StartTransactionCommand();
	FinishPreparedTransaction(TEST_TRANSP_PREPARE_ID, isCommit);
	CommitTransactionCommand();
	cout << "after finish......" << endl;

	test_TransPrepare_GetOids(table_space_id, heap_id, false);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	/* check heap exist */
	if(!isCommit){
		begin_transaction();
		try{
			FDPG_Heap::fd_heap_open(heap_id, AccessShareLock, MyDatabaseId);
			//SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, false);
			//heap.Open(AccessShareLock);
		}
		catch(StorageEngineException &ex)
		{
			user_abort_transaction();
			cout << ex.getErrorNo() << endl;
			cout << ex.getErrorMsg() << endl;

			if(ex.getErrorNo() == ERRCODE_UNDEFINED_TABLE){
				return true;
			}
		}

		return false;
	}else{
		/* drop heap */
		begin_transaction();
		try{
			SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, true);
			heap.Open(ExclusiveLock);
		}
		CATCHEXCEPTION
		commit_transaction();

		return true;
	}
}

bool test_TransPrepare_CreateHeap_Commit_P1(void)
{
	return test_TransPrepare_CreateHeap_P1(true);
}

bool test_TransPrepare_CreateHeap_Commit_P2(void)
{
	return test_TransPrepare_CreateHeap_P2(true);
}

bool test_TransPrepare_CreateHeap_Abort_P1(void)
{
	return test_TransPrepare_CreateHeap_P1(false);
}

bool test_TransPrepare_CreateHeap_Abort_P2(void)
{
	return test_TransPrepare_CreateHeap_P2(false);
}
















/*******************************************************************************
**						test isolation
*******************************************************************************/
void test_TransPrepare_Isolation_UpdateAfter(Oid table_space_id,
	Oid heap_id, string old_data, string new_data, bool isCommit, bool *p_ret)
{
	BackendParameters *param = (BackendParameters *)malloc(sizeof(BackendParameters));
	fxdb_save_thread_variables(param, 0, backend);
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_ret = false;

	cout << __FUNCTION__ << endl;

	bool need_commit_trans = false;
	begin_transaction();
	cout << "after begin_transaction" << endl;
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, false);
		heap.Open(AccessShareLock);
		cout << "Before update..........." << endl;
		heap.Update(old_data, new_data);
		cout << "After update..........." << endl;
		if(isCommit){
			*p_ret = true;
		}
		need_commit_trans = true;
	}
	catch(TupleUpdateConcurrent &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		user_abort_transaction();
		if(!isCommit){
			*p_ret = true;
		}
	}
	CATCHNORETURN

	if(need_commit_trans){
		commit_transaction();
	}

	proc_exit(0);
}

void test_TransPrepare_BuildIsolationTestData(bool isCommit,
	string& data_insert, string& data_update_in_prepare,
	string& data_update_after, set<string>& data_expected)
{
	data_insert.append("abcd");
	data_update_in_prepare.append("efasa");
	data_update_after.append("fsafa");

	if(isCommit){
		data_expected.insert(data_update_in_prepare);
	}else{
		data_expected.insert(data_update_after);
	}
}

bool test_TransPrepare_Isolation_P1(bool isCommit)
{
	string data_insert;
	string data_update_in_prepare;
	string data_update_after;
	set<string> data_expected;
	Oid table_space_id;
	Oid heap_id;
	
	test_TransPrepare_BuildIsolationTestData(isCommit, data_insert,
		data_update_in_prepare, data_update_after, data_expected);
	test_TransPrepare_GetOids(table_space_id, heap_id, true);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	/* create heap */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, true, false);
		heap.Open(AccessShareLock);
		heap.Insert(data_insert);
	}
	CATCHEXCEPTION
	commit_transaction();

	/* update data */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, false);
		heap.Open(AccessShareLock);
		heap.Update(data_insert, data_update_in_prepare);
	}
	CATCHEXCEPTION
	
	/* prepare */
	StartTransactionCommand();
	PrepareTransactionBlock(TEST_TRANSP_PREPARE_ID);
	CommitTransactionCommand();

	return true;
}

bool test_TransPrepare_Isolation_P2(bool isCommit)
{
	string data_insert;
	string data_update_in_prepare;
	string data_update_after;
	set<string> data_expected;
	Oid table_space_id;
	Oid heap_id;
	
	test_TransPrepare_BuildIsolationTestData(isCommit, data_insert,
		data_update_in_prepare, data_update_after, data_expected);
	test_TransPrepare_GetOids(table_space_id, heap_id, false);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	/* update in other thread */
	boost::thread_group tg;
	bool ret;

	tg.create_thread(boost::bind(&test_TransPrepare_Isolation_UpdateAfter,
		table_space_id, heap_id, data_insert, data_update_after, isCommit, &ret));

	/* let update do first */
	boost::thread::sleep(boost::get_system_time()
		+ boost::posix_time::milliseconds(2000));

	/* commit prepare */
	cout << "start finish......" << endl;
	StartTransactionCommand();
	FinishPreparedTransaction(TEST_TRANSP_PREPARE_ID, isCommit);
	CommitTransactionCommand();
	cout << "after finish......" << endl;

	tg.join_all();
	cout << "ret: " << ret << endl;

	/* check data */
	ret = test_TransPrepare_HeapEqual(table_space_id, heap_id, data_expected);

	/* drop heap */
    if(!test_TransPrepare_DropHeapInThread(table_space_id, heap_id))
    {
        cout << "drop heap failed." << endl;
        return false;
    }

	return ret;
}

bool test_TransPrepare_Isolation_Commit_P1(void)
{
	return test_TransPrepare_Isolation_P1(true);
}

bool test_TransPrepare_Isolation_Commit_P2(void)
{
	return test_TransPrepare_Isolation_P2(true);
}

bool test_TransPrepare_Isolation_Abort_P1(void)
{
	return test_TransPrepare_Isolation_P1(false);
}

bool test_TransPrepare_Isolation_Abort_P2(void)
{
	return test_TransPrepare_Isolation_P2(false);
}














/*******************************************************************************
**						test subtransation
*******************************************************************************/
void test_TransPrepare_BuildIsolationTestData(bool isCommit,
	string& data_insert, string& data_update, set<string>& data_expected)
{
	data_insert.append("abcd");
	data_update.append("efasa");

	if(isCommit){
		data_expected.insert(data_update);
	}
}

bool test_TransPrepare_SubTrans_P1(bool isCommit)
{
	string data_insert;
	string data_update;
	set<string> data_expected;
	Oid table_space_id;
	Oid heap_id;
	
	test_TransPrepare_BuildIsolationTestData(isCommit, data_insert,
		data_update, data_expected);
	test_TransPrepare_GetOids(table_space_id, heap_id, true);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	/* create heap */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, true, false);
		heap.Open(AccessShareLock);
	}
	CATCHEXCEPTION
	commit_transaction();

	/* begin first transaction */
	begin_first_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, false);
		heap.Open(AccessShareLock);
		heap.Insert(data_insert);
		FDPG_Transaction::fd_CommandCounterIncrement();

		begin_subtransaction();
		heap.Update(data_insert, data_update);
		commit_subtransaction();
	}
	CATCHEXCEPTION

	/* prepare */
	StartTransactionCommand();
	PrepareTransactionBlock(TEST_TRANSP_PREPARE_ID);
	CommitTransactionCommand();

	return true;
}

bool test_TransPrepare_SubTrans_P2(bool isCommit)
{
	string data_insert;
	string data_update;
	set<string> data_expected;
	Oid table_space_id;
	Oid heap_id;
	
	test_TransPrepare_BuildIsolationTestData(isCommit, data_insert,
		data_update, data_expected);
	test_TransPrepare_GetOids(table_space_id, heap_id, false);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	/* commit prepare */
	cout << "start finish......" << endl;
	StartTransactionCommand();
	FinishPreparedTransaction(TEST_TRANSP_PREPARE_ID, isCommit);
	CommitTransactionCommand();
	cout << "after finish......" << endl;

	/* check data */
	bool ret;
	ret = test_TransPrepare_HeapEqual(table_space_id, heap_id, data_expected);

	/* drop heap */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, true);
		heap.Open(ExclusiveLock);
	}
	CATCHEXCEPTION
	commit_transaction();

	return ret;
}

bool test_TransPrepare_SubTrans_Commit_P1(void)
{
	return test_TransPrepare_SubTrans_P1(true);
}

bool test_TransPrepare_SubTrans_Commit_P2(void)
{
	return test_TransPrepare_SubTrans_P2(true);
}

bool test_TransPrepare_SubTrans_Abort_P1(void)
{
	return test_TransPrepare_SubTrans_P1(false);
}

bool test_TransPrepare_SubTrans_Abort_P2(void)
{
	return test_TransPrepare_SubTrans_P2(false);
}












/*******************************************************************************
**						test deadlock
*******************************************************************************/
static bool test_TransPrepare_DeadLock_CreateTwoHeaps(Oid& table_space_id,
	Oid& heap_id1, Oid& heap_id2)
{
	table_space_id = OIDGenerator::instance().GetTableSpaceID();
	heap_id1 = OIDGenerator::instance().GetHeapID();
	heap_id2 = OIDGenerator::instance().GetHeapID();
	
	/* create heap */
	begin_transaction();
	try{
		SimpleHeap heap1(table_space_id, heap_id1, FixSpliter::split, true, false);
		SimpleHeap heap2(table_space_id, heap_id2, FixSpliter::split, true, false);
		heap1.Open(AccessShareLock);
		heap2.Open(AccessShareLock);
	}
	CATCHEXCEPTION
	commit_transaction();
	
	return true;
}

static bool test_TransPrepare_DeadLock_DropTwoHeaps(Oid table_space_id,
	Oid heap_id1, Oid heap_id2)
{
    return test_TransPrepare_DropHeapInThread(table_space_id, heap_id1)
            && test_TransPrepare_DropHeapInThread(table_space_id, heap_id2);
}

void test_TransPrepare_DeadLock_Thread1(Oid heap_id1, Oid heap_id2, bool *p_deadlock)
{
	BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_deadlock = false;
	
	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_open(heap_id1, AccessExclusiveLock);
		/* let thread2 run */
		boost::thread::sleep(boost::get_system_time()
			+ boost::posix_time::milliseconds(2000));
		FDPG_Heap::fd_heap_open(heap_id2, AccessExclusiveLock);
		
		/* prepare */
		StartTransactionCommand();
		PrepareTransactionBlock(TEST_TRANSP_PREPARE_ID);
		CommitTransactionCommand();
	}
	catch(DeadLockException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		user_abort_transaction();
		*p_deadlock = true;
	}

	proc_exit(0);
	return;
}

void test_TransPrepare_DeadLock_Thread2(Oid heap_id1, Oid heap_id2, bool *p_deadlock)
{
	BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);
	free(param);

	*p_deadlock = false;
	
	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_open(heap_id2, AccessExclusiveLock);
		FDPG_Heap::fd_heap_open(heap_id1, AccessExclusiveLock);

		/* prepare */
		StartTransactionCommand();
		PrepareTransactionBlock(TEST_TRANSP_PREPARE_ID);
		CommitTransactionCommand();
	}
	catch(DeadLockException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		user_abort_transaction();
		*p_deadlock = true;
	}

	proc_exit(0);
}

bool test_TransPrepare_DeadLock(bool isCommit)
{
	Oid table_space_id;
	Oid heap_id1;
	Oid heap_id2;

	/* create heap */
	if(!test_TransPrepare_DeadLock_CreateTwoHeaps(table_space_id, heap_id1, heap_id2)){
		return false;
	}

	boost::thread_group thgrp;
	bool deadlock1;
	bool deadlock2;
	thgrp.create_thread(boost::bind(&test_TransPrepare_DeadLock_Thread1,
		heap_id1, heap_id2, &deadlock1));
	/* let thread1 run first */
	boost::thread::sleep(boost::get_system_time()
		+ boost::posix_time::milliseconds(1000));
	thgrp.create_thread(boost::bind(&test_TransPrepare_DeadLock_Thread2,
		heap_id1, heap_id2, &deadlock2));
	thgrp.join_all();

	cout << "ret1: " << deadlock1 << ", ret2: " << deadlock2 << endl;
	if((!deadlock1) && (!deadlock2)){
		return false;
	}

	/* commit prepare */
	cout << "start finish......" << endl;
	try{
		bool flag = false;
		PG_TRY();{
			StartTransactionCommand();
			FinishPreparedTransaction(TEST_TRANSP_PREPARE_ID, isCommit);
			CommitTransactionCommand();
		} PG_CATCH(); {
			flag=true;
		} PG_END_TRY(); 
		if(flag){
			ThrowException();
		}
	}
	CATCHEXCEPTION
	cout << "after finish......" << endl;

	/* drop heap */
	if(!test_TransPrepare_DeadLock_DropTwoHeaps(table_space_id, heap_id1, heap_id2)){
		return false;
	}

	return true;
}

bool test_TransPrepare_DeadLock_Commit(void)
{
	return test_TransPrepare_DeadLock(true);
}

bool test_TransPrepare_DeadLock_Abort(void)
{
	return test_TransPrepare_DeadLock(false);
}










/*******************************************************************************
**						test lock
*******************************************************************************/
void test_TransPrepare_Lock_Open_Thread(bool isCommit, Oid heap_id,
	LOCKMODE lockMode2, bool *pOpenFailed)
{
	BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);
	free(param);

	cout << "start to open heap: " << heap_id << endl;

	*pOpenFailed = true;
	
	begin_transaction();
	try
	{
		FDPG_Heap::fd_heap_open(heap_id, lockMode2, MyDatabaseId);
	}
	catch(StorageEngineException& ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		user_abort_transaction();
		return;
	}

	StartTransactionCommand();
	PrepareTransactionBlock("prepare111");
	CommitTransactionCommand();

	StartTransactionCommand();
	FinishPreparedTransaction("prepare111", isCommit);
	CommitTransactionCommand();

	cout << "open heap successed " << endl;

	*pOpenFailed = false;

	proc_exit(0);
}

bool test_TransPrepare_Lock_P1(bool isCommit, LOCKMODE lockMode1,
	LOCKMODE loclMode2)
{
	set<string> test_data;
	Oid table_space_id;
	Oid heap_id;
	
	test_TransPrepare_GetOids(table_space_id, heap_id, true);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	/* create heap */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, true, false);
		heap.Open(AccessShareLock);
	}
	CATCHEXCEPTION
	commit_transaction();

	/* lock heap */
	begin_transaction();
	try{
		FDPG_Heap::fd_heap_open(heap_id, lockMode1, MyDatabaseId);
	}
	CATCHEXCEPTION
	
	/* prepare */
	StartTransactionCommand();
	PrepareTransactionBlock(TEST_TRANSP_PREPARE_ID);
	CommitTransactionCommand();
	
	return true;
}

bool test_TransPrepare_Lock_P2(bool isCommit, LOCKMODE lockMode1,
	LOCKMODE lockMode2)
{
	set<string> test_data;
	Oid table_space_id;
	Oid heap_id;
	bool ret = false;
	bool expect_open_failed = false;
	bool open_failed;

	test_TransPrepare_GetOids(table_space_id, heap_id, false);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	if((lockMode1 == AccessExclusiveLock) || (lockMode2 == AccessExclusiveLock)){
		expect_open_failed = true;
	}

	open_failed = expect_open_failed ? false : true;
	cout << "expect_open_failed: " << (expect_open_failed ? "true" : "false")
		<< ", open_failed: " << (open_failed ? "true" : "false") << endl;

	boost::thread_group thgrp;
	thgrp.create_thread(boost::bind(&test_TransPrepare_Lock_Open_Thread,
		isCommit, heap_id, lockMode2, &open_failed));
	/* let child thread run first */
	boost::thread::sleep(boost::get_system_time()
		+ boost::posix_time::milliseconds(1000));

	cout << "open_failed: " << (open_failed ? "true" : "false") << endl;

	ret = expect_open_failed ? (open_failed) : (!open_failed);

	/* finish prepare */
	cout << "start finish......" << endl;
	StartTransactionCommand();
	FinishPreparedTransaction(TEST_TRANSP_PREPARE_ID, isCommit);
	CommitTransactionCommand();

	thgrp.join_all();

	/* drop heap */
   if(!test_TransPrepare_DropHeapInThread(table_space_id, heap_id))
    {
        cout << "drop heap failed." << endl;
        return false;
    }

	cout << "ret: " << ret << ", open_failed: " << open_failed << endl;

	return ret && (!open_failed);
}

bool test_TransPrepare_Lock_ShareShare_P1(void)
{
	return test_TransPrepare_Lock_P1(true, AccessShareLock, AccessShareLock);
}

bool test_TransPrepare_Lock_ShareShare_P2(void)
{
	return test_TransPrepare_Lock_P2(true, AccessShareLock, AccessShareLock);
}

bool test_TransPrepare_Lock_ShareExclusive_P1(void)
{
	return test_TransPrepare_Lock_P1(true, AccessShareLock, AccessExclusiveLock);
}

bool test_TransPrepare_Lock_ShareExclusive_P2(void)
{
	return test_TransPrepare_Lock_P2(true, AccessShareLock, AccessExclusiveLock);
}

bool test_TransPrepare_Lock_ExclusiveShare_P1(void)
{
	return test_TransPrepare_Lock_P1(true, AccessExclusiveLock, AccessShareLock);
}

bool test_TransPrepare_Lock_ExclusiveShare_P2(void)
{
	return test_TransPrepare_Lock_P2(true, AccessExclusiveLock, AccessShareLock);
}

bool test_TransPrepare_Lock_ExclusiveExclusive_P1(void)
{
	return test_TransPrepare_Lock_P1(true, AccessExclusiveLock, AccessExclusiveLock);
}

bool test_TransPrepare_Lock_ExclusiveExclusive_P2(void)
{
	return test_TransPrepare_Lock_P2(true, AccessExclusiveLock, AccessExclusiveLock);
}












void test_TransPrepare_Concurrency_Thread(bool isPrepare, bool isCommit,
	Oid heap_id, Oid table_space_id, int thread_id)
{
	BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);
	free(param);

	char gid[20];
	snprintf(gid, sizeof(gid), "Prepare%d", thread_id);

	begin_transaction();
	try{
		set<string> test_data;
		test_TransPrepare_BuildInsertTestData(test_data);

		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, false, false);
		heap.Open(AccessShareLock);
		for(set<string>::iterator it = test_data.begin(); it != test_data.end(); ++it){
			heap.Insert(*it);
		}
	}
	catch(StorageEngineExceptionUniversal &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		goto out;
	}

	if(isPrepare){
		StartTransactionCommand();
		PrepareTransactionBlock(gid);
		CommitTransactionCommand();

		StartTransactionCommand();
		FinishPreparedTransaction(gid, isCommit);
		CommitTransactionCommand();
	}else{
		if(isCommit){
			commit_transaction();
		}else{
			user_abort_transaction();
		}
	}

out:
	proc_exit(0);
}

bool test_TransPrepare_Concurrency()
{
	Oid table_space_id;
	Oid heap_id;

	test_TransPrepare_GetOids(table_space_id, heap_id, true);
	cout << "table_space_id: " << table_space_id << ", heap_id: " << heap_id << endl;

	/* create heap */
	begin_transaction();
	try{
		SimpleHeap heap(table_space_id, heap_id, FixSpliter::split, true, false);
		heap.Open(AccessShareLock);
	}
	CATCHEXCEPTION
	commit_transaction();

	boost::thread_group thgrp;
	for(int i = 0; i < 4; i++)
	{
		bool isCommit = (i % 2 == 0);
		bool isPrepare = ((i % 4 == 0) || (i % 4 == 1));
		thgrp.create_thread(boost::bind(&test_TransPrepare_Concurrency_Thread,
					isPrepare, isCommit, heap_id, table_space_id, i));
	}

	thgrp.join_all();

	/* drop heap */
	cout << "start to drop heap" << endl;
    if(!test_TransPrepare_DropHeapInThread(table_space_id, heap_id))
    {
        cout << "drop heap failed." << endl;
        return false;
    }

	return true;
}

