bool test_performpg_indexscan();

/************************************************************************** 
* @brief test_performpg_indexscan_special 
* 检测从海量数据（count）中通过索引找到（FINDTIMES）条指定数据所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_indexscan_special();

/************************************************************************** 
* @brief test_performpg_indexscan_special_random 
* 检测通过索引找到所有的随机数据（数据最大长度为2000)所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_indexscan_special_random();

/************************************************************************** 
* @brief test_performpg_indexscan_large_data 
* 检测通过索引找到较大数据（走toast)所需时间。
* Detailed description. 
**************************************************************************/
bool test_performpg_indexscan_large_data();

/************************************************************************** 
* @brief test_performpg_thread_scan 
* PG多线程查找表中的数据。
* Detailed description. 
**************************************************************************/
bool test_performpg_thread_scan();
