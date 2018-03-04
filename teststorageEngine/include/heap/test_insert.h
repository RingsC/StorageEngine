#define rows 10
#define MANYROWS 10000
#define test_insert_data_one_row "test_insert_data_one_row_"
#define test_insert_data_001_char255 "testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
		   testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
		   estinserttestinserttestinserttestinserttesti"

#define test_insert_data_001_char256 "testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
		   testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
		   estinserttestinserttestinserttestinserttestin"

#define test_insert_long_data_len_1020 "testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
		   testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
		   estinserttestinserttestinserttestinserttestitestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
		   testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
		   estinserttestinserttestinserttestinserttestitestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
		   testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
		   estinserttestinserttestinserttestinserttestitestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert\
		   testinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
		   estinserttestinserttestinserttestinserttesti"

#define test_insert_long_data_len_1000 "testinserttestinserttestinserttestinserttestinserttestinsertte\
stinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttest\
inserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestin\
serttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinser\
ttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertte\
stinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestin\
serttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsertt\
estinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttesti\
nserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinser\
ttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinserttestinsert"

//为测试创建一张表


//删除测试所创建的表


//测试函数
bool test_simple_heap_insert_000();
bool test_simple_heap_insert_001();
bool test_simple_heap_insert_002();
bool test_simple_heap_insert_003();
bool test_simple_heap_insert_004();
bool test_simple_heap_insert_005();
bool test_simple_heap_insert_006();
bool test_simple_heap_insert_007();
bool test_simple_heap_insert_008();
bool test_simple_heap_insert_010();

bool test_thread_simple_heap_insert_011();
bool test_thread_simple_heap_insert_012();
bool test_thread_simple_heap_insert_013();
bool test_thread_simple_heap_insert_014();
bool test_thread_simple_heap_insert_015();
bool test_thread_create_heap_016();
bool test_thread_simple_heap_insert_017();
bool test_thread_simple_heap_insert_018();
bool test_thread_create_heap_019();
bool test_thread_simple_heap_insert_020();
bool test_thread_simple_heap_insert_021();
bool test_thread_simple_heap_insert_022();
bool test_thread_transaction_heap_insert_023_step1();
bool test_thread_transaction_heap_insert_023_step2();
bool test_thread_transaction_heap_insert_023();
bool test_thread_transaction_heap_insert_024_step1();
bool test_thread_transaction_heap_insert_024_step2();
bool test_thread_transaction_heap_insert_025_step1();
bool test_thread_transaction_heap_insert_025_step2();
bool test_thread_transaction_heap_insert_026_step1();
bool test_thread_transaction_heap_insert_026_step2();
bool test_thread_transaction_heap_insert_027_step1();
bool test_thread_transaction_heap_insert_027_step2();
bool test_thread_transaction_heap_insert_028_step1();
bool test_thread_transaction_heap_insert_028_step2();
bool test_thread_transaction_heap_insert_024();
bool test_thread_transaction_heap_insert_025();
bool test_thread_transaction_heap_insert_026();
bool test_thread_transaction_heap_insert_027();
bool test_thread_transaction_heap_insert_028();
bool test_thread_transaction_heap_insert_029();
bool test_thread_transaction_heap_insert_030();
bool test_thread_transaction_heap_insert_031();
bool test_thread_vacuum_lt();
bool testInsertLotsOfData( void );
bool testWalsender();
bool testWalreceiver();

//测试vacuum和pgstat线程
int test_pgstat_vacuum();

//创建index测试vacuum和pgstat线程
int test_index_pgstat_vacuum();

//创建toast测试vacuum和pgstat线程
int test_toast_pgstat_vacuum();

//创建toast索引测试vacuum和pgstat线程
int test_index_toast_pgstat_vacuum();

//测试vacuum能否truncate
int test_vacuum_truncate();

//测试多个线程同时往一张表中插入相同数据
int test_hq_thread_insert_000();

//测试多个线程往不同表中插入数据
int test_hq_thread_insert_001();

//测试启动若干线程插入和查找数据
int test_hq_thread_insert_002();

bool myTest_simple_heap_insert_000();
bool myTest_simple_heap_insert_001();
bool myTest_simple_heap_insert_002();
bool myTest_simple_heap_insert_003();