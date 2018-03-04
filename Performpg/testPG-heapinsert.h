bool test_performpg_heap_insert();
bool test_performpg_heap_insert_createIndexFirst_thenHeap();
bool test_performpg_heap_insert_createHeapFisrt_thenIndex();

/************************************************************************** 
* @brief test_performpg_heap_insert_special_indexfirst 
* 检测PG先建索引再插入随机数据，插入的数据最大长度为MAX_RAM_STR_SIZE 所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insert_special_indexfirst();

/************************************************************************** 
* @brief test_performpg_heap_insert_special_indexlater 
* 检测PG先插入随机数据再建索引，插入的数据最大长度为MAX_RAM_STR_SIZE 所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insert_special_indexlater();

/************************************************************************** 
* @brief test_performpg_heap_insertentries_indexfisrt 
* 检测PG通过insertentries将数据插入表中所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insertentries_indexfisrt();

/************************************************************************** 
* @brief test_performpg_heap_insertentries_later(); 
* 检测PG通过insertentries将数据插入表中所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insertentries_indexlater();

/************************************************************************** 
* @brief test_performpg_heap_insert_large_data(); 
* 检测PG插入较大数据（走toast）时所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insert_large_data();

/************************************************************************** 
* @brief test_performpg_heap_insertentries_large_data(); 
* 检测PG通过insertentries插入较大数据（走toast）时所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_insertentries_large_data();

/************************************************************************** 
* @brief test_performpg_thread_insert(); 
* 检测PG多线程向一个表中插入数据所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_thread_insert();