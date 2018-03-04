bool test_create_drop_heap();
bool test_heap_drop();
bool heap_drop_OpenWithoutClose();
extern int rid;
extern int THREAD_RID;
extern void startTest_CreateHeap();
extern void finishTest_DropHeap();


//创建多个线程同时删除同一张表
int test_thread_heap_drop_000();

//创建多个线程查询和删除同一张表
int test_thread_heap_drop_001();

//创建多个线程插入和删除表
int test_thread_heap_drop_002();
