#ifndef PG_TEMP_ENTRY_SET_H
#define PG_TEMP_ENTRY_SET_H 

/// @file PGTempEntrySet.h
/// @brief Define temp entry set interface.
///
/// This file defines the temp entry set interface.
///
/*#include <Platform/MemManager.h>*/
#include <interface/PGEntrySet.h>

#include "postgres.h"
#include "utils/rel.h"
//#include "utils/tuplestore.h"

struct Tuplestorestate;

namespace FounderXDB{
namespace StorageEngineNS {
class TempEntrySetScan;
/// @ingroup StorageEngine
/// PGTempEntrySet defines the interface of temp entry set
class PGTempEntrySet : public EntrySet
{
	friend class TempEntrySetScan;
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
	inline virtual bool getIndexInfo(Transaction* txn, PGIndinfoData& indinfo) 
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

	/// store the tuples in this entry set into a file named using this entry set's ID, 
	/// the file will be created in dir directory. The format of the dump file is as follows:
	/*
	��ָ��·���´���dump�ļ���Ȼ����дһ��ͷ�ṹ��Ȼ����һ���´�����������  Scan EntrySet��
	��EntrySet���tuple����д��һ���ļ���
	ÿ��tuple�ĸ�ʽ�ǣ�{���ȣ�tuple�ֽ���N��4�ֽڣ���tuple����N���ֽ�}
	dumpͷ�ṹ�ǹ̶����ȵģ����������ֶ�:
	{uint32 magicNum; uint8 version; uint32 numTuples; time_t dumpWhen; TransactionID currentTransactionID; uint32 entrySetID; uint32 chksum;}
	����chksum��dump�ļ��г������ͷ�ṹ���������������ݵ�У��ͣ���dump��������㲢��д�뵽ͷ�С�
	*/
	virtual int dump(const char *dir,Transaction *pTrans)
	{
		return 0;
	}

	/// Load tuples from a dump file created by EntrySet::dump.
	/*��dump�ļ������magicNum��Ԥ��ֵ��Ȼ����У�����ȷ��Ȼ��������ȡtuple���Ҳ���heap���У������������ô����������*/
	virtual int load(const char *dumpFilePath,Transaction *pTrans)
	{
		return 0;
	}

    virtual int deleteEntries(Transaction* /*txn*/, std::list<EntryID> &/*eids*/)
    {
        return 0;
    }

    virtual int updateEntries(Transaction* /*txn*/,
		std::list<std::pair<EntryID, DataItem> >&/* eids*/, bool isInplaceUpdate = false)
	{
	    isInplaceUpdate = true;//keep compiler quiet
	    return 0;
	}    
        
public:
	/// Constructor
	PGTempEntrySet(StoreSortType sst,
		bool randomAccess, 
		bool interXact, 
		int maxKBytes,
		unsigned int spaceId);

	/// Deconstructor
	virtual ~PGTempEntrySet();   

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

	/// truncate a relation, physically shrink the relation file length to 0, 
	/// all entries physically removed. call heap_truncate_one_rel function to implement, 
	/// make sure heap_truncate_one_rel works right first.
	virtual int truncate();

	/// Open a scan for iteration.
	/// Open a scan for iteration.
	/// @param[in]  txn     transaction
	/// @param[in]  opt    access option
	/// @param[out] compare  compare function
	/// @param[in] scankeys  scan key vector
	virtual EntrySetScan* startEntrySetScan
		(Transaction* txn,
		SnapshotTypes opt,  
		const std::vector<ScanCondition>&scankeys, filter_func_t filterFunc = 0, void*userData = 0,const EntryID* eid = 0,const EntryID* endid = 0);

	/// End a EntrySetScan.
	/// @param[in]  opt    access option
	/// @param[in]  cursor  entry set cursor
	virtual void endEntrySetScan(EntrySetScan* scan);

private:
	/// The tablespace for underlying buffile
	Oid m_TableSpaceId;

	/// Working memory size, KBytes
	int m_iWorkingMemorySize;

	Tuplestorestate* m_pTupleStoreState;

	StoreSortType m_sst;
};

class TempEntrySetScan : public EntrySetScan
{
	friend class PGTempEntrySet;
public:
	/// Constructor
	explicit TempEntrySetScan(PGTempEntrySet *tmpEntry);
	 
	virtual ~TempEntrySetScan();

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
	Tuplestorestate *m_pTupleStoreState;
	int m_iCurrentReader;
	int m_iMarkedReader;
};

}
}
#endif // PG_TEMP_ENTRY_SET_H 