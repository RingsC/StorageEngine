#ifndef PG_XDB_SEQUENCE_HPP
#define PG_XDB_SEQUENCE_HPP

#include <pthread.h>
#include "XdbSequence.h"
#include "XdbLock.h"


namespace FounderXDB{
namespace StorageEngineNS{

class PgXdbSequenceFactory : public SequenceFactory
{
private:
	PgXdbSequenceFactory(){};
	virtual ~PgXdbSequenceFactory(){};
public:
	static PgXdbSequenceFactory* getInstance();
    static void releaseSequenceFactory(PgXdbSequenceFactory* instance);

	virtual SeqID createSequence(xdb_seq_t initialValue,
    	std::pair<xdb_seq_t, xdb_seq_t> range,
    	const char* name = 0, uint32 flags = INCREMENTAL,
    	DatabaseID dbid = 0);
    virtual SeqID createSequence(xdb_seq_t initialValue,
    	const char* name = 0, uint32 flags = INCREMENTAL,
    	DatabaseID dbid = 0);

    /// open one sequence
    /// if input dbid == 0, use current database
    virtual XdbSequence* openSequence(SeqID seqID, DatabaseID dbid = 0);
	virtual XdbSequence* openSequence(const char *seqName, DatabaseID dbid = 0);

    virtual void deleteSequence(SeqID seqID, DatabaseID dbid = 0);
	virtual void deleteSequence(const char *seqName, DatabaseID dbid = 0);
private:
	SeqID getMaxSeqId(EntrySetID seqTableId, EntrySetID seqIdIdxId);
	static void getPGSequenceFactory();
	static pthread_once_t m_onceMutex;
	static PgXdbSequenceFactory *m_singlePgSequenceFactory;
};

class PgXdbSequence : public XdbSequence
{
friend class PGTransaction;
protected:
    virtual ~PgXdbSequence(){}	
public:
	PgXdbSequence(SeqID seqId, DatabaseID dbid);
	PgXdbSequence(const char *seqName, DatabaseID dbid);
    
    /// Reset sequence value to newInitialValue
    virtual void reset(xdb_seq_t newInitialValue);

    /// Remove this sequence from db, and don't use this object after this call.
    //virtual void remove();

    /// Return current value, and increase new value to current+delta and update the corresponding row.
    virtual xdb_seq_t getValue(int32 delta) const;
	virtual const char* getName()const;
    virtual SeqID getId() const;
    virtual std::pair<xdb_seq_t, xdb_seq_t> getRange() const;
    virtual uint32 getFlags() const;
	virtual sequenceInfo getSequenceInfo() const;
private:
	//Transaction *m_xact;
	DatabaseID m_dbid;
	EntrySetID m_seqTableId;
	EntrySetID m_seqIdIdxId;
	EntrySetID m_seqnameIdxId;
	SeqID m_seqId;
	std::string m_seqName;
	uint32 m_flags;
	bool m_hasRange;
	std::pair<xdb_seq_t, xdb_seq_t> m_range;
};


} //StorageEngineNS
}//FounerXDB


#endif //PG_XDB_SEQUENCE_HPP