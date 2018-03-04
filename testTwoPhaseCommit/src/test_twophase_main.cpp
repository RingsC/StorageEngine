#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include "test_twophase_frame.h"
#include "test_twophase_heap_operations.h"
#include "test_twophase_trans_iso_level.h"

BEGIN_DEBUG(DONOTING)

REGIST_FUNC(DONOTING, test_twophase_heap_create_prepare, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_create_rollback, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_create_rollback_ex, DONOTING)

REGIST_FUNC(DONOTING, test_twophase_heap_insert_prepare, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_insert_commit, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_insert_rollback, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_insert, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_insert_rollback_ex, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_insert_prepare_index, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_insert_commit_index, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_insert_index, DONOTING)

REGIST_FUNC(DONOTING, test_twophase_heap_delete_prepare, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_delete_commit, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_delete_rollback, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_delete, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_delete_rollback_ex, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_delete_prepare_index, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_delete_commit_index, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_delete_index, DONOTING)

REGIST_FUNC(DONOTING, test_twophase_heap_update_prepare, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_update_commit, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_update_rollback, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_update, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_update_rollback_ex, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_update_prepare_index, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_update_commit_index, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_update_index, DONOTING)

REGIST_FUNC(DONOTING, test_twophase_create_index_prepare, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_create_index_commit, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_create_index_rollback, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_create_index, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_create_index_rollback_ex, DONOTING)

REGIST_FUNC(DONOTING, test_twophase_ops_after_prepare, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_lock_conflict, DONOTING)

BOOST_AUTO_TEST_SUITE(MULTI_THREAD)
REGIST_FUNC(DONOTING, test_twophase_heap_insert_thread, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_insert_index_thread, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_delete_thread, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_delete_index_thread, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_update_thread, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_heap_update_index_thread, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_multi_task, DONOTING)
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(MULTI_TRANS)
REGIST_FUNC(DONOTING, test_twophase_trans_read_commit, DONOTING)

REGIST_FUNC(DONOTING, test_twophase_trans_repeatable_update, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_trans_repeatable_delete, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_trans_repeatable_insert, DONOTING)

REGIST_FUNC(DONOTING, test_twophase_trans_serializable_update, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_trans_serializable_insert, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_trans_serializable_delete, DONOTING)

REGIST_FUNC(DONOTING, test_twophase_trans_concurrent_update, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_trans_concurrent_delete, DONOTING)
REGIST_FUNC(DONOTING, test_twophase_high_concurrent, DONOTING)

BOOST_AUTO_TEST_SUITE_END()

END_DEBUG(DONOTING)