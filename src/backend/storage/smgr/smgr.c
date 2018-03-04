/*-------------------------------------------------------------------------
 *
 * smgr.c
 *	  public interface routines to storage manager switch.
 *
 *	  All file system operations in POSTGRES dispatch through these
 *	  routines.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/storage/smgr/smgr.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/xlogutils.h"
#include "catalog/catalog.h"
#include "commands/tablespace.h"
#include "storage/bufmgr.h"
#include "storage/ipc.h"
#include "storage/smgr.h"
#include "utils/hsearch.h"
#include "utils/inval.h"
#include "storage/fileinfo.h"

/*
 * This struct of function pointers defines the API between smgr.c and
 * any individual storage manager module.  Note that smgr subfunctions are
 * generally expected to report problems via elog(ERROR).  An exception is
 * that smgr_unlink should use elog(WARNING), rather than erroring out,
 * because we normally unlink relations during post-commit/abort cleanup,
 * and so it's too late to raise an error.  Also, various conditions that
 * would normally be errors should be allowed during bootstrap and/or WAL
 * recovery --- see comments in md.c for details.
 */
typedef struct f_smgr
{
	void		(*smgr_init) (void);	/* may be NULL */
	void		(*smgr_shutdown) (void);		/* may be NULL */
	void		(*smgr_close) (SMgrRelation reln, ForkNumber forknum);
	void		(*smgr_create) (SMgrRelation reln, ForkNumber forknum,
											bool isRedo);
	bool		(*smgr_exists) (SMgrRelation reln, ForkNumber forknum);
	void		(*smgr_unlink) (RelFileNodeBackend rnode, ForkNumber forknum,
											bool isRedo);
	void		(*smgr_extend) (SMgrRelation reln, ForkNumber forknum,
						 BlockNumber blocknum, char *buffer, bool skipFsync);
	void		(*smgr_prefetch) (SMgrRelation reln, ForkNumber forknum,
											  BlockNumber blocknum);
	void		(*smgr_read) (SMgrRelation reln, ForkNumber forknum,
										  BlockNumber blocknum, char *buffer);
	void		(*smgr_write) (SMgrRelation reln, ForkNumber forknum,
						 BlockNumber blocknum, char *buffer, bool skipFsync);
	BlockNumber (*smgr_nblocks) (SMgrRelation reln, ForkNumber forknum);
	void		(*smgr_truncate) (SMgrRelation reln, ForkNumber forknum,
											  BlockNumber nblocks);
	void		(*smgr_immedsync) (SMgrRelation reln, ForkNumber forknum);
	void		(*smgr_pre_ckpt) (void);		/* may be NULL */
	void		(*smgr_sync) (void);	/* may be NULL */
	void		(*smgr_post_ckpt) (void);		/* may be NULL */
} f_smgr;


static THREAD_LOCAL const f_smgr smgrsw[] = {
	/* magnetic disk */
	{mdinit, NULL, mdclose, mdcreate, mdexists, mdunlink, mdextend,
		mdprefetch, mdread, mdwrite, mdnblocks, mdtruncate, mdimmedsync,
		mdpreckpt, mdsync, mdpostckpt
	}
};

static THREAD_LOCAL const int NSmgr = lengthof(smgrsw);


/*
 * Each backend has a hashtable that stores all extant SMgrRelation objects.
 */
static THREAD_LOCAL HTAB *SMgrRelationHash = NULL;

/* local function prototypes */
static void smgrshutdown(int code, Datum arg);

double fileIncrement = 0.2;

/*
 *	smgrinit(), smgrshutdown() -- Initialize or shut down storage
 *								  managers.
 *
 * Note: smgrinit is called during backend startup (normal or standalone
 * case), *not* during postmaster start.  Therefore, any resources created
 * here or destroyed in smgrshutdown are backend-local.
 */
void
smgrinit(void)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_init)
			(*(smgrsw[i].smgr_init)) ();
	}

	/* register the shutdown proc */
	on_proc_exit(smgrshutdown, 0);
}

/*
 * on_proc_exit hook for smgr cleanup during backend shutdown
 */
static void
smgrshutdown(int code, Datum arg)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_shutdown)
			(*(smgrsw[i].smgr_shutdown)) ();
	}
}

/*
 *	smgropen() -- Return an SMgrRelation object, creating it if need be.
 *
 *		This does not attempt to actually open the object.
 */
SMgrRelation
smgropen(RelFileNode rnode, BackendId backend)
{
	RelFileNodeBackend brnode;
	SMgrRelation reln;
	bool		found;

	if (SMgrRelationHash == NULL)
	{
		/* First time through: initialize the hash table */
		HASHCTL		ctl;

		MemSet(&ctl, 0, sizeof(ctl));
		ctl.keysize = sizeof(RelFileNodeBackend);
		ctl.entrysize = sizeof(SMgrRelationData);
		ctl.hash = tag_hash;
		SMgrRelationHash = hash_create("smgr relation table", 400,
									   &ctl, HASH_ELEM | HASH_FUNCTION);
	}

	/* Look up or create an entry */
	brnode.node = rnode;
	brnode.backend = backend;
	reln = (SMgrRelation) hash_search(SMgrRelationHash,
									  (void *) &brnode,
									  HASH_ENTER, &found);

	/* Initialize it if not present before */
	if (!found)
	{
		int			forknum;

		/* hash_search already filled in the lookup key */
		reln->smgr_owner = NULL;
		reln->smgr_targblock = InvalidBlockNumber;
		reln->smgr_fsm_nblocks = InvalidBlockNumber;
		reln->smgr_vm_nblocks = InvalidBlockNumber;
		reln->smgr_transient = false;
		reln->smgr_which = 0;	/* we only have md.c at present */

		/* mark it not open */
		for (forknum = 0; forknum <= MAX_FORKNUM; forknum++)
			reln->md_fd[forknum] = NULL;
	}
	else
		/* if it was transient before, it no longer is */
		reln->smgr_transient = false;

	return reln;
}

/*
 * smgrsettransient() -- mark an SMgrRelation object as transaction-bound
 *
 * The main effect of this is that all opened files are marked to be
 * kernel-level closed (but not necessarily VFD-closed) when the current
 * transaction ends.
 */
void
smgrsettransient(SMgrRelation reln)
{
	reln->smgr_transient = true;
}

/*
 * smgrsetowner() -- Establish a long-lived reference to an SMgrRelation object
 *
 * There can be only one owner at a time; this is sufficient since currently
 * the only such owners exist in the relcache.
 */
void
smgrsetowner(SMgrRelation *owner, SMgrRelation reln)
{
	/*
	 * First, unhook any old owner.  (Normally there shouldn't be any, but it
	 * seems possible that this can happen during swap_relation_files()
	 * depending on the order of processing.  It's ok to close the old
	 * relcache entry early in that case.)
	 */
	if (reln->smgr_owner)
		*(reln->smgr_owner) = NULL;

	/* Now establish the ownership relationship. */
	reln->smgr_owner = owner;
	*owner = reln;
}

/*
 *	smgrexists() -- Does the underlying file for a fork exist?
 */
bool
smgrexists(SMgrRelation reln, ForkNumber forknum)
{
	return (*(smgrsw[reln->smgr_which].smgr_exists)) (reln, forknum);
}

/*
 *	smgrclose() -- Close and delete an SMgrRelation object.
 */
void
smgrclose(SMgrRelation reln)
{
	SMgrRelation *owner;
	ForkNumber	forknum;

#ifndef FOUNDER_XDB_SE
	for (forknum = 0; forknum <= MAX_FORKNUM; forknum++)
		(*(smgrsw[reln->smgr_which].smgr_close)) (reln, forknum);

#else
    // don't evict fileinfo entry here because it's expensive to call smgrnblocks_internal now, 
    // and the fileinfo hash table is big enough.
    //uint32 hashcode = FileInfoPartitionLock(reln, forknum, true);
	for (forknum = (ForkNumber)0; forknum <= MAX_FORKNUM; forknum = (ForkNumber)(forknum+1)) {
	    //FileInfoDelete(reln, forknum);
		(*(smgrsw[reln->smgr_which].smgr_close)) (reln, forknum);
    }
    //FileInfoPartitionUnlock(hashcode);
#endif // FOUNDER_XDB_SE

	owner = reln->smgr_owner;

	if (hash_search(SMgrRelationHash,
					(void *) &(reln->smgr_rnode),
					HASH_REMOVE, NULL) == NULL)
		ereport(ERROR,
                (errcode(ERRCODE_DATA_CORRUPTED),
                errmsg("SMgrRelation hashtable corrupted")));

	/*
	 * Unhook the owner pointer, if any.  We do this last since in the remote
	 * possibility of failure above, the SMgrRelation object will still exist.
	 */
	if (owner)
		*owner = NULL;
}

/*
 *	smgrcloseall() -- Close all existing SMgrRelation objects.
 */
void
smgrcloseall(void)
{
	HASH_SEQ_STATUS status;
	SMgrRelation reln;

	/* Nothing to do if hashtable not set up */
	if (SMgrRelationHash == NULL)
		return;

	hash_seq_init(&status, SMgrRelationHash);

	while ((reln = (SMgrRelation) hash_seq_search(&status)) != NULL)
		smgrclose(reln);
}

/*
 *	smgrclosenode() -- Close SMgrRelation object for given RelFileNode,
 *					   if one exists.
 *
 * This has the same effects as smgrclose(smgropen(rnode)), but it avoids
 * uselessly creating a hashtable entry only to drop it again when no
 * such entry exists already.
 */
void
smgrclosenode(RelFileNodeBackend rnode)
{
	SMgrRelation reln;

	/* Nothing to do if hashtable not set up */
	if (SMgrRelationHash == NULL)
		return;

	reln = (SMgrRelation) hash_search(SMgrRelationHash,
									  (void *) &rnode,
									  HASH_FIND, NULL);
	if (reln != NULL)
		smgrclose(reln);
}

/*
 *	smgrcreate() -- Create a new relation.
 *
 *		Given an already-created (but presumably unused) SMgrRelation,
 *		cause the underlying disk file or other storage for the fork
 *		to be created.
 *
 *		If isRedo is true, it is okay for the underlying file to exist
 *		already because we are in a WAL replay sequence.
 */
void
smgrcreate(SMgrRelation reln, ForkNumber forknum, bool isRedo)
{
	/*
	 * Exit quickly in WAL replay mode if we've already opened the file. If
	 * it's open, it surely must exist.
	 */
	if (isRedo && reln->md_fd[forknum] != NULL)
		return;

	/*
	 * We may be using the target table space for the first time in this
	 * database, so create a per-database subdirectory if needed.
	 *
	 * XXX this is a fairly ugly violation of module layering, but this seems
	 * to be the best place to put the check.  Maybe TablespaceCreateDbspace
	 * should be here and not in commands/tablespace.c?  But that would imply
	 * importing a lot of stuff that smgr.c oughtn't know, either.
	 */
	TablespaceCreateDbspace(reln->smgr_rnode.node.spcNode,
							reln->smgr_rnode.node.dbNode,
							isRedo);

	(*(smgrsw[reln->smgr_which].smgr_create)) (reln, forknum, isRedo);
}

/*
 *	smgrdounlink() -- Immediately unlink a relation.
 *
 *		The specified fork of the relation is removed from the store.  This
 *		should not be used during transactional operations, since it can't be
 *		undone.
 *
 *		If isRedo is true, it is okay for the underlying file to be gone
 *		already.
 */
void
smgrdounlink(SMgrRelation reln, ForkNumber forknum, bool isRedo)
{
	RelFileNodeBackend rnode = reln->smgr_rnode;
	int			which = reln->smgr_which;
    
    uint32 hashcode = FileInfoPartitionLock(reln, forknum, false);
    FileInfoDelete(reln, forknum);
	/* Close the fork */
	(*(smgrsw[which].smgr_close)) (reln, forknum);

	/*
	 * Get rid of any remaining buffers for the relation.  bufmgr will just
	 * drop them without bothering to write the contents.
	 */
	DropRelFileNodeBuffers(rnode, forknum, 0);

	/*
	 * It'd be nice to tell the stats collector to forget it immediately, too.
	 * But we can't because we don't know the OID (and in cases involving
	 * relfilenode swaps, it's not always clear which table OID to forget,
	 * anyway).
	 */

	/*
	 * Send a shared-inval message to force other backends to close any
	 * dangling smgr references they may have for this rel.  We should do this
	 * before starting the actual unlinking, in case we fail partway through
	 * that step.  Note that the sinval message will eventually come back to
	 * this backend, too, and thereby provide a backstop that we closed our
	 * own smgr rel.
	 */
	CacheInvalidateSmgr(rnode);

	/*
	 * Delete the physical file(s).
	 *
	 * Note: smgr_unlink must treat deletion failure as a WARNING, not an
	 * ERROR, because we've already decided to commit or abort the current
	 * xact.
	 */
	(*(smgrsw[which].smgr_unlink)) (rnode, forknum, isRedo);
    
    FileInfoPartitionUnlock(hashcode);
	
}
// each physical extension extends as min 1 page ,max 16384 pages(16MB).
const static uint32 MinBlocksToExtend = 1;
const static uint32 MaxBlocksToExtend = 2048;
/*
 *	smgrextend() -- Add a new block to a file.
 *
 *		The semantics are nearly the same as smgrwrite(): write at the
 *		specified position.  However, this is to be used for the case of
 *		extending a relation (i.e., blocknum is at or beyond the current
 *		EOF).  Note that we assume writing a block beyond current EOF
 *		causes intervening file space to become filled with zeroes.
 *
 * In order to minimize IO and enhance performance, now we extend multiple (NBlocksToExtend+1) blocks at a time,
 * and since we have fileinfo to note down each relation file's info, we can store the actual(effective) NO. of pages
 * of the file.
 */
extern void LsmOverThreshold(Oid lsmSubIdxId);
void
smgrextend(SMgrRelation reln, ForkNumber forknum, BlockNumber blocknum,
		   char *buffer, bool skipFsync, BlockNumber threshold)
{
	if (fileIncrement > 1)
    {
        fileIncrement = 1;
    }
    else if (fileIncrement <= 0)
    {
        fileIncrement = 0.2;
    }
    BlockNumber datablocks = 0, totalBlocks = 0;
    char *zerobuf = (char *)palloc0(BLCKSZ);
    uint32 hashcode;
    
    /* 
     * The extension action and updating fileinfo should be atomized otherwise other 
     * threads may get an obsolete length value which is fatal. 
     * GetFileNumBlocks locks the same partition internally too, but it's OK, the lwlock does counting.
     */
    hashcode = FileInfoPartitionLock(reln, forknum, false);
    datablocks = GetFileNumBlocks(reln, forknum, &totalBlocks, false);
    BlockNumber incrementBlocks = 0;
    if (datablocks == totalBlocks || blocknum >= totalBlocks) {
        // Need a physical extension.
        // It's OK to extend beyond a segment size, mdextend will create another segment internally.
        Assert(datablocks <= blocknum);// blocknum is often the 1st block after the last existing block.
        
        // The blocknum'th block should have been written, but because we preallocate extra NBlocksToExtend
        // blocks, the blocknum+NBlocksToExtend'th block will be written, so write it with zeros and write the
        // blocknum'th with buffer content.
        if(totalBlocks==0)
        {
            incrementBlocks = MinBlocksToExtend;
        }
        else
        {
            incrementBlocks = (BlockNumber)(totalBlocks * fileIncrement);
        }
        //increase at least 1 blocks once.
        if (incrementBlocks < MinBlocksToExtend)
            incrementBlocks = MinBlocksToExtend;
		
		//increase not more than MaxBlocksToExtend pages
		if (incrementBlocks > MaxBlocksToExtend)
            incrementBlocks = MaxBlocksToExtend;
	    (*(smgrsw[reln->smgr_which].smgr_extend)) (reln, forknum, blocknum + incrementBlocks,
											       zerobuf, skipFsync);

        // make sure blocknum'th block is written with buffer's content. 										       
        smgrwrite(reln, forknum, blocknum, buffer, skipFsync);											       
        SetFileNumBlocks(reln, forknum, blocknum + 1, blocknum + incrementBlocks);
        
    } else {
        Assert(datablocks < totalBlocks && blocknum < totalBlocks && datablocks <= blocknum);
        // mdextend doesn't touch any other pages than the last one(i.e. the blocknum+NBlocksToExtend one)
        // all intermediate ones are supposed to be filled with zeros, so we need to write the blocknum'th block 
        // with buffer content here, but leave other blocks before blocknum alone, if any.
        // callers may pass zero page or a page with data, so we have to do every smgrwrite here, can't write all of them
        // in above branch. 
        // The original page is already filled with zeros, no need to write again.
        if (memcmp(zerobuf, buffer, BLCKSZ) != 0)
            smgrwrite(reln, forknum, blocknum, buffer, skipFsync);
        SetFileNumBlocks(reln, forknum, blocknum + 1, InvalidBlockNumber/*don't update totalBlocks*/);          
    }
    FileInfoPartitionUnlock(hashcode);

    if((threshold != InvalidBlockNumber) && (blocknum + 1 == threshold)){
    	LsmOverThreshold(reln->smgr_rnode.node.relNode);
    }
    pfree(zerobuf);
}

/*
 *	smgrprefetch() -- Initiate asynchronous read of the specified block of a relation.
 */
void
smgrprefetch(SMgrRelation reln, ForkNumber forknum, BlockNumber blocknum)
{
	(*(smgrsw[reln->smgr_which].smgr_prefetch)) (reln, forknum, blocknum);
}

/*
 *	smgrread() -- read a particular block from a relation into the supplied
 *				  buffer.
 *
 *		This routine is called from the buffer manager in order to
 *		instantiate pages in the shared buffer cache.  All storage managers
 *		return pages in the format that POSTGRES expects.
 */
void
smgrread(SMgrRelation reln, ForkNumber forknum, BlockNumber blocknum,
		 char *buffer)
{
	(*(smgrsw[reln->smgr_which].smgr_read)) (reln, forknum, blocknum, buffer);
}

/*
 *	smgrwrite() -- Write the supplied buffer out.
 *
 *		This is to be used only for updating already-existing blocks of a
 *		relation (ie, those before the current EOF).  To extend a relation,
 *		use smgrextend().
 *
 *		This is not a synchronous write -- the block is not necessarily
 *		on disk at return, only dumped out to the kernel.  However,
 *		provisions will be made to fsync the write before the next checkpoint.
 *
 *		skipFsync indicates that the caller will make other provisions to
 *		fsync the relation, so we needn't bother.  Temporary relations also
 *		do not require fsync.
 */
void
smgrwrite(SMgrRelation reln, ForkNumber forknum, BlockNumber blocknum,
		  char *buffer, bool skipFsync)
{
	(*(smgrsw[reln->smgr_which].smgr_write)) (reln, forknum, blocknum,
											  buffer, skipFsync);
}

/*
 *	smgrnblocks() -- Calculate the number of blocks in the
 *					 supplied relation.
 */
BlockNumber
smgrnblocks(SMgrRelation reln, ForkNumber forknum)
{
    // Upper layers don't know that the actual file has more blocks at the end, they know only
    // about the NO. of blocks containing data.
	return GetFileNumBlocks(reln, forknum, 0, true);// Get nblocks from FileInfo
}

/*
 * Count the NO. of valid pages and NO. of total blocks including the new ones at the end of the relation file.
 * This function is expensive because we will read and check many pages.
 */
BlockNumber
smgrnblocks_internal(SMgrRelation reln, ForkNumber forknum, BlockNumber*pTotalBlocks)
{
    Assert(pTotalBlocks != 0);
    BlockNumber nblocks = 0, startBlkno = 0, lastDataBlock = InvalidBlockNumber;
    
    *pTotalBlocks = (*(smgrsw[reln->smgr_which].smgr_nblocks)) (reln, forknum);
    
    // a normal file should now have at least MinlocksToExtend+1 blocks, so in this 
    // case this file was just unlinked or truncated
    if (*pTotalBlocks < MinBlocksToExtend + 1) 
        return *pTotalBlocks;
        
        
    //startBlkno = *pTotalBlocks - NBlocksToExtend - 1;
	//Assert(fileIncrement > 0.0f);

	//startBlkno = (BlockNumber)(((float)(*pTotalBlocks) / (1.0f + fileIncrement)));

	//// be conservative, because double<-->int multiplication/division/conversion can cause minor errors.
	//if (startBlkno * fileIncrement > NBlocksToExtend)
	//    startBlkno -= NBlocksToExtend;
	//else if (startBlkno > NBlocksToExtend)
	//    startBlkno -= NBlocksToExtend;
	//else
	//    startBlkno = 0;  

    char *buf = (char *)palloc0(BLCKSZ);
    
    // because a logical file is segmented, we can't easily read all tail
    // blocks once without breaking the modularity. And we could have read blocks in increasing order hoping
    // the OS's file system can do some prefetching for performance. However, there can be empty pages(holes) in a file after recovery,
    // so we MUST go through these pages from back to front otherwise we would overwrite valid pages.
    for (BlockNumber bn = *pTotalBlocks - 1; ; bn--) {
        smgrread(reln, forknum, bn, buf);
        if (!PageIsNew(buf))     
        {
            lastDataBlock = bn;
            elog(WARNING,"relation %d get %d blocks form tail",reln->smgr_rnode.node.relNode, *pTotalBlocks - 1 - bn);
            break;
        }
        if(bn == 0)
        {
            break;
        }
    }
    if (lastDataBlock == InvalidBlockNumber)
        lastDataBlock = *pTotalBlocks - 1;// no new pages, all are filled, so it's the last block.
    pfree(buf);
    return lastDataBlock + 1;
}

/*
 *	smgrtruncate() -- Truncate supplied relation to the specified number
 *					  of blocks
 *
 * The truncation is done immediately, so this can't be rolled back.
 */
void
smgrtruncate(SMgrRelation reln, ForkNumber forknum, BlockNumber nblocks)
{
    uint32 hashcode;
	/*
	 * Get rid of any buffers for the about-to-be-deleted blocks. bufmgr will
	 * just drop them without bothering to write the contents.
	 */
	DropRelFileNodeBuffers(reln->smgr_rnode, forknum, nblocks);

	/*
	 * Send a shared-inval message to force other backends to close any smgr
	 * references they may have for this rel.  This is useful because they
	 * might have open file pointers to segments that got removed, and/or
	 * smgr_targblock variables pointing past the new rel end.	(The inval
	 * message will come back to our backend, too, causing a
	 * probably-unnecessary local smgr flush.  But we don't expect that this
	 * is a performance-critical path.)  As in the unlink code, we want to be
	 * sure the message is sent before we start changing things on-disk.
	 */
	CacheInvalidateSmgr(reln->smgr_rnode);

	/*
	 * Do the truncation.
	 *
     * The truncation action and updating fileinfo should be atomized otherwise other 
     * threads may get an obsolete length value which is fatal. 
     */
	hashcode = FileInfoPartitionLock(reln, forknum, false);
	(*(smgrsw[reln->smgr_which].smgr_truncate)) (reln, forknum, nblocks);
	SetFileNumBlocks(reln, forknum, nblocks, nblocks);// update file info
	FileInfoPartitionUnlock(hashcode);
}

/*
 *	smgrimmedsync() -- Force the specified relation to stable storage.
 *
 *		Synchronously force all previous writes to the specified relation
 *		down to disk.
 *
 *		This is useful for building completely new relations (eg, new
 *		indexes).  Instead of incrementally WAL-logging the index build
 *		steps, we can just write completed index pages to disk with smgrwrite
 *		or smgrextend, and then fsync the completed index file before
 *		committing the transaction.  (This is sufficient for purposes of
 *		crash recovery, since it effectively duplicates forcing a checkpoint
 *		for the completed index.  But it is *not* sufficient if one wishes
 *		to use the WAL log for PITR or replication purposes: in that case
 *		we have to make WAL entries as well.)
 *
 *		The preceding writes should specify skipFsync = true to avoid
 *		duplicative fsyncs.
 *
 *		Note that you need to do FlushRelationBuffers() first if there is
 *		any possibility that there are dirty buffers for the relation;
 *		otherwise the sync is not very meaningful.
 */
void
smgrimmedsync(SMgrRelation reln, ForkNumber forknum)
{
	(*(smgrsw[reln->smgr_which].smgr_immedsync)) (reln, forknum);
}


/*
 *	smgrpreckpt() -- Prepare for checkpoint.
 */
void
smgrpreckpt(void)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_pre_ckpt)
			(*(smgrsw[i].smgr_pre_ckpt)) ();
	}
}

/*
 *	smgrsync() -- Sync files to disk during checkpoint.
 */
void
smgrsync(void)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_sync)
			(*(smgrsw[i].smgr_sync)) ();
	}
}

/*
 *	smgrpostckpt() -- Post-checkpoint cleanup.
 */
void
smgrpostckpt(void)
{
	int			i;

	for (i = 0; i < NSmgr; i++)
	{
		if (smgrsw[i].smgr_post_ckpt)
			(*(smgrsw[i].smgr_post_ckpt)) ();
	}
}

#ifdef FOUNDER_XDB_SE
void fxdb_free_smgrhash()
{
	if(SMgrRelationHash)
	{
		hash_destroy(SMgrRelationHash);
		SMgrRelationHash = NULL;
	}
}
#endif //FOUNDER_XDB_SE
