/*-------------------------------------------------------------------------
 *
 * resowner.h
 *	  POSTGRES resource owner definitions.
 *
 * Query-lifespan resources are tracked by associating them with
 * ResourceOwner objects.  This provides a simple mechanism for ensuring
 * that such resources are freed at the right time.
 * See utils/resowner/README for more info.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/resowner.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef RESOWNER_H
#define RESOWNER_H

#include "storage/buf.h"
#include "storage/fd.h"
#ifndef FOUNDER_XDB_SE
#include "utils/catcache.h"
#include "utils/plancache.h"
#else
#include "utils/relcache.h"
#endif

#include "utils/snapshot.h"


/*
 * ResourceOwner objects look like this
 */
typedef struct ResourceOwnerData
{
	ResourceOwner parent;		/* NULL if no parent (toplevel owner) */
	ResourceOwner firstchild;	/* head of linked list of children */
	ResourceOwner nextchild;	/* next child of same parent */
	const char *name;			/* name (just for debugging) */

	/* We have built-in support for remembering owned buffers */
	int			nbuffers;		/* number of owned buffer pins */
	Buffer	   *buffers;		/* dynamically allocated array */
	int			maxbuffers;		/* currently allocated array size */

	/* We have built-in support for remembering catcache references */
	int			ncatrefs;		/* number of owned catcache pins */
	HeapTuple  *catrefs;		/* dynamically allocated array */
	int			maxcatrefs;		/* currently allocated array size */
#ifndef FOUNDER_XDB_SE
	int			ncatlistrefs;	/* number of owned catcache-list pins */
	CatCList  **catlistrefs;	/* dynamically allocated array */
	int			maxcatlistrefs; /* currently allocated array size */
#endif
	/* We have built-in support for remembering relcache references */
	int			nrelrefs;		/* number of owned relcache pins */
	Relation   *relrefs;		/* dynamically allocated array */
	int			maxrelrefs;		/* currently allocated array size */
#ifndef FOUNDER_XDB_SE
	/* We have built-in support for remembering plancache references */
	int			nplanrefs;		/* number of owned plancache pins */
	CachedPlan **planrefs;		/* dynamically allocated array */
	int			maxplanrefs;	/* currently allocated array size */
#endif
	/* We have built-in support for remembering tupdesc references */
	int			ntupdescs;		/* number of owned tupdesc references */
	TupleDesc  *tupdescs;		/* dynamically allocated array */
	int			maxtupdescs;	/* currently allocated array size */

	/* We have built-in support for remembering snapshot references */
	int			nsnapshots;		/* number of owned snapshot references */
	Snapshot   *snapshots;		/* dynamically allocated array */
	int			maxsnapshots;	/* currently allocated array size */

	/* We have built-in support for remembering open temporary files */
	int			nfiles;			/* number of owned temporary files */
	File	   *files;			/* dynamically allocated array */
	int			maxfiles;		/* currently allocated array size */
}	ResourceOwnerData;
/*
 * ResourceOwner objects are an opaque data structure known only within
 * resowner.c.
 */
typedef struct ResourceOwnerData *ResourceOwner;

#ifndef FOUNDER_XDB_SE
/*
* Globally known ResourceOwners
*/
extern THREAD_LOCAL ResourceOwner CurrentResourceOwner;
extern THREAD_LOCAL ResourceOwner CurTransactionResourceOwner;
extern THREAD_LOCAL ResourceOwner TopTransactionResourceOwner;
#endif //FOUNDER_XDB_SE

#ifdef FOUNDER_XDB_SE
/*
 * Globally known ResourceOwners
 */
extern THREAD_LOCAL ResourceOwner CurrentResourceOwner;
extern THREAD_LOCAL ResourceOwner CurTransactionResourceOwner;
extern THREAD_LOCAL ResourceOwner TopTransactionResourceOwner;
#endif //FOUNDER_XDB_SE

/*
 * Resource releasing is done in three phases: pre-locks, locks, and
 * post-locks.	The pre-lock phase must release any resources that are
 * visible to other backends (such as pinned buffers); this ensures that
 * when we release a lock that another backend may be waiting on, it will
 * see us as being fully out of our transaction.  The post-lock phase
 * should be used for backend-internal cleanup.
 */
typedef enum
{
	RESOURCE_RELEASE_BEFORE_LOCKS,
	RESOURCE_RELEASE_LOCKS,
	RESOURCE_RELEASE_AFTER_LOCKS
} ResourceReleasePhase;

/*
 *	Dynamically loaded modules can get control during ResourceOwnerRelease
 *	by providing a callback of this form.
 */
typedef void (*ResourceReleaseCallback) (ResourceReleasePhase phase,
													 bool isCommit,
													 bool isTopLevel,
													 void *arg);


/*
 * Functions in resowner.c
 */

/* generic routines */
extern ResourceOwner ResourceOwnerCreate(ResourceOwner parent,
					const char *name);
extern void ResourceOwnerRelease(ResourceOwner owner,
					 ResourceReleasePhase phase,
					 bool isCommit,
					 bool isTopLevel);
extern void ResourceOwnerDelete(ResourceOwner owner);
extern ResourceOwner ResourceOwnerGetParent(ResourceOwner owner);
extern void ResourceOwnerNewParent(ResourceOwner owner,
					   ResourceOwner newparent);
extern void RegisterResourceReleaseCallback(ResourceReleaseCallback callback,
								void *arg);
extern void UnregisterResourceReleaseCallback(ResourceReleaseCallback callback,
								  void *arg);

/* support for buffer refcount management */
extern void ResourceOwnerEnlargeBuffers(ResourceOwner owner);
extern void ResourceOwnerRememberBuffer(ResourceOwner owner, Buffer buffer);
extern void ResourceOwnerForgetBuffer(ResourceOwner owner, Buffer buffer);

/* support for catcache refcount management */
extern void ResourceOwnerEnlargeCatCacheRefs(ResourceOwner owner);
extern void ResourceOwnerRememberCatCacheRef(ResourceOwner owner,
								 HeapTuple tuple);
extern void ResourceOwnerForgetCatCacheRef(ResourceOwner owner,
							   HeapTuple tuple);
extern void ResourceOwnerEnlargeCatCacheListRefs(ResourceOwner owner);
#ifndef FOUNDER_XDB_SE
extern void ResourceOwnerRememberCatCacheListRef(ResourceOwner owner,
									 CatCList *list);
extern void ResourceOwnerForgetCatCacheListRef(ResourceOwner owner,
								   CatCList *list);
#endif
/* support for relcache refcount management */
extern void ResourceOwnerEnlargeRelationRefs(ResourceOwner owner);
extern void ResourceOwnerRememberRelationRef(ResourceOwner owner,
								 Relation rel);
extern void ResourceOwnerForgetRelationRef(ResourceOwner owner,
							   Relation rel);
#ifndef FOUNDER_XDB_SE
/* support for plancache refcount management */
extern void ResourceOwnerEnlargePlanCacheRefs(ResourceOwner owner);
extern void ResourceOwnerRememberPlanCacheRef(ResourceOwner owner,
								  CachedPlan *plan);
extern void ResourceOwnerForgetPlanCacheRef(ResourceOwner owner,
								CachedPlan *plan);
#endif
/* support for tupledesc refcount management */
extern void ResourceOwnerEnlargeTupleDescs(ResourceOwner owner);
extern void ResourceOwnerRememberTupleDesc(ResourceOwner owner,
							   TupleDesc tupdesc);
extern void ResourceOwnerForgetTupleDesc(ResourceOwner owner,
							 TupleDesc tupdesc);

/* support for snapshot refcount management */
extern void ResourceOwnerEnlargeSnapshots(ResourceOwner owner);
extern void ResourceOwnerRememberSnapshot(ResourceOwner owner,
							  Snapshot snapshot);
extern void ResourceOwnerForgetSnapshot(ResourceOwner owner,
							Snapshot snapshot);

/* support for temporary file management */
extern void ResourceOwnerEnlargeFiles(ResourceOwner owner);
extern void ResourceOwnerRememberFile(ResourceOwner owner,
						  File file);
extern void ResourceOwnerForgetFile(ResourceOwner owner,
						File file);

#endif   /* RESOWNER_H */
