#ifdef FOUNDER_XDB_SE
#include <string.h>
#include "StorageEngine.h"
#include "MemoryContext.h"
#include "XdbLock.h"
#include "raptor2.h"
#include "pthread.h"
#include <map>
#include <cassert>

using namespace FounderXDB::StorageEngineNS;

MemoryContext*  GRAPH_TOP_MEMCONTEXT = NULL;
THREAD_LOCAL MemoryContext* GRAPH_STORAGE_MEMCONTEXT = NULL;
THREAD_LOCAL MemoryContext* GRAPH_MODEL_MEMCONTEXT = NULL;
THREAD_LOCAL MemoryContext* GRAPH_PARSER_MEMCONTEXT = NULL;

THREAD_LOCAL MemoryContext* GRAPH_CURRENT_MEMCONTEXT = NULL;

MemoryContext* get_current_txn_mcx()
{
	MemoryContext* mcx = NULL;
	Transaction* pTran = StorageEngine::getStorageEngine()->GetCurrentTransaction();
	if (pTran)
	{
		mcx = pTran->getAssociatedMemoryContext();
	}

	return mcx;
}

void* mcxalloc(size_t size)
{
	if (!GRAPH_CURRENT_MEMCONTEXT)
	{
		GRAPH_CURRENT_MEMCONTEXT = GRAPH_TOP_MEMCONTEXT;
	}
	
	return GRAPH_CURRENT_MEMCONTEXT->alloc(size);
}
void* mcxalloc0(size_t size)
{
	void* p = mcxalloc(size);
	memset(p,0,size);
	return p;
}
void* remcxalloc(void* pOld, size_t size)
{
	if (!pOld)
		return mcxalloc(size);
	
	return MemoryContext::reAlloc(pOld,size);
}
void mcxfree(void* p)
{
  if(p != NULL)
  {
  	MemoryContext::deAlloc(p);
  }
}
void initGraphEngineTopMcx()
{
	if (!GRAPH_TOP_MEMCONTEXT)
	{
		MemoryContext *cxt = MemoryContext::getMemoryContext(MemoryContext::ProcessTop);
		MemoryContext* top_cxt = MemoryContext::getMemoryContext(MemoryContext::AlwaysCreate);
		top_cxt->create("GraphTop",cxt);
		GRAPH_TOP_MEMCONTEXT = top_cxt;
	}
	if (!GRAPH_MODEL_MEMCONTEXT)
	{
		GRAPH_MODEL_MEMCONTEXT = create_graph_mcx("model",GRAPH_TOP_MEMCONTEXT);
	}
	if (!GRAPH_STORAGE_MEMCONTEXT)
	{
		GRAPH_STORAGE_MEMCONTEXT = create_graph_mcx("storage",GRAPH_TOP_MEMCONTEXT);
	}
	if (!GRAPH_PARSER_MEMCONTEXT)
	{
		GRAPH_PARSER_MEMCONTEXT = create_graph_mcx("parser",GRAPH_TOP_MEMCONTEXT);
	}
}
void releaseGraphEngineTopMcx()
{
	if (GRAPH_MODEL_MEMCONTEXT)
	{
		GRAPH_MODEL_MEMCONTEXT->destroy();
		GRAPH_MODEL_MEMCONTEXT = NULL;
	}
	if (GRAPH_STORAGE_MEMCONTEXT)
	{
		GRAPH_STORAGE_MEMCONTEXT->destroy();
		GRAPH_STORAGE_MEMCONTEXT = NULL;
	}
	if (GRAPH_PARSER_MEMCONTEXT)
	{
		GRAPH_PARSER_MEMCONTEXT->destroy();
		GRAPH_PARSER_MEMCONTEXT = NULL;
	}

	if (GRAPH_TOP_MEMCONTEXT)
	{
		GRAPH_TOP_MEMCONTEXT->destroy();
		GRAPH_TOP_MEMCONTEXT = NULL;
	}
}
MemoryContext* create_graph_mcx(const char* name, MemoryContext* parent)
{
	MemoryContext* pParent = parent;

	if (!pParent)
		pParent = GRAPH_TOP_MEMCONTEXT;

	MemoryContext* new_cxt = MemoryContext::getMemoryContext(MemoryContext::AlwaysCreate);
	new_cxt->create(name,pParent);

	return new_cxt;	
}
MemoryContext* switch2mcx(MemoryContext* memcxt)
{
	assert(memcxt);

	MemoryContext* old = GRAPH_CURRENT_MEMCONTEXT;

	GRAPH_CURRENT_MEMCONTEXT = memcxt;

	return old;
}
MemoryContext* switch2StorageMcx()
{
	return switch2mcx(GRAPH_STORAGE_MEMCONTEXT);
}
MemoryContext* switch2ModelMcx()
{
	return switch2mcx(GRAPH_MODEL_MEMCONTEXT);
}
MemoryContext* switch2ParserMcx()
{
	return switch2mcx(GRAPH_PARSER_MEMCONTEXT);
}
MemoryContext* switch2CurTxnMcx()
{
	return switch2mcx(get_current_txn_mcx());
}
MemoryContext* switch2TopMcx()
{
	return switch2mcx(GRAPH_TOP_MEMCONTEXT);
}

#endif