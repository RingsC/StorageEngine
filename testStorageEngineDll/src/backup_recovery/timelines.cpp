#include <iostream>
#include "backup_recovery/timelines.h"
#include "test_fram.h"

#include "utils/utils_dll.h"
#include "StorageEngineException.h"
//#include "StorageEngine.h"
#include "PGSETypes.h"
#include "Transaction.h"
#include <boost/filesystem.hpp>

#include <boost/filesystem/path.hpp>
#include "backup_recovery/BRTest.h"
#include <ctime>
#include <boost/thread/thread.hpp>
#include <stdio.h>
#include "utils/attr_setter.h"
#include "Configs.h"
#include <time.h>
#include <fstream>

using namespace boost;

const std::string TABLE_NAME_TXT = "../heapid_columnid.txt";
const std::string RECOVERY_TIME_FILE = "../recovery_time_file.txt";
const std::string DATA_DIR_SRC = "E:\\Data_backup";

const uint32 BACKUP_HEAP_COLUMN_ID = 10000;
const uint32 BACKUP_INDEX_COLUMN_ID = 10100;
//EntrySetID entrySetId = 0;
EntrySetID entryIndexId = 0;
const std::string strDbPath = "E:\\Data\\base\\11967\\";
extern std::map<std::string,std::string> ProgramOpts;


void my_heap_split_ten(RangeDatai& rangedata, const char* str,int col,size_t len = 0)
{
	rangedata.len = 0;
	rangedata.start = 0;

	if (NULL != str)
	{
		rangedata.len = len;
	}
}

bool make_base_backup()
{
    if (!is_need_run())
        return true;

	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		FXTransactionId fxTid = 0;
		Transaction* pTransaction = pStorageEngine->getTransaction(fxTid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		//make a base backup
		pStorageEngine->startArchive(true);
		std::string strDir = ProgramOpts["-d"];

		thread_group thrGroup;
		thrGroup.create_thread(boost::bind(BackUpThread,boost::ref(strDir)));
		thrGroup.join_all();

		pTransaction->commit();
	}
	catch (StorageEngineException& e)
	{
		std::cout<<e.getErrorNo()<<std::endl;
		std::cout<<e.getErrorMsg()<<std::endl;

		return false;
	}
	return true;
}

void write_heapname_columnId(uint32 unHeapName, uint32 unColumnId)
{
	MySleep(1000);
	char chTime[20];
	time_t t;
	t = time(&t);
	struct tm *pTm = localtime(&t);
	strftime(chTime,sizeof(chTime),"%Y-%m-%d %H:%M:%S",pTm);
	std::string strTime;
	strTime = chTime;
	strTime += "#";

	char chName[30];
	memset(chName,0,sizeof(chName));
	sprintf(chName, "%d#%d",unHeapName, unColumnId);

	std::string strLine = strTime;
	strLine += chName;
	std::fstream fWrite(TABLE_NAME_TXT.c_str(),std::ios_base::app|std::ios_base::out|std::ios_base::in	);
	if (	fWrite.is_open())
	{
		fWrite<<strLine<<std::endl;
	}

	fWrite.close();
}

bool create_heap_by_id(uint32 unColumnId)
{
	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		FXTransactionId fxTid = 0;
		Transaction* pTransaction = pStorageEngine->getTransaction(fxTid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		//create heap
		my_heap_setcolumnInfo(unColumnId);
		EntrySetID entrySetId = pStorageEngine->createEntrySet(pTransaction, unColumnId);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, entrySetId);
		bool bIsSucceed = true;
		if(NULL == pEntrySet)
			bIsSucceed = false;
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pTransaction->commit();

		//write heap name and column id to file
		if(bIsSucceed)
			write_heapname_columnId(entrySetId,unColumnId);
	}
	catch (StorageEngineException& e)
	{
		std::cout<<e.getErrorNo()<<std::endl;
		std::cout<<e.getErrorMsg()<<std::endl;

		return false;
	}

	return true;		
}
void my_heap_setcolumnInfo(uint32 unColumnId)
{
	//set heap columninfo
	ColumnInfo* pColumnInfo = new ColumnInfo;
	pColumnInfo->col_number = NULL;
	pColumnInfo->keys = 1;
	pColumnInfo->split_function = my_heap_split_ten;
	pColumnInfo->rd_comfunction = NULL;
	setColumnInfo(unColumnId, pColumnInfo);
}

namespace bf = boost::filesystem;
/************************************************************************** 
* @brief CopyFiles 
* 
* Detailed description 递归地拷贝from到to
* @param[in] from 被拷贝的文件夹或者文件
* @param[in] to   目的文件夹或者文件
**************************************************************************/
static void CopyFiles( const bf::path& from, const bf::path& to )
{
	if (boost::filesystem::exists(to))
	{
		boost::filesystem::remove_all(to);
	}

	if( !bf::exists(to) )
	{
		bf::create_directories( to );
	}    

	bf::directory_iterator end;

	for( bf::directory_iterator it(from); it != end; ++it )
	{
		bf::path newFrom = from;
		newFrom /= ((boost::filesystem::path)(*it)).filename();

		bf::path newTo = to;
		newTo /= ((boost::filesystem::path)(*it)).filename();

		if( bf::is_directory( newFrom ) )
		{
			CopyFiles( newFrom, newTo );
		}
		else if( bf::is_regular( newFrom ) )
		{
			bf::copy_file( newFrom, newTo, bf::copy_option::overwrite_if_exists );
		}
	}
}
bool test_base_backup()
{
	if (!is_need_run())
		return true;

	storage_params* pStorePara = GetStorageParam(ProgramOpts["-conf"]);
	if (pStorePara->doRecovery)
		return true;

	//remove history file
	remove(TABLE_NAME_TXT.c_str());

	//create three heaps;
	uint32 unColumnId = BACKUP_HEAP_COLUMN_ID;
	CHECK_BOOL(create_heap_by_id(unColumnId));
	MySleep(2000);

	CHECK_BOOL(make_base_backup());

	unColumnId = BACKUP_HEAP_COLUMN_ID + 1;
	CHECK_BOOL(create_heap_by_id(unColumnId));
	MySleep(2000);
	write_recovery_time_file(unColumnId);

	unColumnId = BACKUP_HEAP_COLUMN_ID + 2;
	CHECK_BOOL(create_heap_by_id(unColumnId));
	MySleep(2000);

	return true;
}

void my_heap_index_setColinfo()
{
	ColumnInfo* pIndexColInfo = new ColumnInfo;
	pIndexColInfo->col_number = new size_t[1];
	pIndexColInfo->col_number[0] = 1;
	pIndexColInfo->keys = 1;
	pIndexColInfo->split_function = my_heap_split_ten;
	pIndexColInfo->rd_comfunction = new CompareCallbacki[1];
	pIndexColInfo->rd_comfunction[0] = str_compare;
	setColumnInfo(BACKUP_INDEX_COLUMN_ID, pIndexColInfo);
}

bool test_recovery_modify()
{
	if (!is_need_run())
		return true;

	uint32 unColumnId = BACKUP_HEAP_COLUMN_ID + 3;
	CHECK_BOOL(create_heap_by_id(unColumnId));
	MySleep(2000);
	write_recovery_time_file(BACKUP_HEAP_COLUMN_ID + 2);

	return true;
}
bool test_heap_recovery()
{
	if (!is_need_run())
		return true;

	bool bRet = test_heap_level_recovery();
	return bRet;
}

bool test_heap_recovery_again()
{
	if (!is_need_run())
		return true;

	bool bRet = test_heap_level_recovery();
	test_clear_files();

	return bRet;
}

bool test_heap_level_recovery()
{
	storage_params* pStorePara = GetStorageParam(ProgramOpts["-conf"]);
	if (pStorePara->XLogArchiveMode)
		return true;

	std::vector<HeapInfo> vExist;
	std::vector<HeapInfo> vDelete;
	std::ifstream readFile;
	readFile.open(TABLE_NAME_TXT.c_str(),std::ios_base::out);
	if (readFile.is_open())
	{
		std::string strLine;
		while (!readFile.eof())
		{
			getline(readFile,strLine);
			if(strLine.empty())
				continue;

			//find the time
			int nTimePos = strLine.find("#");
			std::string strTime = strLine.substr(0,nTimePos);
			//find heap name
			int nHeapPos = strLine.find("#",nTimePos+1);
			std::string strHeapName = strLine.substr(nTimePos+1,nHeapPos - nTimePos-1);
			//find column id
			std::string strColumnId = strLine.substr(nHeapPos + 1,strLine.length() - nHeapPos);

			int nHeapId = atoi(strHeapName.c_str());
			int nColumnId = atoi(strColumnId.c_str());	
			HeapInfo heap;
			heap.unHeapName = nHeapId;
			heap.unColumnId = nColumnId;

			if (strTime.compare(pStorePara->recoveryTargetTime) > 0)
				vDelete.push_back(heap);
			else
				vExist.push_back(heap);
		}
	}

	bool bRet = true;
	for (std::vector<HeapInfo>::iterator it = vExist.begin();it != vExist.end(); it ++)
	{
		CHECK_BOOL(bRet = is_heap_exists(*it));
		MySleep(1000);
	}

	for (std::vector<HeapInfo>::iterator it = vDelete.begin(); it != vDelete.end(); it ++)
	{
		CHECK_BOOL(bRet = !is_heap_exists(*it));
		MySleep(1000);
	}

	return bRet;
}

bool is_heap_exists(HeapInfo heap)
{
	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		FXTransactionId fxTid = 0;
		Transaction* pTransaction = pStorageEngine->getTransaction(fxTid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		my_heap_setcolumnInfo(heap.unColumnId);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, heap.unHeapName);
		if (NULL == pEntrySet)
		{
			std::cout<<"heap not exist !"<<std::endl;
			return false;
		}

		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		pTransaction->commit();
	}
	catch (StorageEngineException& e)
	{
		std::cout<<e.getErrorNo()<<std::endl;
		std::cout<<e.getErrorMsg()<<std::endl;

		return false;
	}

	return true;
}

void write_recovery_time_file(uint32 unColumnId)
{
	std::string strTarget = "recoveryTargetTime  = ";

	std::fstream fReadTime(TABLE_NAME_TXT.c_str(), std::ios_base::in|std::ios_base::out);
	if (fReadTime.is_open())
	{
		std::string strLine;
		while (!fReadTime.eof())
		{
			getline(fReadTime,strLine);
			if(strLine.empty())
				continue;

			//find the time
			int nTimePos = strLine.find("#");
			std::string strTime = strLine.substr(0,nTimePos);
			//find heap name
			int nHeapPos = strLine.find("#",nTimePos+1);
			std::string strHeapName = strLine.substr(nTimePos+1,nHeapPos - nTimePos-1);
			//find column id
			std::string strColumnId = strLine.substr(nHeapPos + 1,strLine.length() - nHeapPos);

			int nColumnId = atoi(strColumnId.c_str());	
			if (nColumnId == unColumnId)
			{
				strTarget += strTime;
				break;
			}
		}
	}

	std::fstream fWrite(RECOVERY_TIME_FILE.c_str(),std::ios_base::trunc|std::ios_base::out);
	if (	fWrite.is_open())
	{
		fWrite<<strTarget<<std::endl;
	}

	fWrite.close();
}

void test_clear_files()
{
	remove(RECOVERY_TIME_FILE.c_str());
	remove(TABLE_NAME_TXT.c_str());
}

bool is_need_run()
{
	if (	ProgramOpts.find("-hh") != ProgramOpts.end())
		return true;
	return false;
}
