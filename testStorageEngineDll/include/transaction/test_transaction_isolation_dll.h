#ifndef __TEST_TRANSACTION_ISOLATION_DLL_H__
#define __TEST_TRANSACTION_ISOLATION_DLL_H__
void test_reads_heap_insert_data(int *i);
void test_reads_heap_select_data(int *i);
void test_reads_heap_update_data(int *i);
bool TestTransactionReadIsolation(void);
void test_Repeatable_heap_insert_data(int *i);
void test_Repeatable_heap_select_data(int *i);
void test_Repeatable_heap_update_data(int *i);
bool TestTransactionRepeatableIsolation(void);
void test_Serializable_heap_insert_data(int *i);
void test_Serializable_heap_select_data(int *i);
void test_Serializable_heap_update_data(int *i);
bool TestTransactionSerializableIsolation(void);
#endif
