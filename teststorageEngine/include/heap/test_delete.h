//����һ�ű���OidΪ10000
//��������Ԫ��
void beginTest();

//ɾ����
void endTest();

//����ɾ�����ݱ��е�һ��Ԫ��
int testDeleteFirstTupleFromHeap();

//��������ͬԪ�飬����ɾ������һ��Ԫ��
int testDeleteOneTupleFromManyTuple();

//��������ͬԪ�飬����ɾ������һ��Ԫ��
int testDeleteOneTupleFromTheSameTuple();

//�����ɸ����Ĳ���ɾ��һ��Ԫ��
int testDeleteFromGiveSteps();

//����ɾ�����ݱ��в����ڵ�Ԫ��
int testDeleteNoExistTuple();

//������û��CommandCounterIncrement�����������ɾ��ͬһ��Ԫ��
int testDeleteTheSameTupleWithoutIncrement();

//ɾ������һ��Ψһ��Ԫ�飬���Բ�����ͬ��Ԫ���ٸ��ݿ�ź�ƫ����ȥɾ��
int testDeleteByBlockAndOffset();

//����һ����Ԫ�飬�ٸ��ݲ���ɹ�֮���ȡ���Ŀ���Ϣɾ����Ԫ��
int testDeleteByTupleInsertGetBlockAndOffset();

//��ȡ����testdata_3�Ŀ���Ϣ��������Ϊunique_data
int testUpdateSimple();

//��ɾ��һ��Ԫ�鵫��incrementȻ����¸�Ԫ��
int testUpdateAfterDeleteWithoutIncrement();

//�漴���ɱ�Ŀ���Ϣ��Ȼ�����
int testUpdateOnNoExistBlock();

//���Ŷ��updateͬһ��Ԫ�鵫ֻ�����һ��increment�����Բ鿴���
int testUpdateTupleWithoutIncrementManyTimes();

//���Ŷ��updateͬһ��Ԫ�鲢�Ҷ�increment�����Բ鿴���
int testUpdateTupleManyTimes();

//ɾ���������������ı�
void endTestNotStopEngine();

//ɾ���������������ı�
void beginTestNotStartEngine();



//���߳�ɾ��ͬһ�ű�ͬһ�����ݵĲ���
int test_thread_delete_000();

//���߳�ɾ��ͬһ�ű��������ݵĲ���
int test_thread_delete_001();

//���߳�ɾ��ͬһ�ű�ͬ���ݵĲ���
int test_thread_delete_002();

//���߳��໥ɾ����ͬ�����Ϣ
int test_thread_delete_003();

//���߳�ɾ��Ԫ���ͬʱ����Ԫ��
int test_thread_delete_004();



//���߳�ͬʱ����ͬһ��Ԫ��
int test_thread_update_000();

//���߳�˳�����ͬһ��Ԫ��(����֮ǰ�Ļ����ϸ���)
int test_thread_update_001();

//���̲߳�ѯ��ͬʱ����Ԫ��
int test_thread_update_002();



//���߳�ͬ��ɾ������
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
* ���Ը��º����user_abort_transaction�ع�
* Detailed description.
* @param[in] void 
**************************************************************************/
int test_thread_update_rollback();

/************************************************************************** 
* @thread_update_rollback 
* �����µ��̣߳�������߳��и��±�����ݣ�������������user_abort_tran
* saction()"���񽫻ع���������������м��
* Detailed description.
* @param[in] void 
**************************************************************************/
void thread_update_rollback(void *param,Oid rid);

int test_delete_no();
bool myTest_heap_delete_000();
bool myTest_heap_delete_001();