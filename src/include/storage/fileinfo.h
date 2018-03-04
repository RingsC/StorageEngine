/*-------------------------------------------------------------------------
 *
 * fileinfo.h
 * 	Relation file global information manager
 *
 * Global information of a relation file, including all its forks, such as the size of a file was absent
 * before. Initially in order to improve smgrnblocks performance we add this module. And it can be
 * used to manage more information about each file in future.
 *
 * -------------------------------------------------------------------------
 */

#ifndef FILEINFO_H
#define FILEINFO_H

#include "storage/lwlock.h"
#include "storage/shmem.h"
#include "utils/relcache.h"
#include "storage/relfilenode.h"
#include "storage/smgr.h"

#define NUM_FILEINFO_ENTRIES 524288 // 512K file entries, big enough.

#define FileInfoHashPartition(hashcode) \
	((hashcode) % NUM_FILEINFO_HASHTBL_PARTITIONS)
#define GetFileInfoPartitionLock(hashcode) \
	((LWLockId) (FirstFileInfoLock + FileInfoHashPartition(hashcode)))

struct FileInfoHashKey {
	RelFileNode relnode;
	ForkNumber forknum; // a relation's fork number
};

// FileInfo, used as hash entry for the file info hash table.
struct FileInfo {
	FileInfoHashKey fileInfoKey;// the key has to be in the hash entry too
	BlockNumber nblocks; // Number of blocks. 
	BlockNumber ntotalBlocks;// total number of blocks, including those new ones produced by last smgrextend_internal.
	// May add more info fields into this struct in future.
};

BlockNumber GetFileNumBlocks(SMgrRelation smgrreln, ForkNumber forknum, BlockNumber*, bool);
void SetFileNumBlocks(SMgrRelation smgrreln, ForkNumber forknum, BlockNumber newNBlocks, BlockNumber);
Size FileInfoShmemSize();
void FileInfoHashInitialize();
void FileInfoDelete(SMgrRelation smgrreln, ForkNumber forknum);
void FileInfoPartitionUnlock(uint32 hashcode);
uint32 FileInfoPartitionLock(SMgrRelation smgrreln, ForkNumber forknum, bool shareLock);
#endif /*FILEINFO_H*/
