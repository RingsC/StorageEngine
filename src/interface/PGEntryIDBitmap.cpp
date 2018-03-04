#include "postgres.h"
#include "interface/PGEntryIDBitmap.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/utils.h"
#include "access/relscan.h"


using namespace FounderXDB::StorageEngineNS;

namespace FounderXDB{
namespace StorageEngineNS {
	
#define THROW_CALL(FUNC,...)   \
	bool flag = false;\
	PG_TRY(); {\
	FUNC(__VA_ARGS__);\
} PG_CATCH(); { \
	flag=true;   \
} PG_END_TRY(); \
	if (flag)\
{\
	ThrowException(); \
}
	
PGEntryIDBitmap::PGEntryIDBitmap(TIDBitmap* pgtidbitmap,Transaction *txn):m_tidbitmap(pgtidbitmap),m_curtxn(txn)
{
}
PGEntryIDBitmap::PGEntryIDBitmap(TIDBitmap* pgtidbitmap,Transaction *txn,IndexScanDesc indexScanDesc):m_tidbitmap(pgtidbitmap),m_curtxn(txn),m_IndexScan(indexScanDesc)
{
}
PGEntryIDBitmap::~PGEntryIDBitmap()
{
	THROW_CALL(tbm_free,m_tidbitmap)
}

void PGEntryIDBitmap::doUnion(const EntryIDBitmap& other)
{
	const PGEntryIDBitmap* pgother = static_cast<const PGEntryIDBitmap*>(&other); 
	THROW_CALL(tbm_union,m_tidbitmap,pgother->m_tidbitmap)
}

void PGEntryIDBitmap::doIntersect(const EntryIDBitmap& other)
{
	const PGEntryIDBitmap* pgother = static_cast<const PGEntryIDBitmap*>(&other); 
	THROW_CALL(tbm_intersect,m_tidbitmap,pgother->m_tidbitmap)
}

bool PGEntryIDBitmap::isEmpty()
{
	bool ret = false;
	THROW_CALL(ret = tbm_is_empty,m_tidbitmap)
	return ret;
}

EntryIDBitmapIterator* PGEntryIDBitmap::beginIterate()
{
	BitmapHeapScanState* bmpScanState = NULL;
	THROW_CALL(bmpScanState = ExecInitBitmapHeapScanNew, m_IndexScan, m_tidbitmap)

	MemoryContext *cxt = m_curtxn->getAssociatedMemoryContext();
	return new(*cxt) PGEntryIDBitmapIterator(bmpScanState,m_curtxn);//·Åµ½txn memcxtÖÐ
}
void PGEntryIDBitmap::endIterate(EntryIDBitmapIterator* tbmIterator)
{
	PGEntryIDBitmapIterator* pgtbmIterator = (PGEntryIDBitmapIterator*)tbmIterator;
	THROW_CALL(ExecEndBitmapHeapScan,pgtbmIterator->getBmpScanState())
}
PGEntryIDBitmapIterator::PGEntryIDBitmapIterator(TBMIterator* pgtbmiterator,Transaction *txn):m_tbmiterator(pgtbmiterator), m_curtxn(txn)
{
	/*MemoryContext *cxt =*/ m_curtxn->getAssociatedMemoryContext();
}
PGEntryIDBitmapIterator::PGEntryIDBitmapIterator(BitmapHeapScanState* pgtbmScanState, Transaction *txn):m_curtxn(txn),m_tbmScanState(pgtbmScanState)
{
	
}

PGEntryIDBitmapIterator::~PGEntryIDBitmapIterator()
{
}

int PGEntryIDBitmapIterator::getNext(EntryID& eid, DataItem& item)
{
	HeapTuple tuple = NULL;
	THROW_CALL(tuple = BitmapHeapNext,m_tbmScanState);
	if (NULL == tuple)
		return NO_DATA_FOUND;

	TidToEntryId(tuple->t_self,eid);

	int len = 0;
	char* pData = FDPG_Common::fd_tuple_to_chars_with_len(tuple,len);
	item.setData(pData);
	item.setSize(len);

	return 0;
}

BitmapHeapScanState* PGEntryIDBitmapIterator::getBmpScanState()
{
	return m_tbmScanState;
}

}
}