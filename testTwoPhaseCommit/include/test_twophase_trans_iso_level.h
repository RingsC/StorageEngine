#ifndef TEST_TWOPHASE_TRANS_ISO_LEVEL_H
#define TEST_TWOPHASE_TRANS_ISO_LEVEL_H

bool test_twophase_trans_read_commit();

bool test_twophase_trans_repeatable_update();
bool test_twophase_trans_repeatable_delete();
bool test_twophase_trans_repeatable_insert();

bool test_twophase_trans_serializable_update();
bool test_twophase_trans_serializable_insert();
bool test_twophase_trans_serializable_delete();

bool test_twophase_trans_concurrent_update();
bool test_twophase_trans_concurrent_delete();
bool test_twophase_high_concurrent();

bool test_twophase_multi_task();
bool test_twophase_ops_after_prepare();
bool test_twophase_lock_conflict();
#endif//TEST_TWOPHASE_TRANS_ISO_LEVEL_H
