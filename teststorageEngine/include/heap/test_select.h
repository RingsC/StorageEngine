extern int RID;

bool test_heap_sqscan_000();
bool test_heap_sqscan_001();
bool test_heap_sqscan_002();
bool test_heap_sqscan_003();

//无 scankey 向前扫描
bool myTest_test_heap_sqscan_000();

//多个 scankey 从前向后扫描，扫描条件包含：等于，大于，小于
bool myTest_test_heap_sqscan_001();

//多个 scankey 扫描查询任意列的数据，从后向前扫描，扫描规则包括：等于，大于，小于
bool myTest_test_heap_sqscan_002();

//同一列设置多个 scankey ，扫描条件包含：等于，大于，小于
bool myTest_test_heap_sqscan_003();

#define ROWS 100

#define INSERT_TEST "test_select_no_scankey_"
#define test_select_no_scankey "test_select_no_scankey_"