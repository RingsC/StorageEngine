bool test_heap_multcreate();
extern int rid;
extern void startTest_CreateHeap();
extern void finishTest_DropHeap();

//������ʱ��Ĵ�������ɾ�Ĳ����
int simple_test_temprel();

//���Զ��߳��������ʱ��Ĵ�������ɾ�Ĳ����
int thread_test_temprel();

//���Զ��߳�����´���20000����ʱ��(��ʱ���id��15000�ķ�Χ��ѭ��)
int thread_test_maxnum_create_tmprel();

//���Զ���߳�ͬʱ������ͬid�ı�
int test_thread_create_heap_000();

//���Զ���߳�ͬʱ������ͬid�ı�
int test_thread_create_heap_001();

//���������̴߳�������1000����
int test_thread_create_heap_002();
