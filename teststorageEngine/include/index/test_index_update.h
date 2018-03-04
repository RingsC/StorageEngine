#ifndef _TEST_INDEX_UPDATE_H
#define _TEST_INDEX_UPDATE_H
bool testIndexUpdate_SingleColumn();
bool testIndexUpdate_MultiColumn();
bool testIndexUpdate_InAnotherTrans();


extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);
extern void thread_create_index(int heap_id, Colinfo heap_colinfo, Colinfo ind_colinfo, int ind_id = INDEX_ID, bool unique = false);
extern void thread_index_scan(BackendParameters *param,
							  const Oid index_id, 
							  const Colinfo ind_info, 
							  const Oid heap_id, 
							  const Colinfo heap_info,
							  const IndexScanInfo *isi, 
							  DataGenerater *dg,
							  unsigned int *array_len);
extern void prepare_param(BackendParameters *paramArr[],const int nThread);

extern void free_param(BackendParameters *paramArr[],const int nThread);
extern void thread_create_heap_xy(int nTables,const int rel_id, const Colinfo heap_info);
extern void thread_drop_heap(int nTables);
extern int THREAD_RID;
extern void thread_remove_index(const int heap_id, int ind_id = INDEX_ID);
extern void thread_insert_data(const char data[][DATA_LEN], 
							   const int array_len, 
							   const int data_len,
							   const int rel_id);

/************************************************************************** 
* @test_thread_index_update_01
* ������������̷ֱ߳���±�,��Щ���ǵ��е�,ÿ���̶߳�������
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_index_update_01();
void thread_index_update_01(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id);
/************************************************************************** 
* @test_thread_index_update_02
* ������������̷ֱ߳���±�,��Щ���Ƕ��еģ�ÿ���̶߳������ݡ�
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_index_update_02();

void thread_index_update_02(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id);
/************************************************************************** 
* @test_thread_index_update_03
* ������һ�����в������ݣ���������߳������������������,Ȼ������������ѯ���º��Ľ������Щ���Ƕ��еġ�
* Detailed description.
* @param[in] void 
**************************************************************************/
bool test_thread_index_update_03();
void thread_index_update_0301(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id);
void thread_index_update_0302(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id);
void thread_index_update_0303(void *param,Oid rid,Colinfo heap_info,Colinfo index_info,int index_id);
#endif