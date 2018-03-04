#ifndef PERFORM_UTILS_H
#define PERFORM_UTILS_H
#include "Configs.h"
#include <stdlib.h>
#include <iostream>
#include <vector>
#include "PGSETypes.h"
#include "Transaction.h"
#include "EntrySet.h"

using namespace FounderXDB::StorageEngineNS;

struct TestData 
{
	TestData()
	{
		tuple_count = 0;
		tuple_len = 0;
		pData = NULL;
	}

	void Clear_Mem()
	{
		if (pData)
		{
			free(pData);
			pData = NULL;
		}
	}
	uint32 tuple_count;//tuple count in mem pData
	uint32 tuple_len;//single tuple len in mem pData
	char* pData;
	~TestData()
	{
		Clear_Mem();		
	}
};
class CPerformBase
{
public:
	CPerformBase();

	//EntrySetID Create_EntrySet(bool bIsMultiCol, bool bWithIndex = false);
	EntrySetID Create_EntrySet(bool bIsMultiCol, bool bWithIndex,uint32& colId);
	void SetData(std::vector<std::string> &vData, uint32 count);

	void WriteStatInfo2File(std::string strTaskInfo, double taskSeconds);

	void WriteStatInfo2File_Ex(std::string strFlag, double taskSeconds);

	//bool Begin(EntrySetID entrySetId);
	bool Begin(std::string strTaskInfo, EntrySetID entrySetId, bool bIsWrite2File, uint32 times = 1);

	bool ClearTask();

	std::string GetTaskInfo(std::string strTaskInfo, uint32 TupleCount);

	void InsertData(Transaction* pTrans, EntrySet* pEntrySet, const uint32& count);

	void SetDelData(const std::vector<std::string>& vData);

	virtual void Execute(Transaction* pTrans, EntrySet* pEntrySet) {};

	virtual ~CPerformBase();
	static uint32 MyGetColId();
protected:
	std::vector<std::string> m_vOpData;//用于多线程并发删除、更新时，每个线程分配一个数据
	bool IsNeedDelOrUpdate(const std::string& strData);

private:
	EntrySetID m_EntrySetID;
	std::vector<EntrySetID> m_vEntrySetID;
};

int str_compare(const char *str1, size_t len1, const char *str2, size_t len2);
int start_engine_();
int stop_engine_();
storage_params *get_param( void );
storage_params* GetStorageParam(std::string& strConfigFile);
void GetTestDataPtr(TestData& data);
std::string GenerateRandomString(uint32 len);
uint32 GetDataCount();
uint32 GetTupleLen();
void perform_split_1_col(RangeDatai& range, const char* pSrc, int colnum, size_t len);
void MySetColInfo_Heap(const uint32& colId);
void MySetColInfo_Index(const uint32& colId);
bool perform_drop_heap(const EntrySetID& entrySetId);
#endif
