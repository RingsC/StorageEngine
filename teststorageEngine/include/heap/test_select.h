extern int RID;

bool test_heap_sqscan_000();
bool test_heap_sqscan_001();
bool test_heap_sqscan_002();
bool test_heap_sqscan_003();

//�� scankey ��ǰɨ��
bool myTest_test_heap_sqscan_000();

//��� scankey ��ǰ���ɨ�裬ɨ���������������ڣ����ڣ�С��
bool myTest_test_heap_sqscan_001();

//��� scankey ɨ���ѯ�����е����ݣ��Ӻ���ǰɨ�裬ɨ�������������ڣ����ڣ�С��
bool myTest_test_heap_sqscan_002();

//ͬһ�����ö�� scankey ��ɨ���������������ڣ����ڣ�С��
bool myTest_test_heap_sqscan_003();

#define ROWS 100

#define INSERT_TEST "test_select_no_scankey_"
#define test_select_no_scankey "test_select_no_scankey_"