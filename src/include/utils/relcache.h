/*-------------------------------------------------------------------------
 *
 * relcache.h
 *	  Relation descriptor cache definitions.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/relcache.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef RELCACHE_H
#define RELCACHE_H

#include "access/tupdesc.h"
#include "nodes/bitmapset.h"
#include "nodes/pg_list.h"
#ifdef FOUNDER_XDB_SE
#include "postmaster/xdb_main.h"
#endif //FOUNDER_XDB_SE

typedef struct RelationData *Relation;
typedef struct ColinfoData *Colinfo;

/* ----------------
 *		RelationPtr is used in the executor to support index scans
 *		where we have to keep track of several index relations in an
 *		array.	-cim 9/10/89
 * ----------------
 */
typedef Relation *RelationPtr;

/*
 * Routines to open (lookup) and close a relcache entry
 */
#ifdef FOUNDER_XDB_SE
extern Relation RelationIdGetRelation(Oid relationId);
#else
extern Relation RelationIdGetRelation(Oid relationId);
#endif //FOUNDER_XDB_SE
extern void RelationClose(Relation relation);

/*
 * Routines to compute/retrieve additional cached information
 */
extern List *RelationGetIndexList(Relation relation);
#ifndef FOUNDER_XDB_SE
extern Oid	RelationGetOidIndex(Relation relation);
extern List *RelationGetIndexExpressions(Relation relation);
extern List *RelationGetIndexPredicate(Relation relation);
#endif
extern Bitmapset *RelationGetIndexAttrBitmap(Relation relation);
extern void RelationGetExclusionInfo(Relation indexRelation,
						 Oid **operators,
						 Oid **procs,
						 uint16 **strategies);

extern void RelationSetIndexList(Relation relation,
					 List *indexIds, Oid oidIndex);
#ifndef FOUNDER_XDB_SE
extern void RelationInitIndexAccessInfo(Relation relation);
#endif
/*
 * Routines for backend startup
 */
extern void RelationCacheInitialize(void);
extern void RelationCacheInitializePhase2(void);
extern void RelationCacheInitializePhase3(void);

/*
 * Routine to create a relcache entry for an about-to-be-created relation
 */
extern Relation RelationBuildLocalRelation(const char *relname,
						   Oid relnamespace,
						   TupleDesc tupDesc,
						   Oid relid,
							 Oid object_id,
						   Oid reltablespace,
						   Oid dbid,
						   bool shared_relation,
						   bool mapped_relation,
						   char relpersisten,
						   Colinfo colinfo = NULL,
						   bool insertIt = true,
						   bool pitIt = true);

/*
 * Routine to manage assignment of new relfilenode to a relation
 */
extern void RelationSetNewRelfilenode(Relation relation,
						  TransactionId freezeXid);

/*
 * Routines for flushing/rebuilding relcache entries in various scenarios
 */
extern void RelationForgetRelation(Oid rid);

extern void RelationCacheInvalidateEntry(Oid relationId);

extern void RelationCacheInvalidate(void);

extern void RelationCloseSmgrByOid(Oid relationId);

extern void AtEOXact_RelationCache(bool isCommit);
extern void AtEOSubXact_RelationCache(bool isCommit, SubTransactionId mySubid,
						  SubTransactionId parentSubid);

/*
 * Routines to help manage rebuilding of relcache init files
 */
extern bool RelationIdIsInInitFile(Oid relationId);
#ifndef FOUNDER_XDB_SE
extern void RelationCacheInitFileInvalidate(bool beforeSend);
extern void RelationCacheInitFilePreInvalidate(void);
extern void RelationCacheInitFilePostInvalidate(void);
#endif
extern void RelationCacheInitFileRemove(void);

/* should be used only by relcache.c and catcache.c */
extern THREAD_LOCAL bool criticalRelcachesBuilt;

/* should be used only by relcache.c and postinit.c */
extern THREAD_LOCAL bool criticalSharedRelcachesBuilt;

#ifdef FOUNDER_XDB_SE
extern void fxdb_free_relid_cache();
extern void RelationCacheDeleteIfAny(Oid relid);
#endif //FOUNDER_XDB_SE

#endif   /* RELCACHE_H */
