#ifndef XDB_HEAP_HPP
#define XDB_HEAP_HPP
#include "access/htup.h"
#include "access/skey.h"
#include "access/heapam.h"

//catalog/namespace.h
struct RangeVar;
extern Oid	RangeVarGetRelid(const RangeVar *relation, bool failOK);  //not definition

extern bool fdxdb_HeapKeyTest(HeapTuple tuple, 
				   HeapScanDesc scan,
				   int nkeys, 
				   ScanKey keys 
				   );

extern bool fdxdb_HeapKeyTestNew(HeapTuple tuple, void *pIndexScan);

#endif