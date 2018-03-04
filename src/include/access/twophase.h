/*-------------------------------------------------------------------------
 *
 * twophase.h
 *	  Two-phase-commit related declarations.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/twophase.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TWOPHASE_H
#define TWOPHASE_H

#include "access/xlogdefs.h"
#include "storage/backendid.h"
#include "storage/proc.h"
#include "utils/timestamp.h"

/*
 * GlobalTransactionData is defined in twophase.c; other places have no
 * business knowing the internal definition.
 */
typedef struct GlobalTransactionData *GlobalTransaction;

#ifdef FOUNDER_XDB_SE
struct GlobalPrepareTxnInfo 
{
	uint32 gxid;
	Oid    dbid;
};

#endif //FOUNDER_XDB_SE

#ifndef FOUNDER_XDB_SE
/* GUC variable */
extern THREAD_LOCAL int	max_prepared_xacts;
#endif //FOUNDER_XDB_SE

#ifdef FOUNDER_XDB_SE
/* GUC variable */
extern THREAD_LOCAL int	max_prepared_xacts;
#endif //FOUNDER_XDB_SE

extern Size TwoPhaseShmemSize(void);
extern void TwoPhaseShmemInit(void);

extern PGPROC *TwoPhaseGetDummyProc(TransactionId xid);
extern BackendId TwoPhaseGetDummyBackendId(TransactionId xid);

extern GlobalTransaction MarkAsPreparing(TransactionId xid, const char *gid,
				TimestampTz prepared_at,
				Oid owner, Oid databaseid);

extern void StartPrepare(GlobalTransaction gxact);
extern void EndPrepare(GlobalTransaction gxact);
extern bool StandbyTransactionIdIsPrepared(TransactionId xid);

extern TransactionId PrescanPreparedTransactions(TransactionId **xids_p,
							int *nxids_p);
extern void StandbyRecoverPreparedTransactions(bool overwriteOK);
extern void RecoverPreparedTransactions(void);

extern void RecreateTwoPhaseFile(TransactionId xid, void *content, int len);
extern void RemoveTwoPhaseFile(TransactionId xid, bool giveWarning);

extern void CheckPointTwoPhase(XLogRecPtr redo_horizon);

extern void FinishPreparedTransaction(const char *gid, bool isCommit);

#ifdef FOUNDER_XDB_SE
extern bool GlobalTransactionIdIsPrepared(char *gxid);
extern uint32 GetNextMaxPreparedGlobalTxnId();
extern int GetPreparedGlobalTxnId(GlobalPrepareTxnInfo** prepareList);
extern bool TransactionIdIsPrepared(TransactionId xid);
#endif //FOUNDER_XDB_SE

extern void AddPrepareXact2Array(TransactionId xid);

#endif   /* TWOPHASE_H */
