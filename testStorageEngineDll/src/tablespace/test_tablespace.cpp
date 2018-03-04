#include "tablespace/test_tablespace.h"
#include "heap/test_toast_dll.h"
#include <set>
#include <vector>
#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "index/test_index_insert_dll.h"
#include "EntrySet.h"
#include "backup_recovery/timelines.h"
#include "StorageEngineException.h"
extern Transaction *pTransaction;
using namespace std;
using namespace FounderXDB::StorageEngineNS;
extern StorageEngine* pStorageEngine ;
extern std::string g_strDataDir;
const std::string TABLE_SPACE_NAME_DLL = "my_table_space";
const std::string TABLE_SPACE_DIR_DLL = "table_space_test";
const std::string RECOVERY_TIME_FILE_NAME = "../tblspc_recovery_time_file";
const std::string CREATE_HEAP_ID_FILE = "../heap_id_file";
const std::string CREATE_INDEX_ID_FILE = "../index_id_file";
const std::string DATABASE_NAME_DLL = "my_db_a";
const uint32 HEAP_ID = 10010;
const uint32 INDEX_ID = 10011;
char chData[] = "123456";

bool show_err_msg(Transaction* pTrans, char* err)
{
    pTrans->abort();
    delete pTrans;
    pTrans = NULL;
    std::cout<<err<<std::endl;
    return false;
}

void my_split_tblspc(RangeDatai &range, const char*, int nCol, size_t charlen)
{

    range.start = 0;
    range.len = charlen;

    return;
}
void my_set_column_info(uint32 colid)
{
    ColumnInfo* pIndexColInfo = new ColumnInfo;
    pIndexColInfo->col_number = NULL;
    pIndexColInfo->keys = 1;
    pIndexColInfo->split_function = (Spliti)my_split_tblspc;
    pIndexColInfo->rd_comfunction = new CompareCallbacki[1];
    pIndexColInfo->rd_comfunction[0] = str_compare;
    setColumnInfo(colid, pIndexColInfo);
}

EntrySetID create_entryset_mine(Transaction* pTrans, uint32 colid,DatabaseID dbid, const char* tblspcname)
{
    EntrySetID heapid = 0;
    my_set_column_info(colid);
    heapid = pStorageEngine->createEntrySet(pTrans,colid,tblspcname,dbid);
    EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,heapid,dbid);
    DataItem item;
    item.data = chData;
    item.size = sizeof(chData);
    EntryID entryid;
    pEntrySet->insertEntry(pTrans, entryid,item);
    
    return heapid;
}
bool is_insert_data_valid(EntrySetID heapid, DatabaseID dbid)
{
    Transaction* pTrans = NULL;
    try
    {
        TransactionId txid = 0;
        pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
        EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans, EntrySet::OPEN_EXCLUSIVE, heapid,dbid);
        EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,(std::vector<ScanCondition>)NULL);
        EntryID entryid;
        DataItem item;
        pScan->getNext(EntrySetScan::NEXT_FLAG,entryid,item);
        char* pdata = (char*)item.getData();
        if (0 != strcmp(pdata,chData))
        {
            return show_err_msg(pTrans,"query insert data failed !");
        }
				pEntrySet->endEntrySetScan(pScan);
				pStorageEngine->closeEntrySet(pTrans, pEntrySet);
        pTrans->commit(); 
    }
    catch(StorageEngineException& e)
    {
        std::cout<<e.getErrorMsg()<<std::endl;
        pTrans->abort();
        delete pTrans;
        pTrans = NULL;
        return false;        
    }
    
    return true;
}
TableSpaceID create_tblspc_mine(const char* tblspcname, const char* dirname)
{
    using namespace boost::filesystem;
    TableSpaceID tblspcid = 0;
    path fullpath(initial_path());
    fullpath = system_complete(path(".")); 
    fullpath /= dirname;
    std::string strTblspcPath = fullpath.string();
    //std::cout<<">><<<<"<<strTblspcPath<<std::endl;
    if (!exists(strTblspcPath))
    {
        create_directory(strTblspcPath);
    }
    tblspcid = pStorageEngine->createTableSpace(tblspcname,strTblspcPath.c_str(),true);

    return tblspcid;
}

bool testCreateTablespace( void )
{
	Transaction *pTrans = NULL;

	try{
		TransactionId xid = 0;
        
		std::string strTablespace = g_strDataDir + "/tablespace";
		if (!boost::filesystem::exists(strTablespace))
		{
			boost::filesystem::create_directory(strTablespace);
		}
	    pStorageEngine->createTableSpace("test",strTablespace.c_str(),false);

		return true;
	}
	VOID_CATCHEXCEPTION(pTrans)

	return false;
}

bool testDropTablespace( void )
{
	Transaction *pTrans = NULL;

	try{

		pStorageEngine->dropTableSpace("test");

		return true;
	}
	VOID_CATCHEXCEPTION(pTrans)

		return false;
}

bool test_create_tblspc_prepare()
{
    if (!is_need_run())
        return true;
        
    using namespace boost::filesystem;
    namespace bf = boost::filesystem;

    path fullpath(initial_path());
    fullpath = system_complete(path(".")); 
    fullpath /= TABLE_SPACE_DIR_DLL;
    //std::string strTblspcPath = g_strDataDir;
    //strTblspcPath += "/";
    //strTblspcPath += TABLE_SPACE_DIR_DLL;
    std::string strTblspcPath = fullpath.string();
    std::cout<<">><<<<"<<strTblspcPath<<std::endl;
    if (!bf::exists(strTblspcPath))
        bf::create_directory(strTblspcPath);
    
    return true;
}

bool test_create_tblspc()
{
    if (!is_need_run())
        return true;

    Transaction *pTrans = NULL;
    try
    {
        TableSpaceID tblspcid = create_tblspc_mine(TABLE_SPACE_NAME_DLL.c_str(),TABLE_SPACE_DIR_DLL.c_str());
        if (0 == tblspcid)
        {
            std::cout<<"create table space failed! "<<std::endl;
            return false;
        }

        bool bret = database_heap_index_create(TABLE_SPACE_NAME_DLL.c_str());
        if (!bret)
        {
            std::cout<<"create database failed! "<<std::endl;
            return false;
        }
    }
    catch(StorageEngineException &ex)
    {
        cout << ex.getErrorNo() << endl;
        cout << ex.getErrorMsg() << endl;
        pTrans->abort();
        delete pTrans;
        pTrans = NULL;
        return false;
    }
    
    return true;
}

bool test_create_heap_under_diff_tblspc()
{
    Transaction* pTrans = NULL;
    try
    {
        TransactionId txid = 0;
        //pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
        //create three tablespaces
        char chTblspcName[][25] = {"my_tblspc_name_one",
            "my_tblspc_name_two",
            "my_tblspc_name_three"};

        char chTblspcDirName[][15] = {"my_tblspc_dir1",
            "my_tblspc_dir2",
            "my_tblspc_dir3"};

        for (int i = 0; i < 3; i++)
        {
            create_tblspc_mine(chTblspcName[i],chTblspcDirName[i]);
        }
        //pTrans->commit();
        
        //create database
        txid = 0;
        pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
        char chDbname[] = "my_db_x";
        DatabaseID dbid = pStorageEngine->createDatabase(pTrans,chDbname,chTblspcName[0]);
        pTrans->commit();
        
        //create heap
        txid = 0;
        pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
        EntrySetID heapid0 = create_entryset_mine(pTrans,HEAP_ID,dbid,chTblspcName[0]);
        EntrySetID heapid1 = create_entryset_mine(pTrans,HEAP_ID + 1,dbid,chTblspcName[1]);
        EntrySetID heapid2 = create_entryset_mine(pTrans,HEAP_ID + 2,dbid,chTblspcName[2]);
        pTrans->commit();
        
        //query data from heap
        CHECK_BOOL(is_insert_data_valid(heapid0,dbid));
        CHECK_BOOL(is_insert_data_valid(heapid1,dbid));
        CHECK_BOOL(is_insert_data_valid(heapid2,dbid));
        
        //drop database
        txid = 0;
        pTrans = pStorageEngine->getTransaction(txid,Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
        //pStorageEngine->setCurrentDatabase("defaultdatabase");
        pStorageEngine->dropDatabase(pTrans,chDbname);
        pTrans->commit();
        
        //drop table space
        pStorageEngine->dropTableSpace(chTblspcName[0]);
        pStorageEngine->dropTableSpace(chTblspcName[1]);
        pStorageEngine->dropTableSpace(chTblspcName[2]);
        
        //remove directory
        remove_my_dir(chTblspcDirName[0]);
        remove_my_dir(chTblspcDirName[1]);
        remove_my_dir(chTblspcDirName[2]);
    }
    
    VOID_CATCHEXCEPTION(pTrans)
    
    return true;   
}
bool test_drop_tblspc()
{
    if (!is_need_run())
        return true;

    Transaction *pTrans = NULL;
    try
    {
        TransactionId xid = 0;
        pTrans = pStorageEngine->getTransaction(xid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
        pStorageEngine->setCurrentDatabase("defaultdatabase");
        pStorageEngine->dropDatabase(pTrans, DATABASE_NAME_DLL.c_str());
        pTrans->commit();
        
        TableSpaceID tblspcid = pStorageEngine->dropTableSpace( TABLE_SPACE_NAME_DLL.c_str());
    }
    catch(StorageEngineException &ex)
    {
        cout << ex.getErrorNo() << endl;
        cout << ex.getErrorMsg() << endl;
        pTrans->abort();
        delete pTrans;
        pTrans = NULL;
        return false;
    }

    return true;
}
bool test_tblspc_recovery()
{
    if (!is_need_run())
        return true;

    Transaction* pTrans = NULL;
    try
    {
        TransactionId txd = 0;
        pTrans = pStorageEngine->getTransaction(txd, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        TableSpaceID tblspcid = pStorageEngine->getTableSpaceID(pTrans,TABLE_SPACE_NAME_DLL.c_str());
        if (0 == tblspcid)
            return show_err_msg(pTrans,"recovery table space failed !");
            
        pStorageEngine->setCurrentDatabase(DATABASE_NAME_DLL.c_str());
        DatabaseID dbid = pStorageEngine->getDatabaseID(DATABASE_NAME_DLL.c_str());
        if(0 == dbid)
            return show_err_msg(pTrans,"recovery database failed !");
        
        //read heap id
        EntrySetID heapid = 0;
        std::string strHeap;
        std::fstream fWrite(CREATE_HEAP_ID_FILE.c_str(),std::ios_base::out|std::ios_base::in);
        if (fWrite.is_open())
        {
            getline(fWrite,strHeap);
        }
        if (strHeap.empty())
            return show_err_msg(pTrans,"heap id is not saved !");
        fWrite.close();
        heapid = atoi(strHeap.c_str());
        EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans, EntrySet::OPEN_EXCLUSIVE,heapid,dbid);
        if (NULL == pEntrySet)
            return show_err_msg(pTrans,"recovery entryset failed !");
        
        //read index id from file
        EntrySetID indexid = 0;
        std::string strIndex;
        fWrite.open(CREATE_INDEX_ID_FILE.c_str(),std::ios_base::out|std::ios_base::in);
        if (fWrite.is_open())
        {
            getline(fWrite,strIndex);
        }
        if (strIndex.empty())
            return show_err_msg(pTrans,"index id is not saved !");
        fWrite.close();
        indexid = atoi(strIndex.c_str());
        IndexEntrySet* pIndex = pStorageEngine->openIndexEntrySet(pTrans, pEntrySet,EntrySet::OPEN_EXCLUSIVE,indexid,dbid);
        if (NULL == pIndex)
            return show_err_msg(pTrans,"index recovery failed !");
       
        EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);
		if (NULL == pScan)
		{
			std::cout<<"pScan == NULL"<<std::endl;
		}

        DataItem scanitem;
        EntryID entryid;
        pScan->getNext(EntrySetScan::NEXT_FLAG,entryid,scanitem);
        char* pdata = (char*)scanitem.getData();
        if (0 != strcmp(pdata,chData))
		{
			std::cout<<"恢复后数据："<<pdata<<std::endl;
			std::cout<<"恢复前数据："<<chData<<std::endl;
            return show_err_msg(pTrans,"recovery data from entry set failed !");
		}
               
        int nByte = 1<<10;
        char* pLargeData = (char*)malloc(nByte+1);
        memset(pLargeData,1,nByte);
        memset(pLargeData+nByte,0,1); 

        pScan->getNext(EntrySetScan::NEXT_FLAG,entryid,scanitem);
        pdata = (char*)scanitem.getData();
        if (0 != strcmp(pdata,pLargeData))
            return show_err_msg(pTrans,"recovery large data into entry set failed !");
    }
    catch(StorageEngineException &ex)
    {
        cout << ex.getErrorNo() << endl;
        cout << ex.getErrorMsg() << endl;
        pTrans->abort();
        delete pTrans;
        pTrans = NULL;
        return false;
    }
    
    return true;
}

bool test_get_tblspc_oid()
{
    Transaction* pTrans = NULL;
    try
    {
        TransactionId txd = 0;
        pTrans = pStorageEngine->getTransaction(txd, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
        //TableSpaceID tblspcid = pStorageEngine->createTableSpace(pTrans, TABLE_SPACE_NAME_DLL.c_str(), TABLE_SPACE_DIR_DLL)
        pTrans->commit();
    }
    VOID_CATCHEXCEPTION(pTrans)
    
    return true;
}

bool database_heap_index_create(const char* tblpsc)
{
    TransactionId txid = 0;
    Transaction* pTrans = pStorageEngine->getTransaction(txid,Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
    DatabaseID dbid = pStorageEngine->createDatabase(pTrans,
        DATABASE_NAME_DLL.c_str(), 
        tblpsc);
    
    pStorageEngine->setCurrentDatabase(DATABASE_NAME_DLL.c_str());
    my_set_column_info(HEAP_ID);
    EntrySetID heapid = pStorageEngine->createEntrySet(pTrans, HEAP_ID, TABLE_SPACE_NAME_DLL.c_str(),dbid);
    
    //create index
    EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans, EntrySet::OPEN_EXCLUSIVE, heapid,dbid);
    if (NULL == pEntrySet)
    {
        std::cout<<"open entry set failed !"<<std::endl;
        pTrans->abort();
        delete pTrans;
        pTrans = NULL;
        return false;
    }

    ColumnInfo* pIndexColInfo = new ColumnInfo;
    pIndexColInfo->col_number = new size_t[1];
    pIndexColInfo->col_number[0] = 1;
    pIndexColInfo->keys = 1;
    pIndexColInfo->split_function = (Spliti)my_split_tblspc;
    pIndexColInfo->rd_comfunction = new CompareCallbacki[1];
    pIndexColInfo->rd_comfunction[0] = str_compare;
    setColumnInfo(INDEX_ID, pIndexColInfo);
    EntrySetID indexid = pStorageEngine->createIndexEntrySet(pTrans,
        pEntrySet,
        BTREE_INDEX_ENTRY_SET_TYPE,
        INDEX_ID,
        TABLE_SPACE_NAME_DLL.c_str(),
        dbid);
    
    //insert normal data to entry set
    DataItem item;
    item.data = chData;
    item.size = sizeof(chData);
    EntryID entryid;
    pEntrySet->insertEntry(pTrans,entryid, item);
    //insert large data
    int nByte = 1<<10;
    char* pLargeData = (char*)malloc(nByte+1);
    memset(pLargeData,1,nByte);
    memset(pLargeData+nByte,0,1); 
    item.data = pLargeData;
    item.size = strlen(pLargeData) + 1;
    pEntrySet->insertEntry(pTrans,entryid,item);
    pStorageEngine->closeEntrySet(pTrans, pEntrySet);
    
    pTrans->commit();
    
    //query insert data
    pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
    pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,heapid, dbid);
    EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans, 
                                                        BaseEntrySet::SnapshotMVCC, 
                                                        (std::vector<ScanCondition>)NULL);
    DataItem scanitem;
    pScan->getNext(EntrySetScan::NEXT_FLAG,entryid,scanitem);
    char* pdata = (char*)scanitem.getData();
    if (0 != strcmp(pdata,chData))
    {
        std::cout<<"insert data into entry set failed !"<<std::endl;
        pTrans->abort();
        delete pTrans;
        pTrans = NULL;
        return false;
    }
    pScan->getNext(EntrySetScan::NEXT_FLAG,entryid,scanitem);
    pdata = (char*)scanitem.getData();
    if (0 != strcmp(pdata,pLargeData))
    {
        std::cout<<"insert large data into entry set failed !"<<std::endl;
        pTrans->abort();
        delete pTrans;
        pTrans = NULL;
        return false;
    }
    free(pLargeData);
    pStorageEngine->closeEntrySet(pTrans,pEntrySet);
    pTrans->commit();
    write_heap_id_to_file(CREATE_HEAP_ID_FILE.c_str(), heapid);
    write_heap_id_to_file(CREATE_INDEX_ID_FILE.c_str(), indexid);
    write_recovery_time_to_file();
    
    pTrans = pStorageEngine->getTransaction(txid,Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
    pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,heapid,dbid);
    pScan = pEntrySet->startEntrySetScan(pTrans, 
                                            BaseEntrySet::SnapshotMVCC, 
                                            (std::vector<ScanCondition>)NULL);
    pScan->getNext(EntrySetScan::NEXT_FLAG,entryid,scanitem);
    pEntrySet->deleteEntry(pTrans,entryid);
    pTrans->commit();
    
    return true;
}
void write_recovery_time_to_file()
{
    MySleep(1000);
    char chTime[20];
    time_t t;
    t = time(&t);
    struct tm *pTm = localtime(&t);
    strftime(chTime,sizeof(chTime),"%Y-%m-%d %H:%M:%S",pTm);
    
    std::string strLine = "recoveryTargetTime = ";
    strLine += chTime;
    
    std::fstream fWrite(RECOVERY_TIME_FILE_NAME.c_str(),std::ios_base::out);
    if (fWrite.is_open())
    {
        fWrite<<strLine.c_str()<<std::endl;
    }

    fWrite.close();
}

void write_heap_id_to_file(const char* filename, int heapid)
{
    char chName[30];
    memset(chName,0,sizeof(chName));
    sprintf(chName, "%d",heapid);

    std::fstream fWrite(filename,std::ios_base::out);
    if (fWrite.is_open())
    {
        fWrite<<chName<<std::endl;
    }

    fWrite.close();
}

void remove_my_dir(const char* dirname)
{
    using namespace boost::filesystem;
    path fullpath(initial_path());
    fullpath = system_complete(path("."));
    fullpath /= dirname;
    std::string strTblspcPath = fullpath.string();
    
    if (exists(strTblspcPath))
    {
        remove_all(strTblspcPath);
    }
}

bool test_get_create_db_oid()
{
    const uint32 MIN_DB_OID = 11967;
    Transaction* pTrans = NULL;
    TransactionId txid = 0;
    try
    {
        char chDbName[] = "my_db_x0";
        pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
        DatabaseID dbid = pStorageEngine->createDatabase(pTrans, chDbName, "defaulttablespace");
				/* 
				 * 以前的数据库ID应该是大于11967的，但是现在改为跟表ID使用相同的分配方法，所以 
				 * 有可能数据库的ID分配是小于11967的
				 */
        if (dbid <= 0)
        {
            std::cout<<"create database with dbid <= 0 !"<<std::endl;
            pTrans->abort();
            delete pTrans;
            pTrans = NULL;
            return false;
        }
        pTrans->commit();
        
		txid = 0;
		pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
        pStorageEngine->dropDatabase(pTrans, chDbName);
        pTrans->commit();
    }
    VOID_CATCHEXCEPTION(pTrans)
    
    return true;
}

bool TestDBExtraData( void )
{
	Transaction* pTrans = NULL;
	TransactionId txid = 0;
	std::string strDBName("shgliDB");
	std::string strExtra("niwoweiwewowewewewio");
	try
	{
		char* p = NULL;
		DatabaseID dbid =  0;
		//创建数据库并检查extra data
		{
			txid = 0;
			pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
			dbid = pStorageEngine->createDatabase(pTrans,strDBName.c_str(),"defaulttablespace",strExtra.c_str(),strExtra.length() + 1);

			DBMetaInfo info;
			pStorageEngine->getDatabaseInfo(strDBName.c_str(),info);

			p = (char*)info.getMetaData();
			BOOST_CHECK_EQUAL(dbid,info.dbid);
			BOOST_CHECK((0 == strcmp(strExtra.c_str(),p)));
			pTrans->commit();
		}

		
        //重启一个事务，检查extra data
		{
			txid = 0;
			pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);

			DBMetaInfo info2;
			pStorageEngine->getDatabaseInfo(strDBName.c_str(),info2);

			p = (char*)info2.getMetaData();
			BOOST_CHECK_EQUAL(dbid,info2.dbid);
			BOOST_CHECK((0 == strcmp(strExtra.c_str(),p)));
			pTrans->commit();
		}


		//更新extra data
		{
			strExtra.clear();
			txid = 0;
			pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
			strExtra += "ewionwiowtaewauaw";
			pStorageEngine->updateDatabaseExtraData(strDBName.c_str(),strExtra.c_str(),strExtra.length() + 1);

			DBMetaInfo info1;
			pStorageEngine->getDatabaseInfo(strDBName.c_str(),info1);

			p = (char*)info1.getMetaData();
			BOOST_CHECK_EQUAL(dbid,info1.dbid);
			BOOST_CHECK((0 == strcmp(strExtra.c_str(),p)));
			pTrans->commit();
		}

		//重启一个事务来检查extra data
		{
			txid = 0;
			pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);

			DBMetaInfo info3;
			pStorageEngine->getDatabaseInfo(strDBName.c_str(),info3);

			p = (char*)info3.getMetaData();
			BOOST_CHECK_EQUAL(dbid,info3.dbid);
			BOOST_CHECK((0 == strcmp(strExtra.c_str(),p)));
			pTrans->commit();
		}


		//删除数据库
		{
			txid = 0;
			pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
			pStorageEngine->dropDatabase(pTrans, strDBName.c_str());
			pTrans->commit();
		}
	}
	VOID_CATCHEXCEPTION(pTrans)

	return true;
}

bool TestGetAllDBInfos( void )
{
	Transaction* pTrans = NULL;
	TransactionId txid = 0;
	std::string strDBName1("shgliDB1");
	std::string strDBName2("shgliDB2");
	std::string strExtra1("niwoweiwewowewewewio");
    std::string strExtra2("mmniwoweiwewowewewewio");
	try
	{
		char* p = NULL;
		DatabaseID dbid1 =  0;
		DatabaseID dbid2 =  0;
		//创建两个数据库
		{
			txid = 0;
			pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
			dbid1 = pStorageEngine->createDatabase(pTrans,strDBName1.c_str(),"defaulttablespace",strExtra1.c_str(),strExtra1.length() + 1);
			dbid2 = pStorageEngine->createDatabase(pTrans,strDBName2.c_str(),"defaulttablespace",strExtra2.c_str(),strExtra2.length() + 1);
            pTrans->commit();
		}

		{
			 size_t nSize;
			 std::vector<DBMetaInfo>pInfos;
			 txid = 0;
             pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
			 pStorageEngine->getAllDatabaseInfos(pInfos);
             nSize = pInfos.size();
			 for (int i = 0; i < nSize; ++i)
			 {
                 p = (char*)pInfos[i].getMetaData();
				 int id = pInfos[i].dbid;
				 ++id;
			 }
			 pTrans->commit();
		}

		{
			txid = 0;
			pTrans = pStorageEngine->getTransaction(txid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			pStorageEngine->dropDatabase(pTrans,strDBName1.c_str());
			pStorageEngine->dropDatabase(pTrans,strDBName2.c_str());
			pTrans->commit();
		}
	}
	CATCHEXCEPTION1(pTrans)

	return true;
}

bool TestGetDBInfoById( void )
{
	DatabaseID dbid = 0;
	std::string strDBName("testDB");
	std::string strExtra("niwoweiwewowewewewio");
	Transaction *pTrans = NULL;

	try
	{
		TransactionId txid = 0;

		/* create two database */
		pTrans = pStorageEngine->getTransaction(txid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);
		dbid = pStorageEngine->createDatabase(pTrans, strDBName.c_str(), "defaulttablespace", strExtra.c_str(), strExtra.length() + 1);
        pTrans->commit();

		/* get DBMetaInfo*/
		DBMetaInfo dbmeta;
		bool get_ok = false;
		txid = 0;
		pTrans = pStorageEngine->getTransaction(txid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		if(pStorageEngine->getDatabaseInfoByID(dbid, dbmeta)){
			if((dbid == dbmeta.dbid) && (dbmeta.dbname == strDBName)){
				get_ok = true;
			}
		}
		if(!get_ok){
			printf("Get database failed.\n");

			pTrans->abort();
			delete pTrans;
			pTrans = NULL;
			return false;
		}

		/* drop database */
		pTrans = pStorageEngine->getTransaction(txid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pStorageEngine->dropDatabase(pTrans,strDBName.c_str());
		pTrans->commit();
	}
	CATCHEXCEPTION1(pTrans)

	return true;
}

bool test_getalldatabaseInfo_tbspc()
{

	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		std::string strDir = "tbspc_test";
		TableSpaceID tbid = InvalidTableSpaceID;
		tbid = create_tblspc_mine("test_tbspc",strDir.c_str());

		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid, Transaction::READ_UNCOMMITTED_ISOLATION_LEVEL);

		DatabaseID dbid = pStorageEngine->createDatabase(pTrans,
			"test_db", 
			"test_tbspc");

		//pStorageEngine->setCurrentDatabase("test_db");

		std::vector<DBMetaInfo> dbs;
		pSE->getAllDatabaseInfos(dbs);

		pTrans->commit();

	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		return false;
	}

	return true;
}