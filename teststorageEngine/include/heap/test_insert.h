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

//Ϊ���Դ���һ�ű�


//ɾ�������������ı�


//���Ժ���
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

//����vacuum��pgstat�߳�
int test_pgstat_vacuum();

//����index����vacuum��pgstat�߳�
int test_index_pgstat_vacuum();

//����toast����vacuum��pgstat�߳�
int test_toast_pgstat_vacuum();

//����toast��������vacuum��pgstat�߳�
int test_index_toast_pgstat_vacuum();

//����vacuum�ܷ�truncate
int test_vacuum_truncate();

//���Զ���߳�ͬʱ��һ�ű��в�����ͬ����
int test_hq_thread_insert_000();

//���Զ���߳�����ͬ���в�������
int test_hq_thread_insert_001();

//�������������̲߳���Ͳ�������
int test_hq_thread_insert_002();

bool myTest_simple_heap_insert_000();
bool myTest_simple_heap_insert_001();
bool myTest_simple_heap_insert_002();
bool myTest_simple_heap_insert_003();