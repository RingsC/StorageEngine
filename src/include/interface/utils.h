#ifndef UTILS_HPP
#define UTILS_HPP

#include "postgres.h"
#include "PGSETypes.h"
#include "EntrySet.h"
#include "storage/itemptr.h"
#include "utils/rel.h"
#include <pthread.h>
#include "interface/FDPGAdapter.h"



namespace FounderXDB{
namespace StorageEngineNS {

    /// get tid from EntryID
    /// @param[in]    eid
    /// @param[out]    tid
    extern void entryIdToTid(const EntryID &eid, ItemPointerData &tid);
	extern void pentryIdToTid(const EntryID* eid, ItemPointerData &tid);

    /// get EntryID from tid
    /// @param[in]    tid
    /// @param[out]    eid
    extern void TidToEntryId(const ItemPointerData &tid, EntryID &eid);
	extern MetaTable formMtInfo(EntrySetType    type, TableSpaceID spcid, IndinfoData sinfo);
	extern void deformMtInfo(MetaTable mt_info);
	extern MetaTable formMtInfo(EntrySetType    type, TableSpaceID spcid, IndinfoData sinfo);
	extern void deformMtInfo(MetaTable mt_info);

    extern void my_split(RangeData &rangeData, char *str, int col);
    extern int wfh_compare(const char *str1, size_t len1, const char *str2, size_t len2);
	extern int seq_compare(const char *str1, size_t len1, const char *str2, size_t len2);
    extern slock_t setColinfo_mutex;


class PGSpinLock{
public:
	PGSpinLock(slock_t &slock);

	~PGSpinLock();

private:
	PGSpinLock(const PGSpinLock&);
	const PGSpinLock&operator=(const PGSpinLock&);
	slock_t &m_slock;
};

}
}



#endif //UTILS_HPP