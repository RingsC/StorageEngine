#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <time.h>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/preprocessor/repeat.hpp>
#include "attr_setter.h"
#include "perform_utils.h"
#include "boost/timer.hpp"
#include "StorageEngineException.h"
#include "StorageEngine.h"
#include "Transaction.h"
#include "MemoryContext.h"
#include "boost/thread.hpp"
#include "boost/format.hpp"

using namespace FounderXDB::StorageEngineNS;

int str_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
	size_t i = 0;
	while(i < len1 && i < len2)
	{
		if(str1[i] < str2[i])
			return -1;
		else if(str1[i] > str2[i])
			return 1;
		else i++;

	}
	if(len1 == len2)
		return 0;
	if(len1 > len2)
		return 1;
	return -1;
}

int start_engine_()
{
	extern std::string g_strDataDir;
	StorageEngine::getStorageEngine()->initialize(const_cast<char*>(g_strDataDir.c_str()), 80, get_param());
	return 1;
}

int stop_engine_()
{
	StorageEngine::getStorageEngine()->shutdown();
	return 1;
}


extern std::map<std::string,std::string> ProgramOpts;
storage_params *get_param( void )
{
	storage_params* pRet = NULL;
	if (ProgramOpts.find("-conf") != ProgramOpts.end())
	{
		return GetStorageParam(ProgramOpts["-conf"]);
	}
	else if (ProgramOpts.find("-c") != ProgramOpts.end())
	{
		if ("archive" == ProgramOpts["-c"])
		{
			static storage_params para;
			para.doRecovery = false;
			para.XLogArchiveMode = true;	
			//para.XLogArchiveCommand = "python copy.py \"%p\" \"../archive/%f\"";
			para.XLogArchivePath = "../archive";
			pRet = &para;
		}
		else if ("recovery" == ProgramOpts["-c"])
		{
			static storage_params para;
			para.doRecovery = true;
			//para.recoveryRestoreCommand = "python copy.py  \"../archive/%f\" \"%p\"";
			para.recoveryRestorePath = "../archive";
			pRet = &para;
		}
	}
	return pRet;
}

std::string GenerateRandomString(uint32 len)
{
	std::string strRet;

	char chDict[] = {"123456789abcdefghijklmnopqrstuvwxyz!@#$%^&*()_+|.~=-/"};
	uint32 arr_len = sizeof(chDict);

	for (uint32 i = 0; i < len; i ++)
	{
		uint32 index = rand() / (RAND_MAX/(arr_len-1));
		if (index < arr_len)
		{
			char chTmp[2] = {0};
			memcpy(chTmp,chDict + index,1);
			std::string strTmp(chTmp);
			strRet += strTmp;
		}
	}

	return strRet;
}
//通过命令行读取测试数据量，如果没有默认为10000
uint32 GetDataCount()
{
	uint32 count = 10000;

	std::map<std::string,std::string>::iterator it = ProgramOpts.find("-test_count");
	if (ProgramOpts.end() != it)
	{
		count = atoi(it->second.c_str());
	}

	return count;
}
//get tuple length during cmdline , 10 default
uint32 GetTupleLen()
{
	uint32 len = 10;
	std::map<std::string,std::string>::iterator it = ProgramOpts.find("-test_len");
	if (ProgramOpts.end() != it)
	{
		len = atoi(it->second.c_str());
	}

	return len;
}
void perform_split_1_col(RangeDatai& range, const char* pSrc, int colnum, size_t len)
{
	range.start = 0;
	range.len = 0;

	if (1 == colnum)
	{
		range.len = len;
	}
}
void MySetColInfo_Heap(const uint32& colId)
{
	ColumnInfo *pCol = new ColumnInfo;
	pCol->col_number = NULL;
	pCol->keys = 0;
	pCol->rd_comfunction = NULL;
	pCol->split_function = perform_split_1_col;
	setColumnInfo(colId,pCol);
}
void MySetColInfo_Index(const uint32& colId)
{
	ColumnInfo *pCol = new ColumnInfo;
	pCol->col_number = new size_t[1];
	pCol->col_number[0] = 1;
	pCol->keys = 1;
	pCol->rd_comfunction = new CompareCallbacki[1];
	pCol->rd_comfunction[0] = str_compare;
	pCol->split_function = perform_split_1_col;
	setColumnInfo(colId,pCol);
}
void GetTestDataPtr(TestData& data)
{
	if (data.tuple_count == 0 || data.tuple_len == 0)
		return;

	char* pData = data.pData;
	if (!pData)
	{
		pData = (char*)malloc(data.tuple_count * data.tuple_len);
		data.pData = pData;
	}
	
	if (pData)
	{
		memset(pData,0,data.tuple_count * data.tuple_len);
		char* pTmp = pData;
		srand((int)time(0));
		for (uint32 i = 0; i < data.tuple_count; i ++)
		{
			std::string strRandom = GenerateRandomString(data.tuple_len);
			memcpy(pTmp,strRandom.c_str(),data.tuple_len);
			pTmp += data.tuple_len;
		}
	}

	assert(pData);
}
/************************************************************************/
/* CPerformBase 执行类的基类                                            */
/************************************************************************/
CPerformBase::CPerformBase() : m_EntrySetID(InvalidEntrySetID)
{

}
uint32 CPerformBase::MyGetColId()
{
	static uint32 col_base_id = 10000;

	return col_base_id++;
}
EntrySetID CPerformBase::Create_EntrySet(bool bIsMultiCol, bool bWithIndex,uint32& colId)
{
	EntrySetID entrySetId = InvalidEntrySetID;
	StorageEngine* pStorageEngine = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pStorageEngine = StorageEngine::getStorageEngine();

		TransactionId transId = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		if (bIsMultiCol)
		{
			//entrySetId = pStorageEngine->createEntrySet(pTrans,GetMultiCollInfo());
		}
		else
		{
			uint32 colid = MyGetColId();
			MySetColInfo_Heap(colid);
			entrySetId = pStorageEngine->createEntrySet(pTrans,colid);
		}

		if (bWithIndex)
		{
			EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,entrySetId);
			if (pEntrySet)
			{
				if (bIsMultiCol)
				{
					//pStorageEngine->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,GetMultiCollInfo());
				}
				else
				{
					uint32 colid = MyGetColId();
					MySetColInfo_Index(colid);
					pStorageEngine->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,colid);
					colId = colid;
				}
			}
		}

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
	}

	if (InvalidTransactionID != entrySetId)
		m_vEntrySetID.push_back(entrySetId);

	return entrySetId;
}
void CPerformBase::SetData(std::vector<std::string> &vData, uint32 count)
{
	const uint32 DATA_KIND_COUNT = 20; 

	char data[DATA_KIND_COUNT][15] = 
	{
		"!sd[de5e1=",
		"F&^$52LH*)",
		"8}](%ge8;e",
		"}#gfKP]^8e",
		"!)ok8+=-_9",
		"@50?/hR%#s",
		"*/+..5IUH&",
		"+=+__(*&HG",
		"7.61OP^bTS",
		"1234567890",
		"abcdefghij",
		"ABFRJKIJKK",
		"9503264895",
		"...0001586",
		"AAHH%%$$-=",
		"aABbCcX586",
		"2.0.25871l",
		"QQ11098IJG",
		"AAAAAAAAAA",
		"xxxxxxxxxx"
	};

	uint32 unDivision = count/DATA_KIND_COUNT;
	for (uint32 unIndex = 0;unIndex < unDivision; unIndex ++)
	{
		for (uint32 i = 0; i < DATA_KIND_COUNT; i ++)
		{
			std::string strTmp = data[i];
			vData.push_back(strTmp);
		}
	}

	//补足上面没有插满的个数
	std::string strAdd = "!@#$%^&*()";
	uint32 nValue = count - vData.size();
	for (uint32 j = 0; j < nValue; j ++)
	{
		vData.push_back(strAdd);
	}
}
bool CPerformBase::Begin(std::string strTaskInfo, EntrySetID entrySetId, bool bIsWrite2File, uint32 times)
{
	if (InvalidEntrySetID == entrySetId)
		return false;

	StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
	Transaction* pTrans = NULL;

	try
	{
		TransactionId transId = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		//计时开始
		boost::timer t;
		for (uint32 i = 0; i < times; i ++)
		{
			Execute(pTrans,pEntrySet);
		}
		//计时结束
		double elapse_time = t.elapsed();

		if (bIsWrite2File)
		{
			std::string strFlag = strTaskInfo;
			WriteStatInfo2File_Ex(strFlag,elapse_time);
		}

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}
	catch(std::exception &ex)
	{
		std::cout<<ex.what()<<std::endl;
		pTrans->abort();
		return false;
	}

	return true;
}
std::string CPerformBase::GetTaskInfo(std::string strTaskInfo, uint32 TupleCount)
{
	char chInfo[150];
	memset(chInfo,0,sizeof(chInfo));
	sprintf(chInfo,strTaskInfo.c_str(),TupleCount);

	std::string strRet = chInfo;

	return strRet;
}
bool CPerformBase::ClearTask()
{
	Transaction* pTrans = NULL;
	StorageEngine* pStorageEngine = NULL;
	try
	{
		pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		std::vector<EntrySetID>::iterator it;
		for (it = m_vEntrySetID.begin(); it != m_vEntrySetID.end(); it ++)
		{
			EntrySetID eid = *it;
			pStorageEngine->removeEntrySet(pTrans,eid);
		}

		pTrans->commit();
	}
	catch(StorageEngineException &ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();

		return false;
	}

	return true;
}
void CPerformBase::WriteStatInfo2File(std::string strTaskInfo, double taskSeconds)
{
	//记录当前时间，可以换用C++类的方式，比如boost的
	time_t t;
	time(&t);
	struct tm* pTime = localtime(&t);
	char chTime[20];
	memset(chTime,0,sizeof(chTime));
	strftime(chTime,sizeof(chTime),"%Y-%m-%d %H:%M:%S",pTime);
	std::string strTime = chTime;
	strTime += "\n";

	//格式化消耗时间为字符串
	char chSecond[50];
	memset(chSecond,0,sizeof(chSecond));
	sprintf(chSecond,"excute time : %f seconds\n",taskSeconds);
	std::string strExTime = chSecond;

	//把当前时间和执行任务、消耗时间写入文件
	const std::string strFile = "D:/perform_statistic/branch_test";

	strTaskInfo += "\n";
	std::string strWrite = strTime + strTaskInfo + strExTime + "\n *******-------********----------*********\n";
	std::fstream fwrite(strFile.c_str(),std::ios_base::app | std::ios_base::in | std::ios_base::out);

	if (fwrite.is_open())
	{
		fwrite<<strWrite<<std::endl;
	}
}
void CPerformBase::WriteStatInfo2File_Ex(std::string strFlag, double taskSeconds)
{
	std::string strSeconds = boost::str(boost::format("%f") % taskSeconds);
	std::string strWrite = strFlag + " " + strSeconds;

	//file path
	const std::string strFile = "../trunk_test";
	std::fstream myfile(strFile.c_str(),std::ios_base::in | std::ios_base::out | std::ios_base::app);
	if (!myfile.is_open())
		assert(false);

	myfile<<strWrite<<std::endl;
}
void CPerformBase::InsertData(Transaction* pTrans, EntrySet* pEntrySet, const uint32& count)
{
	uint32 flag = 0;
	TestData data;
	data.tuple_len = GetTupleLen();
	data.tuple_count = count;
	GetTestDataPtr(data);
	assert(data.pData != NULL);

	char* pData = (char*)data.pData;
	for (uint32 i = 0; i < count; i ++)
	{
		DataItem item;
		item.setData((void*)pData);
		item.setSize(data.tuple_len);
		EntryID eid;
		pEntrySet->insertEntry(pTrans,eid,item);

		pData += data.tuple_len;
		flag ++;
	}

	assert(flag == count);
}
void CPerformBase::SetDelData(const std::vector<std::string>& vData)
{
	m_vOpData = vData;
}
bool CPerformBase::IsNeedDelOrUpdate(const std::string& strData)
{
	bool bRet = false;
	if (m_vOpData.empty())
		return true;

	for (std::vector<std::string>::iterator it = m_vOpData.begin(); it != m_vOpData.end(); it ++)
	{
		if (*it == strData)
		{
			bRet = true;
			break;
		}
	}

	return bRet;
}
CPerformBase::~CPerformBase()
{

}

bool perform_drop_heap(const EntrySetID& entrySetId)
{
	Transaction* pTrans = NULL;
	StorageEngine* pStorageEngine = NULL;
	try
	{
		pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pStorageEngine->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		pStorageEngine->removeEntrySet(pTrans,entrySetId);

		pTrans->commit();
	}
	catch(StorageEngineException &ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();

		return false;
	}

	return true;
}