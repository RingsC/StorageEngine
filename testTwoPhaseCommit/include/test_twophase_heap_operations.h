#ifndef TEST_TWOPHASE_HEAP_OPERATIONS_H
#define TEST_TWOPHASE_HEAP_OPERATIONS_H

//heap create
bool test_twophase_heap_create_prepare();
bool test_twophase_heap_create_rollback();
bool test_twophase_heap_create_rollback_ex();

//heap insert cases
bool test_twophase_heap_insert();
bool test_twophase_heap_insert_rollback_ex();
bool test_twophase_heap_insert_prepare();
bool test_twophase_heap_insert_commit();
bool test_twophase_heap_insert_rollback();
bool test_twophase_heap_insert_prepare_index();
bool test_twophase_heap_insert_commit_index();
bool test_twophase_heap_insert_index();
//heap delete
bool test_twophase_heap_delete_prepare();
bool test_twophase_heap_delete_commit();
bool test_twophase_heap_delete_rollback();
bool test_twophase_heap_delete();
bool test_twophase_heap_delete_rollback_ex();
bool test_twophase_heap_delete_prepare_index();
bool test_twophase_heap_delete_commit_index();
bool test_twophase_heap_delete_index();
//heap update
bool test_twophase_heap_update_prepare();
bool test_twophase_heap_update_commit();
bool test_twophase_heap_update_rollback();
bool test_twophase_heap_update();
bool test_twophase_heap_update_rollback_ex();
bool test_twophase_heap_update_prepare_index();
bool test_twophase_heap_update_commit_index();
bool test_twophase_heap_update_index();
//create index 
bool test_twophase_create_index_prepare();
bool test_twophase_create_index_commit();
bool test_twophase_create_index_rollback();
bool test_twophase_create_index();
bool test_twophase_create_index_rollback_ex();

//thread switch
bool test_twophase_heap_insert_thread();
bool test_twophase_heap_insert_index_thread();
bool test_twophase_heap_delete_thread();
bool test_twophase_heap_delete_index_thread();
bool test_twophase_heap_update_thread();
bool test_twophase_heap_update_index_thread();
//
#endif//TEST_TWOPHASE_HEAP_OPERATIONS_H
