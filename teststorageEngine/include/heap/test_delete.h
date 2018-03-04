//创建一张表，其Oid为10000
//插入若干元组
void beginTest();

//删除表
void endTest();

//测试删除数据表中第一个元组
int testDeleteFirstTupleFromHeap();

//插入多个不同元组，测试删除其中一个元组
int testDeleteOneTupleFromManyTuple();

//插入多个相同元组，测试删除其中一个元组
int testDeleteOneTupleFromTheSameTuple();

//测试由给定的步数删除一个元组
int testDeleteFromGiveSteps();

//测试删除数据表中不存在的元组
int testDeleteNoExistTuple();

//测试在没有CommandCounterIncrement的情况下连续删除同一个元组
int testDeleteTheSameTupleWithoutIncrement();

//删除表中一个唯一的元组，测试插入相同的元组再根据块号和偏移量去删除
int testDeleteByBlockAndOffset();

//插入一个新元组，再根据插入成功之后获取到的块信息删除该元组
int testDeleteByTupleInsertGetBlockAndOffset();

//获取表中testdata_3的块信息，并更新为unique_data
int testUpdateSimple();

//先删除一个元组但不increment然后更新该元组
int testUpdateAfterDeleteWithoutIncrement();

//随即生成表的块信息，然后更新
int testUpdateOnNoExistBlock();

//连着多次update同一个元组但只在最后一次increment，测试查看结果
int testUpdateTupleWithoutIncrementManyTimes();

//连着多次update同一个元组并且都increment，测试查看结果
int testUpdateTupleManyTimes();

//删除测试用例创建的表
void endTestNotStopEngine();

//删除测试用例创建的表
void beginTestNotStartEngine();



//多线程删除同一张表同一个数据的测试
int test_thread_delete_000();

//多线程删除同一张表所有数据的测试
int test_thread_delete_001();

//多线程删除同一张表不同数据的测试
int test_thread_delete_002();

//多线程相互删除不同表的信息
int test_thread_delete_003();

//多线程删除元组的同时查找元组
int test_thread_delete_004();



//多线程同时更新同一个元组
int test_thread_update_000();

//多线程顺序更新同一个元组(即在之前的基础上更新)
int test_thread_update_001();

//多线程查询的同时更新元组
int test_thread_update_002();



//多线程同步删除测试
int test_thread_sync_000();


extern void prepare_param(BackendParameters *paramArr[],const int nThread);
extern void free_param(BackendParameters *paramArr[],const int nThread);
extern void thread_insert_data(const char data[][DATA_LEN], 
						const int array_len, 
						const int data_len,
						const int rel_id );
extern int THREAD_RID;
extern void thread_drop_heap(int nTables);
extern void thread_create_heap_xy(int nTables,const int rel_id, const Colinfo heap_info);
extern bool check_long_result(Oid rid,char cmpData[][1024],const int dataRows);
extern void insert_data(const char insert_data[][DATA_LEN], 
						const int array_len, 
						const int data_len,
						const int rel_id);
/************************************************************************** 
* @test_thread_update_rollback 
* 测试更新后调用user_abort_transaction回滚
* Detailed description.
* @param[in] void 
**************************************************************************/
int test_thread_update_rollback();

/************************************************************************** 
* @thread_update_rollback 
* 启动新的线程，在这个线程中更新表的数据，但是在最后调用user_abort_tran
* saction()"事务将回滚，这个将在主线中检测
* Detailed description.
* @param[in] void 
**************************************************************************/
void thread_update_rollback(void *param,Oid rid);

int test_delete_no();
bool myTest_heap_delete_000();
bool myTest_heap_delete_001();