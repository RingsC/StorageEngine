extern void begin_transaction();
extern void end_transaction();
extern void userAbort_transaction();
extern void begin_transaction_after_abort();
extern int RID;
extern int THREAD_RID;

bool test_indexscan_000();
bool test_indexscan_001();
bool test_indexscan_002();
bool test_indexscan_003();
bool test_indexscan_004();
bool test_thread_index_scan_005();
bool test_thread_index_scan_006();
bool test_thread_index_scan_007();
bool test_thread_index_scan_008();
bool test_thread_index_scan_009();
bool test_thread_index_create_010();

bool myTest_test_indexscan_000();

bool test_create_index_abort();

//���Զ��߳���������ɨ���
int test_thread_indexscan_000();

/* ���Զ��߳���������ɨ�������ͬ���� */
int test_thread_indexscan_001();

/* ���Զ��̴߳��������Ͷ������ */
int test_thread_indexscan_002();

/* ���Զ��߳��ڵ����ϴ����������ɨ�� */
int test_thread_indexscan_003();

/* ���Զ��߳�ʹ�ñ䳤�ֶ�����ɨ�� */
int test_thread_indexscan_004();

/*�����ؽ�����*/
int test_reindex_index();
int test_reindex_index_Unique_delete();
int test_reindex_index_Unique_Update();
int test_reindex_relation();
int test_reindex_relation_toast();
bool test_reindex_database();
bool test_reindex_index_mult_thread();
bool test_reindex_relation_mult_thread();
bool test_reindex_database_mult_thread();
/*hash index*/
bool HashIndexTest();
bool ReIndexHashIndexTest();
bool HashIndexToastTest();
bool HashIndexBitmapTest();
bool HashIndexMultTest();
bool ReInexHashIndexMultTest();
bool ToastHashIndexMultTest();