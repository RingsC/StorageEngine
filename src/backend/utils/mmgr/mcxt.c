/*-------------------------------------------------------------------------
 *
 * mcxt.c
 *	  POSTGRES memory context management code.
 *
 * This module handles context management operations that are independent
 * of the particular kind of context being operated on.  It calls
 * context-type-specific operations via the function pointers in a
 * context's MemoryContextMethods struct.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/utils/mmgr/mcxt.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "utils/memutils.h"
#include "storage/ipc.h"


/*****************************************************************************
 *	  GLOBAL MEMORY															 *
 *****************************************************************************/
#ifndef FOUNDER_XDB_SE
/*
* CurrentMemoryContext
*		Default memory context for allocations.
*/
MemoryContext CurrentMemoryContext = NULL;

/*
* Standard top-level contexts. For a description of the purpose of each
* of these contexts, refer to src/backend/utils/mmgr/README
*/
MemoryContext TopMemoryContext = NULL;
MemoryContext ErrorContext = NULL;
MemoryContext PostmasterContext = NULL;
MemoryContext CacheMemoryContext = NULL;
MemoryContext MessageContext = NULL;
MemoryContext TopTransactionContext = NULL;
MemoryContext CurTransactionContext = NULL;

/* This is a transient link to the active portal's memory context: */
MemoryContext PortalContext = NULL;
#endif //FOUNDER_XDB_SE

#ifdef FOUNDER_XDB_SE
/*
 * CurrentMemoryContext
 *		Default memory context for allocations.
 */
THREAD_LOCAL MemoryContext CurrentMemoryContext = NULL;

/*
 * Standard top-level contexts. For a description of the purpose of each
 * of these contexts, refer to src/backend/utils/mmgr/README
 */
THREAD_LOCAL MemoryContext TopMemoryContext = NULL;
MemoryContext ProcessTopMemoryContext = NULL;
#ifdef ALLOC_INFO
uint32 nbacktraceMemcontext = 0;
char**backtraceMemcontext = NULL;
#endif
THREAD_LOCAL MemoryContext ErrorContext = NULL;
THREAD_LOCAL MemoryContext PostmasterContext = NULL;
THREAD_LOCAL MemoryContext CacheMemoryContext = NULL;
THREAD_LOCAL MemoryContext MessageContext = NULL;
THREAD_LOCAL MemoryContext TopTransactionContext = NULL;
THREAD_LOCAL MemoryContext CurTransactionContext = NULL;

/* This is a transient link to the active portal's memory context: */
THREAD_LOCAL MemoryContext PortalContext = NULL;
#endif //FOUNDER_XDB_SE
#if defined(ALLOC_INFO)
static void MemoryContextStatsInternal(MemoryContext context, int level,
	FILE *pFile, bool memleak=false, int depth=5, size_t size=1024*1024);
#else
static void MemoryContextStatsInternal(MemoryContext context, int level);
#endif

/*****************************************************************************
 *	  EXPORTED ROUTINES														 *
 *****************************************************************************/
inline unsigned int getThreadId()
{
#ifdef WIN32
	return::GetCurrentThreadId();
#else
	return pthread_self();
#endif

}

/*
 * MemoryContextInit
 *		Start up the memory-context subsystem.
 *
 * This must be called before creating contexts or allocating memory in
 * contexts.  TopMemoryContext and ErrorContext are initialized here;
 * other contexts must be created afterwards.
 *
 * In normal multi-backend operation, this is called once during
 * postmaster startup, and not at all by individual backend startup
 * (since the backends inherit an already-initialized context subsystem
 * by virtue of being forked off the postmaster).
 *
 * In a standalone backend this must be called during backend startup.
 */
void
MemoryContextInit(bool thread)
{
    if(NULL != TopMemoryContext)
	{
		return;
	}

	/*
	 * Initialize TopMemoryContext as an AllocSetContext with slow growth rate
	 * --- we don't really expect much to be allocated in it.
	 *
	 * (There is special-case code in MemoryContextCreate() for this call.)
	 */
	if(!thread)
	{
		AssertState(ProcessTopMemoryContext == NULL);

		ProcessTopMemoryContext = AllocSetContextCreate((MemoryContext) NULL,
											 "ProcessTopMemoryContext",
											 0,
											 8 * 1024,
											 8 * 1024);
		TopMemoryContext = ProcessTopMemoryContext;
	}
	else
	{

		unsigned int threadid = getThreadId();
		char topname[128] = {0};
		sprintf(topname, "TopMemoryContext:%u", threadid);
		TopMemoryContext = AllocSetContextCreate((MemoryContext) ProcessTopMemoryContext,
													 topname,
													 0,
													 8 * 1024,
													 1024 * 1024);
	}
	/*
	 * Not having any other place to point CurrentMemoryContext, make it point
	 * to TopMemoryContext.  Caller should change this soon!
	 */
	CurrentMemoryContext = TopMemoryContext;


	/*
	 * Initialize ErrorContext as an AllocSetContext with slow growth rate ---
	 * we don't really expect much to be allocated in it. More to the point,
	 * require it to contain at least 8K at all times. This is the only case
	 * where retained memory in a context is *essential* --- we want to be
	 * sure ErrorContext still has some memory even if we've run out
	 * elsewhere!
	 */
	ErrorContext = AllocSetContextCreate(TopMemoryContext,
										 "ErrorContext",
										 8 * 1024,
										 8 * 1024,
										 8 * 1024);

}

/*
 * MemoryContextReset
 *		Release all space allocated within a context and its descendants,
 *		but don't delete the contexts themselves.
 *
 * The type-specific reset routine handles the context itself, but we
 * have to do the recursion for the children.
 */
void
MemoryContextReset(MemoryContext context, bool useLock)
{

	AssertArg(MemoryContextIsValid(context));
	if(useLock)
	SpinLockAcquire(&context->mem_lck);
	/* save a function call in common case where there are no children */
	if (context->firstchild != NULL)
		MemoryContextResetChildren(context);

	/* Nothing to do if no pallocs since startup or last reset */
	if (!context->isReset)
	{
		(*context->methods->reset) (context);
		context->isReset = true;
	}
	if(useLock)
	SpinLockRelease(&context->mem_lck);
}

/*
 * MemoryContextResetChildren
 *		Release all space allocated within a context's descendants,
 *		but don't delete the contexts themselves.  The named context
 *		itself is not touched.
 */
void
MemoryContextResetChildren(MemoryContext context)
{
	MemoryContext child;

	AssertArg(MemoryContextIsValid(context));

	for (child = context->firstchild; child != NULL; child = child->nextchild)
		MemoryContextReset(child, false);
}

/*
 * MemoryContextDelete
 *		Delete a context and its descendants, and release all space
 *		allocated therein.
 *
 * The type-specific delete routine removes all subsidiary storage
 * for the context, but we have to delete the context node itself,
 * as well as recurse to get the children.	We must also delink the
 * node from its parent, if it has one.
 */
void
MemoryContextDelete(MemoryContext context)
{
	AssertArg(MemoryContextIsValid(context));
	/* We had better not be deleting TopMemoryContext ... */
	Assert(context != ProcessTopMemoryContext);
	/* And not CurrentMemoryContext, either */
	Assert(context != CurrentMemoryContext);

	MemoryContextDeleteChildren(context);

	/*
	 * We delink the context from its parent before deleting it, so that if
	 * there's an error we won't have deleted/busted contexts still attached
	 * to the context tree.  Better a leak than a crash.
	 */
	if (context->parent)
	{
		MemoryContext parent = context->parent;
		SpinLockAcquire(&parent->mem_lck);
		if (context == parent->firstchild)
			parent->firstchild = context->nextchild;
		else
		{
			MemoryContext child;

			for (child = parent->firstchild; child != 0 ; child = child->nextchild)
			{
				if (context == child->nextchild)
				{
					child->nextchild = context->nextchild;
					context->nextchild = 0;
					break;
				}
			}
		}
		SpinLockRelease(&parent->mem_lck);
	}
	(*context->methods->delete_context) (context);
	pfree(context);
}

/*
 * MemoryContextDeleteChildren
 *		Delete all the descendants of the named context and release all
 *		space allocated therein.  The named context itself is not touched.
 */
void
MemoryContextDeleteChildren(MemoryContext context)
{
	AssertArg(MemoryContextIsValid(context));

	/*
	 * MemoryContextDelete will delink the child from me, so just iterate as
	 * long as there is a child.
	 */
	while (context->firstchild != NULL)
		MemoryContextDelete(context->firstchild);
}

/*
 * MemoryContextResetAndDeleteChildren
 *		Release all space allocated within a context and delete all
 *		its descendants.
 *
 * This is a common combination case where we want to preserve the
 * specific context but get rid of absolutely everything under it.
 */
void
MemoryContextResetAndDeleteChildren(MemoryContext context)
{
	
	AssertArg(MemoryContextIsValid(context));
	//SpinLockAcquire(&context->mem_lck);
	MemoryContextDeleteChildren(context);
	MemoryContextReset(context, false);
	//SpinLockRelease(&context->mem_lck);
}

/*
 * GetMemoryChunkSpace
 *		Given a currently-allocated chunk, determine the total space
 *		it occupies (including all memory-allocation overhead).
 *
 * This is useful for measuring the total space occupied by a set of
 * allocated chunks.
 */
Size
GetMemoryChunkSpace(void *pointer)
{
	StandardChunkHeader *header;

	/*
	 * Try to detect bogus pointers handed to us, poorly though we can.
	 * Presumably, a pointer that isn't MAXALIGNED isn't pointing at an
	 * allocated chunk.
	 */
	Assert(pointer != NULL);
	Assert(pointer == (void *) MAXALIGN(pointer));

	/*
	 * OK, it's probably safe to look at the chunk header.
	 */
	header = (StandardChunkHeader *)
		((char *) pointer - STANDARDCHUNKHEADERSIZE);

	AssertArg(MemoryContextIsValid(header->context));

	return (*header->context->methods->get_chunk_space) (header->context,
														 pointer);
}

/*
 * GetMemoryChunkContext
 *		Given a currently-allocated chunk, determine the context
 *		it belongs to.
 */
MemoryContext
GetMemoryChunkContext(void *pointer)
{
	StandardChunkHeader *header;

	/*
	 * Try to detect bogus pointers handed to us, poorly though we can.
	 * Presumably, a pointer that isn't MAXALIGNED isn't pointing at an
	 * allocated chunk.
	 */
	Assert(pointer != NULL);
	Assert(pointer == (void *) MAXALIGN(pointer));

	/*
	 * OK, it's probably safe to look at the chunk header.
	 */
	header = (StandardChunkHeader *)
		((char *) pointer - STANDARDCHUNKHEADERSIZE);

	AssertArg(MemoryContextIsValid(header->context));

	return header->context;
}

/*
 * MemoryContextIsEmpty
 *		Is a memory context empty of any allocated space?
 */
bool
MemoryContextIsEmpty(MemoryContext context)
{
	
	AssertArg(MemoryContextIsValid(context));
	SpinLockAcquire(&context->mem_lck);
	bool ret = false;
	/*
	 * For now, we consider a memory context nonempty if it has any children;
	 * perhaps this should be changed later.
	 */
	if (context->firstchild != NULL)
	{
		SpinLockRelease(&context->mem_lck);
		return false;
	}
	/* Otherwise use the type-specific inquiry */
	ret = (*context->methods->is_empty) (context);
	SpinLockRelease(&context->mem_lck);
	return ret;
}

/*
 * MemoryContextStats
 *		Print statistics about the named context and all its descendants.
 *
 * This is just a debugging utility, so it's not fancy.  The statistics
 * are merely sent to stderr.
 */
#if defined(ALLOC_INFO)
void
MemoryContextStats(MemoryContext context, bool memleak, int depth, size_t size)
#else
void
MemoryContextStats(MemoryContext context)
#endif
{
#if defined(ALLOC_INFO)
	FILE *pFile = NULL;
	pFile = fopen("MemLeak.txt","w+");

	MemoryContextStatsInternal(context, 0, pFile, memleak, depth, size);
	fclose(pFile);
#else
	MemoryContextStatsInternal(context, 0);
	fprintf(stderr, "----------------------------------------------------------------\n\n\n");
#endif
	
}

#if defined(ALLOC_INFO)
static void
MemoryContextStatsInternal(MemoryContext context, int level, FILE *pFile,
	bool memleak, int depth, size_t size)
#else
static void
MemoryContextStatsInternal(MemoryContext context, int level)
#endif
{
	MemoryContext child;
	AssertArg(MemoryContextIsValid(context));
	SpinLockAcquire(&context->mem_lck);
#if defined(ALLOC_INFO)
	(*context->methods->stats) (context, level, pFile, memleak, depth, size);
#else
	(*context->methods->stats) (context, level);
#endif
	for (child = context->firstchild; child != NULL; child = child->nextchild)
	{
	#if defined(ALLOC_INFO)
		MemoryContextStatsInternal(child, level + 1, pFile, memleak, depth, size);
	#else
		MemoryContextStatsInternal(child, level + 1);
	#endif
	}
	SpinLockRelease(&context->mem_lck);
}

/*
 * MemoryContextCheck
 *		Check all chunks in the named context.
 *
 * This is just a debugging utility, so it's not fancy.
 */
#ifdef MEMORY_CONTEXT_CHECKING
void
MemoryContextCheck(MemoryContext context)
{	
	AssertArg(MemoryContextIsValid(context));
	SpinLockAcquire(&context->mem_lck);
	MemoryContext child;
	(*context->methods->check) (context);
	for (child = context->firstchild; child != NULL; child = child->nextchild)
		MemoryContextCheck(child);
	SpinLockRelease(&context->mem_lck);
}
#endif

/*
 * MemoryContextContains
 *		Detect whether an allocated chunk of memory belongs to a given
 *		context or not.
 *
 * Caution: this test is reliable as long as 'pointer' does point to
 * a chunk of memory allocated from *some* context.  If 'pointer' points
 * at memory obtained in some other way, there is a small chance of a
 * false-positive result, since the bits right before it might look like
 * a valid chunk header by chance.
 */
bool
MemoryContextContains(MemoryContext context, void *pointer)
{
	StandardChunkHeader *header;

	/*
	 * Try to detect bogus pointers handed to us, poorly though we can.
	 * Presumably, a pointer that isn't MAXALIGNED isn't pointing at an
	 * allocated chunk.
	 */
	if (pointer == NULL || pointer != (void *) MAXALIGN(pointer))
		return false;

	/*
	 * OK, it's probably safe to look at the chunk header.
	 */
	header = (StandardChunkHeader *)
		((char *) pointer - STANDARDCHUNKHEADERSIZE);

	/*
	 * If the context link doesn't match then we certainly have a non-member
	 * chunk.  Also check for a reasonable-looking size as extra guard against
	 * being fooled by bogus pointers.
	 */
	SpinLockAcquire(&context->mem_lck);
	bool ret = header->context == context && AllocSizeIsValid(header->size);
	SpinLockRelease(&context->mem_lck);
	return ret;
}

/*--------------------
 * MemoryContextCreate
 *		Context-type-independent part of context creation.
 *
 * This is only intended to be called by context-type-specific
 * context creation routines, not by the unwashed masses.
 *
 * The context creation procedure is a little bit tricky because
 * we want to be sure that we don't leave the context tree invalid
 * in case of failure (such as insufficient memory to allocate the
 * context node itself).  The procedure goes like this:
 *	1.	Context-type-specific routine first calls MemoryContextCreate(),
 *		passing the appropriate tag/size/methods values (the methods
 *		pointer will ordinarily point to statically allocated data).
 *		The parent and name parameters usually come from the caller.
 *	2.	MemoryContextCreate() attempts to allocate the context node,
 *		plus space for the name.  If this fails we can ereport() with no
 *		damage done.
 *	3.	We fill in all of the type-independent MemoryContext fields.
 *	4.	We call the type-specific init routine (using the methods pointer).
 *		The init routine is required to make the node minimally valid
 *		with zero chance of failure --- it can't allocate more memory,
 *		for example.
 *	5.	Now we have a minimally valid node that can behave correctly
 *		when told to reset or delete itself.  We link the node to its
 *		parent (if any), making the node part of the context tree.
 *	6.	We return to the context-type-specific routine, which finishes
 *		up type-specific initialization.  This routine can now do things
 *		that might fail (like allocate more memory), so long as it's
 *		sure the node is left in a state that delete will handle.
 *
 * This protocol doesn't prevent us from leaking memory if step 6 fails
 * during creation of a top-level context, since there's no parent link
 * in that case.  However, if you run out of memory while you're building
 * a top-level context, you might as well go home anyway...
 *
 * Normally, the context node and the name are allocated from
 * TopMemoryContext (NOT from the parent context, since the node must
 * survive resets of its parent context!).	However, this routine is itself
 * used to create TopMemoryContext!  If we see that TopMemoryContext is NULL,
 * we assume we are creating TopMemoryContext and use malloc() to allocate
 * the node.
 *
 * Note that the name field of a MemoryContext does not point to
 * separately-allocated storage, so it should not be freed at context
 * deletion.
 *--------------------
 */
MemoryContext
MemoryContextCreate(NodeTag tag, Size size,
					MemoryContextMethods *methods,
					MemoryContext parent,
					const char *name)
{
	MemoryContext node;
	Size		needed = size + strlen(name) + 1;

	/* Get space for node and name */
	if (parent != NULL)
	{
		/* Normal case: allocate the node in TopMemoryContext */
		node = (MemoryContext) MemoryContextAlloc(parent,
												  needed);
	}
	else
	{
		/* Special case for startup: use good ol' malloc */
		node = (MemoryContext) malloc(needed);
		Assert(node != NULL);
	}

	/* Initialize the node as best we can */
	MemSet(node, 0, size);
	node->type = tag;
	node->methods = methods;
	node->parent = NULL;		/* for the moment */
	node->firstchild = NULL;
	node->nextchild = NULL;
	node->isReset = true;
	node->name = ((char *) node) + size;
	strcpy(node->name, name);
#ifdef ALLOC_INFO
	if(nbacktraceMemcontext != 0)
	{
		node->backtrace = false;

		char *bt_name;
		for(uint32 i = 0; i < nbacktraceMemcontext; i++)
		{
			bt_name = backtraceMemcontext[i];
			if(strcmp(node->name, bt_name) == 0)
			{
				node->backtrace = true;
				break;
			}
		}
	}
	else
	{
		node->backtrace = true;
	}
#endif
	/* Type-specific routine finishes any other essential initialization */
	(*node->methods->init) (node);

	/* OK to link node to parent (if any) */
	if (parent)
	{
		SpinLockAcquire(&parent->mem_lck);
		node->parent = parent;
		node->nextchild = parent->firstchild;
		parent->firstchild = node;
		SpinLockRelease(&parent->mem_lck);
	}

	/* Return to type-specific creation routine to finish up */
	return node;
}

/*
 * MemoryContextAlloc
 *		Allocate space within the specified context.
 *
 * This could be turned into a macro, but we'd have to import
 * nodes/memnodes.h into postgres.h which seems a bad idea.
 */
void *
MemoryContextAlloc(MemoryContext context, Size size)
{
	void * returnPointer = NULL;
	
	AssertArg(MemoryContextIsValid(context));

	if (!AllocSizeIsValid(size))
		ereport(ERROR,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg( "invalid memory alloc request size %lu",
			                 (unsigned long) size)));
	SpinLockAcquire(&context->mem_lck);
	context->isReset = false;
	returnPointer = (*context->methods->alloc) (context, size);
	SpinLockRelease(&context->mem_lck);
	return returnPointer;
}

/*
 * MemoryContextAllocZero
 *		Like MemoryContextAlloc, but clears allocated memory
 *
 *	We could just call MemoryContextAlloc then clear the memory, but this
 *	is a very common combination, so we provide the combined operation.
 */
void *
MemoryContextAllocZero(MemoryContext context, Size size)
{
	void	   *ret;
	
	AssertArg(MemoryContextIsValid(context));

	if (!AllocSizeIsValid(size))
		ereport(ERROR,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg( "invalid memory alloc request size %lu",
			                 (unsigned long) size)));
	SpinLockAcquire(&context->mem_lck);
	context->isReset = false;

	ret = (*context->methods->alloc) (context, size);

	MemSetAligned(ret, 0, size);
	SpinLockRelease(&context->mem_lck);
	return ret;
}

/*
 * MemoryContextAllocZeroAligned
 *		MemoryContextAllocZero where length is suitable for MemSetLoop
 *
 *	This might seem overly specialized, but it's not because newNode()
 *	is so often called with compile-time-constant sizes.
 */
void *
MemoryContextAllocZeroAligned(MemoryContext context, Size size)
{
	void	   *ret;
	
	AssertArg(MemoryContextIsValid(context));

	if (!AllocSizeIsValid(size))
		ereport(ERROR,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg( "invalid memory alloc request size %lu",
			                 (unsigned long) size)));
	SpinLockAcquire(&context->mem_lck);
	context->isReset = false;

	ret = (*context->methods->alloc) (context, size);

	MemSetLoop(ret, 0, size);
	SpinLockRelease(&context->mem_lck);
	return ret;
}

/*
 * pfree
 *		Release an allocated chunk.
 */
void
pfree(void *pointer)
{
	StandardChunkHeader *header;

	/*
	 * Try to detect bogus pointers handed to us, poorly though we can.
	 * Presumably, a pointer that isn't MAXALIGNED isn't pointing at an
	 * allocated chunk.
	 */
	Assert(pointer != NULL);
	Assert(pointer == (void *) MAXALIGN(pointer));

	/*
	 * OK, it's probably safe to look at the chunk header.
	 */
	header = (StandardChunkHeader *)
		((char *) pointer - STANDARDCHUNKHEADERSIZE);

	AssertArg(MemoryContextIsValid(header->context));
	MemoryContext context = header->context;
	SpinLockAcquire(&context->mem_lck);
	(*context->methods->free_p) (context, pointer);
	SpinLockRelease(&context->mem_lck);
}

/*
 * repalloc
 *		Adjust the size of a previously allocated chunk.
 */
void *
repalloc(void *pointer, Size size)
{
	StandardChunkHeader *header;
	void * newPointer = NULL;
	/*
	 * Try to detect bogus pointers handed to us, poorly though we can.
	 * Presumably, a pointer that isn't MAXALIGNED isn't pointing at an
	 * allocated chunk.
	 */
	Assert(pointer != NULL);
	Assert(pointer == (void *) MAXALIGN(pointer));

	/*
	 * OK, it's probably safe to look at the chunk header.
	 */
	header = (StandardChunkHeader *)
		((char *) pointer - STANDARDCHUNKHEADERSIZE);

	AssertArg(MemoryContextIsValid(header->context));

	if (!AllocSizeIsValid(size))
		ereport(ERROR,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                errmsg( "invalid memory alloc request size %lu",
			                 (unsigned long) size)));

	/* isReset must be false already */
	Assert(!header->context->isReset);
	MemoryContext context = header->context;
	SpinLockAcquire(&context->mem_lck);
	newPointer = (*context->methods->realloc) (context,
												 pointer, size);
	SpinLockRelease(&context->mem_lck);
	return newPointer;
}

/*
 * MemoryContextSwitchTo
 *		Returns the current context; installs the given context.
 *
 * palloc.h defines an inline version of this function if allowed by the
 * compiler; in which case the definition below is skipped.
 */
#ifndef USE_INLINE

MemoryContext
MemoryContextSwitchTo(MemoryContext context)
{
	MemoryContext old;

	AssertArg(MemoryContextIsValid(context));

	old = CurrentMemoryContext;
	CurrentMemoryContext = context;
	return old;
}
#endif   /* ! USE_INLINE */

/*
 * MemoryContextStrdup
 *		Like strdup(), but allocate from the specified context
 */
char *
MemoryContextStrdup(MemoryContext context, const char *string)
{
	char	   *nstr;
	Size		len = strlen(string) + 1;	
	nstr = (char *) MemoryContextAlloc(context, len);	
	memcpy(nstr, string, len);

	return nstr;
}

/*
 * pnstrdup
 *		Like pstrdup(), but append null byte to a
 *		not-necessarily-null-terminated input string.
 */
char *
pnstrdup(const char *in, Size len)
{
	char	   *out = (char *)palloc(len + 1);

	memcpy(out, in, len);
	out[len] = '\0';
	return out;
}


#if defined(WIN32) || defined(__CYGWIN__)
/*
 *	Memory support routines for libpgport on Win32
 *
 *	Win32 can't load a library that PGDLLIMPORTs a variable
 *	if the link object files also PGDLLIMPORT the same variable.
 *	For this reason, libpgport can't reference CurrentMemoryContext
 *	in the palloc macro calls.
 *
 *	To fix this, we create several functions here that allow us to
 *	manage memory without doing the inline in libpgport.
 */
void *
pgport_palloc(Size sz)
{
	return palloc(sz);
}


char *
pgport_pstrdup(const char *str)
{
	return pstrdup(str);
}


/* Doesn't reference a PGDLLIMPORT variable, but here for completeness. */
void
pgport_pfree(void *pointer)
{
	pfree(pointer);
}

#endif

#ifdef FOUNDER_XDB_SE
void fxdb_free_topmemory(int code, Datum arg)
{
	if (TopMemoryContext != NULL)
	{
		MemoryContextSwitchTo(ProcessTopMemoryContext);
		MemoryContextDelete(TopMemoryContext);
	}
}
#endif //FOUNDER_XDB_SE
