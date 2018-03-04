/*-------------------------------------------------------------------------
 *
 * scankey.c
 *	  scan key support code
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/access/common/scankey.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/skey.h"
#ifndef FOUNDER_XDB_SE
#include "catalog/pg_collation.h"
#else
#include "access/xdb_common.h"
#endif //FOUNDER_XDB_SE

/*
 * ScanKeyEntryInitialize
 *		Initializes a scan key entry given all the field values.
 *		The target procedure is specified by OID (but can be invalid
 *		if SK_SEARCHNULL or SK_SEARCHNOTNULL is set).
 *
 * Note: CurrentMemoryContext at call should be as long-lived as the ScanKey
 * itself, because that's what will be used for any subsidiary info attached
 * to the ScanKey's FmgrInfo record.
 */
void
ScanKeyEntryInitialize(ScanKey entry,
					   int flags,
					   AttrNumber attributeNumber,
					   StrategyNumber strategy,
					   Oid subtype,
					   Oid collation,
					   RegProcedure procedure,
					   Datum argument)
{
	entry->sk_flags = flags;
	entry->sk_attno = attributeNumber;
	entry->sk_strategy = strategy;
	entry->sk_subtype = subtype;
	entry->sk_collation = collation;
	entry->sk_argument = argument;
	if (RegProcedureIsValid(procedure))
	{
		fmgr_info(procedure, &entry->sk_func);
	}
	else
	{
		Assert(flags & (SK_SEARCHNULL | SK_SEARCHNOTNULL));
		MemSet(&entry->sk_func, 0, sizeof(entry->sk_func));
	}
}

/*
 * ScanKeyInit
 *		Shorthand version of ScanKeyEntryInitialize: flags and subtype
 *		are assumed to be zero (the usual value), and collation is defaulted.
 *
 * This is the recommended version for hardwired lookups in system catalogs.
 * It cannot handle NULL arguments, unary operators, or nondefault operators,
 * but we need none of those features for most hardwired lookups.
 *
 * We set collation to DEFAULT_COLLATION_OID always.  This is appropriate
 * for textual columns in system catalogs, and it will be ignored for
 * non-textual columns, so it's not worth trying to be more finicky.
 *
 * Note: CurrentMemoryContext at call should be as long-lived as the ScanKey
 * itself, because that's what will be used for any subsidiary info attached
 * to the ScanKey's FmgrInfo record.
 */
void
ScanKeyInit(ScanKey entry,
			AttrNumber attributeNumber,
			StrategyNumber strategy,
			RegProcedure procedure,
			Datum argument)
{
	entry->sk_flags = 0;
	entry->sk_attno = attributeNumber;
	entry->sk_strategy = strategy;
	entry->sk_subtype = InvalidOid;
	entry->sk_collation = DEFAULT_COLLATION_OID;
	entry->sk_argument = argument;
	fmgr_info(procedure, &entry->sk_func);
}

/*
 * ScanKeyEntryInitializeWithInfo
 *		Initializes a scan key entry using an already-completed FmgrInfo
 *		function lookup record.
 *
 * Note: CurrentMemoryContext at call should be as long-lived as the ScanKey
 * itself, because that's what will be used for any subsidiary info attached
 * to the ScanKey's FmgrInfo record.
 */
void
ScanKeyEntryInitializeWithInfo(ScanKey entry,
							   int flags,
							   AttrNumber attributeNumber,
							   StrategyNumber strategy,
							   Oid subtype,
							   Oid collation,
							   FmgrInfo *finfo,
							   Datum argument)
{
	entry->sk_flags = flags;
	entry->sk_attno = attributeNumber;
	entry->sk_strategy = strategy;
	entry->sk_subtype = subtype;
	entry->sk_collation = collation;
	entry->sk_argument = argument;
	fmgr_info_copy(&entry->sk_func, finfo, CurrentMemoryContext);
}

#ifdef FOUNDER_XDB_SE
void
Fdxdb_ScanKeyInitWithCallbackInfo(ScanKey entry,
			AttrNumber attributeNumber,
			StrategyNumber strategy,
			CompareCallback compareFunction,
			Datum argument)
{
	entry->sk_flags = 0;
	entry->sk_attno = attributeNumber;
	entry->sk_strategy = strategy;
	entry->sk_subtype = InvalidOid;
	entry->sk_collation = DEFAULT_COLLATION_OID; 
	entry->sk_compfun = compareFunction;
	if(0 == argument)
	{
		entry->sk_flags |= (SK_ISNULL | SK_SEARCHNOTNULL);
	}
	entry->sk_argument = argument;
	if (0 != argument)
	{
		entry->sk_address = (size_t)VARDATA_ANY(DatumGetCString(argument));;
		entry->sk_arglen = VARSIZE_ANY_EXHDR(DatumGetCString(argument));
	}
}
#endif //FOUNDER_XDB_SE

void freeScanKeys(ScanKey entry,int keyNum)
{
	for(int i = 0; i < keyNum; i++)
	{
		if(entry[i].sk_argument)
		{
			pfree(DatumGetPointer(entry[i].sk_argument));			
		}
	}
	pfree(entry);
}
