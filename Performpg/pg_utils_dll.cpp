#include <iostream>
#include <string>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <cstdarg>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/timer.hpp>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "pg_utils_dll.h"
//#include "sequence/utils.h"


using std::cout;
using std::endl;
using std::vector;
using std::string;
using namespace FounderXDB::StorageEngineNS;

StorageEngine *pStorageEngine = NULL;
Transaction *pTransaction = NULL;

extern EntrySetID HEAP_ID;

extern uint32 EntryColId = 65535;
extern uint32 IndexId = 65536;
extern uint32 VarColId = 65537;
void commit_transaction()
{
	try
	{
		pTransaction->commit();//提交事务
	}
	catch(StorageEngineException &ex)
	{
		pTransaction->abort();
		cout << ex.getErrorMsg() << endl;
		cout << ex.getErrorNo() << endl;
	}
}

void user_abort_transaction()
{
	pTransaction->abort();//取消事务
	delete pTransaction;
	pTransaction = NULL;
} 

void get_new_transaction()
{	
	FXTransactionId invalid_transaction = ((TransactionId) 0);
	pTransaction = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);//获得一个事务
}

Transaction* start_new_transaction(Transaction::IsolationLevel   level)
{
	FXTransactionId invalid_transaction = ((TransactionId) 0);
	Transaction* pTransaction = pStorageEngine->getTransaction(invalid_transaction, level);//获得一个事务
	return pTransaction;
}

void command_counter_increment()
{
	pStorageEngine->endStatement();
}


MyColumnInfo::MyColumnInfo(const std::map<int,CompareCallbacki>& mapCompare,Spliti split)
{
	m_columnInfo.keys = mapCompare.size();
	m_columnInfo.col_number = new size_t[m_columnInfo.keys];
	m_columnInfo.rd_comfunction = new CompareCallbacki[m_columnInfo.keys];
	m_columnInfo.split_function = split;
	int idx = 0;
	BOOST_FOREACH(BOOST_TYPEOF(*mapCompare.begin()) comp,mapCompare)
	{
		m_columnInfo.col_number[idx] = comp.first;
		m_columnInfo.rd_comfunction[idx++] = comp.second;
	}
}

MyColumnInfo::~MyColumnInfo()
{
	delete []m_columnInfo.col_number;
	m_columnInfo.col_number = NULL;
	delete []m_columnInfo.rd_comfunction;
	m_columnInfo.rd_comfunction = NULL;
}


std::vector<int> FixSpliter::g_vecOfSplitPos;
FixSpliter::FixSpliter(const std::vector<int> &vec)
{
	g_vecOfSplitPos.clear();
	for (std::vector<int>::const_iterator it = vec.begin();
		it != vec.end();
		++it)
	{
		g_vecOfSplitPos.push_back(*it + (g_vecOfSplitPos.size() > 0 ? g_vecOfSplitPos.back() : 0));
	}
}

void FixSpliter::split(RangeDatai& rangeData, const char *pszNeedSplit,int iIndexOfColumn, size_t len)
{
	pszNeedSplit;
	assert(iIndexOfColumn >= 1 && iIndexOfColumn <= (int)g_vecOfSplitPos.size());

	if (iIndexOfColumn > 1)
	{
		rangeData.start = g_vecOfSplitPos[iIndexOfColumn - 2];
		rangeData.len = g_vecOfSplitPos[iIndexOfColumn - 1] -  g_vecOfSplitPos[iIndexOfColumn - 2];
	}
	else
	{
		rangeData.len = g_vecOfSplitPos[iIndexOfColumn - 1];
	}
}

uint32 GetSingleColInfo()
{

	static uint32 colid = 0;

	if (0 == colid)
	{
		colid = 1241234;
		using namespace boost::assign;
		std::map<int,CompareCallbacki> mapComp;
		insert(mapComp)(1,str_compare);

		std::vector<int> v;
		v.push_back(1);
		FixSpliter spliter(v);
		static MyColumnInfo columnInfo(mapComp,FixSpliter::split);

		setColumnInfo(colid,&columnInfo.Get());

	}

	return EntrySetCollInfo<4>::get();
}

uint32 GetMultiCollInfo()
{
	static uint32 colid = 0;

	if (0 == colid)
	{

		colid = 32589;
		using namespace boost::assign;
		std::map<int,CompareCallbacki> mapComp;
		insert(mapComp)(1,str_compare)(2,str_compare)(3,str_compare);

		std::vector<int> v;
		v += 3,2,1;
		FixSpliter spliter(v);
		static MyColumnInfo columnInfo(mapComp,FixSpliter::split);

		setColumnInfo(colid,&columnInfo.Get());
	}

	return EntrySetCollInfo<3,2,1>::get();
}

void initKeyVector(vector<ScanCondition> &keyVec,int col_number, ScanCondition::CompareOperation cmp_op,const char *keydata,int (*compare_func)(const char *, size_t , const char *, size_t))
{
	int key_len = strlen(keydata); 
	char *tKeydata = (char*)malloc(key_len+1);
	memcpy(tKeydata,keydata,key_len+1);
	ScanCondition key(col_number, cmp_op, (se_uint64)tKeydata, key_len, compare_func);
	keyVec.push_back(key);
}

int compare_str(const char *str1, size_t len1, const char *str2, size_t len2)
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

bool readfromfile = true;
void RandomGenString(std::string& strLine,size_t nLen)
{
	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::ifstream inFile(szFileName);
		if (inFile.is_open())
		{
			inFile.seekg(0,std::ios::end);
			size_t length = inFile.tellg();
			inFile.seekg(0,std::ios::beg);
			boost::shared_ptr<char> psz(new char[length]);
			inFile.read(psz.get(),length);
			strLine.append(psz.get(),length);
			return;
		}
	}

	std::string s;
	srand((unsigned)time( NULL ));
	size_t nGenerated = 0;
	for (nGenerated = 0;nGenerated < 10;++nGenerated)
	{
		char c = 0;
		s += c;
	}
	for (nGenerated = 10; nGenerated < nLen - 1; ++nGenerated)
	{
		char c = (rand()) % 26;
		s += 'a' + c;
	}
	char c = 0;
	s += c;
	strLine.append(s);

	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::fstream outFile(szFileName,std::ios_base::out);
		if(outFile.is_open())
		{
			outFile<<s;
		}
	}

}


SearchCondition::SearchCondition():
m_nCount(0)
{

}

SearchCondition::~SearchCondition()
{

}

void SearchCondition::Add(int nColumn,EScanOp op,const char* pszValue,int (*compare)(const char *, size_t , const char *, size_t))
{
	m_vConditions.push_back(ScanCondition(nColumn,GetOption(op),(se_uint64)pszValue,strlen(pszValue),compare));
	++m_nCount;
}

void SearchCondition::Add(int nColumn,EScanOp op,const string& str,int (*compare)(const char *, size_t , const char *, size_t))
{
	m_vConditions.push_back(ScanCondition(nColumn,GetOption(op),(se_uint64)str.c_str(),str.length(),compare));
	++m_nCount;
}
ScanCondition::CompareOperation SearchCondition::GetOption(EScanOp op)
{
	switch (op)
	{
	case LessThan:
		return ScanCondition::LessThan;
	case LessEqual:
		return ScanCondition::LessEqual;
	case Equal:
		return ScanCondition::Equal;
	case GreaterEqual:
		return ScanCondition::GreaterEqual;
	case GreaterThan:
		return ScanCondition::GreaterThan;
	default:
		return ScanCondition::InvalidOperation;
	}
}

void thread_heap_create(int *status)
{
	try {
		pStorageEngine->beginThread();
		pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		HEAP_ID = pStorageEngine->createEntrySet(NULL,EntrySetCollInfo<1>::get());

		pTransaction->commit();
		} 
		catch (StorageEngineException &ex)
		{
			cout << ex.getErrorMsg() << endl;
			if (pTransaction != NULL) 
			{
				pTransaction->abort();
			}
			*status = 0; 
		}

	*status = 1;

	pStorageEngine->endThread(); 

}

void thread_heap_insert(EntrySetID heap_id , int data_amount , int data_size , int *status , vector<EntryID>& data_eid)
{
	try {
			pStorageEngine->beginThread();
			pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			heap_insert(heap_id , data_amount , data_size , data_eid);
			pTransaction->commit();
		} 
	catch (StorageEngineException &ex)
	{
		cout << ex.getErrorMsg() << endl;
		if (pTransaction != NULL)
		{
			pTransaction->abort();
		}
		*status = 0;
	}
	*status = 1;
	pStorageEngine->endThread();  
}

bool checkeid(EntryID eid)
{
	if(eid.bi_hi == 0 && eid.bi_lo == 0 && eid.ip_posid == 0){
		return false;
	}
	return true;
}
boost::mutex mut____;
bool heap_insert(EntrySetID heapId , int data_amount , int data_size , vector<EntryID>& data_eid)
{
	EntrySet *heap_entry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heapId));

	EntryID eid = {0};

	try {
			for (int i = 0; i < data_amount; i++)
			{
				std::string temp_data;
				RandomGenString(temp_data,1<<data_size);

				DataItem data;
				int len = temp_data.length();		
				char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
				memcpy(pstr, temp_data.c_str(), len);//构建DataItem 
				data.setData((void *)pstr);//pstr中的字符及参数传给data
				data.setSize(len);

				heap_entry->insertEntry(pTransaction, eid, data);
				{
					boost::unique_lock<boost::mutex> lock(mut____);

					data_eid.push_back(eid);
				}
			}
		} 
		catch (StorageEngineException &ex) 
		{
			cout << ex.getErrorMsg() << endl;
			pStorageEngine->closeEntrySet(pTransaction, heap_entry);
			return false;
		}

		pStorageEngine->closeEntrySet(pTransaction, heap_entry);

	if(checkeid(eid))
	{
		return true;
	}
	return false;

}

void thread_heap_remove(EntrySetID heapId, int *status)
{
	try {
		pStorageEngine->beginThread();
		pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pStorageEngine->removeEntrySet(pTransaction, heapId);

		pTransaction->commit();
		} 
		catch (StorageEngineException &ex)
		{
			cout << ex.getErrorMsg() << endl;
			if (pTransaction != NULL) 
			{
				pTransaction->abort();
			}
			*status = 0; 
		}

		*status = 1;
		pStorageEngine->endThread(); 

}

void thread_heap_delete(int *status , int thread_id ,  vector<vector<EntryID>>&data_eid)
{
	try
	{
		pStorageEngine->beginThread();
		pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		DataItem getdata;

		int times = data_eid[0].size();
		for (int i=0 ; i<times ; i++)
		{
			penry_set->deleteEntry(pTransaction,data_eid[thread_id][i]);//删除元组
		}

		penry_set->endEntrySetScan(entry_scan);
		pTransaction->commit();
	
	}
	catch (StorageEngineException &ex)
	{
		cout << ex.getErrorMsg() << endl;
		if (pTransaction != NULL) 
		{
			pTransaction->abort();
		}
		*status = 0; 
}

*status = 1;
pStorageEngine->endThread(); 
}

#define TESTDATA_NEW "test_new"
void thread_heap_update(int *status , int thread_id ,  vector<vector<EntryID>>&data_eid)
{
	try
	{
		pStorageEngine->beginThread();
		pTransaction = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet *penry_set = NULL;
		penry_set=static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID));
		EntrySetScan *entry_scan = penry_set->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描

		DataItem getdata;
		DataItem newdata;
		int newlen = sizeof(TESTDATA_NEW);		
		char *newpstr = (char *)malloc(newlen);//记得释放空间...free(pstr)
		memcpy(newpstr, TESTDATA_NEW, newlen);//构建DataItem
		newdata.setData((void *)newpstr);//pstr中的字符及参数传给data
		newdata.setSize(newlen);
		int times = data_eid[0].size();
		for (int i=0 ; i<times ; i++)
		{
			penry_set->updateEntry(pTransaction , data_eid[thread_id][i] , newdata);//删除元组
		}

		penry_set->endEntrySetScan(entry_scan);
		pTransaction->commit();

	}
	catch (StorageEngineException &ex)
	{
		cout << ex.getErrorMsg() << endl;
		if (pTransaction != NULL) 
		{
			pTransaction->abort();
		}
		*status = 0; 
	}

	*status = 1;
	pStorageEngine->endThread(); 
}
//void thread_heap_scan()

bool check_status(int status)
{
	if (status == 0)
	{
		cout<<"operation error!"<<endl;
	}
	return true;
}