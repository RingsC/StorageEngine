/*-------------------------------------------------------------------------
 *
 * timeline.c
 *		Functions for reading and writing timeline history files.
 *
 * A timeline history file lists the timeline changes of the timeline, in
 * a simple text format. They are archived along with the WAL segments.
 *
 * The files are named like "<tli>.history". For example, if the database
 * starts up and switches to timeline 5, the timeline history file would be
 * called "00000005.history".
 *
 * Each line in the file represents a timeline switch:
 *
 * <parentTLI> <switchpoint> <reason>
 *
 *	parentTLI	ID of the parent timeline
 *	switchpoint	XLogRecPtr of the WAL position where the switch happened
 *	reason		human-readable explanation of why the timeline was changed
 *
 * The fields are separated by tabs. Lines beginning with # are comments, and
 * are ignored. Empty lines are also ignored.
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/access/transam/timeline.c
 *
 *-------------------------------------------------------------------------
 */
#ifdef FOUNDER_XDB_SE
#include "postgres.h"
	 
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>	 

#include "access/timeline.h"
#include "access/xlog_internal.h"
#include "access/xlogdefs.h"
#include "storage/fd.h"
#include "pthread/pthread.h"

extern bool InArchiveRecovery;
static void GetCurrentTimeString(char *str, int strsize);
static pg_time_t StringToPgtime(const char *timestr);

/*
* Try to read a timeline's history file.
*
* If successful, return the list of component TLIs (the given TLI followed by
* its ancestor TLIs).	If we can't find the history file, assume that the
* timeline has no parents, and return a list of just the specified timeline
* ID.
*/
List *
readTimeLineHistory(TimeLineID targetTLI)
{
	List	   *result;
	char		path[MAXPGPATH];
	char		histfname[MAXFNAMELEN];
	char		fline[MAXPGPATH];
	FILE	   *fd;
	TimeLineHistoryEntry *entry;
	TimeLineID	lasttli = 0;
	XLogRecPtr	prevend;

	/* Timeline 1 does not have a history file, so no need to check */
	if (targetTLI == 1)
	{
		entry = (TimeLineHistoryEntry *) palloc(sizeof(TimeLineHistoryEntry));
		entry->tli = targetTLI;
		entry->begin = entry->end = InvalidXLogRecPtr;
		return list_make1(entry);
	}

	if (InArchiveRecovery)
	{
		TLHistoryFileName(histfname, targetTLI);
		RestoreArchivedFile(path, histfname, "RECOVERYHISTORY", 0);
	}
	else
		TLHistoryFilePath(path, targetTLI);

	fd = AllocateFile(path, "r");
	if (fd == NULL)
	{
		if (errno != ENOENT)
			ereport(FATAL,
					(errcode_for_file_access(),
					 errmsg("could not open file \"%s\": %m", path)));
		/* Not there, so assume no parents */
		entry = (TimeLineHistoryEntry *) palloc(sizeof(TimeLineHistoryEntry));
		entry->tli = targetTLI;
		entry->begin = entry->end = InvalidXLogRecPtr;
		return list_make1(entry);
	}

	result = NIL;

	/*
	 * Parse the file...
	 */
	prevend = InvalidXLogRecPtr;
	while (fgets(fline, sizeof(fline), fd) != NULL)
	{
		/* skip leading whitespace and check for # comment */
		char	   *ptr;
		TimeLineID	tli;
		XLogRecPtr	switchpoint;
		int 		nfields;

		for (ptr = fline; *ptr; ptr++)
		{
			if (!isspace((unsigned char) *ptr))
				break;
		}
		if (*ptr == '\0' || *ptr == '#')
			continue;

		nfields = sscanf(fline, "%u\t%X/%X", &tli, &switchpoint.xlogid, &switchpoint.xrecoff);

		if (nfields < 1)
		{
			/* expect a numeric timeline ID as first field of line */
			ereport(FATAL,
					(errmsg("syntax error in history file: %s", fline),
					 errhint("Expected a numeric timeline ID.")));
		}
		if (nfields != 3)
			ereport(FATAL,
					(errmsg("syntax error in history file: %s", fline),
					 errhint("Expected an XLOG switchpoint location.")));

		if (result && tli <= lasttli)
			ereport(FATAL,
					(errmsg("invalid data in history file: %s", fline),
				   errhint("Timeline IDs must be in increasing sequence.")));

		lasttli = tli;

		entry = (TimeLineHistoryEntry *) palloc(sizeof(TimeLineHistoryEntry));
		entry->tli = tli;
		entry->begin = prevend;
		entry->end = switchpoint;
		prevend = entry->end;

		/* Build list with newest item first */
		result = lcons(entry, result);

		/* we ignore the remainder of each line */
	}

	FreeFile(fd);

	if (result && targetTLI <= lasttli)
		ereport(FATAL,
				(errmsg("invalid data in history file \"%s\"", path),
			errhint("Timeline IDs must be less than child timeline's ID.")));

	/*
	 * Create one more entry for the "tip" of the timeline, which has no
	 * entry in the history file.
	 */
	entry = (TimeLineHistoryEntry *) palloc(sizeof(TimeLineHistoryEntry));
	entry->tli = targetTLI;
	entry->begin = prevend;
	entry->end = InvalidXLogRecPtr;

	result = lcons(entry, result);

	return result;
}

/*
* Probe whether a timeline history file exists for the given timeline ID
*/
bool
existsTimeLineHistory(TimeLineID probeTLI)
{
	char		path[MAXPGPATH];
	char		histfname[MAXFNAMELEN];
	FILE	   *fd;

	/* Timeline 1 does not have a history file, so no need to check */
	if (probeTLI == 1)
		return false;

	if (InArchiveRecovery)
	{
		TLHistoryFileName(histfname, probeTLI);
		RestoreArchivedFile(path, histfname, "RECOVERYHISTORY", 0);
	}
	else
		TLHistoryFilePath(path, probeTLI);

	fd = AllocateFile(path, "r");
	if (fd != NULL)
	{
		FreeFile(fd);
		return true;
	}
	else
	{
		if (errno != ENOENT)
			ereport(FATAL,
			(errcode_for_file_access(),
			errmsg("could not open file \"%s\": %m", path)));
		return false;
	}
}

/*
* Find the newest existing timeline, assuming that startTLI exists.
*
* Note: while this is somewhat heuristic, it does positively guarantee
* that (result + 1) is not a known timeline, and therefore it should
* be safe to assign that ID to a new timeline.
*/
TimeLineID
findNewestTimeLine(TimeLineID startTLI)
{
	TimeLineID	newestTLI;
	TimeLineID	probeTLI;

	/*
	* The algorithm is just to probe for the existence of timeline history
	* files.  XXX is it useful to allow gaps in the sequence?
	*/
	newestTLI = startTLI;

	for (probeTLI = startTLI + 1;; probeTLI++)
	{
		if (existsTimeLineHistory(probeTLI))
		{
			newestTLI = probeTLI;		/* probeTLI exists */
		}
		else
		{
			/* doesn't exist, assume we're done */
			break;
		}
	}

	return newestTLI;
}

/*
* Create a new timeline history file.
*
*	newTLI: ID of the new timeline
*	parentTLI: ID of its immediate parent
*	endTLI et al: ID of the last used WAL file, for annotation purposes
*
* Currently this is only used during recovery, and so there are no locking
* considerations.	But we should be just as tense as XLogFileInit to avoid
* emplacing a bogus file.
*/
void
writeTimeLineHistory(TimeLineID newTLI, TimeLineID parentTLI, 
					XLogRecPtr switchpoint, char *reason)
{
	char		path[MAXPGPATH];
	char		tmppath[MAXPGPATH];
	char		histfname[MAXFNAMELEN];
	char		buffer[BLCKSZ];
	char        timestr[64];
	int 		srcfd;
	int 		fd;
	int 		nbytes;

	Assert(newTLI > parentTLI); /* else bad selection of newTLI */

	/*
	* Write into a temp file name.
	*/
	snprintf(tmppath, MAXPGPATH, XLOGDIR "/xlogtemp.%d", (int) pthread_self());

	unlink(tmppath);

	/* do not use get_sync_bit() here --- want to fsync only at end of fill */
	fd = BasicOpenFile(tmppath, O_RDWR | O_CREAT | O_EXCL,
		S_IRUSR | S_IWUSR);
	if (fd < 0)
		ereport(ERROR,
		(errcode_for_file_access(),
		errmsg("could not create file \"%s\": %m", tmppath)));

	/*
	* If a history file exists for the parent, copy it verbatim
	*/
	if (InArchiveRecovery)
	{
		TLHistoryFileName(histfname, parentTLI);
		RestoreArchivedFile(path, histfname, "RECOVERYHISTORY", 0);
	}
	else
		TLHistoryFilePath(path, parentTLI);

	srcfd = BasicOpenFile(path, O_RDONLY, 0);
	if (srcfd < 0)
	{
		if (errno != ENOENT)
			ereport(ERROR,
			(errcode_for_file_access(),
			errmsg("could not open file \"%s\": %m", path)));
		/* Not there, so assume parent has no parents */
	}
	else
	{
		for (;;)
		{
			errno = 0;
			nbytes = (int) read(srcfd, buffer, sizeof(buffer));
			if (nbytes < 0 || errno != 0)
				ereport(ERROR,
				(errcode_for_file_access(),
				errmsg("could not read file \"%s\": %m", path)));
			if (nbytes == 0)
				break;
			errno = 0;
			if ((int) write(fd, buffer, nbytes) != nbytes)
			{
				int 		save_errno = errno;

				/*
				* If we fail to make the file, delete it to release disk
				* space
				*/
				unlink(tmppath);

				/*
				* if write didn't set errno, assume problem is no disk space
				*/
				errno = save_errno ? save_errno : ENOSPC;

				ereport(ERROR,
					(errcode_for_file_access(),
					errmsg("could not write to file \"%s\": %m", tmppath)));
			}
		}
		close(srcfd);
	}

	GetCurrentTimeString(timestr, sizeof(timestr));	
	snprintf(buffer, sizeof(buffer),
			 "%s%u\t%X/%X\t%s\t%s\n",
			 (srcfd < 0) ? "" : "\n",
			 parentTLI,
			 switchpoint.xlogid, switchpoint.xrecoff,
			 timestr,
			 reason);

	nbytes = (int)strlen(buffer);
	errno = 0;
	if ((int) write(fd, buffer, nbytes) != nbytes)
	{
		int 		save_errno = errno;

		/*
		* If we fail to make the file, delete it to release disk space
		*/
		unlink(tmppath);
		/* if write didn't set errno, assume problem is no disk space */
		errno = save_errno ? save_errno : ENOSPC;

		ereport(ERROR,
			(errcode_for_file_access(),
			errmsg("could not write to file \"%s\": %m", tmppath)));
	}

	if (pg_fsync(fd) != 0)
		ereport(ERROR,
		(errcode_for_file_access(),
		errmsg("could not fsync file \"%s\": %m", tmppath)));

	if (close(fd))
		ereport(ERROR,
		(errcode_for_file_access(),
		errmsg("could not close file \"%s\": %m", tmppath)));


	/*
	* Now move the completed history file into place with its final name.
	*/
	TLHistoryFilePath(path, newTLI);

	/*
	* Prefer link() to rename() here just to be really sure that we don't
	* overwrite an existing logfile.  However, there shouldn't be one, so
	* rename() is an acceptable substitute except for the truly paranoid.
	*/
#if HAVE_WORKING_LINK
	if (link(tmppath, path) < 0)
		ereport(ERROR,
		(errcode_for_file_access(),
		errmsg("could not link file \"%s\" to \"%s\": %m",
		tmppath, path)));
	unlink(tmppath);
#else
	if (rename(tmppath, path) < 0)
		ereport(ERROR,
		(errcode_for_file_access(),
		errmsg("could not rename file \"%s\" to \"%s\": %m",
		tmppath, path)));
#endif

	/* The history file can be archived immediately. */
	TLHistoryFileName(histfname, newTLI);
	XLogArchiveNotify(histfname);
}

/*
 * Writes a history file for given timeline and contents.
 *
 * Currently this is only used in the walreceiver process, and so there are
 * no locking considerations.  But we should be just as tense as XLogFileInit
 * to avoid emplacing a bogus file.
 */
void
writeTimeLineHistoryFile(TimeLineID tli, char *content, int size)
{
	char		path[MAXPGPATH];
	char		tmppath[MAXPGPATH];
	int 		fd;

	/*
	 * Write into a temp file name.
	 */
	snprintf(tmppath, MAXPGPATH, XLOGDIR "/xlogtemp.%d", (int) pthread_self());

	unlink(tmppath);

	/* do not use get_sync_bit() here --- want to fsync only at end of fill */
	fd = OpenTransientFile(tmppath, O_RDWR | O_CREAT | O_EXCL,
						   S_IRUSR | S_IWUSR);
	if (fd < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not create file \"%s\": %m", tmppath)));

	errno = 0;
	if ((int) write(fd, content, size) != size)
	{
		int 		save_errno = errno;

		/*
		 * If we fail to make the file, delete it to release disk space
		 */
		unlink(tmppath);
		/* if write didn't set errno, assume problem is no disk space */
		errno = save_errno ? save_errno : ENOSPC;

		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not write to file \"%s\": %m", tmppath)));
	}

	if (pg_fsync(fd) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not fsync file \"%s\": %m", tmppath)));

	if (CloseTransientFile(fd))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m", tmppath)));


	/*
	 * Now move the completed history file into place with its final name.
	 */
	TLHistoryFilePath(path, tli);

	/*
	 * Prefer link() to rename() here just to be really sure that we don't
	 * overwrite an existing logfile.  However, there shouldn't be one, so
	 * rename() is an acceptable substitute except for the truly paranoid.
	 */
#if HAVE_WORKING_LINK
	if (link(tmppath, path) < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not link file \"%s\" to \"%s\": %m",
						tmppath, path)));
	unlink(tmppath);
#else
	if (rename(tmppath, path) < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not rename file \"%s\" to \"%s\": %m",
						tmppath, path)));
#endif
}

/*
 * Returns true if 'expectedTLEs' contains a timeline with id 'tli'
 */
bool
tliInHistory(TimeLineID tli, List *expectedTLEs)
{
	ListCell *cell;

	foreach(cell, expectedTLEs)
	{
		if (((TimeLineHistoryEntry *) lfirst(cell))->tli == tli)
			return true;
	}

	return false;
}

/*
 * Returns the ID of the timeline in use at a particular point in time, in
 * the given timeline history.
 */
TimeLineID
tliOfPointInHistory(XLogRecPtr ptr, List *history)
{
	ListCell *cell;

	foreach(cell, history)
	{
		TimeLineHistoryEntry *tle = (TimeLineHistoryEntry *) lfirst(cell);
		if ((XLogRecPtrIsInvalid(tle->begin) || XLByteLE(tle->begin, ptr)) &&
			(XLogRecPtrIsInvalid(tle->end) || XLByteLT(ptr, tle->end)))
		{
			/* found it */
			return tle->tli;
		}
	}

	/* shouldn't happen. */
	elog(ERROR, "timeline history was not contiguous");
	return 0;	/* keep compiler quiet */
}

/*
 * Returns the point in history where we branched off the given timeline.
 * Returns InvalidXLogRecPtr if the timeline is current (= we have not
 * branched off from it), and throws an error if the timeline is not part of
 * this server's history.
 */
XLogRecPtr
tliSwitchPoint(TimeLineID tli, List *history)
{
	ListCell   *cell;

	foreach (cell, history)
	{
		TimeLineHistoryEntry *tle = (TimeLineHistoryEntry *) lfirst(cell);

		if (tle->tli == tli)
			return tle->end;
	}

	ereport(ERROR,
			(errmsg("requested timeline %u is not in this server's history",
					tli)));
	return InvalidXLogRecPtr; /* keep compiler quiet */
}

TimeLineID
DecideRecoveryTargetTLI(TimeLineID tli, pg_time_t recoveryTime)
{
	TimeLineID  lastTLI = -1;
	TimeLineID  resultTLI = -1;
	char		path[MAXPGPATH];
	char		fline[MAXPGPATH];
	FILE	    *fd;

	/* Open the target timeline histroy file */
	TLHistoryFilePath(path, tli);
	fd = AllocateFile(path, "r");
	if (fd == NULL)
		ereport(FATAL,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": %m", path)));

	/* Scan the target timeline histroy file , found the right timeline for recovery*/
	while (fgets(fline, sizeof(fline), fd) != NULL)
	{
		/* skip leading whitespace and check for # comment */
		char	   *ptr;
		TimeLineID	tli;
		XLogRecPtr	switchpoint;
		char        timestr[32];
		char        datestr[32];
		int 		nfields;
		pg_time_t   switchtime;

		for (ptr = fline; *ptr; ptr++)
		{
			if (!isspace((unsigned char) *ptr))
				break;
		}
		if (*ptr == '\0' || *ptr == '#')
			continue;

		nfields = sscanf(fline, "%u\t%X/%X\t%s %s",
								&tli, 
								&switchpoint.xlogid, 
								&switchpoint.xrecoff,
								datestr,
								timestr);

		if (nfields < 1)
		{
			/* expect a numeric timeline ID as first field of line */
			ereport(FATAL,
					(errmsg("syntax error in history file: %s", fline),
					 errhint("Expected a numeric timeline ID.")));
		}
		if (nfields != 5)
			ereport(FATAL,
					(errmsg("syntax error in history file: %s", fline),
					 errhint("Expected an XLOG switchpoint location.")));

		if (lastTLI != (uint32) -1 && tli <= lastTLI)
			ereport(FATAL,
					(errmsg("invalid data in history file: %s", fline),
				   errhint("Timeline IDs must be in increasing sequence.")));

		strcat(datestr, timestr);
		switchtime = StringToPgtime(datestr);
		if (switchtime < 0)
			ereport(FATAL,
					(errmsg("invalid data in history file: %s", fline),
				   errhint("Expected time string.")));

		if (recoveryTime < switchtime)
		{
			resultTLI = tli;
			break;
		}
		
		lastTLI = tli;
		/* we ignore the remainder of each line */
	}
	FreeFile(fd);

	/*
	 * Reach at the file end, not found the suitable tli. We use the latest tli
	 * as the recovery target tli.
	 */
	if (resultTLI == (uint32) -1)
		resultTLI = tli;
		
	return resultTLI;
}

static void
GetCurrentTimeString(char *str, int strsize)
{
	pg_time_t	stamp_time;

	if (str == NULL)
		return;
	
	stamp_time = (pg_time_t) time(NULL);
	pg_strftime(str, strsize, "%Y-%m-%d %H:%M:%S",
					pg_localtime(&stamp_time, NULL));
}

static pg_time_t
StringToPgtime(const char *timestr)
{
	struct tm tm_t;
	int nfields;

	if (timestr == NULL)
		return -1;
	
	nfields = sscanf(timestr, "%4d-%2d-%2d%2d:%2d:%2d",
					&tm_t.tm_year,&tm_t.tm_mon,&tm_t.tm_mday,
					&tm_t.tm_hour,&tm_t.tm_min,&tm_t.tm_sec);
	if (nfields < 1 || nfields != 6)
		return -1;
	
	tm_t.tm_year -= 1900;
	tm_t.tm_mon -= 1;
	tm_t.tm_isdst = -1;
	return (pg_time_t)mktime(&tm_t);
}
#endif
