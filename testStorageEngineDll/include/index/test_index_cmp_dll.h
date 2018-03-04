//index 大量数据测试
bool test_indexscan_LargeMount_dll();

//创建多个索引，在相同的列上，验证结果
bool test_indexMult_SameCol_dll();

//测试先创建索引再插入数据
bool test_indexCreatefirst_thenInsert();

//测试在多列上给予多个scancondition
bool test_indexMultmethod_oneCol_dll();

//测试创建唯一索引，打开这个索引，然后通过gettype获得type。
bool test_index_uniqe_01_dll();


extern ColumnInfo heap_colinfo;
extern ColumnInfo index_colinfo;
void form_heap_colinfo_uniqe_01_dll(ColumnInfo &colinfo);
void form_index_colinfo_uniqe_01_dll(ColumnInfo &colinfo);

