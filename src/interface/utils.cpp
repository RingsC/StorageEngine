#include <assert.h>
#include "postgres.h"

#include "catalog/metaData.h"


#include "interface/utils.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/ErrNo.h"

//#include "interface/pgStorageEngine.h"

namespace FounderXDB{
namespace StorageEngineNS {

//inline void entryIdToTid(const EntryID &eid, ItemPointerData &tid)

bool operator !=(const EntryID &eid, ItemPointerData &tid)
{
    if(eid.bi_hi != tid.ip_blkid.bi_hi 
        || eid.bi_lo != tid.ip_blkid.bi_lo 
        || eid.ip_posid != tid.ip_posid)
    return true;
    return false;
}
bool operator !=(const ItemPointerData &tid, EntryID &eid)
{
    if(eid.bi_hi != tid.ip_blkid.bi_hi 
        || eid.bi_lo != tid.ip_blkid.bi_lo 
        || eid.ip_posid != tid.ip_posid)
    return true;
    return false;    
}

void entryIdToTid(const EntryID &eid, ItemPointerData &tid)
{
    //assert(eid == tid);
    memcpy((void*)&tid, (void*)&eid, sizeof(EntryID));
}

void pentryIdToTid(const EntryID* eid, ItemPointerData &tid)
{
    //assert(eid == tid);
    memcpy((void*)&tid, (void*)eid, sizeof(EntryID));
}
//inline void TidToEntryId(const ItemPointerData &tid, EntryID &eid)
void TidToEntryId(const ItemPointerData &tid, EntryID &eid)
{
    //assert(tid = eid);
    memcpy((void*)&eid, (void*)&tid, sizeof(EntryID));
}

MetaTable formMtInfo(EntrySetType    type, TableSpaceID spcid, IndinfoData sinfo)
{
	MetaTable mt_info = NULL;
	mt_info = (MetaTableInfo *)FDPG_Memory::fd_palloc(sizeof(MetaTableInfo));
	mt_info->type = type;
	mt_info->tblspace = spcid;
	if (sinfo.index_num != 0) {
		mt_info->indinfo = (IndinfoData *)FDPG_Memory::fd_palloc(sizeof(IndinfoData));
		mt_info->indinfo->index_num = sinfo.index_num;
		int len_array = sizeof(int) * sinfo.index_num;
		mt_info->indinfo->index_array = (unsigned int *)FDPG_Memory::fd_palloc(len_array);
		memcpy(mt_info->indinfo->index_array, sinfo.index_array, len_array);
		int len_info = sizeof(Colinfo)* sinfo.index_num;
		mt_info->indinfo->index_info = (Colinfo *)FDPG_Memory::fd_palloc(len_info);
		for(unsigned int i = 0; i < sinfo.index_num; i++) {
			mt_info->indinfo->index_info[i] = (Colinfo)FDPG_Memory::fd_palloc(sizeof(ColinfoData));
			memcpy(mt_info->indinfo->index_info + i, sinfo.index_info + i, sizeof(ColinfoData));
		}
	} else {
		mt_info->indinfo = NULL;
	}
	return mt_info;
}

void deformMtInfo(MetaTable mt_info)
{
	Colinfo tmp = NULL;
	for(unsigned int i = 0; i < mt_info->indinfo->index_num; i++){
		tmp = mt_info->indinfo->index_info[i];
		if (tmp!= NULL) {
			FDPG_Memory::fd_pfree(tmp);
		}
	}
	if (mt_info->indinfo->index_array != NULL) {
		FDPG_Memory::fd_pfree(mt_info->indinfo->index_array);
		mt_info->indinfo->index_array = NULL;
	}
	if (mt_info->indinfo != NULL) {
		FDPG_Memory::fd_pfree(mt_info->indinfo);
		mt_info->indinfo = NULL;
	}
	if (mt_info) {
		FDPG_Memory::fd_pfree(mt_info);
	}
	mt_info = NULL;
}

// FOR TEST AND DELETE IN FUTURE
void my_split(RangeData &rangeData, char *str, int col)
{
    if (col == 1)
    {
        rangeData.start = 0;
        rangeData.len = 11;
    }
    if (col == 2)
    {
        rangeData.start = 3;
        rangeData.len = 2;
    }
    if (col == 3)
    {
        rangeData.start = 5;
        rangeData.len = 1;
    }
    return;
}

int seq_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
	size_t len = sizeof(SeqID);
	SeqID s1, s2;
	
	if (len1 == len && len2 == len){
		s1 = *(SeqID *)str1;
		s2 = *(SeqID*)str2;
		return s1 > s2 ? 1 : (s1 < s2 ? -1 : 0);
	} else {
		throw StorageEngineExceptionUniversal(LOGIC_ERR, "arg len error");
	}
}

int wfh_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
	size_t i = 0;
    while(i < len1 && i < len2)
    {
        if(str1[i] < str2[i])
            return -1;
        else if(str1[i] > str2[i])
            return 1;
        else i++;

    }
    if(len1 == len2)
        return 0;
    if(len1 > len2)
        return 1;
    return -1;
}



void setColumnInfo(uint32 colid, ColumnInfo *pcol_info)
{
    //pthread_mutex_lock(&setColinfo_mutex);
	//PGSpinLock lock(setColinfo_mutex);
    Colinfo colinfo = (Colinfo)pcol_info;
    setColInfo(colid, colinfo);
    //pthread_mutex_unlock(&setColinfo_mutex);

    return ;
}

PGSpinLock::PGSpinLock(slock_t &slock) : m_slock(slock)
{
	FDPG_Lock::fd_spinlock_acquire(m_slock);
	
}
PGSpinLock::~PGSpinLock() 
{ 
	FDPG_Lock::fd_spinlock_release(m_slock);
}

} //StorageEngineNS
} //FounderXDB