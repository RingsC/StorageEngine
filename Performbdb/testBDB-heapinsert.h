bool test_performbdb_heap_insert();

/************************************************************************** 
* @brief test_performbdb_heap_insert_special_random 
* 检测BDB中插入随机数据所需时间，数据的最大长度为MAX_RAM_STR_SIZE。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_insert_special_random();

/************************************************************************** 
* @brief test_performbdb_heap_insert_large_data 
* 检测BDB中插入大的随机数据所需时间，数据的大小可配置。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_insert_large_data();

/************************************************************************** 
* @brief test_performpg_thread_insert 
* 检测BDB多线程插入数据。
* Detailed description. 
**************************************************************************/
bool test_performpg_thread_insert();

/************************************************************************** 
* @brief test_performbdb_heap_insert_special 
* 检测BDB设置page size为8KB，并且对database设置DB_REVSPLITOFF标志
插入数据所需时间。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_insert_special();