#ifndef _PGMEMCONTEXT_H
#define _PGMEMCONTEXT_H
#include "MemoryContext.h"

typedef struct MemoryContextData *MemoryContext;

namespace FounderXDB {
	namespace StorageEngineNS {
class PgMemcontext
	:public MemoryContext
{

public:
	PgMemcontext(::MemoryContext pContext = 0, bool bTxnMemCxt = false);
	~PgMemcontext();
friend class PGTransaction;
public:
    virtual int createAsOwnerOf(void *p);
	virtual int create(const char *name, MemoryContext *parent = 0 ) ;
	/// delete memory hold by this cxt. if onlyChildren is true, only do so
	/// for all its child cxt.
	virtual void destroy(bool onlyChildren = false);


	/// Allocate bytesAlloc bytes in this cxt and init each byte in allocated 
	/// memory with initByte.
	virtual void *alloc(size_t bytesAlloc) ;
	/// do realloc for addr in this cxt.
	//virtual void *reAlloc(void *addr, size_t newSize) ;
	/// Returns whether this cxt contains any allocated space.
	virtual bool isEmpty() const ;
	/*
	* Detect whether an allocated chunk of memory belongs to a given
	* context or not.
	*/ 
	virtual bool contains(void *) const ;
	// Print statistics about the named context and all its descendants.
	virtual void getStat(bool memleak = false, int depth = 5, size_t size = 1024*1024) ;
	///Check all chunks in the named context.
	virtual void check() ;
	/// duplicate the first 'n' bytes of string s by allocating memory in this cxt. 
	virtual char *strDup(const char *s, size_t n ) ;
	/*
	*Release all space allocated within a context and its descendants,
	*but don't delete the contexts themselves.
	*/
	virtual void reset(bool onlyChildren = false) ;

	virtual std::string getContextName() const;

	//Help functions
	void checkValid( void )const;
public:
	bool CanDestory( void );
	inline void incrementChildren(bool inc) 
	{
		if (inc)
		{
			++static_cast<PgMemcontext*>(m_pParent)->m_nChildren;
		}
		else
		{
			--static_cast<PgMemcontext*>(m_pParent)->m_nChildren;
		}
	}

	::MemoryContext m_pContext;
	MemoryContext* m_pParent;
    int m_nChildren;
    bool m_bTxnMemCxt;

    static	THREAD_LOCAL PgMemcontext* g_pProcessTopContext;
	static	THREAD_LOCAL PgMemcontext* g_pTopContext;
	static	THREAD_LOCAL PgMemcontext* g_pErrContext;
	//static  THREAD_LOCAL PgMemcontext* g_pTransactionContext;
	static	THREAD_LOCAL PgMemcontext* g_pPostmasterContext;
	static	THREAD_LOCAL PgMemcontext* g_pCacheContext;
	//static  THREAD_LOCAL PgMemcontext* g_pTopTransactionContext;
	static  THREAD_LOCAL PgMemcontext* g_pMessageContext;
};
}
}
#endif
