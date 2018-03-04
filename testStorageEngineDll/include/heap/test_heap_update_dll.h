//测试更新第一个元组
bool test_Update_simple_dll();

//先删除一个元组但不increment然后更新该元组
int testUpdateAfterDeleteWithoutIncrement();

//随机生成表的块信息，然后更新
bool test_Update_OnNoExistBlock_dll();

//连着多次update同一个元组但只在最后一次commit，测试查看结果
bool test_Update_WithoutCommitManyTimes_dll();

//连着多次update同一个元组并且都increment，测试查看结果
bool test_Update_TupleManyTimes_dll();

//测试经过优化的Update、Delete和Inserte
bool test_Update_Delete_Insert();

bool test_Update_simple_shot_dll();