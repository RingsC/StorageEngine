bool test_heap_close();
bool test_heap_close_WrongLock();
extern int rid;

extern void startTest_CreateHeap();
extern void finishTest_DropHeap();
/************************************************************************** 
* @test_thread_heap_close
* ��������̣߳��򿪱�Ȼ��ر�
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_heap_close();

/************************************************************************** 
* @thread_simple_heap_close 
* ͨ����֤�Ѿ��رյı����Ƿ��ܹ�����������֤��Ĺر�
* Detailed description.
* @param[in] void 
**************************************************************************/
void thread_simple_heap_close(void *param,Oid rid,int sta);

extern void prepare_param(BackendParameters *paramArr[],const int nThread);

extern void free_param(BackendParameters *paramArr[],const int nThread);

extern void thread_create_heap_xy(int nTables,const int rel_id, const Colinfo heap_info);
extern void thread_drop_heap(int nTables);
extern int THREAD_RID;