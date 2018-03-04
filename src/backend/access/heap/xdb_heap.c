#ifdef FOUNDER_XDB_SE
#include "postgres.h"

#include "access/xdb_heap.h"
#include "utils/rel.h"
#include "access/heapam.h"
#include "access/relscan.h"

Oid	RangeVarGetRelid(const RangeVar *relation, bool failOK)
{
    return 0;
}


static bool test(char *str1, int len1, char *str2, int len2, CompareCallback compareFunction, StrategyNumber strategyNum);

extern TupleDesc single_attibute_tupDesc;
bool fdxdb_HeapKeyTest(HeapTuple tuple, 
					   HeapScanDesc scan,
					   int nkeys, 
					   ScanKey keys 
					   ) 
{
    Relation rel = scan->rs_rd;
	Datum * values = (Datum *) palloc(sizeof(Datum));
	bool * isnull = (bool *) palloc(sizeof(bool));
	char *pstr0 = NULL;
	int len0 = 0;
	bool needDelete = false;
	heap_deform_tuple(tuple, single_attibute_tupDesc, values, isnull);
	if (VARATT_IS_COMPRESSED(values[0]) 
		|| VARATT_IS_EXTERNAL(values[0]))
	{
		char *text_to_cstring_with_len(const text *t, int &len);
		pstr0 = text_to_cstring_with_len((text *) DatumGetPointer(values[0]), len0);
		needDelete = true;
	}
	else
	{
		pstr0 = VARDATA_ANY(DatumGetCString(values[0]));
		len0 = VARSIZE_ANY_EXHDR(DatumGetCString(values[0]));
	}

	ScanKey cur_key = keys;
	int  cur_nkeys = nkeys;

	for( ; nkeys--; cur_key++)
	{
        RangeData rang = {0};
		rang.userData = scan->rs_rd->user_data;
        rel->rd_colinfo->split_function(rang, pstr0, cur_key->sk_attno, len0);
		int start = (int)rang.start;
		int len1 = (int)rang.len;
		char*pstr2 = (char*)cur_key->sk_address;
		int len2 = cur_key->sk_arglen;
		if(!test(&pstr0[start], len1, pstr2, len2, cur_key->sk_compfun, cur_key->sk_strategy))
		{
			if(needDelete)
			{
				pfree(pstr0);
			}
			if (values)
				pfree(values);
			if (isnull)
				pfree(isnull);
			return false;
		}
	}
    
    // when control reaches here, all scan keys are met, do filtering if necessary.
    bool ret = true;
    if (scan->filter_func && scan->filter_func(scan->user_data, pstr0, len0) == false)
        ret = false;
	if(needDelete)
	{
		pfree(pstr0);
	}
	if (values)
		pfree(values);
	if (isnull)
		pfree(isnull);
	
	return ret;
}

extern Datum *get_index_keys(Relation heapRel, Relation indexRel, HeapTuple tup, size_t &nIndexKeys);
bool fdxdb_HeapKeyTestNew(HeapTuple tuple, void *pindexScan) 
{
	IndexScanDesc indexScan = (IndexScanDesc)pindexScan;
	int nScankeys = indexScan->numberOfKeys;
	ScanKey scankeys = indexScan->keyData;
	Relation heapRel = indexScan->heapRelation;
	Relation indexRel = indexScan->indexRelation;

	bool match = false;

	size_t nIndexKeys = 0;
	Datum *pDatums = get_index_keys(heapRel, indexRel, tuple, nIndexKeys);

	if((pDatums == NULL) || (nIndexKeys == 0))
	{
		return match;
	}

	for(size_t i = 0; i < nIndexKeys; i++)
	{
		char *pstr = VARDATA_ANY(DatumGetCString(pDatums[i]));
		int len = VARSIZE_ANY_EXHDR(DatumGetCString(pDatums[i]));

		int iScanKey = 0;
		for(iScanKey = 0; iScanKey < nScankeys; iScanKey++)
		{
			ScanKey curScankey = &scankeys[iScanKey];
			RangeData rang = {0};
			rang.userData = MtInfo_GetUserData(indexRel->mt_info);
			indexRel->rd_colinfo->split_function(rang, pstr, curScankey->sk_attno, len);

			if(!test((char*)&pstr[rang.start], (int)rang.len,
					 (char*)curScankey->sk_address, (int)curScankey->sk_arglen,
					 curScankey->sk_compfun, curScankey->sk_strategy))
			{
				break;
			}
		}
		if(iScanKey == nScankeys)
		{
			match = true;
			break;
		}
	}

	for (size_t i = 0; i < nIndexKeys; i++)
	{
		pfree(DatumGetPointer(pDatums[i]));
	}
	pfree(pDatums);

	return match;
}

static bool test(char *str1, int len1, char *str2, int len2, CompareCallback compareFunction, StrategyNumber strategyNum)
{
	Assert(strategyNum >= BTLessStrategyNumber && strategyNum <= BTGreaterStrategyNumber);
	int result = compareFunction(str1, len1, str2, len2);
	if (strategyNum == BTLessStrategyNumber)
		return result < 0;
	if (strategyNum == BTLessEqualStrategyNumber)
		return result <= 0;
	if (strategyNum == BTEqualStrategyNumber)
		return result == 0;
	if (strategyNum == BTGreaterEqualStrategyNumber)
		return result >= 0;
	if (strategyNum == BTGreaterStrategyNumber)
		return result > 0;
	return false;
}
#endif //FOUNDER_XDB_SE


