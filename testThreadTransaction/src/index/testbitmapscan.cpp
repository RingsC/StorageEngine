/*
  *author  :hao wang
  *purpose:test bitmap scan
  *date     : 2012-9-20
*/

#include <sstream>
#include "testbitmapscan.h"
#include "StorageEngine.h"
#include "EntryIDBitmap.h"
#include "test_utils/test_utils.h"
#include "TestFrame/TestFrame.h"
#include "TestFrameCommon/TestFrameCommon.h"


/*table id range:3000-4000*/

using namespace std;
using namespace FounderXDB::StorageEngineNS;

extern StorageEngine* pStorageEngine;
extern void my_split_heap_321(RangeDatai &, const char *str, int col, size_t len = 0);
extern void my_split_heap_32(RangeDatai &, const char *str, int col, size_t len = 0);
extern void my_split_heap_321321(RangeDatai &, const char *str, int col, size_t len = 0);
extern void my_split_index_23(RangeDatai &, const char* str, int col, size_t len = 0);
extern int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2);

void my_split_index_33(RangeDatai &rangeData, const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 3;
	}
	return;
}

void my_split_index_13(RangeDatai &rangeData,const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return;
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
	return;
}

bool TestBitmapScanFristCreate(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction * Tran = paraIndex->GetTransaction();
		ColumnInfo *pCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321;
		uint32 ColId = 3000;
		setColumnInfo(ColId,pCol);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Tran,ColId);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		ColumnInfo *pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys*sizeof(size_t));
		pIndexCol->col_number[0] = 1;
		pIndexCol->col_number[1] = 2;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys*sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_heap_32;
		uint32 Col_Id = 3001;
		setColumnInfo(Col_Id,pIndexCol);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Tran,pEntrySet,
						BTREE_INDEX_ENTRY_SET_TYPE,Col_Id);
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

bool TestBitmapScanFristInsert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		DataItem DI;
		EntryID Eid;
		for (int digstr=100000; digstr<300000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			DI.setData((void*)str.c_str());
			DI.setSize(str.length());
			pEntrySet->insertEntry(Tran,Eid,DI);
		}
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

bool TestBitmapScanFristSelect(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		SearchCondition cond;
		cond.Add(1,LessThan,"235",str_compare);
		cond.Add(2,GreaterThan,"56",str_compare);

		IndexEntrySetScan* pScan = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,cond.Keys());
		
		EntryIDBitmap* pBitmap = pScan->getBitmap(Tran);
		EntryIDBitmapIterator* pIter = pBitmap->beginIterate();

		EntryID Eid;
		DataItem DI;
		bool flag = false;
		char* temp;
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
			temp = (char*)DI.getData();
			if (NULL!=temp && strcmp(temp,"234999")==0)
				flag = true;
		}
		if (!flag)
		{
			pBitmap->endIterate(pIter);
			pScan->deleteBitmap(pBitmap);
			pIndexSet->endEntrySetScan(pScan);
			pStorageEngine->closeEntrySet(Tran,pIndexSet);
			pStorageEngine->closeEntrySet(Tran,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
			pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pBitmap->endIterate(pIter);
		pScan->deleteBitmap(pBitmap);
		pIndexSet->endEntrySetScan(pScan);
		pStorageEngine->closeEntrySet(Tran,pIndexSet);
		pStorageEngine->closeEntrySet(Tran,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
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


bool TestBitmapScanFrist()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(TestBitmapScanFristCreate,pPara);
	REGTASK(TestBitmapScanFristInsert,pPara);
	REGTASK(TestBitmapScanFristSelect,pPara);
	return true;
}

bool TestBitmapScanSecondCreate(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction * Tran = paraIndex->GetTransaction();
		ColumnInfo *pCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321321;
		uint32 ColId = 3002;
		setColumnInfo(ColId,pCol);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Tran,ColId);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		ColumnInfo *pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys*sizeof(size_t));
		pIndexCol->col_number[0] = 1;
		pIndexCol->col_number[1] = 4;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys*sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_index_33;
		uint32 Col_Id = 3003;
		setColumnInfo(Col_Id,pIndexCol);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Tran,pEntrySet,
						BTREE_INDEX_ENTRY_SET_TYPE,Col_Id);
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

bool TestBitmapScanSecondInsert(ParamBase* para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);

		DataItem DI;
		EntryID Eid;
		for (uint64 digstr=100550130550; digstr<100550250890; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			DI.setData((void*)str.c_str());
			DI.setSize(str.length());
			pEntrySet->insertEntry(Tran,Eid,DI);
		}
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

bool TestBitmapScanSecondSelect(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		SearchCondition cond;
		cond.Add(1,Equal,"100",my_compare_str);
		cond.Add(2,GreaterThan,"140",my_compare_str);

		IndexEntrySetScan* pScan = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,cond.Keys());
		
		EntryIDBitmap* pBitmap = pScan->getBitmap(Tran);
		EntryIDBitmapIterator* pIter = pBitmap->beginIterate();

		EntryID Eid;
		DataItem DI;
		bool flag = false;
		char* temp;
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
			temp = (char*)DI.getData();
			if (NULL!=temp && strcmp(temp,"100550180890")==0)
				flag = true;
		}
		if (!flag)
		{
			pBitmap->endIterate(pIter);
			pScan->deleteBitmap(pBitmap);
			pIndexSet->endEntrySetScan(pScan);
			pStorageEngine->closeEntrySet(Tran,pIndexSet);
			pStorageEngine->closeEntrySet(Tran,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
			pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pBitmap->endIterate(pIter);
		pScan->deleteBitmap(pBitmap);
		pIndexSet->endEntrySetScan(pScan);
		pStorageEngine->closeEntrySet(Tran,pIndexSet);
		pStorageEngine->closeEntrySet(Tran,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
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


bool TestBitmapScanSecond()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(TestBitmapScanSecondCreate,pPara);
	REGTASK(TestBitmapScanSecondInsert,pPara);
	REGTASK(TestBitmapScanSecondSelect,pPara);
	return true;
}

bool TestBitmapScanThirdCreate(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction * Tran = paraIndex->GetTransaction();
		ColumnInfo *pCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321;
		uint32 ColId = 3004;
		setColumnInfo(ColId,pCol);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Tran,ColId);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		ColumnInfo *pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys*sizeof(size_t));
		pIndexCol->col_number[0] = 1;
		pIndexCol->col_number[1] = 2;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys*sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_heap_32;
		uint32 Col_Id = 3005;
		setColumnInfo(Col_Id,pIndexCol);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Tran,pEntrySet,
						BTREE_INDEX_ENTRY_SET_TYPE,Col_Id);
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

bool TestBitmapScanThirdInsert(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		DataItem DI;
		EntryID Eid;
		for (int digstr=200000; digstr<400000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			DI.setData((void*)str.c_str());
			DI.setSize(str.length());
			pEntrySet->insertEntry(Tran,Eid,DI);
		}
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

bool TestBitmapScanThirdUpdate(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		SearchCondition cond;
		cond.Add(1,LessThan,"335",str_compare);
		cond.Add(2,GreaterThan,"56",str_compare);

		IndexEntrySetScan* pScan = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,cond.Keys());
		
		EntryIDBitmap* pBitmap = pScan->getBitmap(Tran);
		EntryIDBitmapIterator* pIter = pBitmap->beginIterate();

		EntryID Eid;
		DataItem DI;
		DataItem UpdateDI;
		int UpdateTemp = 500000;
		char Data[20];
		char* temp;
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
			temp = (char*)DI.getData();
			sprintf(Data,"%d",(atoi((char*)DI.getData()))+UpdateTemp);
			UpdateDI.setData((void*)Data);
			UpdateDI.setSize(strlen(Data)+1);
			pEntrySet->updateEntry(Tran,Eid,UpdateDI);
		}
		command_counter_increment();
		pBitmap->endIterate(pIter);
		pScan->deleteBitmap(pBitmap);
		pIndexSet->endEntrySetScan(pScan);
		pStorageEngine->closeEntrySet(Tran,pIndexSet);
		pStorageEngine->closeEntrySet(Tran,pEntrySet);
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

bool TestBitmapScanThirdSelect(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		SearchCondition cond;
		cond.Add(1,LessThan,"835",my_compare_str);
		cond.Add(2,GreaterThan,"56",my_compare_str);

		IndexEntrySetScan* pScan = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,cond.Keys());
		
		EntryIDBitmap* pBitmap = pScan->getBitmap(Tran);
		EntryIDBitmapIterator* pIter = pBitmap->beginIterate();

		EntryID Eid;
		DataItem DI;
		bool flag = false;
		char* temp;
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
				temp = (char*)DI.getData();
				if (0!=temp && strcmp(temp,"734578")==0)
					flag = true;
		}
		if (!flag)
		{
			pBitmap->endIterate(pIter);
			pScan->deleteBitmap(pBitmap);
			pIndexSet->endEntrySetScan(pScan);			
			pStorageEngine->closeEntrySet(Tran,pIndexSet);
			pStorageEngine->closeEntrySet(Tran,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
			pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pBitmap->endIterate(pIter);
		pScan->deleteBitmap(pBitmap);
		pIndexSet->endEntrySetScan(pScan);
		pStorageEngine->closeEntrySet(Tran,pIndexSet);
		pStorageEngine->closeEntrySet(Tran,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
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

bool TestBitmapScanThird()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(TestBitmapScanThirdCreate,pPara);
	REGTASK(TestBitmapScanThirdInsert,pPara);
	REGTASK(TestBitmapScanThirdUpdate,pPara);
	REGTASK(TestBitmapScanThirdSelect,pPara);
	return true;
}

bool TestBitmapScanFourthCreate(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction * Tran = paraIndex->GetTransaction();
		ColumnInfo *pCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321;
		uint32 ColId = 3006;
		setColumnInfo(ColId,pCol);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Tran,ColId);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		ColumnInfo *pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys*sizeof(size_t));
		pIndexCol->col_number[0] = 1;
		pIndexCol->col_number[1] = 2;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys*sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_heap_32;
		uint32 Col_Id = 3007;
		setColumnInfo(Col_Id,pIndexCol);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Tran,pEntrySet,
						BTREE_INDEX_ENTRY_SET_TYPE,Col_Id);
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

bool TestBitmapScanFourthInsert(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		DataItem DI;
		EntryID Eid;
		for (int digstr=400000; digstr<600000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			DI.setData((void*)str.c_str());
			DI.setSize(str.length());
			pEntrySet->insertEntry(Tran,Eid,DI);
		}
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

bool TestBitmapScanFourthDelete(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		SearchCondition cond;
		cond.Add(1,LessThan,"535",str_compare);
		cond.Add(2,GreaterThan,"56",str_compare);

		IndexEntrySetScan* pScan = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,cond.Keys());
		
		EntryIDBitmap* pBitmap = pScan->getBitmap(Tran);
		EntryIDBitmapIterator* pIter = pBitmap->beginIterate();
		
		DataItem DI;
		EntryID Eid;
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
			pEntrySet->deleteEntry(Tran,Eid);
		}
		command_counter_increment();
		pBitmap->endIterate(pIter);
		pScan->deleteBitmap(pBitmap);
		pIndexSet->endEntrySetScan(pScan);
		pStorageEngine->closeEntrySet(Tran,pIndexSet);
		pStorageEngine->closeEntrySet(Tran,pEntrySet);
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

bool TestBitmapScanFourthSelect(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		SearchCondition cond;
		cond.Add(1,LessThan,"535",my_compare_str);
		cond.Add(2,GreaterThan,"56",my_compare_str);

		IndexEntrySetScan* pScan = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,cond.Keys());
		
		EntryIDBitmap* pBitmap = pScan->getBitmap(Tran);
		EntryIDBitmapIterator* pIter = pBitmap->beginIterate();

		EntryID Eid;
		DataItem DI;
		bool flag = false;
		char* temp;
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
			temp = (char*)DI.getData();
			if (0 != temp)
				flag = false;
			else
				flag = true;
		}
		if (!flag)
		{
			pBitmap->endIterate(pIter);
			pScan->deleteBitmap(pBitmap);
			pIndexSet->endEntrySetScan(pScan);			
			pStorageEngine->closeEntrySet(Tran,pIndexSet);
			pStorageEngine->closeEntrySet(Tran,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
			pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pBitmap->endIterate(pIter);
		pScan->deleteBitmap(pBitmap);
		pIndexSet->endEntrySetScan(pScan);
		pStorageEngine->closeEntrySet(Tran,pIndexSet);
		pStorageEngine->closeEntrySet(Tran,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
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

bool TestBitmapScanFourth()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(TestBitmapScanFourthCreate,pPara);
	REGTASK(TestBitmapScanFourthInsert,pPara);
	REGTASK(TestBitmapScanFourthDelete,pPara);
	REGTASK(TestBitmapScanFourthSelect,pPara);
	return true;
}

bool TestBitmapScanFifthCreate(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction * Tran = paraIndex->GetTransaction();
		ColumnInfo *pCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321321;
		uint32 ColId = 3008;
		setColumnInfo(ColId,pCol);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Tran,ColId);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		ColumnInfo *pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys*sizeof(size_t));
		pIndexCol->col_number[0] = 3;
		pIndexCol->col_number[1] = 4;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys*sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_index_13;
		uint32 Col_Id = 3009;
		setColumnInfo(Col_Id,pIndexCol);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Tran,pEntrySet,
						BTREE_INDEX_ENTRY_SET_TYPE,Col_Id);
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

bool TestBitmapScanFifthInsert(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);

		DataItem DI;
		EntryID Eid;
		for (uint64 digstr=100551130550; digstr<100553230890; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			DI.setData((void*)str.c_str());
			DI.setSize(str.length());
			pEntrySet->insertEntry(Tran,Eid,DI);
		}
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

bool TestBitmapScanFifthUnion(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		SearchCondition cond;
		cond.Add(1,LessThan,"2",my_compare_str);
		cond.Add(2,LessThan,"180",my_compare_str);
		IndexEntrySetScan* pScanFirst = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,cond.Keys());
		EntryIDBitmap* pBitmapFirst = pScanFirst->getBitmap(Tran);
		
		SearchCondition condSecond;
		condSecond.Add(1,GreaterThan,"2",my_compare_str);
		condSecond.Add(2,GreaterThan,"180",my_compare_str);
		IndexEntrySetScan* pScanSecond = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,condSecond.Keys());
		EntryIDBitmap* pBitmapSecond = pScanSecond->getBitmap(Tran);

		pBitmapFirst->doUnion(*pBitmapSecond);

		EntryIDBitmapIterator* pIter = pBitmapFirst->beginIterate();

		EntryID Eid;
		DataItem DI;
		bool flag = false;
		char* temp;
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
			temp = (char*)DI.getData();
			if (strcmp(temp,"100553182888")==0)
			{
				flag = true;
			}
		}
		
		if (!flag)
		{
			pBitmapFirst->endIterate(pIter);
			pScanFirst->deleteBitmap(pBitmapFirst);
			pIndexSet->endEntrySetScan(pScanFirst);
			pScanSecond->deleteBitmap(pBitmapSecond);
			pIndexSet->endEntrySetScan(pScanSecond);		
			pStorageEngine->closeEntrySet(Tran,pIndexSet);
			pStorageEngine->closeEntrySet(Tran,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
			pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pBitmapFirst->endIterate(pIter);
		pScanFirst->deleteBitmap(pBitmapFirst);
		pIndexSet->endEntrySetScan(pScanFirst);
		pScanSecond->deleteBitmap(pBitmapSecond);
		pIndexSet->endEntrySetScan(pScanSecond);
		pStorageEngine->closeEntrySet(Tran,pIndexSet);
		pStorageEngine->closeEntrySet(Tran,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
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

bool TestBitmapScanFifth()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(TestBitmapScanFifthCreate,pPara);
	REGTASK(TestBitmapScanFifthInsert,pPara);
	REGTASK(TestBitmapScanFifthUnion,pPara);
	return true;
}

bool TestBitmapScanSixthCreate(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction * Tran = paraIndex->GetTransaction();
		ColumnInfo *pCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321321;
		uint32 ColId = 3010;
		setColumnInfo(ColId,pCol);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Tran,ColId);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		ColumnInfo *pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys*sizeof(size_t));
		pIndexCol->col_number[0] = 3;
		pIndexCol->col_number[1] = 4;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys*sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_index_13;
		uint32 Col_Id = 3011;
		setColumnInfo(Col_Id,pIndexCol);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Tran,pEntrySet,
						BTREE_INDEX_ENTRY_SET_TYPE,Col_Id);
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

bool TestBitmapScanSixthInsert(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);

		DataItem DI;
		EntryID Eid;
		for (uint64 digstr=100551130550; digstr<100553230890; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			DI.setData((void*)str.c_str());
			DI.setSize(str.length());
			pEntrySet->insertEntry(Tran,Eid,DI);
		}
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

bool TestBitmapScanSixthIntersect(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		SearchCondition cond;
		cond.Add(1,LessThan,"2",my_compare_str);
		cond.Add(2,LessThan,"180",my_compare_str);
		IndexEntrySetScan* pScanFirst = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,cond.Keys());
		EntryIDBitmap* pBitmapFirst = pScanFirst->getBitmap(Tran);
		
	
		SearchCondition condSecond;
		condSecond.Add(1,GreaterThan,"2",my_compare_str);
		condSecond.Add(2,GreaterThan,"180",my_compare_str);
		IndexEntrySetScan* pScanSecond = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,condSecond.Keys());
		EntryIDBitmap* pBitmapSecond = pScanSecond->getBitmap(Tran);

		pBitmapFirst->doIntersect(*pBitmapSecond);

		EntryIDBitmapIterator* pIter = pBitmapFirst->beginIterate();

		EntryID Eid;
		DataItem DI;
		bool flag = true;
		char* temp;
		while(NO_DATA_FOUND != pIter->getNext(Eid,DI))
		{
			temp = (char*)DI.getData();
			if (NULL != temp)
				flag = false;
		}

		if (!flag)
		{
			pScanFirst->deleteBitmap(pBitmapFirst);
			pIndexSet->endEntrySetScan(pScanFirst);
			pScanSecond->deleteBitmap(pBitmapSecond);
			pIndexSet->endEntrySetScan(pScanSecond);		
			pStorageEngine->closeEntrySet(Tran,pIndexSet);
			pStorageEngine->closeEntrySet(Tran,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
			pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pScanFirst->deleteBitmap(pBitmapFirst);
		pIndexSet->endEntrySetScan(pScanFirst);
		pScanSecond->deleteBitmap(pBitmapSecond);
		pIndexSet->endEntrySetScan(pScanSecond);
		pStorageEngine->closeEntrySet(Tran,pIndexSet);
		pStorageEngine->closeEntrySet(Tran,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
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

bool TestBitmapScanSixth()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(TestBitmapScanSixthCreate,pPara);
	REGTASK(TestBitmapScanSixthInsert,pPara);
	REGTASK(TestBitmapScanSixthIntersect,pPara);
	return true;
}

bool TestBitmapScanSeventhCreate(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction * Tran = paraIndex->GetTransaction();
		ColumnInfo *pCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pCol->keys = 0;
		pCol->col_number = NULL;
		pCol->rd_comfunction = NULL;
		pCol->split_function = my_split_heap_321;
		uint32 ColId = 3012;
		setColumnInfo(ColId,pCol);
		paraIndex->m_nHeapEntryID = pStorageEngine->createEntrySet(Tran,ColId);
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);

		ColumnInfo *pIndexCol = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		pIndexCol->keys = 2;
		pIndexCol->col_number = (size_t*)malloc(pIndexCol->keys*sizeof(size_t));
		pIndexCol->col_number[0] = 2;
		pIndexCol->col_number[1] = 1;
		pIndexCol->rd_comfunction = (CompareCallbacki*)malloc(pIndexCol->keys*sizeof(CompareCallbacki));
		pIndexCol->rd_comfunction[0] = my_compare_str;
		pIndexCol->rd_comfunction[1] = my_compare_str;
		pIndexCol->split_function = my_split_index_23;
		uint32 Col_Id = 3013;
		setColumnInfo(Col_Id,pIndexCol);
		paraIndex->m_nIndexEntryID = pStorageEngine->createIndexEntrySet(Tran,pEntrySet,
						BTREE_INDEX_ENTRY_SET_TYPE,Col_Id);
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

bool TestBitmapScanSeventhInsert(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);

		DataItem DI;
		EntryID Eid;
		for (uint32 digstr=400000; digstr<800000; digstr++)
		{
			string str;
			str.clear();
			stringstream stream;
			stream<<digstr;
			stream>>str;
			DI.setData((void*)str.c_str());
			DI.setSize(str.length());
			pEntrySet->insertEntry(Tran,Eid,DI);
		}
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

bool TestBitmapScanSeventhEmpty(ParamBase * para)
{
	ParamIndex* paraIndex = (ParamIndex*)para;
	printf("\n--------%s--------\n",__FUNCTION__);
	try
	{
		Transaction* Tran = paraIndex->GetTransaction();
		EntrySet* pEntrySet = pStorageEngine->openEntrySet(Tran,EntrySet::OPEN_EXCLUSIVE,paraIndex->m_nHeapEntryID);
		IndexEntrySet* pIndexSet = pStorageEngine->openIndexEntrySet(Tran,
								pEntrySet, EntrySet::OPEN_EXCLUSIVE,
								paraIndex->m_nIndexEntryID);
		SearchCondition cond;
		cond.Add(1,LessThan,"80",my_compare_str);
		cond.Add(2,GreaterThan,"800",my_compare_str);

		IndexEntrySetScan* pScan = (IndexEntrySetScan*)pIndexSet->startEntrySetScan(
								Tran,BaseEntrySet::SnapshotMVCC,cond.Keys());
		EntryIDBitmap* pBitmap = pScan->getBitmap(Tran);

		bool flag = false;
		if (pBitmap->isEmpty())
			flag = true;
		
		if (!flag)
		{
			pScan->deleteBitmap(pBitmap);
			pIndexSet->endEntrySetScan(pScan);			
			pStorageEngine->closeEntrySet(Tran,pIndexSet);
			pStorageEngine->closeEntrySet(Tran,pEntrySet);
			pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
			pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
			paraIndex->SetSuccessFlag(false);
			return false;
		}
		pScan->deleteBitmap(pBitmap);
		pIndexSet->endEntrySetScan(pScan);
		pStorageEngine->closeEntrySet(Tran,pIndexSet);
		pStorageEngine->closeEntrySet(Tran,pEntrySet);
		pStorageEngine->removeIndexEntrySet(Tran,paraIndex->m_nHeapEntryID,paraIndex->m_nIndexEntryID);
		pStorageEngine->removeEntrySet(Tran,paraIndex->m_nHeapEntryID);
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

bool TestBitmapScanSeventh()
{
	INITRANID()

	ParamIndex* pPara = new ParamIndex();
	REGTASK(TestBitmapScanSeventhCreate,pPara);
	REGTASK(TestBitmapScanSeventhInsert,pPara);
	REGTASK(TestBitmapScanSeventhEmpty,pPara);
	return true;
}
