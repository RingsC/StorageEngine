/*
* 测试tuple_store
*/

/* 插入数据并读取元组，放入一个Tuplestorestate中 */
extern int test_simple_tuple_store();

/* 插入数据并读取元组，放入一个Tuplestorestate中并排序 */
extern int test_simple_tuple_sort();

extern int test_sorted_tuple_sort();
bool test_tuplesort_datum();
