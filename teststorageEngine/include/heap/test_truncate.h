/***********************************************************************
* Description: truncate feature test
*
*
***********************************************************************/

#ifndef __TEST_HEAP_TRUNCATE__
#define __TEST_HEAP_TRUNCATE__


extern bool testtruncate_NoIndex(bool is_toast);
extern bool testtruncate_OneIndex(bool is_toast, bool is_unique);
extern bool testtruncate_TwoIndexes(bool is_toast);

/*
* test no index
*/
static inline bool testtruncate_HeapNoIndex(void)
{
	return testtruncate_NoIndex(false);
}
static inline bool testtruncate_ToastNoIndex(void)
{
	return testtruncate_NoIndex(true);
}

/*
* test btree index
*/
static inline bool testtruncate_HeapOneBtreeIndex(void)
{
	return testtruncate_OneIndex(false, false);
}
static inline bool testtruncate_HeapOneBtreeUniqueIndex(void)
{
	return testtruncate_OneIndex(false, true);
}

/*
* test btree unique index
*/
static inline bool testtruncate_ToastOneBtreeIndex(void)
{
	return testtruncate_OneIndex(true, false);
}
static inline bool testtruncate_ToastOneBtreeUniqueIndex(void)
{
	return testtruncate_OneIndex(true, true);
}

/*
* test two index
*/
static inline bool testtruncate_HeapTwoIndexes(void)
{
	return testtruncate_TwoIndexes(false);
}
static inline bool testtruncate_ToastTwoIndexes(void)
{
	return testtruncate_TwoIndexes(true);
}

extern bool testtruncate_StopEngine_step1(void);
extern bool testtruncate_StopEngine_step2(void);

#endif

