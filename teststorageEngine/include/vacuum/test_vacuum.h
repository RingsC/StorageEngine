#ifndef __TEST_VACUUM_H__
#define __TEST_VACUUM_H__

bool testvacuum_SmallHeap(void);
bool testvacuum_LargeHeap(void);
bool testvacuum_SmallToast(void);
bool testvacuum_LargeToast(void);

bool testvacuum_SmallHeapWithIndex(void);
bool testvacuum_LargeHeapWithIndex(void);
bool testvacuum_SmallToastWithIndex(void);
bool testvacuum_LargeToastWithIndex(void);

bool testvacuum_ConTestSimpleHeap(void);
bool testvacuum_ConTestToast(void);
bool testvacuum_ConTestHeapWithIndex(void);
bool testvacuum_ConTestToastWithIndex(void);

bool testvacuum_Database(void);

#endif

