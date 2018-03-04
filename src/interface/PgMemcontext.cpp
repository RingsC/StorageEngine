#include "PgMemcontext.h"
#include "Macros.h"
#include <cassert>
#include "c.h"
#include "pg_config_manual.h"
#include "utils/palloc.h"
#include "utils/memutils.h"
#include "utils/elog.h"
#include "interface/ErrNo.h"
#include "interface/StorageEngineExceptionUniversal.h"
namespace FounderXDB {
	namespace StorageEngineNS {
THREAD_LOCAL PgMemcontext* PgMemcontext::g_pProcessTopContext= NULL;
THREAD_LOCAL PgMemcontext* PgMemcontext::g_pTopContext= NULL;
THREAD_LOCAL PgMemcontext* PgMemcontext::g_pErrContext= NULL;
//THREAD_LOCAL PgMemcontext* PgMemcontext::g_pTransactionContext= NULL;
THREAD_LOCAL PgMemcontext* PgMemcontext::g_pPostmasterContext= NULL;
THREAD_LOCAL PgMemcontext* PgMemcontext::g_pCacheContext= NULL;
//THREAD_LOCAL PgMemcontext* PgMemcontext::g_pTopTransactionContext= NULL;
THREAD_LOCAL PgMemcontext* PgMemcontext::g_pMessageContext= NULL;




PgMemcontext::PgMemcontext(::MemoryContext pContext, bool bTxnMemCxt)
:m_pContext(pContext)
,m_pParent(NULL)
,m_nChildren(0)
,m_bTxnMemCxt(bTxnMemCxt)
{

}
int PgMemcontext::createAsOwnerOf(void *p)
{
	THROW_CALL(m_pContext = ::GetMemoryChunkContext,p);
    assert(m_pContext != NULL);
    return 0;
}
#if !defined(_MSC_VER)
#pragma GCC diagnostic ignored "-Wclobbered"
#endif
 int PgMemcontext::create(const char *name, MemoryContext *parent )
 {
	 if (NULL == m_pContext)
	 {
		 if (NULL == parent)
		 {
			throw StorageEngineExceptionUniversal(LOGIC_ERR,"create MemContex with no parent!");			
		 }
	     m_pParent = parent;
		 incrementChildren(true);//增加PgMemcontext::incrementChildren(bool inc)方法，在其中完成本操作

		 THROW_CALL(m_pContext = AllocSetContextCreate
			 ,static_cast<PgMemcontext*>(parent)->m_pContext
			 ,name
			 ,ALLOCSET_DEFAULT_MINSIZE
			 ,ALLOCSET_DEFAULT_INITSIZE
			 ,ALLOCSET_DEFAULT_MAXSIZE);

		 return 0;
	 }
	 return -1;
 }

#if !defined(_MSC_VER)
#pragma GCC diagnostic warning "-Wclobbered"
#endif
PgMemcontext::~PgMemcontext()
{
	
}

void PgMemcontext::destroy(bool onlyChildren )
{
	if (NULL == m_pContext)
	{
	    delete this;
		return;
	}

	if(CanDestory())
	{
		if (m_nChildren > 0)
		{
			throw StorageEngineExceptionUniversal(LOGIC_ERR,"Please destory it's children first!");
		}

		if (onlyChildren)
		{
			THROW_CALL(::MemoryContextDeleteChildren,m_pContext);

		} else {
			THROW_CALL(::MemoryContextDelete,m_pContext); 
		}
		incrementChildren(false);
		//同上，在incrementChildren中完成。

	}
	delete this;
}

bool PgMemcontext::CanDestory( void )
{
	return !m_bTxnMemCxt
	    && (this !=  g_pTopContext)
		&& (this !=  g_pErrContext)
		&& (this !=  g_pPostmasterContext)
		&& (this !=  g_pCacheContext)
		&& (this !=  g_pMessageContext);
}

MemoryContext *MemoryContext::getMemoryContext(Type cxttype)  
{
    switch(cxttype)
	{
	case ProcessTop:
		if (NULL == PgMemcontext::g_pProcessTopContext && NULL != ::ProcessTopMemoryContext)
		{
			PgMemcontext::g_pProcessTopContext = new PgMemcontext(::ProcessTopMemoryContext);
		}

		return PgMemcontext::g_pProcessTopContext;
	case Top:
		if (NULL == PgMemcontext::g_pTopContext && NULL != ::TopMemoryContext)
		{
			PgMemcontext::g_pTopContext = new PgMemcontext(::TopMemoryContext);
		}
		
		return PgMemcontext::g_pTopContext;
	case Error:
		if (NULL == PgMemcontext::g_pErrContext && NULL != ::ErrorContext)
		{
			PgMemcontext::g_pErrContext = new PgMemcontext(::ErrorContext);
		}
		
		return PgMemcontext::g_pErrContext;
	case Transaction://txn memory context will be created in pgtransaction
	    assert(false);
	//	if (NULL == PgMemcontext::g_pTransactionContext && NULL != ::CurTransactionContext)
	//	{
	//		PgMemcontext::g_pTransactionContext = new PgMemcontext(::CurTransactionContext);
	//	}

	//	if (NULL != PgMemcontext::g_pTransactionContext)
	//	{
	//		PgMemcontext::g_pTransactionContext->m_pContext = ::CurTransactionContext;
	//	}
	//	
	//	return PgMemcontext::g_pTransactionContext;
	case Postmaster:
		if (NULL == PgMemcontext::g_pPostmasterContext && NULL != PgMemcontext::g_pPostmasterContext)
		{
			PgMemcontext::g_pPostmasterContext = new PgMemcontext(::PostmasterContext);
		}
		
		return PgMemcontext::g_pPostmasterContext;
	case Cache:
		if (NULL == PgMemcontext::g_pCacheContext && NULL != ::CacheMemoryContext)
		{
			PgMemcontext::g_pCacheContext =  new PgMemcontext(::CacheMemoryContext);
		}
		return PgMemcontext::g_pCacheContext;
	case TopTransaction:
	    assert(false);
		/*if (NULL == PgMemcontext::g_pTopTransactionContext && NULL != ::TopTransactionContext)
		{
			PgMemcontext::g_pTopTransactionContext =  new PgMemcontext(::TopTransactionContext);
		}
		
		return PgMemcontext::g_pTopTransactionContext;*/
	case Message:
		if (NULL == PgMemcontext::g_pMessageContext && NULL != ::MessageContext)
		{
			PgMemcontext::g_pMessageContext = new PgMemcontext(::MessageContext);
		}
		
		return PgMemcontext::g_pMessageContext;
	case AlwaysCreate:
		return new PgMemcontext(NULL);
	default:
		break;
	}
	return NULL;
}

void *PgMemcontext::alloc(size_t bytesAlloc)
{
	checkValid();
	void *pVoid = NULL;
	pVoid = MemoryContextAlloc(m_pContext,bytesAlloc);
    //two reasons to comment memset calling.
    //1. for performance, if memory size is too large, memset takes long time.
    //2. for explicit initialize. if some data is not initialized, it will lead some unexpect result.
    // developer should control the initialize process.
	//memset(pVoid,initByte,bytesAlloc);
	if(!pVoid)
		throw OutOfMemoryException();
	return pVoid;
}

void *MemoryContext::reAlloc(void *addr, size_t newSize)
{
	assert(NULL != addr);
	void *pVoid = NULL;
	THROW_CALL(pVoid = ::repalloc,addr,newSize);
	return pVoid;
}

void MemoryContext::deAlloc(void *p)
{
	pfree(p);
}

bool PgMemcontext::isEmpty() const  
{
	checkValid();
	bool bEmpty = false;
	THROW_CALL(bEmpty = MemoryContextIsEmpty,m_pContext);
	return bEmpty;
}

 bool PgMemcontext::contains(void *p) const
{
	checkValid();
	bool bContains = false;
	THROW_CALL(bContains = ::MemoryContextContains,m_pContext,p);
	return bContains;
}

void PgMemcontext::getStat(bool memleak, int depth, size_t size)
{
	m_pContext->name; 
	checkValid();
#if defined(ALLOC_INFO)
	THROW_CALL(::MemoryContextStats, m_pContext, memleak, depth, size);
#else
	THROW_CALL(::MemoryContextStats, m_pContext);
#endif
}
void PgMemcontext::check()  
{
#ifdef MEMORY_CONTEXT_CHECKING
	checkValid();
	THROW_CALL(::MemoryContextCheck,m_pContext);
#endif
}

char *PgMemcontext::strDup(const char *s, size_t len )  
{
	checkValid();
	char* nstr = NULL;

	THROW_CALL(nstr = (char *) MemoryContextAlloc,m_pContext, len);
	memcpy(nstr, s, len);

	return nstr;
}

void PgMemcontext::reset(bool onlyChildren)
{
	checkValid();
	
	if (!onlyChildren)
	{
		THROW_CALL(::MemoryContextReset,m_pContext,true);
	} else {
	    THROW_CALL(::MemoryContextResetChildren,m_pContext);
    }
}

std::string PgMemcontext::getContextName() const
{
	return std::string (m_pContext->name);
}

void PgMemcontext::checkValid( void )const
{
    if (NULL == m_pContext)
    {
		throw StorageEngineExceptionUniversal(LOGIC_ERR,"havn't invoke create on this memory context!");
    }
}


#undef THROW_CALL
}
}
