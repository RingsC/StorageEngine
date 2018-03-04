#define BOOST_TEST_NO_MAIN
#include "utils/util.h"
#include "test_fram.h"

#include "heap/test_delete.h"
#include "heap/test_insert.h"
#include "heap/test_select.h"
#include "interface/test_entry_set.h"
#include "heap/test_heap_drop.h"
#include "heap/test_heap_create.h"
#include "heap/test_heap_open.h"
#include "heap/test_heap_close.h"
#include "heap/test_toast.h"
#include "transaction/test_transaction_insert.h"
#include "transaction/test_transaction_delete.h"
#include "transaction/test_transaction_update.h"
#include "transaction/test_transactionWithIndex.h"
#include "transaction/test_index_transaction_search.h"
#include "transaction/test_trans_toast_index.h"
#include "transaction/test_transactionWithToast.h"
#include "transaction/test_isolation_level.h"
#include "transaction/test_trans_prepare.h"
#include "index/test_index_delete.h"
#include "index/test_index_insert.h"
#include "index/test_index_update.h"
#include "index/test_index.h"
#include "index/test_index_cmp.h"
#include "meta/meta_test.h"
#include "thread_commu/thread_commu.h"
#include "bgwriter/test_bgwriter.h"
#include "thread_commu/test_multthread_communicate.h"
#include "thread_commu/thread_commu_hs.h"
#include "thread_commu/thread_commu_hs.h"
#include "transaction/test_subtrans.h"
#include "index/test_toast_index.h"
#include "heap/compress.h"
#include "heap_sort_store/heap_store.h"
#include "large_object/large_object_test.h"
#include "tablespace/create_tablespace.h"
#include "deadlock/deadlocktest.h"
#include "thread_commu/thread_atexit_text.h"
#include "heap/test_heap_hot_update.h"
#include "fatal_exception/fatal_exception.h"
#include "heap/test_truncate.h"
#include "database/test_db.h"
#include "cluster/cluster_simple.h"
#include "cluster/cluster_thread.h"
#include "index/test_lsm_index.h"
#include "vacuum/test_vacuum.h"


//多进程测试函数MULTI_REGIST_FUNC要在单进程测试函数REGIST_FUNC之前测试
BEGIN_DEBUG(DONOTING)
//----多进程测试函数
//...
/*
MULTI_REGIST_FUNC(start_engine_, test_create_drop_heap, stop_engine_, 1)
MULTI_REGIST_FUNC(start_engine_, test_heap_drop, stop_engine_, 1)
*/
//...
//----多进程测试函数


//----单进程测试函数

//REGIST_FUNC(DONOTING, test_ust_readcommited_readcommited, DONOTING)
//REGIST_FUNC(DONOTING, test_ust_readcommited_serializable, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Insert_Commit_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Insert_Commit_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Insert_Abort_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Insert_Abort_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_CreateHeap_Commit_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_CreateHeap_Commit_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_CreateHeap_Abort_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_CreateHeap_Abort_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Isolation_Commit_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Isolation_Commit_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Isolation_Abort_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Isolation_Abort_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_SubTrans_Commit_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_SubTrans_Commit_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_SubTrans_Abort_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_SubTrans_Abort_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_DeadLock_Commit, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_DeadLock_Abort, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Lock_ShareShare_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Lock_ShareShare_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Lock_ShareExclusive_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Lock_ShareExclusive_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Lock_ExclusiveShare_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Lock_ExclusiveShare_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Lock_ExclusiveExclusive_P1, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Lock_ExclusiveExclusive_P2, DONOTING)
REGIST_FUNC(DONOTING, test_TransPrepare_Concurrency, DONOTING)

REGIST_FUNC(DONOTING, testdb_DefaultDatabaseSequenceTable,DONOTING)
REGIST_FUNC(DONOTING, testdb_NewDatabaseSequenceTable,DONOTING)

REGIST_FUNC(DONOTING,test_tuplesort_datum,DONOTING)

////...
//test for table space
REGIST_FUNC(DONOTING, HardLockTest,DONOTING)
//REGIST_FUNC(DONOTING, SoftLockTest,DONOTING)
REGIST_FUNC(DONOTING,test_thread_normal_exit,DONOTING)
BOOST_AUTO_TEST_SUITE(CHTING_TEST)
//exception test
REGIST_FUNC(DONOTING,fatal_exception_test,DONOTING)
//REGIST_FUNC(DONOTING,panic_exception_test,DONOTING)
REGIST_FUNC(DONOTING,test_object_in_use_exception,DONOTING)
REGIST_FUNC(DONOTING,test_tuple_update_concurrent_exception,DONOTING)
REGIST_FUNC(DONOTING,test_two_masters_exception,DONOTING)
REGIST_FUNC(DONOTING,test_unique_violation_exception,DONOTING)
REGIST_FUNC(DONOTING,test_table_space_already_exists,DONOTING)
REGIST_FUNC(DONOTING,test_out_of_memory,DONOTING)

//create tablespace
REGIST_FUNC(DONOTING, test_create_tablespace_valid, test_drop_tablespace_empty)
REGIST_FUNC(DONOTING, test_create_tablespace_with_system_reserve_name, DONOTING)
REGIST_FUNC(DONOTING, test_create_tblspc_with_occupied_name,test_drop_tablespace_empty)
REGIST_FUNC(DONOTING, test_create_tblspc_with_occupied_path, test_drop_tablespace_empty)
REGIST_FUNC(DONOTING, test_create_tblspc_with_relative_path,DONOTING)
REGIST_FUNC(DONOTING, test_create_tblspc_with_nonexist_path, DONOTING)
REGIST_FUNC(DONOTING, test_concurrently_create_tablespace, DONOTING)
//REGIST_FUNC(DONOTING, test_create_tblspc_with_too_long_path, test_drop_tablespace_empty)
REGIST_FUNC(DONOTING, test_create_tblspc_with_pg_Prefix_name, DONOTING)
////REGIST_FUNC(DONOTING, test_create_tblspc_transaction_abort, test_drop_tablespace_empty)
////drop tablespace
REGIST_FUNC(DONOTING,test_drop_default_tablespace, DONOTING)
REGIST_FUNC(DONOTING, test_drop_tablesapce_exist_miss_err, DONOTING)
REGIST_FUNC(test_create_tablespace_valid, test_drop_tablespace_empty, DONOTING)
REGIST_FUNC(DONOTING,test_drop_tablespace_nonexist_miss_err , DONOTING)
REGIST_FUNC(DONOTING,test_drop_tablespace_nonexist_miss_ok , DONOTING)
REGIST_FUNC(DONOTING, test_drop_tablespace_with_empty_db, test_clear_and_drop_tablespace)
REGIST_FUNC(DONOTING, test_drop_tablespace_with_unempty_db, test_clear_and_drop_tablespace)
////get tablespace oid or name
REGIST_FUNC(DONOTING, test_get_tablespace_oid_exist,test_drop_tablespace_empty)
REGIST_FUNC(DONOTING, test_get_tablespace_oid_nonexist, DONOTING)
REGIST_FUNC(DONOTING, test_get_tablespace_name, test_drop_tablespace_empty)
//REGIST_FUNC(DONOTING, test_get_default_tablespace_id, DONOTING)  //this funtion reserved for later

//create database
REGIST_FUNC(test_create_tablespace_valid, test_create_db_under_tablespace, test_clear_and_drop_tablespace)
REGIST_FUNC(test_create_tablespace_valid,test_create_db_with_normal_dbname_len, test_drop_tablespace_empty)
REGIST_FUNC(test_create_tablespace_valid,test_create_db_with_dbname_len_63, test_drop_tablespace_empty)
REGIST_FUNC(test_create_tablespace_valid,test_create_db_with_dbname_len_64, test_drop_tablespace_empty)
REGIST_FUNC(test_create_tablespace_valid,test_create_db_with_larger_than_max_dbname_len, test_drop_tablespace_empty)
REGIST_FUNC(DONOTING, test_create_db_with_invalid_tblspc, DONOTING)
REGIST_FUNC(DONOTING, test_create_database_under_tablespace_no_dir, test_drop_tablespace_empty)
REGIST_FUNC(test_create_tablespace_valid,test_create_db_with_occupied_name, test_drop_tablespace_empty)
REGIST_FUNC(DONOTING, test_create_db_under_diff_tblspc, test_drop_tablespace_empty)
REGIST_FUNC(test_create_tablespace_valid, test_drop_heap_under_tablespace, test_drop_tablespace_empty)
REGIST_FUNC(DONOTING, test_concurrently_createdb, DONOTING)
REGIST_FUNC(DONOTING,test_object_not_exist,DONOTING)
REGIST_FUNC(DONOTING,test_not_in_transaction,DONOTING)


//REGIST_FUNC(DONOTING, test_pause, DONOTING)
BOOST_AUTO_TEST_SUITE_END()

////test case for meta
//REGIST_FUNC(DONOTING, test_debug_001, DONOTING)
//REGIST_FUNC(DONOTING, test_debug_002, DONOTING)
REGIST_FUNC(DONOTING, test_insert_metadata, DONOTING)
REGIST_FUNC(DONOTING, test_get_max_id, DONOTING)
REGIST_FUNC(DONOTING, test_delete_metadata, DONOTING)
REGIST_FUNC(DONOTING, test_get_meta_info, DONOTING)
/////////////////REGIST_FUNC(DONOTING, testInsertLotsOfData, DONOTING)

REGIST_FUNC(DONOTING, testlsm_SubTypeOperation,DONOTING)
REGIST_FUNC(DONOTING, testlsm_SubtypeCreateWhenInsert,DONOTING)
REGIST_FUNC(DONOTING, testlsm_Insert,DONOTING)
//REGIST_FUNC(DONOTING, testlsm_Merge,DONOTING)


//REGIST_FUNC(DONOTING,testWalsender,DONOTING)
//REGIST_FUNC(DONOTING,testWalreceiver,DONOTING)
REGIST_FUNC(DONOTING,test_thread_communicate_Onesend_Multreceive,DONOTING)
REGIST_FUNC(DONOTING,test_thread_communicate_Multsend_Onereceive,DONOTING)
REGIST_FUNC(DONOTING,test_thread_communicate,DONOTING)
REGIST_FUNC(DONOTING,test_multthread_communicate,DONOTING)
REGIST_FUNC(DONOTING,test_thread_communicate_RandDely,DONOTING)
REGIST_FUNC(DONOTING,test_thread_communicate_RandDely_thread,DONOTING)
REGIST_FUNC(DONOTING,test_index_uniqe_01,DONOTING)//error_stack --
REGIST_FUNC(DONOTING,test_index_uniqe_02,DONOTING)//error_stack --
REGIST_FUNC(DONOTING,test_index_uniqe_03,DONOTING)
REGIST_FUNC(DONOTING,test_index_update_multi,DONOTING)

REGIST_FUNC(beginTestNotStartEngine, testDeleteTheSameTupleWithoutIncrement, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testDeleteFirstTupleFromHeap, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testDeleteOneTupleFromManyTuple, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testUpdateTupleWithoutIncrementManyTimes, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testDeleteOneTupleFromTheSameTuple, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testDeleteFromGiveSteps, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testDeleteByTupleInsertGetBlockAndOffset, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testDeleteByBlockAndOffset, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testUpdateSimple, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testUpdateTupleManyTimes, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testUpdateAfterDeleteWithoutIncrement, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testTransactionUpdate001, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testTransactionUpdate002, endTestNotStopEngine)
REGIST_FUNC(beginTestNotStartEngine, testTransactionUpdate003, endTestNotStopEngine)
REGIST_FUNC(benginIndexTest, testIndexDelete001, endIndexTest)
REGIST_FUNC(benginIndexTest, testIndexDelete002, endIndexTest)
REGIST_FUNC(benginIndexTest, testIndexSimpleSearch, endIndexTest)
REGIST_FUNC(benginIndexTest, testIndexSearch001, endIndexTest)
REGIST_FUNC(benginIndexTest, testIndexSearch002, endIndexTest)
REGIST_FUNC(benginIndexTest_2, testIndexSearch003, endIndexTest)

REGIST_FUNC(DONOTING, test_simple_heap_insert_000, DONOTING)
REGIST_FUNC(DONOTING, test_simple_heap_insert_001, DONOTING)
REGIST_FUNC(DONOTING, test_simple_heap_insert_002, DONOTING)
REGIST_FUNC(DONOTING, test_simple_heap_insert_005, DONOTING)
REGIST_FUNC(DONOTING, test_simple_heap_insert_006, DONOTING)
REGIST_FUNC(DONOTING, test_simple_heap_insert_007, DONOTING)
//REGIST_FUNC(DONOTING, test_simple_heap_insert_008, DONOTING)

//REGIST_FUNC(DONOTING, test_debug_001, DONOTING)
//REGIST_FUNC(DONOTING, test_debug_001, DONOTING)

REGIST_FUNC(DONOTING, test_heap_sqscan_000, DONOTING)
REGIST_FUNC(DONOTING, test_heap_sqscan_001, DONOTING)
REGIST_FUNC(DONOTING, test_heap_sqscan_002, DONOTING)
REGIST_FUNC(DONOTING, test_heap_sqscan_003, DONOTING)

REGIST_FUNC(DONOTING, test_indexscan_000, DONOTING)
REGIST_FUNC(DONOTING, test_indexscan_001, DONOTING)
REGIST_FUNC(DONOTING, test_indexscan_002, DONOTING)
REGIST_FUNC(DONOTING, test_indexscan_003, DONOTING)
REGIST_FUNC(DONOTING, test_indexscan_004, DONOTING)
REGIST_FUNC(DONOTING, test_cluster_heap_01, DONOTING)
REGIST_FUNC(DONOTING, test_cluster_heap_02, DONOTING)
REGIST_FUNC(DONOTING, thread_test_cluster_001, DONOTING)
REGIST_FUNC(DONOTING, thread_test_cluster_002, DONOTING)
//REGIST_FUNC(DONOTING, thread_test_cluster_003, DONOTING);
BOOST_FIXTURE_TEST_SUITE(TransExitCase7, EmptyFixture)
REGIST_FUNC(DONOTING, test_cluster_exit_heap_01, DONOTING)
BOOST_AUTO_TEST_SUITE_END()
/////////////REGIST_FUNC(DONOTING, test_cluster_exit_heap_02, DONOTING)----cluster性能测试
REGIST_FUNC(DONOTING, test_reindex_index, DONOTING)
REGIST_FUNC(DONOTING, test_reindex_index_Unique_delete, DONOTING)
REGIST_FUNC(DONOTING, test_reindex_index_Unique_Update, DONOTING)
REGIST_FUNC(DONOTING, test_reindex_relation_toast, DONOTING)
REGIST_FUNC(DONOTING, test_reindex_relation, DONOTING)
REGIST_FUNC(DONOTING, test_reindex_database, DONOTING)
REGIST_FUNC(DONOTING, test_reindex_index_mult_thread, DONOTING)
REGIST_FUNC(DONOTING, test_reindex_relation_mult_thread, DONOTING)
REGIST_FUNC(DONOTING, test_reindex_database_mult_thread, DONOTING)
REGIST_FUNC(DONOTING, HashIndexTest,DONOTING)
REGIST_FUNC(DONOTING, ReIndexHashIndexTest,DONOTING)
REGIST_FUNC(DONOTING, HashIndexToastTest,DONOTING)
REGIST_FUNC(DONOTING, HashIndexMultTest,DONOTING)
REGIST_FUNC(DONOTING, ReInexHashIndexMultTest,DONOTING)
REGIST_FUNC(DONOTING, ToastHashIndexMultTest,DONOTING)
REGIST_FUNC(DONOTING, HashIndexBitmapTest,DONOTING)
//
//
REGIST_FUNC(DONOTING, test_transaction_insert_000, DONOTING)
REGIST_FUNC(DONOTING, test_transaction_insert_001, DONOTING)
REGIST_FUNC(DONOTING,test_transaction_delete_atom,DONOTING)
REGIST_FUNC(DONOTING,test_transaction_delete_atom_halfdelete,DONOTING)
//
REGIST_FUNC(DONOTING,test_heap_multcreate,DONOTING)
REGIST_FUNC(DONOTING,test_heap_open,DONOTING)
REGIST_FUNC(DONOTING,test_heap_open_mult_close,DONOTING)
REGIST_FUNC(DONOTING,test_heap_open_mult_withoutclose,DONOTING)
REGIST_FUNC(DONOTING,test_heap_close,DONOTING)
REGIST_FUNC(DONOTING,test_heap_close_WrongLock,DONOTING)
REGIST_FUNC(DONOTING,heap_drop_OpenWithoutClose,DONOTING)
REGIST_FUNC(DONOTING,test_indexscan_NumberCmp,DONOTING)

BOOST_AUTO_TEST_SUITE(INDEX_SCAN_MULTICOL_TEST)
REGIST_FUNC(DONOTING,test_indexscan_LargeMount,DONOTING)
REGIST_FUNC(DONOTING,test_indexscan_OneCol_MultMethod,DONOTING)
REGIST_FUNC(DONOTING,test_indexscan_MultCol_MultMethod,DONOTING)
REGIST_FUNC(DONOTING,test_indexscan_MultCol_Col1OneMethod_Col2MultMethod,DONOTING)
BOOST_AUTO_TEST_SUITE_END()
////
REGIST_FUNC(DONOTING,testIndexUpdate_SingleColumn,DONOTING)
REGIST_FUNC(DONOTING,testIndexUpdate_MultiColumn,DONOTING)
REGIST_FUNC(DONOTING,testIndexUpdate_InAnotherTrans,DONOTING)
REGIST_FUNC(DONOTING,testIndexInsert_SingleColumn,DONOTING)
//REGIST_FUNC(DONOTING,testIndexInsert_SameScakKey,DONOTING)
//
REGIST_FUNC(DONOTING,testIndex_InAnotherTrans,DONOTING)
REGIST_FUNC(testTransWidthIndexSetUp,testTransWidthIndexInsert,DONOTING)
REGIST_FUNC(DONOTING,testTransWidthIndexUpdate,DONOTING)
REGIST_FUNC(DONOTING,testTransWidthIndexDelete,testTransWidthIndexClearUp)
BOOST_FIXTURE_TEST_SUITE(ToastTest,TimerFixture)
REGIST_FUNC(DONOTING,TestToast_Insert,DONOTING)
REGIST_FUNC(DONOTING,TestToast_Update,DONOTING)
REGIST_FUNC(DONOTING,TestToast_Delete,DONOTING)
REGIST_FUNC(DONOTING,TestToast_temp_Insert,DONOTING)
REGIST_FUNC(DONOTING,TestToast_temp_Update,DONOTING)
REGIST_FUNC(DONOTING,TestToast_temp_Delete,TestToast_end)
REGIST_FUNC(DONOTING,TestIndexWithToast_Insert,DONOTING)
REGIST_FUNC(DONOTING,TestIndexWithToast_MultiOrder,DONOTING)
REGIST_FUNC(DONOTING,TestIndexWithToast_Update,DONOTING)
REGIST_FUNC(DONOTING,TestIndexWithToast_Delete,DONOTING)
REGIST_FUNC(DONOTING,TestIndexWithToast_temp_Insert,DONOTING)
REGIST_FUNC(DONOTING,TestIndexWithToast_temp_MultiOrder,DONOTING)
REGIST_FUNC(DONOTING,TestIndexWithToast_temp_Update,DONOTING)
REGIST_FUNC(DONOTING,TestIndexWithToast_temp_Delete,TestIndexWithToast_end)
REGIST_FUNC(DONOTING,testToastTrans_InsertRollback,DONOTING)
REGIST_FUNC(DONOTING,testToastTrans_UpdateRollback,DONOTING)
REGIST_FUNC(DONOTING,testToastTrans_DeleteRollback,DONOTING)
REGIST_FUNC(DONOTING,test_toastIndex_InsertRollabck,DONOTING)
REGIST_FUNC(DONOTING,test_toastIndex_UpdateRollback,DONOTING)
REGIST_FUNC(DONOTING,test_toastIndex_DeleteRollback,DONOTING)
BOOST_AUTO_TEST_SUITE_END()

/************************************************************************
* Truncate Feature Test ---- START
*************************************************************************/
BOOST_FIXTURE_TEST_SUITE(TestTruncate, TimerFixture)
REGIST_FUNC(DONOTING, testtruncate_HeapNoIndex, DONOTING)
REGIST_FUNC(DONOTING, testtruncate_ToastNoIndex, DONOTING)
REGIST_FUNC(DONOTING, testtruncate_HeapOneBtreeIndex, DONOTING)
REGIST_FUNC(DONOTING, testtruncate_ToastOneBtreeIndex, DONOTING)
REGIST_FUNC(DONOTING, testtruncate_HeapOneBtreeUniqueIndex, DONOTING)
REGIST_FUNC(DONOTING, testtruncate_ToastOneBtreeUniqueIndex, DONOTING)
REGIST_FUNC(DONOTING, testtruncate_HeapTwoIndexes, DONOTING)
REGIST_FUNC(DONOTING, testtruncate_ToastTwoIndexes, DONOTING)
#if 0
BOOST_FIXTURE_TEST_SUITE(StopEngine, TimerFixture)
REGIST_FUNC(DONOTING, testtruncate_StopEngine_step1, DONOTING)
REGIST_FUNC(DONOTING, testtruncate_StopEngine_step2, DONOTING)
BOOST_AUTO_TEST_SUITE_END()
#endif
BOOST_AUTO_TEST_SUITE_END()
/************************************************************************
*Truncate Feature Test ---- END
*************************************************************************/


/************************************************************************
* Vacuum Feature Test ---- START
*************************************************************************/
BOOST_FIXTURE_TEST_SUITE(TestVacuum, TimerFixture)
REGIST_FUNC(DONOTING, testvacuum_SmallHeap, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_LargeHeap, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_SmallToast, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_LargeToast, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_SmallHeapWithIndex, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_LargeHeapWithIndex, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_SmallToastWithIndex, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_LargeToastWithIndex, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_ConTestSimpleHeap, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_ConTestToast, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_ConTestHeapWithIndex, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_ConTestToastWithIndex, DONOTING)
REGIST_FUNC(DONOTING, testvacuum_Database, DONOTING)
BOOST_AUTO_TEST_SUITE_END()
/************************************************************************
*Vacuum Feature Test ---- END
*************************************************************************/




REGIST_FUNC(DONOTING,test_simple_tuple_store,DONOTING)
REGIST_FUNC(DONOTING,test_simple_tuple_sort,DONOTING)
REGIST_FUNC(DONOTING,test_sorted_tuple_sort,DONOTING)
REGIST_FUNC(DONOTING,test_hot_update_data,DONOTING)
REGIST_FUNC(DONOTING,test_hot_update_toast_data,DONOTING)

//BOOST_FIXTURE_TEST_SUITE(LTTest,EmptyFixture)
//REGIST_FUNC(DONOTING,test_simple_large_object,DONOTING)
//BOOST_AUTO_TEST_SUITE_END()

//REGIST_FUNC(DONOTING, test_thread_large_object, DONOTING)
REGIST_FUNC(DONOTING, thread_test_temprel, DONOTING);
REGIST_FUNC(DONOTING, simple_test_temprel, DONOTING);
//
//----单进程测试函数
//
//
//
//----多线程测试函数
// ...

//REGIST_FUNC(DONOTING, thread_test_maxnum_create_tmprel, DONOTING);
//BOOST_FIXTURE_TEST_SUITE(LTTest,EmptyFixture)
//REGIST_FUNC(DONOTING,test_thread_vacuum_lt,DONOTING)
//BOOST_AUTO_TEST_SUITE_END()

//REGIST_FUNC(DONOTING,test_pgstat_vacuum,DONOTING)
//REGIST_FUNC(DONOTING,test_index_pgstat_vacuum,DONOTING)
//REGIST_FUNC(DONOTING,test_toast_pgstat_vacuum,DONOTING)
//REGIST_FUNC(DONOTING,test_index_toast_pgstat_vacuum,DONOTING)
//REGIST_FUNC(DONOTING,test_vacuum_truncate,DONOTING)

//REGIST_FUNC(DONOTING, test_thread_delete_000 , DONOTING)
REGIST_FUNC(DONOTING, test_thread_delete_001 , DONOTING)
// // REGIST_FUNC(DONOTING, test_thread_delete_002 , DONOTING)
// // REGIST_FUNC(DONOTING, test_thread_delete_003 , DONOTING)
REGIST_FUNC(DONOTING, test_thread_delete_004 , DONOTING)
// // REGIST_FUNC(DONOTING, test_thread_update_000 , DONOTING)
REGIST_FUNC(DONOTING, test_thread_update_001 , DONOTING)
REGIST_FUNC(DONOTING, test_thread_update_002 , DONOTING)
REGIST_FUNC(DONOTING,test_thread_update_rollback,DONOTING)

//REGIST_FUNC(DONOTING, test_thread_create_heap_000 , DONOTING)
REGIST_FUNC(DONOTING, test_thread_create_heap_001 , DONOTING)
//REGIST_FUNC(DONOTING, test_thread_create_heap_002 , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_drop_000 , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_drop_001 , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_drop_002 , DONOTING)
REGIST_FUNC(DONOTING, test_hq_thread_insert_000 , DONOTING)
REGIST_FUNC(DONOTING, test_hq_thread_insert_001 , DONOTING)
REGIST_FUNC(DONOTING, test_hq_thread_insert_002 , DONOTING)

REGIST_FUNC(DONOTING,test_thread_heap_open,DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_drop_and_open, DONOTING)
REGIST_FUNC(DONOTING,test_thread_heap_close,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_011,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_012,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_013,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_014,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_015,DONOTING)
//REGIST_FUNC(DONOTING,test_thread_create_heap_016,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_017,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_018,DONOTING)
//REGIST_FUNC(DONOTING,test_thread_create_heap_019,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_020,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_021,DONOTING)
REGIST_FUNC(DONOTING,test_thread_simple_heap_insert_022,DONOTING)

BOOST_FIXTURE_TEST_SUITE(TransPersistence,EmptyFixture)
///////////////REGIST_FUNC(DONOTING,test_thread_transaction_heap_insert_023,DONOTING)
///////////////REGIST_FUNC(DONOTING,test_thread_transaction_heap_insert_025,DONOTING)
///////////////REGIST_FUNC(DONOTING,test_thread_transaction_heap_insert_027,DONOTING)
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(TransExitCase1,EmptyFixture)
REGIST_FUNC(DONOTING,test_thread_transaction_heap_insert_024,DONOTING)
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(TransExitCase2,EmptyFixture)
REGIST_FUNC(DONOTING,test_thread_transaction_heap_insert_026,DONOTING)
BOOST_AUTO_TEST_SUITE_END()
 
BOOST_FIXTURE_TEST_SUITE(TransExitCase3,EmptyFixture)
REGIST_FUNC(DONOTING,test_thread_transaction_heap_insert_028,DONOTING)
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(TransExitCase4,EmptyFixture)
REGIST_FUNC(DONOTING,test_thread_transaction_heap_insert_029,DONOTING)
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(TransExitCase5,EmptyFixture)
REGIST_FUNC(DONOTING,test_thread_transaction_heap_insert_030,DONOTING)
BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(TransExitCase6,EmptyFixture)
REGIST_FUNC(DONOTING,test_thread_transaction_heap_insert_031,DONOTING)
BOOST_AUTO_TEST_SUITE_END()
//
REGIST_FUNC(DONOTING, test_thread_index_scan_005, DONOTING)
////////////////REGIST_FUNC(DONOTING, test_thread_index_scan_006, DONOTING)
////////////////REGIST_FUNC(DONOTING, test_thread_index_scan_007, DONOTING)
////////////////REGIST_FUNC(DONOTING, test_thread_index_scan_008, DONOTING)
REGIST_FUNC(DONOTING, test_thread_index_scan_009, DONOTING)
REGIST_FUNC(DONOTING, test_thread_index_create_010, DONOTING)
//
////////////////REGIST_FUNC(DONOTING,test_thread_index_update_01,DONOTING)
REGIST_FUNC(DONOTING,test_thread_index_update_02,DONOTING)
REGIST_FUNC(DONOTING,test_thread_index_update_03,DONOTING)
//
REGIST_FUNC(DONOTING,test_thread_indexscan_000,DONOTING)
REGIST_FUNC(DONOTING,test_thread_indexscan_001,DONOTING)
////////////////REGIST_FUNC(DONOTING,test_thread_indexscan_002,DONOTING)
REGIST_FUNC(DONOTING,test_thread_indexscan_003,DONOTING)
REGIST_FUNC(DONOTING,test_thread_indexscan_004,DONOTING)
//
//REGIST_FUNC(DONOTING,TestSubTransaction,DONOTING)
//
REGIST_FUNC(DONOTING,TestSubTransactionAbort,DONOTING)
REGIST_FUNC(DONOTING,TestMultiSubTransaction,DONOTING)
REGIST_FUNC(DONOTING,TestMultiSubTransactionAbort,DONOTING)
REGIST_FUNC(DONOTING,TestMultiLevelTransaction,DONOTING)
REGIST_FUNC(DONOTING,TestMultiLevelTransactionAbort,DONOTING)

REGIST_FUNC(DONOTING, myTest_test_heap_sqscan_000, DONOTING)
REGIST_FUNC(DONOTING, myTest_test_heap_sqscan_001, DONOTING)
REGIST_FUNC(DONOTING, myTest_test_heap_sqscan_002, DONOTING)
REGIST_FUNC(DONOTING, myTest_test_heap_sqscan_003, DONOTING)

REGIST_FUNC(DONOTING, myTest_simple_heap_insert_000, DONOTING)
REGIST_FUNC(DONOTING, myTest_simple_heap_insert_001, DONOTING)
REGIST_FUNC(DONOTING, myTest_simple_heap_insert_002, DONOTING)
REGIST_FUNC(DONOTING, myTest_simple_heap_insert_003, DONOTING)

REGIST_FUNC(DONOTING, myTest_test_indexscan_000, DONOTING)
REGIST_FUNC(DONOTING, myTest_heap_delete_000, DONOTING)
REGIST_FUNC(DONOTING, myTest_heap_delete_001, DONOTING)
//REGIST_FUNC(DONOTING, myTest_open_heap_conflict, DONOTING)

//----多线程测试函数
//END_DEBUG(stop_engine_)
END_DEBUG(DONOTING)
