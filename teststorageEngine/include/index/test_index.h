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

//测试多线程利用索引扫描表
int test_thread_indexscan_000();

/* 测试多线程利用索引扫描大量相同数据 */
int test_thread_indexscan_001();

/* 测试多线程创建多个表和多个索引 */
int test_thread_indexscan_002();

/* 测试多线程在单表上创建多个索引扫描 */
int test_thread_indexscan_003();

/* 测试多线程使用变长字段索引扫描 */
int test_thread_indexscan_004();

/*测试重建索引*/
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