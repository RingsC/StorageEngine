/*-------------------------------------------------------------------------
 *
 * xact.h
 *	  postgres transaction system definitions
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/access/xact.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef XACT_H
#define XACT_H

#include "access/xlog.h"
#include "nodes/pg_list.h"
#include "storage/relfilenode.h"
#include "utils/timestamp.h"

#ifdef FOUNDER_XDB_SE
#include "utils/resowner.h"
#include "storage/lock.h"
#include "storage/predicate_internals.h"
#include "storage/sinval.h"
#include "access/nbtree.h"
#include "access/hash.h"
#endif //FOUNDER_XDB_SE

#include <pthread.h>


/*
 * Xact isolation levels
 */
#define PGPROC_MAX_CACHED_SUBXIDS 64
#define XACT_READ_UNCOMMITTED	0
#define XACT_READ_COMMITTED		1
#define XACT_REPEATABLE_READ	2
#define XACT_SERIALIZABLE		3
#define DefaultXactIsoLevel (CurrentTransactionState->mTopLevelState->defaultXactIsoLevel)
#define XactIsoLevel  (CurrentTransactionState->mTopLevelState->xactIsoLevel)
#define MyXactAccessedTempRel (CurrentTransactionState->mTopLevelState->myXactAccessedTempRel)


/*
 * We implement three isolation levels internally.
 * The two stronger ones use one snapshot per database transaction;
 * the others use one snapshot per statement.
 * Serializable uses predicate locks in addition to snapshots.
 * These macros should be used to check which isolation level is selected.
 */
#define IsolationUsesXactSnapshot() (XactIsoLevel >= XACT_REPEATABLE_READ)
#define IsolationIsSerializable() (XactIsoLevel == XACT_SERIALIZABLE)

/* Xact read-only state */
#define DefaultXactReadOnly  (CurrentTransactionState->mTopLevelState->defaultXactReadOnly)
#define XactReadOnly (CurrentTransactionState->mTopLevelState->xactReadOnly)

/*
 * Xact is deferrable -- only meaningful (currently) for read only
 * SERIALIZABLE transactions
 */
extern bool DefaultXactDeferrable;
extern bool XactDeferrable;
typedef enum
{
	SYNCHRONOUS_COMMIT_OFF,		/* asynchronous commit */
	SYNCHRONOUS_COMMIT_LOCAL_FLUSH,		/* wait for local flush only */
	SYNCHRONOUS_COMMIT_REMOTE_FLUSH		/* wait for local and remote flush */
}	SyncCommitLevel;

/* Define the default setting for synchonous_commit */
#define SYNCHRONOUS_COMMIT_ON	SYNCHRONOUS_COMMIT_REMOTE_FLUSH

/* Synchronous commit level */
//extern int	synchronous_commit;
#define synchronous_commit (CurrentTransactionState->mTopLevelState->nSynchronous_commit)

/* Kluge for 2PC support */

/*
 *	start- and end-of-transaction callbacks for dynamically loaded modules
 */
typedef enum
{
	XACT_EVENT_COMMIT,
	XACT_EVENT_ABORT,
	XACT_EVENT_PREPARE
} XactEvent;

typedef void (*XactCallback) (XactEvent event, void *arg);

typedef enum
{
	SUBXACT_EVENT_START_SUB,
	SUBXACT_EVENT_COMMIT_SUB,
	SUBXACT_EVENT_ABORT_SUB
} SubXactEvent;

typedef void (*SubXactCallback) (SubXactEvent event, SubTransactionId mySubid,
									SubTransactionId parentSubid, void *arg);


/* ----------------
 *		transaction-related XLOG entries
 * ----------------
 */

/*
 * XLOG allows to store some information in high 4 bits of log
 * record xl_info field
 */
#define XLOG_XACT_COMMIT			0x00
#define XLOG_XACT_PREPARE			0x10
#define XLOG_XACT_ABORT				0x20
#define XLOG_XACT_COMMIT_PREPARED	0x30
#define XLOG_XACT_ABORT_PREPARED	0x40
#define XLOG_XACT_ASSIGNMENT		0x50

typedef struct xl_xact_assignment
{
	TransactionId xtop;			/* assigned XID's top-level XID */
	int			nsubxacts;		/* number of subtransaction XIDs */
	TransactionId xsub[1];		/* assigned subxids */
} xl_xact_assignment;

#define MinSizeOfXactAssignment offsetof(xl_xact_assignment, xsub)

typedef struct xl_xact_commit
{
	TimestampTz xact_time;		/* time of commit */
	uint32		xinfo;			/* info flags */
	int			nrels;			/* number of RelFileNodes */
	int			nsubxacts;		/* number of subtransaction XIDs */
	int			nmsgs;			/* number of shared inval msgs */
	Oid			dbId;			/* MyDatabaseId */
	Oid			tsId;			/* MyDatabaseTableSpace */
	/* Array of RelFileNode(s) to drop at commit */
	RelFileNode xnodes[1];		/* VARIABLE LENGTH ARRAY */
	/* ARRAY OF COMMITTED SUBTRANSACTION XIDs FOLLOWS */
	/* ARRAY OF SHARED INVALIDATION MESSAGES FOLLOWS */
} xl_xact_commit;

#define MinSizeOfXactCommit offsetof(xl_xact_commit, xnodes)

/*
 * These flags are set in the xinfo fields of WAL commit records,
 * indicating a variety of additional actions that need to occur
 * when emulating transaction effects during recovery.
 * They are named XactCompletion... to differentiate them from
 * EOXact... routines which run at the end of the original
 * transaction completion.
 */
#define XACT_COMPLETION_UPDATE_RELCACHE_FILE	0x01
#define XACT_COMPLETION_FORCE_SYNC_COMMIT		0x02

/* Access macros for above flags */
#define XactCompletionRelcacheInitFileInval(xlrec)	((xlrec)->xinfo & XACT_COMPLETION_UPDATE_RELCACHE_FILE)
#define XactCompletionForceSyncCommit(xlrec)		((xlrec)->xinfo & XACT_COMPLETION_FORCE_SYNC_COMMIT)

typedef struct xl_xact_abort
{
	TimestampTz xact_time;		/* time of abort */
	int			nrels;			/* number of RelFileNodes */
	int			nsubxacts;		/* number of subtransaction XIDs */
	/* Array of RelFileNode(s) to drop at abort */
	RelFileNode xnodes[1];		/* VARIABLE LENGTH ARRAY */
	/* ARRAY OF ABORTED SUBTRANSACTION XIDs FOLLOWS */
} xl_xact_abort;

/* Note the intentional lack of an invalidation message array c.f. commit */

#define MinSizeOfXactAbort offsetof(xl_xact_abort, xnodes)

/*
 * COMMIT_PREPARED and ABORT_PREPARED are identical to COMMIT/ABORT records
 * except that we have to store the XID of the prepared transaction explicitly
 * --- the XID in the record header will be for the transaction doing the
 * COMMIT PREPARED or ABORT PREPARED command.
 */

typedef struct xl_xact_commit_prepared
{
	TransactionId xid;			/* XID of prepared xact */
	xl_xact_commit crec;		/* COMMIT record */
	/* MORE DATA FOLLOWS AT END OF STRUCT */
} xl_xact_commit_prepared;

#define MinSizeOfXactCommitPrepared offsetof(xl_xact_commit_prepared, crec.xnodes)

typedef struct xl_xact_abort_prepared
{
	TransactionId xid;			/* XID of prepared xact */
	xl_xact_abort arec;			/* ABORT record */
	/* MORE DATA FOLLOWS AT END OF STRUCT */
} xl_xact_abort_prepared;

#define MinSizeOfXactAbortPrepared offsetof(xl_xact_abort_prepared, arec.xnodes)


/* ----------------
 *		extern definitions
 * ----------------
 */
#ifdef FOUNDER_XDB_SE
extern TransactionId Fxdb_GetTmpCurrentTransactionId(void);
#endif //FOUNDER_XDB_SE
extern bool IsTransactionState(void);
extern bool IsAbortedTransactionBlockState(void);
extern TransactionId GetTopTransactionId(void);
extern TransactionId GetTopTransactionIdIfAny(void);
extern TransactionId GetCurrentTransactionId(void);
extern TransactionId GetCurrentTransactionIdIfAny(void);
extern SubTransactionId GetCurrentSubTransactionId(void);
extern CommandId GetCurrentCommandId(bool used);
extern TimestampTz GetCurrentTransactionStartTimestamp(void);
extern TimestampTz GetCurrentStatementStartTimestamp(void);
extern TimestampTz GetCurrentTransactionStopTimestamp(void);
extern void SetCurrentStatementStartTimestamp(void);
extern int	GetCurrentTransactionNestLevel(void);
extern bool TransactionIdIsCurrentTransactionId(TransactionId xid);
#ifndef FOUNDER_XDB_SE
extern void CommandCounterIncrement(void);
#else
extern void CommandCounterIncrement(bool invalidMsg = false);
#endif //FOUNDER_XDB_SE
extern void ForceSyncCommit(void);
extern void StartTransactionCommand(void);
extern void CommitTransactionCommand(void);
extern void AbortCurrentTransaction(void);
extern void BeginTransactionBlock(void);
extern bool EndTransactionBlock(void);
extern bool PrepareTransactionBlock(char *gid);
extern void UserAbortTransactionBlock(void);
extern void ReleaseSavepoint(List *options);
extern void DefineSavepoint(char *name);
extern void RollbackToSavepoint(List *options);
extern void BeginInternalSubTransaction(char *name);
extern void ReleaseCurrentSubTransaction(void);
extern void RollbackAndReleaseCurrentSubTransaction(void);
extern bool IsSubTransaction(void);
extern bool IsTransactionBlock(void);
extern bool IsTransactionOrTransactionBlock(void);
extern char TransactionBlockStatusCode(void);
extern void AbortOutOfAnyTransaction(void);
extern void PreventTransactionChain(bool isTopLevel, const char *stmtType);
extern void RequireTransactionChain(bool isTopLevel, const char *stmtType);
extern bool IsInTransactionChain(bool isTopLevel);
extern void RegisterXactCallback(XactCallback callback, void *arg);
extern void UnregisterXactCallback(XactCallback callback, void *arg);
extern void RegisterSubXactCallback(SubXactCallback callback, void *arg);
extern void UnregisterSubXactCallback(SubXactCallback callback, void *arg);

extern int	xactGetCommittedChildren(TransactionId **ptr);

extern void xact_redo(XLogRecPtr lsn, XLogRecord *record);
extern void xact_desc(StringInfo buf, uint8 xl_info, char *rec);
extern void initTopXact();

#ifdef FOUNDER_XDB_SE
typedef enum TransState
{
	TRANS_DEFAULT,				/* idle */
	TRANS_START,				/* transaction starting */
	TRANS_INPROGRESS,			/* inside a valid transaction */
	TRANS_COMMIT,				/* commit in progress */
	TRANS_ABORT,				/* abort in progress */
	TRANS_PREPARE				/* prepare in progress */
} TransState;

/*
 *	transaction block states - transaction state of client queries
 *
 * Note: the subtransaction states are used only for non-topmost
 * transactions; the others appear only in the topmost transaction.
 */
typedef enum TBlockState
{
	/* not-in-transaction-block states */
	TBLOCK_DEFAULT,				/* idle */
	TBLOCK_STARTED,				/* running single-query transaction */

	/* transaction block states */
	TBLOCK_BEGIN,				/* starting transaction block */
	TBLOCK_INPROGRESS,			/* live transaction */
	TBLOCK_END,					/* COMMIT received */
	TBLOCK_ABORT,				/* failed xact, awaiting ROLLBACK */
	TBLOCK_ABORT_END,			/* failed xact, ROLLBACK received */
	TBLOCK_ABORT_PENDING,		/* live xact, ROLLBACK received */
	TBLOCK_PREPARE,				/* live xact, PREPARE received */

	/* subtransaction states */
	TBLOCK_SUBBEGIN,			/* starting a subtransaction */
	TBLOCK_SUBINPROGRESS,		/* live subtransaction */
	TBLOCK_SUBEND,				/* RELEASE received */
	TBLOCK_SUBABORT,			/* failed subxact, awaiting ROLLBACK */
	TBLOCK_SUBABORT_END,		/* failed subxact, ROLLBACK received */
	TBLOCK_SUBABORT_PENDING,	/* live subxact, ROLLBACK received */
	TBLOCK_SUBRESTART,			/* live subxact, ROLLBACK TO received */
	TBLOCK_SUBABORT_RESTART		/* failed subxact, ROLLBACK TO received */
} TBlockState;

struct xllist
{
	XLogRecData *head;			/* first data block in the chain */
	XLogRecData *tail;			/* last block in chain */
	uint32		bytes_free;		/* free bytes left in tail block */
	uint32		total_len;		/* total data bytes in chain */
};

struct XidCache
{
	bool		overflowed;
	int			nxids;
	TransactionId xids[PGPROC_MAX_CACHED_SUBXIDS];
};

struct TopLevelState
{
	VirtualTransactionId vid;
	int			defaultXactIsoLevel;
	int			xactIsoLevel;

	bool		defaultXactReadOnly;
	bool		xactReadOnly;

	int			commitDelay;
	int			commitSiblings;
	bool		myXactAccessedTempRel;

	int	mUnreportedXids;
	TransactionId munreportedXids[PGPROC_MAX_CACHED_SUBXIDS];

	SubTransactionId mCurrentSubTransactionId;
	CommandId mCurrentCommandId;
	bool mCurrentCommandIdUsed;


	TimestampTz mXactStartTimestamp;
	TimestampTz mStmtStartTimestamp;
	TimestampTz mXactStopTimestamp;

	char *mPrepareGID;
	bool bForceSyncCommit;

	MemoryContext mTransactionAbortContext;

	LocalTransactionId lxid;	/* local id of top-level transaction currently
								 * being executed by this proc, if running;
								 * else InvalidLocalTransactionId */

	TransactionId xid;			/* id of top-level transaction currently being
								 * executed by this proc, if running and XID
								 * is assigned; else InvalidTransactionId */

	TransactionId xmin;			/* minimal running XID as it was when we were
								 * starting our xact, excluding LAZY VACUUM:
								 * vacuum must not remove tuples deleted by
								 * xid >= xmin ! */

	 /*
     * CurrentSnapshot points to the only snapshot taken in a serializable
     * transaction, and to the latest one taken in a read-committed transaction.
     * SecondarySnapshot is a snapshot that's always up-to-date as of the current
     * instant, even on a serializable transaction.  It should only be used for
     * special-purpose code (say, RI checking.)
     *
     * These SnapshotData structs are static to simplify memory allocation
     * (see the hack in GetSnapshotData to avoid repeated malloc/free).
     */

	SnapshotData mCurrentSnapshotData;  //default <=> HeapTupleSatisfiesMVCC
	SnapshotData mSecondarySnapshotData;

	Snapshot mCurrentSnapshot;
	Snapshot mSecondarySnapshot;

	/* Dynahash */
    #define MAX_SEQ_SCANS 100

	struct HTAB *mSeq_scan_tables[MAX_SEQ_SCANS];	/* tables being scanned */
	int	mSeq_scan_level[MAX_SEQ_SCANS];		/* subtransaction nest level */
	int	mNum_seq_scans;

   /*: comes from catalog/storage.c 
    holding all the pendingDeletes*/
    struct PendingRelDelete *mPendingDeletes;

	    /*
     * How many snapshots is resowner.c tracking for us?
     *
     * Note: for now, a simple counter is enough.  However, if we ever want to be
     * smarter about advancing our MyProc->xmin we will need to be more
     * sophisticated about this, perhaps keeping our own list of snapshots.
     */
    int	nRegisteredSnapshots;

    /* first GetTransactionSnapshot call in a transaction? */
    bool		bFirstSnapshotSet;

    /*
     * Remembers whether this transaction registered a serializable snapshot at
     * start.  We cannot trust FirstSnapshotSet in combination with
     * IsXactIsoLevelSerializable, because GUC may be reset before us.
     */
    bool bRegistered_serializable;


    /* Top of the stack of active snapshots */
    struct ActiveSnapshotElt *mActiveSnapshot;

    /*:
    * This flag lets us optimize away work in AtEO(Sub)Xact_RelationCache().
    */
    bool bNeed_eoxact_work;

    struct TransInvalidationInfo *mTransInvalInfo;// = NULL; used by invalidation (inval.c)
    
    /*
     * During prepare, the state file is assembled in memory before writing it
     * to WAL and the actual state file.  We use a chain of XLogRecData blocks
     * so that we will be able to pass the state file contents directly to
     * XLogInsert.
     */
    struct xllist mRecords;

   
    /* 
     * This array is moved from BufferTlsState to here because of the new execution engine architecture. In the old architecture,
     * a txn is executed entirely by a single thread, but in the new architecture, plansteps of any statement of a txn may be
     * executed by multiple threads and even concurrently. Consequently the following case may appear:
     * Thread TH1 executes planstep P1 of transaction T1, at the end of P1, buffer 3's private refcount goes to 0 as expected;
     * Thread TH1 executes planstep P2 of transaction T2, and P2 is a commit plan step, thus resource owner will release buffers, and it sees buffer 3 should be released, but it sees TH1's private refcount[3] is 0, which is wrong, an assert will occur.
     *
     * But actually this is caused by the fact that the private refcount array of TH1 was accessed not only by T2, but also by T1, and possibly more other transactions. 
     * In order to avoid the above issue, we need to move PrivateRefCount into txn. And we need to use a spinlock to protect access
     * to this array since multiple threads may execute the same txn concurrently. The problem with this approach is that it will
     * take up a lot more memory --- each txn has such an array, and each array takes up (by default) 8192*4 bytes. If this turns out
     * to be a big issue, we will have to drop this array entirely. The benifit of this array is to avoid frequently lwlock ops to access
     * the shared buffer header, but since we now need spinlock ops too, and might consume too much memory, the cost may be too high. -- dazhao
     */ 
    int32	   *nPrivateRefCount;

    struct PgStat_SubXactStatus *pStatXactStack;// = NULL;  This txn's table stats stack base. Only a top txn has a valid pointer, which points at the lowest subtxn's stat info. In this structure we can get higher txn's structures.

 
	//for multi transaction
	struct mXactCacheEnt *mXactCache;
	MemoryContext mXactContext;
	
	//these values is set by GetCurrentSnapshot
	//TODO:For one-transaction multi-thread, add spin lock protection 
	//when using these fields 
	TransactionId mRecentXmin;// = FirstNormalTransactionId;
	TransactionId mRecentGlobalXmin;// = InvalidTransactionId;

	struct XidCache subxids;	/* cache for subtransaction XIDs */

	bool		inCommit;		/* true if within commit critical section */	

	//uint8		vacuumFlags;	/* vacuum-related flags, see above */

	List *mOnCommits;
	HTAB *mLocalLOInRelHash;
    struct TabStatusArray *mPgStatTabList;

	SERIALIZABLEXACT *mMySerializableXact;
	bool mMyXactDidWrite;

	HTAB *mLocalPredicateLockHash;

	/*
	* An array of cmin,cmax pairs, indexed by combo command id.
	* To convert a combo cid to cmin and cmax, you do a simple array lookup.
	*/
	HTAB *mComboHash;
	typedef struct ComboCidKeyData *ComboCidKey;
	ComboCidKey mComboCids;
	int	nUsedComboCids;	/* number of elements in comboCids */
	int	nSizeComboCids;	/* allocated size of array */

	int nSynchronous_commit;

	HTAB *mLockMethodLocalHash;

	struct FdInTransInfo* pFdInfo;
	struct SinvalTlsState*  pSinvalState;

	Oid			databaseId;
	struct LocalBufferInfo* pLocalBufInfo;
	PGPROC *lockProc;
	SharedInvalidationMessage *mSharedInvalidMessagesArray;
	int	mNumSharedInvalidMessagesArray;
	int	mMaxSharedInvalidMessagesArray;
	TransactionId mTransactionXmin;

	/*System index reindexing support*/
	Oid	ncurrentlyReindexedHeap;
	Oid	ncurrentlyReindexedIndex;
	List *mpendingReindexedIndexes;

	/*hash index*/
	HashScanList pHashScans;

	/* node cache */
	HTAB *mNodeCache;

	/* node id cache */
	HTAB *mNodeIDCache;
};

typedef struct TransactionStateData
{
	pthread_t pid;
	TransactionId transactionId;	/* my XID, or Invalid if none */
	SubTransactionId subTransactionId;	/* my subxact ID */
	char	   *name;			/* savepoint name, if any */
	int			savepointLevel; /* savepoint level */
	TransState	state;			/* low-level state */
	TBlockState blockState;		/* high-level state */
	int			nestingLevel;	/* transaction nesting depth */
	int			gucNestLevel;	/* GUC context nesting depth */
	MemoryContext curTransactionContext;		/* my xact-lifetime context */
	ResourceOwner curTransactionOwner;	/* my query resources */
	TransactionId *childXids;	/* subcommitted child XIDs, in XID order */
	int			nChildXids;		/* # of subcommitted child XIDs */
	int			maxChildXids;	/* allocated size of childXids[] */
	Oid			prevUser;		/* previous CurrentUserId setting */
	int			prevSecContext; /* previous SecurityRestrictionContext */
	bool		prevXactReadOnly;		/* entry-time xact r/o state */
	bool		startedInRecovery;		/* did we start in recovery? */
	struct TransactionStateData *parent;		/* back link to parent */
    struct TransactionStateData *top;

    struct TopLevelState* mTopLevelState;

#define NUM_BTSOD_PER_TXN 4
	/* used by nbtree index scan, create one here for all scans within this txn to use for better performance.
	 * This structure is over 20K and in one txn sometimes there can be millions of new scans and at the same
	 * time there can be multiple scans running, and it's observed that performance is impacted a lot in such cases.
	 * thus we put it here instead of allocating for each scan. This is a break of module isolation but performance
	 * is more important!
	 * in a txn there can be multiple scans started at the same time, if this isn't enough, alloc as needed 
	 * and link to the end*/
	BTScanOpaqueData btsod[NUM_BTSOD_PER_TXN];
} TransactionStateData;

typedef TransactionStateData *TransactionState;
 

extern TransactionState get_top_transaction_state();
extern TransactionState get_current_transaction_state();
extern void set_top_transaction_state(TransactionState top_stransaction_state);
extern void set_current_transaction_state(TransactionState cuurent_stransaction_state);

extern void setXactIsoLevel(int level);
extern int getXactIsoLevel();
extern TransactionState GetDefaultXact();

extern BTScanOpaque allocBTScanOpaqueStruct(TransactionState txn);
extern void deAllocBTScanOpaqueStruct(BTScanOpaque btso);

#define CurrentTransactionState (MyProc->mCurrentTransState)
#define CurrentTopLevelState (CurrentTransactionState->mTopLevelState)
#endif //FOUNDER_XDB_SE

#endif   /* XACT_H */
