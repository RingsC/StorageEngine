#ifndef PG_SORT_ENTRY_SET_H 
#define PG_SORT_ENTRY_SET_H  

/// @file PGSortEntrySet.h
/// @brief Define sort entry set interface.
///
/// This file defines the sort entry set interface.
///
#include "EntrySet.h"

struct Tuplesortstate;

namespace FounderXDB{
	namespace StorageEngineNS {
		class PGSortEntrySetScan;
/// @ingroup StorageEngine
/// PGSortEntrySet defines the interface of sort entry set
class PGSE_DLLEXTERN PGSortEntrySet : public SortEntrySet
{
	friend class PGSortEntrySetScan;
	friend class PGStorageEngine;
private:

	/// Not support.
	inline virtual int deleteEntry(Transaction* txn, const EntryID &eid) 
	{
		return -1;
	}

	/// Not support.
	inline virtual int deleteEntries(Transaction* /*txn*/, const std::vector<EntryID> &/*eids*/) 
	{
		return -1;
	}

	/// Not support.
	inline virtual int updateEntry(Transaction* txn,
		const EntryID& eid, const DataItem& entry, bool isinplaceupdate = false) 
	{
		return -1;
	}

	/// Not support.
	inline virtual int updateEntries(Transaction* txn,
		const std::vector<EntryID>&/* eids*/, const std::vector<DataItem>& /*entryValues*/, bool isinplaceupdate = false) 
	{
		return -1;
	}    

	/// must Recursive delete member of indinfo
	inline virtual bool getIndexInfo(Transaction* txn,PGIndinfoData& indinfo) 
	{
		return false;
	}

	/// Get number of pages for the relation.
	/// @param[in]  txn    the transaction
	/// @param[out] number  the number of pages
	inline virtual se_uint64 getNumberOfPages()
	{
		return 0;
	}

	/// truncate a relation, physically shrink the relation file length to 0, 
	/// all entries physically removed. call heap_truncate_one_rel function to implement, 
	/// make sure heap_truncate_one_rel works right first.
	inline virtual int truncate()
	{
		return 0;
	}

	/// store the tuples in this entry set into a file named using this entry set's ID, 
	/// the file will be created in dir directory. The format of the dump file is as follows:
	/*
	在指定路径下创建dump文件，然后先写一个头结构，然后在一个新创建的事务中  Scan EntrySet，
	将EntrySet表的tuple逐条写入一个文件。
	每个tuple的格式是：{长度（tuple字节数N，4字节），tuple所有N个字节}
	dump头结构是固定长度的，包括以下字段:
	{uint32 magicNum; uint8 version; uint32 numTuples; time_t dumpWhen; TransactionID currentTransactionID; uint32 entrySetID; uint32 chksum;}
	其中chksum是dump文件中除了这个头结构以外所有其余数据的校验和，在dump结束后计算并且写入到头中。

	*/
	inline virtual int dump(const char *dir,Transaction *pTrans)
	{
		return 0;
	}

	/// Load tuples from a dump file created by EntrySet::dump.
	/*打开dump文件，检查magicNum是预期值，然后检查校验和正确。然后，逐条读取tuple并且插入heap表中，如果有索引那么更新索引。*/
	inline virtual int load(const char *dumpFilePath,Transaction *pTrans)
	{
		return 0;
	}

    virtual int deleteEntries(Transaction* /*txn*/, std::list<EntryID> &/*eids*/){return 0;}
    virtual int updateEntries(Transaction* /*txn*/,
		std::list<std::pair<EntryID, DataItem> >&/* eids*/, bool isInplaceUpdate = false)
	{
	    isInplaceUpdate = true;//keep compiler quiet
	    return 0;
	}
        
	virtual void doPut(const DataItem &di);
public:
	/// Constructor
	PGSortEntrySet(StoreSortType sst);

	/// Deconstructor
	virtual ~PGSortEntrySet();  

	//initialize the sort entry set, this method must be called 
	//after entry set descriptor is set
	//@param[in] mayIndex		index
	//@param[in] indexUnique	is unique index
	//@param[in] workMem	working memory
	//@param[in] randomAccess		true to support mark restore and rescan
	//@param[in] pCompare		compare pcode
	void initialize(int workMem,
		bool randomAccess,
		CompareCallbacki     pCompare);

	inline bool isInitialized()
	{
		return m_state != NULL;
	}
	
	/// Insert a set of entries into this entry set.
	/// @param[in]  txn     transaction
	/// @param[in]  rset    set of input entries
	/// @param[in]  bIndex  insert index tuple or not
	/// @return     value yet to be defined
	virtual int insertEntries(Transaction* txn, const std::vector<DataItem>& rset);

	/// Insert a set of entries into this entry set.
	/// @param[in]  txn     transaction
	/// @param[in]  rset    set of input entries
	/// @param[in]  bIndex  insert index tuple or not
	/// @return     value yet to be define
	virtual int insertEntries
		(Transaction* txn, 
		const EntrySet& rset);

	/// Insert an entry into the entry set.
	/// @param[in]  txn     transaction
	/// @param[out] eid		not useful
	/// @param[in]  entry   entry value
	/// @param[in]  bIndex  insert index tuple or not
	/// @return     value yet to be defined
	virtual int insertEntry(Transaction* txn,
		EntryID &eid, const DataItem& entry);

	/// Open a scan for iteration.
	/// @param[in]  txn     transaction
	/// @param[in]  opt    access option
	/// @param[out] compare  compare function
	/// @param[in] scankeys  scan key vector
	virtual EntrySetScan* startEntrySetScan
		(Transaction* txn,
		SnapshotTypes opt,  
		const std::vector<ScanCondition>&scankeys, filter_func_t filterFunc = 0, void* userData = 0,const EntryID* eid = 0,const EntryID* endid = 0);

	virtual EntrySetScan* startEntrySetScan(Transaction* txn);

	/// End a EntrySetScan.
	/// @param[in]  opt    access option
	/// @param[in]  cursor  entry set cursor
	virtual void endEntrySetScan(EntrySetScan* scan);

	///Perform the sort algorithm
	virtual void performSort(CompareCallbacki pCompare = 0);
	
	///the call guarantee that the entryset is sorted.
	virtual void setSorted();

private:
	Tuplesortstate* m_state;
	bool            m_bSortDone;
	CompareCallbacki    m_pCompare;
	StoreSortType m_sst;

};

class PGSE_DLLEXTERN PGSortEntrySetScan : public EntrySetScan
{
	friend class PGSortEntrySet;
public:
	/// Constructor
	explicit PGSortEntrySetScan(PGSortEntrySet *tmpEntry);

	virtual ~PGSortEntrySetScan();

	/// Get next entry.
	/// @param[in]  opt     access option
	/// @param[out] eid     The entry ID to take out 
	/// @param[out] entry     The entry to take out 
	/// @return     value yet to be defined
	virtual int getNext(CursorMovementFlags opt, EntryID &eid, DataItem& entry);

	/// Mark the current position.
	virtual int markPosition();

	/// Restore to the latest marked position.
	virtual int restorePosition();

	/// Get next batch of entries.
	/// @param[in]  opt     access option
	/// @param[out] eid_entries     array of (entry-id, entry) pairs
	/// @return     value yet to be defined
	virtual int getNextBatch(CursorMovementFlags opt, 
		std::vector<std::pair<EntryID, DataItem> > &eid_entries);

private:
	Tuplesortstate *m_pTupleStoreState;
	EntrySet::StoreSortType m_sst;
};
}
}
#endif // SORT_ENTRY_SET_H 