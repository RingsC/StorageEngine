bool test_performpg_heap_update();

/************************************************************************** 
* @brief test_performbdb_heap_delete_special 
* 检测更新表中所有随机数据为某条固定数据所需时间。插入的数据最大长度为MAX_RAM_STR_SIZE。
* Detailed description. 
**************************************************************************/
bool test_performpg_heap_update_special();

/************************************************************************** 
* @brief test_performpg_thread_update 
* 多线程更新表中的数据。
* Detailed description. 
**************************************************************************/
bool test_performpg_thread_update();