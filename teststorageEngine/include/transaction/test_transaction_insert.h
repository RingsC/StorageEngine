/**
* @file test_transaction_insert.h
* @brief 
* @author ¿Ó È‰∆
* @date 2011-11-24 16:10:54
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANSACTION_INSERT_H
#define _TEST_TRANSACTION_INSERT_H
extern void begin_transaction();
extern void end_transaction();
extern void userAbort_transaction();
extern void begin_transaction_after_abort();
extern int RID;

bool test_transaction_insert_000();
bool test_transaction_insert_001();
bool test_transaction_insert_002_step1();
bool test_transaction_insert_002_step2();

#define insert_transaction_data "insert_transaction_data_"
#define insert_transaction_rows 10
#endif 
