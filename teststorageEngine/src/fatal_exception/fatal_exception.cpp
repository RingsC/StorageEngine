#include "postgres.h"
#include "test_fram.h"
#include "StorageEngine/StorageEngineException.h"
#include "fatal_exception/fatal_exception.h"
#include "Macros.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "exception/XdbExceptions.h"

using namespace FounderXDB::StorageEngineNS;
using namespace FounderXDB::EXCEPTION;
void throw_fun_panic()
{
    ereport(PANIC,
        (errcode(ERRCODE_DATA_EXCEPTION),
        errmsg("panic exception test!")));
}
void throw_fun_fatal()
{
    ereport(FATAL,
        (errcode(ERRCODE_DATA_EXCEPTION),
        errmsg("fatal exception test!")));
}
bool fatal_exception_test()
{
    try
    {
        THROW_CALL(throw_fun_fatal);
    }
    catch(StorageEngineFatalException& e)
    {
        std::cout<<e.getErrorMsg()<<std::endl;
        return true;
    }
    
    std::cout<<"fatal exception catch failed !"<<std::endl;
    return false;
}

bool panic_exception_test()
{
    try
    {
        THROW_CALL(throw_fun_panic);
    }
    catch(StorageEngineFatalException& e)
    {
        std::cout<<e.getErrorMsg()<<std::endl;
        return true;
    }

    std::cout<<"panic exception catch failed !"<<std::endl;
    return false;
}

void throw_fun_object_in_use()
{
	ereport(ERROR,
		(errcode(ERRCODE_OBJECT_IN_USE),
		errmsg("Object in use, can not drop it!")));
}

bool test_object_in_use_exception()
{
	try
	{
		THROW_CALL(throw_fun_object_in_use);
	}
	catch(ObjectInUseException& e)
	{
		if (e.getErrorNo() == XdbBaseException::ERR_STORAGE_OBJECT_IN_USE)
			return true;
		return false;
	}

	std::cout<<"ObjectInUse exception catch failed !"<<std::endl;
	return false;
}

void throw_fuc_tupe_update_concurrent()
{
	ereport(ERROR,
		(errcode(ERRCODE_TUPLE_UPDATE_CONCURRENTLY),
		errmsg("tuple concurrently updated")));
}
bool test_tuple_update_concurrent_exception()
{
	try
	{
		THROW_CALL(throw_fuc_tupe_update_concurrent);
	}
	catch(TupleUpdateConcurrent& ex)
	{
		if (XdbBaseException::ERR_STORAGE_TUPLE_UPDATE_CONCURRENT == ex.getErrorNo())
			return true;
		return false;
	}

	std::cout<<"Tuple update concurrently exception throw test fail !"<<std::endl;
	return false;
}

void throw_fuc_two_masters()
{
	ereport(ERROR,
		(errcode(ERRCODE_REPHA_TWO_MASTERS),
        errmsg("I am master, but got BcastNewMaster msg from other.")));
}
bool test_two_masters_exception()
{
	try
	{
		THROW_CALL(throw_fuc_two_masters);
	}
	catch(TwoMasterException& ex)
	{
		if (XdbBaseException::ERR_STORAGE_TWO_MASTERS == ex.getErrorNo())
			return true;
		return false;
	}

	std::cout<<"Two masters exception throw test fail !"<<std::endl;
	return false;
}

void throw_fuc_unique_violation()
{
	ereport(ERROR,
		(errcode(ERRCODE_UNIQUE_VIOLATION),
        errmsg("found index relation unique violation.")));
}
bool test_unique_violation_exception()
{
	try
	{
		THROW_CALL(throw_fuc_unique_violation);
	}
	catch(UniqueViolationException& ex)
	{
		if (XdbBaseException::ERR_STORAGE_UNIQUE_VIOLATION == ex.getErrorNo()){
			printf("exception content: [%s]\n", ex.what());
			return true;
		}
		return false;
	}

	std::cout<<"Unique violation exception throw test fail !"<<std::endl;
	return false;
}

void throw_func_table_sapce_exists()
{
	ereport(ERROR,
		(errcode(ERRCODE_DUPLICATE_OBJECT),
		errmsg("tablespace already exists")));
}
bool test_table_space_already_exists()
{
	try
	{
		THROW_CALL(throw_func_table_sapce_exists);

	}
	catch(ObjectAlreadyExist& ex)
	{
		if (XdbBaseException::ERR_STORAGE_TABLE_SPACE_ALREADY_EXIST == ex.getErrorNo())
			return true;
		return false;
	}

	return false;
}

void object_not_exist_exception()
{
	ereport(ERROR,
		(errcode(ERRCODE_ACCESS_OBJECT_NOT_EXIST),
		errmsg("Object not exist!")));
}
bool test_object_not_exist()
{
	try
	{
		THROW_CALL(object_not_exist_exception);
	}
	catch(FounderXDB::StorageEngineNS::ObjectNotExistException& ex)
	{
		if (XdbBaseException::ERR_STORAGE_OBJECT_NOT_EXIST == ex.getErrorNo())
			return true;

		std::cout<<ex.getErrorCode()<<std::endl;
		return false;
	}
	std::cout<<"not catch"<<std::endl;

	return false;

}
void out_of_memory_exception()
{
	ereport(ERROR,
		(errcode(ERRCODE_OUT_OF_MEMORY),
		errmsg("Out of memory!")));
}
bool test_out_of_memory()
{
	try
	{
		THROW_CALL(out_of_memory_exception);
	}
	catch(FounderXDB::StorageEngineNS::OutOfMemoryException& ex)
	{
		if (XdbBaseException::ERR_STORAGE_OUT_OF_MEMORY == ex.getErrorNo())
			return true;

		std::cout<<ex.getErrorCode()<<std::endl;
		return false;
	}
	std::cout<<"out of memory exception not catched!"<<std::endl;

	return false;
}

void not_in_transaction_exception()
{
	ereport(ERROR,
		(errcode(ERRCODE_NOT_IN_TRANSACTION),
		errmsg("not in transaction!")));
}
bool test_not_in_transaction()
{
	try
	{
		THROW_CALL(not_in_transaction_exception);
	}
	catch(FounderXDB::StorageEngineNS::NotInTransactionException& ex)
	{
		if (XdbBaseException::ERR_STORAGE_NOT_IN_TRANSACTION == ex.getErrorNo())
			return true;

		std::cout<<ex.getErrorCode()<<std::endl;
		return false;
	}
	std::cout<<"not in transaction exception not catched!"<<std::endl;

	return false;
}