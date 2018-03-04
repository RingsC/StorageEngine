#include <stdio.h>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include "boost/thread/thread.hpp"
#include "utils/util.h"
#include "tablespace/create_tablespace.h"
#include "commands/tablespace.h"
#include "nodes/parsenodes.h"
#include "test_fram.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "postgres_ext.h"
#include "catalog/pg_database.h"
#include "StorageEngine.h"
#include "MemoryContext.h"
#include "access/xact.h"
#include "interface/FDPGAdapter.h"
#include "utils/tqual.h"
#include "catalog/xdb_catalog.h"
#include "Macros.h"

#define RETURN_FALSE(objid,errinfo)\
	if(InvalidOid == objid)\
	{\
	std::cout<<errinfo<<std::endl;\
	user_abort_transaction();\
	return false;\
	}\

using namespace boost;
using namespace FounderXDB::StorageEngineNS;
namespace bf = boost::filesystem;

const unsigned int MAX_LENGTH_PATH = MAXPGPATH;
const std::string TABLE_SPACE_DIR_NAME = "myTableSpace";
char TABLE_SPACE_NAME[] = {"my_table_space"};
char TABLE_SPACE_NAME_EX[] = {"same_dir_test"};
std::string my_database_name;
std::string my_talspc_path;
const Oid COLUMN_ID = 10000;
const Oid RELATION_ID = 10001;
extern std::string g_strDataDir;
std::vector<std::string> vMyCreatePath;

void my_split(RangeData& range, const char*, int nCol , size_t charlen)
{
	range.start = 0;
	range.len = charlen;
}

bool test_fatal_exception()
{
    try
    {
        begin_transaction();
        char testname[] = "test_name'";
        Oid tblspcid = get_tablespace_oid(testname,true);
        if (InvalidOid == tblspcid)
        {
            tblspcid = my_create_tablespace(testname, "abd'd");
        }

        commit_transaction();
    }
    catch(StorageEngineFatalException& e)
    {
        std::cout<<e.getErrorNo()<<std::endl;
        std::cout<<e.getErrorMsg()<<std::endl;
        return false;    
    }
}
bool test_create_tablespace_valid()
{
	try
	{
		begin_transaction();
		Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME,true);
		if (InvalidOid == tblspcid)
		{
            tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
            RETURN_FALSE(tblspcid, "create table space failed !")
		}

		commit_transaction();
	}
	CATCHEXCEPTION

	return true;
}

bool test_create_tablespace_with_system_reserve_name()
{
	try
	{
		begin_transaction();

		char chSysReserveName[] = "defaulttablespace";
		my_create_tablespace(chSysReserveName, TABLE_SPACE_DIR_NAME.c_str());

		commit_transaction();
	}
	catch(...)
	{
	    user_abort_transaction();
	    return true;	
	}

	return false;
}

bool test_create_tblspc_with_occupied_name()
{
	try
	{
		begin_transaction();
		Oid tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
		RETURN_FALSE(tblspcid, "create table space failed !")
		commit_transaction();

		begin_transaction();
		std::string strDirName = "test_path"; 
		my_create_tablespace(TABLE_SPACE_NAME, strDirName.c_str());
		commit_transaction();
	}
	catch(...)
	{
		user_abort_transaction();
		return true;
	}

	return false;
}

bool test_create_tblspc_with_pg_Prefix_name()
{
    try
    {
        begin_transaction();
        char chTblspcName[] = "pg_my_table_space";
        my_create_tablespace(chTblspcName, TABLE_SPACE_DIR_NAME.c_str());
        commit_transaction();
        
        begin_transaction();
        my_drop_tablespace(chTblspcName,true);
        commit_transaction();
    }
    catch(...)
    {
        user_abort_transaction();
        return true;
    }

    std::cout<<"create table space with \"pg_\" prefix happened !"<<std::endl;
    return false;
}

bool test_create_tblspc_with_occupied_path()
{
	try
	{
		begin_transaction();
		Oid tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
		RETURN_FALSE(tblspcid, "create table space failed !")
		commit_transaction();
		
		begin_transaction();		
		tblspcid = my_create_tablespace(TABLE_SPACE_NAME_EX, TABLE_SPACE_DIR_NAME.c_str());
		if (InvalidOid != tblspcid)
		{
			std::cout<<"create table space with occupied path succeed which should not happen !";
			user_abort_transaction();
			return false;
		}
		commit_transaction();
	}
	catch(...)
	{
		user_abort_transaction();
		return true;
	}

	return false;
}

bool test_create_tblspc_with_too_long_path()
{
    //construct a too long path larger than 1024
    int nflag = MAX_LENGTH_PATH;
    std::string strDirName;
    while (nflag)
    {
        strDirName += "0";
        nflag --;
    }
    
    std::string strTbspacePath;
    if (g_strDataDir.at(g_strDataDir.length()-1) == '\\')
    {
        strTbspacePath = g_strDataDir + strDirName;
    }
    else
    {
        strTbspacePath = g_strDataDir + "\\" + strDirName;
    }
    
    char* pTblspcPath = new char[strTbspacePath.length() + 1];
    memset(pTblspcPath,0,strTbspacePath.length() + 1);
    memcpy(pTblspcPath,strTbspacePath.c_str(),strTbspacePath.length());

    try
    {
        begin_transaction();
        
        if (!bf::exists(pTblspcPath))
        {
            bf::create_directory(pTblspcPath);
        }
        
        CreateTableSpaceStmt stmt;
        stmt.location = pTblspcPath;
        stmt.owner = NULL;
        stmt.tablespacename = TABLE_SPACE_NAME;
        stmt.type = T_Invalid;
        THROW_CALL(CreateTableSpace,&stmt,true);
        
        commit_transaction();
        
        //delete dir created if succeed
        if (bf::exists(pTblspcPath))
        {
            bf::remove_all(pTblspcPath);
        }
        if (pTblspcPath)
        {
            delete[] pTblspcPath;
            pTblspcPath = NULL;
        }
    }
    catch(std::runtime_error& e)
    {
        std::cout<<e.what()<<std::endl;
        //if (bf::exists(pTblspcPath))
        //{
        //    bf::remove_all(pTblspcPath);
        //}
        if (pTblspcPath)
        {
            delete[] pTblspcPath;
            pTblspcPath = NULL;
        }
        user_abort_transaction();
        
        return true;            
    }
    
    std::cout<<"create tablespace with too long path happened !"<<std::endl;
    return false;
}


bool test_create_tblspc_with_nonexist_path()
{
	try
	{
		begin_transaction();

		std::string strRelativePath = "path_no_exist";
		std::string strTestPath;
		my_create_absolute_path(g_strDataDir, strRelativePath.c_str(), strTestPath);
		if (strTestPath.empty())
		{
			std::cout<<"construct absolute path failed !"<<std::endl;
			return false;
		}

		if (bf::exists(strTestPath))
		{
			bf::remove_all(strTestPath);
		}

		char chPath[MAX_LENGTH_PATH];
		memset(chPath,0,MAX_LENGTH_PATH);
		memcpy(chPath, strTestPath.c_str(),strTestPath.length());
		CreateTableSpaceStmt stmt;
		stmt.location = chPath;
		stmt.owner = NULL;
		stmt.tablespacename = TABLE_SPACE_NAME;
		stmt.type = T_Invalid;
		
		THROW_CALL(CreateTableSpace,&stmt, true);

		commit_transaction();
	}
	catch(...)
	{
		user_abort_transaction();
		return true;
	}

	std::cout<<"create table space with nonexistent path succeed which should not happen !";
	return true;
}

bool test_create_tblspc_with_relative_path()
{
	std::string strTestPath;
	try
	{
		begin_transaction();

		char chRelativePath[] = {"my_path_relative"};
		my_create_absolute_path(g_strDataDir, chRelativePath, strTestPath);
		if (strTestPath.empty())
		{
			std::cout<<"construct absolute path failed !"<<std::endl;
			return false;
		}

		if (!bf::exists(strTestPath))
		{
			bf::create_directory(strTestPath);
		}

		CreateTableSpaceStmt stmt;
		stmt.location = chRelativePath;
		stmt.owner = NULL;
		stmt.tablespacename = TABLE_SPACE_NAME;
		stmt.type = T_Invalid;

		Oid tblspcid = InvalidOid;
		THROW_CALL(tblspcid = CreateTableSpace,&stmt, true);

		commit_transaction();
	}
	catch(...)
	{
		user_abort_transaction();
		if (bf::exists(strTestPath))
		{
			bf::remove_all(strTestPath);
		}

		return true;
	}

	if (bf::exists(strTestPath))
	{
		bf::remove_all(strTestPath);
	}
	std::cout<<"create table space with relative path succeed which should not happen !";
	return false;
}
bool test_create_tblspc_transaction_abort()
{
	try
	{
		begin_transaction();
		Oid tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
		RETURN_FALSE(tblspcid, "create table space failed !")
		user_abort_transaction();

		begin_transaction();
		tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
		RETURN_FALSE(tblspcid, "create table space failed !")
		commit_transaction();
	}
 	catch(StorageEngineExceptionUniversal& e)
 	{
 	    std::cout<<"recreate tablespace failed after transaction abort  !"<<std::endl;
 	    std::cout<<e.getErrorNo()<<std::endl;
 	    std::cout<<e.getErrorMsg()<<std::endl;
 	    std::string strPath;
 	    my_create_absolute_path(g_strDataDir, TABLE_SPACE_DIR_NAME.c_str(),strPath);
 	    if (bf::exists(strPath))
 	    {
 	        bf::remove_all(strPath);
 	    }
 	    user_abort_transaction();
 	    return false; 	
 	}

	return true;
}

bool test_create_tblspc_recovery()
{
    try
    {
        begin_transaction();
        Oid tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
        commit_transaction();
    }
    CATCHEXCEPTION
    
    return true;
}

bool test_clear_and_drop_tablespace()
{
	try
	{
		begin_transaction();

		if (!my_database_name.empty())
		{
            my_drop_database(my_database_name.c_str());
            my_database_name.clear();
		}

		my_drop_tablespace(TABLE_SPACE_NAME, true);

		commit_transaction();
	}
	CATCHEXCEPTION

	return true;
}
//////////////////////////////////////////////////////////////////////////
//get oid or name
bool test_get_tablespace_oid_exist()
{
	try
	{
		begin_transaction();
		Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, true);
		if (InvalidOid == tblspcid)
		{
            tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
            RETURN_FALSE(tblspcid, "create table space failed !")
		}

		Oid get_tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, false);
		RETURN_FALSE(get_tblspcid, "get table space id from table space name failed !")
		
		commit_transaction();
	}
	CATCHEXCEPTION

	return true;
}

bool test_get_tablespace_oid_nonexist()
{
    try
    {
        begin_transaction();
        my_drop_tablespace(TABLE_SPACE_NAME, true);
        
        Oid get_tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, true);
        if (InvalidOid != get_tblspcid)
        {
            std::cout<<"get invalid tablesapce oid happened !"<<std::endl;
            user_abort_transaction();
            return false;
        }
        commit_transaction();
    }
    CATCHEXCEPTION
    
    return true;
}
bool test_get_tablespace_name()
{
	try
	{
		begin_transaction();

		Oid tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
		RETURN_FALSE(tblspcid, "create tablespace failed !")
	
		char* pTableSpace = NULL;
		THROW_CALL(pTableSpace = get_tablespace_name,tblspcid);
		if (0 != strcmp(TABLE_SPACE_NAME,pTableSpace))
		{
		    std::cout<<"get tablesapce name during oid failed !"<<std::endl;
			user_abort_transaction();
			return false;
		}

		commit_transaction();
	}
	CATCHEXCEPTION

	return true;
}

bool test_get_default_tablespace_id()
{
	bool bRet = false;
	try
	{
		begin_transaction();

		Oid default_tblspcid = GetDefaultTablespace(RELKIND_RELATION);
		RETURN_FALSE(default_tblspcid, "current default table space not exist !")

		commit_transaction();
	}
	CATCHEXCEPTION

	return bRet;
}

static
void thread_create_db(BackendParameters *GET_PARAM(), char *dbname, int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	*sta = 1;
	try
	{
		begin_transaction();
		my_create_database(dbname, DEFAULT_TABLESPACE_NAME);
		commit_transaction();
	} catch (StorageEngineException &se)
	{
		*sta = 0;
	}

	proc_exit(0);
}

static
void thread_drop_db(BackendParameters *GET_PARAM(), char *dbname, int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	*sta = 1;
	try
	{
		begin_transaction();
		my_drop_database(dbname);
		commit_transaction();
	} catch (StorageEngineException &se)
	{
		*sta = 0;
	}

	proc_exit(0);
}

bool test_concurrently_createdb()
{
	INTENT("启动多个线程去并发创建数据库，任何一个backend线程"
		"创建数据库失败测例就失败。");

#define CREATE_NUM 20

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	PREPARE_TEST();

	bool result = true;
	char db_name[CREATE_NUM][1024];
	int sta[CREATE_NUM];
	/* 初始化即将要创建的所有数据库的名字 */
	for (int i = 0; i < CREATE_NUM; ++i)
	{
		memset(db_name[i], 0, 1024);
		sprintf(db_name[i], "%s_%d", "database", i);
	}

	/* 创建CREATE_NUM个线程去创建CREATE_NUM个数据库 */
	for (int i = 0; i < CREATE_NUM; ++i)
	{
		BackendParameters *GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(&thread_create_db, GET_PARAM(), db_name[i], &sta[i]));
	}
	GET_THREAD_GROUP().join_all();

	/* 检测结果 */
	for (int i = 0; i < CREATE_NUM; ++i)
	{
		if (sta[i] != 1)
		{
			result = false;
			break;
		}
	}

	/* 删除所有的数据库 */
	/* 创建CREATE_NUM个线程去创建CREATE_NUM个数据库 */
	for (int i = 0; i < CREATE_NUM; ++i)
	{
		BackendParameters *GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(&thread_drop_db, GET_PARAM(), db_name[i], &sta[i]));
	}
	GET_THREAD_GROUP().join_all();

	FREE_PARAM(BackendParameters *);

	/* 检测删除数据库是否成功 */
	for (int i = 0; i < CREATE_NUM; ++i)
	{
		if (sta[i] != 1)
		{
			result = false;
			break;
		}
	}

	return result;
}

bool test_create_db_under_tablespace()
{
	try
	{
		begin_transaction();
		
		Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, true);
		RETURN_FALSE(tblspcid, "get tablespace oid failed !")
		
		char chDbName[] = {"my_test_db"};
		Oid dbid = my_create_database(chDbName,TABLE_SPACE_NAME);
		RETURN_FALSE(dbid, "create database under table space failed !")

		Oid heapid = my_create_heap(TABLE_SPACE_NAME, chDbName, RELATION_ID, COLUMN_ID);
		RETURN_FALSE(heapid, "create heap under table space failed !")
		CommandCounterIncrement();
		
		Relation pRelation = FDPG_Heap::fd_heap_open(heapid,RowExclusiveLock);
		if (NULL == pRelation)
		{
			std::cout<<"create table under table space failed !"<<std::endl;
			user_abort_transaction();
			return false;
		}
		//else
		//{
		//    HeapTuple tup;
  //          simple_heap_insert(pRelation,)

		//}
		FDPG_Heap::fd_heap_close(pRelation,RowExclusiveLock);
		
		commit_transaction();
	}
    CATCHEXCEPTION

	return true;
}

bool test_create_database_under_tablespace_no_dir()
{
	try
	{
		begin_transaction();
		
		Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, true);
		if (InvalidOid == tblspcid)
		{
            tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
            RETURN_FALSE(tblspcid, "create table space failed !")
		}
		
		commit_transaction();
	}
	CATCHEXCEPTION
	
	try
	{
	    //delete pg_dir manual
		if (bf::exists(my_talspc_path))
		{
            bf::remove_all(my_talspc_path);
		}

		//create database
		begin_transaction();
		char chDbName[] = {"my_db_c"};
		if (GetDatabaseOid(chDbName) != InvalidOid)
		{
		    my_drop_database(chDbName);
		}
		Oid dbid = my_create_database(chDbName, TABLE_SPACE_NAME);
        if (dbid != InvalidOid)
        {
            my_drop_database(chDbName);
            std::cout<<"create database under nonexistent dir succeed which should not happen !"<<std::endl;
            user_abort_transaction();
            return false;
        }

		//create heap
		Oid heapid = my_create_heap(TABLE_SPACE_NAME, chDbName, RELATION_ID, COLUMN_ID);
        if (heapid != InvalidOid)
        {
            std::cout<<"create database under nonexistent dir succeed which should not happen !"<<std::endl;
            user_abort_transaction();
            return false;
        }
        my_drop_database(chDbName);
        commit_transaction();
	}
	catch(...)
	{
		user_abort_transaction();
		return true;
	}

    std::cout<<"create database under nonexistent dir happened !"<<std::endl;
	return false;
}
bool test_create_db_with_invalid_tblspc()
{
	try
	{
		begin_transaction();

		char chDbName[] = {"my_db_b"};
		char chInvalidTblspcName[] = {"invalid_tblspc_name"};
		Oid tblspcid = get_tablespace_oid(chInvalidTblspcName,true);
		if (InvalidOid != tblspcid)
		{
		    my_drop_tablespace(chInvalidTblspcName, true);
		}
		my_create_database(chDbName,chInvalidTblspcName);

		my_create_heap(chInvalidTblspcName, chDbName, RELATION_ID, COLUMN_ID);
		
		my_drop_database(chDbName);

		commit_transaction();
	}
	catch(...)
	{
		user_abort_transaction();
		return true;
	}

    std::cout<<"create database under nonexistent tablespace happened !"<<std::endl;
	return false;
}

bool test_create_db_with_normal_dbname_len()
{
    return my_create_db_name_len_test(10);
}

bool test_create_db_with_dbname_len_63()
{
    return my_create_db_name_len_test(NAMEDATALEN - 1);
}

bool test_create_db_with_dbname_len_64()
{
    return my_create_db_name_len_test(NAMEDATALEN);
}

bool test_create_db_with_larger_than_max_dbname_len()
{
    return my_create_db_name_len_test(NAMEDATALEN + 10);
}
bool my_create_db_name_len_test(const int &nLen)
{
    std::string strDbName;
    int nflag = nLen;
    while (nflag)
    {
        strDbName += "a";
        nflag --;
    }
    char* pchDbName = new char[strDbName.length() + 1];
    memset(pchDbName, 0, strDbName.length() + 1);
    memcpy(pchDbName,strDbName.c_str(),strDbName.length());
    
    try
    {
        begin_transaction();
        Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, true);
        RETURN_FALSE(tblspcid, "get tablespace oid failed !")
        
        Oid dbid_get = InvalidOid;
        THROW_CALL(dbid_get = GetDatabaseOid, pchDbName);
        if (InvalidOid != dbid_get)
            my_drop_database(pchDbName);
        
        my_create_database(pchDbName,TABLE_SPACE_NAME);
				my_drop_database(pchDbName);
        if (pchDbName)
        {
            delete[] pchDbName;
            pchDbName = NULL;
        }
        
        commit_transaction();
    }
    catch(StorageEngineExceptionUniversal& e)
    {
        if (pchDbName)
        {
            delete[] pchDbName;
            pchDbName = NULL;
        }
        
        user_abort_transaction();
        if (NAMEDATALEN-1 < nLen)
        {
            return true;
        }
        std::cout<<e.getErrorMsg()<<std::endl;
        return false;    
    }
    
    if (NAMEDATALEN < nLen)
    {
        return false;
    }
    
    return true;    
}

bool test_create_db_with_occupied_name()
{
    try
    {
        begin_transaction();
        Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, false);
        RETURN_FALSE(tblspcid, "get table space failed !")
        
        char dbname[] = "my_db_a";
        Oid dbid_get = GetDatabaseOid(dbname);
        if (InvalidOid != dbid_get)
            my_drop_database(dbname);
        
        Oid dbid = my_create_database(dbname, TABLE_SPACE_NAME);
        RETURN_FALSE(dbid, "create database failed !")
        
        my_create_database(dbname, TABLE_SPACE_NAME);
        
        my_drop_database(dbname);
        
        commit_transaction();    
    }
    catch(...)
    {
        user_abort_transaction();
        return true;            
    }
    
    std::cout<<"recreate database with the same dbname happened !"<<std::endl;
    return false;
}

bool test_create_db_under_diff_tblspc()
{
    try
    {
        begin_transaction();
        
        //create three tablespaces
        char chTblspcName[][25] = {"my_tblspc_name_one",
        "my_tblspc_name_two",
        "my_tblspc_name_three"};
        
        char chTblspcDirName[][15] = {"my_tblspc_dir1",
        "my_tblspc_dir2",
        "my_tblspc_dir3"};
                
        for (int i = 0; i < 3; i++)
        {
            Oid tblspcid = get_tablespace_oid(chTblspcName[i], true);
            if (InvalidOid == tblspcid)
            {
                my_create_tablespace(chTblspcName[i], chTblspcDirName[i]);
            }
        }
        
        //create db heaps to diff tblspc
        char chDbName[] = "my_db_name";
        Oid dbid = GetDatabaseOid(chDbName);
        if (InvalidOid != dbid)
        {
            my_drop_database(chDbName);
        }
        
        dbid = my_create_database(chDbName, chTblspcName[0]);
        RETURN_FALSE(dbid, "create database failed !")
        
        Oid heapid = my_create_heap(chTblspcName[0], chDbName, RELATION_ID, COLUMN_ID);
        RETURN_FALSE(heapid, "create heap failed !")
        
        heapid = my_create_heap(chTblspcName[1], chDbName, RELATION_ID + 1, COLUMN_ID + 1);
        RETURN_FALSE(heapid, "create heap failed !")
        
        heapid = my_create_heap(chTblspcName[2], chDbName, RELATION_ID + 2, COLUMN_ID + 2);
        RETURN_FALSE(heapid, "create heap failed !")
        
        //drop database 
        my_drop_database(chDbName);
        CommandCounterIncrement();
        dbid = GetDatabaseOid(chDbName);
        if (InvalidOid != dbid)
        {
            std::cout<<"drop database from system table failed !"<<std::endl;
            return false;
        }
        
        for (int i = 0; i < 3; i ++)
        {
            my_drop_tablespace(chTblspcName[i], false);
            std::string strAbsPath;
            my_create_absolute_path(g_strDataDir, chTblspcDirName[i], strAbsPath);
            bf::remove_all(strAbsPath);
        }
        
        commit_transaction();
    }
    CATCHEXCEPTION
    
    return true;
}

bool test_drop_database_empty()
{
	bool bRet = false;
	try
	{
		char dbname[] = {"my_db_a"};
		Oid dbid = my_create_database(dbname, TABLE_SPACE_NAME);
		if (InvalidOid != dbid)
		{
			my_drop_database(dbname);
			if (InvalidOid == get_tablespace_oid(TABLE_SPACE_NAME,false))
			{
			}
			bRet = true;
		}
	}
	CATCHEXCEPTION

		return bRet;
}

bool test_drop_heap_under_tablespace()
{
    try
    {
        begin_transaction();
        
        Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, false);
        RETURN_FALSE(tblspcid, "create table sapce failed !")
        
        char chDbName[] = "my_db_name1";
        Oid dbid = my_create_database(chDbName, TABLE_SPACE_NAME);
        RETURN_FALSE(dbid, "create database failed !")
        
        Oid heapid = my_create_heap(TABLE_SPACE_NAME, chDbName, RELATION_ID, COLUMN_ID);
        RETURN_FALSE(heapid, "create heap failed !")
        
        heapid = my_create_heap(TABLE_SPACE_NAME, chDbName, RELATION_ID + 1, COLUMN_ID + 1);
        RETURN_FALSE(heapid, "create heap failed !")
        
        heapid = my_create_heap(TABLE_SPACE_NAME, chDbName, RELATION_ID + 2, COLUMN_ID + 2);
        RETURN_FALSE(heapid, "create heap failed !")
        
        FDPG_Heap::fd_heap_drop(heapid, dbid);
        CommandCounterIncrement();

        Relation rel = relation_open(heapid,AccessShareLock);
        if (rel)
        {
            std::cout<<"drop heap failed !"<<std::endl;
            my_drop_database(chDbName);
            return false;
        }
        my_drop_database(chDbName);
        my_drop_tablespace(TABLE_SPACE_NAME, false);
        commit_transaction();    
    }
    CATCHEXCEPTION
    
    return true;
}

//////////////////////////////////////////////////////////////////////////
//drop table space tests
bool test_drop_tablespace_empty()
{
	try
	{
		begin_transaction();

		DropTableSpaceStmt stmt;
		stmt.missing_ok = true;
		stmt.tablespacename = TABLE_SPACE_NAME;
		stmt.type = T_Invalid;
		THROW_CALL(DropTableSpace,&stmt);
		CommandCounterIncrement();
		
		for (std::vector<std::string>::iterator it = vMyCreatePath.begin(); it != vMyCreatePath.end(); it ++)
		{
		    bf::remove_all(*it);
		}

		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		user_abort_transaction();
		return false;
	}

	return true;
}
bool test_drop_tablesapce_exist_miss_err()
{
    try
    {
        begin_transaction();
        Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME,true);
        if (InvalidOid == tblspcid)
        {
            my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
        }

        my_drop_tablespace(TABLE_SPACE_NAME, false);

        commit_transaction();
    }
   CATCHEXCEPTION
   
   return true;
}
bool test_drop_tablespace_nonexist_miss_err()
{
    try
    {
        begin_transaction();
        Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME,true);
        if (InvalidOid != tblspcid)
        {
            my_drop_tablespace(TABLE_SPACE_NAME, true);
        }
        
        my_drop_tablespace(TABLE_SPACE_NAME, false);
        
        
        commit_transaction();
    }
    catch(...)
    {
        user_abort_transaction();
        return true;
    }
    std::cout<<"drop  nonexistent tablespace with \"miss_ok = false\" happened !"<<std::endl;
    return false;
}
bool test_drop_tablespace_nonexist_miss_ok()
{
    try
    {
        begin_transaction();
        Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME,true);
        if (InvalidOid != tblspcid)
        {
            my_drop_tablespace(TABLE_SPACE_NAME, true);
        }

        my_drop_tablespace(TABLE_SPACE_NAME, true);

        commit_transaction();
    }
    catch(...)
    {
        std::cout<<"drop  nonexistent tablespace with \"miss_ok = true\" failed !"<<std::endl;
        user_abort_transaction();
        return false;
    }
    
    return true;
}


bool test_drop_tablespace_with_unempty_db()
{
	try
	{
		begin_transaction();

        Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, true);
        if (InvalidOid == tblspcid)
        {
            tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
            RETURN_FALSE(tblspcid, "create tablespace failed !")
        }
        
		char dbname[] = {"my_db_a"};
		Oid dbid = my_create_database(dbname, TABLE_SPACE_NAME);
		RETURN_FALSE(dbid, "create database under table space failed !")
		
		Oid heapid = my_create_heap(TABLE_SPACE_NAME, dbname, RELATION_ID, COLUMN_ID);
		RETURN_FALSE(heapid, "create heap under table space failed !")
		
		commit_transaction();
		
		begin_transaction();
	
		my_drop_tablespace(TABLE_SPACE_NAME, false);

		commit_transaction();
	}
	catch(...)
	{
	    user_abort_transaction();
	    return true;
	}

    std::cout<<"drop tablespace with unempty database happened !"<<std::endl;
	return false;
}

bool test_drop_tablespace_with_empty_db()
{
	try
	{
		begin_transaction();
		Oid tblspcid = get_tablespace_oid(TABLE_SPACE_NAME, true);
		if (InvalidOid != tblspcid)
		{
		    tblspcid = my_create_tablespace(TABLE_SPACE_NAME, TABLE_SPACE_DIR_NAME.c_str());
		    RETURN_FALSE(tblspcid, "create tablespace failed !")		    
		}

		char dbname[] = {"my_db_a"};
		Oid dbid = my_create_database(dbname, TABLE_SPACE_NAME);
		RETURN_FALSE(dbid,"create database under table space failed !")

		my_drop_tablespace(TABLE_SPACE_NAME, false);
		
		commit_transaction();
	}
	catch(...)
	{
	    user_abort_transaction();
	    return true;
	}

    std::cout<<"drop tablespace with empty database happened !"<<std::endl;
	return false;
}

bool test_drop_default_tablespace()
{
    try
    {
        begin_transaction();
        my_drop_tablespace(DEFAULT_TABLESPACE_NAME, false);
        commit_transaction();
    }
    catch(ObjectInUseException& e)
    {
        std::cout<<e.getErrorMsg()<<std::endl;
        user_abort_transaction();
        return true;
    }
    
    std::cout<<"drop default tablespace happened !"<<std::endl;
    return false;
}

bool test_pause()
{
	char c;
	std::cin>>c;
	return true;
}

//////////////////////////////////////////////////////////////////////////
//utils
Oid my_create_tablespace(char* tblspcName, const char* tblspcDirName)
{
    using namespace boost::filesystem;

    path fullpath(initial_path());
    fullpath = system_complete(path(".")); 
    fullpath /= tblspcDirName;
    if (!bf::exists(fullpath))
    {
        bf::create_directories(fullpath);
        vMyCreatePath.push_back(fullpath.string());
    }

    my_talspc_path = fullpath.string();

    CreateTableSpaceStmt stmt;
    stmt.location = const_cast<char*>(my_talspc_path.c_str());
    stmt.owner = "";
    stmt.tablespacename = tblspcName;
    stmt.type = T_Invalid;
    Oid tblspcid = InvalidOid;
    THROW_CALL(tblspcid = CreateTableSpace,&stmt,true);
    CommandCounterIncrement();


    return tblspcid;
}

std::string my_create_absolute_path(std::string &strDirPath, const char* pchDirName, std::string &strDesPath)
{
	std::string strTbspacePath;
	if (strDirPath.at(strDirPath.length()-1) == '\\')
	{
		strTbspacePath = strDirPath + pchDirName;
	}
	else
	{
		strTbspacePath = strDirPath + "\\" + pchDirName;
	}

	if (strTbspacePath.length() > MAX_LENGTH_PATH)
	{
		std::cout<<"table space path is too long!"<<std::endl;
		strTbspacePath.clear();
	}
	strDesPath = strTbspacePath;

	return strTbspacePath;
}
Oid my_create_database(char* dbname, const char* tblspcname)
{
	TransactionId txId = GetCurrentTransactionId();
	Oid dbId = InvalidOid;
	THROW_CALL(dbId = CreateDatabase,txId,dbname,tblspcname);
	CommandCounterIncrement();
	my_database_name = dbname;

	return dbId;
}

void my_drop_tablespace(char* tblspcname, bool miss_ok)
{
    DropTableSpaceStmt stmt;
    stmt.missing_ok = miss_ok;
    stmt.tablespacename = tblspcname;
    stmt.type = T_Invalid;
    
    THROW_CALL(DropTableSpace,&stmt);
    CommandCounterIncrement();
}
Oid my_create_heap(const char* tblspcname, const char* dbname, const Oid& relid, const Oid& colid)
{
	ColinfoData* pColumnInfo = new ColinfoData;
	pColumnInfo->keys = 1;
	pColumnInfo->col_number = NULL;
	pColumnInfo->rd_comfunction = (CompareCallback*)str_compare;
	pColumnInfo->split_function = my_split;
	setColInfo(colid, pColumnInfo);

	Oid tablespaceId = InvalidOid;
	THROW_CALL(tablespaceId = get_tablespace_oid,tblspcname,false);
	SetCurrentDatabase(dbname, InvalidDatabaseID, InvalidTableSpaceID );
	Oid dbid = GetDatabaseOid(dbname);
	Oid heapid = FDPG_Heap::fd_heap_create(tablespaceId, relid, dbid, colid);
	CommandCounterIncrement();
	
	Relation rel = FDPG_Heap::fd_heap_open(heapid, ExclusiveLock,dbid);
	if (NULL == rel)
	{
	    return InvalidOid;
	}
	
	char chdata[] = "123456";
	HeapTuple tuple = FDPG_Heap::fd_heap_form_tuple(chdata,sizeof(chdata));
	FDPG_Heap::fd_simple_heap_insert(rel,tuple);
	FDPG_Heap::fd_heap_close(rel, ExclusiveLock);

	return heapid;
}

void my_drop_database(const char* chDbName)
{
    SetCurrentDatabase(DEFAULT_DATABASE_NAME, InvalidDatabaseID, InvalidTableSpaceID);
    THROW_CALL(DropDatabase,chDbName);
    CommandCounterIncrement();
}

extern void thread_drop_tablespcN(std::vector<std::string> *v_tblspc, 
																	bool *sta, BackendParameters *GET_PARAM());
extern void simple_create_tblspc(const string spcname,
																 const string dirname,
																 bool *retval);

static 
void thread_create_tbc(BackendParameters *GET_PARAM(), const char *tbc_name, bool *sta)
{
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(GET_PARAM());

	simple_create_tblspc(tbc_name, tbc_name, sta);

	proc_exit(0);
}

bool test_concurrently_create_tablespace()
{
	INTENT("多线程并发创建表空间，有任意一个线程创建失败说明测试失败。");

	using namespace std;
	using namespace boost;
	using namespace FounderXDB::StorageEngineNS;

#define CREATE_NUM 20

	bool result = true;
	bool sta[CREATE_NUM];

	PREPARE_TEST();

	/* 初始化表空间的名字和路径，表空间路径以表空间名字在数据库目录下创建 */
	vector<string> v_tblspc;
	for(int i = 0; i < CREATE_NUM; ++i)
	{
		char dat[100];
		memset(dat, 0, sizeof(dat));
		sprintf(dat, "tablespace%d", i + 1);
		v_tblspc.push_back(dat);
	}

	/* 创建CREATE_NUM个线程去并发创建CREATE_NUM个表空间 */
	for (int i = 0; i < CREATE_NUM; ++i)
	{
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(thread_create_tbc, GET_PARAM(), v_tblspc[i].c_str(), &sta[i]));
	}
	GET_THREAD_GROUP().join_all();

	/* 检测结果 */
	for (int i = 0; i < CREATE_NUM; ++i)
	{
		if (!sta[i])
		{
			result = false;
			break;
		}
	}

	/* 创建线程去删除所有表空间 */
	GET_PARAM() = get_param();
	SAVE_PARAM(GET_PARAM());
	GET_THREAD_GROUP().create_thread(bind(thread_drop_tablespcN, &v_tblspc, &result, GET_PARAM()));
	GET_THREAD_GROUP().join_all();

	FREE_PARAM(BackendParameters *);

	return result;
}