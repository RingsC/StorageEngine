bool test_performpg_heap_delete();

/************************************************************************** 
* @brief test_performpg_heap_delete_special 
* 检测删除表中随机数据所需时间。插入的数据最大长度为MAX_RAM_STR_SIZE。
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_delete_special();

/************************************************************************** 
* @brief test_performpg_heap_delete_large_data 
* 检测删除表中大数据(需要走toast)所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_delete_large_data();

/************************************************************************** 
* @brief test_performpg_thread_delete 
* 检测PG多线程删除数据，每个线程删除（总数据量除以删除线程数）的数据。
* Detailed description. 
**************************************************************************/
bool test_performpg_thread_delete();