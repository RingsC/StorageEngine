bool test_performbdb_heap_delete();

/************************************************************************** 
* @brief test_performbdb_heap_delete_special_random 
* ���BDB��ɾ��������е������������ʱ�䣬����������ݵ���󳤶�ΪMAX_RAM_STR_SIZE��
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_delete_special_random();

/************************************************************************** 
* @brief test_performbdb_heap_delete_large_data 
* ���BDB��ɾ��������д����������ʱ�䣬���ݴ�С�����á�
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_delete_large_data();

/************************************************************************** 
* @brief test_performbdb_thread_delete 
* ���BDB���߳�ɾ�����������������ʱ�䣬���ݴ�С�����á�
* Detailed description. 
**************************************************************************/
bool test_performbdb_thread_delete();

/************************************************************************** 
* @brief test_performbdb_heap_delete_special 
* ���BDB����page sizeΪ8KB�����Ҷ�database����DB_REVSPLITOFF��־
ɾ����������ʱ�䡣
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_delete_special();