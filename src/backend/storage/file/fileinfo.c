#include "postgres.h"
#include "storage/fileinfo.h"
#include "utils/hsearch.h"

static void LockFileInfoPartition(uint32 hashcode, bool isshared);
static void InsertFileInfo(SMgrRelation smgrreln, ForkNumber forknum, BlockNumber initSize, BlockNumber);
static FileInfo *GetFileInfo (SMgrRelation smgrreln, ForkNumber forknum, bool forupdate, uint32 *phashcode, int);

#define PRINTHASH //ereport(LOG, (errmsg("\nhashcode: %u", hashcode)));

BlockNumber smgrnblocks_internal(SMgrRelation reln, ForkNumber forknum, BlockNumber*);

static THREAD_LOCAL HTAB *SharedFileInfoHash;// shared file info hash table
void FileInfoHashInitialize()
{
	HASHCTL		info;

	/* assume no locking is needed yet */

	/* FileInfoKey maps to FileInfo */
	info.keysize = sizeof(FileInfoHashKey);
	info.entrysize = sizeof(FileInfo);
	info.hash = tag_hash;
	info.num_partitions = NUM_FILEINFO_HASHTBL_PARTITIONS;

	SharedFileInfoHash = ShmemInitHash("Shared File Info Table",
		  NUM_FILEINFO_ENTRIES, NUM_FILEINFO_ENTRIES,
		  &info,
		  HASH_ELEM | HASH_FUNCTION | HASH_PARTITION);
}


static inline void LockFileInfoPartition(uint32 hashcode, bool isshared)
{
    LWLockId partLwlock = GetFileInfoPartitionLock(hashcode);
    LWLockAcquire(partLwlock, isshared ? LW_SHARED : LW_EXCLUSIVE);
}

static inline void UnlockFileInfoPartition(uint32 hashcode)
{
    LWLockRelease(GetFileInfoPartitionLock(hashcode));
}

Size FileInfoShmemSize()
{
    return hash_estimate_size(NUM_FILEINFO_ENTRIES, sizeof(FileInfo));
}

// Get file block counts, insert this info if it's not yet in the table.
// caller should tell via dolock whether we need to lock/unlock the partition.
BlockNumber GetFileNumBlocks(SMgrRelation smgrreln, ForkNumber forknum, BlockNumber*pTotalBlocks, bool dolock)
{
    uint32 hashcode;
	FileInfo *fi = GetFileInfo(smgrreln, forknum, false, &hashcode, dolock ? 1 : 0); 
	BlockNumber initsize = 0, ntotalBlocks = 0;

	if (fi == 0) {
	    /* 
	     * The entry for this file isn't in the hash table yet, need to physically 
	     * count the NO. of blocks and insert an entry for this file. Exclusive lock in case
	     * another thread is extending/truncating the file or in this function while we are in this function.
	     */
	    if (dolock)
	        LockFileInfoPartition(hashcode, false);
	    fi = GetFileInfo(smgrreln, forknum, false, &hashcode, 0);
	    /* someone else may have just inserted the entry for this file.*/
	    if (fi == 0) {
		    initsize = smgrnblocks_internal(smgrreln, forknum, &ntotalBlocks);
            if (pTotalBlocks)
		        *pTotalBlocks = ntotalBlocks;
		    InsertFileInfo(smgrreln, forknum, initsize, ntotalBlocks);
		} else {
		    initsize = fi->nblocks;
		    if (pTotalBlocks)
		        *pTotalBlocks = fi->ntotalBlocks;
		}
		if (dolock)
		    UnlockFileInfoPartition(hashcode);
	} else {
		initsize = fi->nblocks;
	    if (pTotalBlocks)
	        *pTotalBlocks = fi->ntotalBlocks;		
        if (dolock)
            UnlockFileInfoPartition(hashcode);
    }
	return initsize;
	
}

// delete the entry in the fileinfo hash table for the specified relation.
// the caller should lock and unlock the partition outside.
void
FileInfoDelete(SMgrRelation smgrreln, ForkNumber forknum)
{
	uint32 hashcode = 0;
	FileInfoHashKey fihk;
	fihk.relnode = smgrreln->smgr_rnode.node;
	fihk.forknum = forknum;
	hashcode = get_hash_value(SharedFileInfoHash, (void *) &fihk);
	PRINTHASH
	//LockFileInfoPartition(hashcode, false);

	FileInfo *result = (FileInfo *)
		hash_search_with_hash_value(SharedFileInfoHash,
									(void *) &fihk,
									hashcode,
									HASH_REMOVE,
									NULL);
    //UnlockFileInfoPartition(hashcode);
	//if (!result)				/* shouldn't happen */
	//	ereport(ERROR,
    //(errcode(ERRCODE_FUNCTION_RETURN_VALUE_WRONG),
    //errmsg("shared file info hash table corrupted")));
	// It's likely that some tables are not in the file info cache, if Set/GetFileNumBlocks are not called for them.
}

uint32 FileInfoPartitionLock(SMgrRelation smgrreln, ForkNumber forknum, bool shareLock)
{
	uint32 hashcode = 0;
	FileInfoHashKey fihk;
	fihk.relnode = smgrreln->smgr_rnode.node;
	fihk.forknum = forknum;
	hashcode = get_hash_value(SharedFileInfoHash, (void *) &fihk);
	PRINTHASH
	LockFileInfoPartition(hashcode, shareLock);
	return hashcode;
}
void FileInfoPartitionUnlock(uint32 hashcode)
{
    UnlockFileInfoPartition(hashcode);
}

// Get FileInfo struct from hash table for the specified relation's fork.
// Caller should unlock the partition lock for returned hash entry if it gave non-zero to dolock.
// lockbehavior: 0 for no lock, nonzero for lock
static FileInfo *GetFileInfo (SMgrRelation smgrreln, ForkNumber forknum, bool forupdate, uint32 *phashcode, int dolock)
{	
	FileInfo *result;
	uint32 hashcode = 0;
	FileInfoHashKey fihk;
	fihk.relnode = smgrreln->smgr_rnode.node;
	fihk.forknum = forknum;
	hashcode = get_hash_value(SharedFileInfoHash, (void *) &fihk);
	PRINTHASH
	Assert(phashcode != 0);
	*phashcode = hashcode;
	if (dolock)
	    LockFileInfoPartition(hashcode, !forupdate);
	result = (FileInfo *)hash_search_with_hash_value(SharedFileInfoHash,
								(void *) &fihk,
								hashcode,
								HASH_FIND,
								NULL);
    if (!result && dolock)// nothing returned, no need to keep the lwlock
        UnlockFileInfoPartition(hashcode);
	return result;
}	

// totalBlocks is a block count, so is totalBlocksInclNew. If totalBlocksInclNew is InvalidBlockNumber, don't update that field in the hash entry.
// caller should have locked the smgrreln's partition.
void SetFileNumBlocks(SMgrRelation smgrreln, ForkNumber forknum, BlockNumber totalBlocks, BlockNumber totalBlocksInclNew)
{
    FileInfo *result;
    uint32 hashcode;
    
    result = GetFileInfo(smgrreln, forknum, true, &hashcode, 0/*no lock*/);
    if (result) {
        result->nblocks = totalBlocks;
        if (totalBlocksInclNew != InvalidBlockNumber)
            result->ntotalBlocks = totalBlocksInclNew;
        //UnlockFileInfoPartition(hashcode);
    } else {
        Assert(InvalidBlockNumber != totalBlocksInclNew);
        InsertFileInfo(smgrreln, forknum, totalBlocks, totalBlocksInclNew);
    }
}
// nblocks must be the Number of Blocks, rather than a block number which starts from 0.
// caller should have locked the smgrreln's partition.
static void InsertFileInfo(SMgrRelation smgrreln, ForkNumber forknum, BlockNumber nblocks, BlockNumber totalBlocksInclNew)
{
	FileInfo *result;
	bool		found;
	FileInfoHashKey fihk;
	fihk.relnode = smgrreln->smgr_rnode.node;
	fihk.forknum = forknum;
	
    Assert(InvalidBlockNumber != totalBlocksInclNew);
	uint32 hashcode = get_hash_value(SharedFileInfoHash, (void *) &fihk);
	PRINTHASH

	//LockFileInfoPartition(hashcode, false);

	result = (FileInfo *)hash_search_with_hash_value(SharedFileInfoHash,
									(void *) &fihk,
									hashcode,
									HASH_ENTER_NULL,
									&found);
    // the hash table is full. this is unlikely to happen. if it happens, we should empty the hash partition here
    // since we have the partition lock. However dynahash doesn't yet support emptying a partition. so this is a TODO for fileinfo and dynahash.
    // For now when the fileinfo hash is full, we fall back to old style (i.e. directly calling mdnblocks, see GetFileNumBlocks)
    // for those files not in the fileinfo hash table. And more files will have a chance to be in the fileinfo hash table when any
    // ones already in the table are closed or deleted via smgrclose and smgrdounlink.
    if (result == NULL) {
        // !!! if we get here, performance will be degraded greatly by smgrnblocks_internal.
        elog(DEBUG1, "\nFile info hash table already full!");
    } else if (!found) {// some one else may have already inserted the entry for this file while we were waiting 
            // for the partition lock above in this function, in that case give up, our value is likely to be obsolete.
	    result->fileInfoKey = fihk;
	    result->nblocks = nblocks;
	    result->ntotalBlocks = totalBlocksInclNew;
	}
    //UnlockFileInfoPartition(hashcode);
	//Assert(!found); see above comment
}


