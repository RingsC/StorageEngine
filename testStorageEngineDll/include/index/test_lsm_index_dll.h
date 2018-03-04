#ifndef __TEST_LSM_INDEX_DLL_H__
#define __TEST_LSM_INDEX_DLL_H__

extern bool testlsmdll_Operation(void);
extern bool testlsmdll_InsertBigDataFirst(void);
extern bool testlsmdll_QueryBigData(void);
extern bool testlsmdll_BitmapScanBigData(void);
extern bool testlsmdll_ScanAll(void);
extern bool testlsmdll_InsertBigData(void);
extern bool testlsmdll_InternalMergeConcurrency(void);
extern bool testlsmdll_InternalInsertConcurrency(void);
extern bool testlsmdll_Concurrency(void);

#endif

