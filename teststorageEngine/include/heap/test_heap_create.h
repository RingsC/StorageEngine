bool test_heap_multcreate();
extern int rid;
extern void startTest_CreateHeap();
extern void finishTest_DropHeap();

//测试临时表的创建和增删改查操作
int simple_test_temprel();

//测试多线程情况下临时表的创建和增删改查操作
int thread_test_temprel();

//测试多线程情况下创建20000张临时表(临时表的id在15000的范围内循环)
int thread_test_maxnum_create_tmprel();

//测试多个线程同时创建相同id的表
int test_thread_create_heap_000();

//测试多个线程同时创建不同id的表
int test_thread_create_heap_001();

//测试若干线程创建至少1000个表
int test_thread_create_heap_002();
