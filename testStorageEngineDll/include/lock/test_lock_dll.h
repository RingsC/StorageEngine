#ifndef __TEST_LOCK_DLL_H__
#define __TEST_LOCK_DLL_H__
#include "XdbLock.h"

void test_Spin_lock1(int* i ,XdbLock* testlock);
void test_Spin_lock2(int* i ,XdbLock* testlock);
void test_Spin_lock3(int* i ,XdbLock* testlock);
void test_Spin_lock4(int* i ,XdbLock* testlock);
bool TestSpinLock(void);
void test_mutex_lock_exclusive1(int* i ,XdbLock* testlock);
void test_mutex_lock_exclusive2(int* i ,XdbLock* testlock);
void test_mutex_lock_exclusive3(int* i ,XdbLock* testlock);
void test_mutex_lock_exclusive4(int* i ,XdbLock* testlock);
bool TestMutexLockExclusive(void);
void test_mutex_lock_share1(int* i ,XdbLock* testlock );
void test_mutex_lock_share2(int* i ,XdbLock* testlock);
void test_mutex_lock_share3(int* i ,XdbLock* testlock);
void test_mutex_lock_share4(int* i ,XdbLock* testlock);
bool TestMutexLockShare(void);
void test_first_transaction(int *i);
void test_second_transaction(int *i);
void test_third_transaction(int *i );
bool TestTransactionLock(void);
#endif

