/**
* @file test_transaction_delete.h
* @brief 
* @author ª∆Í…
* @date 2011-11-24 16:09:03
* @version 1.0
* @copyright: founder.com
* @email: hs@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANSACTION_DELETE_H
#define _TEST_TRANSACTION_DELETE_H
extern void begin_transaction();
extern void end_transaction();
extern void userAbort_transaction();
extern void begin_transaction_after_abort();
extern int rid;
extern int THREAD_RID;

bool test_transaction_delete_atom();
bool test_transaction_delete_atom_halfdelete();


#endif 
