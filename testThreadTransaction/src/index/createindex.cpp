#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include "StorageEngine.h"
#include "test_utils/test_utils.h"
#include "TestFrame/TestFrame.h"
#include "TestFrameCommon/TestFrameCommon.h"

using namespace std;
using namespace FounderXDB::StorageEngineNS;

extern StorageEngine *pStorageEngine;

void my_spilt(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.len = 0;
	rangeData.start = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 1;
	}
}

int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2)
{
	int i = 0;
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

void my_spilt_heap_1(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.len = 0;
	rangeData.start = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 1;
	}
}

void my_split_heap_321(RangeDatai& rangeData, const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 1;
	}
}

void my_split_heap_32(RangeDatai& rangeData, const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
}

void my_split_index_31(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 1;
	}
}

void my_split_index_34(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 4;
	}
}

void my_split_index_24(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 4;
	}
}

bool testIndexUpdate_SingleColumn_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt_heap_1;
		uint32 colid = 2000;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID= pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		//插入一些数据
		std::vector<std::string> vInsertData;
		for(char start= 'A'; start< 'z'; start++)
		{
			string str;
			str.clear();
			str = start;
			vInsertData.push_back(str);
		}
		vInsertData.push_back("3");
		vInsertData.push_back("4");
		vInsertData.push_back("2");
		vInsertData.push_back("6");
		vInsertData.push_back("9");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);

		//创建index并插入一些其他的数据
		ColumnInfo *co= (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 1;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->split_function = my_spilt_heap_1;
		uint32 co_lid = 2001;
		setColumnInfo(co_lid, co);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,co_lid);
		IndexEntrySet *pIndex= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		vInsertData.clear();
		vInsertData.push_back("1");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndexUpdate_SingleColumn_Update(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		//update data
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		std::map<EntryID,std::string> mapUpdateData;
		mapUpdateData.insert(std::pair<EntryID,std::string>(paraIndex->m_vEntryID[2],"8"));
		mapUpdateData.insert(std::pair<EntryID,std::string>(paraIndex->m_vEntryID[4],"0"));
		UpdateData(paraIndex->GetTransaction(),pEntrySet,mapUpdateData);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessEqual,"6",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,Trans,pIndexSet,scanCondition.Keys());

		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired.insert("0");
		sDesired.insert("1");
		sDesired.insert("2");
		sDesired.insert("3");
		sDesired.insert("4");
		sDesired.insert("6");
		assert(sResult == sDesired);
		pStorageEngine->closeEntrySet(Trans,pIndexSet);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndexUpdate_SingleColumn_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(testIndexUpdate_SingleColumn_Insert,pPara);
	REGTASK(testIndexUpdate_SingleColumn_Update,pPara);
	return true;
}

bool testIndexUpdate_MultiColumn_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_split_heap_321;
		uint32 colid = 2002;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);

		//插入一些数据
		std::vector<std::string> vInsertData;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			vInsertData.push_back(str);
		}
		vInsertData.push_back("123456");
		vInsertData.push_back("123789");
		vInsertData.push_back("012562");
		vInsertData.push_back("012120");
		vInsertData.push_back("012456");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);

		//创建index并插入一些其他的数据
		ColumnInfo *co= (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 3;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->col_number[1] = 2;
		co->col_number[2] = 3;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->rd_comfunction[2] = my_compare_str;
		co->split_function = my_split_heap_321;
		uint32 co_lid = 2003;
		setColumnInfo(co_lid, co);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,co_lid);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
			pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		vInsertData.clear();
		vInsertData.push_back("012506");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndexUpdate_MultiColumn_Update(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		//update data
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		std::map<EntryID,std::string> mapUpdateData;
		mapUpdateData.insert(std::pair<EntryID,std::string>(paraIndex->m_vEntryID[5],"124889"));
		mapUpdateData.insert(std::pair<EntryID,std::string>(paraIndex->m_vEntryID[0],"121671"));
		UpdateData(Trans,pEntrySet,mapUpdateData);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessThan,"123",str_compare);
		scanCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,Trans,pIndexSet,scanCondition.Keys());

		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired.insert("012506");
		sDesired.insert("012562");
		sDesired.insert("121671");
		assert(543 == sResult.size());
		pStorageEngine->closeEntrySet(Trans,pIndexSet);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool testIndexUpdate_MultiColumn_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(testIndexUpdate_MultiColumn_Insert,pPara);
	REGTASK(testIndexUpdate_MultiColumn_Update,pPara);
	return true;
}

bool testIndexUpdate_InAnotherTrans_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt;
		uint32 colid = 2004;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		//插入一些数据
		std::vector<std::string> vInsertData;
		for (int digstr=122400; digstr<124000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			vInsertData.push_back(str);
		}
		vInsertData.push_back("123456");
		vInsertData.push_back("123789");
		vInsertData.push_back("012562");
		vInsertData.push_back("012120");
		vInsertData.push_back("012456");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);

		//创建index并插入一些其他的数据
		uint32 col_id = 2005;
		ColumnInfo *co = (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 3;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->col_number[1] = 2;
		co->col_number[2] = 3;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->rd_comfunction[2] = my_compare_str;
		co->split_function =  my_spilt;
		setColumnInfo(col_id, co);
		
		paraIndex->m_nIndexEntryID= pStorageEngine->createIndexEntrySet(Trans,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,col_id);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
			pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		vInsertData.clear();
		vInsertData.push_back("012506");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndexUpdate_InAnotherTrans_Update(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		//update data
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		std::map<EntryID,std::string> mapUpdateData;
		mapUpdateData.insert(std::pair<EntryID,std::string>(paraIndex->m_vEntryID[5],"124889"));
		mapUpdateData.insert(std::pair<EntryID,std::string>(paraIndex->m_vEntryID[0],"121671"));
		UpdateData(Trans,pEntrySet,mapUpdateData);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessThan,"123",str_compare);
		scanCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,Trans,pIndexSet,scanCondition.Keys());

		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired.insert("012506");
		sDesired.insert("012562");
		sDesired.insert("121671");
		assert(543 == sResult.size());
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndexUpdate_InAnotherTrans_Select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							 paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		
		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessThan,"123",str_compare);
		scanCondition.Add(2,GreaterThan,"45",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,Trans,pIndexSet,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired.insert("012506");
		sDesired.insert("012562");
		sDesired.insert("121671");
		assert(543 == sResult.size());

		pStorageEngine->closeEntrySet(Trans,pIndexSet);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndexUpdate_InAnotherTrans_DLL_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(testIndexUpdate_InAnotherTrans_Insert,pPara);
	REGTASK(testIndexUpdate_InAnotherTrans_Update,pPara);
	REGTASK(testIndexUpdate_InAnotherTrans_Select,pPara);
	return true;
}

bool testIndexInsert_SingleColumn_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt_heap_1;
		uint32 colid = 2006;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
					paraIndex->m_nHeapEntryID);

		//插入一些数据
		std::vector<std::string> vInsertData;
		for(char start= 'A'; start< 'z'; start++)
		{
			string str;
			str.clear();
			str = start;
			vInsertData.push_back(str);
		}
		vInsertData.push_back("1");
		vInsertData.push_back("2");
		vInsertData.push_back("3");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);

		//创建index并插入一些其他的数据
		ColumnInfo *co= (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 1;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->split_function = my_spilt_heap_1;
		uint32 co_lid = 2007;
		setColumnInfo(co_lid, co);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,co_lid);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
			pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		vInsertData.clear();
		vInsertData.push_back("5");
		vInsertData.push_back("6");
		vInsertData.push_back("7");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndexInsert_SingleColumn_Select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessEqual,"7",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,Trans,pIndexSet,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired.insert("1");
		sDesired.insert("2");
		sDesired.insert("3");
		sDesired.insert("5");
		sDesired.insert("6");
		sDesired.insert("7");
		assert(sResult == sDesired);

		pStorageEngine->closeEntrySet(Trans,pIndexSet);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndexInsert_SingleColumn_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(testIndexInsert_SingleColumn_Insert,pPara);
	REGTASK(testIndexInsert_SingleColumn_Select,pPara);
	return true;
}

bool testIndexInsert_SameScaKey_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt_heap_1;
		uint32 colid = 2008;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
				paraIndex->m_nHeapEntryID);

		//插入一些数据
		std::vector<std::string> vInsertData;
		for(char start= 'A'; start< 'z'; start++)
		{
			string str;
			str.clear();
			str = start;
			vInsertData.push_back(str);
		}
		vInsertData.push_back("1");
		vInsertData.push_back("2");
		vInsertData.push_back("3");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);

		//创建index并插入一些其他的数据
		ColumnInfo *co= (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 1;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->split_function = my_spilt_heap_1;
		uint32 co_lid = 2009;;
		setColumnInfo(co_lid, co);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,co_lid);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
			pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		vInsertData.clear();
		vInsertData.push_back("5");
		vInsertData.push_back("6");
		vInsertData.push_back("7");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndexInsert_SameScaKey_Select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessEqual,"7",str_compare);
		scanCondition.Add(1,LessEqual,"8",str_compare);
		
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,Trans,pIndexSet,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired.insert("1");
		sDesired.insert("2");
		sDesired.insert("3");
		sDesired.insert("5");
		sDesired.insert("6");
		sDesired.insert("7");
		assert(sResult == sDesired);

		pStorageEngine->closeEntrySet(Trans,pIndexSet);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool testIndexInsert_SameScaKey_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(testIndexInsert_SameScaKey_Insert,pPara);
	REGTASK(testIndexInsert_SameScaKey_Select,pPara);
	return true;

}

bool testIndex_InAnotherTrans_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt_heap_1;
		uint32 colid = 2010;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		//插入一些数据
		std::vector<std::string> vInsertData;
		for(char start= ' '; start< 'z'; start++)
		{
			string str;
			str.clear();
			str = start;
			vInsertData.push_back(str);
		}
		vInsertData.push_back("1");
		vInsertData.push_back("2");
		vInsertData.push_back("3");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);

		//创建index并插入一些其他的数据
		ColumnInfo *co= (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 1;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->split_function = my_spilt_heap_1;
		uint32 co_lid = 2011;
		setColumnInfo(co_lid, co);
		paraIndex->m_nIndexEntryID= pStorageEngine->createIndexEntrySet(Trans,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,co_lid);
		IndexEntrySet * pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
			pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		vInsertData.clear();
		vInsertData.push_back("5");
		vInsertData.push_back("6");
		vInsertData.push_back("7");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndex_InAnotherTrans_Select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
							pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,LessEqual,"7",str_compare);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,Trans,pIndexSet,scanCondition.Keys());


		//检查结果
		std::set<std::string> sResult(vResult.begin(),vResult.end());
		std::set<std::string> sDesired;
		sDesired.insert("1");
		sDesired.insert("2");
		sDesired.insert("3");
		sDesired.insert("5");
		sDesired.insert("6");
		sDesired.insert("7");
		assert(24 == sResult.size());

		pStorageEngine->closeEntrySet(Trans,pIndexSet);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndex_InAnotherTrans_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(testIndex_InAnotherTrans_Insert,pPara);
	REGTASK(testIndex_InAnotherTrans_Select,pPara);
	return true;
}

bool testIndex_Sort_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_split_heap_321;
		uint32 colid = 2012;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		//插入一些数据
		std::vector<std::string> vInsertData;
		for (int digstr=122400; digstr<124000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream>>digstr;
			stream<<str;
			vInsertData.push_back(str);
		}
		vInsertData.push_back("123456");
		vInsertData.push_back("123789");
		vInsertData.push_back("012562");
		vInsertData.push_back("012120");
		vInsertData.push_back("012456");
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);

		//创建index并插入一些其他的数据
		ColumnInfo *co= (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 2;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->col_number[1] = 2;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->split_function = my_split_heap_32;
		uint32 co_lid = 2013;
		setColumnInfo(co_lid, co);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,co_lid);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
			pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndex_Sort_Sort(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		//构造一个查询
		SearchCondition scanCondition;
		scanCondition.Add(1,Equal,"012",my_compare_str);

		//检查结果是否是按照第二关健字排序
		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,Trans,pIndexSet,scanCondition.Keys());

		std::vector<std::string> vDesired;
		vDesired.push_back("012120");
		vDesired.push_back("012456");
		vDesired.push_back("012562");
		assert(CompareVector(vResult, vDesired));

		ostream_iterator<std::string> out(std::cout," ");
		std::copy(vResult.begin(),vResult.end(),out);
		std::cout<<endl;

		std::copy(vDesired.begin(),vDesired.end(),out);
		std::cout<<endl;

		pStorageEngine->closeEntrySet(Trans,pIndexSet);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool testIndex_Sort_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(testIndex_Sort_Insert,pPara);
	REGTASK(testIndex_Sort_Sort,pPara);
	return true;
}

//改写test_index_dll.cpp中的函数，进行测试
typedef int (*PToCompareFunc)(const char*, size_t, const char*, size_t);
void my_split_index_23541(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 1;
	}
	if (col == 3)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 4)
	{
		rangeData.start = 5;
		rangeData.len = 3;
	}
	if (col == 5)
	{
		rangeData.start = 8;
		rangeData.len = 3;
	}
}

void my_split_index_Q32(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	else
	{
		rangeData.start = col-1;
		rangeData.len = 1;
	}
}

void my_split_index_23(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 3;
	}
}

void my_split_index_21(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 1;
	}
}

void my_split_heap_321321(RangeDatai& rangeData, const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 1;
	}
	if (col == 4)
	{
		rangeData.start = 6;
		rangeData.len = 3;
	}
	if (col == 5)
	{
		rangeData.start = 9;
		rangeData.len = 2;
	}
	if (col == 6)
	{
		rangeData.start = 11;
		rangeData.len = 1;
	}
}

void my_split_heap_Q32(RangeDatai& rangeData, const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	else
	{
		rangeData.start = col-1;
		rangeData.len = 1;
	}
}


//init scan condition
void my_scankey_init(const char datum_data[][20], 
										 const ScanCondition::CompareOperation co[],
										 const int fieldno[],
										 PToCompareFunc compare_func[],
										 const int col_count,
										 std::vector<ScanCondition> &vc)
{
	ScanCondition sc;
	for(int i = 0; i < col_count; ++i)
	{
		sc.compare_func = compare_func[i];
		sc.fieldno = fieldno[i];
		sc.argument = (se_uint64)datum_data[i];
		sc.arg_length = strlen(datum_data[i]);
		sc.compop = co[i];
		vc.push_back
			(
				ScanCondition
				(
				fieldno[i], 
				co[i], 
				(se_uint64)datum_data[i], 
				strlen(datum_data[i]), 
				compare_func[i]
				)
			);
	}
}

bool test_indexscan_dll_000_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt;
		uint32 colid = 2014;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
			EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		char dataarr[20][20] = {"012562", "012120", "012456"};
		DataItem di;
		EntryID eid;
		char data[20];
		//insert data into pEntrySet
		for(int i = 0; i < 3; i++)
		{
			di.setData(&dataarr[i]);
			di.setSize(strlen(dataarr[i])+1);
			pEntrySet->insertEntry(Trans,eid,di);
		}
		for (int digstr=123000; digstr<124999; digstr++)
		{
			sprintf(data,"%d",digstr);
			di.setData(&data);
			di.setSize(strlen(data)+1);
			pEntrySet->insertEntry(Trans,eid,di);
		}
		command_counter_increment();
		
		uint32 col_id = 2015;
		ColumnInfo *co = (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 2;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->col_number[1] = 2;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->split_function =  my_split_heap_32;
		setColumnInfo(col_id, co);
		
		//create a index on testRelation
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,
						pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,col_id);
		IndexEntrySet *pIndexSet= static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(
							Trans,
							pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nIndexEntryID));
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_000_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		//begin index scan
		char datum_data[20][20] = {"123", "45"};
		int fieldno[2] = {1, 2};
		ScanCondition::CompareOperation co[2] = 
		{
			ScanCondition::LessThan,
			ScanCondition::GreaterThan
		};
		PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,co,fieldno,compare_func,2,vc);
		EntrySetScan *iess = pIndexSet->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,vc);
		DataItem di;
		EntryID eid;
		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));
		char *temp;
		int flag = 65536;
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			temp = (char*)di.getData();
			flag = strcmp(temp, "012562");
		}
		pIndexSet->endEntrySetScan(iess);
		assert(flag == 0);
		if(flag != 0)
		{			
			pStorageEngine->closeEntrySet(Trans,pIndexSet);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet->getId());
			pStorageEngine->removeEntrySet(Trans,pEntrySet->getId());//删除heap
			paraIndex->SetSuccessFlag(true);
			return false;
		}
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexSet->getId();
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, entid, indxid);
		pStorageEngine->removeEntrySet(Trans,entid);//删除heap
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_000_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_indexscan_dll_000_Insert,pPara);
	REGTASK(test_indexscan_dll_000_Scan,pPara);
	return true;
}

bool test_indexscan_dll_001_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt;
		uint32 colid = 2016;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
			EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		DataItem di;
		EntryID eid;
		char data[20];
		//insert data into pEntrySet
		for (int digstr=123000; digstr<123999; digstr++)
		{
			sprintf(data,"%d",digstr);
			di.setData(&data);
			di.setSize(strlen(data)+1);
			pEntrySet->insertEntry(Trans,eid,di);
		}
		command_counter_increment();

		uint32 col_id = 2017;
		ColumnInfo *co = (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 2;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 2;
		co->col_number[1] = 3;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->split_function =  my_split_index_21;
		setColumnInfo(col_id, co);
		
		//create a index on testRelation
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,pEntrySet,
													BTREE_INDEX_ENTRY_SET_TYPE,												
													col_id);
		IndexEntrySet *pIndexSet = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(
												Trans,
												pEntrySet,
												EntrySet::OPEN_EXCLUSIVE,
												paraIndex->m_nIndexEntryID));
		command_counter_increment();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_001_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		//begin index scan
		char datum_data[20][20] = {"45", "2"};
		int fieldno[2] = {1, 2};
		ScanCondition::CompareOperation co[2] = 
		{
			ScanCondition::LessThan,
			ScanCondition::LessThan
		};
		PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,co,fieldno,compare_func,2,vc);
		EntrySetScan *iess = pIndexSet->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,vc);
		DataItem di;
		EntryID eid;
		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));
		char *temp;
		int flag = 65536;
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			temp = (char*)di.getData();
			flag = strcmp(temp, "123441");
		}
		pIndexSet->endEntrySetScan(iess);
		assert(flag == 0);
		if(flag != 0)
		{			
			pStorageEngine->closeEntrySet(Trans,pIndexSet);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet->getId());
			pStorageEngine->removeEntrySet(Trans,pEntrySet->getId());//删除heap
			paraIndex->SetSuccessFlag(true);
			return false;
		}
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexSet->getId();
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, entid, indxid);
		pStorageEngine->removeEntrySet(Trans,entid);//删除heap
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_001_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_indexscan_dll_001_Insert,pPara);
	REGTASK(test_indexscan_dll_001_Scan,pPara);
	return true;
}

bool test_indexscan_dll_002_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt;
		uint32 colid = 2018;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		char dataarr[20][20] = {"012562", "012120", "012456"};
		DataItem di;
		EntryID eid;
		char data[20];
		//insert data into pEntrySet
		for(int i = 0; i < 3; i++)
		{
			di.setData(&dataarr[i]);
			di.setSize(strlen(dataarr[i])+1);
			pEntrySet->insertEntry(Trans,eid,di);
		}
		for (int digstr=123000; digstr<123999; digstr++)
		{
			sprintf(data,"%d",digstr);
			di.setData(&data);
			di.setSize(strlen(data)+1);
			pEntrySet->insertEntry(Trans,eid,di);
		}
		command_counter_increment();
		
		//create a index on pEntrySet
		uint32 col_id = 2019;
		ColumnInfo *co = (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 2;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 2;
		co->col_number[1] = 1;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->split_function =  my_split_index_23;
		setColumnInfo(col_id, co);

		//create a index on testRelation
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,pEntrySet,
																				BTREE_INDEX_ENTRY_SET_TYPE,												
																				col_id);
		IndexEntrySet *pIndexSet= static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(Trans,
																			pEntrySet,
																			EntrySet::OPEN_EXCLUSIVE,
																			paraIndex->m_nIndexEntryID,NULL));
		command_counter_increment();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_002_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
			paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
			EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		//begin index scan
		char datum_data[20][20] = {"45", "123"};
		int fieldno[2] = {1, 2};
		ScanCondition::CompareOperation co[2] = 
		{
			ScanCondition::GreaterThan,
			ScanCondition::LessThan
		};
		PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,co,fieldno,compare_func,2,vc);
		EntrySetScan *iess = pIndexSet->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,vc);
		DataItem di;
		EntryID eid;
		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));
		char *temp;
		int flag = 65536;
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			temp = (char*)di.getData();
			flag = strcmp(temp, "012562");
		}
		pIndexSet->endEntrySetScan(iess);
		assert(flag == 0);
		if(flag != 0)
		{			
			pStorageEngine->closeEntrySet(Trans,pIndexSet);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet->getId());
			pStorageEngine->removeEntrySet(Trans,pEntrySet->getId());//删除heap	
			paraIndex->SetSuccessFlag(true);
			return false;
		}
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexSet->getId();
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->removeIndexEntrySet(Trans, entid, indxid);
		pStorageEngine->removeEntrySet(Trans,entid);//删除heap		
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_002_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_indexscan_dll_002_Insert,pPara);
	REGTASK(test_indexscan_dll_002_Scan,pPara);
	return true;
}

bool test_indexscan_dll_003_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_split_heap_321321;
		uint32 colid = 2020;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		char data[20][20] = {"123206jim456", "123789jim789", "012562tom562", 
					"012120tom120", "255456ann456","123456ann458","255206ann568"};
		//3 5 4 1
		//"123 20 6 jim 45 6" 
		//"123 78 9 jim 78 9"
		//"012 56 2 tom 56 2"
		//"012 12 0 tom 12 0" 
		//"255 45 6 ann 45 6"
		//"123 45 6 ann 45 8"
		//"255 20 6 ann 56 8
		DataItem di;
		EntryID eid;
		//insert data into pEntrySet
		for(int i = 0; i < 7; i++)
		{
			di.setData(&data[i]);
			di.setSize(strlen(data[i])+1);
			pEntrySet->insertEntry(Trans,eid,di);
		}
		command_counter_increment();
		
		//create a index on pEntrySet
		uint32 col_id = 2021;
		ColumnInfo *co = (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 5;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 2;
		co->col_number[1] = 3;
		co->col_number[2] = 5;
		co->col_number[3] = 4;
		co->col_number[4] = 1;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->rd_comfunction[2] = my_compare_str;
		co->rd_comfunction[3] = my_compare_str;
		co->rd_comfunction[4] = my_compare_str;
		co->split_function =  my_split_index_23541;
		setColumnInfo(col_id, co);
		
		//create a index on testRelation
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,pEntrySet,
																				BTREE_INDEX_ENTRY_SET_TYPE,												
																				col_id);
		IndexEntrySet *pIndexSet= static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(Trans,
																			pEntrySet,
																			EntrySet::OPEN_EXCLUSIVE,
																			paraIndex->m_nIndexEntryID));
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_003_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
			paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
			EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		//begin index scan
		const int nkeys = 5;
		char datum_data[20][20] = {"20", "6", "56", "ann", "255"};
		int fieldno[nkeys] = {1, 2,3,4,5};
		ScanCondition::CompareOperation co[nkeys] = 
		{
			ScanCondition::GreaterThan,
			ScanCondition::LessEqual,
			ScanCondition::LessThan,
			ScanCondition::Equal,
			ScanCondition::GreaterEqual
		};
		PToCompareFunc compare_func[nkeys] = 
		{
			my_compare_str,
			my_compare_str,
			my_compare_str,
			my_compare_str,
			my_compare_str
		};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,co,fieldno,compare_func,nkeys,vc);
		EntrySetScan *iess = pIndexSet->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,vc);
		DataItem di;
		EntryID eid;
		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));
		char *temp;
		int flag = 65536;
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			temp = (char*)di.getData();
			flag = strcmp(temp, "255456ann456");
		}
		pIndexSet->endEntrySetScan(iess);
		assert(flag == 0);
		if(flag != 0)
		{			
			pStorageEngine->closeEntrySet(Trans,pIndexSet);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet->getId());
			pStorageEngine->removeEntrySet(Trans,pEntrySet->getId());//删除heap	
			paraIndex->SetSuccessFlag(true);
			return false;
		}
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexSet->getId();
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->removeIndexEntrySet(Trans, entid, indxid);
		pStorageEngine->removeEntrySet(Trans,entid);//删除heap		
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_003_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_indexscan_dll_003_Insert,pPara);
	REGTASK(test_indexscan_dll_003_Scan,pPara);
	return true;
}

#define index_cols 32
bool test_indexscan_dll_004_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_split_heap_Q32;
		uint32 colid = 2022;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
			paraIndex->m_nHeapEntryID);

		const int rows = 10000;
		const int cols = index_cols;
		char data[rows][cols+1];
		char data_row1[cols+1];
		memset(data_row1,'a',sizeof(data_row1));
		char data_row2[cols+1];
		memset(data_row2,'b',sizeof(data_row2));
		char data_row3[cols+1];
		memset(data_row3,'c',sizeof(data_row3));

		data_row1[cols] = '\0';
		data_row2[cols] = '\0';
		data_row3[cols] = '\0';
		
		for(int i = 0;i<rows;i++)
		{
			if((i%3)==0)
			{
				memcpy(data[i],data_row1,sizeof(data_row1));
			}
			if((i%3)==1)
			{
				memcpy(data[i],data_row2,sizeof(data_row2));
			}
			if((i%3)==2)
			{
				memcpy(data[i],data_row3,sizeof(data_row3));
			}
		}

		int time_begin,time_end;
		time_begin = clock();
		DataItem di;
		EntryID eid;
		//insert data into pEntrySet
		for(int i = 0; i < rows; i++)
		{
			di.setData(&data[i]);
			di.setSize(strlen(data[i])+1);
			pEntrySet->insertEntry(Trans,eid,di);
		}
		time_end = clock();
		printf("插入%d行数据耗时%dms\n",rows,time_end - time_begin);
		command_counter_increment();
		
		//create a index on testRelation
		ColumnInfo *co= (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = index_cols;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		for(int i = 0; i < index_cols; i++)
		{
			co->col_number[i] = i+1;
		}
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		for(int i = 0; i < index_cols; i++)
		{
			co->rd_comfunction[i] = my_compare_str;
		}
		co->split_function = my_split_index_Q32;
		uint32 co_lid = 2023;
		ColumnInfo* p_co_info=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_co_info,co,sizeof(ColumnInfo));
		setColumnInfo(co_lid, p_co_info);
		//create a index on testRelation
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,
						pEntrySet,
						BTREE_INDEX_ENTRY_SET_TYPE,												
						co_lid);
		IndexEntrySet *pIndexSet= static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(
						Trans,
						pEntrySet,
						EntrySet::OPEN_EXCLUSIVE,
						paraIndex->m_nIndexEntryID));
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_004_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		//select use scankey
		//form datum
		//必须与索引列顺序保持一致
		char datum_data[index_cols][20];
		for(int i = 0; i < index_cols; i++)
		{
			for(int j = 0;j<1;j++)
			{
				datum_data[i][j] = 'b';
				datum_data[i][j+1] = '\0';		
			}
		}
		ScanCondition::CompareOperation co[index_cols];
		//index_cols个比较策略全用Equal
		for(int i = 0; i < index_cols; i++)
		{
			co[i] = ScanCondition::Equal;
		}

		int fieldno[index_cols];
		for(int i = 0; i < index_cols; i++)
		{
			fieldno[i] = i+1;
		}
		//所有的比较函数都为my_compare_str
		PToCompareFunc compare_func[index_cols];
		for(int i = 0; i < index_cols; ++i)
		{
			compare_func[i] = my_compare_str;
		}
		
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,co,fieldno,compare_func,index_cols,vc);
		
		EntrySetScan *iess = pIndexSet->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,vc);

		DataItem di;
		EntryID eid;
		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));
		char *temp;
		int flag = 65536;
		int counter = 0;
		int time_begin,time_end;
		time_begin = clock();
		const int rows = 10000;
		const int cols = index_cols;
		char data_row2[cols+1];
		memset(data_row2,'b',sizeof(data_row2));
		data_row2[cols] = '\0';
		
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			++counter;
			temp = (char*)di.getData();
			flag = strcmp(temp, data_row2); //255206ann568
		}
		time_end = clock();
		printf("索引列为32字段，查询%d行数据耗时%dms\n",rows,time_end);
		pIndexSet->endEntrySetScan(iess);
		assert(flag==0 && counter==rows/3);
		if(flag != 0)
		{			
			pStorageEngine->closeEntrySet(Trans,pIndexSet);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet->getId());
			pStorageEngine->removeEntrySet(Trans,pEntrySet->getId());//删除heap		
			paraIndex->SetSuccessFlag(true);
			return false;
		}
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexSet->getId();
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, entid, indxid);
		pStorageEngine->removeEntrySet(Trans,entid);//删除heap		
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_dll_004_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_indexscan_dll_004_Insert,pPara);
	REGTASK(test_indexscan_dll_004_Scan,pPara);
	return true;
}

#define ARRAY_LEN_CALC(ptr) sizeof(ptr)/sizeof(ptr[0])
#define DATA_LEN 1024

//计算数据长度，包括"\0"符
unsigned int data_len_calc(const char data[], unsigned int len)
{
	--len;
	while(len != 0 && data[len--] == '\0');
	return len != 0 ? len += 3 : 1;
}

void insert_data(char data[][1024], 
								 const int data_len,
								 EntrySet **pEntrySet,
								 Transaction *transaction)
{
	EntryID eid;
	DataItem di;
	for(int i = 0; i < data_len; ++i)
	{
		di.setData(&data[i]);
		di.setSize(data_len_calc(data[i], 200));
		(*pEntrySet)->insertEntry(transaction, eid, di);
	}
}

bool test_indexscan_mark_restore_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt;
		uint32 colid = 2024;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
			EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		const uint32 INSERT_ROW = 100;
		DataGenerater dg(INSERT_ROW, DATA_LEN);
		dg.dataGenerate();
		char data[INSERT_ROW][DATA_LEN];
		dg.dataToDataArray2D(data);
		insert_data(data, ARRAY_LEN_CALC(data), &pEntrySet,Trans);
		pStorageEngine->endStatement();

		uint32 col_id = 2025;
		ColumnInfo *co = (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 2;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->col_number[1] = 2;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->split_function =  my_split_heap_32;
		setColumnInfo(col_id, co);

		//create a index on testRelation
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans, 
			pEntrySet,
			BTREE_INDEX_ENTRY_SET_TYPE,
			col_id,
			0,
			NULL);
		IndexEntrySet *pIndexSet = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(
			Trans,
			pEntrySet,
			EntrySet::OPEN_EXCLUSIVE,
			paraIndex->m_nIndexEntryID,
			NULL));
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_indexscan_mark_restore_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	bool return_sta = true;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		//begin index scan
		char datum_data[20][20] = {"AAA", "AA"};
		int fieldno[2] = {1, 2};
		ScanCondition::CompareOperation co[2] = 
		{
			ScanCondition::GreaterThan,
			ScanCondition::GreaterThan
		};
		PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,co,fieldno,compare_func,2,vc);
		EntrySetScan *iess = pIndexSet->startEntrySetScan(Trans, EntrySet::SnapshotMVCC, vc);
		EntryID eid;
		DataItem di;
		char *temp;
		vector<string> v_cmp1, v_cmp2;
		iess->markPosition();
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			v_cmp1.push_back((char*)di.getData());
		}
		/* 读取标记 */
		iess->restorePosition();
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			v_cmp2.push_back((char*)di.getData());
		}
		pIndexSet->endEntrySetScan(iess);
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet->getId());	
		pStorageEngine->removeEntrySet(Trans,pEntrySet->getId());//删除heap		
		sort(v_cmp1.begin(), v_cmp1.end());
		sort(v_cmp2.begin(), v_cmp2.end());

		return_sta = (v_cmp1 == v_cmp2) ? true : false;
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_mark_restore_TestFrame()
{
	INITRANID()
		
	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_indexscan_mark_restore_Insert,pPara);
	REGTASK(test_indexscan_mark_restore_Scan,pPara);
	return true;
}


//改写test_index_transation_dll.cpp中的函数，进行测试

#define LEN_CALC(data, div) sizeof(data)/sizeof(div)

int current_index_entry_colid = 800;
ColumnInfo *current_entryset_colinfo = NULL;
ColumnInfo *current_index_entryset_colinfo = NULL;

void insert_data1(char data[][20], 
								 const int data_len,
								 EntrySet **pEntrySet,
								 Transaction *transaction)
{
	EntryID eid;
	DataItem di;
	for(int i = 0; i < data_len; ++i)
	{
		di.setData(&data[i]);
		di.setSize(strlen(data[i])+1);
		(*pEntrySet)->insertEntry(transaction, eid, di);
	}
}

ColumnInfo *alloc_fill_index_colinfo(const int nkeys, 
													const Spliti spi_func,
													const unsigned int col_num[],
													const int col_num_len,
													const CompareCallbacki ccb[],
													const int ccb_len)
{
	ColumnInfo *ci = (ColumnInfo*)malloc(sizeof(ColumnInfo));
	ci->keys = nkeys;
	ci->split_function = spi_func;
	ci->col_number = (size_t*)malloc(sizeof(size_t) * nkeys);
	for(int i = 0; i < col_num_len; ++i)
	{
		ci->col_number[i] = col_num[i];
	}
	ci->rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki) * nkeys);
	for(int i = 0; i < ccb_len; ++i)
	{
		ci->rd_comfunction[i] = ccb[i];
	}
	return ci;
}

/*
* entryset_spilt_to_any:
* 将entryset分割为任何数量的列
* 本次构造的列的长度分别为: 1 3 3 2 1 4
*/
void entryset_spilt_to_any(RangeDatai& rangeData, const char* str, int col, size_t len =0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 1;
	}
	if (col == 2)
	{
		rangeData.start = 1;
		rangeData.len = 3;
	}
	if (col == 3)
	{
		rangeData.start = 4;
		rangeData.len = 3;
	}
	if (col == 4)
	{
		rangeData.start = 7;
		rangeData.len = 2;
	}
	if (col == 5)
	{
		rangeData.start = 9;
		rangeData.len = 1;
	}
	if (col == 6)
	{
		rangeData.start = 10;
		rangeData.len = 4;
	}
}

bool all_data_equal(IndexEntrySet **pIndexEntrySet,
										std::vector<ScanCondition> &vc_scancondition,
										char data[][20],
										const int data_len,
										Transaction *transation,
										const EntrySet::SnapshotTypes opt = BaseEntrySet::SnapshotMVCC,
										const EntrySetScan::CursorMovementFlags scanTo = EntrySetScan::NEXT_FLAG)
{
	EntrySetScan *ess = (*pIndexEntrySet)->startEntrySetScan(transation,opt,vc_scancondition);
	DataItem di;
	EntryID eid;
	char *tmp;
	while(ess->getNext(scanTo, eid, di) == 0)
	{
		tmp = (char*)di.getData();
		for(int i = 0; i < data_len; ++i)
		{
			if(memcmp(data[i], tmp, strlen(tmp)) == 0)
			{
				memset(&data[i][0], 0, 20);
				break;
			}
		}
	}
	(*pIndexEntrySet)->endEntrySetScan(ess);
	for(int i = 0; i < data_len; ++i)
	{
		if(strlen(data[i]) != 0)
		{
			return false;
		}
	}
	return true;
}

//init scan condition
void new_scankey_init(const char datum_data[][20], 
										 const ScanCondition::CompareOperation co[],
										 const int fieldno[],
										 PToCompareFunc const compare_func[],
										 const int col_count,
										 std::vector<ScanCondition> &vc)
{
	ScanCondition sc;
	for(int i = 0; i < col_count; ++i)
	{
		sc.compare_func = compare_func[i];
		sc.fieldno = fieldno[i];
		sc.argument = (se_uint64)datum_data[i];
		sc.arg_length = strlen(datum_data[i]);
		sc.compop = co[i];
		vc.push_back(sc);
	}
}

bool test_index_transaction_001_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
	
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = entryset_spilt_to_any;
		uint32 colid = 2026;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID= pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
			EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		//测试数据
		char data[5][20] = 
		{
			"abcdefghijklmn",
			"abcdesssjklmn",
			"abcdeaaajklmn",
			"abdddfghijklmn",
			"abcdefghfgggmn"
		};
		insert_data1(data, LEN_CALC(data, data[0]), &pEntrySet,Trans);
		command_counter_increment();
		
		std::vector<std::string> vInsertData;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			str += "abcdefgh";
			vInsertData.push_back(str);
		}
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);

		//在第一列和第三列上建索引
		unsigned int keys[] = {1, 3};
		CompareCallbacki ccb[] = 
		{
			my_compare_str,
			my_compare_str
		};
		ColumnInfo *ci = alloc_fill_index_colinfo(2,entryset_spilt_to_any,keys,LEN_CALC(keys,
			keys[0]),ccb,LEN_CALC(ccb, ccb[0]));

		uint32 col_id = 2027;
		setColumnInfo(col_id, ci);
		
		paraIndex->m_nIndexEntryID= pStorageEngine->createIndexEntrySet(Trans,
		pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,col_id,0,NULL);
		
		IndexEntrySet *pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID,NULL);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_index_transaction_001_Select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	bool is_equal =false;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
							pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		//测试数据
		char data[5][20] = 
		{
			"abcdefghijklmn",
			"abcdesssjklmn",
			"abcdeaaajklmn",
			"abdddfghijklmn",
			"abcdefghfgggmn"
		};
				
		//扫描所有的数据
		char scankey_data[][20] = 
		{
			"a",
			"aaa"
		};
		ScanCondition::CompareOperation co[] = 
		{
			ScanCondition::GreaterEqual,
			ScanCondition::GreaterEqual
		};
		int fieldno[] = {1, 2};
		PToCompareFunc compare_func[] = 
		{
			my_compare_str,
			my_compare_str
		};
		std::vector<ScanCondition> vc_scancondition;
		new_scankey_init(scankey_data, co, fieldno, compare_func, 2, vc_scancondition);
		//scankey扫描所有的数据，查看索引是否同步更新
		is_equal = all_data_equal(&pIndexSet,vc_scancondition,data,LEN_CALC(data, data[0]),Trans);
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap		
		paraIndex->SetSuccessFlag(true);
		//扫描到所有的数据，测试成功
		if(is_equal)
		{		
			return true;
		}else
		{
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
}

bool test_index_transaction_001_TestFrame()
{
	INITRANID()
		
	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_index_transaction_001_Insert,pPara);
	REGTASK(test_index_transaction_001_Select,pPara);
	return true;
}

bool test_index_transaction_002_Create(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
	
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = entryset_spilt_to_any;
		uint32 colid = 2028;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		//在第一列和第三列上建索引
		unsigned int keys[] = {1, 2};
		CompareCallbacki ccb[] = 
		{
			my_compare_str,
			my_compare_str
		};
		ColumnInfo *ci = alloc_fill_index_colinfo(2,entryset_spilt_to_any,keys,LEN_CALC(keys,
			keys[0]),ccb,LEN_CALC(ccb, ccb[0]));

		uint32 col_id = 2029;
		setColumnInfo(col_id, ci);
		
		paraIndex->m_nIndexEntryID= pStorageEngine->createIndexEntrySet(Trans,
		pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,col_id,0,NULL);
		
		IndexEntrySet *pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID,NULL);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_index_transaction_002_Select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	bool is_equal =false;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
						pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		//测试数据
		char data[5][20] = 
		{
			"abcdefghijklmn",
			"abcdesssjklmn",
			"abcdeaaajklmn",
			"abdddfghijklmn",
			"abcdefghfgggmn"
		};
		//插入测试数据
		insert_data1(data, LEN_CALC(data, data[0]), &pEntrySet,Trans);	
		command_counter_increment();
		std::vector<std::string> vInsertData;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			str += "abcdefgh";
			vInsertData.push_back(str);
		}
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
		//扫描所有的数据
		char scankey_data[][20] = 
		{
			"a",
			"aaa"
		};
		ScanCondition::CompareOperation co[] = 
		{
			ScanCondition::GreaterEqual,
			ScanCondition::GreaterEqual
		};
		int fieldno[] = {1, 2};
		PToCompareFunc compare_func[] = 
		{
			my_compare_str,
			my_compare_str
		};
		std::vector<ScanCondition> vc_scancondition;
		new_scankey_init(scankey_data, co, fieldno, compare_func, 2, vc_scancondition);
		//scankey扫描所有的数据，查看索引是否同步更新
		is_equal = all_data_equal(&pIndexSet,vc_scancondition,data,LEN_CALC(data, data[0]),Trans);
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap		
		paraIndex->SetSuccessFlag(true);
		//扫描到所有的数据，测试成功
		if(is_equal)
		{		
			return true;
		}else
		{
			return false;
		}
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
}

bool test_index_transaction_002_TestFrame()
{
	INITRANID()
		
	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_index_transaction_002_Create,pPara);
	REGTASK(test_index_transaction_002_Select,pPara);
	return true;
}

bool test_index_transaction_003_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
	
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = entryset_spilt_to_any;
		uint32 colid = 2030;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID= pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
			EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		//测试数据
		char data[5][20] = 
		{
			"abcdefghijklmn",
			"abcdesssjklmn",
			"abcdeaaajklmn",
			"abdddfghijklmn",
			"abcdefghfgggmn"
		};
		
		insert_data1(data, LEN_CALC(data, data[0]), &pEntrySet,Trans);
		command_counter_increment();
		
		std::vector<std::string> vInsertData;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			str += "abcdefgh";
			vInsertData.push_back(str);
		}
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
		
		//在第一列和第三列上建索引
		unsigned int keys[] = {2, 5};
		CompareCallbacki ccb[] = 
		{
			my_compare_str,
			my_compare_str
		};
		ColumnInfo *ci = alloc_fill_index_colinfo(2,my_split_index_31,keys,LEN_CALC(keys,
			keys[0]),ccb,LEN_CALC(ccb, ccb[0]));

		uint32 col_id = 2031;
		setColumnInfo(col_id, ci);
		
		paraIndex->m_nIndexEntryID= pStorageEngine->createIndexEntrySet(Trans,
						pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,col_id,0,NULL);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

EntryID find_single_data_use_seq_scan(const char *dest_data,
												EntrySet *pEntrySet, 
												std::vector<ScanCondition> &vc_scancondition,
												bool &found,
												Transaction *transcation,
												EntrySetScan::CursorMovementFlags scanTo = EntrySetScan::NEXT_FLAG,
												EntrySet::SnapshotTypes opt = EntrySet::SnapshotMVCC)
{
	found = false;
	EntrySetScan *ess = pEntrySet->startEntrySetScan(transcation,opt,vc_scancondition);
	EntryID eid, return_eid;
	memset(&eid, 0, sizeof(eid));
	memset(&return_eid, 0, sizeof(return_eid));
	DataItem di;
	while(ess->getNext(scanTo, eid, di) == 0)
	{
		if(memcmp(dest_data, di.getData(), di.getSize()) == 0)
		{
			return_eid = eid;
			found = true;
		}
	}
	pEntrySet->endEntrySetScan(ess);
	return return_eid;
}

bool is_found_single_data_use_index(IndexEntrySet **pIndexEntrySet,
											std::vector<ScanCondition> &vc_scancondition,
											const char *dest_data,
											Transaction *transaction,
											EntrySetScan::CursorMovementFlags opt = EntrySetScan::NEXT_FLAG)
{
	EntrySetScan *ess = (*pIndexEntrySet)->startEntrySetScan(transaction,BaseEntrySet::SnapshotMVCC,vc_scancondition);
	int found_count = -1;
	DataItem di;
	EntryID eid;
	while(ess->getNext(opt, eid, di) == 0)
	{
		if(memcmp(dest_data, di.getData(), di.getSize()) == 0)
		{
			found_count = 1;
		}
	}
	(*pIndexEntrySet)->endEntrySetScan(ess);
	if(1 == found_count)
	{
		return true;
	}else
	{
		return false;
	}
}

bool test_index_transaction_003_Select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	bool is_equal =false;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
						pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		
		//查找数据 abdddfghijklmn 并删除
		std::vector<ScanCondition> vc_scancondition;
		bool found;
		EntryID delete_eid = find_single_data_use_seq_scan("abdddfghijklmn",pEntrySet,
															vc_scancondition,found,Trans);
		if(found)
			pEntrySet->deleteEntry(Trans, delete_eid);
		else//没有找到 abdddfghijklmn 测试失败
			return false;
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_index_transaction_003_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	bool is_equal =false;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
							pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		/*
		* 利用索引单独扫描 a bdd dfg hi j klmn
		* 于第二和第五列上使用scankey
		*/
		char scankey_data[][20] = 
		{
			"bdd",
			"j"
		};
		ScanCondition::CompareOperation co[] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		int fieldno[] = {1, 2};
		PToCompareFunc compare_func[] = 
		{
			my_compare_str,
			my_compare_str
		};
		//查找数据 abdddfghijklmn 并删除
		std::vector<ScanCondition> vc_scancondition;
		new_scankey_init(scankey_data, co, fieldno, compare_func, 2, vc_scancondition);
		bool is_found = is_found_single_data_use_index(&pIndexSet,vc_scancondition,"abdddfghijklmn",Trans);
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap			
		paraIndex->SetSuccessFlag(true);
		if(is_found/*找到则测试失败*/)
		{
			return false;
		}else
		{
			return true;
		}
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
}

bool test_index_transaction_003_TestFrame()
{
	INITRANID()
		
	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_index_transaction_003_Insert,pPara);
	REGTASK(test_index_transaction_003_Select,pPara);
	REGTASK(test_index_transaction_003_Scan,pPara);
	return true;
}

bool test_index_transaction_004_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
	
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = entryset_spilt_to_any;
		uint32 colid = 2032;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID= pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
					 EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		/*
		* 创建第一个索引，该索引建于第二列和
		* 第六列上
		*/
		unsigned int idnex1_col_num[] ={2, 6};
		CompareCallbacki index1_cco[] = 
		{
			my_compare_str,
			my_compare_str
		};
		ColumnInfo *index1_ci = alloc_fill_index_colinfo(2,my_split_index_34,
										idnex1_col_num,
										LEN_CALC(idnex1_col_num, idnex1_col_num[0]),
										index1_cco,LEN_CALC(index1_cco, index1_cco[0]));
		
		uint32 col_id = 2033;
		setColumnInfo(col_id, index1_ci);
		
		paraIndex->m_nIndexEntryID= pStorageEngine->createIndexEntrySet(Trans,
					pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,col_id,0,NULL);
		
		IndexEntrySet *pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID,NULL);

		//构造索引1的测试数据
		char index1_data[][20] = 
		{
			"testdata123456",
			"testdata585o,sm",
			"dddddddddddddd",
			"pppppppdwwwwww",
			"ddlsmmmmswoskm",
			"aaaa7aaaaaaaaa",
			"ddddoegher7,sm",
			"dwoodmekkssmas"
		};
		insert_data1(index1_data, LEN_CALC(index1_data,index1_data[0]),&pEntrySet,Trans);
		command_counter_increment();
		std::vector<std::string> vInsertData;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			str += "abcdefgh";
			vInsertData.push_back(str);
		}
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_index_transaction_004_Insert2(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	bool is_equal =false;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		unsigned int index2_col_num[] ={4, 6};
		CompareCallbacki index2_cco[] = 
		{
			my_compare_str,
			my_compare_str
		};
		ColumnInfo *index2_ci = 
			alloc_fill_index_colinfo
			(
			2,
			my_split_index_24,
			index2_col_num,
			LEN_CALC(index2_col_num, index2_col_num[0]),
			index2_cco,
			LEN_CALC(index2_cco, index2_cco[0])
			);
		uint32 col_id = 2034;
		setColumnInfo(col_id, index2_ci);
		
		paraIndex->m_nIndexEntryID2 = pStorageEngine->createIndexEntrySet(Trans,
						pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,col_id,0,NULL);
		
		IndexEntrySet *pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID2,NULL);
		//构造索引2的测试数据
		char index2_data[][20] = 
		{
			"testdata2yuooo",
			"mmma&&&&1uuuam",
			"kkkk$$$$kkkkgg",
			"testdata10uyio",
			"HHHHHHH888!9sm",
			"uuuu5555ggggrt",
			"iphone575gepin"
		};
		insert_data1(index2_data, LEN_CALC(index2_data,index2_data[0]),&pEntrySet,Trans);
		command_counter_increment();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_index_transaction_004_Select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	bool is_equal =false;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID,NULL);
		
		IndexEntrySet *pIndexSet2 = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID2,NULL);
		
		/*
		* 构建索引1的scankey
		* 这里需要查找 d ddd oem sm a \0,sm dfe
		*/
		vector<ScanCondition> vc_index1_scancondition;
		char index1_scankey[][20] = {"ddd", "7,sm"};
		ScanCondition::CompareOperation index1_co[] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		int index1_fieldno[] = {1, 2};
		PToCompareFunc index1_cmp_func[] =
		{
			my_compare_str,
			my_compare_str
		};
		new_scankey_init
			(
				index1_scankey,
				index1_co,
				index1_fieldno,
				index1_cmp_func,
				2,
				vc_index1_scancondition
			);
		bool is_found = is_found_single_data_use_index
			(
				&pIndexSet,
				vc_index1_scancondition,
				"ddddoegher7,sm",Trans
			);
		if(!is_found)
			return false;

		/*
		* 构建索引2的scankey
		* 这里需要查找 & &&& &&& \0\0 ! \0,sm
		*/
		vector<ScanCondition> vc_index2_scancondition;
		char index2_scankey[][20] = {"88","!9sm"};
		ScanCondition::CompareOperation index2_co[] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		int index2_fieldno[] = {1, 2};
		PToCompareFunc index2_cmp_func[] =
		{
			my_compare_str,
			my_compare_str
		};
		new_scankey_init
			(
			index2_scankey,
			index2_co,
			index2_fieldno,
			index2_cmp_func,
			2,
			vc_index2_scancondition
			);
		bool is_found2 = is_found_single_data_use_index
			(
			&pIndexSet2,
			vc_index2_scancondition,
			"HHHHHHH888!9sm",Trans
			);
		if(!is_found2)
			return false;
		
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pIndexSet2);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID2);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap		
		paraIndex->SetSuccessFlag(true);
		return true;
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
}

bool test_index_transaction_004_TestFrame()
{
	INITRANID()
		
	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_index_transaction_004_Insert,pPara);
	REGTASK(test_index_transaction_004_Insert2,pPara);
	REGTASK(test_index_transaction_004_Select,pPara);
	return true;
}

bool test_index_transaction_005_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
	
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = entryset_spilt_to_any;
		uint32 colid = 2035;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);


		/*
		* 构建测试数据
		*/
		char data[][20] = 
		{
			"testdatatest23",
			"hhhhhhhhhhhh88",
			"diyiehaohxiann",
			"donggcheffffkk",
			"testacebtatest", //这是要找的数据
			"testabcdshifff",
			"adfklendkeyyyy"
		};
		insert_data1(data, LEN_CALC(data, data[0]),&pEntrySet,Trans);
		command_counter_increment();
		std::vector<std::string> vInsertData;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			str += "abcdefgh";
			vInsertData.push_back(str);
		}
		InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
		/*
		* 建索引，于第三列和第四列上建索引(1 3 3 2 1 4)
		*/

		unsigned int idnex1_col_num[] ={3, 4};
		CompareCallbacki index1_cco[] = 
		{
			my_compare_str,
			my_compare_str
		};
		ColumnInfo *index1_ci = alloc_fill_index_colinfo(2,my_split_heap_32,
										idnex1_col_num,
										LEN_CALC(idnex1_col_num, idnex1_col_num[0]),
										index1_cco,LEN_CALC(index1_cco, index1_cco[0]));
		
		uint32 col_id = 2036;
		setColumnInfo(col_id, index1_ci);
		
		paraIndex->m_nIndexEntryID= pStorageEngine->createIndexEntrySet(Trans,
				pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,col_id,0,NULL);
		
		IndexEntrySet *pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID,NULL);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_index_transaction_005_Select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	bool is_equal =false;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
						pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		/*
		* 构建测试数据
		*/
		const char *dest_data = "testacebtatest";
		vector<ScanCondition> vc_scancondition;
		bool is_found = false;
		find_single_data_use_seq_scan(dest_data,pEntrySet,vc_scancondition,is_found,Trans);
		if(!is_found/*没有找到dest_data,测试失败*/)
			return false;

		char index_scankey_data[][20] = {"ace","bt"};
		ScanCondition::CompareOperation index_co[] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		int index_fieldno[] = {1, 2};
		PToCompareFunc index_filedno_cmp_func[] = 
		{
			my_compare_str,
			my_compare_str
		};
		new_scankey_init(index_scankey_data,index_co,index_fieldno, 
				index_filedno_cmp_func,2,vc_scancondition);
		is_found = false;
		is_found = is_found_single_data_use_index(&pIndexSet,vc_scancondition,dest_data,Trans);
		if(!is_found/*没有找到dest_data,测试失败*/)
			return false;
		
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap				
		paraIndex->SetSuccessFlag(true);
		return true;
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
}

bool test_index_transaction_005_TestFrame()
{
	INITRANID()
		
	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_index_transaction_005_Insert,pPara);
	REGTASK(test_index_transaction_005_Select,pPara);
	return true;

}


//改写test_index_cmp_dll.cpp中的函数，进行测试

#define endnum 13000
char storeData[20000][20];
typedef EntrySetCollInfo<3,2> EntrySetT;

void predict_data_largeMount()
{
    int i;	
    char baseData[6]={"12248"};
    for (i=0;i<endnum;i++)//存储预测数据
       memcpy(storeData[i],baseData,6);//预估数据存在storeData中
}

//init scan condition
void my_scankey(const char datum_data[][20], 
        const ScanCondition::CompareOperation co[],
        const int fieldno[],
        PToCompareFunc compare_func[],
        const int col_count,
        std::vector<ScanCondition> &vc)
{
    ScanCondition sc;
    for(int i = 0; i < col_count; ++i)
    {
        sc.compare_func = compare_func[i];
        sc.fieldno = fieldno[i];
        sc.argument = (se_uint64)datum_data[i];
        sc.arg_length = strlen(datum_data[i]);
        sc.compop = co[i];
        vc.push_back(ScanCondition(fieldno[i],co[i],(se_uint64)datum_data[i], 
              strlen(datum_data[i]),compare_func[i]));
    }
}

bool test_indexscan_LargeMount_dll_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		predict_data_largeMount();
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo* p_col=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		p_col->col_number = NULL;
		p_col->keys = 0;
		p_col->rd_comfunction = NULL;
		p_col->split_function = my_spilt;
		uint32 colid = 2037;
		setColumnInfo(colid, p_col);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
					EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		DataItem di;
        EntryID eid;
        char data[20]={"12248"};
        //insert data into pEntrySet
        for(int i = 0; i < endnum; i++)
        {
            di.setData(data);
            di.setSize(strlen(data)+1);
            pEntrySet->insertEntry(Trans,eid,di);
        }
        command_counter_increment();
		
		//create a index on testRelation
		//创建index并插入一些其他的数据
		ColumnInfo *co= (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 2;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->col_number[1] = 2;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->split_function = my_split_heap_32;
		uint32 co_lid = 2038;
		setColumnInfo(co_lid, co);
        paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,
        				pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,co_lid);
        IndexEntrySet *pIndexSet= pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
        command_counter_increment();
		return true;
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
}

bool test_indexscan_LargeMount_dll_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	bool return_sta = true;
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,
						pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);

		//begin index scan
        char datum_data[20][20] = {"123", "45"};
        int fieldno[2] = {1, 2};
        ScanCondition::CompareOperation co[2] = 
        {
            ScanCondition::LessThan,
            ScanCondition::GreaterThan
        };
        PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
        vector<ScanCondition> vc;
        my_scankey(datum_data,co,fieldno,compare_func,2,vc);
        EntrySetScan *iess = pIndexSet->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,vc);
        DataItem di;
        EntryID eid;
		memset(&eid, 0, sizeof(eid));
        memset(&di, 0, sizeof(di));
        char *temp;
        int flag = 65536;
        int storeRow=0;
        while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
        {
            temp = (char*)di.getData();
            flag = memcmp(temp,storeData[storeRow],sizeof(temp));
            assert(flag==0);
            if(flag != 0)
            {			
                cout<<"查询的数据与预测数据不一致，出错!"<<endl;
            }
            storeRow++;
        }
        pIndexSet->endEntrySetScan(iess);
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, paraIndex->m_nHeapEntryID, paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap			
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_indexscan_LargeMount_dll_TestFrame()
{
	INITRANID()
		
	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_indexscan_LargeMount_dll_Insert,pPara);
	REGTASK(test_indexscan_LargeMount_dll_Scan,pPara);
	return true;
}


//改写test_index_cmp_dll.cpp中的测试用例，进行测试
void heap_split_uniqe_01_dll(RangeDatai& rang, const char*, int i, size_t len)
{
    rang.start = 0;
    rang.len = 2;
}

void index_split_uniqe_01_dll(RangeDatai& rang, const char*, int i , size_t len)
{
    rang.start = 0;
    rang.len = 2;
}

bool test_index_uniqe_01_dll_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();

		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		p_colinfo->col_number = NULL;
		p_colinfo->keys = 1;
		p_colinfo->split_function = heap_split_uniqe_01_dll;
        p_colinfo->rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki));
        p_colinfo->rd_comfunction[0] = my_compare_str;
		uint32 colid = 2039;
		setColumnInfo(colid,p_colinfo);
		
		uint32 indexcolid = 2040;
		ColumnInfo* p_indexcolinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		size_t *p = (size_t*)malloc(sizeof(size_t));
        *p = 1;
		p_indexcolinfo->col_number = p;
		p_indexcolinfo->keys = 1;
		p_indexcolinfo->split_function = index_split_uniqe_01_dll;
        p_indexcolinfo->rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki));
        p_indexcolinfo->rd_comfunction[0] = my_compare_str;
		setColumnInfo(indexcolid,p_indexcolinfo);//strlen不计算最后"/0"的长度
		
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		pStorageEngine->endStatement();

		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
        std::vector<std::string> vInsertData;
		vInsertData.push_back("19");
		vInsertData.push_back("28");
        InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);
        
        paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,
				pEntrySet,UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE,indexcolid);//建索引
        IndexEntrySet *pIndexSet= static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(
						Trans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID,NULL));
        std::vector<std::string> vResult1;
        SearchCondition scanCondition1;
        GetIndexScanResults(vResult1,Trans,pIndexSet,scanCondition1.Keys());
		pStorageEngine->closeEntrySet(Trans, pIndexSet);
		pStorageEngine->closeEntrySet(Trans, pEntrySet);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_index_uniqe_01_dll_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	Transaction* Trans = paraIndex->GetTransaction();
	EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
	IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(
						Trans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
	try
	{
		std::vector<std::string> vInsertData;
	    vInsertData.push_back("19"); //插入相同的数据检验唯一索引
        InsertData(Trans,pEntrySet,vInsertData,&paraIndex->m_vEntryID);// catch the problem

        pStorageEngine->endStatement();

        pIndexSet->getType();

        //构造一个查询
        SearchCondition scanCondition;
        scanCondition.Add(1,LessEqual,"9",str_compare);

        std::vector<std::string> vResult;
        GetIndexScanResults(vResult,Trans,pIndexSet,scanCondition.Keys());

        //检查结果
        std::set<std::string> sResult(vResult.begin(),vResult.end());
        std::set<std::string> sDesired;
		sDesired.insert("19");
		sDesired.insert("28");
		sDesired.insert("19");

        assert(sResult == sDesired);

		pStorageEngine->closeEntrySet(Trans,pIndexSet);
        pStorageEngine->closeEntrySet(Trans,pEntrySet);
        pStorageEngine->removeIndexEntrySet(Trans,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID, NULL);
        pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID,NULL);
        paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		pStorageEngine->closeEntrySet(Trans,pIndexSet);
        pStorageEngine->closeEntrySet(Trans,pEntrySet);
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return true;
	}
	return false;

}

bool test_index_uniqe_01_dll_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_index_uniqe_01_dll_Insert,pPara);
	REGTASK(test_index_uniqe_01_dll_Scan,pPara);
	return true;
}

#define predictdata "012562"

bool test_indexMult_SameCol_dll_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt;
		uint32 colid = 2041;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid,0,NULL);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
			EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		//insert data into pEntrySet
        char data[20][20] = {"123456", "123789", "012562", "012120", "012456"};
        DataItem di;
        EntryID eid;
        //insert data into pEntrySet
        for(int i = 0; i < 5; i++)
        {
            di.setData(&data[i]);
            di.setSize(strlen(data[i])+1);
            pEntrySet->insertEntry(Trans,eid, di);
        }
		char dataarr[20];
		for (int digstr=123000; digstr<124999; digstr++)
		{
			sprintf(dataarr,"%d",digstr);
			di.setData(&dataarr);
			di.setSize(strlen(dataarr)+1);
			pEntrySet->insertEntry(Trans,eid,di);
		}
        command_counter_increment();
		
		//create a index on testRelation
		//创建index并插入一些其他的数据
		ColumnInfo *co= (ColumnInfo *)malloc(sizeof(ColumnInfo));
		co->keys = 2;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->col_number[1] = 2;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->split_function = my_split_heap_32;
		uint32 co_lid = 2042;
		setColumnInfo(co_lid, co);
		
        paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Trans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,co_lid);
		paraIndex->m_nIndexEntryID2= pStorageEngine->createIndexEntrySet(Trans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,co_lid);
		paraIndex->m_nIndexEntryID3 = pStorageEngine->createIndexEntrySet(Trans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,co_lid);
		IndexEntrySet *pIndexSet= pStorageEngine->openIndexEntrySet(Trans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		IndexEntrySet *pIndexSet2= pStorageEngine->openIndexEntrySet(Trans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID2);
		IndexEntrySet *pIndexSet3= pStorageEngine->openIndexEntrySet(Trans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID3);
        command_counter_increment();
		
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_indexMult_SameCol_dll_Scan(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,
							paraIndex->m_nHeapEntryID);
		IndexEntrySet *pIndexSet= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID);
		IndexEntrySet *pIndexSet2= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID2);
		IndexEntrySet *pIndexSet3= (IndexEntrySet *)pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nIndexEntryID3);

		//begin index scan
        char datum_data[20][20] = {"123", "45"};
        int fieldno[2] = {1, 2};
        ScanCondition::CompareOperation co[2] = 
        {
            ScanCondition::LessThan,
            ScanCondition::GreaterThan
        };
        PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
        vector<ScanCondition> vc;
        my_scankey(datum_data,co,fieldno,compare_func,2,vc);
        EntrySetScan *iess1 = pIndexSet->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,vc);
        DataItem di;
        EntryID eid;
		memset(&eid, 0, sizeof(eid));
        memset(&di, 0, sizeof(di));
        char *temp;
        int flag = 65536;
        while(iess1->getNext(EntrySetScan::PREV_FLAG, eid, di) == 0)
        {
            temp = (char*)di.getData();
            flag = memcmp(temp,predictdata,sizeof(temp));
        }
        pIndexSet->endEntrySetScan(iess1);
        assert(flag == 0);
        if(flag != 0)
        {			
            cout<<"查询的数据与预测数据不一致，出错!"<<endl;
            pStorageEngine->closeEntrySet(Trans,pIndexSet);
            pStorageEngine->closeEntrySet(Trans,pEntrySet);
            pStorageEngine->removeIndexEntrySet(Trans,pIndexSet->getId(),pIndexSet->getId());
			pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap	
            paraIndex->SetSuccessFlag(false);
            return false;
        }

        EntrySetScan *iess2 = pIndexSet2->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,vc);
        memset(&eid, 0, sizeof(eid));
        memset(&di, 0, sizeof(di));
        while(iess2->getNext(EntrySetScan::PREV_FLAG, eid, di) == 0)
        {
            temp = (char*)di.getData();
            flag = memcmp(temp,predictdata,sizeof(temp));
        }
        pIndexSet2->endEntrySetScan(iess2);
        assert(flag == 0);
        if(flag != 0)
        {			
            cout<<"查询的数据与预测数据不一致，出错!"<<endl;
            pStorageEngine->closeEntrySet(Trans,pIndexSet2);
            pStorageEngine->closeEntrySet(Trans,pEntrySet);
            pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet2->getId());
			pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap	
            paraIndex->SetSuccessFlag(false);
            return false;
        }

        EntrySetScan *iess3 = pIndexSet3->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,vc);
        memset(&eid, 0, sizeof(eid));
        memset(&di, 0, sizeof(di));
        while(iess3->getNext(EntrySetScan::PREV_FLAG, eid, di) == 0)
        {
            temp = (char*)di.getData();
            flag = memcmp(temp,predictdata,sizeof(temp));
        }
        pIndexSet3->endEntrySetScan(iess3);
        assert(flag == 0);
        if(flag != 0)
        {			
            cout<<"查询的数据与预测数据不一致，出错!"<<endl;
            pStorageEngine->closeEntrySet(Trans, pIndexSet);
            pStorageEngine->closeEntrySet(Trans, pEntrySet);
            pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet3->getId());
			pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap	
            paraIndex->SetSuccessFlag(false);
            return false;
        }
        pStorageEngine->closeEntrySet(Trans, pIndexSet);
        pStorageEngine->closeEntrySet(Trans, pIndexSet2);
        pStorageEngine->closeEntrySet(Trans, pIndexSet3);
        pStorageEngine->closeEntrySet(Trans, pEntrySet);
        pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet->getId());
        pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet2->getId());
        pStorageEngine->removeIndexEntrySet(Trans,pEntrySet->getId(),pIndexSet3->getId());
		pStorageEngine->removeEntrySet(Trans,paraIndex->m_nHeapEntryID);//删除heap			
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_indexMult_SameCol_dll_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_indexMult_SameCol_dll_Insert,pPara);
	REGTASK(test_indexMult_SameCol_dll_Scan,pPara);
	return true;
}

bool test_trans_index_DeleteRollback_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
    try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt_heap_1;
		uint32 colid = 2043;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		//插入数据
		std::vector<std::string> vInserted;
		vInserted.push_back("2");
		vInserted.push_back("9");
		vInserted.push_back("7");
		vInserted.push_back("5");
        InsertData(Trans,pEntrySet,vInserted,&paraIndex->m_vEntryID);

		//关闭
        pStorageEngine->closeEntrySet(Trans,pEntrySet);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_trans_index_DeleteRollback_Delete(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
    try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		//删除一些数据
		std::vector<EntryID> vDeleteIds;
		vDeleteIds.push_back(paraIndex->m_vEntryID[0]);
		vDeleteIds.push_back(paraIndex->m_vEntryID[2]);
		vDeleteIds.push_back(paraIndex->m_vEntryID[3]);
		DeleteData(Trans,pEntrySet,vDeleteIds);

		//回滚
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		paraIndex->SetSuccessFlag(false);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_trans_index_DeleteRollback_TestFrame()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(test_trans_index_DeleteRollback_Insert,pPara);
	REGTASK(test_trans_index_DeleteRollback_Delete,pPara);
	return true;

}

//事务之间有先后顺序的测试用例

EntrySetID seq_heapId;
EntrySetID seq_indexId;
bool test_sequence_index_trans_insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
    try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_split_heap_321;
		uint32 colid = 2044;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		seq_heapId = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_EXCLUSIVE,seq_heapId);

		//插入数据
		std::vector<std::string> vInserted;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			vInserted.push_back(str);
		}
		vInserted.push_back("123456");
		vInserted.push_back("234567");
		vInserted.push_back("345678");
		vInserted.push_back("456789");
        InsertData(Trans,pEntrySet,vInserted,&paraIndex->m_vEntryID);
	
		ColumnInfo* co = (ColumnInfo* )malloc(sizeof(ColumnInfo));
		co->keys = 2;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->col_number[1] = 2;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->split_function = my_split_heap_32;
		uint32 col_id = 2045;
		setColumnInfo(col_id, co);
		seq_indexId= pStorageEngine->createIndexEntrySet(Trans,
							pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,col_id,0,NULL);
		
		IndexEntrySet *pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,seq_indexId,NULL);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_insert_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_trans_insert,pPara,1,100);
	return true;
}
bool test_sequence_index_trans_select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_EXCLUSIVE,seq_heapId);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_EXCLUSIVE,seq_indexId);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"234","56"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"234567",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, seq_heapId, seq_indexId);
		pStorageEngine->removeEntrySet(Trans,seq_heapId);//删除heap				
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_select_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_trans_select,pPara,2,100);
	return true;
}

EntrySetID seq_heapId_1;
EntrySetID Seq_indexId_1;
bool test_sequence_index_001_create(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo* pCol = (ColumnInfo* )malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321;
		uint32 colid = 2046;
		setColumnInfo(colid, pCol);
		seq_heapId_1 = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,seq_heapId_1);

		ColumnInfo* pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys * sizeof(size_t));
		pIndexCol->col_number[0] = 1;
		pIndexCol->col_number[1] = 2;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys * sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_heap_32;
		uint32 col_id = 2047;
		setColumnInfo(col_id,pIndexCol);
		Seq_indexId_1 = pStorageEngine->createIndexEntrySet(Trans,pEntrySet,
								BTREE_INDEX_ENTRY_SET_TYPE,col_id,NULL);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}
bool test_sequence_index_001_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,seq_heapId_1);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,Seq_indexId_1);
		std::vector<std::string> vInsert;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			vInsert.push_back(str);
		}
		vInsert.push_back("123456");
		vInsert.push_back("234567");
		vInsert.push_back("345678");
		vInsert.push_back("456789");
		InsertData(Trans,pEntrySet,vInsert,&paraIndex->m_vEntryID);
		command_counter_increment();
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_001_insert_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_001_create,pPara,1,101);
	REGTASKSEQ(test_sequence_index_001_Insert,pPara,1,101);
	return true;
}

bool test_sequence_index_001_select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_EXCLUSIVE,seq_heapId_1);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_EXCLUSIVE,Seq_indexId_1);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"234","56"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);

		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"234567",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pStorageEngine->removeIndexEntrySet(Trans, seq_heapId_1, Seq_indexId_1);
		pStorageEngine->removeEntrySet(Trans,seq_heapId_1);//删除heap			
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_sequence_index_001_select_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_001_select,pPara,2,101);
	return true;
}

EntrySetID seq_heapId_2;
EntrySetID Seq_indexId_2;
bool test_sequence_index_002_create(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo* pCol = (ColumnInfo* )malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321;
		uint32 colid = 2048;
		setColumnInfo(colid, pCol);
		seq_heapId_2 = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,seq_heapId_2);

		ColumnInfo* pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys * sizeof(size_t));
		pIndexCol->col_number[0] = 1;
		pIndexCol->col_number[1] = 2;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys * sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_heap_32;
		uint32 col_id = 2049;
		setColumnInfo(col_id,pIndexCol);
		Seq_indexId_2 = pStorageEngine->createIndexEntrySet(Trans,pEntrySet,
								BTREE_INDEX_ENTRY_SET_TYPE,col_id,NULL);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_002_create_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_002_create,pPara,1,102);
	return true;
}

bool test_sequence_index_002_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,seq_heapId_2);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,Seq_indexId_2);
		std::vector<std::string> vInsert;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			vInsert.push_back(str);
		}
		vInsert.push_back("123456");
		vInsert.push_back("234567");
		vInsert.push_back("345678");
		vInsert.push_back("456789");
		InsertData(Trans,pEntrySet,vInsert,&paraIndex->m_vEntryID);
		command_counter_increment();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_002_select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_EXCLUSIVE,seq_heapId_2);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_EXCLUSIVE,Seq_indexId_2);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"234","56"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		new_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"234567",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, seq_heapId_2, Seq_indexId_2);
		pStorageEngine->removeEntrySet(Trans,seq_heapId_2);//删除heap	
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_sequence_index_002_select_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_002_Insert,pPara,2,102);
	REGTASKSEQ(test_sequence_index_002_select,pPara,2,102);
	return true;
}

EntrySetID seq_heapId_3;
EntrySetID Seq_indexId_3;
bool test_sequence_index_003_create(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo* pCol = (ColumnInfo* )malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321;
		uint32 colid = 2050;
		setColumnInfo(colid, pCol);
		seq_heapId_3 = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,seq_heapId_3);

		ColumnInfo* pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys * sizeof(size_t));
		pIndexCol->col_number[0] = 1;
		pIndexCol->col_number[1] = 2;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys * sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_heap_32;
		uint32 col_id = 2051;
		setColumnInfo(col_id,pIndexCol);
		Seq_indexId_3 = pStorageEngine->createIndexEntrySet(Trans,pEntrySet,
								BTREE_INDEX_ENTRY_SET_TYPE,col_id,NULL);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_003_create_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_003_create,pPara,1,103);
	return true;
}

bool test_sequence_index_003_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,seq_heapId_3);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,Seq_indexId_3);
		std::vector<std::string> vInsert;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			vInsert.push_back(str);
		}
		vInsert.push_back("123456");
		vInsert.push_back("234567");
		vInsert.push_back("345678");
		vInsert.push_back("456789");
		InsertData(Trans,pEntrySet,vInsert,&paraIndex->m_vEntryID);
		command_counter_increment();
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_003_insert_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_003_Insert,pPara,2,103);
	return true;
}

bool test_sequence_index_003_select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_EXCLUSIVE,seq_heapId_3);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_EXCLUSIVE,Seq_indexId_3);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"234","56"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"234567",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, seq_heapId_3, Seq_indexId_3);
		pStorageEngine->removeEntrySet(Trans,seq_heapId_3);//删除heap	
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_sequence_index_003_select_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_003_select,pPara,3,103);
	return true;
}

EntrySetID seq_heapId_4;
EntrySetID Seq_indexId_4;
bool test_sequence_index_004_create(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo* pCol = (ColumnInfo* )malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321;
		uint32 colid = 2052;
		setColumnInfo(colid, pCol);
		seq_heapId_4 = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,seq_heapId_4);

		ColumnInfo* pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys * sizeof(size_t));
		pIndexCol->col_number[0] = 1;
		pIndexCol->col_number[1] = 2;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys * sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_heap_32;
		uint32 col_id = 2053;
		setColumnInfo(col_id,pIndexCol);
		Seq_indexId_4 = pStorageEngine->createIndexEntrySet(Trans,pEntrySet,
								BTREE_INDEX_ENTRY_SET_TYPE,col_id,NULL);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_004_create_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_004_create,pPara,1,104);
	return true;
}

bool test_sequence_index_004_Insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans,EntrySet::OPEN_EXCLUSIVE,seq_heapId_4);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_EXCLUSIVE,Seq_indexId_4);
		std::vector<std::string> vInsert;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			vInsert.push_back(str);
		}
		vInsert.push_back("123456");
		vInsert.push_back("234567");
		vInsert.push_back("345678");
		vInsert.push_back("456789");
		InsertData(Trans,pEntrySet,vInsert,&paraIndex->m_vEntryID);
		command_counter_increment();
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_004_insert_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_004_Insert,pPara,2,104);
	return true;
}

bool test_sequence_index_004_select(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_EXCLUSIVE,seq_heapId_4);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_EXCLUSIVE,Seq_indexId_4);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"234","56"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		new_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"234567",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_sequence_index_004_select_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_004_select,pPara,3,104);
	return true;
}

bool test_sequence_index_004_update(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_EXCLUSIVE,seq_heapId_4);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_EXCLUSIVE,Seq_indexId_4);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"456","78"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"456789",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		else
		{
			DataItem Sel_Update;
			char Update[] = "789abc";
			Sel_Update.setData((void*)Update);
			Sel_Update.setSize(strlen(Update)+1);
			pEntrySet->updateEntry(Trans,Sel_Eid,Sel_Update);
			command_counter_increment();
			Sel_Vc.clear();
			char Sel_Data[2][20] = {"789","ab"};
			my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
			pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);
			memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        	memset(&Sel_Di, 0, sizeof(Sel_Di));
			while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
			{
				char* Data = (char*)Sel_Di.getData();
				flag = memcmp(Data,"789abc",6);
			}
			pIndexEntry->endEntrySetScan(pScan);
			
			assert(flag==0);
			if (0 != flag)
			{
				pStorageEngine->closeEntrySet(Trans,pIndexEntry);
				pStorageEngine->closeEntrySet(Trans,pEntrySet);
				paraIndex->SetSuccessFlag(false);
				return false;
			}
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, seq_heapId_4, Seq_indexId_4);
		pStorageEngine->removeEntrySet(Trans,seq_heapId_4);//删除heap		
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;

}

bool test_sequence_index_004_update_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_004_update,pPara,4,104);
	return true;
}

EntrySetID seq_heapId_5;
EntrySetID Seq_indexId_5;
std::vector<EntryID> vTranEntryID;

bool test_sequence_index_005_insert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
    try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_split_heap_321;
		uint32 colid = 2054;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		seq_heapId_5 = pStorageEngine->createEntrySet(Trans,colid);
		EntrySet*pEntrySet = (EntrySet*)pStorageEngine->openEntrySet(Trans,
						EntrySet::OPEN_SHARED,seq_heapId_5);

		//插入数据
		std::vector<std::string> vInserted;
		for (int digstr=122400; digstr<125000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			vInserted.push_back(str);
		}
		vInserted.push_back("123456");
		vInserted.push_back("234567");
		vInserted.push_back("345678");
		vInserted.push_back("456789");
		vInserted.push_back("56789a");
        InsertData(Trans,pEntrySet,vInserted,&vTranEntryID);

		ColumnInfo* co = (ColumnInfo* )malloc(sizeof(ColumnInfo));
		co->keys = 2;
		co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
		co->col_number[0] = 1;
		co->col_number[1] = 2;
		co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
		co->rd_comfunction[0] = my_compare_str;
		co->rd_comfunction[1] = my_compare_str;
		co->split_function = my_split_heap_32;
		uint32 col_id = 2055;
		setColumnInfo(col_id, co);
		Seq_indexId_5= pStorageEngine->createIndexEntrySet(Trans,
							pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,col_id,0,NULL);
		
		IndexEntrySet *pIndexSet = pStorageEngine->openIndexEntrySet(Trans,pEntrySet,
							EntrySet::OPEN_SHARED,Seq_indexId_5,NULL);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_005_insert_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_005_insert,pPara,1,105);
	return true;
}

bool test_sequence_index_005_update_01(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_SHARED,seq_heapId_5);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_SHARED,Seq_indexId_5);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"456","78"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"456789",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		else
		{
			DataItem Sel_Update;
			char Update[] = "789abc";
			Sel_Update.setData((void*)Update);
			Sel_Update.setSize(strlen(Update)+1);
			pEntrySet->updateEntry(Trans,Sel_Eid,Sel_Update);
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_005_update_01_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_005_update_01,pPara,2,105);
	return true;
}

bool test_sequence_index_005_update_02(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_SHARED,seq_heapId_5);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_SHARED,Seq_indexId_5);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"789","ab"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"789abc",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		else
		{
			DataItem Sel_Update;
			char Update[] = "bcd123";
			Sel_Update.setData((void*)Update);
			Sel_Update.setSize(strlen(Update)+1);
			pEntrySet->updateEntry(Trans,Sel_Eid,Sel_Update);
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_005_update_02_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_005_update_02,pPara,3,105);
	return true;
}

bool test_sequence_index_005_update_03(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_SHARED,seq_heapId_5);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_SHARED,Seq_indexId_5);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"bcd","12"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"bcd123",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		else
		{
			DataItem Sel_Update;
			char Update[] = "bcd789";
			Sel_Update.setData((void*)Update);
			Sel_Update.setSize(strlen(Update)+1);
			pEntrySet->updateEntry(Trans,Sel_Eid,Sel_Update);
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_005_update_03_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_005_update_03,pPara,4,105);
	return true;
}

bool test_sequence_index_005_update_04(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_SHARED,seq_heapId_5);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_SHARED,Seq_indexId_5);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"bcd","78"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"bcd789",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		else
		{
			DataItem Sel_Update;
			char Update[] = "bcd234";
			Sel_Update.setData((void*)Update);
			Sel_Update.setSize(strlen(Update)+1);;
			pEntrySet->updateEntry(Trans,Sel_Eid,Sel_Update);
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_005_update_04_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_005_update_04,pPara,5,105);
	return true;
}

bool test_sequence_index_005_update_05(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Trans = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Trans, 
					EntrySet::OPEN_SHARED,seq_heapId_5);
		IndexEntrySet *pIndexEntry = pStorageEngine->openIndexEntrySet(Trans,
					pEntrySet,EntrySet::OPEN_SHARED,Seq_indexId_5);
		
		//begin index scan
		vector<ScanCondition> Sel_Vc;
		char Sel_Data[2][20] = {"bcd","23"};
		int Sel_Col[2] = {1,2};
		ScanCondition::CompareOperation Sel_Co[2] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		PToCompareFunc Sel_Func[2] = {my_compare_str,my_compare_str};
		my_scankey_init(Sel_Data,Sel_Co,Sel_Col,Sel_Func,2,Sel_Vc);
		EntrySetScan* pScan = pIndexEntry->startEntrySetScan(Trans,BaseEntrySet::SnapshotMVCC,Sel_Vc);

		DataItem Sel_Di;
		EntryID Sel_Eid;
		memset(&Sel_Eid, 0, sizeof(Sel_Eid));
        memset(&Sel_Di, 0, sizeof(Sel_Di));
		int flag = -1;
		while(pScan->getNext(EntrySetScan::NEXT_FLAG,Sel_Eid,Sel_Di)==0)
		{
			char* Data = (char*)Sel_Di.getData();
			flag = memcmp(Data,"bcd234",6);
		}
		pIndexEntry->endEntrySetScan(pScan);
		
		assert(flag==0);
		if (0 != flag)
		{
			pStorageEngine->closeEntrySet(Trans,pIndexEntry);
			pStorageEngine->closeEntrySet(Trans,pEntrySet);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		else
		{
			DataItem Sel_Update;
			char Update[] = "bcd345";
			Sel_Update.setData((void*)Update);
			Sel_Update.setSize(strlen(Update)+1);
			pEntrySet->updateEntry(Trans,Sel_Eid,Sel_Update);
		}
		pStorageEngine->closeEntrySet(Trans,pIndexEntry);
		pStorageEngine->closeEntrySet(Trans,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Trans, seq_heapId_5, Seq_indexId_5);
		pStorageEngine->removeEntrySet(Trans,seq_heapId_5);//删除heap	
		paraIndex->SetSuccessFlag(true);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		paraIndex->SetSuccessFlag(false);
		return false;
	}
	return true;
}

bool test_sequence_index_005_update_05_TestFrame()
{
	INITRANID()
			
	ParamIndex* pPara = new ParamIndex();
	REGTASKSEQ(test_sequence_index_005_update_05,pPara,6,105);
	return true;
}

#define CONST_INSERT_DATA "test"
#define CONST_ROW_INSERT_DATA 10
#define CONST_INDEX_SCANKEY "t"

static
bool test_index_ci_step1(ParamBase* para)
{
	/* 线程A中创建表T*/
	ParamIndex* pArgs = (ParamIndex*)para;
	try
	{
		ColumnInfo* pCol = (ColumnInfo* )malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_spilt_heap_1;
		uint32 colid = 10011;
		setColumnInfo(colid, pCol);
		StorageEngine *se = StorageEngine::getStorageEngine();
		Transaction *trans = pArgs->GetTransaction();

		/* 创建表 */
		EntrySetID eid = se->createEntrySet(trans, colid);
		pArgs->m_nHeapEntryID = eid;
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs);

	return true;
}

static
bool test_index_ci_step2(ParamBase* para)
{
	/* 线程B为表T创建索引I */
	ParamIndex* pArgs = (ParamIndex*)para;
	try
	{
		StorageEngine *se = StorageEngine::getStorageEngine();
		Transaction *trans = pArgs->GetTransaction();
		EntrySet *entryset = se->openEntrySet(trans, EntrySet::OPEN_SHARED, pArgs->m_nHeapEntryID);

		ColumnInfo* pCol = (ColumnInfo* )malloc(sizeof(ColumnInfo));
		/* 构建索引 */
		pCol = (ColumnInfo *)malloc(sizeof(ColumnInfo));
		pCol->keys = 1;
		pCol->col_number = (size_t*)malloc(pCol->keys * sizeof(size_t));
		pCol->col_number[0] = 1;
		pCol->rd_comfunction = (CompareCallbacki*)malloc(pCol->keys * sizeof(CompareCallbacki));
		pCol->rd_comfunction[0] = my_compare_str;
		pCol->split_function = my_spilt_heap_1;
		uint32 colid = 10012;
		setColumnInfo(colid, pCol);
		EntrySetID indexeid = se->createIndexEntrySet(trans, entryset, BTREE_INDEX_ENTRY_SET_TYPE, colid);
		pArgs->m_nIndexEntryID = indexeid;
		se->closeEntrySet(trans, entryset);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs);
	return true;
}

static
bool test_index_ci_step3(ParamBase* para)
{
	/* 线程A插入若干数据并使用索引I扫描这些数据 */
	ParamIndex* pArgs = (ParamIndex*)para;
	try
	{
		StorageEngine *se = StorageEngine::getStorageEngine();
		Transaction *trans = pArgs->GetTransaction();
		EntrySet *entryset = se->openEntrySet(trans, EntrySet::OPEN_SHARED, pArgs->m_nHeapEntryID);

		vector<DataItem> v_di;
		for (int i = 0; i < CONST_ROW_INSERT_DATA; ++i)
		{
			v_di.push_back(DataItem((void*)CONST_INSERT_DATA, strlen(CONST_INSERT_DATA) + 1));
		}
		entryset->insertEntries(trans, v_di);
		se->endStatement();

		IndexEntrySet *indexentryset = se->openIndexEntrySet(trans, entryset, EntrySet::OPEN_SHARED, pArgs->m_nIndexEntryID);
		
		/* 扫数据 */
		SearchCondition scanCondition;
		scanCondition.Add(1, LessEqual, CONST_INDEX_SCANKEY, my_compare_str);

		std::vector<std::string> vResult;
		GetIndexScanResults(vResult,trans, indexentryset, scanCondition.Keys());

		se->closeEntrySet(trans, entryset);
		se->closeEntrySet(trans, indexentryset);
		se->removeEntrySet(trans, pArgs->m_nHeapEntryID);

		pArgs->SetSuccessFlag(true);

		if(vResult.size() != CONST_ROW_INSERT_DATA)
			return false;
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs);
	return true;
}

/* 
 * 1--在线程A中创建表T
 * 2--在线程B中为表T创建索引I
 * 3--在线程A中插入若干数据并且使用索引I扫面这些数据
 */
bool test_index_create_insert_dll_000_TestFrame()
{
	INITRANID();

	ParamIndex *pPara = new ParamIndex();
	int wid = GetTransWaitId();
	REGTASKSEQ(test_index_ci_step1, pPara, 1, wid);
	REGTASKSEQ(test_index_ci_step2, pPara, 1, wid);
	REGTASKSEQ(test_index_ci_step3, pPara, 1, wid);
	return true;
}

#undef CONST_INDEX_SCANKEY
#undef CONST_ROW_INSERT_DATA
#undef CONST_INSERT_DATA

