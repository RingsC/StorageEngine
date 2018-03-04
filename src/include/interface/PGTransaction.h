#ifndef PG_TRANSACTION_HPP
#define PG_TRANSACTION_HPP

#include <set>
#include <map>
#include "Transaction.h"
#include "access/transam.h"
#include "access/xact.h"
#include "MemoryContext.h"

namespace FounderXDB{
namespace StorageEngineNS {

class BaseEntrySet;
class SequenceFactory;
class XdbSequence;
class LargeObject;

class PGTransaction : public Transaction{
public:
	typedef void(*beginFunc)( void );
    ///constructor
    PGTransaction(const FXTransactionId transaction_id/* = InvalidTransactionId*/, IsolationLevel isolation_level = NO_ISOLATION_LEVEL_SPECIFIED
                  , beginFunc beg = PGTransaction::begin);

    ///destuctor
    virtual ~PGTransaction();

    /// Abort the transaction.
    virtual void abort();

		virtual void abortCurrent();

    /// Commit the transaction.
    virtual void commit();

    /// Prepare the transaction.
    virtual void prepare();

	bool isPrepared()
	{
		return m_isPrepared;
	}

    /// Get gloal transaction id.
    /// @param[out] id          global transaction id
    virtual FXTransactionId getTransactionId() const;

    /// Get isolation level of the transaction.
    /// @param[out] level       isolation level
    virtual IsolationLevel getIsolationLevel() const;
    virtual void setIsolationLevel(IsolationLevel level);
    virtual Transaction *startSubTransaction(IsolationLevel isolevel );

    TransactionState get_top_transaction_state();
    TransactionState get_current_transaction_state();
    void set_top_transaction_state(TransactionState top_stransaction_state);
    void set_current_transaction_state(TransactionState cuurent_stransaction_state);
	
	///return this transaction's associated MemoryContext
	virtual MemoryContext *getAssociatedMemoryContext() const;

    virtual void *getUserData() const { return userData_; }
    /// Set user data pointer.
    virtual void setUserData(void *ud) { userData_ = ud; }
    BaseEntrySet *getEntrySet(EntrySetID esid, DatabaseID dbid) const;
    void cacheEntrySet(EntrySetID esid, DatabaseID dbid, BaseEntrySet *bes);
	void cacheLargeObject(LargeObject *lo, bool isWrite);
	LargeObject *getRecentOpenLO(std::string name);
	std::map<std::pair<DatabaseID, std::string>, LargeObject*> *getRecentOpenLOHash();
	void unCacheLargeObject(const uint32 loid);
	void unCacheLargeObject(const char *name);
	LargeObject *getLargeObject(const char *name, bool isWrite) const;
	LargeObject *getLargeObject(const uint32 loid, bool isWrite) const;
	Snapshot getSnapshot(bool isNew = false);
#if 0
	//    void setSequenceFactory(CreateSequence *cs) {seqFactory_ = cs;}
	//    CreateSequence *getSequenceFactory() const { return seqFactory_;}
	//    XdbSequence *getSequence(SeqID seqId) const;
	//    void cacheSequence(SeqID seqId, XdbSequence *);
	//    void closeSequence(SeqID seqId);
	//    void closeCachedSequences();
#else
	//XdbSequence *getSequence(std::pair<DatabaseID, SeqID>) const;
    //void cacheSequence(std::pair<DatabaseID, SeqID>, std::pair<DatabaseID, std::string>, XdbSequence *);
    //void closeSequence(std::pair<DatabaseID, SeqID>);
    //void closeCachedSequences();
    XdbSequence *getSequence(DatabaseID dbid, SeqID seqId) const;
	XdbSequence *getSequence(DatabaseID dbid, const char *seqName) const;
	void cacheSequence(DatabaseID dbid, SeqID seqId,
		const char * seqName, XdbSequence *pSeq);
	void clearCachedSequence(DatabaseID dbid, SeqID seqId, const char * seqName);
	void clearCachedSequence(DatabaseID dbid, SeqID seqId);
	void clearCachedSequence(DatabaseID dbid, const char *seqName);
	//void closeSequence(DatabaseID dbid, SeqID seqId);
	//void closeSequence(DatabaseID dbid, std::string seqName);
	void closeCachedSequences();
#endif

    void closeEntrySet(EntrySetID esid, bool unlock = false, bool del = false);
	void closeCachedEntrySets(bool delObj = false); 

	void closeCacheLargeObject();
	void endStatementLargeObject();
	void setGXID(FXGlobalTransactionId gxid);

protected:  

    PGTransaction(const PGTransaction &other);
    /// overload operator =
    PGTransaction& operator = (const PGTransaction &other);
    static void begin( void );
    FXTransactionId m_transaction_id;
	FXGlobalTransactionId m_global_trans_id;
private:
	bool m_isPrepared;

    IsolationLevel m_isolation_level;

    TransactionState m_CurrentTransactionState;
    TransactionState m_TopTransactionState;

	MemoryContext *m_TransactionMemoryContext;
    void *userData_;
    // Caches all entry set objects opened with this txn. all entryset open calls
    // will fetch entry set objects from here first, and caches them here.
    // A txn always uses one database, so no need to use DatabaseID as part of the key.
	//
	// When commands in a txn executes in different threads one by one, the cached entryset objects
	// always uses the Relation handle of the thread where the Container or entryset is opened. 
	// this might be a problem in future ! XXXX  when error occur on this, either remove the Relation members in PGEntryset/PGIndexEntrySet, open it whenever needed(performance issue!)
	// OR whenever attaching to a new thread, refresh every entryset object's Relation member.
	//
	// the cached largeobjs don't have this issue because they don't internally store Relation handle, only store relation id.
    std::map<EntrySetID, BaseEntrySet *> myOpenEntrySets_;

	/* caches all opened seqs within this xact */
    std::map<std::pair<DatabaseID, SeqID>, XdbSequence *> m_myIDOpenSeqs;
    std::map<std::pair<DatabaseID, std::string>, XdbSequence *> m_myNameOpenSeqs;

    // Caches all large objects opened with this txn(like entry set).
    std::map<std::pair<DatabaseID, std::string>, uint32> m_myNameOpenLargeObject_;
    std::map<std::pair<DatabaseID, uint32>, LargeObject *> m_myWriteIDOpenLargeObject_;
    std::map<std::pair<DatabaseID, uint32>, LargeObject *> m_myReadIDOpenLargeObject_;
    std::map<std::pair<DatabaseID, std::string>, LargeObject*> m_recentOpenLO;
	Snapshot currentSnapshot;
    //CreateSequence *seqFactory_;
};

class PGSubTransaction:public PGTransaction
{
public:
	PGSubTransaction(FXTransactionId transaction_id = InvalidTransactionId
		, IsolationLevel isolation_level = NO_ISOLATION_LEVEL_SPECIFIED);
	/// Abort the transaction.
	virtual void abort();

	/// Commit the transaction.
	virtual void commit();

	/// Prepare the transaction.
	virtual void prepare();

private:
	static void begin(void);
};
} //StorageEngineNS
}//FounerXDB

#endif //PG_TRANSACTION_HPP
