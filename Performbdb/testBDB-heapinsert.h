bool test_performbdb_heap_insert();

/************************************************************************** 
* @brief test_performbdb_heap_insert_special_random 
* ���BDB�в��������������ʱ�䣬���ݵ���󳤶�ΪMAX_RAM_STR_SIZE��
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_insert_special_random();

/************************************************************************** 
* @brief test_performbdb_heap_insert_large_data 
* ���BDB�в����������������ʱ�䣬���ݵĴ�С�����á�
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_insert_large_data();

/************************************************************************** 
* @brief test_performpg_thread_insert 
* ���BDB���̲߳������ݡ�
* Detailed description. 
**************************************************************************/
bool test_performpg_thread_insert();

/************************************************************************** 
* @brief test_performbdb_heap_insert_special 
* ���BDB����page sizeΪ8KB�����Ҷ�database����DB_REVSPLITOFF��־
������������ʱ�䡣
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_insert_special();