#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "heap/test_heap_insert_dll.h"
#include "heap/test_heap_select_dll.h"
#include "transaction/test_transaction_insert_dll.h"
#include "transaction/test_transaction_delete_dll.h"
#include "heap/test_heap_create_dll.h"
#include "heap/test_heap_open_dll.h"
#include "heap/test_heap_close_dll.h"
#include "heap/test_heap_drop_dll.h"
#include "heap/test_heap_delete_dll.h"
#include "heap/test_heap_update_dll.h"
#include "heap/test_toast_dll.h"
#include "index/test_index_insert_dll.h"
#include "index/test_index_update_dll.h"
#include "index/test_index_dll.h"
#include "transaction/test_transaction_index_create_dll.h"
#include "index/test_index_update_dll.h"
#include "index/test_toast_index_dll.h"
#include "index/test_index_cmp_dll.h"
#include "transaction/test_transaction_update_dll.h"
#include "index/test_index_transation_dll.h"
#include "index/test_lsm_index_dll.h"
#include "transaction/test_transaction_index_dll.h"
#include "transaction/test_transaction_toast_dll.h"
#include "transaction/test_trans_toast_index_dll.h"
#include "transaction/test_trans_multi_index_update.h"
#include "transaction/test_subtrans_dll.h"
#include "transaction/test_transaction_isolation_dll.h"
#include "transaction/test_transactionid_min_and_max.h"
#include "sequence/utils.h"
#include "memcontext/test_memcontext.h"
#include "heap_sort_store/heap_store_dll.h"
#include "heap/test_dump_load.h"
#include "backup_recovery/BRTest.h"
#include "tablespace/test_tablespace.h"
#include "large_object/large_object_test.h"

#include "backup_recovery/timelines.h"
#include "DeadLock/deadlocktest.h"
#include "backup_recovery/Recovery2Target.h"
#include "backup_recovery/Recovery2Recently.h"
#include "lock/test_lock_dll.h"
#include "bitmap/test_bitmap_dll.h"
#include "heap/test_heap_multi_insert.h"
#include "cluster/test_cluster_simple.hpp"


//多进程测试函数MULTI_REGIST_FUNC要在单进程测试函数REGIST_FUNC之前测试
BEGIN_DEBUG(DONOTING)
//----多进程测试函数
//...
//MULTI_REGIST_FUNC(DONOTING,test_heap_create_drop_dll,DONOTING,1)
//MULTI_REGIST_FUNC(DONOTING,test_heap_drop_dll,DONOTING,1)
//MULTI_REGIST_FUNC(startEngineAndCreateTable, test_transaction_insert_dll_002_step1, stop_engine_, 1)
//MULTI_REGIST_FUNC(start_engine_, test_transaction_insert_dll_002_step2, dropTableAndStopEngine, 1)
//MULTI_REGIST_FUNC(startEngineAndCreateTable, test_transaction_delete_dll_002_step1, stop_engine_, 1)
//MULTI_REGIST_FUNC(start_engine_, test_transaction_delete_dll_002_step2, dropTable, 1)
/*MULTI_REGIST_FUNC(start_engine_, test_create_drop_heap, stop_engine_, 1)
MULTI_REGIST_FUNC(start_engine_, test_heap_drop, stop_engine_, 1)
*/
//...
//----多进程测试函数
//REGIST_FUNC(DONOTING, test_toast_vacuum, DONOTING)


//----单进程测试函数
//...
REGIST_FUNC(DONOTING, testlsmdll_Operation, DONOTING)
#if 0
REGIST_FUNC(DONOTING, testlsmdll_InsertBigDataFirst, DONOTING)
REGIST_FUNC(DONOTING, testlsmdll_ScanAll, DONOTING)
REGIST_FUNC(DONOTING, testlsmdll_InsertBigData, DONOTING)
REGIST_FUNC(DONOTING, testlsmdll_QueryBigData, DONOTING)
REGIST_FUNC(DONOTING, testlsmdll_BitmapScanBigData, DONOTING)
REGIST_FUNC(DONOTING, testlsmdll_InternalMergeConcurrency, DONOTING)
REGIST_FUNC(DONOTING, testlsmdll_InternalInsertConcurrency, DONOTING)
REGIST_FUNC(DONOTING, testlsmdll_Concurrency, DONOTING)
#endif

//REGIST_FUNC(DONOTING, test_replication_check, DONOTING)
BOOST_AUTO_TEST_SUITE(CHTING_TEST)
REGIST_FUNC(DONOTING ,test_base_backup, DONOTING)
REGIST_FUNC(DONOTING, test_heap_recovery,DONOTING)
REGIST_FUNC(DONOTING, test_recovery_modify, DONOTING)
REGIST_FUNC(DONOTING, test_heap_recovery_again,DONOTING)
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(GUO_CT_TBLSPC)
REGIST_FUNC(DONOTING, test_create_tblspc_prepare,DONOTING)
REGIST_FUNC(DONOTING, make_base_backup, DONOTING)
REGIST_FUNC(DONOTING, test_create_tblspc, DONOTING)
REGIST_FUNC(DONOTING,test_drop_tblspc,DONOTING)
REGIST_FUNC(DONOTING, test_tblspc_recovery, DONOTING)
REGIST_FUNC(DONOTING, test_get_create_db_oid, DONOTING)
REGIST_FUNC(DONOTING, test_create_heap_under_diff_tblspc,DONOTING)
BOOST_AUTO_TEST_SUITE_END()

REGIST_FUNC(DONOTING, testseq_SeqFactoryMethod, DONOTING)
REGIST_FUNC(DONOTING, testseq_SeqMethod, DONOTING)
REGIST_FUNC(DONOTING, testseq_CreateSeqConcurrency, DONOTING)
REGIST_FUNC(DONOTING, testseq_GetValueConcurrency, DONOTING)
//REGIST_FUNC(DONOTING, test_thread_create_index_check_valid_1 , DONOTING)
 
REGIST_FUNC(DONOTING,test_getMemoryContext,DONOTING)
REGIST_FUNC(DONOTING,test_ReAlloc,DONOTING)
//REGIST_FUNC(DONOTING,test_operator_new_delete_trans,DONOTING)
REGIST_FUNC(create_entryset, test_index_transaction_001, remove_entryset)
REGIST_FUNC(create_entryset, test_index_transaction_002, remove_entryset)
REGIST_FUNC(create_entryset, test_index_transaction_003, remove_entryset)
////REGIST_FUNC(create_entryset, test_index_transaction_004, remove_entryset) 
////REGIST_FUNC(create_entryset, test_index_transaction_005, remove_entryset) 

REGIST_FUNC(index_create_table, test_indexscan_dll_000, index_drop_table)
REGIST_FUNC(index_create_table, test_indexscan_dll_001, index_drop_table)
REGIST_FUNC(index_create_table, test_indexscan_dll_002, index_drop_table)
REGIST_FUNC(index_create_table_split_to_6, test_indexscan_dll_003, index_drop_table)
//REGIST_FUNC(index_create_table_split_to_100, test_indexscan_dll_004, index_drop_table)

 REGIST_FUNC(createTable, test_heap_insert_dll_000, dropTable)
 REGIST_FUNC(createTable, test_heap_insert_dll_001, dropTable)
 REGIST_FUNC(createTable, test_heap_insert_dll_002, dropTable)
 REGIST_FUNC(createTable, test_heap_insert_dll_003, dropTable)
// REGIST_FUNC(createTable, test_heap_insert_dll_004, dropTable)
REGIST_FUNC(createTable, test_heap_insert_dll_005, dropTable)
REGIST_FUNC(createTable, test_heap_insert_dll_006, dropTable)
REGIST_FUNC(createTable, test_heap_select_dll_000, dropTable)
REGIST_FUNC(createTable, test_heap_select_dll_001, dropTable)
REGIST_FUNC(createTable, test_heap_select_dll_002, dropTable)
REGIST_FUNC(createTable, test_heap_select_dll_003, dropTable)

//
REGIST_FUNC(createTable, test_transaction_insert_dll_000, dropTable)
REGIST_FUNC(createTable, test_transaction_insert_dll_001, dropTable)
REGIST_FUNC(createTable, test_transaction_insert_dll_003, dropTable)
REGIST_FUNC(createTable, test_transaction_delete_dll_000, dropTable)
REGIST_FUNC(createTable, test_transaction_delete_dll_001, dropTable)
REGIST_FUNC(createTable, test_transaction_update_dll_000, dropTable)
REGIST_FUNC(createTable, test_transaction_update_dll_001, dropTable)
REGIST_FUNC(createTable,test_heap_create_dll,dropTable)
REGIST_FUNC(createTable,test_heap_open_dll,dropTable)
REGIST_FUNC(createTable,test_heap_multopen_dll,dropTable)
REGIST_FUNC(createTable,test_heap_close_dll,dropTable)
REGIST_FUNC(createTable,test_heap_close_Theninsert_dll,dropTable)
REGIST_FUNC(createTable,test_heap_close_withoutopen_dll,dropTable)
REGIST_FUNC(beginTest,test_Delete_FirstTupleFromHeap_dll,dropTable)
REGIST_FUNC(beginTest,test_Delete_OneTupleFromManyTuple_dll,dropTable)
REGIST_FUNC(beginTest,test_Delete_FromGiveSteps_dll,dropTable)
REGIST_FUNC(beginTest,test_DeleteTheSameTuple_Without_commit_dll,dropTable)
REGIST_FUNC(createTable,test_DeleteByTupleInsert_GetBlockAndOffset_dll,dropTable)
//REGIST_FUNC(beginTest,test_Delete_NoExistTuple_dll,dropTable) 
REGIST_FUNC(beginTest,test_Update_simple_dll,dropTable)
REGIST_FUNC(beginTest,test_Update_WithoutCommitManyTimes_dll,dropTable)
REGIST_FUNC(beginTest,test_Update_TupleManyTimes_dll,dropTable)
//REGIST_FUNC(DONOTING,test_Delete_Shot_Dll,DONOTING)
REGIST_FUNC(DONOTING,test_Update_simple_shot_dll,DONOTING)
//REGIST_FUNC(beginTest,test_Update_OnNoExistBlock_dll,dropTable) 
////
////
REGIST_FUNC(createTable, test_index_transaction_create_dll_000, dropTable)
REGIST_FUNC(createTable, test_index_transaction_create_dll_001, dropTable)
REGIST_FUNC(createTable, test_index_transaction_create_dll_002, dropTable)
REGIST_FUNC(createTable, test_index_transaction_create_dll_003, dropTable)
REGIST_FUNC(createTable, test_index_transaction_create_dll_004, dropTable)
//
REGIST_FUNC(DONOTING,test_index_uniqe_01_dll,DONOTING)

BOOST_AUTO_TEST_SUITE(SHGLI_TEST)
REGIST_FUNC(DONOTING,PrepareBasebackup,DONOTING)
REGIST_FUNC(DONOTING,RecoveryFirst,DONOTING)
REGIST_FUNC(DONOTING,MakeMoreChanges1,DONOTING)

REGIST_FUNC(DONOTING,Recovery2Recently,DONOTING)
REGIST_FUNC(DONOTING,DeadLockTest,DONOTING);
//REGIST_FUNC(DONOTING, testCreateTablespace, DONOTING)
//REGIST_FUNC(DONOTING, testDropTablespace, DONOTING)
REGIST_FUNC(DONOTING,TestDBExtraData, DONOTING)
REGIST_FUNC(DONOTING,TestGetAllDBInfos, DONOTING)
REGIST_FUNC(DONOTING,TestGetDBInfoById, DONOTING)
REGIST_FUNC(DONOTING, test_dump_load, DONOTING)
/*REGIST_FUNC(DONOTING,test_toast_insert,TableDroper::Drop)
REGIST_FUNC(DONOTING,test_toast_update,TableDroper::Drop)
REGIST_FUNC(DONOTING,test_toast_delete,TableDroper::Drop)

REGIST_FUNC(DONOTING,test_toastIndex_InsertRollabck_DLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,test_toastIndex_UpdateRollback_DLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,test_toastIndex_DeleteRollback_DLL,TableDroper::Drop)*/ //influence
REGIST_FUNC(DONOTING,testIndexInsert_SingleColumn,TableDroper::Drop)
////REGIST_FUNC(DONOTING,testIndexInsert_SameScaKey,TableDroper::Drop)
REGIST_FUNC(DONOTING,testIndex_InAnotherTrans,TableDroper::Drop)
REGIST_FUNC(DONOTING,testIndex_Sort,TableDroper::Drop)
REGIST_FUNC(DONOTING,test_cluster_index,TableDroper::Drop)
//
REGIST_FUNC(DONOTING,testIndexUpdate_SingleColumn_DLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,testIndexUpdate_MultiColumn_DLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,testIndexUpdate_InAnotherTrans_DLL,TableDroper::Drop)
//
////////////////REGIST_FUNC(DONOTING,test_toast_index_insert,TableDroper::Drop)
////////////////REGIST_FUNC(DONOTING,test_toast_index_update,TableDroper::Drop)
////////////////REGIST_FUNC(DONOTING,test_toast_index_delete,TableDroper::Drop)
////////////////REGIST_FUNC(DONOTING,test_toast_index_order,TableDroper::Drop)

REGIST_FUNC(DONOTING,test_trans_index_InsertRollback,TableDroper::Drop)
REGIST_FUNC(DONOTING,test_trans_index_UpdateRollback,TableDroper::Drop)
REGIST_FUNC(DONOTING,test_trans_index_DeleteRollback,TableDroper::Drop)

////////////////REGIST_FUNC(DONOTING,testToastTrans_InsertRollback_DLL,TableDroper::Drop)
////////////////REGIST_FUNC(DONOTING,testToastTrans_UpdateRollback_DLL,TableDroper::Drop)
////////////////REGIST_FUNC(DONOTING,testToastTrans_DeleteRollback_DLL,TableDroper::Drop)

REGIST_FUNC(DONOTING,TestSubTransactionDLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,TestSubTransactionAbortDLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,TestMultiSubTransactionDLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,TestMultiSubTransactionAbortDLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,TestMultiLevelTransactionDLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,TestMultiLevelTransactionAbortDLL,TableDroper::Drop)
REGIST_FUNC(DONOTING,test_trans_multi_index,DONOTING)
//REGIST_FUNC(DONOTING,test_trans_multi_index_nullvalue,DONOTING)

//TEST backup and recovery
REGIST_FUNC(DONOTING,prepareBackup,DONOTING)
REGIST_FUNC(DONOTING,test_backup,DONOTING)
REGIST_FUNC(DONOTING,test_Recovery,DONOTING)
REGIST_FUNC(DONOTING,TestRecovery2Target,DONOTING)
BOOST_AUTO_TEST_SUITE_END()

REGIST_FUNC(DONOTING,test_Update_Delete_Insert,DONOTING)

REGIST_FUNC(DONOTING,test_heap_store_dll,DONOTING)
REGIST_FUNC(DONOTING,test_heap_sort_dll,DONOTING)
REGIST_FUNC(DONOTING,test_transactionid_max,DONOTING)
REGIST_FUNC(DONOTING,test_transactionid_next,DONOTING)
REGIST_FUNC(DONOTING,test_transaction_del,DONOTING)
REGIST_FUNC(DONOTING,test_DataBase,DONOTING)

//REGIST_FUNC(DONOTING,test_transactionid_max_second,DONOTING)
//REGIST_FUNC(DONOTING,test_transactionid_min,DONOTING)

//REGIST_FUNC(DONOTING,test_lo_seek_tell_dll,DONOTING)
//REGIST_FUNC(DONOTING,test_simple_large_object_dll,DONOTING)
REGIST_FUNC(DONOTING, test_database_large_object_dll, DONOTING)

REGIST_FUNC(DONOTING, test_cluster_entryset_001, DONOTING)
////----单进程测试函数

//////多线程测试函数//////
////warining about lock once and core once fxdb_get_index_info(index_endscan())/////

REGIST_FUNC(DONOTING, TestTidBitmap, TableDroper::Drop)
REGIST_FUNC(DONOTING, TestSpinLock, DONOTING)
REGIST_FUNC(DONOTING, TestMutexLockExclusive, DONOTING)
REGIST_FUNC(DONOTING, TestMutexLockShare, DONOTING)
REGIST_FUNC(DONOTING, TestTransactionLock , DONOTING)


REGIST_FUNC(DONOTING, TestTransactionReadIsolation , DONOTING)
REGIST_FUNC(DONOTING, TestTransactionRepeatableIsolation , DONOTING)
REGIST_FUNC(DONOTING, TestTransactionSerializableIsolation , DONOTING)


REGIST_FUNC(DONOTING, test_thread_create_heap , DONOTING)
REGIST_FUNC(DONOTING, test_thread_create_transaction , DONOTING)
//REGIST_FUNC(DONOTING, test_thread_create_transaction_noncommit , DONOTING)

REGIST_FUNC(DONOTING, test_thread_del_heap , DONOTING)
///logic error for heap delete operation///
REGIST_FUNC(DONOTING, test_thread_del_heap_nonexist_zero , DONOTING)
REGIST_FUNC(DONOTING, test_thread_del_heap_nonexist , DONOTING)
REGIST_FUNC(DONOTING, test_thread_del_heap_meta , DONOTING)

///core test_thread_create_index////
REGIST_FUNC(DONOTING, test_thread_create_index , DONOTING)
REGIST_FUNC(DONOTING, test_thread_create_index_insert_unique , DONOTING)

REGIST_FUNC(DONOTING, test_thread_open_heap , DONOTING)
REGIST_FUNC(DONOTING, test_thread_open_heap_nonexist , DONOTING)
/// the num of success of status is different and core more times///
REGIST_FUNC(DONOTING, test_thread_open_heap_nonexist_zero , DONOTING)

REGIST_FUNC(DONOTING, test_thread_open_heap_meta , DONOTING)
REGIST_FUNC(DONOTING, test_thread_close_heap_exist , DONOTING)
REGIST_FUNC(DONOTING, test_thread_close_heap_nonexist , DONOTING)

//////对表中内容的操作（插入删除更新和查找）///
REGIST_FUNC(DONOTING, test_thread_heap_insert , DONOTING)
//fail once time
REGIST_FUNC(DONOTING, test_thread_heap_insert_large , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_insert_large_find , DONOTING)
//
REGIST_FUNC(DONOTING, test_thread_heap_delete , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_delete_large , DONOTING)

REGIST_FUNC(DONOTING, test_thread_heap_update , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_update_large , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_update_large_find , DONOTING)

REGIST_FUNC(DONOTING, test_thread_heap_find , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_delete_find , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_find_large , DONOTING)

REGIST_FUNC(DONOTING, test_thread_heap_getNext , DONOTING)
REGIST_FUNC(DONOTING, test_thread_index_find , DONOTING)
REGIST_FUNC(DONOTING, test_thread_heap_getNext_large , DONOTING)

REGIST_FUNC(DONOTING, test_thread_index_find_del , DONOTING)
REGIST_FUNC(DONOTING, test_thread_index_getNext , DONOTING)
REGIST_FUNC(DONOTING, test_thread_index_getNext_large , DONOTING)
 
BOOST_FIXTURE_TEST_SUITE(TransPersistence,EmptyFixture)
REGIST_FUNC(DONOTING, test_trans_persistence_insert , DONOTING)
REGIST_FUNC(DONOTING, test_trans_persistence_update , DONOTING)
REGIST_FUNC(DONOTING, test_trans_persistence_delete , DONOTING)
BOOST_AUTO_TEST_SUITE_END()

REGIST_FUNC(DONOTING, test_get_current_transaction, DONOTING)

BOOST_AUTO_TEST_SUITE(HEAP_MULTI_INSERT)
REGIST_FUNC(DONOTING, test_heap_multi_insert_frm_items, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_frm_entrySet, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_frm_items_index, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_frm_entrySet_index, DONOTING)

REGIST_FUNC(DONOTING, test_heap_multi_insert_identify, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_identify_index, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_frm_entrySet_ShutDown, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_frm_items_ShutDown, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_frm_items_ShutDown_index, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_frm_entrySet_ShutDown_index, DONOTING)

REGIST_FUNC(DONOTING, test_heap_multi_insert_multi, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_multi_index, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_multi_frmEntrySet, DONOTING)
REGIST_FUNC(DONOTING, test_heap_multi_insert_multi_frmEntrySet_index, DONOTING)

REGIST_FUNC(DONOTING, test_heap_multi_insert_toast, DONOTING)

REGIST_FUNC(DONOTING, test_heap_multi_insert_thread_transaction, DONOTING)

BOOST_AUTO_TEST_SUITE_END()

REGIST_FUNC(DONOTING,test_bitmap_scan_new,DONOTING)
REGIST_FUNC(DONOTING,test_bitmap_scan_hot_update,DONOTING)

REGIST_FUNC(DONOTING,test_getalldatabaseInfo_tbspc,DONOTING)
REGIST_FUNC(DONOTING,test_thread_create_and_open_heap,DONOTING)
REGIST_FUNC(DONOTING, test_thread_create_insert_index_dll, DONOTING)
REGIST_FUNC(DONOTING, test_thread_create_insert_index_dll2, DONOTING)

END_DEBUG(DONOTING)
