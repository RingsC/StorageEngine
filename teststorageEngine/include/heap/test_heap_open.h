/**************************************************************************
* @file thread_heap_open.h
* @brief 
* @author 黄晟
* @date 2011-11-09 15:05:25
* @version 1.0
**************************************************************************/
bool test_heap_open();
bool test_heap_open_notexists();
bool test_heap_open_mult_close();
bool test_heap_open_mult_withoutclose();
extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);
extern void startTest_CreateHeap();
extern void finishTest_DropHeap();

/************************************************************************** 
* @test_thread_heap_open 
* 测试两个线程发信号，对应的两个线程接收信号
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_heap_open();

/************************************************************************** 
* @test_thread_heap_drop_and_open 
* 测试多个线程在某个表删除后时候能正确接收到消息并更新RelationIdCache信息
* Detailed description.
* @param[in] void 
**************************************************************************/
int test_thread_heap_drop_and_open();

/************************************************************************** 
* @thread_simple_heap_open 
* 测试两个线程发信号，对应的两个线程接收信号
* Detailed description.
* @param[in] void 
**************************************************************************/
void thread_simple_heap_open(void *param,Oid rid);

extern void prepare_param(BackendParameters *paramArr[],const int nThread);

extern void free_param(BackendParameters *paramArr[],const int nThread);
extern void thread_drop_heap(int nTables);
extern void thread_create_heap_xy(int nTables,const int rel_id, const Colinfo heap_info);
extern int THREAD_RID;