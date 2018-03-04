/**************************************************************************
* @file thread_heap_open.h
* @brief 
* @author ����
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
* ���������̷߳��źţ���Ӧ�������߳̽����ź�
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_heap_open();

/************************************************************************** 
* @test_thread_heap_drop_and_open 
* ���Զ���߳���ĳ����ɾ����ʱ������ȷ���յ���Ϣ������RelationIdCache��Ϣ
* Detailed description.
* @param[in] void 
**************************************************************************/
int test_thread_heap_drop_and_open();

/************************************************************************** 
* @thread_simple_heap_open 
* ���������̷߳��źţ���Ӧ�������߳̽����ź�
* Detailed description.
* @param[in] void 
**************************************************************************/
void thread_simple_heap_open(void *param,Oid rid);

extern void prepare_param(BackendParameters *paramArr[],const int nThread);

extern void free_param(BackendParameters *paramArr[],const int nThread);
extern void thread_drop_heap(int nTables);
extern void thread_create_heap_xy(int nTables,const int rel_id, const Colinfo heap_info);
extern int THREAD_RID;