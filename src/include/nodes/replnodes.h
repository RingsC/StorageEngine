/*-------------------------------------------------------------------------
 *
 * replnodes.h
 *	  definitions for replication grammar parse nodes
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/nodes/replnodes.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef REPLNODES_H
#define REPLNODES_H

#include "access/xlogdefs.h"
#include "nodes/pg_list.h"


/* ----------------------
 *		IDENTIFY_SYSTEM command
 * ----------------------
 */
typedef struct IdentifySystemCmd
{
	NodeTag		type;
} IdentifySystemCmd;


/* ----------------------
 *		BASE_BACKUP command
 * ----------------------
 */
typedef struct BaseBackupCmd
{
	NodeTag		type;
	List	   *options;
} BaseBackupCmd;


#ifdef FOUNDER_XDB_SE
typedef struct YYXlogRecPtr
{
	/*
	 * startpoint is the real postion where walsender send xlog from,
	 * requestpoint is the postion where slave want to receive
	 * xlog from. startpoint must be less or equal requestpoint.
	 */
	XLogRecPtr startpoint;
	XLogRecPtr requestpoint;
} YYXlogRecPtr;
#endif //FOUNDER_XDB_SE

/* ----------------------
 *		START_REPLICATION command
 * ----------------------
 */
typedef struct StartReplicationCmd
{
	NodeTag		type;
#ifndef FOUNDER_XDB_SE
	XLogRecPtr	startpoint;
#else
	TimeLineID	timeline;
	YYXlogRecPtr	startpos;
#endif //FOUNDER_XDB_SE
} StartReplicationCmd;

#ifdef FOUNDER_XDB_SE
/* ----------------------
 *		TIMELINE_HISTORY command
 * ----------------------
 */
typedef struct TimeLineHistoryCmd
{
	NodeTag		type;
	TimeLineID	timeline;
} TimeLineHistoryCmd;
#endif

#endif   /* REPLNODES_H */
