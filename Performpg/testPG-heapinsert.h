bool test_performpg_heap_insert();
bool test_performpg_heap_insert_createIndexFirst_thenHeap();
bool test_performpg_heap_insert_createHeapFisrt_thenIndex();

/************************************************************************** 
* @brief test_performpg_heap_insert_special_indexfirst 
* ���PG�Ƚ������ٲ���������ݣ������������󳤶�ΪMAX_RAM_STR_SIZE ����ʱ�䡣
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insert_special_indexfirst();

/************************************************************************** 
* @brief test_performpg_heap_insert_special_indexlater 
* ���PG�Ȳ�����������ٽ������������������󳤶�ΪMAX_RAM_STR_SIZE ����ʱ�䡣
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insert_special_indexlater();

/************************************************************************** 
* @brief test_performpg_heap_insertentries_indexfisrt 
* ���PGͨ��insertentries�����ݲ����������ʱ�䡣
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insertentries_indexfisrt();

/************************************************************************** 
* @brief test_performpg_heap_insertentries_later(); 
* ���PGͨ��insertentries�����ݲ����������ʱ�䡣
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insertentries_indexlater();

/************************************************************************** 
* @brief test_performpg_heap_insert_large_data(); 
* ���PG����ϴ����ݣ���toast��ʱ����ʱ�䡣
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insert_large_data();

/************************************************************************** 
* @brief test_performpg_heap_insertentries_large_data(); 
* ���PGͨ��insertentries����ϴ����ݣ���toast��ʱ����ʱ�䡣
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insertentries_large_data();

/************************************************************************** 
* @brief test_performpg_thread_insert(); 
* ���PG���߳���һ�����в�����������ʱ�䡣
* Detailed description. 
**************************************************************************/
bool test_performpg_thread_insert();