//开始测试，并插入元组
bool beginTest();

//测试删除数据表中第一个元组
bool test_Delete_FirstTupleFromHeap_dll();

//插入多个不同元组，测试删除其中一个指定的元组
bool test_Delete_OneTupleFromManyTuple_dll();

//测试由给定的步数删除一个元组
bool test_Delete_FromGiveSteps_dll();

//测试删除数据表中不存在的元组
bool test_Delete_NoExistTuple_dll();

//测试在没有CommandCounterIncrement的情况下连续删除同一个元组
bool test_DeleteTheSameTuple_Without_commit_dll();

//插入一个新元组，再根据插入成功之后获取到的块信息删除该元组
int test_DeleteByTupleInsert_GetBlockAndOffset_dll();

bool test_Delete_Shot_Dll();



