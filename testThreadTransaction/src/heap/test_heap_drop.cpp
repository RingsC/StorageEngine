#include "heap/test_heap_drop.h"
int DatabasePara::nFlag = false;
bool test_database_remove_task(ParamBase* arg)
{
	DatabasePara* pArgs = (DatabasePara*)arg;

	try
	{
		StorageEngine::getStorageEngine()->dropDatabase(pArgs->GetTransaction(),pArgs->strDbName.c_str());

	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
	}

	DatabasePara::nFlag = true;
	return true;
}
bool test_database_create_task(ParamBase* arg)
{
	DatabasePara* pArgs = (DatabasePara*)arg;

	try
	{
		while (1)
		{
			if (DatabasePara::nFlag)
			{
				StorageEngine::getStorageEngine()->createDatabase(pArgs->GetTransaction(),pArgs->strDbName.c_str(),"defaulttablespace");
				break;
			}
		}
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
	}

	return true;
}

bool test_database_01()
{
	//删除一个不存在的数据库name1，然后，再创建名字为name1的数据库。
	DatabasePara* arg = new DatabasePara;
	arg->strDbName = "xdb_test_11";
	INITRANID();
	REGTASK(test_database_remove_task, arg);

	return true;
}

bool test_database_02()
{
	DatabasePara* arg = new DatabasePara;
	arg->strDbName = "xdb_test_11";
	INITRANID();

	REGTASK(test_database_create_task, arg);

	return true;
}