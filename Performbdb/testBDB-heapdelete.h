bool test_performbdb_heap_delete();

/************************************************************************** 
* @brief test_performbdb_heap_delete_special_random 
* 检测BDB中删除插入表中的随机数据所需时间，插入表中数据的最大长度为MAX_RAM_STR_SIZE。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_delete_special_random();

/************************************************************************** 
* @brief test_performbdb_heap_delete_large_data 
* 检测BDB中删除插入表中大的数据所需时间，数据大小可配置。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_delete_large_data();

/************************************************************************** 
* @brief test_performbdb_thread_delete 
* 检测BDB多线程删除插入表中数据所需时间，数据大小可配置。
* Detailed description. 
**************************************************************************/
bool test_performbdb_thread_delete();

/************************************************************************** 
* @brief test_performbdb_heap_delete_special 
* 检测BDB设置page size为8KB，并且对database设置DB_REVSPLITOFF标志
删除数据所需时间。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_delete_special();