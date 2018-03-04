bool test_performbdb_heap_scan();

/************************************************************************** 
* @test_performbdb_heap_scan_special
* 从海量数据中检测出100条数据所需时间。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_scan_special();

/************************************************************************** 
* @brief test_performbdb_heap_scan_special_random 
* 检测BDB中检测出插入的随机数据所需时间，数据的最大长度为MAX_RAM_STR_SIZE。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_scan_special_random();

/************************************************************************** 
* @brief test_performbdb_heap_scan_special_random 
* 检测BDB中检测出插入大数据所需时间，数据长度可配置。
* Detailed description. 
**************************************************************************/
bool test_performbdb_heap_scan_large_data();
