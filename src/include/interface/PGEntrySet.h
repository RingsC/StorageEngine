#ifndef PS_ENTRY_SET_HPP
#define PS_ENTRY_SET_HPP

#include "EntrySet.h"

#include "storage/itemptr.h"
#include "access/heapam.h"

struct RelationData;
struct HeapTupleData;

namespace FounderXDB{
namespace StorageEngineNS {

class PGEntrySet : public EntrySet{
public:
    /// constructor
    PGEntrySet(EntrySetID entry_set_id, Relation heapRelation);

    /// destructor
    virtual ~PGEntrySet();

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
    /// @return     value yet to be defined
    virtual int insertEntries(Transaction* txn, const EntrySet& rset);

    /// Insert an entry into the entry set.
    /// @param[in]  txn     transaction
    /// @param[out] eid		entry ID of inserted entry
    /// @param[in]  entry   entry value
	/// @param[in]  bIndex  insert index tuple or not
    /// @return     value yet to be defined
    virtual int insertEntry(Transaction* txn,
        EntryID &eid, const DataItem& entry);

    /// Delete an entry from the entry set.
    /// @param[in]  txn     transaction
    /// @param[in]  eid		entry ID of the entry to delete
    /// @return     value yet to be defined
    virtual int deleteEntry(Transaction* txn, const EntryID &eid);

    virtual int deleteEntries(Transaction* /*txn*/, std::list<EntryID> &/*eids*/);

    /// Update an entry within the entry set.
    /// @param[in]  txn     transaction
    /// @param[in]  eid     entry ID
    /// @param[in]  entry   the new entry 
    /// @return     value yet to be defined
    virtual int updateEntry(Transaction* txn, const EntryID& eid, const DataItem& entry, bool isInplaceUpdate = false);

	virtual int updateEntries(Transaction* , 
			std::list<std::pair<EntryID, DataItem> >& ,
			bool isInplaceUpdate /* = false */);
    
    /// Inplace Update an entry within the entry set.
    /// @param[in]  eid     entry ID
    /// @param[in]  entry   the new entry 
    /// @return     value yet to be defined
    int InplaceUpdateEntry(const EntryID& eid, const DataItem& entry);

    /// Open a scan for iteration.
    /// @param[in]  txn     transaction
    /// @param[in]  opt    snapshot option
    /// @param[in] scankeys  scan key vector
    virtual EntrySetScan* startEntrySetScan
        (Transaction* txn,
         SnapshotTypes opt,  
         const std::vector<ScanCondition>&scankeys, filter_func_t filterFunc = 0, void*userData = 0,const EntryID* eid = 0, const EntryID* endid = 0);

    /// End a EntrySetScan.
    /// @param[in]  scan  entry set cursor
    virtual void endEntrySetScan(EntrySetScan* scan);
    virtual bool getIndexInfo(Transaction* txn, PGIndinfoData& indinfo);

    /// truncate a relation, physically shrink the relation file length to 0, 
    /// all entries physically removed. call heap_truncate_one_rel function to implement, 
    /// make sure heap_truncate_one_rel works right first.
    virtual int truncate();
    
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
    virtual int dump(const char *dir,Transaction *pTrans);
    
    /// Load tuples from a dump file created by EntrySet::dump.
    /*打开dump文件，检查magicNum是预期值，然后检查校验和正确。然后，逐条读取tuple并且插入heap表中，如果有索引那么更新索引。*/
    virtual int load(const char *dumpFilePath,Transaction *pTrans);



	virtual se_uint64 getNumberOfPages();

	inline Relation getRelation(){
	    return m_heapRelation;
    }



private:
    void insert_index(RelationData* heapRelation, HeapTupleData* tup);
    /// copy constructor
     PGEntrySet(const PGEntrySet& other);

    /// overload operator =
    PGEntrySet& operator = (const PGEntrySet& other);
	Relation m_heapRelation;
	
};


class HeapEntrySetScan : public EntrySetScan {
public:
    ///constructor
    explicit HeapEntrySetScan(const std::vector<ScanCondition>& scankeys, HeapScanDesc heap_scan_desc = NULL);
   /// Destructor.
    virtual ~HeapEntrySetScan();
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
    ///todo wangfh@founder.com  will optimize in the future
    virtual int getNextBatch(CursorMovementFlags opt, 
        std::vector<std::pair<EntryID, DataItem> > &eid_entries);  


    /// Mark the current position.
    virtual int markPosition();

    /// Restore to the latest marked position.
    virtual int restorePosition();

    inline HeapScanDesc getHeapScanDesc()
    {
        return  m_heap_scan_desc;
    }

private:
    HeapScanDesc m_heap_scan_desc;
    std::vector<ScanCondition> m_scankeys;
};

} //StorageEngineNS
}//FounerXDB

#endif //PS_ENTRY_SET_HPP