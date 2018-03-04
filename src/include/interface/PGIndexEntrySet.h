#ifndef PGINDEX_ENTRY_SET_HPP
#define PGINDEX_ENTRY_SET_HPP

#include <string>

#include "EntrySet.h"
#include "access/genam.h"
#include "utils/relcache.h"


namespace FounderXDB{
namespace StorageEngineNS {

class PGIndexEntrySet : public IndexEntrySet {
public:
    PGIndexEntrySet(EntrySetID entry_set_id, EntrySetID index_entry_id, Relation heapRelation = NULL, Relation indexRelation = NULL);
    /// Destructor.
    virtual ~PGIndexEntrySet();

    /// Open a scan for iteration.
    /// @param[in]  txn     transaction
    /// @param[in]  opt    access option
    /// @param[in] scankeys  scan key vector
    /// @param[in] filterFunc   A callback function to filter index keys when an index key 
    ///         already meets all scankeys. If this function returns true, this index key is qualified and heap tuple will be fetched.
    /// @param[in] userData    User data used during each filtering operation, it's passed into filterFunc when it's called.
    virtual EntrySetScan* startEntrySetScan
        (Transaction* txn,
         SnapshotTypes opt,  
         const std::vector<ScanCondition> &scankeys, filter_func_t filterFunc = 0, void*userData = 0,const EntryID* eid = 0, const EntryID* endid = 0);

    /// End a EntrySetScan.
    /// @param[in]  opt    access option
    /// @param[in]  cursor  entry set cursor
    virtual void endEntrySetScan(EntrySetScan* scan);
    
    virtual int insertEntry(Transaction* txn,
        const DataItem& index_key, const DataItem &index_data,const EntryID* eid = 0);

    /// Get key range percentage, only works when the index has only one column.
	virtual void getKeyRange(Transaction* txn,std::vector<ScanCondition> &vscans, KeyRange &range);
	/// Get entry set key range
	virtual void getEntrySetKeyRange(Transaction* txn, std::string& minKey, std::string& maxKey);

	virtual se_uint64 getNumberOfPages();
	inline Relation getIndexRelation()
    {
	    return m_indexRelation;
    }
private:
        //construtor
    PGIndexEntrySet(){}
    /// copy constructor
    PGIndexEntrySet(const PGIndexEntrySet& other);

    /// overload operator =
    PGIndexEntrySet& operator = (const PGIndexEntrySet& other);
        
	void getNumsOfKey(Transaction* txn,std::vector<ScanCondition> &vscans, const ScanCondition::CompareOperation &comp, se_uint64 &nums);

private:

    EntrySetID m_heap_entry_id;

	Relation m_heapRelation;
	Relation m_indexRelation;
	
};


class PGIndexEntrySetScan : public IndexEntrySetScan{
public:
    ///constructor
	
    explicit PGIndexEntrySetScan(const std::vector<ScanCondition>& scankeys, IndexScanDesc index_scan =NULL);

    /// Destructor.
    virtual ~PGIndexEntrySetScan();

    /// Get next entry's ID.
    /// @param[in]  opt     access option
    /// @param[out] eid     The entry ID to take out 
    /// @return     value yet to be defined
    virtual int getNext(CursorMovementFlags opt, EntryID &eid);


    /// Get the entry IDs of next batch of entries.
    /// @param[in]  opt     access option
    /// @param[out] eids     array of entry IDs
    /// @return     value yet to be defined
    virtual int getNextBatch(CursorMovementFlags opt, std::vector<EntryID>& eids);

    /// Get next entry.
    /// @param[in]  opt     access option
    /// @param[out] eid     The entry ID to take out 
    /// @param[out] entry     The entry to take out 
    /// @return     value yet to be defined
    virtual int getNext(CursorMovementFlags opt, EntryID &eid, DataItem& entry);

    /// Get next batch of entries.
    /// @param[in]  opt     access option
    /// @param[out] eid_entries     array of (entry-id, entry) pairs
    /// @return     value yet to be defined

    virtual int getNextBatch(CursorMovementFlags opt, 
        std::vector<std::pair<EntryID, DataItem> > &eid_entries);

    /// Mark the current position.
    virtual int markPosition();

    /// Restore to the latest marked position.
    virtual int restorePosition();
	
	/// Do a bitmap scan and get the bitmap, in order to scan the primary entry set(heap) by pages.
    virtual EntryIDBitmap*getBitmap(Transaction* txn);

	///Delete the bitmap
	virtual void deleteBitmap(EntryIDBitmap* pbitmap);

    inline IndexScanDesc getIndexScanDesc()
    {
        return m_index_scan_desc;
    }

private:
    IndexScanDesc m_index_scan_desc; 
    std::vector<ScanCondition> m_scankeys;
    char *m_curentry;// stores  cluster index current key-value pair.
    size_t entrysz_;
	//Relation m_heapRelation;
	//Relation m_indexRelation;
};


} // namespace StorageEngineNS
} //namespace FounderXDB
#endif //PGINDEX_ENTRY_SET_HPP