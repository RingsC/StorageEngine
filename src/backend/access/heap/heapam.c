/*-------------------------------------------------------------------------
 *
 * heapam.c
 *	  heap access method code
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/access/heap/heapam.c
 *
 *
 * INTERFACE ROUTINES
 *		relation_open	- open any relation by relation OID
 *		relation_openrv - open any relation specified by a RangeVar
 *		relation_close	- close any relation
 *		heap_open		- open a heap relation by relation OID
 *		heap_openrv		- open a heap relation specified by a RangeVar
 *		heap_close		- (now just a macro for relation_close)
 *		heap_beginscan	- begin relation scan
 *		heap_rescan		- restart a relation scan
 *		heap_endscan	- end relation scan
 *		heap_getnext	- retrieve next tuple in scan
 *		heap_fetch		- retrieve tuple with given tid
 *		heap_insert		- insert tuple into a relation
 *		heap_delete		- delete a tuple from a relation
 *		heap_update		- replace a tuple in a relation with another tuple
 *		heap_markpos	- mark scan position
 *		heap_restrpos	- restore position to marked location
 *		heap_sync		- sync heap, for when no WAL has been written
 *
 * NOTES
 *	  This file contains the heap_ routines which implement
 *	  the POSTGRES heap access method used for all POSTGRES
 *	  relations.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"


#include "access/heapam.h"
#include "access/hio.h"
#include "access/multixact.h"
#include "access/relscan.h"
#include "access/sysattr.h"
#include "access/transam.h"
#include "access/tuptoaster.h"
#include "access/valid.h"
#include "access/visibilitymap.h"
#include "access/xact.h"
#include "access/xlogutils.h"
#include "catalog/catalog.h"
#ifndef FOUNDER_XDB_SE
#include "catalog/namespace.h"
#else
#include "access/xdb_heap.h"
#include "catalog/xdb_catalog.h"
#endif //FOUNDER_XDB_SE
#include "miscadmin.h"
#include "pgstat.h"
#include "storage/proc.h"
#include "storage/bufmgr.h"
#include "storage/buf_internals.h"

#include "storage/freespace.h"
#include "storage/lmgr.h"
#include "storage/predicate.h"
#include "storage/procarray.h"
#include "storage/smgr.h"
#include "storage/standby.h"
#include "utils/datum.h"
#include "utils/inval.h"
#include "utils/lsyscache.h"
#include "utils/relcache.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"
#include "utils/tqual.h"
#include "utils/tuplesort.h"
#include "storage/proc.h"
extern TupleDesc single_attibute_tupDesc;
extern RelationAmInfo btree_aminfo ;
extern RelationAmInfo hash_aminfo ;
/* GUC variable */
bool		synchronize_seqscans = true;


static HeapScanDesc heap_beginscan_internal(Relation relation,
						Snapshot snapshot,
						int nkeys, ScanKey key,
						bool allow_strat, bool allow_sync,
						bool is_bitmapscan, filter_func_t filterfunc, void* userdata,BlockNumber start = 0,BlockNumber end = 0);
static XLogRecPtr log_heap_update(Relation reln, Buffer oldbuf,
				ItemPointerData from, Buffer newbuf, HeapTuple newtup,
				bool all_visible_cleared, bool new_all_visible_cleared);
static bool HeapSatisfiesHOTUpdate(Relation relation, Bitmapset *hot_attrs,
					   HeapTuple oldtup, HeapTuple newtup);

static HeapTuple heap_prepare_insert(Relation relation, HeapTuple tup,
					TransactionId xid, CommandId cid, int options);
static void heap_xlog_multi_insert(XLogRecPtr lsn, XLogRecord *record);
static void bitgetpage(HeapScanDesc scan, TBMIterateResult *tbmres, IndexScanDesc indexScan);
/* ----------------------------------------------------------------
 *						 heap support routines
 * ----------------------------------------------------------------
 */

/* ----------------
 *		initscan - scan code common to heap_beginscan and heap_rescan
 * ----------------
 */
static void
initscan(HeapScanDesc scan, ScanKey key, bool is_rescan)
{
	bool		allow_strat;
	bool		allow_sync;

	/*
	 * Determine the number of blocks we have to scan.
	 *
	 * It is sufficient to do this once at scan start, since any tuples added
	 * while the scan is in progress will be invisible to my snapshot anyway.
	 * (That is not true when using a non-MVCC snapshot.  However, we couldn't
	 * guarantee to return tuples added after scan start anyway, since they
	 * might go into pages we already scanned.	To guarantee consistent
	 * results for a non-MVCC snapshot, the caller must hold some higher-level
	 * lock that ensures the interesting tuple(s) won't change.)
	 */
	if(scan->rs_startblock && scan->rs_endblock)
	{
		scan->rs_nblocks = scan->rs_endblock;
	}
	else
	scan->rs_nblocks = RelationGetNumberOfBlocks(scan->rs_rd);

	/*
	 * If the table is large relative to NBuffers, use a bulk-read access
	 * strategy and enable synchronized scanning (see syncscan.c).	Although
	 * the thresholds for these features could be different, we make them the
	 * same so that there are only two behaviors to tune rather than four.
	 * (However, some callers need to be able to disable one or both of these
	 * behaviors, independently of the size of the table; also there is a GUC
	 * variable that can disable synchronized scanning.)
	 *
	 * During a rescan, don't make a new strategy object if we don't have to.
	 */
	if (!RelationUsesLocalBuffers(scan->rs_rd) &&
		scan->rs_nblocks > (BlockNumber)NBuffers / 4)
	{
		allow_strat = scan->rs_allow_strat;
		allow_sync = scan->rs_allow_sync;
	}
	else
		allow_strat = allow_sync = false;

	if (allow_strat)
	{
		if (scan->rs_strategy == NULL)
			scan->rs_strategy = GetAccessStrategy(BAS_BULKREAD);
	}
	else
	{
		if (scan->rs_strategy != NULL)
			FreeAccessStrategy(scan->rs_strategy);
		scan->rs_strategy = NULL;
	}

	if (is_rescan)
	{
		/*
		 * If rescan, keep the previous startblock setting so that rewinding a
		 * cursor doesn't generate surprising results.  Reset the syncscan
		 * setting, though.
		 */
		scan->rs_syncscan = (allow_sync && synchronize_seqscans);
	}
	else if (allow_sync && synchronize_seqscans)
	{
		scan->rs_syncscan = true;
		scan->rs_startblock = ss_get_location(scan->rs_rd, scan->rs_nblocks);
	}
	else
	{
		scan->rs_syncscan = false;
		//scan->rs_startblock = 0;
	}

	scan->rs_inited = false;
	scan->rs_ctup.t_data = NULL;
	ItemPointerSetInvalid(&scan->rs_ctup.t_self);
	scan->rs_cbuf = InvalidBuffer;
	scan->rs_cblock = InvalidBlockNumber;

	/* we don't have a marked position... */
	ItemPointerSetInvalid(&(scan->rs_mctid));

	/* page-at-a-time fields are always invalid when not rs_inited */

	/*
	 * copy the scan key, if appropriate
	 */
	if (key != NULL)
		memcpy(scan->rs_key, key, scan->rs_nkeys * sizeof(ScanKeyData));

	/*
	 * Currently, we don't have a stats counter for bitmap heap scans (but the
	 * underlying bitmap index scans will be counted).
	 */
	if (!scan->rs_bitmapscan)
		pgstat_count_heap_scan(scan->rs_rd);
}

/*
 * heapgetpage - subroutine for heapgettup()
 *
 * This routine reads and pins the specified page of the relation.
 * In page-at-a-time mode it performs additional work, namely determining
 * which tuples on the page are visible.
 */
static void
heapgetpage(HeapScanDesc scan, BlockNumber page)
{
	Buffer		buffer;
	Snapshot	snapshot;
	Page		dp;
	int			lines;
	int			ntup;
	OffsetNumber lineoff;
	ItemId		lpp;
	bool		all_visible;

	Assert(page < scan->rs_nblocks);

	/* release previous scan buffer, if any */
	if (BufferIsValid(scan->rs_cbuf))
	{
		ReleaseBuffer(scan->rs_cbuf);
		scan->rs_cbuf = InvalidBuffer;
	}

	/* read page using selected strategy */
	scan->rs_cbuf = ReadBufferExtended(scan->rs_rd, MAIN_FORKNUM, page,
									   RBM_NORMAL, scan->rs_strategy);
	scan->rs_cblock = page;

	if (!scan->rs_pageatatime)
		return;

	buffer = scan->rs_cbuf;
	snapshot = scan->rs_snapshot;

	/*
	 * Prune and repair fragmentation for the whole page, if possible.
	 */
	Assert(TransactionIdIsValid(RecentGlobalXmin));
	heap_page_prune_opt(scan->rs_rd, buffer, RecentGlobalXmin);

	/*
	 * We must hold share lock on the buffer content while examining tuple
	 * visibility.	Afterwards, however, the tuples we have found to be
	 * visible are guaranteed good as long as we hold the buffer pin.
	 */
	LockBuffer(buffer, BUFFER_LOCK_SHARE);

	dp = (Page) BufferGetPage(buffer);
	lines = PageGetMaxOffsetNumber(dp);
	ntup = 0;

	/*
	 * If the all-visible flag indicates that all tuples on the page are
	 * visible to everyone, we can skip the per-tuple visibility tests. But
	 * not in hot standby mode. A tuple that's already visible to all
	 * transactions in the master might still be invisible to a read-only
	 * transaction in the standby.
	 */
	all_visible = PageIsAllVisible(dp) && !snapshot->takenDuringRecovery;

	for (lineoff = FirstOffsetNumber, lpp = PageGetItemId(dp, lineoff);
		 lineoff <= lines;
		 lineoff++, lpp++)
	{
		if (ItemIdIsNormal(lpp))
		{
			HeapTupleData loctup;
			bool		valid;

			loctup.t_data = (HeapTupleHeader) PageGetItem((Page) dp, lpp);
			loctup.t_len = ItemIdGetLength(lpp);
			ItemPointerSet(&(loctup.t_self), page, lineoff);

			if (all_visible)
				valid = true;
			else
				valid = HeapTupleSatisfiesVisibility(&loctup, snapshot, buffer);

			CheckForSerializableConflictOut(valid, scan->rs_rd, &loctup,
											buffer, snapshot);

			if (valid)
				scan->rs_vistuples[ntup++] = lineoff;
		}
	}

	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

	Assert(ntup <= MaxHeapTuplesPerPage);
	scan->rs_ntuples = ntup;
}

/* ----------------
 *		heapgettup - fetch next heap tuple
 *
 *		Initialize the scan if not already done; then advance to the next
 *		tuple as indicated by "dir"; return the next tuple in scan->rs_ctup,
 *		or set scan->rs_ctup.t_data = NULL if no more tuples.
 *
 * dir == NoMovementScanDirection means "re-fetch the tuple indicated
 * by scan->rs_ctup".
 *
 * Note: the reason nkeys/key are passed separately, even though they are
 * kept in the scan descriptor, is that the caller may not want us to check
 * the scankeys.
 *
 * Note: when we fall off the end of the scan in either direction, we
 * reset rs_inited.  This means that a further request with the same
 * scan direction will restart the scan, which is a bit odd, but a
 * request with the opposite scan direction will start a fresh scan
 * in the proper direction.  The latter is required behavior for cursors,
 * while the former case is generally undefined behavior in Postgres
 * so we don't care too much.
 * ----------------
 */
static void
heapgettup(HeapScanDesc scan,
		   ScanDirection dir,
		   int nkeys,
		   ScanKey key)
{
	HeapTuple	tuple = &(scan->rs_ctup);
	Snapshot	snapshot = scan->rs_snapshot;
	bool		backward = ScanDirectionIsBackward(dir);
	BlockNumber page;
	bool		finished;
	Page		dp;
	int			lines;
	OffsetNumber lineoff;
	int			linesleft;
	ItemId		lpp;

	/*
	 * calculate next starting lineoff, given scan direction
	 */
	if (ScanDirectionIsForward(dir))
	{
		if (!scan->rs_inited)
		{
			/*
			 * return null immediately if relation is empty
			 */
			if (scan->rs_nblocks == 0)
			{
				Assert(!BufferIsValid(scan->rs_cbuf));
				tuple->t_data = NULL;
				return;
			}
			page = scan->rs_startblock; /* first page */
			heapgetpage(scan, page);
			lineoff = FirstOffsetNumber;		/* first offnum */
			scan->rs_inited = true;
		}
		else
		{
			/* continue from previously returned page/tuple */
			page = scan->rs_cblock;		/* current page */
			lineoff =			/* next offnum */
				OffsetNumberNext(ItemPointerGetOffsetNumber(&(tuple->t_self)));
		}

		LockBuffer(scan->rs_cbuf, BUFFER_LOCK_SHARE);

		dp = (Page) BufferGetPage(scan->rs_cbuf);
		lines = PageGetMaxOffsetNumber(dp);
		/* page and lineoff now reference the physically next tid */

		linesleft = lines - lineoff + 1;
	}
	else if (backward)
	{
		if (!scan->rs_inited)
		{
			/*
			 * return null immediately if relation is empty
			 */
			if (scan->rs_nblocks == 0)
			{
				Assert(!BufferIsValid(scan->rs_cbuf));
				tuple->t_data = NULL;
				return;
			}

			/*
			 * Disable reporting to syncscan logic in a backwards scan; it's
			 * not very likely anyone else is doing the same thing at the same
			 * time, and much more likely that we'll just bollix things for
			 * forward scanners.
			 */
			scan->rs_syncscan = false;
			/* start from last page of the scan */
			if (scan->rs_startblock > 0)
				page = scan->rs_startblock - 1;
			else
				page = scan->rs_nblocks - 1;
			heapgetpage(scan, page);
		}
		else
		{
			/* continue from previously returned page/tuple */
			page = scan->rs_cblock;		/* current page */
		}

		LockBuffer(scan->rs_cbuf, BUFFER_LOCK_SHARE);

		dp = (Page) BufferGetPage(scan->rs_cbuf);
		lines = PageGetMaxOffsetNumber(dp);

		if (!scan->rs_inited)
		{
			lineoff = lines;	/* final offnum */
			scan->rs_inited = true;
		}
		else
		{
			lineoff =			/* previous offnum */
				OffsetNumberPrev(ItemPointerGetOffsetNumber(&(tuple->t_self)));
		}
		/* page and lineoff now reference the physically previous tid */

		linesleft = lineoff;
	}
	else
	{
		/*
		 * ``no movement'' scan direction: refetch prior tuple
		 */
		if (!scan->rs_inited)
		{
			Assert(!BufferIsValid(scan->rs_cbuf));
			tuple->t_data = NULL;
			return;
		}

		page = ItemPointerGetBlockNumber(&(tuple->t_self));
		if (page != scan->rs_cblock)
			heapgetpage(scan, page);

		/* Since the tuple was previously fetched, needn't lock page here */
		dp = (Page) BufferGetPage(scan->rs_cbuf);
		lineoff = ItemPointerGetOffsetNumber(&(tuple->t_self));
		lpp = PageGetItemId(dp, lineoff);
		Assert(ItemIdIsNormal(lpp));

		tuple->t_data = (HeapTupleHeader) PageGetItem((Page) dp, lpp);
		tuple->t_len = ItemIdGetLength(lpp);

		return;
	}

	/*
	 * advance the scan until we find a qualifying tuple or run out of stuff
	 * to scan
	 */
	lpp = PageGetItemId(dp, lineoff);
	for (;;)
	{
		while (linesleft > 0)
		{
			if (ItemIdIsNormal(lpp))
			{
				bool		valid;

				tuple->t_data = (HeapTupleHeader) PageGetItem((Page) dp, lpp);
				tuple->t_len = ItemIdGetLength(lpp);
				ItemPointerSet(&(tuple->t_self), page, lineoff);

				/*
				 * if current tuple qualifies, return it.
				 */
				valid = HeapTupleSatisfiesVisibility(tuple,
													 snapshot,
													 scan->rs_cbuf);

				CheckForSerializableConflictOut(valid, scan->rs_rd, tuple,
												scan->rs_cbuf, snapshot);

				if ((valid && key != NULL) || (valid && scan->filter_func))
				{
#ifndef FOUNDER_XDB_SE
					HeapKeyTest(tuple, RelationGetDescr(scan->rs_rd),
						nkeys, key, valid);
#else
					valid = fdxdb_HeapKeyTest(tuple, scan, nkeys, key);
#endif //FOUNDER_XDB_SE
				}

				if (valid)
				{
					LockBuffer(scan->rs_cbuf, BUFFER_LOCK_UNLOCK);
					return;
				}				
			}

			/*
			 * otherwise move to the next item on the page
			 */
			--linesleft;
			if (backward)
			{
				--lpp;			/* move back in this page's ItemId array */
				--lineoff;
			}
			else
			{
				++lpp;			/* move forward in this page's ItemId array */
				++lineoff;
			}
		}

		/*
		 * if we get here, it means we've exhausted the items on this page and
		 * it's time to move to the next.
		 */
		LockBuffer(scan->rs_cbuf, BUFFER_LOCK_UNLOCK);

		/*
		 * advance to next/prior page and detect end of scan
		 */
		if (backward)
		{
			finished = (page == scan->rs_startblock);
			if (page == 0)
				page = scan->rs_nblocks;
			page--;
		}
		else
		{
			page++;
			if (page >= scan->rs_nblocks)
				page = 0;
			finished = (page == scan->rs_startblock);

			/*
			 * Report our new scan position for synchronization purposes. We
			 * don't do that when moving backwards, however. That would just
			 * mess up any other forward-moving scanners.
			 *
			 * Note: we do this before checking for end of scan so that the
			 * final state of the position hint is back at the start of the
			 * rel.  That's not strictly necessary, but otherwise when you run
			 * the same query multiple times the starting position would shift
			 * a little bit backwards on every invocation, which is confusing.
			 * We don't guarantee any specific ordering in general, though.
			 */
			if (scan->rs_syncscan)
				ss_report_location(scan->rs_rd, page);
		}

		/*
		 * return NULL if we've exhausted all the pages
		 */
		if (finished)
		{
			if (BufferIsValid(scan->rs_cbuf))
				ReleaseBuffer(scan->rs_cbuf);
			scan->rs_cbuf = InvalidBuffer;
			scan->rs_cblock = InvalidBlockNumber;
			tuple->t_data = NULL;
			scan->rs_inited = false;
			return;
		}

		heapgetpage(scan, page);

		LockBuffer(scan->rs_cbuf, BUFFER_LOCK_SHARE);

		dp = (Page) BufferGetPage(scan->rs_cbuf);
		lines = PageGetMaxOffsetNumber((Page) dp);
		linesleft = lines;
		if (backward)
		{
			lineoff = lines;
			lpp = PageGetItemId(dp, lines);
		}
		else
		{
			lineoff = FirstOffsetNumber;
			lpp = PageGetItemId(dp, FirstOffsetNumber);
		}
	}
}

/* ----------------
 *		heapgettup_pagemode - fetch next heap tuple in page-at-a-time mode
 *
 *		Same API as heapgettup, but used in page-at-a-time mode
 *
 * The internal logic is much the same as heapgettup's too, but there are some
 * differences: we do not take the buffer content lock (that only needs to
 * happen inside heapgetpage), and we iterate through just the tuples listed
 * in rs_vistuples[] rather than all tuples on the page.  Notice that
 * lineindex is 0-based, where the corresponding loop variable lineoff in
 * heapgettup is 1-based.
 * ----------------
 */
static void
heapgettup_pagemode(HeapScanDesc scan,
					ScanDirection dir,
					int nkeys,
					ScanKey key)
{
	HeapTuple	tuple = &(scan->rs_ctup);
	bool		backward = ScanDirectionIsBackward(dir);
	BlockNumber page;
	bool		finished;
	Page		dp;
	int			lines;
	int			lineindex;
	OffsetNumber lineoff;
	int			linesleft;
	ItemId		lpp;

	/*
	 * calculate next starting lineindex, given scan direction
	 */
	if (ScanDirectionIsForward(dir))
	{
		if (!scan->rs_inited)
		{
			/*
			 * return null immediately if relation is empty
			 */
			if (scan->rs_nblocks == 0)
			{
				Assert(!BufferIsValid(scan->rs_cbuf));
				tuple->t_data = NULL;
				return;
			}
			page = scan->rs_startblock; /* first page */
			heapgetpage(scan, page);
			lineindex = 0;
			scan->rs_inited = true;
		}
		else
		{
			/* continue from previously returned page/tuple */
			page = scan->rs_cblock;		/* current page */
			lineindex = scan->rs_cindex + 1;
		}

		dp = (Page) BufferGetPage(scan->rs_cbuf);
		lines = scan->rs_ntuples;
		/* page and lineindex now reference the next visible tid */

		linesleft = lines - lineindex;
	}
	else if (backward)
	{
		if (!scan->rs_inited)
		{
			/*
			 * return null immediately if relation is empty
			 */
			if (scan->rs_nblocks == 0)
			{
				Assert(!BufferIsValid(scan->rs_cbuf));
				tuple->t_data = NULL;
				return;
			}

			/*
			 * Disable reporting to syncscan logic in a backwards scan; it's
			 * not very likely anyone else is doing the same thing at the same
			 * time, and much more likely that we'll just bollix things for
			 * forward scanners.
			 */
			scan->rs_syncscan = false;
			/* start from last page of the scan */
			if (scan->rs_startblock > 0)
				page = scan->rs_startblock - 1;
			else
				page = scan->rs_nblocks - 1;
			heapgetpage(scan, page);
		}
		else
		{
			/* continue from previously returned page/tuple */
			page = scan->rs_cblock;		/* current page */
		}

		dp = (Page) BufferGetPage(scan->rs_cbuf);
		lines = scan->rs_ntuples;

		if (!scan->rs_inited)
		{
			lineindex = lines - 1;
			scan->rs_inited = true;
		}
		else
		{
			lineindex = scan->rs_cindex - 1;
		}
		/* page and lineindex now reference the previous visible tid */

		linesleft = lineindex + 1;
	}
	else
	{
		/*
		 * ``no movement'' scan direction: refetch prior tuple
		 */
		if (!scan->rs_inited)
		{
			Assert(!BufferIsValid(scan->rs_cbuf));
			tuple->t_data = NULL;
			return;
		}

		page = ItemPointerGetBlockNumber(&(tuple->t_self));
		if (page != scan->rs_cblock)
			heapgetpage(scan, page);

		/* Since the tuple was previously fetched, needn't lock page here */
		dp = (Page) BufferGetPage(scan->rs_cbuf);
		lineoff = ItemPointerGetOffsetNumber(&(tuple->t_self));
		lpp = PageGetItemId(dp, lineoff);
		Assert(ItemIdIsNormal(lpp));

		tuple->t_data = (HeapTupleHeader) PageGetItem((Page) dp, lpp);
		tuple->t_len = ItemIdGetLength(lpp);

		/* check that rs_cindex is in sync */
		Assert(scan->rs_cindex < scan->rs_ntuples);
		Assert(lineoff == scan->rs_vistuples[scan->rs_cindex]);

		return;
	}

	/*
	 * advance the scan until we find a qualifying tuple or run out of stuff
	 * to scan
	 */
	for (;;)
	{
		while (linesleft > 0)
		{
			lineoff = scan->rs_vistuples[lineindex];
			lpp = PageGetItemId(dp, lineoff);
			Assert(ItemIdIsNormal(lpp));

			tuple->t_data = (HeapTupleHeader) PageGetItem((Page) dp, lpp);
			tuple->t_len = ItemIdGetLength(lpp);
			ItemPointerSet(&(tuple->t_self), page, lineoff);

			/*
			 * if current tuple qualifies, return it.
			 */
			if (key != NULL)
			{
				bool		valid;

#ifndef FOUNDER_XDB_SE
					HeapKeyTest(tuple, RelationGetDescr(scan->rs_rd),
						nkeys, key, valid);
#else
					valid = fdxdb_HeapKeyTest(tuple, scan, nkeys, key);
#endif //FOUNDER_XDB_SE
				if (valid)
				{
					scan->rs_cindex = lineindex;
					return;
				}
				
			}
			else
			{
				scan->rs_cindex = lineindex;
				return;
			}

			/*
			 * otherwise move to the next item on the page
			 */
			--linesleft;
			if (backward)
				--lineindex;
			else
				++lineindex;
		}

		/*
		 * if we get here, it means we've exhausted the items on this page and
		 * it's time to move to the next.
		 */
		if (backward)
		{
			finished = (page == scan->rs_startblock);
			if (page == 0)
				page = scan->rs_nblocks;
			page--;
		}
		else
		{
			page++;
			if (page >= scan->rs_nblocks)
				page = scan->rs_startblock;
			finished = (page == scan->rs_startblock);

			/*
			 * Report our new scan position for synchronization purposes. We
			 * don't do that when moving backwards, however. That would just
			 * mess up any other forward-moving scanners.
			 *
			 * Note: we do this before checking for end of scan so that the
			 * final state of the position hint is back at the start of the
			 * rel.  That's not strictly necessary, but otherwise when you run
			 * the same query multiple times the starting position would shift
			 * a little bit backwards on every invocation, which is confusing.
			 * We don't guarantee any specific ordering in general, though.
			 */
			if (scan->rs_syncscan)
				ss_report_location(scan->rs_rd, page);
		}

		/*
		 * return NULL if we've exhausted all the pages
		 */
		if (finished)
		{
			if (BufferIsValid(scan->rs_cbuf))
				ReleaseBuffer(scan->rs_cbuf);
			scan->rs_cbuf = InvalidBuffer;
			scan->rs_cblock = InvalidBlockNumber;
			tuple->t_data = NULL;
			scan->rs_inited = false;
			return;
		}

		heapgetpage(scan, page);

		dp = (Page) BufferGetPage(scan->rs_cbuf);
		lines = scan->rs_ntuples;
		linesleft = lines;
		if (backward)
			lineindex = lines - 1;
		else
			lineindex = 0;
	}
}


#if defined(DISABLE_COMPLEX_MACRO)
/*
 * This is formatted so oddly so that the correspondence to the macro
 * definition in access/heapam.h is maintained.
 */
Datum
fastgetattr(HeapTuple tup, int attnum, TupleDesc tupleDesc,
			bool *isnull)
{
	return (
			(attnum) > 0 ?
			(
			 (*(isnull) = false),
			 HeapTupleNoNulls(tup) ?
			 (
			  (tupleDesc)->attrs[(attnum) - 1]->attcacheoff >= 0 ?
			  (
			   fetchatt((tupleDesc)->attrs[(attnum) - 1],
						(char *) (tup)->t_data + (tup)->t_data->t_hoff +
						(tupleDesc)->attrs[(attnum) - 1]->attcacheoff)
			   )
			  :
			  nocachegetattr((tup), (attnum), (tupleDesc))
			  )
			 :
			 (
			  att_isnull((attnum) - 1, (tup)->t_data->t_bits) ?
			  (
			   (*(isnull) = true),
			   (Datum) NULL
			   )
			  :
			  (
			   nocachegetattr((tup), (attnum), (tupleDesc))
			   )
			  )
			 )
			:
			(
			 (Datum) NULL
			 )
		);
}
#endif   /* defined(DISABLE_COMPLEX_MACRO) */


/* ----------------------------------------------------------------
 *					 heap access method interface
 * ----------------------------------------------------------------
 */

/* ----------------
 *		relation_open - open any relation by relation OID
 *
 *		If lockmode is not "NoLock", the specified kind of lock is
 *		obtained on the relation.  (Generally, NoLock should only be
 *		used if the caller knows it has some appropriate lock on the
 *		relation already.)
 *
 *		An error is raised if the relation does not exist.
 *
 *		NB: a "relation" is anything with a pg_class entry.  The caller is
 *		expected to check whether the relkind is something it can handle.
 * ----------------
 */
Relation
relation_open(Oid relationId, LOCKMODE lockmode,Oid dbid)
{
	Relation	r;

	Assert(lockmode >= NoLock && lockmode < MAX_LOCKMODES);

	/* Get the lock before trying to open the relcache entry */
	if (lockmode != NoLock && !IsTmpOid(relationId))
		LockRelationOid(relationId, lockmode);

	/* The relcache does all the real work... */
	r = RelationIdGetRelation(relationId);

#ifdef FOUNDER_XDB_SE
    MemoryContext oldcxt = MemoryContextSwitchTo(CacheMemoryContext);
    if (!r) 
	{		
    	if(IsTmpOid(relationId))
    	{
    		MemoryContextSwitchTo(oldcxt);
    		return NULL;
    	}
		extern Relation RelationBuildDesc(Oid relationId, bool insertIt,bool,Oid);
		r = RelationBuildDesc(relationId,true,true,dbid);
	
		if (!r)
		{
			MemoryContextSwitchTo(oldcxt);
			if (lockmode != NoLock && !IsTmpOid(relationId))
				UnlockRelationOid(relationId, lockmode);
			return r;
		}
		else
		{
			r->lockMode = lockmode;
		}
    }

	MemoryContextSwitchTo(oldcxt);
#endif //FOUNDER_XDB_SE
#ifndef FOUNDER_XDB_SE

	if (!RelationIsValid(r))
		elog(ERROR, "could not open relation with OID %u", relationId);

	/* Make note that we've accessed a temporary relation */
	if (RelationUsesLocalBuffers(r))
		MyXactAccessedTempRel = true;

	pgstat_initstats(r);

#endif //FOUNDER_XDB_SE

	return r;
}

/* ----------------
 *		try_relation_open - open any relation by relation OID
 *
 *		Same as relation_open, except return NULL instead of failing
 *		if the relation does not exist.
 * ----------------
 */
Relation
try_relation_open(Oid relationId, LOCKMODE lockmode)
{
	Relation	r;

	Assert(lockmode >= NoLock && lockmode < MAX_LOCKMODES);

	/* Get the lock first */
	if (lockmode != NoLock)
		LockRelationOid(relationId, lockmode);

	/*
	 * Now that we have the lock, probe to see if the relation really exists
	 * or not.
	 */
#ifndef FOUNDER_XDB_SE
	if (!SearchSysCacheExists1(RELOID, ObjectIdGetDatum(relationId)))
#else
	if (!fxdb_search_meta_table_copy(relationId))
#endif //FOUNDER_XDB_SE
	{
		/* Release useless lock */
		if (lockmode != NoLock)
			UnlockRelationOid(relationId, lockmode);

		return NULL;
	}

	/* Should be safe to do a relcache load */
	r = RelationIdGetRelation(relationId);

	if (!RelationIsValid(r))
		ereport(ERROR,
                (errcode(ERRCODE_HEAP_OPEN_WITH_OID_FAILURE),
                errmsg("could not open relation with OID %u", relationId)));

	/* Make note that we've accessed a temporary relation */
	if (RelationUsesLocalBuffers(r))
		MyXactAccessedTempRel = true;

	pgstat_initstats(r);

	return r;
}

/* ----------------
 *		relation_openrv - open any relation specified by a RangeVar
 *
 *		Same as relation_open, but the relation is specified by a RangeVar.
 * ----------------
 */
Relation
relation_openrv(const RangeVar *relation, LOCKMODE lockmode)
{
	Oid			relOid;

	/*
	 * Check for shared-cache-inval messages before trying to open the
	 * relation.  This is needed to cover the case where the name identifies a
	 * rel that has been dropped and recreated since the start of our
	 * transaction: if we don't flush the old syscache entry then we'll latch
	 * onto that entry and suffer an error when we do RelationIdGetRelation.
	 * Note that relation_open does not need to do this, since a relation's
	 * OID never changes.
	 *
	 * We skip this if asked for NoLock, on the assumption that the caller has
	 * already ensured some appropriate lock is held.
	 */
	if (lockmode != NoLock)
		AcceptInvalidationMessages();

	/* Look up the appropriate relation using namespace search */
	relOid = RangeVarGetRelid(relation, false);

	/* Let relation_open do the rest */
	return relation_open(relOid, lockmode);
}

/* ----------------
 *		try_relation_openrv - open any relation specified by a RangeVar
 *
 *		Same as relation_openrv, but return NULL instead of failing for
 *		relation-not-found.  (Note that some other causes, such as
 *		permissions problems, will still result in an ereport.)
 * ----------------
 */
Relation
try_relation_openrv(const RangeVar *relation, LOCKMODE lockmode)
{
	Oid			relOid;

	/*
	 * Check for shared-cache-inval messages before trying to open the
	 * relation.  This is needed to cover the case where the name identifies a
	 * rel that has been dropped and recreated since the start of our
	 * transaction: if we don't flush the old syscache entry then we'll latch
	 * onto that entry and suffer an error when we do RelationIdGetRelation.
	 * Note that relation_open does not need to do this, since a relation's
	 * OID never changes.
	 *
	 * We skip this if asked for NoLock, on the assumption that the caller has
	 * already ensured some appropriate lock is held.
	 */
	if (lockmode != NoLock)
		AcceptInvalidationMessages();

	/* Look up the appropriate relation using namespace search */
	relOid = RangeVarGetRelid(relation, true);

	/* Return NULL on not-found */
	if (!OidIsValid(relOid))
		return NULL;

	/* Let relation_open do the rest */
	return relation_open(relOid, lockmode);
}

/* ----------------
 *		relation_close - close any relation
 *
 *		If lockmode is not "NoLock", we then release the specified lock.
 *
 *		Note that it is often sensible to hold a lock beyond relation_close;
 *		in that case, the lock is released automatically at xact end.
 * ----------------
 */
void
relation_close(Relation relation, LOCKMODE lockmode)
{
	LockRelId	relid = relation->rd_lockInfo.lockRelId;

	Assert(lockmode >= NoLock && lockmode < MAX_LOCKMODES);

	/* The relcache does the real work... */
	RelationClose(relation);

	if (lockmode != NoLock && !IsTmpOid(relation->rd_node.relNode))
	{
		UnlockRelationId(&relid, lockmode);
		relation->lockMode = NoLock; 
	}
}


/* ----------------
 *		heap_open - open a heap relation by relation OID
 *
 *		This is essentially relation_open plus check that the relation
 *		is not an index nor a composite type.  (The caller should also
 *		check that it's not a view or foreign table before assuming it has
 *		storage.)
 * ----------------
 */
Relation
heap_open(Oid relationId,LOCKMODE lockmode,Oid dbid)
{
	Relation	r;
#ifdef FOUNDER_XDB_SE
    IndexType indexType = UNKNOWN_TYPE;
#endif //FOUNDER_XDB_SE	
#ifndef FOUNDER_XDB_SE
	r = relation_open(relationId, lockmode, tableSpace, indexType);
#else
	r = relation_open(relationId, lockmode);
#endif //FOUNDER_XDB_SE
#ifdef FOUNDER_XDB_SE
    if (!r) {
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_TABLE),
                errmsg("file does not exist on disk")));      
    }

#endif //FOUNDER_XDB_SE

	if (r->rel_kind == RELKIND_INDEX)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("\"%s\" is an index",
						RelationGetRelationName(r))));
	else if (r->rel_kind == RELKIND_COMPOSITE_TYPE)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("\"%s\" is a composite type",
						RelationGetRelationName(r))));

	return r;
}
#ifndef FOUNDER_XDB_SE
/* ----------------
 *		heap_openrv - open a heap relation specified
 *		by a RangeVar node
 *
 *		As above, but relation is specified by a RangeVar.
 * ----------------
 */
Relation
heap_openrv(const RangeVar *relation, LOCKMODE lockmode)
{
	Relation	r;

	r = relation_openrv(relation, lockmode);

	if (r->rel_kind == RELKIND_INDEX)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("\"%s\" is an index",
						RelationGetRelationName(r))));
	else if (r->rel_kind == RELKIND_COMPOSITE_TYPE)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("\"%s\" is a composite type",
						RelationGetRelationName(r))));

	return r;
}

/* ----------------
 *		try_heap_openrv - open a heap relation specified
 *		by a RangeVar node
 *
 *		As above, but return NULL instead of failing for relation-not-found.
 * ----------------
 */
Relation
try_heap_openrv(const RangeVar *relation, LOCKMODE lockmode)
{
	Relation	r;

	r = try_relation_openrv(relation, lockmode);

	if (r)
	{
		if (r->rel_kind == RELKIND_INDEX)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("\"%s\" is an index",
							RelationGetRelationName(r))));
		else if (r->rel_kind == RELKIND_COMPOSITE_TYPE)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("\"%s\" is a composite type",
							RelationGetRelationName(r))));
	}

	return r;
}
#endif

/* ----------------
 *		heap_beginscan	- begin relation scan
 *
 * heap_beginscan_strat offers an extended API that lets the caller control
 * whether a nondefault buffer access strategy can be used, and whether
 * syncscan can be chosen (possibly resulting in the scan not starting from
 * block zero).  Both of these default to TRUE with plain heap_beginscan.
 *
 * heap_beginscan_bm is an alternative entry point for setting up a
 * HeapScanDesc for a bitmap heap scan.  Although that scan technology is
 * really quite unlike a standard seqscan, there is just enough commonality
 * to make it worth using the same data structure.
 * ----------------
 */
HeapScanDesc
heap_beginscan(Relation relation, Snapshot snapshot,
			   int nkeys, ScanKey key, filter_func_t filterfunc, void* userdata,BlockNumber start,BlockNumber end)
{
	return heap_beginscan_internal(relation, snapshot, nkeys, key,
								   true, true, false, filterfunc, userdata, start,end);
}

HeapScanDesc
heap_beginscan_strat(Relation relation, Snapshot snapshot,
					 int nkeys, ScanKey key,
					 bool allow_strat, bool allow_sync, filter_func_t filterfunc, void* userdata)
{
	return heap_beginscan_internal(relation, snapshot, nkeys, key,
								   allow_strat, allow_sync, false, filterfunc, userdata);
}

HeapScanDesc
heap_beginscan_bm(Relation relation, Snapshot snapshot,
				  int nkeys, ScanKey key, filter_func_t filterfunc, void* userdata)
{
	return heap_beginscan_internal(relation, snapshot, nkeys, key,
								   false, false, true, filterfunc, userdata);
}

static HeapScanDesc
heap_beginscan_internal(Relation relation, Snapshot snapshot,
						int nkeys, ScanKey key,
						bool allow_strat, bool allow_sync,
						bool is_bitmapscan, filter_func_t filterfunc, void* userdata,BlockNumber start,BlockNumber end)
{
	HeapScanDesc scan;

	/*
	 * increment relation ref count while scanning relation
	 *
	 * This is just to make really sure the relcache entry won't go away while
	 * the scan has a pointer to it.  Caller should be holding the rel open
	 * anyway, so this is redundant in all normal scenarios...
	 */
	RelationIncrementReferenceCount(relation);

	/*
	 * allocate and initialize scan descriptor
	 */
	scan = (HeapScanDesc) palloc0(sizeof(HeapScanDescData));

	scan->rs_rd = relation;
	scan->rs_snapshot = snapshot;
	scan->rs_nkeys = nkeys;
	scan->rs_bitmapscan = is_bitmapscan;
	scan->rs_strategy = NULL;	/* set in initscan */
	scan->rs_allow_strat = allow_strat;
	scan->rs_allow_sync = allow_sync;
	scan->rs_startblock = start;
	scan->rs_endblock = end;
	/*
	 * we can use page-at-a-time mode if it's an MVCC-safe snapshot
	 */
	scan->rs_pageatatime = IsMVCCSnapshot(snapshot);

	/*
	 * For a seqscan in a serializable transaction, acquire a predicate lock
	 * on the entire relation. This is required not only to lock all the
	 * matching tuples, but also to conflict with new insertions into the
	 * table. In an indexscan, we take page locks on the index pages covering
	 * the range specified in the scan qual, but in a heap scan there is
	 * nothing more fine-grained to lock. A bitmap scan is a different story,
	 * there we have already scanned the index and locked the index pages
	 * covering the predicate. But in that case we still have to lock any
	 * matching heap tuples.
	 */
	if (!is_bitmapscan)
		PredicateLockRelation(relation, snapshot);

	/* we only need to set this up once */
	scan->rs_ctup.t_tableOid = RelationGetRelid(relation);

	/*
	 * we do this here instead of in initscan() because heap_rescan also calls
	 * initscan() and we don't want to allocate memory again
	 */
	if (nkeys > 0)
		scan->rs_key = (ScanKey) palloc(sizeof(ScanKeyData) * nkeys);
	else
		scan->rs_key = NULL;
    
    scan->filter_func = filterfunc;
    scan->user_data = userdata;
    
	initscan(scan, key, false);

	return scan;
}

/* ----------------
 *		heap_rescan		- restart a relation scan
 * ----------------
 */
void
heap_rescan(HeapScanDesc scan,
			ScanKey key)
{
	/*
	 * unpin scan buffers
	 */
	if (BufferIsValid(scan->rs_cbuf))
		ReleaseBuffer(scan->rs_cbuf);

	/*
	 * reinitialize scan descriptor
	 */
	initscan(scan, key, true);
}

/* ----------------
 *		heap_endscan	- end relation scan
 *
 *		See how to integrate with index scans.
 *		Check handling if reldesc caching.
 * ----------------
 */
void
heap_endscan(HeapScanDesc scan)
{
	/* Note: no locking manipulations needed */

	/*
	 * unpin scan buffers
	 */
	if (BufferIsValid(scan->rs_cbuf))
		ReleaseBuffer(scan->rs_cbuf);

	/*
	 * decrement relation reference count and free scan descriptor storage
	 */
	RelationDecrementReferenceCount(scan->rs_rd);

	if (scan->rs_key)
	{
		freeScanKeys(scan->rs_key,scan->rs_nkeys);
	}
	if (scan->rs_strategy != NULL)
		FreeAccessStrategy(scan->rs_strategy);

	pfree(scan);
}

/* ----------------
 *		heap_getnext	- retrieve next tuple in scan
 *
 *		Fix to work with index relations.
 *		We don't return the buffer anymore, but you can get it from the
 *		returned HeapTuple.
 * ----------------
 */

#ifdef HEAPDEBUGALL
#define HEAPDEBUG_1 \
	elog(DEBUG2, "heap_getnext([%s,nkeys=%d],dir=%d) called", \
		 RelationGetRelationName(scan->rs_rd), scan->rs_nkeys, (int) direction)
#define HEAPDEBUG_2 \
	elog(DEBUG2, "heap_getnext returning EOS")
#define HEAPDEBUG_3 \
	elog(DEBUG2, "heap_getnext returning tuple")
#else
#define HEAPDEBUG_1
#define HEAPDEBUG_2
#define HEAPDEBUG_3
#endif   /* !defined(HEAPDEBUGALL) */


HeapTuple
heap_getnext(HeapScanDesc scan, ScanDirection direction)
{
	/* Note: no locking manipulations needed */

	HEAPDEBUG_1;				/* heap_getnext( info ) */

	if (scan->rs_pageatatime)
		heapgettup_pagemode(scan, direction,
							scan->rs_nkeys, scan->rs_key);
	else
		heapgettup(scan, direction, scan->rs_nkeys, scan->rs_key);

	if (scan->rs_ctup.t_data == NULL)
	{
		HEAPDEBUG_2;			/* heap_getnext returning EOS */
		return NULL;
	}

	/*
	 * if we get here it means we have a new current scan tuple, so point to
	 * the proper return buffer and return the tuple.
	 */
	HEAPDEBUG_3;				/* heap_getnext returning tuple */

	pgstat_count_heap_getnext(scan->rs_rd);

	return &(scan->rs_ctup);
}

/*
 *	heap_fetch		- retrieve tuple with given tid
 *
 * On entry, tuple->t_self is the TID to fetch.  We pin the buffer holding
 * the tuple, fill in the remaining fields of *tuple, and check the tuple
 * against the specified snapshot.
 *
 * If successful (tuple found and passes snapshot time qual), then *userbuf
 * is set to the buffer holding the tuple and TRUE is returned.  The caller
 * must unpin the buffer when done with the tuple.
 *
 * If the tuple is not found (ie, item number references a deleted slot),
 * then tuple->t_data is set to NULL and FALSE is returned.
 *
 * If the tuple is found but fails the time qual check, then FALSE is returned
 * but tuple->t_data is left pointing to the tuple.
 *
 * keep_buf determines what is done with the buffer in the FALSE-result cases.
 * When the caller specifies keep_buf = true, we retain the pin on the buffer
 * and return it in *userbuf (so the caller must eventually unpin it); when
 * keep_buf = false, the pin is released and *userbuf is set to InvalidBuffer.
 *
 * stats_relation is the relation to charge the heap_fetch operation against
 * for statistical purposes.  (This could be the heap rel itself, an
 * associated index, or NULL to not count the fetch at all.)
 *
 * heap_fetch does not follow HOT chains: only the exact TID requested will
 * be fetched.
 *
 * It is somewhat inconsistent that we ereport() on invalid block number but
 * return false on invalid item number.  There are a couple of reasons though.
 * One is that the caller can relatively easily check the block number for
 * validity, but cannot check the item number without reading the page
 * himself.  Another is that when we are following a t_ctid link, we can be
 * reasonably confident that the page number is valid (since VACUUM shouldn't
 * truncate off the destination page without having killed the referencing
 * tuple first), but the item number might well not be good.
 */
bool
heap_fetch(Relation relation,
		   Snapshot snapshot,
		   HeapTuple tuple,
		   Buffer *userbuf,
		   bool keep_buf,
		   Relation stats_relation)
{
	ItemPointer tid = &(tuple->t_self);
	ItemId		lp;
	Buffer		buffer;
	Page		page;
	OffsetNumber offnum;
	bool		valid;

	/*
	 * Fetch and pin the appropriate page of the relation.
	 */
	buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(tid));

	/*
	 * Need share lock on buffer to examine tuple commit status.
	 */
	LockBuffer(buffer, BUFFER_LOCK_SHARE);
	page = BufferGetPage(buffer);

	/*
	 * We'd better check for out-of-range offnum in case of VACUUM since the
	 * TID was obtained.
	 */
	offnum = ItemPointerGetOffsetNumber(tid);
	if (offnum < FirstOffsetNumber || offnum > PageGetMaxOffsetNumber(page))
	{
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		if (keep_buf)
			*userbuf = buffer;
		else
		{
			ReleaseBuffer(buffer);
			*userbuf = InvalidBuffer;
		}
		tuple->t_data = NULL;
		return false;
	}

	/*
	 * get the item line pointer corresponding to the requested tid
	 */
	lp = PageGetItemId(page, offnum);

	/*
	 * Must check for deleted tuple.
	 */
	if (!ItemIdIsNormal(lp))
	{
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
		if (keep_buf)
			*userbuf = buffer;
		else
		{
			ReleaseBuffer(buffer);
			*userbuf = InvalidBuffer;
		}
		tuple->t_data = NULL;
		return false;
	}

	/*
	 * fill in *tuple fields
	 */
	tuple->t_data = (HeapTupleHeader) PageGetItem(page, lp);
	tuple->t_len = ItemIdGetLength(lp);
	tuple->t_tableOid = RelationGetRelid(relation);

	/*
	 * check time qualification of tuple, then release lock
	 */
	valid = HeapTupleSatisfiesVisibility(tuple, snapshot, buffer);

	if (valid)
		PredicateLockTuple(relation, tuple, snapshot);

	CheckForSerializableConflictOut(valid, relation, tuple, buffer, snapshot);

	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

	if (valid)
	{
		/*
		 * All checks passed, so return the tuple as valid. Caller is now
		 * responsible for releasing the buffer.
		 */
		*userbuf = buffer;

		/* Count the successful fetch against appropriate rel, if any */
		if (stats_relation != NULL)
			pgstat_count_heap_fetch(stats_relation);

		return true;
	}

	/* Tuple failed time qual, but maybe caller wants to see it anyway. */
	if (keep_buf)
		*userbuf = buffer;
	else
	{
		ReleaseBuffer(buffer);
		*userbuf = InvalidBuffer;
	}

	return false;
}

/*
 *	heap_hot_search_buffer	- search HOT chain for tuple satisfying snapshot
 *
 * On entry, *tid is the TID of a tuple (either a simple tuple, or the root
 * of a HOT chain), and buffer is the buffer holding this tuple.  We search
 * for the first chain member satisfying the given snapshot.  If one is
 * found, we update *tid to reference that tuple's offset number, and
 * return TRUE.  If no match, return FALSE without modifying *tid.
 *
 * If all_dead is not NULL, we check non-visible tuples to see if they are
 * globally dead; *all_dead is set TRUE if all members of the HOT chain
 * are vacuumable, FALSE if not.
 *
 * Unlike heap_fetch, the caller must already have pin and (at least) share
 * lock on the buffer; it is still pinned/locked at exit.  Also unlike
 * heap_fetch, we do not report any pgstats count; caller may do so if wanted.
 */
bool
heap_hot_search_buffer(ItemPointer tid, Relation relation, Buffer buffer,
					   Snapshot snapshot, bool *all_dead)
{
	Page		dp = (Page) BufferGetPage(buffer);
	TransactionId prev_xmax = InvalidTransactionId;
	OffsetNumber offnum;
	bool		at_chain_start;
	bool		valid;

	if (all_dead)
		*all_dead = true;

	Assert(TransactionIdIsValid(RecentGlobalXmin));

	Assert(ItemPointerGetBlockNumber(tid) == BufferGetBlockNumber(buffer));
	offnum = ItemPointerGetOffsetNumber(tid);
	at_chain_start = true;

	/* Scan through possible multiple members of HOT-chain */
	for (;;)
	{
		ItemId		lp;
		HeapTupleData heapTuple;

		/* check for bogus TID */
		if (offnum < FirstOffsetNumber || offnum > PageGetMaxOffsetNumber(dp))
			break;

		lp = PageGetItemId(dp, offnum);

		/* check for unused, dead, or redirected items */
		if (!ItemIdIsNormal(lp))
		{
			/* We should only see a redirect at start of chain */
			if (ItemIdIsRedirected(lp) && at_chain_start)
			{
				/* Follow the redirect */
				offnum = ItemIdGetRedirect(lp);
				at_chain_start = false;
				continue;
			}
			/* else must be end of chain */
			break;
		}

		heapTuple.t_data = (HeapTupleHeader) PageGetItem(dp, lp);
		heapTuple.t_len = ItemIdGetLength(lp);
		heapTuple.t_tableOid = relation->rd_id;
		heapTuple.t_self = *tid;

		/*
		 * Shouldn't see a HEAP_ONLY tuple at chain start.
		 */
		if (at_chain_start && HeapTupleIsHeapOnly(&heapTuple))
			break;

		/*
		 * The xmin should match the previous xmax value, else chain is
		 * broken.
		 */
		if (TransactionIdIsValid(prev_xmax) &&
			!TransactionIdEquals(prev_xmax,
								 HeapTupleHeaderGetXmin(heapTuple.t_data)))
			break;

		/* If it's visible per the snapshot, we must return it */
		valid = HeapTupleSatisfiesVisibility(&heapTuple, snapshot, buffer);
		CheckForSerializableConflictOut(valid, relation, &heapTuple, buffer,
										snapshot);
		if (valid)
		{
			ItemPointerSetOffsetNumber(tid, offnum);
			PredicateLockTuple(relation, &heapTuple, snapshot);
			if (all_dead)
				*all_dead = false;
			return true;
		}

		/*
		 * If we can't see it, maybe no one else can either.  At caller
		 * request, check whether all chain members are dead to all
		 * transactions.
		 */
		if (all_dead && *all_dead &&
			HeapTupleSatisfiesVacuum(heapTuple.t_data, RecentGlobalXmin,
									 buffer) != HEAPTUPLE_DEAD)
			*all_dead = false;

		/*
		 * Check to see if HOT chain continues past this tuple; if so fetch
		 * the next offnum and loop around.
		 */
		if (HeapTupleIsHotUpdated(&heapTuple))
		{
			Assert(ItemPointerGetBlockNumber(&heapTuple.t_data->t_ctid) ==
				   ItemPointerGetBlockNumber(tid));
			offnum = ItemPointerGetOffsetNumber(&heapTuple.t_data->t_ctid);
			at_chain_start = false;
			prev_xmax = HeapTupleHeaderGetXmax(heapTuple.t_data);
		}
		else
			break;				/* end of chain */
	}

	return false;
}

/*
 *	heap_hot_search		- search HOT chain for tuple satisfying snapshot
 *
 * This has the same API as heap_hot_search_buffer, except that the caller
 * does not provide the buffer containing the page, rather we access it
 * locally.
 */
bool
heap_hot_search(ItemPointer tid, Relation relation, Snapshot snapshot,
				bool *all_dead)
{
	bool		result;
	Buffer		buffer;

	buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(tid));
	LockBuffer(buffer, BUFFER_LOCK_SHARE);
	result = heap_hot_search_buffer(tid, relation, buffer, snapshot, all_dead);
	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
	ReleaseBuffer(buffer);
	return result;
}

/*
 *	heap_get_latest_tid -  get the latest tid of a specified tuple
 *
 * Actually, this gets the latest version that is visible according to
 * the passed snapshot.  You can pass SnapshotDirty to get the very latest,
 * possibly uncommitted version.
 *
 * *tid is both an input and an output parameter: it is updated to
 * show the latest version of the row.	Note that it will not be changed
 * if no version of the row passes the snapshot test.
 */
void
heap_get_latest_tid(Relation relation,
					Snapshot snapshot,
					ItemPointer tid)
{
	BlockNumber blk;
	ItemPointerData ctid;
	TransactionId priorXmax;

	/* this is to avoid Assert failures on bad input */
	if (!ItemPointerIsValid(tid))
		return;

	/*
	 * Since this can be called with user-supplied TID, don't trust the input
	 * too much.  (RelationGetNumberOfBlocks is an expensive check, so we
	 * don't check t_ctid links again this way.  Note that it would not do to
	 * call it just once and save the result, either.)
	 */
	blk = ItemPointerGetBlockNumber(tid);
	if (blk >= RelationGetNumberOfBlocks(relation))
		ereport(ERROR,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg( "block number %u is out of range for relation \"%s\"",
			                 blk, RelationGetRelationName(relation))));

	/*
	 * Loop to chase down t_ctid links.  At top of loop, ctid is the tuple we
	 * need to examine, and *tid is the TID we will return if ctid turns out
	 * to be bogus.
	 *
	 * Note that we will loop until we reach the end of the t_ctid chain.
	 * Depending on the snapshot passed, there might be at most one visible
	 * version of the row, but we don't try to optimize for that.
	 */
	ctid = *tid;
	priorXmax = InvalidTransactionId;	/* cannot check first XMIN */
	for (;;)
	{
		Buffer		buffer;
		Page		page;
		OffsetNumber offnum;
		ItemId		lp;
		HeapTupleData tp;
		bool		valid;

		/*
		 * Read, pin, and lock the page.
		 */
		buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(&ctid));
		LockBuffer(buffer, BUFFER_LOCK_SHARE);
		page = BufferGetPage(buffer);

		/*
		 * Check for bogus item number.  This is not treated as an error
		 * condition because it can happen while following a t_ctid link. We
		 * just assume that the prior tid is OK and return it unchanged.
		 */
		offnum = ItemPointerGetOffsetNumber(&ctid);
		if (offnum < FirstOffsetNumber || offnum > PageGetMaxOffsetNumber(page))
		{
			UnlockReleaseBuffer(buffer);
			break;
		}
		lp = PageGetItemId(page, offnum);
		if (!ItemIdIsNormal(lp))
		{
			UnlockReleaseBuffer(buffer);
			break;
		}

		/* OK to access the tuple */
		tp.t_self = ctid;
		tp.t_data = (HeapTupleHeader) PageGetItem(page, lp);
		tp.t_len = ItemIdGetLength(lp);

		/*
		 * After following a t_ctid link, we might arrive at an unrelated
		 * tuple.  Check for XMIN match.
		 */
		if (TransactionIdIsValid(priorXmax) &&
		  !TransactionIdEquals(priorXmax, HeapTupleHeaderGetXmin(tp.t_data)))
		{
			UnlockReleaseBuffer(buffer);
			break;
		}

		/*
		 * Check time qualification of tuple; if visible, set it as the new
		 * result candidate.
		 */
		valid = HeapTupleSatisfiesVisibility(&tp, snapshot, buffer);
		CheckForSerializableConflictOut(valid, relation, &tp, buffer, snapshot);
		if (valid)
			*tid = ctid;

		/*
		 * If there's a valid t_ctid link, follow it, else we're done.
		 */
		if ((tp.t_data->t_infomask & (HEAP_XMAX_INVALID | HEAP_IS_LOCKED)) ||
			ItemPointerEquals(&tp.t_self, &tp.t_data->t_ctid))
		{
			UnlockReleaseBuffer(buffer);
			break;
		}

		ctid = tp.t_data->t_ctid;
		priorXmax = HeapTupleHeaderGetXmax(tp.t_data);
		UnlockReleaseBuffer(buffer);
	}							/* end of loop */
}


/*
 * UpdateXmaxHintBits - update tuple hint bits after xmax transaction ends
 *
 * This is called after we have waited for the XMAX transaction to terminate.
 * If the transaction aborted, we guarantee the XMAX_INVALID hint bit will
 * be set on exit.	If the transaction committed, we set the XMAX_COMMITTED
 * hint bit if possible --- but beware that that may not yet be possible,
 * if the transaction committed asynchronously.  Hence callers should look
 * only at XMAX_INVALID.
 */
static void
UpdateXmaxHintBits(HeapTupleHeader tuple, Buffer buffer, TransactionId xid)
{
	Assert(TransactionIdEquals(HeapTupleHeaderGetXmax(tuple), xid));

	if (!(tuple->t_infomask & (HEAP_XMAX_COMMITTED | HEAP_XMAX_INVALID)))
	{
		if (TransactionIdDidCommit(xid))
			HeapTupleSetHintBits(tuple, buffer, HEAP_XMAX_COMMITTED,
								 xid);
		else
			HeapTupleSetHintBits(tuple, buffer, HEAP_XMAX_INVALID,
								 InvalidTransactionId);
	}
}


/*
 * GetBulkInsertState - prepare status object for a bulk insert
 */
BulkInsertState
GetBulkInsertState(void)
{
	BulkInsertState bistate;

	bistate = (BulkInsertState) palloc(sizeof(BulkInsertStateData));
	bistate->strategy = GetAccessStrategy(BAS_BULKWRITE);
	bistate->current_buf = InvalidBuffer;
	return bistate;
}

/*
 * FreeBulkInsertState - clean up after finishing a bulk insert
 */
void
FreeBulkInsertState(BulkInsertState bistate)
{
	if (bistate->current_buf != InvalidBuffer)
		ReleaseBuffer(bistate->current_buf);
	FreeAccessStrategy(bistate->strategy);
	pfree(bistate);
}


/*
 *	heap_insert		- insert tuple into a heap
 *
 * The new tuple is stamped with current transaction ID and the specified
 * command ID.
 *
 * If the HEAP_INSERT_SKIP_WAL option is specified, the new tuple is not
 * logged in WAL, even for a non-temp relation.  Safe usage of this behavior
 * requires that we arrange that all new tuples go into new pages not
 * containing any tuples from other transactions, and that the relation gets
 * fsync'd before commit.  (See also heap_sync() comments)
 *
 * The HEAP_INSERT_SKIP_FSM option is passed directly to
 * RelationGetBufferForTuple, which see for more info.
 *
 * Note that these options will be applied when inserting into the heap's
 * TOAST table, too, if the tuple requires any out-of-line data.
 *
 * The BulkInsertState object (if any; bistate can be NULL for default
 * behavior) is also just passed through to RelationGetBufferForTuple.
 *
 * The return value is the OID assigned to the tuple (either here or by the
 * caller), or InvalidOid if no OID.  The header fields of *tup are updated
 * to match the stored tuple; in particular tup->t_self receives the actual
 * TID where the tuple was stored.	But note that any toasting of fields
 * within the tuple data is NOT reflected into *tup.
 */
Oid
heap_insert(Relation relation, HeapTuple tup, CommandId cid,
			int options, BulkInsertState bistate)
{
#ifndef FOUNDER_XDB_SE
	TransactionId xid = GetCurrentTransactionId();
#else
	TransactionId xid = InvalidTransactionId;
	if(RelationIsTemp(relation))
	{
		xid = Fxdb_GetTmpCurrentTransactionId();
	} else
	{
		CheckStandbyUpdateError("Can not execute insert during recovery.");

		xid = GetCurrentTransactionId();
	}
#endif //FOUNDER_XDB_SE
	HeapTuple	heaptup;
	Buffer		buffer;
	bool		all_visible_cleared = false;

	if (relation->rd_rel->relhasoids)
	{
#ifdef NOT_USED
		/* this is redundant with an Assert in HeapTupleSetOid */
		Assert(tup->t_data->t_infomask & HEAP_HASOID);
#endif

		/*
		 * If the object id of this tuple has already been assigned, trust the
		 * caller.	There are a couple of ways this can happen.  At initial db
		 * creation, the backend program sets oids for tuples. When we define
		 * an index, we set the oid.  Finally, in the future, we may allow
		 * users to set their own object ids in order to support a persistent
		 * object store (objects need to contain pointers to one another).
		 */
		if (!OidIsValid(HeapTupleGetOid(tup)))
			HeapTupleSetOid(tup, GetNewOid(relation));
	}
	else
	{
		/* check there is not space for an OID */
		Assert(!(tup->t_data->t_infomask & HEAP_HASOID));
	}

	tup->t_data->t_infomask &= ~(HEAP_XACT_MASK);
	tup->t_data->t_infomask2 &= ~(HEAP2_XACT_MASK);
	tup->t_data->t_infomask |= HEAP_XMAX_INVALID;
	HeapTupleHeaderSetXmin(tup->t_data, xid);
	HeapTupleHeaderSetCmin(tup->t_data, cid);
	HeapTupleHeaderSetXmax(tup->t_data, 0);		/* for cleanliness */
	tup->t_tableOid = RelationGetRelid(relation);

	/*
	 * If the new tuple is too big for storage or contains already toasted
	 * out-of-line attributes from some other relation, invoke the toaster.
	 *
	 * Note: below this point, heaptup is the data we actually intend to store
	 * into the relation; tup is the caller's original untoasted data.
	 */
	if (relation->rel_kind != RELKIND_RELATION)
	{
		/* toast table entries should never be recursively toasted */
		Assert(!HeapTupleHasExternal(tup));
		heaptup = tup;
	}
	else if (HeapTupleHasExternal(tup) || tup->t_len > TOAST_TUPLE_THRESHOLD)
		heaptup = toast_insert_or_update(relation, tup, NULL, options);
	else
		heaptup = tup;

	/*
	 * We're about to do the actual insert -- but check for conflict first,
	 * to avoid possibly having to roll back work we've just done.
	 *
	 * For a heap insert, we only need to check for table-level SSI locks.
	 * Our new tuple can't possibly conflict with existing tuple locks, and
	 * heap page locks are only consolidated versions of tuple locks; they do
	 * not lock "gaps" as index page locks do.  So we don't need to identify
	 * a buffer before making the call.
	 */
	CheckForSerializableConflictIn(relation, NULL, InvalidBuffer);

	/* Find buffer to insert this tuple into */
	buffer = RelationGetBufferForTuple(relation, heaptup->t_len,
									   InvalidBuffer, options, bistate);

	/* NO EREPORT(ERROR) from here till changes are logged */
	START_CRIT_SECTION();

	RelationPutHeapTuple(relation, buffer, heaptup);

	if (PageIsAllVisible(BufferGetPage(buffer)))
	{
		all_visible_cleared = true;
		PageClearAllVisible(BufferGetPage(buffer));
	}

	/*
	 * XXX Should we set PageSetPrunable on this page ?
	 *
	 * The inserting transaction may eventually abort thus making this tuple
	 * DEAD and hence available for pruning. Though we don't want to optimize
	 * for aborts, if no other tuple in this page is UPDATEd/DELETEd, the
	 * aborted tuple will never be pruned until next vacuum is triggered.
	 *
	 * If you do add PageSetPrunable here, add it in heap_xlog_insert too.
	 */

	MarkBufferDirty(buffer);

	/* XLOG stuff */
	if (!(options & HEAP_INSERT_SKIP_WAL) && RelationNeedsWAL(relation))
	{
		xl_heap_insert xlrec;
		xl_heap_header xlhdr;
		XLogRecPtr	recptr;
		XLogRecData rdata[3];
		Page		page = BufferGetPage(buffer);
		uint8		info = XLOG_HEAP_INSERT;

		xlrec.all_visible_cleared = all_visible_cleared;
		xlrec.target.node = relation->rd_node;
		xlrec.target.tid = heaptup->t_self;
		rdata[0].data = (char *) &xlrec;
		rdata[0].len = SizeOfHeapInsert;
		rdata[0].buffer = InvalidBuffer;
		rdata[0].next = &(rdata[1]);

		xlhdr.t_infomask2 = heaptup->t_data->t_infomask2;
		xlhdr.t_infomask = heaptup->t_data->t_infomask;
		xlhdr.t_hoff = heaptup->t_data->t_hoff;

		/*
		 * note we mark rdata[1] as belonging to buffer; if XLogInsert decides
		 * to write the whole page to the xlog, we don't need to store
		 * xl_heap_header in the xlog.
		 */
		rdata[1].data = (char *) &xlhdr;
		rdata[1].len = SizeOfHeapHeader;
		rdata[1].buffer = buffer;
		rdata[1].buffer_std = true;
		rdata[1].next = &(rdata[2]);

		/* PG73FORMAT: write bitmap [+ padding] [+ oid] + data */
		rdata[2].data = (char *) heaptup->t_data + offsetof(HeapTupleHeaderData, t_bits);
		rdata[2].len = heaptup->t_len - offsetof(HeapTupleHeaderData, t_bits);
		rdata[2].buffer = buffer;
		rdata[2].buffer_std = true;
		rdata[2].next = NULL;

		/*
		 * If this is the single and first tuple on page, we can reinit the
		 * page instead of restoring the whole thing.  Set flag, and hide
		 * buffer references from XLogInsert.
		 */
		if (ItemPointerGetOffsetNumber(&(heaptup->t_self)) == FirstOffsetNumber &&
			PageGetMaxOffsetNumber(page) == FirstOffsetNumber)
		{
			info |= XLOG_HEAP_INIT_PAGE;
			rdata[1].buffer = rdata[2].buffer = InvalidBuffer;
		}

		recptr = XLogInsert(RM_HEAP_ID, info, rdata);

		PageSetLSN(page, recptr);
		PageSetTLI(page, ThisTimeLineID);
	}

	END_CRIT_SECTION();

	UnlockReleaseBuffer(buffer);

	/* Clear the bit in the visibility map if necessary */
	if (all_visible_cleared)
		visibilitymap_clear(relation,
							ItemPointerGetBlockNumber(&(heaptup->t_self)));

	/*
	 * If tuple is cachable, mark it for invalidation from the caches in case
	 * we abort.  Note it is OK to do this after releasing the buffer, because
	 * the heaptup data structure is all in local memory, not in the shared
	 * buffer.
	 */
	CacheInvalidateHeapTuple(relation, heaptup);

	pgstat_count_heap_insert(relation);

	/*
	 * If heaptup is a private copy, release it.  Don't forget to copy t_self
	 * back to the caller's image, too.
	 */
	if (heaptup != tup)
	{
		tup->t_self = heaptup->t_self;
		heap_freetuple(heaptup);
	}

	return HeapTupleGetOid(tup);
}

void insert_tuple_to_index(Relation heapRel, Relation indexRel, HeapTuple tup)
{
	IndexType indexType = (IndexType)indexRel->mt_info.type;
	bool isnull = false;
	Datum values = fdxdb_form_index_datum(heapRel, indexRel, tup);

	if (values != 0 && values != (Datum)-1) {// skip if no index key needed for this tuple    
	    index_insert(indexRel,&values,&isnull,&(tup->t_self),heapRel
		    ,(indexType >= UINQUE_TYPE_BASE) ? UNIQUE_CHECK_YES : UNIQUE_CHECK_NO);
		if (DatumGetPointer(values) != NULL)
			pfree(DatumGetPointer(values));
    } else if (values == (Datum)-1) {// multiple keys per heaptuple.
        size_t nkeys = 0;
        Datum *pvalues = combineIndexKeys(heapRel, indexRel, tup, &nkeys);
        for (size_t i = 0; i < nkeys; i++) {
            index_insert(indexRel,&pvalues[i],&isnull,&(tup->t_self),heapRel
		        ,(indexType >= UINQUE_TYPE_BASE) ? UNIQUE_CHECK_YES : UNIQUE_CHECK_NO);
		    pfree(DatumGetPointer(pvalues[i]));
        }
        pfree(pvalues);
    }
}

Datum *get_index_keys(Relation heapRel, Relation indexRel, HeapTuple tup, size_t &nIndexKeys)
{
	Datum *pvalues = NULL;

	nIndexKeys = 0;

	Datum value = fdxdb_form_index_datum(heapRel, indexRel, tup);
	if(value == (Datum)-1)
	{
		pvalues = combineIndexKeys(heapRel, indexRel, tup, &nIndexKeys);
	}
	else if(value != (Datum)0)
	{
		nIndexKeys = 1;
		pvalues = (Datum *)palloc0(sizeof(Datum));
		pvalues[0] = value;
	}

	return pvalues;
}

Datum get_index_key(Relation heapRel, Relation indexRel, HeapTuple tup, bool min)
{
	Datum value = 0;

	value = fdxdb_form_index_datum(heapRel, indexRel, tup);
	if(value == (Datum)-1)
	{
		size_t nkeys = 0;
		Datum *pvalues = combineIndexKeys(heapRel, indexRel, tup, &nkeys);

		/* sort datum */
		Tuplesortstate* pState;
		int workMem  = MaxAllocSize/1024;//need external sorting when memory used greater than this value
		pState = tuplesort_begin_index_datum(workMem, true, indexRel);
		for (size_t i = 0; i < nkeys; i++) {
			tuplesort_putdatum(pState, pvalues[i], &tup->t_self);
			//pfree(DatumGetPointer(pvalues[i]));
		}
		tuplesort_performsort(pState);

		/* get first datum */
		ItemPointerData tid;
		if(!tuplesort_getdatum(pState, min, &value, &tid))
		{
			value = 0;
		}
		tuplesort_end(pState);

		/* free other datum */
		for (size_t i = 0; i < nkeys; i++) {
			if(pvalues[i] != value)
			{
				pfree(DatumGetPointer(pvalues[i]));
			}
		}
		pfree(pvalues);
	}

	return value;
}

extern Oid LsmGetActiveIdxId(Relation lsmIdxRel);
void insert_tuple_to_index_by_oid(Relation relation, HeapTuple tup, Oid indexId)
{
	Relation indexRel = index_open(indexId, RowExclusiveLock);
	Relation insertIdxRel = indexRel;
	PG_TRY(); {
		if(indexRel->mt_info.type == LSM_TYPE)
		{
			// TODO: get activeIdxId
			Oid activeIdxId = LsmGetActiveIdxId(indexRel);
			if(insertIdxRel->lastLsmActiveIdx != activeIdxId)
			{
				insertIdxRel->lastLsmActiveIdx = activeIdxId;
				AcceptInvalidationMessages();			
			}
			if(activeIdxId != indexId)
			{
				insertIdxRel = index_open(activeIdxId, RowExclusiveLock);
			}
		}
			insert_tuple_to_index(relation, insertIdxRel, tup);

		if(insertIdxRel != indexRel)
		{
			index_close(insertIdxRel, RowExclusiveLock);
		}
		index_close(indexRel, RowExclusiveLock);
	} PG_CATCH(); { 
		if(insertIdxRel != indexRel)
		{
			index_close(insertIdxRel, RowExclusiveLock);
		}
		index_close(indexRel, RowExclusiveLock);
		PG_RE_THROW();
	} PG_END_TRY(); 
}
void insert_tuple_to_indexes(Relation relation, HeapTuple tup)
{
	//if(MtInfo_GetIndexCount(relation->mt_info) <= 0)
	//{
	//	fxdb_get_mete_info(relation->rd_node.relNode, &relation->mt_info);
	//}
	if(tup->t_nIndexes != 0)
	{
		Assert(tup->t_indexOids != NULL);
		for(uint32 i = 0; i < tup->t_nIndexes; i++)
		{
			insert_tuple_to_index_by_oid(relation, tup, tup->t_indexOids[i]);
		}
	}
	else
	{
		Assert(tup->t_indexOids == NULL);
		for (unsigned int i = 0; i < MtInfo_GetIndexCount(relation->mt_info); ++i) 
		{
			Oid indexId = MtInfo_GetIndexOid(relation->mt_info)[i];
			insert_tuple_to_index_by_oid(relation, tup, indexId);
		}
	}
}

bool is_tuple_need_insert_index(HeapTuple tup, Oid indexId)
{
	if(tup->t_nIndexes == 0)
	{
		Assert(tup->t_indexOids == NULL);
		return TRUE;
	}
	else
	{
		Assert(tup->t_indexOids != NULL);
		for(uint32 i = 0; i < tup->t_nIndexes; i++)
		{
			if(tup->t_indexOids[i] == indexId)
			{
				return TRUE;
			}
		}
		return FALSE;
	}
}

void index_multi_insert(Relation relation,HeapTuple* tup,int tup_count)
{
	//if(MtInfo_GetIndexCount(relation->mt_info) <= 0)
	//{
	//	fxdb_get_mete_info(relation->rd_node.relNode, &relation->mt_info);
	//}
	
	for (unsigned int i = 0; i < MtInfo_GetIndexCount(relation->mt_info); ++i) 
	{
		IndexType indexType = (IndexType)MtInfo_GetIndexType(relation->mt_info)[i];
		Oid index_id = MtInfo_GetIndexOid(relation->mt_info)[i];
		Relation index = index_open(index_id,AccessShareLock);

		Tuplesortstate* pState;
		int workMem  = MaxAllocSize/1024;//need external sorting when memory used greater than this value
		pState = tuplesort_begin_index_datum(workMem,true,index);
		
		int nTotal = 0;
		for (int nTup = 0; nTup < tup_count; ++nTup)
		{
			HeapTuple tuple = tup[nTup];
			if(is_tuple_need_insert_index(tuple, index_id))
			{
				Datum values = fdxdb_form_index_datum(relation, index, tuple);
				if (values != 0 && values != (Datum)-1)
				{// skip if no index key needed for this tuple    
					tuplesort_putdatum(pState,values,&tuple->t_self);
				}
				else if (values == (Datum)-1)
				{// multiple keys per heaptuple.
					size_t nkeys = 0;
					Datum *pvalues = combineIndexKeys(relation, index, tuple, &nkeys);
					if(pvalues && nkeys != 0)
					{
						for (size_t i = 0; i < nkeys; i++)
						{
							tuplesort_putdatum(pState,pvalues[i],&tuple->t_self);
						}
						pfree(pvalues);
					}
				}
			}
		}
		
		tuplesort_performsort(pState);

		int nTraverse = 0;
		Datum dat_value;
		ItemPointerData tid;
		char* pre = NULL;
		while (tuplesort_getdatum(pState,true,&dat_value,&tid))
		{			
			bool bNull = false;
			index_insert(index,&dat_value,&bNull,&tid,relation
				,(indexType >= UINQUE_TYPE_BASE) ? UNIQUE_CHECK_YES : UNIQUE_CHECK_NO);
			if (DatumGetPointer(dat_value) != NULL)
					pfree(DatumGetPointer(dat_value));
		}
		
		tuplesort_end(pState);
		
		index_close(index,NoLock);
	}
}
/*
 *	simple_heap_insert - insert a tuple
 *
 * Currently, this routine differs from heap_insert only in supplying
 * a default command ID and not allowing access to the speedup options.
 *
 * This should be used rather than using heap_insert directly in most places
 * where we are modifying system catalogs.
 */
Oid
simple_heap_insert(Relation relation, HeapTuple tup)
{
	Oid id = heap_insert(relation, tup, GetCurrentCommandId(true), 0, NULL);
	insert_tuple_to_indexes(relation,tup);
	return id;
}
#ifdef FOUNDER_XDB_SE
Datum fdxdb_form_index_datum(Relation heapRelation, Relation indexRelation, HeapTuple tup)
{
	Datum		values[1];
	bool		isnull[1], isMultiKey = false;
	isnull[0] = false;
    heap_deform_tuple(tup, single_attibute_tupDesc, values, isnull);
	text       *tunpacked = pg_detoast_datum_packed((struct varlena *)DatumGetPointer(values[0]));
	char *pstr0 = VARDATA_ANY(tunpacked);
    size_t len0 = VARSIZE_ANY_EXHDR(tunpacked);

    char* retValue = (char*)palloc(len0 + VARHDRSZ);
    char *pstr = VARDATA(retValue);
    SET_VARSIZE(retValue,len0 + VARHDRSZ);
	size_t len1 = 0;
	for (size_t i = 0; i < indexRelation->rd_colinfo->keys; i++)
	{
		size_t			keycol = indexRelation->rd_colinfo->col_number[i];
		Datum		iDatum2 = (Datum) 0;

		if (keycol != 0)
		{
            RangeData range = {0};
			range.userData = indexRelation->user_data;// pass the index's info rather than the heap's.
			void *storedUserData = MtInfo_GetUserData(indexRelation->mt_info);
			if(storedUserData != NULL)
			{
				Assert(range.userData == NULL);
				range.userData = storedUserData;
			}
            heapRelation->rd_colinfo->split_function(range, pstr0, (int)keycol, len0);
			if (range.nItems > 0) {
			    isMultiKey = true;
			    releaseRangeDataMemory(&range);
			    break;
            }
			size_t len2 = range.len;
			if (len2 == (size_t)-1) {
				pfree(retValue);

				if ((char *)tunpacked != DatumGetPointer(values[0])) {
					pfree(tunpacked);
				}
				return 0;// don't insert index key for this heap tuple.
			}
			Assert(len1 + len2 <= len0);
			memcpy(&pstr[len1], &pstr0[range.start], len2);
			len1 += len2;

		}
	}

	if ((char *)tunpacked != DatumGetPointer(values[0])) {
		pfree(tunpacked);
	}

	// multikey is superior to no key below.
    if (isMultiKey)
	{
		pfree(retValue);
        return -1;
	}
        
	// if this tuple needs no key, return 0 and no index entry will point to this heap tuple.
    if (len1 == 0)
	{
		pfree(retValue);
        return 0;
	}

	SET_VARSIZE(retValue, len1 + VARHDRSZ);
	
	return PointerGetDatum(retValue);
	
}


#endif //FOUNDER_XDB_SE

/*
 *	heap_delete - delete a tuple
 *
 * NB: do not call this directly unless you are prepared to deal with
 * concurrent-update conditions.  Use simple_heap_delete instead.
 *
 *	relation - table to be modified (caller must hold suitable lock)
 *	tid - TID of tuple to be deleted
 *	ctid - output parameter, used only for failure case (see below)
 *	update_xmax - output parameter, used only for failure case (see below)
 *	cid - delete command ID (used for visibility test, and stored into
 *		cmax if successful)
 *	crosscheck - if not InvalidSnapshot, also check tuple against this
 *	wait - true if should wait for any conflicting update to commit/abort
 *
 * Normal, successful return value is HeapTupleMayBeUpdated, which
 * actually means we did delete it.  Failure return codes are
 * HeapTupleSelfUpdated, HeapTupleUpdated, or HeapTupleBeingUpdated
 * (the last only possible if wait == false).
 *
 * In the failure cases, the routine returns the tuple's t_ctid and t_xmax.
 * If t_ctid is the same as tid, the tuple was deleted; if different, the
 * tuple was updated, and t_ctid is the location of the replacement tuple.
 * (t_xmax is needed to verify that the replacement tuple matches.)
 */
HTSU_Result
heap_delete(Relation relation, ItemPointer tid,
			ItemPointer ctid, TransactionId *update_xmax,
			CommandId cid, Snapshot crosscheck, bool wait)
{
	HTSU_Result result;
#ifndef FOUNDER_XDB_SE
	TransactionId xid = GetCurrentTransactionId();
#else
	TransactionId xid = InvalidTransactionId;
	if(RelationIsTemp(relation))
	{
		xid = Fxdb_GetTmpCurrentTransactionId();
	} else
	{
		CheckStandbyUpdateError("Can not execute delete during recovery.");

		xid = GetCurrentTransactionId();
	}
#endif //FOUNDER_XDB_SE
	ItemId		lp;
	HeapTupleData tp;
	Page		page;
	Buffer		buffer;
	bool		have_tuple_lock = false;
	bool		iscombo;
	bool		all_visible_cleared = false;

	Assert(ItemPointerIsValid(tid));

	buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(tid));
	LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

	page = BufferGetPage(buffer);
	lp = PageGetItemId(page, ItemPointerGetOffsetNumber(tid));
	Assert(ItemIdIsNormal(lp));

	tp.t_data = (HeapTupleHeader) PageGetItem(page, lp);
	tp.t_len = ItemIdGetLength(lp);
	tp.t_self = *tid;

l1:
	result = HeapTupleSatisfiesUpdate(tp.t_data, cid, buffer);

	if (result == HeapTupleInvisible)
	{
		UnlockReleaseBuffer(buffer);
		ereport(ERROR,
		        (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
		        errmsg("attempted to delete invisible tuple")));
	}
	else if (result == HeapTupleBeingUpdated && wait)
	{
		TransactionId xwait;
		uint16		infomask;

		/* must copy state data before unlocking buffer */
		xwait = HeapTupleHeaderGetXmax(tp.t_data);
		infomask = tp.t_data->t_infomask;

		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

		/*
		 * Acquire tuple lock to establish our priority for the tuple (see
		 * heap_lock_tuple).  LockTuple will release us when we are
		 * next-in-line for the tuple.
		 *
		 * If we are forced to "start over" below, we keep the tuple lock;
		 * this arranges that we stay at the head of the line while rechecking
		 * tuple state.
		 */
		if (!have_tuple_lock)
		{
			LockTuple(relation, &(tp.t_self), ExclusiveLock);
			have_tuple_lock = true;
		}

		/*
		 * Sleep until concurrent transaction ends.  Note that we don't care
		 * if the locker has an exclusive or shared lock, because we need
		 * exclusive.
		 */

		if (infomask & HEAP_XMAX_IS_MULTI)
		{
			/* wait for multixact */
			MultiXactIdWait((MultiXactId) xwait);
			LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

			/*
			 * If xwait had just locked the tuple then some other xact could
			 * update this tuple before we get to this point.  Check for xmax
			 * change, and start over if so.
			 */
			if (!(tp.t_data->t_infomask & HEAP_XMAX_IS_MULTI) ||
				!TransactionIdEquals(HeapTupleHeaderGetXmax(tp.t_data),
									 xwait))
				goto l1;

			/*
			 * You might think the multixact is necessarily done here, but not
			 * so: it could have surviving members, namely our own xact or
			 * other subxacts of this backend.	It is legal for us to delete
			 * the tuple in either case, however (the latter case is
			 * essentially a situation of upgrading our former shared lock to
			 * exclusive).	We don't bother changing the on-disk hint bits
			 * since we are about to overwrite the xmax altogether.
			 */
		}
		else
		{
			/* wait for regular transaction to end */
			XactLockTableWait(xwait);
			LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

			/*
			 * xwait is done, but if xwait had just locked the tuple then some
			 * other xact could update this tuple before we get to this point.
			 * Check for xmax change, and start over if so.
			 */
			if ((tp.t_data->t_infomask & HEAP_XMAX_IS_MULTI) ||
				!TransactionIdEquals(HeapTupleHeaderGetXmax(tp.t_data),
									 xwait))
				goto l1;

			/* Otherwise check if it committed or aborted */
			UpdateXmaxHintBits(tp.t_data, buffer, xwait);
		}

		/*
		 * We may overwrite if previous xmax aborted, or if it committed but
		 * only locked the tuple without updating it.
		 */
		if (tp.t_data->t_infomask & (HEAP_XMAX_INVALID |
									 HEAP_IS_LOCKED))
			result = HeapTupleMayBeUpdated;
		else
			result = HeapTupleUpdated;
	}

	if (crosscheck != InvalidSnapshot && result == HeapTupleMayBeUpdated)
	{
		/* Perform additional check for transaction-snapshot mode RI updates */
		if (!HeapTupleSatisfiesVisibility(&tp, crosscheck, buffer))
			result = HeapTupleUpdated;
	}

	if (result != HeapTupleMayBeUpdated)
	{
		Assert(result == HeapTupleSelfUpdated ||
			   result == HeapTupleUpdated ||
			   result == HeapTupleBeingUpdated);
		Assert(!(tp.t_data->t_infomask & HEAP_XMAX_INVALID));
		*ctid = tp.t_data->t_ctid;
		*update_xmax = HeapTupleHeaderGetXmax(tp.t_data);
		UnlockReleaseBuffer(buffer);
		if (have_tuple_lock)
			UnlockTuple(relation, &(tp.t_self), ExclusiveLock);
		return result;
	}

	/*
	 * We're about to do the actual delete -- check for conflict first, to
	 * avoid possibly having to roll back work we've just done.
	 */
	CheckForSerializableConflictIn(relation, &tp, buffer);

	/* replace cid with a combo cid if necessary */
	HeapTupleHeaderAdjustCmax(tp.t_data, &cid, &iscombo);

	START_CRIT_SECTION();

	/*
	 * If this transaction commits, the tuple will become DEAD sooner or
	 * later.  Set flag that this page is a candidate for pruning once our xid
	 * falls below the OldestXmin horizon.	If the transaction finally aborts,
	 * the subsequent page pruning will be a no-op and the hint will be
	 * cleared.
	 */
	PageSetPrunable(page, xid);

	if (PageIsAllVisible(page))
	{
		all_visible_cleared = true;
		PageClearAllVisible(page);
	}

	/* store transaction information of xact deleting the tuple */
	tp.t_data->t_infomask &= ~(HEAP_XMAX_COMMITTED |
							   HEAP_XMAX_INVALID |
							   HEAP_XMAX_IS_MULTI |
							   HEAP_IS_LOCKED |
							   HEAP_MOVED);
	HeapTupleHeaderClearHotUpdated(tp.t_data);
	HeapTupleHeaderSetXmax(tp.t_data, xid);
	HeapTupleHeaderSetCmax(tp.t_data, cid, iscombo);
	/* Make sure there is no forward chain link in t_ctid */
	tp.t_data->t_ctid = tp.t_self;

	MarkBufferDirty(buffer);

	/* XLOG stuff */
	if (RelationNeedsWAL(relation))
	{
		xl_heap_delete xlrec;
		XLogRecPtr	recptr;
		XLogRecData rdata[2];

		xlrec.all_visible_cleared = all_visible_cleared;
		xlrec.target.node = relation->rd_node;
		xlrec.target.tid = tp.t_self;
		rdata[0].data = (char *) &xlrec;
		rdata[0].len = SizeOfHeapDelete;
		rdata[0].buffer = InvalidBuffer;
		rdata[0].next = &(rdata[1]);

		rdata[1].data = NULL;
		rdata[1].len = 0;
		rdata[1].buffer = buffer;
		rdata[1].buffer_std = true;
		rdata[1].next = NULL;

		recptr = XLogInsert(RM_HEAP_ID, XLOG_HEAP_DELETE, rdata);

		PageSetLSN(page, recptr);
		PageSetTLI(page, ThisTimeLineID);
	}

	END_CRIT_SECTION();

	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

	/*
	 * If the tuple has toasted out-of-line attributes, we need to delete
	 * those items too.  We have to do this before releasing the buffer
	 * because we need to look at the contents of the tuple, but it's OK to
	 * release the content lock on the buffer first.
	 */
	if (relation->rel_kind != RELKIND_RELATION)
	{
		/* toast table entries should never be recursively toasted */
		Assert(!HeapTupleHasExternal(&tp));
	}
	else if (HeapTupleHasExternal(&tp))
		toast_delete(relation, &tp);

	/*
	 * Mark tuple for invalidation from system caches at next command
	 * boundary. We have to do this before releasing the buffer because we
	 * need to look at the contents of the tuple.
	 */
	CacheInvalidateHeapTuple(relation, &tp);

	/* Clear the bit in the visibility map if necessary */
	if (all_visible_cleared)
		visibilitymap_clear(relation, BufferGetBlockNumber(buffer));

	/* Now we can release the buffer */
	ReleaseBuffer(buffer);

	/*
	 * Release the lmgr tuple lock, if we had it.
	 */
	if (have_tuple_lock)
		UnlockTuple(relation, &(tp.t_self), ExclusiveLock);

	pgstat_count_heap_delete(relation);

	return HeapTupleMayBeUpdated;
}

/*
 *	simple_heap_delete - delete a tuple
 *
 * This routine may be used to delete a tuple when concurrent updates of
 * the target tuple are not expected (for example, because we have a lock
 * on the relation associated with the tuple).	Any failure is reported
 * via ereport().
 */
#ifdef FOUNDER_XDB_SE
void
simple_heap_delete(Relation relation, ItemPointer tid)
#else
void
simple_heap_delete(Relation relation, ItemPointer tid)
#endif
{
	HTSU_Result result;
	ItemPointerData update_ctid;
	TransactionId update_xmax;
#ifdef FOUNDER_XDB_SE
	result = heap_delete(relation, tid,
				&update_ctid, &update_xmax,
				GetCurrentCommandId(true), InvalidSnapshot,
				true /* wait for commit */ );
		
		
#else
	result = heap_delete(relation, tid,
						 &update_ctid, &update_xmax,
						 GetCurrentCommandId(true), InvalidSnapshot,
						 true /* wait for commit */ );
#endif

	switch (result)
	{
		case HeapTupleSelfUpdated:
			/* Tuple was already updated in current command? */
			ereport(ERROR,
			        (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
			        errmsg("tuple already updated by self")));
			break;

		case HeapTupleMayBeUpdated:
			/* done successfully */
			break;

		case HeapTupleUpdated:
			ereport(ERROR,
                    (errcode(ERRCODE_TUPLE_UPDATE_CONCURRENTLY),
                    errmsg("tuple concurrently updated")));
			break;

		default:
			ereport(ERROR,
                    (errcode(ERRCODE_UNDEFINED_PARAMETER),
                    errmsg("unrecognized heap_delete status: %u", result)));
			break;
	}
}

/*
 *	heap_update - replace a tuple
 *
 * NB: do not call this directly unless you are prepared to deal with
 * concurrent-update conditions.  Use simple_heap_update instead.
 *
 *	relation - table to be modified (caller must hold suitable lock)
 *	otid - TID of old tuple to be replaced
 *	newtup - newly constructed tuple data to store
 *	ctid - output parameter, used only for failure case (see below)
 *	update_xmax - output parameter, used only for failure case (see below)
 *	cid - update command ID (used for visibility test, and stored into
 *		cmax/cmin if successful)
 *	crosscheck - if not InvalidSnapshot, also check old tuple against this
 *	wait - true if should wait for any conflicting update to commit/abort
 *
 * Normal, successful return value is HeapTupleMayBeUpdated, which
 * actually means we *did* update it.  Failure return codes are
 * HeapTupleSelfUpdated, HeapTupleUpdated, or HeapTupleBeingUpdated
 * (the last only possible if wait == false).
 *
 * On success, the header fields of *newtup are updated to match the new
 * stored tuple; in particular, newtup->t_self is set to the TID where the
 * new tuple was inserted, and its HEAP_ONLY_TUPLE flag is set iff a HOT
 * update was done.  However, any TOAST changes in the new tuple's
 * data are not reflected into *newtup.
 *
 * In the failure cases, the routine returns the tuple's t_ctid and t_xmax.
 * If t_ctid is the same as otid, the tuple was deleted; if different, the
 * tuple was updated, and t_ctid is the location of the replacement tuple.
 * (t_xmax is needed to verify that the replacement tuple matches.)
 */
HTSU_Result
heap_update(Relation relation, ItemPointer otid, HeapTuple newtup,
			ItemPointer ctid, TransactionId *update_xmax,
			CommandId cid, Snapshot crosscheck, bool wait)
{
	HTSU_Result result;
#ifndef FOUNDER_XDB_SE
	TransactionId xid = GetCurrentTransactionId();
#else
	TransactionId xid = InvalidTransactionId;
	if(RelationIsTemp(relation))
	{
		xid = Fxdb_GetTmpCurrentTransactionId();
	} else
	{
		CheckStandbyUpdateError("Can not execute update during recovery.");

		xid = GetCurrentTransactionId();
	}
#endif //FOUNDER_XDB_SE
	Bitmapset  *hot_attrs = NULL;
	ItemId		lp;
	HeapTupleData oldtup;
	HeapTuple	heaptup;
	Page		page;
	Buffer		buffer,
				newbuf;
	bool		need_toast,
				already_marked;
	Size		newtupsize,
				pagefree;
	bool		have_tuple_lock = false;
	bool		iscombo;
	bool		use_hot_update = false;
	bool		all_visible_cleared = false;
	bool		all_visible_cleared_new = false;

	Assert(ItemPointerIsValid(otid));

	/*
	 * Fetch the list of attributes to be checked for HOT update.  This is
	 * wasted effort if we fail to update or have to put the new tuple on a
	 * different page.	But we must compute the list before obtaining buffer
	 * lock --- in the worst case, if we are doing an update on one of the
	 * relevant system catalogs, we could deadlock if we try to fetch the list
	 * later.  In any case, the relcache caches the data so this is usually
	 * pretty cheap.
	 *
	 * Note that we get a copy here, so we need not worry about relcache flush
	 * happening midway through.
	 */
	//hot_attrs = RelationGetIndexAttrBitmap(relation);

	buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(otid));
	LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

	page = BufferGetPage(buffer);
	lp = PageGetItemId(page, ItemPointerGetOffsetNumber(otid));
	Assert(ItemIdIsNormal(lp));

	oldtup.t_data = (HeapTupleHeader) PageGetItem(page, lp);
	oldtup.t_len = ItemIdGetLength(lp);
	oldtup.t_self = *otid;

	/*
	 * Note: beyond this point, use oldtup not otid to refer to old tuple.
	 * otid may very well point at newtup->t_self, which we will overwrite
	 * with the new tuple's location, so there's great risk of confusion if we
	 * use otid anymore.
	 */

l2:
	result = HeapTupleSatisfiesUpdate(oldtup.t_data, cid, buffer);

	if (result == HeapTupleInvisible)
	{
		UnlockReleaseBuffer(buffer);
		ereport(ERROR,
                (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
                errmsg("attempted to update invisible tuple")));
	}
	else if (result == HeapTupleBeingUpdated && wait)
	{
		TransactionId xwait;
		uint16		infomask;

		/* must copy state data before unlocking buffer */
		xwait = HeapTupleHeaderGetXmax(oldtup.t_data);
		infomask = oldtup.t_data->t_infomask;

		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

		/*
		 * Acquire tuple lock to establish our priority for the tuple (see
		 * heap_lock_tuple).  LockTuple will release us when we are
		 * next-in-line for the tuple.
		 *
		 * If we are forced to "start over" below, we keep the tuple lock;
		 * this arranges that we stay at the head of the line while rechecking
		 * tuple state.
		 */
		if (!have_tuple_lock)
		{
			LockTuple(relation, &(oldtup.t_self), ExclusiveLock);
			have_tuple_lock = true;
		}

		/*
		 * Sleep until concurrent transaction ends.  Note that we don't care
		 * if the locker has an exclusive or shared lock, because we need
		 * exclusive.
		 */

		if (infomask & HEAP_XMAX_IS_MULTI)
		{
			/* wait for multixact */
			MultiXactIdWait((MultiXactId) xwait);
			LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

			/*
			 * If xwait had just locked the tuple then some other xact could
			 * update this tuple before we get to this point.  Check for xmax
			 * change, and start over if so.
			 */
			if (!(oldtup.t_data->t_infomask & HEAP_XMAX_IS_MULTI) ||
				!TransactionIdEquals(HeapTupleHeaderGetXmax(oldtup.t_data),
									 xwait))
				goto l2;

			/*
			 * You might think the multixact is necessarily done here, but not
			 * so: it could have surviving members, namely our own xact or
			 * other subxacts of this backend.	It is legal for us to update
			 * the tuple in either case, however (the latter case is
			 * essentially a situation of upgrading our former shared lock to
			 * exclusive).	We don't bother changing the on-disk hint bits
			 * since we are about to overwrite the xmax altogether.
			 */
		}
		else
		{
			/* wait for regular transaction to end */
			XactLockTableWait(xwait);
			LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);

			/*
			 * xwait is done, but if xwait had just locked the tuple then some
			 * other xact could update this tuple before we get to this point.
			 * Check for xmax change, and start over if so.
			 */
			if ((oldtup.t_data->t_infomask & HEAP_XMAX_IS_MULTI) ||
				!TransactionIdEquals(HeapTupleHeaderGetXmax(oldtup.t_data),
									 xwait))
				goto l2;

			/* Otherwise check if it committed or aborted */
			UpdateXmaxHintBits(oldtup.t_data, buffer, xwait);
		}

		/*
		 * We may overwrite if previous xmax aborted, or if it committed but
		 * only locked the tuple without updating it.
		 */
		if (oldtup.t_data->t_infomask & (HEAP_XMAX_INVALID |
										 HEAP_IS_LOCKED))
			result = HeapTupleMayBeUpdated;
		else
			result = HeapTupleUpdated;
	}

	if (crosscheck != InvalidSnapshot && result == HeapTupleMayBeUpdated)
	{
		/* Perform additional check for transaction-snapshot mode RI updates */
		if (!HeapTupleSatisfiesVisibility(&oldtup, crosscheck, buffer))
			result = HeapTupleUpdated;
	}

	if (result != HeapTupleMayBeUpdated)
	{
		Assert(result == HeapTupleSelfUpdated ||
			   result == HeapTupleUpdated ||
			   result == HeapTupleBeingUpdated);
		Assert(!(oldtup.t_data->t_infomask & HEAP_XMAX_INVALID));
		*ctid = oldtup.t_data->t_ctid;
		*update_xmax = HeapTupleHeaderGetXmax(oldtup.t_data);
		UnlockReleaseBuffer(buffer);
		if (have_tuple_lock)
			UnlockTuple(relation, &(oldtup.t_self), ExclusiveLock);
		//bms_free(hot_attrs);
		return result;
	}

	/*
	 * We're about to do the actual update -- check for conflict first, to
	 * avoid possibly having to roll back work we've just done.
	 */
	CheckForSerializableConflictIn(relation, &oldtup, buffer);

	/* Fill in OID and transaction status data for newtup */
	if (relation->rd_rel->relhasoids)
	{
#ifdef NOT_USED
		/* this is redundant with an Assert in HeapTupleSetOid */
		Assert(newtup->t_data->t_infomask & HEAP_HASOID);
#endif
		HeapTupleSetOid(newtup, HeapTupleGetOid(&oldtup));
	}
	else
	{
		/* check there is not space for an OID */
		Assert(!(newtup->t_data->t_infomask & HEAP_HASOID));
	}

	newtup->t_data->t_infomask &= ~(HEAP_XACT_MASK);
	newtup->t_data->t_infomask2 &= ~(HEAP2_XACT_MASK);
	newtup->t_data->t_infomask |= (HEAP_XMAX_INVALID | HEAP_UPDATED);
	HeapTupleHeaderSetXmin(newtup->t_data, xid);
	HeapTupleHeaderSetCmin(newtup->t_data, cid);
	HeapTupleHeaderSetXmax(newtup->t_data, 0);	/* for cleanliness */
	newtup->t_tableOid = RelationGetRelid(relation);

	/*
	 * Replace cid with a combo cid if necessary.  Note that we already put
	 * the plain cid into the new tuple.
	 */
	HeapTupleHeaderAdjustCmax(oldtup.t_data, &cid, &iscombo);

	/*
	 * If the toaster needs to be activated, OR if the new tuple will not fit
	 * on the same page as the old, then we need to release the content lock
	 * (but not the pin!) on the old tuple's buffer while we are off doing
	 * TOAST and/or table-file-extension work.	We must mark the old tuple to
	 * show that it's already being updated, else other processes may try to
	 * update it themselves.
	 *
	 * We need to invoke the toaster if there are already any out-of-line
	 * toasted values present, or if the new tuple is over-threshold.
	 */
	if (relation->rel_kind != RELKIND_RELATION)
	{
		/* toast table entries should never be recursively toasted */
		Assert(!HeapTupleHasExternal(&oldtup));
		Assert(!HeapTupleHasExternal(newtup));
		need_toast = false;
	}
	else
		need_toast = (HeapTupleHasExternal(&oldtup) ||
					  HeapTupleHasExternal(newtup) ||
					  newtup->t_len > TOAST_TUPLE_THRESHOLD);

	pagefree = PageGetHeapFreeSpace(page);

	newtupsize = MAXALIGN(newtup->t_len);

	if (need_toast || newtupsize > pagefree)
	{
		/* Clear obsolete visibility flags ... */
		oldtup.t_data->t_infomask &= ~(HEAP_XMAX_COMMITTED |
									   HEAP_XMAX_INVALID |
									   HEAP_XMAX_IS_MULTI |
									   HEAP_IS_LOCKED |
									   HEAP_MOVED);
		HeapTupleClearHotUpdated(&oldtup);
		/* ... and store info about transaction updating this tuple */
		HeapTupleHeaderSetXmax(oldtup.t_data, xid);
		HeapTupleHeaderSetCmax(oldtup.t_data, cid, iscombo);
		/* temporarily make it look not-updated */
		oldtup.t_data->t_ctid = oldtup.t_self;
		already_marked = true;
		LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

		/*
		 * Let the toaster do its thing, if needed.
		 *
		 * Note: below this point, heaptup is the data we actually intend to
		 * store into the relation; newtup is the caller's original untoasted
		 * data.
		 */
		if (need_toast)
		{
			/* Note we always use WAL and FSM during updates */
			heaptup = toast_insert_or_update(relation, newtup, &oldtup, 0);
			newtupsize = MAXALIGN(heaptup->t_len);
		}
		else
			heaptup = newtup;

		/*
		 * Now, do we need a new page for the tuple, or not?  This is a bit
		 * tricky since someone else could have added tuples to the page while
		 * we weren't looking.  We have to recheck the available space after
		 * reacquiring the buffer lock.  But don't bother to do that if the
		 * former amount of free space is still not enough; it's unlikely
		 * there's more free now than before.
		 *
		 * What's more, if we need to get a new page, we will need to acquire
		 * buffer locks on both old and new pages.	To avoid deadlock against
		 * some other backend trying to get the same two locks in the other
		 * order, we must be consistent about the order we get the locks in.
		 * We use the rule "lock the lower-numbered page of the relation
		 * first".  To implement this, we must do RelationGetBufferForTuple
		 * while not holding the lock on the old page, and we must rely on it
		 * to get the locks on both pages in the correct order.
		 */
		if (newtupsize > pagefree)
		{
			/* Assume there's no chance to put heaptup on same page. */
			newbuf = RelationGetBufferForTuple(relation, heaptup->t_len,
											   buffer, 0, NULL);
		}
		else
		{
			/* Re-acquire the lock on the old tuple's page. */
			LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
			/* Re-check using the up-to-date free space */
			pagefree = PageGetHeapFreeSpace(page);
			if (newtupsize > pagefree)
			{
				/*
				 * Rats, it doesn't fit anymore.  We must now unlock and
				 * relock to avoid deadlock.  Fortunately, this path should
				 * seldom be taken.
				 */
				LockBuffer(buffer, BUFFER_LOCK_UNLOCK);
				newbuf = RelationGetBufferForTuple(relation, heaptup->t_len,
												   buffer, 0, NULL);
			}
			else
			{
				/* OK, it fits here, so we're done. */
				newbuf = buffer;
			}
		}
	}
	else
	{
		/* No TOAST work needed, and it'll fit on same page */
		already_marked = false;
		newbuf = buffer;
		heaptup = newtup;
	}

	/*
	 * We're about to create the new tuple -- check for conflict first, to
	 * avoid possibly having to roll back work we've just done.
	 *
	 * NOTE: For a tuple insert, we only need to check for table locks, since
	 * predicate locking at the index level will cover ranges for anything
	 * except a table scan.  Therefore, only provide the relation.
	 */
	CheckForSerializableConflictIn(relation, NULL, InvalidBuffer);

	/*
	 * At this point newbuf and buffer are both pinned and locked, and newbuf
	 * has enough space for the new tuple.	If they are the same buffer, only
	 * one pin is held.
	 */

	if (newbuf == buffer)
	{
		/*
		 * Since the new tuple is going into the same page, we might be able
		 * to do a HOT update.	Check if any of the index columns have been
		 * changed.  If not, then HOT update is possible.
		 */
		if (HeapSatisfiesHOTUpdate(relation, hot_attrs, &oldtup, newtup))
			use_hot_update = true;
	}
	else
	{
		/* Set a hint that the old page could use prune/defrag */
		PageSetFull(page);
	}

	/* NO EREPORT(ERROR) from here till changes are logged */
	START_CRIT_SECTION();

	/*
	 * If this transaction commits, the old tuple will become DEAD sooner or
	 * later.  Set flag that this page is a candidate for pruning once our xid
	 * falls below the OldestXmin horizon.	If the transaction finally aborts,
	 * the subsequent page pruning will be a no-op and the hint will be
	 * cleared.
	 *
	 * XXX Should we set hint on newbuf as well?  If the transaction aborts,
	 * there would be a prunable tuple in the newbuf; but for now we choose
	 * not to optimize for aborts.	Note that heap_xlog_update must be kept in
	 * sync if this decision changes.
	 */
	PageSetPrunable(page, xid);

	if (use_hot_update)
	{
		/* Mark the old tuple as HOT-updated */
		HeapTupleSetHotUpdated(&oldtup);
		/* And mark the new tuple as heap-only */
		HeapTupleSetHeapOnly(heaptup);
		/* Mark the caller's copy too, in case different from heaptup */
		HeapTupleSetHeapOnly(newtup);
	}
	else
	{
		/* Make sure tuples are correctly marked as not-HOT */
		HeapTupleClearHotUpdated(&oldtup);
		HeapTupleClearHeapOnly(heaptup);
		HeapTupleClearHeapOnly(newtup);
	}

	RelationPutHeapTuple(relation, newbuf, heaptup);	/* insert new tuple */

	if (!already_marked)
	{
		/* Clear obsolete visibility flags ... */
		oldtup.t_data->t_infomask &= ~(HEAP_XMAX_COMMITTED |
									   HEAP_XMAX_INVALID |
									   HEAP_XMAX_IS_MULTI |
									   HEAP_IS_LOCKED |
									   HEAP_MOVED);
		/* ... and store info about transaction updating this tuple */
		HeapTupleHeaderSetXmax(oldtup.t_data, xid);
		HeapTupleHeaderSetCmax(oldtup.t_data, cid, iscombo);
	}

	/* record address of new tuple in t_ctid of old one */
	oldtup.t_data->t_ctid = heaptup->t_self;

	/* clear PD_ALL_VISIBLE flags */
	if (PageIsAllVisible(BufferGetPage(buffer)))
	{
		all_visible_cleared = true;
		PageClearAllVisible(BufferGetPage(buffer));
	}
	if (newbuf != buffer && PageIsAllVisible(BufferGetPage(newbuf)))
	{
		all_visible_cleared_new = true;
		PageClearAllVisible(BufferGetPage(newbuf));
	}

	if (newbuf != buffer)
		MarkBufferDirty(newbuf);
	MarkBufferDirty(buffer);

	/* XLOG stuff */
	if (RelationNeedsWAL(relation))
	{
		XLogRecPtr	recptr = log_heap_update(relation, buffer, oldtup.t_self,
											 newbuf, heaptup,
											 all_visible_cleared,
											 all_visible_cleared_new);

		if (newbuf != buffer)
		{
			PageSetLSN(BufferGetPage(newbuf), recptr);
			PageSetTLI(BufferGetPage(newbuf), ThisTimeLineID);
		}
		PageSetLSN(BufferGetPage(buffer), recptr);
		PageSetTLI(BufferGetPage(buffer), ThisTimeLineID);
	}

	END_CRIT_SECTION();

	if (newbuf != buffer)
		LockBuffer(newbuf, BUFFER_LOCK_UNLOCK);
	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

	/*
	 * Mark old tuple for invalidation from system caches at next command
	 * boundary. We have to do this before releasing the buffer because we
	 * need to look at the contents of the tuple.
	 */
	CacheInvalidateHeapTuple(relation, &oldtup);

	/* Clear bits in visibility map */
	if (all_visible_cleared)
		visibilitymap_clear(relation, BufferGetBlockNumber(buffer));
	if (all_visible_cleared_new)
		visibilitymap_clear(relation, BufferGetBlockNumber(newbuf));

	/* Now we can release the buffer(s) */
	if (newbuf != buffer)
		ReleaseBuffer(newbuf);
	ReleaseBuffer(buffer);

	/*
	 * If new tuple is cachable, mark it for invalidation from the caches in
	 * case we abort.  Note it is OK to do this after releasing the buffer,
	 * because the heaptup data structure is all in local memory, not in the
	 * shared buffer.
	 */
	CacheInvalidateHeapTuple(relation, heaptup);

	/*
	 * Release the lmgr tuple lock, if we had it.
	 */
	if (have_tuple_lock)
		UnlockTuple(relation, &(oldtup.t_self), ExclusiveLock);

	pgstat_count_heap_update(relation, use_hot_update);

	/*
	 * If heaptup is a private copy, release it.  Don't forget to copy t_self
	 * back to the caller's image, too.
	 */
	if (heaptup != newtup)
	{
		newtup->t_self = heaptup->t_self;
		heap_freetuple(heaptup);
	}

	//bms_free(hot_attrs);

	return HeapTupleMayBeUpdated;
}

/*
 * Check if the specified attribute's value is same in both given tuples.
 * Subroutine for HeapSatisfiesHOTUpdate.
 */
static bool
heap_tuple_attr_equals(TupleDesc tupdesc, int attrnum,
					   HeapTuple tup1, HeapTuple tup2)
{
	Datum		value1,
				value2;
	bool		isnull1,
				isnull2;
	Form_pg_attribute att;

	/*
	 * If it's a whole-tuple reference, say "not equal".  It's not really
	 * worth supporting this case, since it could only succeed after a no-op
	 * update, which is hardly a case worth optimizing for.
	 */
	if (attrnum == 0)
		return false;

	/*
	 * Likewise, automatically say "not equal" for any system attribute other
	 * than OID and tableOID; we cannot expect these to be consistent in a HOT
	 * chain, or even to be set correctly yet in the new tuple.
	 */
	if (attrnum < 0)
	{
		if (attrnum != ObjectIdAttributeNumber &&
			attrnum != TableOidAttributeNumber)
			return false;
	}

	/*
	 * Extract the corresponding values.  XXX this is pretty inefficient if
	 * there are many indexed columns.	Should HeapSatisfiesHOTUpdate do a
	 * single heap_deform_tuple call on each tuple, instead?  But that doesn't
	 * work for system columns ...
	 */
	value1 = heap_getattr(tup1, attrnum, tupdesc, &isnull1);
	value2 = heap_getattr(tup2, attrnum, tupdesc, &isnull2);

	/*
	 * If one value is NULL and other is not, then they are certainly not
	 * equal
	 */
	if (isnull1 != isnull2)
		return false;

	/*
	 * If both are NULL, they can be considered equal.
	 */
	if (isnull1)
		return true;

	/*
	 * We do simple binary comparison of the two datums.  This may be overly
	 * strict because there can be multiple binary representations for the
	 * same logical value.	But we should be OK as long as there are no false
	 * positives.  Using a type-specific equality operator is messy because
	 * there could be multiple notions of equality in different operator
	 * classes; furthermore, we cannot safely invoke user-defined functions
	 * while holding exclusive buffer lock.
	 */
	if (attrnum <= 0)
	{
		/* The only allowed system columns are OIDs, so do this */
		return (DatumGetObjectId(value1) == DatumGetObjectId(value2));
	}
	else
	{
		Assert(attrnum <= tupdesc->natts);
		att = tupdesc->attrs[attrnum - 1];
		return datumIsEqual(value1, value2, att->attbyval, att->attlen);
	}
}

#ifdef FOUNDER_XDB_SE
static bool 
Heap_index_equals(Relation relation, HeapTuple oldtup, HeapTuple newtup)
{
	bool flag = true;
	TupleDesc tupdesc = RelationGetDescr(relation);
	Form_pg_attribute att = tupdesc->attrs[0];
	for (unsigned int i = 0; i < MtInfo_GetIndexCount(relation->mt_info); ++i) 
	{
		Oid index_id = MtInfo_GetIndexOid(relation->mt_info)[i];
		bool isnull = false;
		Relation index = index_open(index_id,RowShareLock);
		Datum valuesold = fdxdb_form_index_datum(relation, index, oldtup);
		Datum valuesnew = fdxdb_form_index_datum(relation, index, newtup);

		if ((valuesold != 0 && valuesold != (Datum)-1) && (valuesnew != 0 && valuesnew != (Datum)-1))
		{
			if (!datumIsEqual(valuesold,valuesnew,att->attbyval,att->attlen))
			{
				if (DatumGetPointer(valuesold) != NULL)
            		pfree(DatumGetPointer(valuesold));
				if (DatumGetPointer(valuesnew) != NULL)
            		pfree(DatumGetPointer(valuesnew));
				index_close(index,NoLock);
				return false;
			}
        }
		else if (valuesold == (Datum)-1 && valuesnew == (Datum)-1) 
		{
            /*size_t noldkeys = 0;
			size_t nnewkeys = 0;
            Datum *pvaluesold = combineIndexKeys(relation, index, oldtup, &noldkeys);
			Datum *pvaluesnew = combineIndexKeys(relation, index, newtup, &nnewkeys);

			size_t i = 0;
			if (noldkeys == nnewkeys)
			{
				for (i=0; i < noldkeys; i++) 
				{
					if (!datumIsEqual(pvaluesold[i],pvaluesnew[i],att->attbyval,att->attlen))
					{
						flag = false;
						break;
					}
				}
			}
			else
			{
				flag = false;
			}
			
			for(i=0; i < noldkeys; i++)
			{
				pfree(DatumGetPointer(pvaluesold[i]));
			}
			for(i=0; i < nnewkeys; i++)
			{
				pfree(DatumGetPointer(pvaluesnew[i]));
			}
            pfree(pvaluesold);
			pfree(pvaluesnew);*/
			flag = false;
        }
		else
		{
			flag = false;
		}
		
		if(valuesold != 0 && valuesold != (Datum)-1)
		{
		    if (DatumGetPointer(valuesold) != NULL)
             pfree(DatumGetPointer(valuesold));
        }
	    	if(valuesnew != 0 && valuesnew != (Datum)-1)
		{
	    	if (DatumGetPointer(valuesnew) != NULL)
             pfree(DatumGetPointer(valuesnew));
        }
		index_close(index,NoLock);

		if (!flag)
			break;
	}
	return flag;
}
#endif

/*
 * Check if the old and new tuples represent a HOT-safe update. To be able
 * to do a HOT update, we must not have changed any columns used in index
 * definitions.
 *
 * The set of attributes to be checked is passed in (we dare not try to
 * compute it while holding exclusive buffer lock...)  NOTE that hot_attrs
 * is destructively modified!  That is OK since this is invoked at most once
 * by heap_update().
 *
 * Returns true if safe to do HOT update.
 */
static bool
HeapSatisfiesHOTUpdate(Relation relation, Bitmapset *hot_attrs,
					   HeapTuple oldtup, HeapTuple newtup)
{
#ifndef FOUNDER_XDB_SE
	int			attrnum;

	while ((attrnum = bms_first_member(hot_attrs)) >= 0)
	{
		/* Adjust for system attributes */
		attrnum += FirstLowInvalidHeapAttributeNumber;

		/* If the attribute value has changed, we can't do HOT update */
		if (!heap_tuple_attr_equals(RelationGetDescr(relation), attrnum,
									oldtup, newtup))
			return false;
	}
	return true;
#else
	bool flag = true;
	if(MtInfo_GetIndexCount(relation->mt_info) <= 0)
		return true;
	else 
		return Heap_index_equals(relation,oldtup,newtup);
#endif //FOUNDER_XDB_SE
}

/*
 *	simple_heap_update - replace a tuple
 *
 * This routine may be used to update a tuple when concurrent updates of
 * the target tuple are not expected (for example, because we have a lock
 * on the relation associated with the tuple).	Any failure is reported
 * via ereport().
 */
#ifdef FOUNDER_XDB_SE
void
simple_heap_update(Relation relation, ItemPointer otid, HeapTuple tup)
#else
void
simple_heap_update(Relation relation, ItemPointer otid, HeapTuple tup)
#endif
{
	HTSU_Result result;
	ItemPointerData update_ctid;
	TransactionId update_xmax;

#ifdef FOUNDER_XDB_SE
	result = heap_update(relation, otid, tup,
						 &update_ctid, &update_xmax,
						 GetCurrentCommandId(true), InvalidSnapshot,
						 true /* wait for commit */ );
		
#else
result = heap_update(relation, otid, tup,
					 &update_ctid, &update_xmax,
					 GetCurrentCommandId(true), InvalidSnapshot,
					 true /* wait for commit */ );
#endif

	switch (result)
	{
		case HeapTupleSelfUpdated:
			/* Tuple was already updated in current command? */
			ereport(ERROR,
                    (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
                    errmsg("tuple already updated by self")));
			break;

		case HeapTupleMayBeUpdated:
			if(!HeapTupleIsHeapOnly(tup))
				insert_tuple_to_indexes(relation,tup);
			/* done successfully */
			break;

		case HeapTupleUpdated:
			ereport(ERROR,
                    (errcode(ERRCODE_TUPLE_UPDATE_CONCURRENTLY),
                    errmsg("tuple concurrently updated")));
			break;

		default:
			ereport(ERROR,
                    (errcode(ERRCODE_UNDEFINED_PARAMETER),
                    errmsg("unrecognized heap_update status: %u", result)));
			break;
	}
}

/*
 *	heap_lock_tuple - lock a tuple in shared or exclusive mode
 *
 * Note that this acquires a buffer pin, which the caller must release.
 *
 * Input parameters:
 *	relation: relation containing tuple (caller must hold suitable lock)
 *	tuple->t_self: TID of tuple to lock (rest of struct need not be valid)
 *	cid: current command ID (used for visibility test, and stored into
 *		tuple's cmax if lock is successful)
 *	mode: indicates if shared or exclusive tuple lock is desired
 *	nowait: if true, ereport rather than blocking if lock not available
 *
 * Output parameters:
 *	*tuple: all fields filled in
 *	*buffer: set to buffer holding tuple (pinned but not locked at exit)
 *	*ctid: set to tuple's t_ctid, but only in failure cases
 *	*update_xmax: set to tuple's xmax, but only in failure cases
 *
 * Function result may be:
 *	HeapTupleMayBeUpdated: lock was successfully acquired
 *	HeapTupleSelfUpdated: lock failed because tuple updated by self
 *	HeapTupleUpdated: lock failed because tuple updated by other xact
 *
 * In the failure cases, the routine returns the tuple's t_ctid and t_xmax.
 * If t_ctid is the same as t_self, the tuple was deleted; if different, the
 * tuple was updated, and t_ctid is the location of the replacement tuple.
 * (t_xmax is needed to verify that the replacement tuple matches.)
 *
 *
 * NOTES: because the shared-memory lock table is of finite size, but users
 * could reasonably want to lock large numbers of tuples, we do not rely on
 * the standard lock manager to store tuple-level locks over the long term.
 * Instead, a tuple is marked as locked by setting the current transaction's
 * XID as its XMAX, and setting additional infomask bits to distinguish this
 * usage from the more normal case of having deleted the tuple.  When
 * multiple transactions concurrently share-lock a tuple, the first locker's
 * XID is replaced in XMAX with a MultiTransactionId representing the set of
 * XIDs currently holding share-locks.
 *
 * When it is necessary to wait for a tuple-level lock to be released, the
 * basic delay is provided by XactLockTableWait or MultiXactIdWait on the
 * contents of the tuple's XMAX.  However, that mechanism will release all
 * waiters concurrently, so there would be a race condition as to which
 * waiter gets the tuple, potentially leading to indefinite starvation of
 * some waiters.  The possibility of share-locking makes the problem much
 * worse --- a steady stream of share-lockers can easily block an exclusive
 * locker forever.	To provide more reliable semantics about who gets a
 * tuple-level lock first, we use the standard lock manager.  The protocol
 * for waiting for a tuple-level lock is really
 *		LockTuple()
 *		XactLockTableWait()
 *		mark tuple as locked by me
 *		UnlockTuple()
 * When there are multiple waiters, arbitration of who is to get the lock next
 * is provided by LockTuple().	However, at most one tuple-level lock will
 * be held or awaited per backend at any time, so we don't risk overflow
 * of the lock table.  Note that incoming share-lockers are required to
 * do LockTuple as well, if there is any conflict, to ensure that they don't
 * starve out waiting exclusive-lockers.  However, if there is not any active
 * conflict for a tuple, we don't incur any extra overhead.
 */
HTSU_Result
heap_lock_tuple(Relation relation, HeapTuple tuple, Buffer *buffer,
				ItemPointer ctid, TransactionId *update_xmax,
				CommandId cid, LockTupleMode mode, bool nowait)
{
	HTSU_Result result;
	ItemPointer tid = &(tuple->t_self);
	ItemId		lp;
	Page		page;
	TransactionId xid;
	TransactionId xmax;
	uint16		old_infomask;
	uint16		new_infomask;
	LOCKMODE	tuple_lock_type;
	bool		have_tuple_lock = false;

	tuple_lock_type = (mode == LockTupleShared) ? ShareLock : ExclusiveLock;

	*buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(tid));
	LockBuffer(*buffer, BUFFER_LOCK_EXCLUSIVE);

	page = BufferGetPage(*buffer);
	lp = PageGetItemId(page, ItemPointerGetOffsetNumber(tid));
	Assert(ItemIdIsNormal(lp));

	tuple->t_data = (HeapTupleHeader) PageGetItem(page, lp);
	tuple->t_len = ItemIdGetLength(lp);
	tuple->t_tableOid = RelationGetRelid(relation);

l3:
	result = HeapTupleSatisfiesUpdate(tuple->t_data, cid, *buffer);

	if (result == HeapTupleInvisible)
	{
		UnlockReleaseBuffer(*buffer);
		ereport(ERROR,
                (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
                errmsg("attempted to lock invisible tuple")));
	}
	else if (result == HeapTupleBeingUpdated)
	{
		TransactionId xwait;
		uint16		infomask;

		/* must copy state data before unlocking buffer */
		xwait = HeapTupleHeaderGetXmax(tuple->t_data);
		infomask = tuple->t_data->t_infomask;

		LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);

		/*
		 * If we wish to acquire share lock, and the tuple is already
		 * share-locked by a multixact that includes any subtransaction of the
		 * current top transaction, then we effectively hold the desired lock
		 * already.  We *must* succeed without trying to take the tuple lock,
		 * else we will deadlock against anyone waiting to acquire exclusive
		 * lock.  We don't need to make any state changes in this case.
		 */
		if (mode == LockTupleShared &&
			(infomask & HEAP_XMAX_IS_MULTI) &&
			MultiXactIdIsCurrent((MultiXactId) xwait))
		{
			Assert(infomask & HEAP_XMAX_SHARED_LOCK);
			/* Probably can't hold tuple lock here, but may as well check */
			if (have_tuple_lock)
				UnlockTuple(relation, tid, tuple_lock_type);
			return HeapTupleMayBeUpdated;
		}

		/*
		 * Acquire tuple lock to establish our priority for the tuple.
		 * LockTuple will release us when we are next-in-line for the tuple.
		 * We must do this even if we are share-locking.
		 *
		 * If we are forced to "start over" below, we keep the tuple lock;
		 * this arranges that we stay at the head of the line while rechecking
		 * tuple state.
		 */
		if (!have_tuple_lock)
		{
			if (nowait)
			{
				if (!ConditionalLockTuple(relation, tid, tuple_lock_type))
					ereport(ERROR,
							(errcode(ERRCODE_LOCK_NOT_AVAILABLE),
					errmsg("could not obtain lock on row in relation \"%s\"",
						   RelationGetRelationName(relation))));
			}
			else
				LockTuple(relation, tid, tuple_lock_type);
			have_tuple_lock = true;
		}

		if (mode == LockTupleShared && (infomask & HEAP_XMAX_SHARED_LOCK))
		{
			/*
			 * Acquiring sharelock when there's at least one sharelocker
			 * already.  We need not wait for him/them to complete.
			 */
			LockBuffer(*buffer, BUFFER_LOCK_EXCLUSIVE);

			/*
			 * Make sure it's still a shared lock, else start over.  (It's OK
			 * if the ownership of the shared lock has changed, though.)
			 */
			if (!(tuple->t_data->t_infomask & HEAP_XMAX_SHARED_LOCK))
				goto l3;
		}
		else if (infomask & HEAP_XMAX_IS_MULTI)
		{
			/* wait for multixact to end */
			if (nowait)
			{
				if (!ConditionalMultiXactIdWait((MultiXactId) xwait))
					ereport(ERROR,
							(errcode(ERRCODE_LOCK_NOT_AVAILABLE),
					errmsg("could not obtain lock on row in relation \"%s\"",
						   RelationGetRelationName(relation))));
			}
			else
				MultiXactIdWait((MultiXactId) xwait);

			LockBuffer(*buffer, BUFFER_LOCK_EXCLUSIVE);

			/*
			 * If xwait had just locked the tuple then some other xact could
			 * update this tuple before we get to this point. Check for xmax
			 * change, and start over if so.
			 */
			if (!(tuple->t_data->t_infomask & HEAP_XMAX_IS_MULTI) ||
				!TransactionIdEquals(HeapTupleHeaderGetXmax(tuple->t_data),
									 xwait))
				goto l3;

			/*
			 * You might think the multixact is necessarily done here, but not
			 * so: it could have surviving members, namely our own xact or
			 * other subxacts of this backend.	It is legal for us to lock the
			 * tuple in either case, however.  We don't bother changing the
			 * on-disk hint bits since we are about to overwrite the xmax
			 * altogether.
			 */
		}
		else
		{
			/* wait for regular transaction to end */
			if (nowait)
			{
				if (!ConditionalXactLockTableWait(xwait))
					ereport(ERROR,
							(errcode(ERRCODE_LOCK_NOT_AVAILABLE),
					errmsg("could not obtain lock on row in relation \"%s\"",
						   RelationGetRelationName(relation))));
			}
			else
				XactLockTableWait(xwait);

			LockBuffer(*buffer, BUFFER_LOCK_EXCLUSIVE);

			/*
			 * xwait is done, but if xwait had just locked the tuple then some
			 * other xact could update this tuple before we get to this point.
			 * Check for xmax change, and start over if so.
			 */
			if ((tuple->t_data->t_infomask & HEAP_XMAX_IS_MULTI) ||
				!TransactionIdEquals(HeapTupleHeaderGetXmax(tuple->t_data),
									 xwait))
				goto l3;

			/* Otherwise check if it committed or aborted */
			UpdateXmaxHintBits(tuple->t_data, *buffer, xwait);
		}

		/*
		 * We may lock if previous xmax aborted, or if it committed but only
		 * locked the tuple without updating it.  The case where we didn't
		 * wait because we are joining an existing shared lock is correctly
		 * handled, too.
		 */
		if (tuple->t_data->t_infomask & (HEAP_XMAX_INVALID |
										 HEAP_IS_LOCKED))
			result = HeapTupleMayBeUpdated;
		else
			result = HeapTupleUpdated;
	}

	if (result != HeapTupleMayBeUpdated)
	{
		Assert(result == HeapTupleSelfUpdated || result == HeapTupleUpdated);
		Assert(!(tuple->t_data->t_infomask & HEAP_XMAX_INVALID));
		*ctid = tuple->t_data->t_ctid;
		*update_xmax = HeapTupleHeaderGetXmax(tuple->t_data);
		LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);
		if (have_tuple_lock)
			UnlockTuple(relation, tid, tuple_lock_type);
		return result;
	}

	/*
	 * We might already hold the desired lock (or stronger), possibly under a
	 * different subtransaction of the current top transaction.  If so, there
	 * is no need to change state or issue a WAL record.  We already handled
	 * the case where this is true for xmax being a MultiXactId, so now check
	 * for cases where it is a plain TransactionId.
	 *
	 * Note in particular that this covers the case where we already hold
	 * exclusive lock on the tuple and the caller only wants shared lock. It
	 * would certainly not do to give up the exclusive lock.
	 */
	xmax = HeapTupleHeaderGetXmax(tuple->t_data);
	old_infomask = tuple->t_data->t_infomask;

	if (!(old_infomask & (HEAP_XMAX_INVALID |
						  HEAP_XMAX_COMMITTED |
						  HEAP_XMAX_IS_MULTI)) &&
		(mode == LockTupleShared ?
		 (old_infomask & HEAP_IS_LOCKED) :
		 (old_infomask & HEAP_XMAX_EXCL_LOCK)) &&
		TransactionIdIsCurrentTransactionId(xmax))
	{
		LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);
		/* Probably can't hold tuple lock here, but may as well check */
		if (have_tuple_lock)
			UnlockTuple(relation, tid, tuple_lock_type);
		return HeapTupleMayBeUpdated;
	}

	/*
	 * Compute the new xmax and infomask to store into the tuple.  Note we do
	 * not modify the tuple just yet, because that would leave it in the wrong
	 * state if multixact.c elogs.
	 */
	xid = GetCurrentTransactionId();

	new_infomask = old_infomask & ~(HEAP_XMAX_COMMITTED |
									HEAP_XMAX_INVALID |
									HEAP_XMAX_IS_MULTI |
									HEAP_IS_LOCKED |
									HEAP_MOVED);

	if (mode == LockTupleShared)
	{
		/*
		 * If this is the first acquisition of a shared lock in the current
		 * transaction, set my per-backend OldestMemberMXactId setting. We can
		 * be certain that the transaction will never become a member of any
		 * older MultiXactIds than that.  (We have to do this even if we end
		 * up just using our own TransactionId below, since some other backend
		 * could incorporate our XID into a MultiXact immediately afterwards.)
		 */
		MultiXactIdSetOldestMember();

		new_infomask |= HEAP_XMAX_SHARED_LOCK;

		/*
		 * Check to see if we need a MultiXactId because there are multiple
		 * lockers.
		 *
		 * HeapTupleSatisfiesUpdate will have set the HEAP_XMAX_INVALID bit if
		 * the xmax was a MultiXactId but it was not running anymore. There is
		 * a race condition, which is that the MultiXactId may have finished
		 * since then, but that uncommon case is handled within
		 * MultiXactIdExpand.
		 *
		 * There is a similar race condition possible when the old xmax was a
		 * regular TransactionId.  We test TransactionIdIsInProgress again
		 * just to narrow the window, but it's still possible to end up
		 * creating an unnecessary MultiXactId.  Fortunately this is harmless.
		 */
		if (!(old_infomask & (HEAP_XMAX_INVALID | HEAP_XMAX_COMMITTED)))
		{
			if (old_infomask & HEAP_XMAX_IS_MULTI)
			{
				/*
				 * If the XMAX is already a MultiXactId, then we need to
				 * expand it to include our own TransactionId.
				 */
				xid = MultiXactIdExpand((MultiXactId) xmax, xid);
				new_infomask |= HEAP_XMAX_IS_MULTI;
			}
			else if (TransactionIdIsInProgress(xmax))
			{
				/*
				 * If the XMAX is a valid TransactionId, then we need to
				 * create a new MultiXactId that includes both the old locker
				 * and our own TransactionId.
				 */
				xid = MultiXactIdCreate(xmax, xid);
				new_infomask |= HEAP_XMAX_IS_MULTI;
			}
			else
			{
				/*
				 * Can get here iff HeapTupleSatisfiesUpdate saw the old xmax
				 * as running, but it finished before
				 * TransactionIdIsInProgress() got to run.	Treat it like
				 * there's no locker in the tuple.
				 */
			}
		}
		else
		{
			/*
			 * There was no previous locker, so just insert our own
			 * TransactionId.
			 */
		}
	}
	else
	{
		/* We want an exclusive lock on the tuple */
		new_infomask |= HEAP_XMAX_EXCL_LOCK;
	}

	START_CRIT_SECTION();

	/*
	 * Store transaction information of xact locking the tuple.
	 *
	 * Note: Cmax is meaningless in this context, so don't set it; this avoids
	 * possibly generating a useless combo CID.
	 */
	tuple->t_data->t_infomask = new_infomask;
	HeapTupleHeaderClearHotUpdated(tuple->t_data);
	HeapTupleHeaderSetXmax(tuple->t_data, xid);
	/* Make sure there is no forward chain link in t_ctid */
	tuple->t_data->t_ctid = *tid;

	MarkBufferDirty(*buffer);

	/*
	 * XLOG stuff.	You might think that we don't need an XLOG record because
	 * there is no state change worth restoring after a crash.	You would be
	 * wrong however: we have just written either a TransactionId or a
	 * MultiXactId that may never have been seen on disk before, and we need
	 * to make sure that there are XLOG entries covering those ID numbers.
	 * Else the same IDs might be re-used after a crash, which would be
	 * disastrous if this page made it to disk before the crash.  Essentially
	 * we have to enforce the WAL log-before-data rule even in this case.
	 * (Also, in a PITR log-shipping or 2PC environment, we have to have XLOG
	 * entries for everything anyway.)
	 */
	if (RelationNeedsWAL(relation))
	{
		xl_heap_lock xlrec;
		XLogRecPtr	recptr;
		XLogRecData rdata[2];

		xlrec.target.node = relation->rd_node;
		xlrec.target.tid = tuple->t_self;
		xlrec.locking_xid = xid;
		xlrec.xid_is_mxact = ((new_infomask & HEAP_XMAX_IS_MULTI) != 0);
		xlrec.shared_lock = (mode == LockTupleShared);
		rdata[0].data = (char *) &xlrec;
		rdata[0].len = SizeOfHeapLock;
		rdata[0].buffer = InvalidBuffer;
		rdata[0].next = &(rdata[1]);

		rdata[1].data = NULL;
		rdata[1].len = 0;
		rdata[1].buffer = *buffer;
		rdata[1].buffer_std = true;
		rdata[1].next = NULL;

		recptr = XLogInsert(RM_HEAP_ID, XLOG_HEAP_LOCK, rdata);

		PageSetLSN(page, recptr);
		PageSetTLI(page, ThisTimeLineID);
	}

	END_CRIT_SECTION();

	LockBuffer(*buffer, BUFFER_LOCK_UNLOCK);

	/*
	 * Don't update the visibility map here. Locking a tuple doesn't change
	 * visibility info.
	 */

	/*
	 * Now that we have successfully marked the tuple as locked, we can
	 * release the lmgr tuple lock, if we had it.
	 */
	if (have_tuple_lock)
		UnlockTuple(relation, tid, tuple_lock_type);

	return HeapTupleMayBeUpdated;
}


/*
 * heap_inplace_update - update a tuple "in place" (ie, overwrite it)
 *
 * Overwriting violates both MVCC and transactional safety, so the uses
 * of this function in Postgres are extremely limited.	Nonetheless we
 * find some places to use it.
 *
 * The tuple cannot change size, and therefore it's reasonable to assume
 * that its null bitmap (if any) doesn't change either.  So we just
 * overwrite the data portion of the tuple without touching the null
 * bitmap or any of the header fields.
 *
 * tuple is an in-memory tuple structure containing the data to be written
 * over the target tuple.  Also, tuple->t_self identifies the target tuple.
 */
void
heap_inplace_update(Relation relation, HeapTuple tuple)
{
	Buffer		buffer;
	Page		page;
	OffsetNumber offnum;
	ItemId		lp = NULL;
	HeapTupleHeader htup;
	uint32		oldlen;
	uint32		newlen;

#ifdef FOUNDER_XDB_SE
	if(!RelationIsTemp(relation))
		CheckStandbyUpdateError("Can not execute inplace update during recovery.");
#endif //FOUNDER_XDB_SE

	buffer = ReadBuffer(relation, ItemPointerGetBlockNumber(&(tuple->t_self)));
	LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
	page = (Page) BufferGetPage(buffer);

	offnum = ItemPointerGetOffsetNumber(&(tuple->t_self));
	if (PageGetMaxOffsetNumber(page) >= offnum)
		lp = PageGetItemId(page, offnum);

	if (PageGetMaxOffsetNumber(page) < offnum || !ItemIdIsNormal(lp))
		ereport(ERROR,
                (errcode(ERRCODE_HEAP_INPLACE_UPDATE_INVALID_ITEMID),
                errmsg("heap_inplace_update: invalid lp")));

	htup = (HeapTupleHeader) PageGetItem(page, lp);

	oldlen = ItemIdGetLength(lp) - htup->t_hoff;
	newlen = tuple->t_len - tuple->t_data->t_hoff;
	if (oldlen != newlen || htup->t_hoff != tuple->t_data->t_hoff)
		ereport(ERROR,
                (errcode(ERRCODE_HEAP_INPLACE_UPDATE_WRONG_TUPLE_LEN),
                errmsg("heap_inplace_update: wrong tuple length")));

	/* NO EREPORT(ERROR) from here till changes are logged */
	START_CRIT_SECTION();

	memcpy((char *) htup + htup->t_hoff,
		   (char *) tuple->t_data + tuple->t_data->t_hoff,
		   newlen);

	MarkBufferDirty(buffer);

	/* XLOG stuff */
	if (RelationNeedsWAL(relation))
	{
		xl_heap_inplace xlrec;
		XLogRecPtr	recptr;
		XLogRecData rdata[2];

		xlrec.target.node = relation->rd_node;
		xlrec.target.tid = tuple->t_self;

		rdata[0].data = (char *) &xlrec;
		rdata[0].len = SizeOfHeapInplace;
		rdata[0].buffer = InvalidBuffer;
		rdata[0].next = &(rdata[1]);

		rdata[1].data = (char *) htup + htup->t_hoff;
		rdata[1].len = newlen;
		rdata[1].buffer = buffer;
		rdata[1].buffer_std = true;
		rdata[1].next = NULL;

		recptr = XLogInsert(RM_HEAP_ID, XLOG_HEAP_INPLACE, rdata);

		PageSetLSN(page, recptr);
		PageSetTLI(page, ThisTimeLineID);
	}

	END_CRIT_SECTION();

	UnlockReleaseBuffer(buffer);

	/* Send out shared cache inval if necessary */
	if (!IsBootstrapProcessingMode())
		CacheInvalidateHeapTuple(relation, tuple);
}


/*
 * heap_freeze_tuple
 *
 * Check to see whether any of the XID fields of a tuple (xmin, xmax, xvac)
 * are older than the specified cutoff XID.  If so, replace them with
 * FrozenTransactionId or InvalidTransactionId as appropriate, and return
 * TRUE.  Return FALSE if nothing was changed.
 *
 * It is assumed that the caller has checked the tuple with
 * HeapTupleSatisfiesVacuum() and determined that it is not HEAPTUPLE_DEAD
 * (else we should be removing the tuple, not freezing it).
 *
 * NB: cutoff_xid *must* be <= the current global xmin, to ensure that any
 * XID older than it could neither be running nor seen as running by any
 * open transaction.  This ensures that the replacement will not change
 * anyone's idea of the tuple state.  Also, since we assume the tuple is
 * not HEAPTUPLE_DEAD, the fact that an XID is not still running allows us
 * to assume that it is either committed good or aborted, as appropriate;
 * so we need no external state checks to decide what to do.  (This is good
 * because this function is applied during WAL recovery, when we don't have
 * access to any such state, and can't depend on the hint bits to be set.)
 *
 * In lazy VACUUM, we call this while initially holding only a shared lock
 * on the tuple's buffer.  If any change is needed, we trade that in for an
 * exclusive lock before making the change.  Caller should pass the buffer ID
 * if shared lock is held, InvalidBuffer if exclusive lock is already held.
 *
 * Note: it might seem we could make the changes without exclusive lock, since
 * TransactionId read/write is assumed atomic anyway.  However there is a race
 * condition: someone who just fetched an old XID that we overwrite here could
 * conceivably not finish checking the XID against pg_clog before we finish
 * the VACUUM and perhaps truncate off the part of pg_clog he needs.  Getting
 * exclusive lock ensures no other backend is in process of checking the
 * tuple status.  Also, getting exclusive lock makes it safe to adjust the
 * infomask bits.
 */
bool
heap_freeze_tuple(HeapTupleHeader tuple, TransactionId cutoff_xid,
				  Buffer buf)
{
	bool		changed = false;
	TransactionId xid;

	xid = HeapTupleHeaderGetXmin(tuple);
	if (TransactionIdIsNormal(xid) &&
		TransactionIdPrecedes(xid, cutoff_xid))
	{
		if (buf != InvalidBuffer)
		{
			/* trade in share lock for exclusive lock */
			LockBuffer(buf, BUFFER_LOCK_UNLOCK);
			LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);
			buf = InvalidBuffer;
		}
		HeapTupleHeaderSetXmin(tuple, FrozenTransactionId);

		/*
		 * Might as well fix the hint bits too; usually XMIN_COMMITTED will
		 * already be set here, but there's a small chance not.
		 */
		Assert(!(tuple->t_infomask & HEAP_XMIN_INVALID));
		tuple->t_infomask |= HEAP_XMIN_COMMITTED;
		changed = true;
	}

	/*
	 * When we release shared lock, it's possible for someone else to change
	 * xmax before we get the lock back, so repeat the check after acquiring
	 * exclusive lock.	(We don't need this pushup for xmin, because only
	 * VACUUM could be interested in changing an existing tuple's xmin, and
	 * there's only one VACUUM allowed on a table at a time.)
	 */
recheck_xmax:
	if (!(tuple->t_infomask & HEAP_XMAX_IS_MULTI))
	{
		xid = HeapTupleHeaderGetXmax(tuple);
		if (TransactionIdIsNormal(xid) &&
			TransactionIdPrecedes(xid, cutoff_xid))
		{
			if (buf != InvalidBuffer)
			{
				/* trade in share lock for exclusive lock */
				LockBuffer(buf, BUFFER_LOCK_UNLOCK);
				LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);
				buf = InvalidBuffer;
				goto recheck_xmax;		/* see comment above */
			}
			HeapTupleHeaderSetXmax(tuple, InvalidTransactionId);

			/*
			 * The tuple might be marked either XMAX_INVALID or XMAX_COMMITTED
			 * + LOCKED.  Normalize to INVALID just to be sure no one gets
			 * confused.
			 */
			tuple->t_infomask &= ~HEAP_XMAX_COMMITTED;
			tuple->t_infomask |= HEAP_XMAX_INVALID;
			HeapTupleHeaderClearHotUpdated(tuple);
			changed = true;
		}
	}
	else
	{
		/*----------
		 * XXX perhaps someday we should zero out very old MultiXactIds here?
		 *
		 * The only way a stale MultiXactId could pose a problem is if a
		 * tuple, having once been multiply-share-locked, is not touched by
		 * any vacuum or attempted lock or deletion for just over 4G MultiXact
		 * creations, and then in the probably-narrow window where its xmax
		 * is again a live MultiXactId, someone tries to lock or delete it.
		 * Even then, another share-lock attempt would work fine.  An
		 * exclusive-lock or delete attempt would face unexpected delay, or
		 * in the very worst case get a deadlock error.  This seems an
		 * extremely low-probability scenario with minimal downside even if
		 * it does happen, so for now we don't do the extra bookkeeping that
		 * would be needed to clean out MultiXactIds.
		 *----------
		 */
	}

	/*
	 * Although xvac per se could only be set by old-style VACUUM FULL, it
	 * shares physical storage space with cmax, and so could be wiped out by
	 * someone setting xmax.  Hence recheck after changing lock, same as for
	 * xmax itself.
	 *
	 * Old-style VACUUM FULL is gone, but we have to keep this code as long as
	 * we support having MOVED_OFF/MOVED_IN tuples in the database.
	 */
recheck_xvac:
	if (tuple->t_infomask & HEAP_MOVED)
	{
		xid = HeapTupleHeaderGetXvac(tuple);
		if (TransactionIdIsNormal(xid) &&
			TransactionIdPrecedes(xid, cutoff_xid))
		{
			if (buf != InvalidBuffer)
			{
				/* trade in share lock for exclusive lock */
				LockBuffer(buf, BUFFER_LOCK_UNLOCK);
				LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);
				buf = InvalidBuffer;
				goto recheck_xvac;		/* see comment above */
			}

			/*
			 * If a MOVED_OFF tuple is not dead, the xvac transaction must
			 * have failed; whereas a non-dead MOVED_IN tuple must mean the
			 * xvac transaction succeeded.
			 */
			if (tuple->t_infomask & HEAP_MOVED_OFF)
				HeapTupleHeaderSetXvac(tuple, InvalidTransactionId);
			else
				HeapTupleHeaderSetXvac(tuple, FrozenTransactionId);

			/*
			 * Might as well fix the hint bits too; usually XMIN_COMMITTED
			 * will already be set here, but there's a small chance not.
			 */
			Assert(!(tuple->t_infomask & HEAP_XMIN_INVALID));
			tuple->t_infomask |= HEAP_XMIN_COMMITTED;
			changed = true;
		}
	}

	return changed;
}


/* ----------------
 *		heap_markpos	- mark scan position
 * ----------------
 */
void
heap_markpos(HeapScanDesc scan)
{
	/* Note: no locking manipulations needed */

	if (scan->rs_ctup.t_data != NULL)
	{
		scan->rs_mctid = scan->rs_ctup.t_self;
		if (scan->rs_pageatatime)
			scan->rs_mindex = scan->rs_cindex;
	}
	else
		ItemPointerSetInvalid(&scan->rs_mctid);
}

/* ----------------
 *		heap_restrpos	- restore position to marked location
 * ----------------
 */
void
heap_restrpos(HeapScanDesc scan)
{
	/* XXX no amrestrpos checking that ammarkpos called */

	if (!ItemPointerIsValid(&scan->rs_mctid))
	{
		scan->rs_ctup.t_data = NULL;

		/*
		 * unpin scan buffers
		 */
		if (BufferIsValid(scan->rs_cbuf))
			ReleaseBuffer(scan->rs_cbuf);
		scan->rs_cbuf = InvalidBuffer;
		scan->rs_cblock = InvalidBlockNumber;
		scan->rs_inited = false;
	}
	else
	{
		/*
		 * If we reached end of scan, rs_inited will now be false.	We must
		 * reset it to true to keep heapgettup from doing the wrong thing.
		 */
		scan->rs_inited = true;
		scan->rs_ctup.t_self = scan->rs_mctid;
		if (scan->rs_pageatatime)
		{
			scan->rs_cindex = scan->rs_mindex;
			heapgettup_pagemode(scan,
								NoMovementScanDirection,
								0,		/* needn't recheck scan keys */
								NULL);
		}
		else
			heapgettup(scan,
					   NoMovementScanDirection,
					   0,		/* needn't recheck scan keys */
					   NULL);
	}
}

/*
 * If 'tuple' contains any visible XID greater than latestRemovedXid,
 * ratchet forwards latestRemovedXid to the greatest one found.
 * This is used as the basis for generating Hot Standby conflicts, so
 * if a tuple was never visible then removing it should not conflict
 * with queries.
 */
void
HeapTupleHeaderAdvanceLatestRemovedXid(HeapTupleHeader tuple,
									   TransactionId *latestRemovedXid)
{
	TransactionId xmin = HeapTupleHeaderGetXmin(tuple);
	TransactionId xmax = HeapTupleHeaderGetXmax(tuple);
	TransactionId xvac = HeapTupleHeaderGetXvac(tuple);

	if (tuple->t_infomask & HEAP_MOVED)
	{
		if (TransactionIdPrecedes(*latestRemovedXid, xvac))
			*latestRemovedXid = xvac;
	}

	/*
	 * Ignore tuples inserted by an aborted transaction or if the tuple was
	 * updated/deleted by the inserting transaction.
	 *
	 * Look for a committed hint bit, or if no xmin bit is set, check clog.
	 * This needs to work on both master and standby, where it is used to
	 * assess btree delete records.
	 */
	if ((tuple->t_infomask & HEAP_XMIN_COMMITTED) ||
		(!(tuple->t_infomask & HEAP_XMIN_COMMITTED) &&
		 !(tuple->t_infomask & HEAP_XMIN_INVALID) &&
		 TransactionIdDidCommit(xmin)))
	{
		if (xmax != xmin &&
			TransactionIdFollows(xmax, *latestRemovedXid))
			*latestRemovedXid = xmax;
	}

	/* *latestRemovedXid may still be invalid at end */
}

/*
 * Perform XLogInsert to register a heap cleanup info message. These
 * messages are sent once per VACUUM and are required because
 * of the phasing of removal operations during a lazy VACUUM.
 * see comments for vacuum_log_cleanup_info().
 */
XLogRecPtr
log_heap_cleanup_info(RelFileNode rnode, TransactionId latestRemovedXid)
{
	xl_heap_cleanup_info xlrec;
	XLogRecPtr	recptr;
	XLogRecData rdata;

	xlrec.node = rnode;
	xlrec.latestRemovedXid = latestRemovedXid;

	rdata.data = (char *) &xlrec;
	rdata.len = SizeOfHeapCleanupInfo;
	rdata.buffer = InvalidBuffer;
	rdata.next = NULL;

	recptr = XLogInsert(RM_HEAP2_ID, XLOG_HEAP2_CLEANUP_INFO, &rdata);

	return recptr;
}

/*
 * Perform XLogInsert for a heap-clean operation.  Caller must already
 * have modified the buffer and marked it dirty.
 *
 * Note: prior to Postgres 8.3, the entries in the nowunused[] array were
 * zero-based tuple indexes.  Now they are one-based like other uses
 * of OffsetNumber.
 *
 * We also include latestRemovedXid, which is the greatest XID present in
 * the removed tuples. That allows recovery processing to cancel or wait
 * for long standby queries that can still see these tuples.
 */
XLogRecPtr
log_heap_clean(Relation reln, Buffer buffer,
			   OffsetNumber *redirected, int nredirected,
			   OffsetNumber *nowdead, int ndead,
			   OffsetNumber *nowunused, int nunused,
			   TransactionId latestRemovedXid)
{
	xl_heap_clean xlrec;
	uint8		info;
	XLogRecPtr	recptr;
	XLogRecData rdata[4];

	/* Caller should not call me on a non-WAL-logged relation */
	Assert(RelationNeedsWAL(reln));

	xlrec.node = reln->rd_node;
	xlrec.block = BufferGetBlockNumber(buffer);
	xlrec.latestRemovedXid = latestRemovedXid;
	xlrec.nredirected = nredirected;
	xlrec.ndead = ndead;

	rdata[0].data = (char *) &xlrec;
	rdata[0].len = SizeOfHeapClean;
	rdata[0].buffer = InvalidBuffer;
	rdata[0].next = &(rdata[1]);

	/*
	 * The OffsetNumber arrays are not actually in the buffer, but we pretend
	 * that they are.  When XLogInsert stores the whole buffer, the offset
	 * arrays need not be stored too.  Note that even if all three arrays are
	 * empty, we want to expose the buffer as a candidate for whole-page
	 * storage, since this record type implies a defragmentation operation
	 * even if no item pointers changed state.
	 */
	if (nredirected > 0)
	{
		rdata[1].data = (char *) redirected;
		rdata[1].len = nredirected * sizeof(OffsetNumber) * 2;
	}
	else
	{
		rdata[1].data = NULL;
		rdata[1].len = 0;
	}
	rdata[1].buffer = buffer;
	rdata[1].buffer_std = true;
	rdata[1].next = &(rdata[2]);

	if (ndead > 0)
	{
		rdata[2].data = (char *) nowdead;
		rdata[2].len = ndead * sizeof(OffsetNumber);
	}
	else
	{
		rdata[2].data = NULL;
		rdata[2].len = 0;
	}
	rdata[2].buffer = buffer;
	rdata[2].buffer_std = true;
	rdata[2].next = &(rdata[3]);

	if (nunused > 0)
	{
		rdata[3].data = (char *) nowunused;
		rdata[3].len = nunused * sizeof(OffsetNumber);
	}
	else
	{
		rdata[3].data = NULL;
		rdata[3].len = 0;
	}
	rdata[3].buffer = buffer;
	rdata[3].buffer_std = true;
	rdata[3].next = NULL;

	info = XLOG_HEAP2_CLEAN;
	recptr = XLogInsert(RM_HEAP2_ID, info, rdata);

	return recptr;
}

/*
 * Perform XLogInsert for a heap-freeze operation.	Caller must already
 * have modified the buffer and marked it dirty.
 */
XLogRecPtr
log_heap_freeze(Relation reln, Buffer buffer,
				TransactionId cutoff_xid,
				OffsetNumber *offsets, int offcnt)
{
	xl_heap_freeze xlrec;
	XLogRecPtr	recptr;
	XLogRecData rdata[2];

	/* Caller should not call me on a non-WAL-logged relation */
	Assert(RelationNeedsWAL(reln));
	/* nor when there are no tuples to freeze */
	Assert(offcnt > 0);

	xlrec.node = reln->rd_node;
	xlrec.block = BufferGetBlockNumber(buffer);
	xlrec.cutoff_xid = cutoff_xid;

	rdata[0].data = (char *) &xlrec;
	rdata[0].len = SizeOfHeapFreeze;
	rdata[0].buffer = InvalidBuffer;
	rdata[0].next = &(rdata[1]);

	/*
	 * The tuple-offsets array is not actually in the buffer, but pretend that
	 * it is.  When XLogInsert stores the whole buffer, the offsets array need
	 * not be stored too.
	 */
	rdata[1].data = (char *) offsets;
	rdata[1].len = offcnt * sizeof(OffsetNumber);
	rdata[1].buffer = buffer;
	rdata[1].buffer_std = true;
	rdata[1].next = NULL;

	recptr = XLogInsert(RM_HEAP2_ID, XLOG_HEAP2_FREEZE, rdata);

	return recptr;
}

/*
 * Perform XLogInsert for a heap-update operation.	Caller must already
 * have modified the buffer(s) and marked them dirty.
 */
static XLogRecPtr
log_heap_update(Relation reln, Buffer oldbuf, ItemPointerData from,
				Buffer newbuf, HeapTuple newtup,
				bool all_visible_cleared, bool new_all_visible_cleared)
{
	xl_heap_update xlrec;
	xl_heap_header xlhdr;
	uint8		info;
	XLogRecPtr	recptr;
	XLogRecData rdata[4];
	Page		page = BufferGetPage(newbuf);

	/* Caller should not call me on a non-WAL-logged relation */
	Assert(RelationNeedsWAL(reln));

	if (HeapTupleIsHeapOnly(newtup))
		info = XLOG_HEAP_HOT_UPDATE;
	else
		info = XLOG_HEAP_UPDATE;

	xlrec.target.node = reln->rd_node;
	xlrec.target.tid = from;
	xlrec.all_visible_cleared = all_visible_cleared;
	xlrec.newtid = newtup->t_self;
	xlrec.new_all_visible_cleared = new_all_visible_cleared;

	rdata[0].data = (char *) &xlrec;
	rdata[0].len = SizeOfHeapUpdate;
	rdata[0].buffer = InvalidBuffer;
	rdata[0].next = &(rdata[1]);

	rdata[1].data = NULL;
	rdata[1].len = 0;
	rdata[1].buffer = oldbuf;
	rdata[1].buffer_std = true;
	rdata[1].next = &(rdata[2]);

	xlhdr.t_infomask2 = newtup->t_data->t_infomask2;
	xlhdr.t_infomask = newtup->t_data->t_infomask;
	xlhdr.t_hoff = newtup->t_data->t_hoff;

	/*
	 * As with insert records, we need not store the rdata[2] segment if we
	 * decide to store the whole buffer instead.
	 */
	rdata[2].data = (char *) &xlhdr;
	rdata[2].len = SizeOfHeapHeader;
	rdata[2].buffer = newbuf;
	rdata[2].buffer_std = true;
	rdata[2].next = &(rdata[3]);

	/* PG73FORMAT: write bitmap [+ padding] [+ oid] + data */
	rdata[3].data = (char *) newtup->t_data + offsetof(HeapTupleHeaderData, t_bits);
	rdata[3].len = newtup->t_len - offsetof(HeapTupleHeaderData, t_bits);
	rdata[3].buffer = newbuf;
	rdata[3].buffer_std = true;
	rdata[3].next = NULL;

	/* If new tuple is the single and first tuple on page... */
	if (ItemPointerGetOffsetNumber(&(newtup->t_self)) == FirstOffsetNumber &&
		PageGetMaxOffsetNumber(page) == FirstOffsetNumber)
	{
		info |= XLOG_HEAP_INIT_PAGE;
		rdata[2].buffer = rdata[3].buffer = InvalidBuffer;
	}

	recptr = XLogInsert(RM_HEAP_ID, info, rdata);

	return recptr;
}

/*
 * Perform XLogInsert of a HEAP_NEWPAGE record to WAL. Caller is responsible
 * for writing the page to disk after calling this routine.
 *
 * Note: all current callers build pages in private memory and write them
 * directly to smgr, rather than using bufmgr.	Therefore there is no need
 * to pass a buffer ID to XLogInsert, nor to perform MarkBufferDirty within
 * the critical section.
 *
 * Note: the NEWPAGE log record is used for both heaps and indexes, so do
 * not do anything that assumes we are touching a heap.
 */
XLogRecPtr
log_newpage(RelFileNode *rnode, ForkNumber forkNum, BlockNumber blkno,
			Page page)
{
	xl_heap_newpage xlrec;
	XLogRecPtr	recptr;
	XLogRecData rdata[2];

	/* NO ELOG(ERROR) from here till newpage op is logged */
	START_CRIT_SECTION();

	xlrec.node = *rnode;
	xlrec.forknum = forkNum;
	xlrec.blkno = blkno;

	rdata[0].data = (char *) &xlrec;
	rdata[0].len = SizeOfHeapNewpage;
	rdata[0].buffer = InvalidBuffer;
	rdata[0].next = &(rdata[1]);

	rdata[1].data = (char *) page;
	rdata[1].len = BLCKSZ;
	rdata[1].buffer = InvalidBuffer;
	rdata[1].next = NULL;

	recptr = XLogInsert(RM_HEAP_ID, XLOG_HEAP_NEWPAGE, rdata);

	/*
	 * The page may be uninitialized. If so, we can't set the LSN and TLI
	 * because that would corrupt the page.
	 */
	if (!PageIsNew(page))
	{
		PageSetLSN(page, recptr);
		PageSetTLI(page, ThisTimeLineID);
	}

	END_CRIT_SECTION();

	return recptr;
}

/*
 * Handles CLEANUP_INFO
 */
static void
heap_xlog_cleanup_info(XLogRecPtr lsn, XLogRecord *record)
{
	xl_heap_cleanup_info *xlrec = (xl_heap_cleanup_info *) XLogRecGetData(record);

	if (InHotStandby)
		ResolveRecoveryConflictWithSnapshot(xlrec->latestRemovedXid, xlrec->node);

	/*
	 * Actual operation is a no-op. Record type exists to provide a means for
	 * conflict processing to occur before we begin index vacuum actions. see
	 * vacuumlazy.c and also comments in btvacuumpage()
	 */
}

/*
 * Handles HEAP2_CLEAN record type
 */
static void
heap_xlog_clean(XLogRecPtr lsn, XLogRecord *record)
{
	xl_heap_clean *xlrec = (xl_heap_clean *) XLogRecGetData(record);
	Buffer		buffer;
	Page		page;
	OffsetNumber *end;
	OffsetNumber *redirected;
	OffsetNumber *nowdead;
	OffsetNumber *nowunused;
	int			nredirected;
	int			ndead;
	int			nunused;
	Size		freespace;

	/*
	 * We're about to remove tuples. In Hot Standby mode, ensure that there's
	 * no queries running for which the removed tuples are still visible.
	 *
	 * Not all HEAP2_CLEAN records remove tuples with xids, so we only want to
	 * conflict on the records that cause MVCC failures for user queries. If
	 * latestRemovedXid is invalid, skip conflict processing.
	 */
	if (InHotStandby && TransactionIdIsValid(xlrec->latestRemovedXid))
		ResolveRecoveryConflictWithSnapshot(xlrec->latestRemovedXid,
											xlrec->node);

	RestoreBkpBlocks(lsn, record, true);

	if (record->xl_info & XLR_BKP_BLOCK_1)
		return;

	buffer = XLogReadBufferExtended(xlrec->node, MAIN_FORKNUM, xlrec->block, RBM_NORMAL);
	if (!BufferIsValid(buffer))
		return;
	LockBufferForCleanup(buffer);
	page = (Page) BufferGetPage(buffer);

	if (XLByteLE(lsn, PageGetLSN(page)))
	{
		UnlockReleaseBuffer(buffer);
		return;
	}

	nredirected = xlrec->nredirected;
	ndead = xlrec->ndead;
	end = (OffsetNumber *) ((char *) xlrec + record->xl_len);
	redirected = (OffsetNumber *) ((char *) xlrec + SizeOfHeapClean);
	nowdead = redirected + (nredirected * 2);
	nowunused = nowdead + ndead;
	nunused = (int)(end - nowunused);
	Assert(nunused >= 0);

	/* Update all item pointers per the record, and repair fragmentation */
	heap_page_prune_execute(buffer,
							redirected, nredirected,
							nowdead, ndead,
							nowunused, nunused);

	freespace = PageGetHeapFreeSpace(page);		/* needed to update FSM below */

	/*
	 * Note: we don't worry about updating the page's prunability hints. At
	 * worst this will cause an extra prune cycle to occur soon.
	 */

	PageSetLSN(page, lsn);
	PageSetTLI(page, ThisTimeLineID);
	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);

	/*
	 * Update the FSM as well.
	 *
	 * XXX: We don't get here if the page was restored from full page image.
	 * We don't bother to update the FSM in that case, it doesn't need to be
	 * totally accurate anyway.
	 */
	XLogRecordPageWithFreeSpace(xlrec->node, xlrec->block, freespace);
}

static void
heap_xlog_freeze(XLogRecPtr lsn, XLogRecord *record)
{
	xl_heap_freeze *xlrec = (xl_heap_freeze *) XLogRecGetData(record);
	TransactionId cutoff_xid = xlrec->cutoff_xid;
	Buffer		buffer;
	Page		page;

	/*
	 * In Hot Standby mode, ensure that there's no queries running which still
	 * consider the frozen xids as running.
	 */
	if (InHotStandby)
		ResolveRecoveryConflictWithSnapshot(cutoff_xid, xlrec->node);

	RestoreBkpBlocks(lsn, record, false);

	if (record->xl_info & XLR_BKP_BLOCK_1)
		return;

	buffer = XLogReadBufferExtended(xlrec->node, MAIN_FORKNUM, xlrec->block, RBM_NORMAL);
	if (!BufferIsValid(buffer))
		return;
	LockBufferForCleanup(buffer);
	page = (Page) BufferGetPage(buffer);

	if (XLByteLE(lsn, PageGetLSN(page)))
	{
		UnlockReleaseBuffer(buffer);
		return;
	}

	if (record->xl_len > SizeOfHeapFreeze)
	{
		OffsetNumber *offsets;
		OffsetNumber *offsets_end;

		offsets = (OffsetNumber *) ((char *) xlrec + SizeOfHeapFreeze);
		offsets_end = (OffsetNumber *) ((char *) xlrec + record->xl_len);

		while (offsets < offsets_end)
		{
			/* offsets[] entries are one-based */
			ItemId		lp = PageGetItemId(page, *offsets);
			HeapTupleHeader tuple = (HeapTupleHeader) PageGetItem(page, lp);

			(void) heap_freeze_tuple(tuple, cutoff_xid, InvalidBuffer);
			offsets++;
		}
	}

	PageSetLSN(page, lsn);
	PageSetTLI(page, ThisTimeLineID);
	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);
}

static void
heap_xlog_newpage(XLogRecPtr lsn, XLogRecord *record)
{
	xl_heap_newpage *xlrec = (xl_heap_newpage *) XLogRecGetData(record);
	Buffer		buffer;
	Page		page;

	/*
	 * Note: the NEWPAGE log record is used for both heaps and indexes, so do
	 * not do anything that assumes we are touching a heap.
	 */
	buffer = XLogReadBufferExtended(xlrec->node, xlrec->forknum, xlrec->blkno,
									RBM_ZERO);
	Assert(BufferIsValid(buffer));
	LockBuffer(buffer, BUFFER_LOCK_EXCLUSIVE);
	page = (Page) BufferGetPage(buffer);

	Assert(record->xl_len == SizeOfHeapNewpage + BLCKSZ);
	memcpy(page, (char *) xlrec + SizeOfHeapNewpage, BLCKSZ);

	/*
	 * The page may be uninitialized. If so, we can't set the LSN and TLI
	 * because that would corrupt the page.
	 */
	if (!PageIsNew(page))
	{
		PageSetLSN(page, lsn);
		PageSetTLI(page, ThisTimeLineID);
	}

	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);
}

static void
heap_xlog_delete(XLogRecPtr lsn, XLogRecord *record)
{
	xl_heap_delete *xlrec = (xl_heap_delete *) XLogRecGetData(record);
	Buffer		buffer;
	Page		page;
	OffsetNumber offnum;
	ItemId		lp = NULL;
	HeapTupleHeader htup;
	BlockNumber blkno;

	blkno = ItemPointerGetBlockNumber(&(xlrec->target.tid));

	/*
	 * The visibility map may need to be fixed even if the heap page is
	 * already up-to-date.
	 */
	if (xlrec->all_visible_cleared)
	{
		Relation	reln = CreateFakeRelcacheEntry(xlrec->target.node);

		visibilitymap_clear(reln, blkno);
		FreeFakeRelcacheEntry(reln);
	}

	if (record->xl_info & XLR_BKP_BLOCK_1)
		return;

	buffer = XLogReadBuffer(xlrec->target.node, blkno, false);
	if (!BufferIsValid(buffer))
		return;
	page = (Page) BufferGetPage(buffer);

	if (XLByteLE(lsn, PageGetLSN(page)))		/* changes are applied */
	{
		UnlockReleaseBuffer(buffer);
		return;
	}

	offnum = ItemPointerGetOffsetNumber(&(xlrec->target.tid));
	if (PageGetMaxOffsetNumber(page) >= offnum)
		lp = PageGetItemId(page, offnum);

	if (PageGetMaxOffsetNumber(page) < offnum || !ItemIdIsNormal(lp))
		ereport(PANIC,
                (errcode(ERRCODE_HEAP_ERROR),
                errmsg("heap_delete_redo: invalid lp")));

	htup = (HeapTupleHeader) PageGetItem(page, lp);

	htup->t_infomask &= ~(HEAP_XMAX_COMMITTED |
						  HEAP_XMAX_INVALID |
						  HEAP_XMAX_IS_MULTI |
						  HEAP_IS_LOCKED |
						  HEAP_MOVED);
	HeapTupleHeaderClearHotUpdated(htup);
	HeapTupleHeaderSetXmax(htup, record->xl_xid);
	HeapTupleHeaderSetCmax(htup, FirstCommandId, false);

	/* Mark the page as a candidate for pruning */
	PageSetPrunable(page, record->xl_xid);

	if (xlrec->all_visible_cleared)
		PageClearAllVisible(page);

	/* Make sure there is no forward chain link in t_ctid */
	htup->t_ctid = xlrec->target.tid;
	PageSetLSN(page, lsn);
	PageSetTLI(page, ThisTimeLineID);
	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);
}

static void
heap_xlog_insert(XLogRecPtr lsn, XLogRecord *record)
{
	xl_heap_insert *xlrec = (xl_heap_insert *) XLogRecGetData(record);
	Buffer		buffer;
	Page		page;
	OffsetNumber offnum;
	struct
	{
		HeapTupleHeaderData hdr;
		char		data[MaxHeapTupleSize];
	}			tbuf;
	HeapTupleHeader htup;
	xl_heap_header xlhdr;
	uint32		newlen;
	Size		freespace;
	BlockNumber blkno;

	blkno = ItemPointerGetBlockNumber(&(xlrec->target.tid));

	/*
	 * The visibility map may need to be fixed even if the heap page is
	 * already up-to-date.
	 */
	if (xlrec->all_visible_cleared)
	{
		Relation	reln = CreateFakeRelcacheEntry(xlrec->target.node);

		visibilitymap_clear(reln, blkno);
		FreeFakeRelcacheEntry(reln);
	}

	if (record->xl_info & XLR_BKP_BLOCK_1)
		return;

	if (record->xl_info & XLOG_HEAP_INIT_PAGE)
	{
		buffer = XLogReadBuffer(xlrec->target.node, blkno, true);
		Assert(BufferIsValid(buffer));
		page = (Page) BufferGetPage(buffer);

		PageInit(page, BufferGetPageSize(buffer), 0);
	}
	else
	{
		buffer = XLogReadBuffer(xlrec->target.node, blkno, false);
		if (!BufferIsValid(buffer))
			return;
		page = (Page) BufferGetPage(buffer);

		if (XLByteLE(lsn, PageGetLSN(page)))	/* changes are applied */
		{
			UnlockReleaseBuffer(buffer);
			return;
		}
	}

	offnum = ItemPointerGetOffsetNumber(&(xlrec->target.tid));
	if (PageGetMaxOffsetNumber(page) + 1 < offnum)
		ereport(PANIC,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg("heap_insert_redo: invalid max offset number")));

	newlen = record->xl_len - SizeOfHeapInsert - SizeOfHeapHeader;
	Assert(newlen <= MaxHeapTupleSize);
	memcpy((char *) &xlhdr,
		   (char *) xlrec + SizeOfHeapInsert,
		   SizeOfHeapHeader);
	htup = &tbuf.hdr;
	MemSet((char *) htup, 0, sizeof(HeapTupleHeaderData));
	/* PG73FORMAT: get bitmap [+ padding] [+ oid] + data */
	memcpy((char *) htup + offsetof(HeapTupleHeaderData, t_bits),
		   (char *) xlrec + SizeOfHeapInsert + SizeOfHeapHeader,
		   newlen);
	newlen += offsetof(HeapTupleHeaderData, t_bits);
	htup->t_infomask2 = xlhdr.t_infomask2;
	htup->t_infomask = xlhdr.t_infomask;
	htup->t_hoff = xlhdr.t_hoff;
	HeapTupleHeaderSetXmin(htup, record->xl_xid);
	HeapTupleHeaderSetCmin(htup, FirstCommandId);
	htup->t_ctid = xlrec->target.tid;

	offnum = PageAddItem(page, (Item) htup, newlen, offnum, true, true);
	if (offnum == InvalidOffsetNumber)
		ereport(PANIC,
                (errcode(ERRCODE_DATA_EXCEPTION),
                errmsg("heap_insert_redo: failed to add tuple")));

	freespace = PageGetHeapFreeSpace(page);		/* needed to update FSM below */

	PageSetLSN(page, lsn);
	PageSetTLI(page, ThisTimeLineID);

	if (xlrec->all_visible_cleared)
		PageClearAllVisible(page);

	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);

	/*
	 * If the page is running low on free space, update the FSM as well.
	 * Arbitrarily, our definition of "low" is less than 20%. We can't do much
	 * better than that without knowing the fill-factor for the table.
	 *
	 * XXX: We don't get here if the page was restored from full page image.
	 * We don't bother to update the FSM in that case, it doesn't need to be
	 * totally accurate anyway.
	 */
	if (freespace < BLCKSZ / 5)
		XLogRecordPageWithFreeSpace(xlrec->target.node, blkno, freespace);
}

/*
 * Handles UPDATE and HOT_UPDATE
 */
static void
heap_xlog_update(XLogRecPtr lsn, XLogRecord *record, bool hot_update)
{
	xl_heap_update *xlrec = (xl_heap_update *) XLogRecGetData(record);
	Buffer		buffer;
	bool		samepage = (ItemPointerGetBlockNumber(&(xlrec->newtid)) ==
							ItemPointerGetBlockNumber(&(xlrec->target.tid)));
	Page		page;
	OffsetNumber offnum;
	ItemId		lp = NULL;
	HeapTupleHeader htup;
	struct
	{
		HeapTupleHeaderData hdr;
		char		data[MaxHeapTupleSize];
	}			tbuf;
	xl_heap_header xlhdr;
	int			hsize;
	uint32		newlen;
	Size		freespace;

	/*
	 * The visibility map may need to be fixed even if the heap page is
	 * already up-to-date.
	 */
	if (xlrec->all_visible_cleared)
	{
		Relation	reln = CreateFakeRelcacheEntry(xlrec->target.node);

		visibilitymap_clear(reln,
							ItemPointerGetBlockNumber(&xlrec->target.tid));
		FreeFakeRelcacheEntry(reln);
	}

	if (record->xl_info & XLR_BKP_BLOCK_1)
	{
		if (samepage)
			return;				/* backup block covered both changes */
		goto newt;
	}

	/* Deal with old tuple version */

	buffer = XLogReadBuffer(xlrec->target.node,
							ItemPointerGetBlockNumber(&(xlrec->target.tid)),
							false);
	if (!BufferIsValid(buffer))
		goto newt;
	page = (Page) BufferGetPage(buffer);

	if (XLByteLE(lsn, PageGetLSN(page)))		/* changes are applied */
	{
		UnlockReleaseBuffer(buffer);
		if (samepage)
			return;
		goto newt;
	}

	offnum = ItemPointerGetOffsetNumber(&(xlrec->target.tid));
	if (PageGetMaxOffsetNumber(page) >= offnum)
		lp = PageGetItemId(page, offnum);

	if (PageGetMaxOffsetNumber(page) < offnum || !ItemIdIsNormal(lp))
		ereport(PANIC,
                (errcode(ERRCODE_HEAP_ERROR),
                errmsg("heap_update_redo: invalid lp")));

	htup = (HeapTupleHeader) PageGetItem(page, lp);

	htup->t_infomask &= ~(HEAP_XMAX_COMMITTED |
						  HEAP_XMAX_INVALID |
						  HEAP_XMAX_IS_MULTI |
						  HEAP_IS_LOCKED |
						  HEAP_MOVED);
	if (hot_update)
		HeapTupleHeaderSetHotUpdated(htup);
	else
		HeapTupleHeaderClearHotUpdated(htup);
	HeapTupleHeaderSetXmax(htup, record->xl_xid);
	HeapTupleHeaderSetCmax(htup, FirstCommandId, false);
	/* Set forward chain link in t_ctid */
	htup->t_ctid = xlrec->newtid;

	/* Mark the page as a candidate for pruning */
	PageSetPrunable(page, record->xl_xid);

	if (xlrec->all_visible_cleared)
		PageClearAllVisible(page);

	/*
	 * this test is ugly, but necessary to avoid thinking that insert change
	 * is already applied
	 */
	if (samepage)
		goto newsame;
	PageSetLSN(page, lsn);
	PageSetTLI(page, ThisTimeLineID);
	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);

	/* Deal with new tuple */

newt:;

	/*
	 * The visibility map may need to be fixed even if the heap page is
	 * already up-to-date.
	 */
	if (xlrec->new_all_visible_cleared)
	{
		Relation	reln = CreateFakeRelcacheEntry(xlrec->target.node);

		visibilitymap_clear(reln, ItemPointerGetBlockNumber(&xlrec->newtid));
		FreeFakeRelcacheEntry(reln);
	}

	if (record->xl_info & XLR_BKP_BLOCK_2)
		return;

	if (record->xl_info & XLOG_HEAP_INIT_PAGE)
	{
		buffer = XLogReadBuffer(xlrec->target.node,
								ItemPointerGetBlockNumber(&(xlrec->newtid)),
								true);
		Assert(BufferIsValid(buffer));
		page = (Page) BufferGetPage(buffer);

		PageInit(page, BufferGetPageSize(buffer), 0);
	}
	else
	{
		buffer = XLogReadBuffer(xlrec->target.node,
								ItemPointerGetBlockNumber(&(xlrec->newtid)),
								false);
		if (!BufferIsValid(buffer))
			return;
		page = (Page) BufferGetPage(buffer);

		if (XLByteLE(lsn, PageGetLSN(page)))	/* changes are applied */
		{
			UnlockReleaseBuffer(buffer);
			return;
		}
	}

newsame:;

	offnum = ItemPointerGetOffsetNumber(&(xlrec->newtid));
	if (PageGetMaxOffsetNumber(page) + 1 < offnum)
		ereport(PANIC,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg("heap_update_redo: invalid max offset number")));

	hsize = SizeOfHeapUpdate + SizeOfHeapHeader;

	newlen = record->xl_len - hsize;
	Assert(newlen <= MaxHeapTupleSize);
	memcpy((char *) &xlhdr,
		   (char *) xlrec + SizeOfHeapUpdate,
		   SizeOfHeapHeader);
	htup = &tbuf.hdr;
	MemSet((char *) htup, 0, sizeof(HeapTupleHeaderData));
	/* PG73FORMAT: get bitmap [+ padding] [+ oid] + data */
	memcpy((char *) htup + offsetof(HeapTupleHeaderData, t_bits),
		   (char *) xlrec + hsize,
		   newlen);
	newlen += offsetof(HeapTupleHeaderData, t_bits);
	htup->t_infomask2 = xlhdr.t_infomask2;
	htup->t_infomask = xlhdr.t_infomask;
	htup->t_hoff = xlhdr.t_hoff;

	HeapTupleHeaderSetXmin(htup, record->xl_xid);
	HeapTupleHeaderSetCmin(htup, FirstCommandId);
	/* Make sure there is no forward chain link in t_ctid */
	htup->t_ctid = xlrec->newtid;

	offnum = PageAddItem(page, (Item) htup, newlen, offnum, true, true);
	if (offnum == InvalidOffsetNumber)
		ereport(PANIC,
                (errcode(ERRCODE_HEAP_ERROR),
                errmsg("heap_update_redo: failed to add tuple")));

	if (xlrec->new_all_visible_cleared)
		PageClearAllVisible(page);

	freespace = PageGetHeapFreeSpace(page);		/* needed to update FSM below */

	PageSetLSN(page, lsn);
	PageSetTLI(page, ThisTimeLineID);
	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);

	/*
	 * If the page is running low on free space, update the FSM as well.
	 * Arbitrarily, our definition of "low" is less than 20%. We can't do much
	 * better than that without knowing the fill-factor for the table.
	 *
	 * However, don't update the FSM on HOT updates, because after crash
	 * recovery, either the old or the new tuple will certainly be dead and
	 * prunable. After pruning, the page will have roughly as much free space
	 * as it did before the update, assuming the new tuple is about the same
	 * size as the old one.
	 *
	 * XXX: We don't get here if the page was restored from full page image.
	 * We don't bother to update the FSM in that case, it doesn't need to be
	 * totally accurate anyway.
	 */
	if (!hot_update && freespace < BLCKSZ / 5)
		XLogRecordPageWithFreeSpace(xlrec->target.node,
					 ItemPointerGetBlockNumber(&(xlrec->newtid)), freespace);
}

static void
heap_xlog_lock(XLogRecPtr lsn, XLogRecord *record)
{
	xl_heap_lock *xlrec = (xl_heap_lock *) XLogRecGetData(record);
	Buffer		buffer;
	Page		page;
	OffsetNumber offnum;
	ItemId		lp = NULL;
	HeapTupleHeader htup;

	if (record->xl_info & XLR_BKP_BLOCK_1)
		return;

	buffer = XLogReadBuffer(xlrec->target.node,
							ItemPointerGetBlockNumber(&(xlrec->target.tid)),
							false);
	if (!BufferIsValid(buffer))
		return;
	page = (Page) BufferGetPage(buffer);

	if (XLByteLE(lsn, PageGetLSN(page)))		/* changes are applied */
	{
		UnlockReleaseBuffer(buffer);
		return;
	}

	offnum = ItemPointerGetOffsetNumber(&(xlrec->target.tid));
	if (PageGetMaxOffsetNumber(page) >= offnum)
		lp = PageGetItemId(page, offnum);

	if (PageGetMaxOffsetNumber(page) < offnum || !ItemIdIsNormal(lp))
		ereport(PANIC,
                (errcode(ERRCODE_HEAP_ERROR),
                errmsg("heap_lock_redo: invalid lp")));

	htup = (HeapTupleHeader) PageGetItem(page, lp);

	htup->t_infomask &= ~(HEAP_XMAX_COMMITTED |
						  HEAP_XMAX_INVALID |
						  HEAP_XMAX_IS_MULTI |
						  HEAP_IS_LOCKED |
						  HEAP_MOVED);
	if (xlrec->xid_is_mxact)
		htup->t_infomask |= HEAP_XMAX_IS_MULTI;
	if (xlrec->shared_lock)
		htup->t_infomask |= HEAP_XMAX_SHARED_LOCK;
	else
		htup->t_infomask |= HEAP_XMAX_EXCL_LOCK;
	HeapTupleHeaderClearHotUpdated(htup);
	HeapTupleHeaderSetXmax(htup, xlrec->locking_xid);
	HeapTupleHeaderSetCmax(htup, FirstCommandId, false);
	/* Make sure there is no forward chain link in t_ctid */
	htup->t_ctid = xlrec->target.tid;
	PageSetLSN(page, lsn);
	PageSetTLI(page, ThisTimeLineID);
	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);
}

static void
heap_xlog_inplace(XLogRecPtr lsn, XLogRecord *record)
{
	xl_heap_inplace *xlrec = (xl_heap_inplace *) XLogRecGetData(record);
	Buffer		buffer;
	Page		page;
	OffsetNumber offnum;
	ItemId		lp = NULL;
	HeapTupleHeader htup;
	uint32		oldlen;
	uint32		newlen;

	if (record->xl_info & XLR_BKP_BLOCK_1)
		return;

	buffer = XLogReadBuffer(xlrec->target.node,
							ItemPointerGetBlockNumber(&(xlrec->target.tid)),
							false);
	if (!BufferIsValid(buffer))
		return;
	page = (Page) BufferGetPage(buffer);

	if (XLByteLE(lsn, PageGetLSN(page)))		/* changes are applied */
	{
		UnlockReleaseBuffer(buffer);
		return;
	}

	offnum = ItemPointerGetOffsetNumber(&(xlrec->target.tid));
	if (PageGetMaxOffsetNumber(page) >= offnum)
		lp = PageGetItemId(page, offnum);

	if (PageGetMaxOffsetNumber(page) < offnum || !ItemIdIsNormal(lp))
		ereport(PANIC,
                (errcode(ERRCODE_HEAP_ERROR),
                errmsg("heap_inplace_redo: invalid lp")));

	htup = (HeapTupleHeader) PageGetItem(page, lp);

	oldlen = ItemIdGetLength(lp) - htup->t_hoff;
	newlen = record->xl_len - SizeOfHeapInplace;
	if (oldlen != newlen)
		ereport(PANIC,
                (errcode(ERRCODE_DATA_EXCEPTION),
                errmsg("heap_inplace_redo: wrong tuple length")));

	memcpy((char *) htup + htup->t_hoff,
		   (char *) xlrec + SizeOfHeapInplace,
		   newlen);

	PageSetLSN(page, lsn);
	PageSetTLI(page, ThisTimeLineID);
	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);
}

void
heap_redo(XLogRecPtr lsn, XLogRecord *record)
{
	uint8		info = record->xl_info & ~XLR_INFO_MASK;

	/*
	 * These operations don't overwrite MVCC data so no conflict processing is
	 * required. The ones in heap2 rmgr do.
	 */

	RestoreBkpBlocks(lsn, record, false);

	switch (info & XLOG_HEAP_OPMASK)
	{
		case XLOG_HEAP_INSERT:
			heap_xlog_insert(lsn, record);
			break;
		case XLOG_HEAP_DELETE:
			heap_xlog_delete(lsn, record);
			break;
		case XLOG_HEAP_UPDATE:
			heap_xlog_update(lsn, record, false);
			break;
		case XLOG_HEAP_HOT_UPDATE:
			heap_xlog_update(lsn, record, true);
			break;
		case XLOG_HEAP_NEWPAGE:
			heap_xlog_newpage(lsn, record);
			break;
		case XLOG_HEAP_LOCK:
			heap_xlog_lock(lsn, record);
			break;
		case XLOG_HEAP_INPLACE:
			heap_xlog_inplace(lsn, record);
			break;
		default:
			ereport(PANIC,
                    (errcode(ERRCODE_HEAP_ERROR),
                    errmsg("heap_redo: unknown op code %u", info)));
	}
}

void
heap2_redo(XLogRecPtr lsn, XLogRecord *record)
{
	uint8		info = record->xl_info & ~XLR_INFO_MASK;

	/*
	 * Note that RestoreBkpBlocks() is called after conflict processing within
	 * each record type handling function.
	 */

	switch (info & XLOG_HEAP_OPMASK)
	{
		case XLOG_HEAP2_FREEZE:
			heap_xlog_freeze(lsn, record);
			break;
		case XLOG_HEAP2_CLEAN:
			heap_xlog_clean(lsn, record);
			break;
		case XLOG_HEAP2_CLEANUP_INFO:
			heap_xlog_cleanup_info(lsn, record);
			break;
		case XLOG_HEAP2_MULTI_INSERT:
			heap_xlog_multi_insert(lsn, record);
			break;
		default:
			ereport(PANIC,
                    (errcode(ERRCODE_HEAP_ERROR),
                    errmsg("heap2_redo: unknown op code %u", info)));
	}
}

static void
out_target(StringInfo buf, xl_heaptid *target)
{
	appendStringInfo(buf, "rel %u/%u/%u; tid %u/%u",
			 target->node.spcNode, target->node.dbNode, target->node.relNode,
					 ItemPointerGetBlockNumber(&(target->tid)),
					 ItemPointerGetOffsetNumber(&(target->tid)));
}

void
heap_desc(StringInfo buf, uint8 xl_info, char *rec)
{
	uint8		info = xl_info & ~XLR_INFO_MASK;

	info &= XLOG_HEAP_OPMASK;
	if (info == XLOG_HEAP_INSERT)
	{
		xl_heap_insert *xlrec = (xl_heap_insert *) rec;

		if (xl_info & XLOG_HEAP_INIT_PAGE)
			appendStringInfo(buf, "insert(init): ");
		else
			appendStringInfo(buf, "insert: ");
		out_target(buf, &(xlrec->target));
	}
	else if (info == XLOG_HEAP_DELETE)
	{
		xl_heap_delete *xlrec = (xl_heap_delete *) rec;

		appendStringInfo(buf, "delete: ");
		out_target(buf, &(xlrec->target));
	}
	else if (info == XLOG_HEAP_UPDATE)
	{
		xl_heap_update *xlrec = (xl_heap_update *) rec;

		if (xl_info & XLOG_HEAP_INIT_PAGE)
			appendStringInfo(buf, "update(init): ");
		else
			appendStringInfo(buf, "update: ");
		out_target(buf, &(xlrec->target));
		appendStringInfo(buf, "; new %u/%u",
						 ItemPointerGetBlockNumber(&(xlrec->newtid)),
						 ItemPointerGetOffsetNumber(&(xlrec->newtid)));
	}
	else if (info == XLOG_HEAP_HOT_UPDATE)
	{
		xl_heap_update *xlrec = (xl_heap_update *) rec;

		if (xl_info & XLOG_HEAP_INIT_PAGE)		/* can this case happen? */
			appendStringInfo(buf, "hot_update(init): ");
		else
			appendStringInfo(buf, "hot_update: ");
		out_target(buf, &(xlrec->target));
		appendStringInfo(buf, "; new %u/%u",
						 ItemPointerGetBlockNumber(&(xlrec->newtid)),
						 ItemPointerGetOffsetNumber(&(xlrec->newtid)));
	}
	else if (info == XLOG_HEAP_NEWPAGE)
	{
		xl_heap_newpage *xlrec = (xl_heap_newpage *) rec;

		appendStringInfo(buf, "newpage: rel %u/%u/%u; fork %u, blk %u",
						 xlrec->node.spcNode, xlrec->node.dbNode,
						 xlrec->node.relNode, xlrec->forknum,
						 xlrec->blkno);
	}
	else if (info == XLOG_HEAP_LOCK)
	{
		xl_heap_lock *xlrec = (xl_heap_lock *) rec;

		if (xlrec->shared_lock)
			appendStringInfo(buf, "shared_lock: ");
		else
			appendStringInfo(buf, "exclusive_lock: ");
		if (xlrec->xid_is_mxact)
			appendStringInfo(buf, "mxid ");
		else
			appendStringInfo(buf, "xid ");
		appendStringInfo(buf, "%u ", xlrec->locking_xid);
		out_target(buf, &(xlrec->target));
	}
	else if (info == XLOG_HEAP_INPLACE)
	{
		xl_heap_inplace *xlrec = (xl_heap_inplace *) rec;

		appendStringInfo(buf, "inplace: ");
		out_target(buf, &(xlrec->target));
	}
	else
		appendStringInfo(buf, "UNKNOWN");
}

void
heap2_desc(StringInfo buf, uint8 xl_info, char *rec)
{
	uint8		info = xl_info & ~XLR_INFO_MASK;

	info &= XLOG_HEAP_OPMASK;
	if (info == XLOG_HEAP2_FREEZE)
	{
		xl_heap_freeze *xlrec = (xl_heap_freeze *) rec;

		appendStringInfo(buf, "freeze: rel %u/%u/%u; blk %u; cutoff %u",
						 xlrec->node.spcNode, xlrec->node.dbNode,
						 xlrec->node.relNode, xlrec->block,
						 xlrec->cutoff_xid);
	}
	else if (info == XLOG_HEAP2_CLEAN)
	{
		xl_heap_clean *xlrec = (xl_heap_clean *) rec;

		appendStringInfo(buf, "clean: rel %u/%u/%u; blk %u remxid %u",
						 xlrec->node.spcNode, xlrec->node.dbNode,
						 xlrec->node.relNode, xlrec->block,
						 xlrec->latestRemovedXid);
	}
	else if (info == XLOG_HEAP2_CLEANUP_INFO)
	{
		xl_heap_cleanup_info *xlrec = (xl_heap_cleanup_info *) rec;

		appendStringInfo(buf, "cleanup info: remxid %u",
						 xlrec->latestRemovedXid);
	}
	else
		appendStringInfo(buf, "UNKNOWN");
}

/*
 *	heap_sync		- sync a heap, for use when no WAL has been written
 *
 * This forces the heap contents (including TOAST heap if any) down to disk.
 * If we skipped using WAL, and WAL is otherwise needed, we must force the
 * relation down to disk before it's safe to commit the transaction.  This
 * requires writing out any dirty buffers and then doing a forced fsync.
 *
 * Indexes are not touched.  (Currently, index operations associated with
 * the commands that use this are WAL-logged and so do not need fsync.
 * That behavior might change someday, but in any case it's likely that
 * any fsync decisions required would be per-index and hence not appropriate
 * to be done here.)
 */
void
heap_sync(Relation rel)
{
	/* non-WAL-logged tables never need fsync */
	if (!RelationNeedsWAL(rel))
		return;

	/* main heap */
	FlushRelationBuffers(rel);
	/* FlushRelationBuffers will have opened rd_smgr */
	smgrimmedsync(rel->rd_smgr, MAIN_FORKNUM);

	/* FSM is not critical, don't bother syncing it */

	/* toast heap, if any */
	if (OidIsValid(rel->rd_rel->reltoastrelid))
	{
		Relation	toastrel;
		toastrel = heap_open(rel->rd_rel->reltoastrelid,AccessShareLock);
		FlushRelationBuffers(toastrel);
		smgrimmedsync(toastrel->rd_smgr, MAIN_FORKNUM);
		heap_close(toastrel, AccessShareLock);
	}
}


/*
 * Subroutine for heap_insert(). Prepares a tuple for insertion. This sets the
 * tuple header fields, assigns an OID, and toasts the tuple if necessary.
 * Returns a toasted version of the tuple if it was toasted, or the original
 * tuple if not. Note that in any case, the header fields are also set in
 * the original tuple.
 */
static HeapTuple
heap_prepare_insert(Relation relation, HeapTuple tup, TransactionId xid,
					CommandId cid, int options)
{
	if (relation->rd_rel->relhasoids)
	{
#ifdef NOT_USED
		/* this is redundant with an Assert in HeapTupleSetOid */
		Assert(tup->t_data->t_infomask & HEAP_HASOID);
#endif

		/*
		 * If the object id of this tuple has already been assigned, trust the
		 * caller.	There are a couple of ways this can happen.  At initial db
		 * creation, the backend program sets oids for tuples. When we define
		 * an index, we set the oid.  Finally, in the future, we may allow
		 * users to set their own object ids in order to support a persistent
		 * object store (objects need to contain pointers to one another).
		 */
		if (!OidIsValid(HeapTupleGetOid(tup)))
			HeapTupleSetOid(tup, GetNewOid(relation));
	}
	else
	{
		/* check there is not space for an OID */
		Assert(!(tup->t_data->t_infomask & HEAP_HASOID));
	}

	tup->t_data->t_infomask &= ~(HEAP_XACT_MASK);
	tup->t_data->t_infomask2 &= ~(HEAP2_XACT_MASK);
	tup->t_data->t_infomask |= HEAP_XMAX_INVALID;
	HeapTupleHeaderSetXmin(tup->t_data, xid);
	HeapTupleHeaderSetCmin(tup->t_data, cid);
	HeapTupleHeaderSetXmax(tup->t_data, 0);		/* for cleanliness */
	tup->t_tableOid = RelationGetRelid(relation);

	/*
	 * If the new tuple is too big for storage or contains already toasted
	 * out-of-line attributes from some other relation, invoke the toaster.
	 */
	if (relation->rel_kind != RELKIND_RELATION)
	{
		/* toast table entries should never be recursively toasted */
		Assert(!HeapTupleHasExternal(tup));
		return tup;
	}
	else if (HeapTupleHasExternal(tup) || tup->t_len > TOAST_TUPLE_THRESHOLD)
		return toast_insert_or_update(relation, tup, NULL, options);
	else
		return tup;
}

/*
 *	heap_multi_insert	- insert multiple tuple into a heap
 *
 * This is like heap_insert(), but inserts multiple tuples in one operation.
 * That's faster than calling heap_insert() in a loop, because when multiple
 * tuples can be inserted on a single page, we can write just a single WAL
 * record covering all of them, and only need to lock/unlock the page once.
 *
 * Note: this leaks memory into the current memory context. You can create a
 * temporary context before calling this, if that's a problem.
 */
void
heap_multi_insert(Relation relation, HeapTuple *tuples, int ntuples, BulkInsertState bistate)
{
#ifndef FOUNDER_XDB_SE
	TransactionId xid = GetCurrentTransactionId();
#else
	TransactionId xid = InvalidTransactionId;
	if(RelationIsTemp(relation))
	{
		xid = Fxdb_GetTmpCurrentTransactionId();
	} else
	{
		CheckStandbyUpdateError("Can not execute insert during recovery.");

		xid = GetCurrentTransactionId();
	}
#endif //FOUNDER_XDB_SE
	CommandId 	  cid = GetCurrentCommandId(true);
	HeapTuple  *heaptuples;
	int			i;
	int			ndone;
	char	   *scratch = NULL;
	Page		page;
	bool		needwal;
	Size		saveFreeSpace;

	needwal = RelationNeedsWAL(relation);
	saveFreeSpace = RelationGetTargetPageFreeSpace(relation,
												   HEAP_DEFAULT_FILLFACTOR);

	/* Toast and set header data in all the tuples */
	heaptuples = (HeapTuple*)palloc(ntuples * sizeof(HeapTuple));
	for (i = 0; i < ntuples; i++)
		heaptuples[i] = heap_prepare_insert(relation, tuples[i],
											xid, cid, 0);

	/*
	 * Allocate some memory to use for constructing the WAL record. Using
	 * palloc() within a critical section is not safe, so we allocate this
	 * beforehand.
	 */
	if (needwal)
		scratch = (char *)palloc(BLCKSZ);

	/*
	 * We're about to do the actual inserts -- but check for conflict first,
	 * to avoid possibly having to roll back work we've just done.
	 *
	 * For a heap insert, we only need to check for table-level SSI locks.
	 * Our new tuple can't possibly conflict with existing tuple locks, and
	 * heap page locks are only consolidated versions of tuple locks; they do
	 * not lock "gaps" as index page locks do.  So we don't need to identify
	 * a buffer before making the call.
	 */
	CheckForSerializableConflictIn(relation, NULL, InvalidBuffer);

	ndone = 0;
	while (ndone < ntuples)
	{
		Buffer		buffer;
		Buffer		vmbuffer = InvalidBuffer;
		bool		all_visible_cleared = false;
		int nthispage;

		/*
		 * Find buffer where at least the next tuple will fit.  If the page
		 * is all-visible, this will also pin the requisite visibility map
		 * page.
		 */
		buffer = RelationGetBufferForTuple(relation, heaptuples[ndone]->t_len,
										   InvalidBuffer, 0, bistate);
		page = BufferGetPage(buffer);

		if (PageIsAllVisible(page))
		{
			all_visible_cleared = true;
			PageClearAllVisible(page);
			visibilitymap_clear(relation,
								BufferGetBlockNumber(buffer	));
		}

		/* NO EREPORT(ERROR) from here till changes are logged */
		START_CRIT_SECTION();

		/* Put as many tuples as fit on this page */
		for (nthispage = 0; ndone + nthispage < ntuples; nthispage++)
		{
			HeapTuple	heaptup = heaptuples[ndone + nthispage];

			if (PageGetHeapFreeSpace(page) < MAXALIGN(heaptup->t_len) + saveFreeSpace)
				break;

			RelationPutHeapTuple(relation, buffer, heaptup);
		}

		/*
		 * XXX Should we set PageSetPrunable on this page ? See heap_insert()
		 */

		MarkBufferDirty(buffer);

		/* XLOG stuff */
		if (needwal)
		{
			XLogRecPtr	recptr;
			xl_heap_multi_insert *xlrec;
			XLogRecData rdata[2];
			uint8		info = XLOG_HEAP2_MULTI_INSERT;
			char	   *tupledata;
			int			totaldatalen;
			char	   *scratchptr = scratch;
			bool		init;

			/*
			 * If the page was previously empty, we can reinit the page
			 * instead of restoring the whole thing.
			 */
			init = (ItemPointerGetOffsetNumber(&(heaptuples[ndone]->t_self)) == FirstOffsetNumber &&
					PageGetMaxOffsetNumber(page) == (uint32)(FirstOffsetNumber + nthispage - 1));

			/* allocate xl_heap_multi_insert struct from the scratch area */
			xlrec = (xl_heap_multi_insert *) scratchptr;
			scratchptr += SizeOfHeapMultiInsert;

			/*
			 * Allocate offsets array. Unless we're reinitializing the page,
			 * in that case the tuples are stored in order starting at
			 * FirstOffsetNumber and we don't need to store the offsets
			 * explicitly.
			 */
			if (!init)
				scratchptr += nthispage * sizeof(OffsetNumber);

			/* the rest of the scratch space is used for tuple data */
			tupledata = scratchptr;

			xlrec->all_visible_cleared = all_visible_cleared;
			xlrec->node = relation->rd_node;
			xlrec->blkno = BufferGetBlockNumber(buffer);
			xlrec->ntuples = nthispage;

			/*
			 * Write out an xl_multi_insert_tuple and the tuple data itself
			 * for each tuple.
			 */
			for (i = 0; i < nthispage; i++)
			{
				HeapTuple	heaptup = heaptuples[ndone + i];
				xl_multi_insert_tuple *tuphdr;
				int			datalen;

				if (!init)
					xlrec->offsets[i] = ItemPointerGetOffsetNumber(&heaptup->t_self);
				/* xl_multi_insert_tuple needs two-byte alignment. */
				tuphdr = (xl_multi_insert_tuple *) SHORTALIGN(scratchptr);
				scratchptr = ((char *) tuphdr) + SizeOfMultiInsertTuple;

				tuphdr->t_infomask2 = heaptup->t_data->t_infomask2;
				tuphdr->t_infomask = heaptup->t_data->t_infomask;
				tuphdr->t_hoff = heaptup->t_data->t_hoff;

				/* write bitmap [+ padding] [+ oid] + data */
				datalen = heaptup->t_len - offsetof(HeapTupleHeaderData, t_bits);
				memcpy(scratchptr,
					   (char *) heaptup->t_data + offsetof(HeapTupleHeaderData, t_bits),
					   datalen);
				tuphdr->datalen = datalen;
				scratchptr += datalen;
			}
			totaldatalen = (int)(scratchptr - tupledata);
			Assert((scratchptr - scratch) < BLCKSZ);

			rdata[0].data = (char *) xlrec;
			rdata[0].len = (uint32)(tupledata - scratch);
			rdata[0].buffer = InvalidBuffer;
			rdata[0].next = &rdata[1];

			rdata[1].data = tupledata;
			rdata[1].len = totaldatalen;
			rdata[1].buffer = buffer;
			rdata[1].buffer_std = true;
			rdata[1].next = NULL;

			/*
			 * If we're going to reinitialize the whole page using the WAL
			 * record, hide buffer reference from XLogInsert.
			 */
			if (init)
			{
				rdata[1].buffer = InvalidBuffer;
				info |= XLOG_HEAP_INIT_PAGE;
			}

			recptr = XLogInsert(RM_HEAP2_ID, info, rdata);

			PageSetLSN(page, recptr);
			PageSetTLI(page, ThisTimeLineID);
		}

		END_CRIT_SECTION();

		UnlockReleaseBuffer(buffer);
		if (vmbuffer != InvalidBuffer)
			ReleaseBuffer(vmbuffer);

		ndone += nthispage;
	}

	

	/*
	 * Copy t_self fields back to the caller's original tuples. This does
	 * nothing for untoasted tuples (tuples[i] == heaptuples[i)], but it's
	 * probably faster to always copy than check.
	 */
	for (i = 0; i < ntuples; i++)
		tuples[i]->t_self = heaptuples[i]->t_self;

	pgstat_count_heap_insert(relation);
	pfree(heaptuples);
	if(needwal)
	{
		if(scratch)
			pfree(scratch);
	}
}
/*
 * Handles MULTI_INSERT record type.
 */
static void
heap_xlog_multi_insert(XLogRecPtr lsn, XLogRecord *record)
{
	char	   *recdata = XLogRecGetData(record);
	xl_heap_multi_insert *xlrec;
	Buffer		buffer;
	Page		page;
	struct
	{
		HeapTupleHeaderData hdr;
		char		data[MaxHeapTupleSize];
	}			tbuf;
	HeapTupleHeader htup;
	uint32		newlen;
	Size		freespace;
	BlockNumber blkno;
	int			i;
	bool		isinit = (record->xl_info & XLOG_HEAP_INIT_PAGE) != 0;

	/*
	 * Insertion doesn't overwrite MVCC data, so no conflict processing is
	 * required.
	 */

	RestoreBkpBlocks(lsn, record, false);

	xlrec = (xl_heap_multi_insert *) recdata;
	recdata += SizeOfHeapMultiInsert;

	/*
	 * If we're reinitializing the page, the tuples are stored in order from
	 * FirstOffsetNumber. Otherwise there's an array of offsets in the WAL
	 * record.
	 */
	if (!isinit)
		recdata += sizeof(OffsetNumber) * xlrec->ntuples;

	blkno = xlrec->blkno;

	/*
	 * The visibility map may need to be fixed even if the heap page is
	 * already up-to-date.
	 */
	if (xlrec->all_visible_cleared)
	{
		Relation	reln = CreateFakeRelcacheEntry(xlrec->node);
		Buffer		vmbuffer = InvalidBuffer;

		visibilitymap_pin(reln, blkno, &vmbuffer);
		visibilitymap_clear(reln, blkno);
		ReleaseBuffer(vmbuffer);
		FreeFakeRelcacheEntry(reln);
	}

	if (record->xl_info & XLR_BKP_BLOCK_1)
		return;

	if (isinit)
	{
		buffer = XLogReadBuffer(xlrec->node, blkno, true);
		Assert(BufferIsValid(buffer));
		page = (Page) BufferGetPage(buffer);

		PageInit(page, BufferGetPageSize(buffer), 0);
	}
	else
	{
		buffer = XLogReadBuffer(xlrec->node, blkno, false);
		if (!BufferIsValid(buffer))
			return;
		page = (Page) BufferGetPage(buffer);

		if (XLByteLE(lsn, PageGetLSN(page)))	/* changes are applied */
		{
			UnlockReleaseBuffer(buffer);
			return;
		}
	}

	for (i = 0; i < xlrec->ntuples; i++)
	{
		OffsetNumber offnum;
		xl_multi_insert_tuple *xlhdr;

		if (isinit)
			offnum = FirstOffsetNumber + i;
		else
			offnum = xlrec->offsets[i];
		if (PageGetMaxOffsetNumber(page) + 1 < offnum)
			elog(PANIC, "heap_multi_insert_redo: invalid max offset number");

		xlhdr = (xl_multi_insert_tuple *) SHORTALIGN(recdata);
		recdata = ((char *) xlhdr) + SizeOfMultiInsertTuple;

		newlen = xlhdr->datalen;
		Assert(newlen <= MaxHeapTupleSize);
		htup = &tbuf.hdr;
		MemSet((char *) htup, 0, sizeof(HeapTupleHeaderData));
		/* PG73FORMAT: get bitmap [+ padding] [+ oid] + data */
		memcpy((char *) htup + offsetof(HeapTupleHeaderData, t_bits),
			   (char *) recdata,
			   newlen);
		recdata += newlen;

		newlen += offsetof(HeapTupleHeaderData, t_bits);
		htup->t_infomask2 = xlhdr->t_infomask2;
		htup->t_infomask = xlhdr->t_infomask;
		htup->t_hoff = xlhdr->t_hoff;
		HeapTupleHeaderSetXmin(htup, record->xl_xid);
		HeapTupleHeaderSetCmin(htup, FirstCommandId);
		ItemPointerSetBlockNumber(&htup->t_ctid, blkno);
		ItemPointerSetOffsetNumber(&htup->t_ctid, offnum);

		offnum = PageAddItem(page, (Item) htup, newlen, offnum, true, true);
		if (offnum == InvalidOffsetNumber)
			elog(PANIC, "heap_multi_insert_redo: failed to add tuple");
	}

	freespace = PageGetHeapFreeSpace(page);		/* needed to update FSM below */

	PageSetLSN(page, lsn);
	PageSetTLI(page, ThisTimeLineID);

	if (xlrec->all_visible_cleared)
		PageClearAllVisible(page);

	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);

	/*
	 * If the page is running low on free space, update the FSM as well.
	 * Arbitrarily, our definition of "low" is less than 20%. We can't do much
	 * better than that without knowing the fill-factor for the table.
	 *
	 * XXX: We don't get here if the page was restored from full page image.
	 * We don't bother to update the FSM in that case, it doesn't need to be
	 * totally accurate anyway.
	 */
	if (freespace < BLCKSZ / 5)
		XLogRecordPageWithFreeSpace(xlrec->node, blkno, freespace);
}

/*
 * bitgetpage - subroutine for BitmapHeapNext()
 *
 * This routine reads and pins the specified page of the relation, then
 * builds an array indicating which tuples on the page are both potentially
 * interesting according to the bitmap, and visible according to the snapshot.
 */
static void
bitgetpage(HeapScanDesc scan, TBMIterateResult *tbmres, IndexScanDesc indexScan)
{
	BlockNumber page = tbmres->blockno;
	Buffer		buffer;
	Snapshot	snapshot;
	int			ntup;

	/*
	 * Acquire pin on the target heap page, trading in any pin we held before.
	 */
	Assert(page < scan->rs_nblocks);

	scan->rs_cbuf = ReleaseAndReadBuffer(scan->rs_cbuf,
										 scan->rs_rd,
										 page);
	buffer = scan->rs_cbuf;
	snapshot = scan->rs_snapshot;

	ntup = 0;

	/*
	 * Prune and repair fragmentation for the whole page, if possible.
	 */
	Assert(TransactionIdIsValid(RecentGlobalXmin));
	heap_page_prune_opt(scan->rs_rd, buffer, RecentGlobalXmin);

	/*
	 * We must hold share lock on the buffer content while examining tuple
	 * visibility.	Afterwards, however, the tuples we have found to be
	 * visible are guaranteed good as long as we hold the buffer pin.
	 */
	LockBuffer(buffer, BUFFER_LOCK_SHARE);

	/*
	 * We need two separate strategies for lossy and non-lossy cases.
	 */
	if (tbmres->ntuples >= 0)
	{
		/*
		 * Bitmap is non-lossy, so we just look through the offsets listed in
		 * tbmres; but we have to follow any HOT chain starting at each such
		 * offset.
		 */
		int			curslot;

		for (curslot = 0; curslot < tbmres->ntuples; curslot++)
		{
			OffsetNumber offnum = tbmres->offsets[curslot];
			ItemPointerData tid;

			ItemPointerSet(&tid, page, offnum);
			if (heap_hot_search_buffer(&tid, scan->rs_rd, buffer, snapshot, NULL))
				scan->rs_vistuples[ntup++] = ItemPointerGetOffsetNumber(&tid);
		}
	}
	else
	{
		/*
		 * Bitmap is lossy, so we must examine each item pointer on the page.
		 * But we can ignore HOT chains, since we'll check each tuple anyway.
		 */
		Page		dp = (Page) BufferGetPage(buffer);
		OffsetNumber maxoff = PageGetMaxOffsetNumber(dp);
		OffsetNumber offnum;

		for (offnum = FirstOffsetNumber; offnum <= maxoff; offnum = OffsetNumberNext(offnum))
		{
			ItemId		lp;
			HeapTupleData loctup;
			bool		valid;
			bool        check;

			lp = PageGetItemId(dp, offnum);
			if (!ItemIdIsNormal(lp))
				continue;
			loctup.t_data = (HeapTupleHeader) PageGetItem((Page) dp, lp);
			loctup.t_len = ItemIdGetLength(lp);
			loctup.t_tableOid = scan->rs_rd->rd_id;
			ItemPointerSet(&loctup.t_self, page, offnum);
			valid = HeapTupleSatisfiesVisibility(&loctup, snapshot, buffer);
			if(valid)
			{
				if(indexScan != NULL)
				{
					check = fdxdb_HeapKeyTestNew(&loctup, indexScan);
				}
				else
				{
					check = fdxdb_HeapKeyTest(&loctup,scan,scan->rs_nkeys,scan->rs_key);
				}

				if(check)
				{
					scan->rs_vistuples[ntup++] = offnum;
					PredicateLockTuple(scan->rs_rd, &loctup, snapshot);
				}
			}
			
			CheckForSerializableConflictOut(valid, scan->rs_rd, &loctup,
											buffer, snapshot);
		}
	}

	LockBuffer(buffer, BUFFER_LOCK_UNLOCK);

	Assert(ntup <= MaxHeapTuplesPerPage);
	scan->rs_ntuples = ntup;
}

HeapTuple
BitmapHeapNext(BitmapHeapScanState *node)
{
	HeapScanDesc scan;
	TIDBitmap  *tbm;
	TBMIterator *tbmiterator;
	TBMIterateResult *tbmres;
	TBMIterator *prefetch_iterator;
	OffsetNumber targoffset;

	/*
	 * extract necessary information from index scan node
	 */
	scan = node->ss_currentScanDesc;
	tbm = node->tbm;
	tbmiterator = node->tbmiterator;
	tbmres = node->tbmres;
	prefetch_iterator = node->prefetch_iterator;


	Assert(tbm != NULL);	

	for (;;)
	{
		Page		dp;
		ItemId		lp;

		/*
		 * Get next page of results if needed
		 */
		if (tbmres == NULL)
		{
			node->tbmres = tbmres = tbm_iterate(tbmiterator);
			if (tbmres == NULL)
			{
				/* no more entries in the bitmap */
				break;
			}
			

			/*
			 * Ignore any claimed entries past what we think is the end of the
			 * relation.  (This is probably not necessary given that we got at
			 * least AccessShareLock on the table before performing any of the
			 * indexscans, but let's be safe.)
			 */
			if (tbmres->blockno >= scan->rs_nblocks)
			{
				node->tbmres = tbmres = NULL;
				continue;
			}

			/*
			 * Fetch the current heap page and identify candidate tuples.
			 */
			bitgetpage(scan, tbmres, node->ss_indexScanDesc);

			/*
			 * Set rs_cindex to first slot to examine
			 */
			scan->rs_cindex = 0;

		}
		else
		{
			/*
			 * Continuing in previously obtained page; advance rs_cindex
			 */
			scan->rs_cindex++;
		}

		/*
		 * Out of range?  If so, nothing more to look at on this page
		 */
		if (scan->rs_cindex < 0 || scan->rs_cindex >= scan->rs_ntuples)
		{
			node->tbmres = tbmres = NULL;
			continue;
		}

		/*
		 * Okay to fetch the tuple
		 */
		targoffset = scan->rs_vistuples[scan->rs_cindex];
		dp = (Page) BufferGetPage(scan->rs_cbuf);
		lp = PageGetItemId(dp, targoffset);
		Assert(ItemIdIsNormal(lp));

		scan->rs_ctup.t_data = (HeapTupleHeader) PageGetItem((Page) dp, lp);
		scan->rs_ctup.t_len = ItemIdGetLength(lp);
		ItemPointerSet(&scan->rs_ctup.t_self, tbmres->blockno, targoffset);

		pgstat_count_heap_fetch(scan->rs_rd);

		/*
		 * Set up the result slot to point to this tuple. Note that the slot
		 * acquires a pin on the buffer.
		 */
		return &scan->rs_ctup;

	}
	return NULL;

}

/* ----------------------------------------------------------------
 *		ExecInitBitmapHeapScan
 *
 *		Initializes the scan's state information.
 * ----------------------------------------------------------------
 */
BitmapHeapScanState *
ExecInitBitmapHeapScanNew(void *pIndexScan, TIDBitmap *tbm)
{
	IndexScanDesc indexScan = (IndexScanDesc)pIndexScan;
	Relation	currentRelation = indexScan->heapRelation;
	Snapshot snapshot = indexScan->xs_snapshot;
	int nKeys = indexScan->numberOfKeys;
	ScanKey key = indexScan->keyData;

	BitmapHeapScanState *scanstate;	

	/*
	 * Assert caller didn't ask for an unsafe snapshot --- see comments at
	 * head of file.
	 */
	Assert(IsMVCCSnapshot(snapshot));

	/*
	 * create state structure
	 */
	scanstate = makeNode(BitmapHeapScanState);
	scanstate->ss_indexScanDesc = indexScan;
	scanstate->tbm = tbm;
	scanstate->tbmiterator = NULL;
	scanstate->tbmres = NULL;
	scanstate->prefetch_iterator = NULL;
	scanstate->prefetch_pages = 0;
	scanstate->prefetch_target = 0;

	scanstate->tbmiterator = tbm_begin_iterate(tbm);

	//scanstate->ss_currentScanDesc->rs_rd = currentRelation;

	ScanKey heapScanKey = (ScanKey)palloc0(nKeys * sizeof(ScanKeyData));

	for(int keyNum = 0; keyNum < nKeys; keyNum ++)
	{
		char* p = VARDATA_ANY(DatumGetCString(key[keyNum].sk_argument));
		Datum datum = fdxdb_string_formdatum(p,key[keyNum].sk_arglen);
		Fdxdb_ScanKeyInitWithCallbackInfo(&heapScanKey[keyNum], key[keyNum].sk_attno,key[keyNum].sk_strategy,key[keyNum].sk_compfun,datum);
	}
	/*
	 * Even though we aren't going to do a conventional seqscan, it is useful
	 * to create a HeapScanDesc --- most of the fields in it are usable.
	 */
	scanstate->ss_currentScanDesc = heap_beginscan_bm(currentRelation,
														 snapshot,
														 nKeys,
														 heapScanKey,0,0);

	pfree(heapScanKey);
	/*
	 * all done.
	 */
	return scanstate;
}

BitmapHeapScanState *
ExecInitBitmapHeapScan(Relation	currentRelation, TIDBitmap  *tbm, Snapshot snapshot,int nKeys, ScanKey key)
{
	BitmapHeapScanState *scanstate;	

	/*
	 * Assert caller didn't ask for an unsafe snapshot --- see comments at
	 * head of file.
	 */
	Assert(IsMVCCSnapshot(snapshot));

	/*
	 * create state structure
	 */
	scanstate = makeNode(BitmapHeapScanState);
	scanstate->ss_indexScanDesc = NULL;
	scanstate->tbm = tbm;
	scanstate->tbmiterator = NULL;
	scanstate->tbmres = NULL;
	scanstate->prefetch_iterator = NULL;
	scanstate->prefetch_pages = 0;
	scanstate->prefetch_target = 0;

	scanstate->tbmiterator = tbm_begin_iterate(tbm);

	//scanstate->ss_currentScanDesc->rs_rd = currentRelation;

	ScanKey heapScanKey = (ScanKey)palloc0(nKeys * sizeof(ScanKeyData));

	for(int keyNum = 0; keyNum < nKeys; keyNum ++)
	{
		char* p = VARDATA_ANY(DatumGetCString(key[keyNum].sk_argument));
		Datum datum = fdxdb_string_formdatum(p,key[keyNum].sk_arglen);
		Fdxdb_ScanKeyInitWithCallbackInfo(&heapScanKey[keyNum], key[keyNum].sk_attno,key[keyNum].sk_strategy,key[keyNum].sk_compfun,datum);
	}
	/*
	 * Even though we aren't going to do a conventional seqscan, it is useful
	 * to create a HeapScanDesc --- most of the fields in it are usable.
	 */
	scanstate->ss_currentScanDesc = heap_beginscan_bm(currentRelation,
														 snapshot,
														 nKeys,
														 heapScanKey,0,0);

	pfree(heapScanKey);
	/*
	 * all done.
	 */
	return scanstate;
}


/* ----------------------------------------------------------------
 *		ExecEndBitmapHeapScan
 * ----------------------------------------------------------------
 */
void
ExecEndBitmapHeapScan(BitmapHeapScanState *node)
{
	Relation	relation;
	HeapScanDesc scanDesc;

	/*
	 * extract information from the node
	 */
	relation = node->ss_currentScanDesc->rs_rd;
	scanDesc = node->ss_currentScanDesc;

	/*
	 * release bitmap if any
	 */
	if (node->tbmiterator)
		tbm_end_iterate(node->tbmiterator);
	if (node->prefetch_iterator)
		tbm_end_iterate(node->prefetch_iterator);
	//if (node->tbm)
	//	tbm_free(node->tbm);

	/*
	 * close heap scan
	 */
	heap_endscan(scanDesc);
}

#ifdef FOUNDER_XDB_SE
size_t CounterHeapSize(Oid relid)
{
	size_t count = 0;
	Relation rel = heap_open(relid, AccessShareLock);
	HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, NULL, NULL);

	while(heap_getnext(scan, ForwardScanDirection) != NULL)
		++count;

	heap_endscan(scan);
	heap_close(rel, NoLock);

	return count;
}

#endif //FOUNDER_XDB_SE