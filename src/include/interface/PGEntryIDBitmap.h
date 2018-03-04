#ifndef PG_ENTRY_ID_BITMAP_H
#define PG_ENTRY_ID_BITMAP_H

#include "EntryIDBitmap.h"
#include "nodes/tidbitmap.h"
#include "Transaction.h"
#include "nodes/execnodes.h"


namespace FounderXDB{
namespace StorageEngineNS {

class EntryIDBitmapIterator; 
class PGEntryIDBitmap :public EntryIDBitmap
{
private:
	PGEntryIDBitmap(const PGEntryIDBitmap &other);
	const PGEntryIDBitmap&operator=(const PGEntryIDBitmap&);
public:
	PGEntryIDBitmap(TIDBitmap* pgtidbitmap,Transaction *txn);
	PGEntryIDBitmap(TIDBitmap* pgtidbitmap,Transaction *txn,IndexScanDesc indexScanDesc);
	virtual ~PGEntryIDBitmap(); // tbm_free
	virtual void doUnion(const EntryIDBitmap& other);
	virtual void doIntersect(const EntryIDBitmap& other);
	virtual bool isEmpty();
	virtual EntryIDBitmapIterator *beginIterate();
	virtual void endIterate(EntryIDBitmapIterator *);
private:
	// implementation class should hold a TidBitmap structure.
	TIDBitmap* m_tidbitmap;
	Transaction *m_curtxn;
	IndexScanDesc m_IndexScan;
};

class PGEntryIDBitmapIterator :public EntryIDBitmapIterator
{
public:
	PGEntryIDBitmapIterator(TBMIterator* pgtbmiterator,Transaction *txn);
	PGEntryIDBitmapIterator(BitmapHeapScanState* pgtbmScanState, Transaction *txn);
	~PGEntryIDBitmapIterator();
	// caller doesn't need to free the returned object, this class takes care of it.
	virtual int getNext(EntryID& eid, DataItem& item);
	BitmapHeapScanState* getBmpScanState();

public:
	TBMIterator* m_tbmiterator;
private:
	Transaction *m_curtxn;
	BitmapHeapScanState* m_tbmScanState;
};

}
}
#endif //PG_ENTRY_ID_BITMAP_H
