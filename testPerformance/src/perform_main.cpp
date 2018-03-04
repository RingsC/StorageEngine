#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include "perform_heap.h"
#include "perform_frame.h"
#include "perform_bulk_insert.h"
#include "perform_sort_index.h"

BEGIN_DEBUG(DONOTING)
BOOST_AUTO_TEST_SUITE(PERFORM_COMPARE)
REGIST_FUNC(DONOTING,test_perform_heap_insert,DONOTING)
REGIST_FUNC(DONOTING,test_perform_heap_insert_index,DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_insert_concurrent,DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_insert_concurrent_index, DONOTING)

REGIST_FUNC(DONOTING, test_perform_heap_traverse, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_traverse_index, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_traverse_concurrent, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_traverse_concurrent_index, DONOTING)

REGIST_FUNC(DONOTING,test_perform_heap_delete , DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_delete_index, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_delete_concurrent, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_delete_concurrent_index, DONOTING)

REGIST_FUNC(DONOTING, test_perform_heap_update, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_update_index, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_update_concurrent, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_update_concurrent_index, DONOTING)

REGIST_FUNC(DONOTING, test_perform_heap_prepare_data, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_query, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_prepare_data_index, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_query_index, DONOTING)

REGIST_FUNC(DONOTING, test_perform_heap_insert_trans, DONOTING)
REGIST_FUNC(DONOTING, test_perform_heap_insert_trans_index, DONOTING)

REGIST_FUNC(DONOTING, test_perform_heap_query_trans_index, DONOTING)

REGIST_FUNC(DONOTING, test_perform_bulk_insert, DONOTING)
REGIST_FUNC(DONOTING, test_perform_bulk_insert_index, DONOTING)

REGIST_FUNC(DONOTING, test_perform_normal_insert, DONOTING)
REGIST_FUNC(DONOTING, test_perform_normal_insert_index, DONOTING)

REGIST_FUNC(DONOTING, test_perform_sort_index_prepare, DONOTING)
REGIST_FUNC(DONOTING, test_perform_multi_insert_index, DONOTING)
REGIST_FUNC(DONOTING, test_perform_multi_insert_index_1, DONOTING)
REGIST_FUNC(DONOTING, test_perform_sort_index_query, DONOTING)
REGIST_FUNC(DONOTING, test_perform_bitmap_scan, DONOTING)


BOOST_AUTO_TEST_SUITE_END()
END_DEBUG(DONOTING)