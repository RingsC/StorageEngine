bool test_heap_open_dll();
bool test_heap_multopen_dll();


////multi thread test///
bool test_thread_open_heap();
bool test_thread_open_heap_nonexist();
bool test_thread_open_heap_nonexist_zero();
bool test_thread_open_heap_index();
bool test_thread_open_heap_index_normal();
bool test_thread_open_heap_meta();
bool test_thread_open_heap_del();
bool test_thread_close_heap_exist();
bool test_thread_close_heap_meta();
bool test_thread_close_heap_nonexist();
bool test_thread_close_heap_nonexist_nonzero();
bool test_thread_del_heap();
bool test_thread_del_heap_nonexist_zero();
bool test_thread_del_heap_nonexist();
bool test_thread_del_heap_meta();
bool test_thread_del_heap_index();
bool test_thread_heap_insert();
bool test_thread_heap_delete();
bool test_thread_heap_delete_large();
bool test_thread_heap_delete_nonexist();
bool test_thread_heap_delete_find();
bool test_thread_heap_update();
bool test_thread_heap_update_large();
bool test_thread_heap_update_large_find();
bool test_thread_heap_update_nonexist();
bool test_thread_heap_find();
bool test_thread_heap_find_large();
bool test_thread_heap_getNext();
bool test_thread_heap_getNext_large();
bool test_thread_index_find();
bool test_thread_index_getNext();
bool test_thread_index_getNext_large();
bool test_thread_heap_insert_large();
bool test_thread_heap_insert_large_find();
bool test_thread_index_find_del();

bool test_replication_check();