bool test_performbdb_heap_update();

/************************************************************************** 
* @brief test_performbdb_heap_update_special_random 
* 检测BDB中将随机数据更新为固定数据"test"所需时间，数据的最大长度为MAX_RAM_STR_SIZE。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_update_special_random();

/************************************************************************** 
* @brief test_performbdb_heap_update_large_data 
* 检测BDB中将大数据更新为固定数据"test"所需时间，数据的最大长度可配置。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_update_large_data();