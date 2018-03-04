#ifndef PG_STORAGE_ENGINE_HPP
#define PG_STORAGE_ENGINE_HPP

#include <map>
#include <string>

#include "StorageEngine.h"
#include "interface/PGEntrySet.h"
#include "XdbLock.h"

#include <pthread.h>
#include "storage/s_lock.h"



namespace FounderXDB{
namespace StorageEngineNS {

class PGTransaction;

class PGStorageEngine : public StorageEngine
{
private:
    PGStorageEngine();
    PGStorageEngine(const PGStorageEngine &other);
    PGStorageEngine& operator =(const PGStorageEngine &other);
    virtual ~PGStorageEngine();
public:
    static PGStorageEngine* getInstance();
    static void releaseStorageEngine(PGStorageEngine* instance);
        /// Destructor.
    //virtual ~PGStorageEngine();

	virtual TableSpaceID createTableSpace(const char *tblspcName, const char *path, bool);

	/// Cluster a relation on an index.
	/// @param[in] entryid		EntrySet ID
	/// @param[in] indexid		IndexEntrySet ID
	virtual void cluster(EntrySetID entryid, EntrySetID indexid, bool use_sort = true);

	virtual void rebuildIndex(EntrySetID entryid, int flags);
    
	virtual void createTablespaceDirs(const char *location, TableSpaceID tablespaceoid);
	virtual void destroyTablespaceDirs(TableSpaceID tablespaceoid);
    /// Get a tablespace's ID by name.
    virtual TableSpaceID getTableSpaceID(Transaction *txn, const char *tblspcName) const;
	virtual int dropTableSpace(const char *tblspcName);
    virtual DatabaseID createDatabase(Transaction *txn, const char *dbname, const char *defaultTblspcName, const void*extraData = 0, size_t extraDataLen = 0);
    virtual int dropDatabase(Transaction *txn, const char *dbname);

		virtual void listTableSpace(std::vector<std::pair<std::string, std::string> > &v_tblspc);
    
		virtual void listTableSpace(std::vector<TablespaceMetaData> &v_tblspc);
    
    virtual DatabaseID setCurrentDatabase(const char *dbname,Oid dbId = InvalidOid,Oid tablespaceId = InvalidOid);
  
	virtual DatabaseID getCurrentDatabase(std::string& dbname) const;
	virtual TableSpaceID getCurrentDatabaseTablespace() const;
	/// Get database metadata (fill into dbmeta object) by db id, return true if dbmeta is filled with valid data, 
	/// otherwise return false.
	virtual bool getDatabaseInfoByID(DatabaseID db_id, DBMetaInfo&dbmeta) const;
	/// Get database metadata (fill into dbmeta object) by name, return true if dbmeta is filled with valid data, 
	/// otherwise return false.
	virtual bool getDatabaseInfo(const char *dbname, DBMetaInfo&dbmeta) const;
	/// Update extra data. db id and name is permanent.
	virtual void updateDatabaseExtraData(const char *dbname, const void *data, size_t len);
    /// Get database ID by name.
    virtual DatabaseID getDatabaseID(const char *dbname) const;
    
    virtual void getAllDatabaseInfos(std::vector<DBMetaInfo>&dbs) const;
    /// Initialize StorageEngine.
    /// database_home: location of storage engine data
    virtual void initialize(const char *database_home, 
			const uint32 thread_num, 
			storage_params * params);

	virtual void interruptInit(int code);

	virtual void initMemoryContext();

		virtual bool amMaster() const;
		virtual void regTriggerFunc(StandbyTriggerFunc func);

    ///bootstrap initdb
    virtual void bootstrap(const char *datadir); 
    virtual void shutdown(void);

    /// Start/End Online/Offline archive
    ///     These are needed for making transactionally consistent
    ///     global archives.
    virtual bool startArchive(bool isOnline);
    virtual bool endArchive(bool isOnline);

	virtual void startBackup(const char* strLabel,bool fast);
	virtual void endBackup( void );

    virtual bool startVacuum();
    virtual bool endVacuum();

    /// Get a transaction by global transaction id, create it if it doesn't exist.
    /// @return           the transaction
    /// @param[in]  xid         the global transaction id
    /// @param[in]  level       the isolation level
    virtual Transaction *getTransaction(FXTransactionId&   xid,
        Transaction::IsolationLevel   level, 
        FXGlobalTransactionId gxid = InvalidTransactionID);

	virtual PGTransaction *createTransaction(Transaction::IsolationLevel   level,
        Transaction* pParent = NULL,
        FXGlobalTransactionId gxid = InvalidTransactionID);

	virtual void deleteTransaction(const FXTransactionId&  xid);
	
    /// Do preparation for asynchronize abort the transaction 
    /// @param[in]  xid         the global transaction id
    /// @return false if the transaction xid does not exist.
    virtual bool prepareAsyncAbortTransaction(FXTransactionId& xid);


    virtual EntrySetID createEntrySet(Transaction*     txn,
                             uint32 colid, const char *tblspcName = 0, 
														 DatabaseID dbid = 0, bool isTempEntrySet = false);

		/// Create a temp entry set.
		/// @param[in]  txn						transaction
		/// @param[in]  maxKBytes			remaining memory available, in bytes
		/// @param[in]  randomAccess  is support random access
		/// @param[in]  interXact			is keep open through transactions
		/// @return the created entry set
		/* 需要注意的是，如果在一个事务中创建一个temp entry set,
		 * 那么关于该temp entry set的所有操作都需要在该事务中完成。
		 * 否则事务的提交将连带着清空temp entry set。
		 * 所以如果要跨事务操作请在事务外头创建。
		 */
		virtual EntrySet* createTempEntrySet(Transaction *txn,
														 EntrySet::StoreSortType sst,
														 uint32 spaceId,
														 const uint32 maxKBytes,
														 const bool randomAccess,
														 const bool interXact);

		/// Create a temp entry set.
		/// @param[in]  txn						transaction
		/* 需要注意的是，如果在一个事务中创建一个sort entry set,
		 * 那么关于该sort entry set的所有操作都需要在该事务中完成。
		 * 否则事务的提交将连带着清空sort entry set。
		 * 所以如果要跨事务操作请在事务外头创建。
		 */
		virtual SortEntrySet* createSortEntrySet(Transaction *txn, 
			EntrySet::StoreSortType sst,
			int workMem,
			bool randomAccess,
			CompareCallbacki     pCompare);


    virtual EntrySetID createIndexEntrySet(Transaction*txn, EntrySet *parentSet,
                             EntrySetType type,
                             uint32 colid, const char *tblspcName = 0, DatabaseID dbid = 0,
                             void *userData =0 ,filter_func_t filterFunc = 0, uint32 userdataLength = 0, bool skipInsert = false);

    /// Remove an entry set.
    /// @param[in]  txn         transaction
    /// @param[in]  entrysetID  current entry set id
    /// @param[in]  dbid        database id
    virtual void removeEntrySet(Transaction*       txn,
                             EntrySetID      entrysetID,
							 DatabaseID dbid = 0);

    /// Remove an index entry set.
    /// @param[in]  txn         transaction
    /// @param[in]  parentID    parent table id
    /// @param[in]  entrysetID  current entry set id
    /// @param[in]  dbid        database id
    virtual void removeIndexEntrySet(Transaction*       txn, 
                            EntrySetID parentID,
                            EntrySetID      entrysetID,
							DatabaseID dbid = 0);

    /// Open an entry set.
    /// @param[in]  txn         transaction
    /// @param[in]  opt         open option
    /// @param[in]  entrysetID  entry set id
    /// @param[in]  dbid        database id
    virtual EntrySet *openEntrySet(Transaction*     txn,
        EntrySet::EntrySetOpenFlags     opt,
        EntrySetID    entrysetID,
		DatabaseID dbid = 0
		);

    /// Open an index entry set.
    /// @param[in]  txn         transaction
    /// @param[in]  parentSet   parent table
    /// @param[in]  opt         open option
    /// @param[in]  entrysetID  entry set id
    /// @param[in]  dbid        database id
    virtual IndexEntrySet *openIndexEntrySet( Transaction*     txn,
        EntrySet *parentSet,
        EntrySet::EntrySetOpenFlags     opt,
        EntrySetID    entrysetID,
		DatabaseID dbid = 0);

    /// Close an entry set.
    /// @param[in]  set         entry set 
    /// @param[in]  txn         transaction
    virtual void closeEntrySet(Transaction*  txn, BaseEntrySet *set);

    /// Run local deadlock detection
    // TODO: may need to specify which transaction should be aborted if deadlock happens.
    /// @param[out] aborted     the number of transactions aborted by this function.
    virtual void localDeadlockDetect(int* aborted);

    /// Construct wait-for-graph 
    ///    This is needed for global deadlock detection.
    virtual void getWaitForGraph(WaitForGraph *wfg);

    /// Invoked when a thread is created.
    virtual void beginThread();
    
    /// Invoked when a thread is terminated.
    virtual void endThread();

	virtual void detachThread();

	virtual bool isThreadInit();

    /// Invoked when a QUERY statement begins.
    virtual void beginStatement(Transaction *transaction = NULL);

    /// Invoked when a QUERY statement ends.
    virtual void endStatement(Transaction *transaction = NULL);

    /// Invoked when a plan step begins.
    virtual void beginPlanStep();

    /// Invoked when a plan step ends.
    virtual void endPlanStep();
    
    virtual LargeObjectStore *getLargeObjectStore(DatabaseID /*dbid*/){return 0;}
    
    void closeEntrySetInternal(Transaction*  txn, IndexEntrySet *set , bool unlock = false);
    void closeEntrySetInternal(Transaction*  txn, EntrySet *set, bool unlock = false);

	virtual uint32 getCurrentTimeLine( void );
	void resetOnExit( void );
	
	virtual FXTransactionId getMinTransactionId(void);
	
	virtual FXTransactionId getMaxTransactionId(void);

	virtual FXTransactionId getNextTransactionId(void);

	virtual Transaction* GetCurrentTransaction();

	virtual XdbLock* GetSequenceLock(EntrySetID seqTableId, SeqID seqId);
	virtual void RemoveSequenceLock(EntrySetID seqTableId, SeqID seqId);
	/// True if global transaction is prepared.
	virtual bool isGlobalTransactionPrepared(FXGlobalTransactionId gxid);

	virtual FXGlobalTransactionId GetNextMaxPreparedGlobalTxnId();

	virtual void commitPreparedTransaction(FXGlobalTransactionId gxid, Transaction::IsolationLevel level = Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	virtual void rollbackPreparedTransaction(FXGlobalTransactionId gxid, Transaction::IsolationLevel level = Transaction::READ_COMMITTED_ISOLATION_LEVEL);

	//xlog interface
	virtual bool XLogArchiveIsBusy(const char *xlog);
	virtual bool FileCanArchive(const char *filename);
	virtual void SetControlFileNodeId(uint32 nodeId);
	virtual uint32 GetControlFileNodeId(void);
	virtual void SetWalSenderFunc(WalSenderFunc copyfunc, WalSenderFunc removefunc);
	virtual void SyncRepSetCancelWait(void);

	// Release All lwlock
	virtual void LWLockReleaseAll(void);
	virtual bool LWLockHeldAnyByMe(void);
	
	/// do vacuum operation
	virtual void vacuumDatabase(void);
	virtual void vacuumAll(void);
	virtual void vacuumRelations(std::vector<EntrySetID>& eids);
	virtual void reIndexes(Transaction* txn, EntrySet *parentSet, const std::vector<IndexEntrySet*>& indexIds);

private:
    ///store global transaction
    std::map<FXTransactionId, PGTransaction*> m_global_transactions;
    /// produce global transaction id and it's value increases maybe static
    FXTransactionId m_global_transation_id;
	FXTransactionId m_min_trans_id;
	FXTransactionId m_max_trans_id;
    /// single instance
    static PGStorageEngine *m_single_pgstorage_engine;
    //std::map<EntrySetID, BaseEntrySet*> m_entry_set;
//todo  m_current_max_oid will come from metadata table in future
    Oid m_current_max_oid;
    
    //static pthread_mutex_t m_single_pgstorage_engine_mutex;

    //pthread_mutex_t transactions_id_mutex;
	//pthread_mutex_t current_max_oid_mutex;
	slock_t transactions_id_mutex;
    slock_t current_max_oid_mutex;
    static pthread_once_t once_mutex;

	// for sequence
    std::map<std::pair<EntrySetID, SeqID>, XdbLock*> m_seqlocks_map;

private :
    static void getPGStorageEngine();
    void CurrentTransactionMustExist();

	void commitAllPreparedTransaction();

	std::string FormatTimeLineInfo();
	bool CheckTimeLineInfo(std::string &tlistr);
	int64 trans_str_time(const std::string& source_str);

public:
	void SetCurrentTransaction(Transaction* pTransaction);
	FounderXDB::StorageEngineNS::XdbLock* transactions_map_mutex;

};

} //StorageEngineNS
}//FounerXDB
#endif //PG_STORAGE_ENGINE_HPP