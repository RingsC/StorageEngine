#ifndef __TEST_TRANS_PREPARE_H__
#define __TEST_TRANS_PREPARE_H__

extern bool test_TransPrepare_Insert_Commit_P1(void);
extern bool test_TransPrepare_Insert_Commit_P2(void);
extern bool test_TransPrepare_Insert_Abort_P1(void);
extern bool test_TransPrepare_Insert_Abort_P2(void);

extern bool test_TransPrepare_CreateHeap_Commit_P1(void);
extern bool test_TransPrepare_CreateHeap_Commit_P2(void);
extern bool test_TransPrepare_CreateHeap_Abort_P1(void);
extern bool test_TransPrepare_CreateHeap_Abort_P2(void);

extern bool test_TransPrepare_Isolation_Commit_P1(void);
extern bool test_TransPrepare_Isolation_Commit_P2(void);
extern bool test_TransPrepare_Isolation_Abort_P1(void);
extern bool test_TransPrepare_Isolation_Abort_P2(void);

extern bool test_TransPrepare_SubTrans_Commit_P1(void);
extern bool test_TransPrepare_SubTrans_Commit_P2(void);
extern bool test_TransPrepare_SubTrans_Abort_P1(void);
extern bool test_TransPrepare_SubTrans_Abort_P2(void);

extern bool test_TransPrepare_DeadLock_Commit(void);
extern bool test_TransPrepare_DeadLock_Abort(void);

extern bool test_TransPrepare_Lock_ShareShare_P1(void);
extern bool test_TransPrepare_Lock_ShareShare_P2(void);
extern bool test_TransPrepare_Lock_ShareExclusive_P1(void);
extern bool test_TransPrepare_Lock_ShareExclusive_P2(void);
extern bool test_TransPrepare_Lock_ExclusiveShare_P1(void);
extern bool test_TransPrepare_Lock_ExclusiveShare_P2(void);
extern bool test_TransPrepare_Lock_ExclusiveExclusive_P1(void);
extern bool test_TransPrepare_Lock_ExclusiveExclusive_P2(void);

extern bool test_TransPrepare_Concurrency(void);
#endif
