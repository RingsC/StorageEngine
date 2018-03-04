#include "index/test_split_funcs_dll.h"
#include "utils/utils_dll.h"

void split_test_dll_heap_2_33(RangeDatai& range,const char* pSrc,int col,size_t len = 0)
{
	range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 6*(col - 1), 2);
	range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 6*(col - 1) + 2, 2);
	range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 6*(col - 1) + 4, 2);
}
void split_test_dll_heap_2_13(RangeDatai& range,const char* pSrc,int col,size_t len = 0)
{
	if (1 == col)
	{
		range.start = 0;
		range.len = 2;
	}
	if (2 == col)
	{
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 6*(col - 1), 2);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 6*(col - 1) + 2, 2);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 6*(col - 1) + 4, 2);
	}
}
void split_test_dll_heap_3_313(RangeDatai& range,const char* pSrc,int col,size_t len = 0)
{
	if (1 == col)
	{
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc, 1);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 1, 1);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 2, 1);
	}
    if (2 == col)
	{
		range.start = 5;
		range.len = 2;
	}
	if (3 == col)
	{
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 9, 1);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 9 + 1, 1);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 9 + 2, 1);
	}
}

void split_test_dll_heap_2_31(RangeDatai& range,const char* pSrc,int col,size_t len = 0)
{
	if (1 == col)
	{
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc, 2);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 2, 2);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 4, 2);
	}
	if (2 == col)
	{
		range.start = 6;
		range.len = 2;
	}
}
void split_test_dll_heap_no_index(RangeDatai& range,const char* pSrc,int col,size_t len = 0)
{
	std::string strBlack = "123456abcdef";
	if (0 == memcmp(strBlack.c_str(),pSrc,len))
	{
		range.len = -1;
	}
	else
	{
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 6*(col - 1), 2);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 6*(col - 1) + 2, 2);
		range.addAddrLen(StorageEngine::getStorageEngine()->GetCurrentTransaction(),pSrc + 6*(col - 1) + 4, 2);
	}
}

void split_test_dll_index_len4(RangeDatai& range,const char* pSrc,int col,size_t len = 0)
{
	if (NULL == pSrc)
	{
		range.start = 0;
		range.len = 0;
	}
	else
	{
		range.start = (col-1)*4;
		range.len = 4;
	}
}

void SetMyColInfoHeap(const uint32& colid,Spliti func_split)
{
	ColumnInfo* colInfo = new ColumnInfo;
	colInfo->keys = 0;
	colInfo->col_number = NULL;
	colInfo->rd_comfunction = NULL;
	colInfo->split_function = func_split;

	setColumnInfo(colid,colInfo);
}
void SetMyColInfoIndex(const uint32& colid)
{
	ColumnInfo* colInfo = new ColumnInfo;
	colInfo->keys = 2;
    colInfo->col_number = new size_t[colInfo->keys];
	colInfo->col_number[0] = 1;
	colInfo->col_number[1] = 2;
	colInfo->rd_comfunction = new CompareCallbacki[colInfo->keys];
	colInfo->rd_comfunction[0] = str_compare;
	colInfo->rd_comfunction[1] = str_compare;
	colInfo->split_function = split_test_dll_index_len4;

	setColumnInfo(colid,colInfo);
}
void SetMyColInfoIndex_3(const uint32& colid)
{
	ColumnInfo* colInfo = new ColumnInfo;
	colInfo->keys = 3;
	colInfo->col_number = new size_t[colInfo->keys];
	colInfo->col_number[0] = 1;
	colInfo->col_number[1] = 2;
	colInfo->col_number[2] = 3;
	colInfo->rd_comfunction = new CompareCallbacki[colInfo->keys];
	colInfo->rd_comfunction[0] = str_compare;
	colInfo->rd_comfunction[1] = str_compare;
	colInfo->rd_comfunction[2] = str_compare;
	colInfo->split_function = split_test_dll_index_len4;

	setColumnInfo(colid,colInfo);
}

uint32 GetColId()
{
	static uint32 unBase = 10000;

	return (++ unBase);
}
bool CheckIndexCorrect(EntrySetID heapId, EntrySetID indexId, const std::vector<ScanCondition>& vCon, const std::set<std::string>& setCmp)
{
	bool bRet = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapId);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,indexId);

		EntrySetScan* pScan = pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);

		std::set<std::string> setScan;
		EntryID eid_scan;
		DataItem item_scan;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid_scan,item_scan))
		{
			std::string strData = (char*)item_scan.getData();
			setScan.insert(strData);
		}
		pIndex->endEntrySetScan(pScan);

		pTrans->commit();

		bRet = (setCmp == setScan);
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}
	return bRet;
}

bool test_split_func_2_13()
{
	bool bRet1 = true;
	bool bRet2 = true;
	bool bRet3 = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		//create heap and index,insert data
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		const uint32 colid_heap = GetColId();
		SetMyColInfoHeap(colid_heap,split_test_dll_heap_2_13);
		const uint32 colid_index = GetColId();
		SetMyColInfoIndex(colid_index);
		EntrySetID heapId = pSE->createEntrySet(pTrans,colid_heap);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapId);
		EntrySetID indexId = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,colid_index);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,indexId);

		std::string strData = "123456abcdef";
		std::vector<std::string> vData;
		vData.push_back(strData);
		vData.push_back("abcdef123456");
		vData.push_back("EFRTYU123456");
		vData.push_back("abcdefASDFGH");
		vData.push_back("!@#$%^&*()00");

		std::vector<DataItem> vItem;
		for (std::vector<std::string>::iterator it = vData.begin(); it != vData.end(); it ++)
		{
			DataItem item;
			item.setData((void*)it->c_str());
			item.setSize(it->size());

			vItem.push_back(item);
		}
		
		pEntrySet->insertEntries(pTrans, vItem);
		pTrans->commit();

		std::vector<ScanCondition> vCon1;
		ScanCondition con1(1,ScanCondition::Equal,se_uint64("12ab"),4,str_compare);
		vCon1.push_back(con1);
		std::set<std::string> set1;
		set1.insert(strData);
		bRet1 = CheckIndexCorrect(heapId, indexId,vCon1, set1);

		std::vector<ScanCondition> vCon2;
		ScanCondition con2(1,ScanCondition::Equal,se_uint64("12cd"),4,str_compare);
		vCon2.push_back(con2);
		std::set<std::string> set2;
		set2.insert(strData);
		bRet2 = CheckIndexCorrect(heapId, indexId, vCon2, set2);

		std::vector<ScanCondition> vCon3;
		ScanCondition con3(1,ScanCondition::Equal,se_uint64("12ef"),4,str_compare);
		vCon3.push_back(con3);
		std::set<std::string> set3;
		set3.insert(strData);
		bRet3 = CheckIndexCorrect(heapId, indexId, vCon3, set3);

		xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pSE->removeEntrySet(pTrans,heapId);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}
	return bRet1&&bRet2&&bRet3;
}
bool test_split_func_2_31()
{
	bool bRet1 = true;
	bool bRet2 = true;
	bool bRet3 = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		//create heap and index,insert data
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		const uint32 colid_heap = GetColId();
		SetMyColInfoHeap(colid_heap,split_test_dll_heap_2_31);
		const uint32 colid_index = GetColId();
		SetMyColInfoIndex(colid_index);
		EntrySetID heapId = pSE->createEntrySet(pTrans,colid_heap);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapId);
		EntrySetID indexId = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,colid_index);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,indexId);

		std::string strData = "123456abcdef";
		std::vector<std::string> vData;
		vData.push_back(strData);
		vData.push_back("abcdef123456");
		vData.push_back("EFRTYU123456");
		vData.push_back("abcdefASDFGH");
		vData.push_back("!@#$%^&*()00");

		std::vector<DataItem> vItem;
		for (std::vector<std::string>::iterator it = vData.begin(); it != vData.end(); it ++)
		{
			DataItem item;
			item.setData((void*)it->c_str());
			item.setSize(it->size());

			vItem.push_back(item);
		}

		pEntrySet->insertEntries(pTrans, vItem);
		pTrans->commit();

		std::vector<ScanCondition> vCon1;
		ScanCondition con1(1,ScanCondition::Equal,se_uint64("12ab"),4,str_compare);
		vCon1.push_back(con1);
		std::set<std::string> set1;
		set1.insert(strData);
		bRet1 = CheckIndexCorrect(heapId, indexId,vCon1, set1);

		std::vector<ScanCondition> vCon2;
		ScanCondition con2(1,ScanCondition::Equal,se_uint64("34ab"),4,str_compare);
		vCon2.push_back(con2);
		std::set<std::string> set2;
		set2.insert(strData);
		bRet2 = CheckIndexCorrect(heapId, indexId, vCon2, set2);

		std::vector<ScanCondition> vCon3;
		ScanCondition con3(1,ScanCondition::Equal,se_uint64("56ab"),4,str_compare);
		vCon3.push_back(con3);
		std::set<std::string> set3;
		set3.insert(strData);
		bRet3 = CheckIndexCorrect(heapId, indexId, vCon3, set3);

		xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pSE->removeEntrySet(pTrans,heapId);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}
	return bRet1&&bRet2&&bRet3;
}


bool test_split_func_2_33()
{
	bool bRet1 = true;
	bool bRet2 = true;
	bool bRet3 = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		//create heap and index,insert data
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		const uint32 colid_heap = GetColId();
		SetMyColInfoHeap(colid_heap,split_test_dll_heap_2_33);
		const uint32 colid_index = GetColId();
		SetMyColInfoIndex(colid_index);
		EntrySetID heapId = pSE->createEntrySet(pTrans,colid_heap);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapId);
		EntrySetID indexId = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,colid_index);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,indexId);

		std::string strData = "123456abcdef";
		std::vector<std::string> vData;
		vData.push_back(strData);
		vData.push_back("abcdef123456");
		vData.push_back("EFRTYU123456");
		vData.push_back("abcdefASDFGH");
		vData.push_back("!@#$%^&*()00");

		std::vector<DataItem> vItem;
		for (std::vector<std::string>::iterator it = vData.begin(); it != vData.end(); it ++)
		{
			DataItem item;
			item.setData((void*)it->c_str());
			item.setSize(it->size());

			vItem.push_back(item);
		}

		pEntrySet->insertEntries(pTrans, vItem);
		pTrans->commit();

		std::vector<ScanCondition> vCon1;
		ScanCondition con1(1,ScanCondition::Equal,se_uint64("12ab"),4,str_compare);
		vCon1.push_back(con1);
		std::set<std::string> set1;
		set1.insert(strData);
		bRet1 = CheckIndexCorrect(heapId, indexId,vCon1, set1);

		std::vector<ScanCondition> vCon2;
		ScanCondition con2(1,ScanCondition::Equal,se_uint64("34cd"),4,str_compare);
		vCon2.push_back(con2);
		std::set<std::string> set2;
		set2.insert(strData);
		bRet2 = CheckIndexCorrect(heapId, indexId, vCon2, set2);

		std::vector<ScanCondition> vCon3;
		ScanCondition con3(1,ScanCondition::Equal,se_uint64("56ef"),4,str_compare);
		vCon3.push_back(con3);
		std::set<std::string> set3;
		set3.insert(strData);
		bRet3 = CheckIndexCorrect(heapId, indexId, vCon3, set3);

		xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pSE->removeEntrySet(pTrans,heapId);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}
	return bRet1&&bRet2&&bRet3;
}
bool test_split_func_range_len()
{
	bool bRet1 = true;
	bool bRet2 = true;
	bool bRet3 = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		//create heap and index,insert data
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		const uint32 colid_heap = GetColId();
		SetMyColInfoHeap(colid_heap,split_test_dll_heap_no_index);
		const uint32 colid_index = GetColId();
		SetMyColInfoIndex(colid_index);
		EntrySetID heapId = pSE->createEntrySet(pTrans,colid_heap);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapId);
		EntrySetID indexId = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,colid_index);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,indexId);

		std::string strData = "123456abcdef";
		std::vector<std::string> vData;
		vData.push_back(strData);
		vData.push_back("abcdef123456");
		vData.push_back("EFRTYU123456");
		vData.push_back("abcdefASDFGH");
		vData.push_back("!@#$%^&*()00");

		std::vector<DataItem> vItem;
		for (std::vector<std::string>::iterator it = vData.begin(); it != vData.end(); it ++)
		{
			DataItem item;
			item.setData((void*)it->c_str());
			item.setSize(it->size());

			vItem.push_back(item);
		}

		pEntrySet->insertEntries(pTrans, vItem);
		pTrans->commit();

		std::vector<ScanCondition> vCon1;
		ScanCondition con1(1,ScanCondition::Equal,se_uint64("12ab"),4,str_compare);
		vCon1.push_back(con1);
		std::set<std::string> set1;
		set1.insert(strData);
		bRet1 = CheckIndexCorrect(heapId, indexId,vCon1, set1);

		std::vector<ScanCondition> vCon2;
		ScanCondition con2(1,ScanCondition::Equal,se_uint64("34cd"),4,str_compare);
		vCon2.push_back(con2);
		std::set<std::string> set2;
		set2.insert(strData);
		bRet2 = CheckIndexCorrect(heapId, indexId, vCon2, set2);

		std::vector<ScanCondition> vCon3;
		ScanCondition con3(1,ScanCondition::Equal,se_uint64("56ef"),4,str_compare);
		vCon3.push_back(con3);
		std::set<std::string> set3;
		set3.insert(strData);
		bRet3 = CheckIndexCorrect(heapId, indexId, vCon3, set3);

		xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pSE->removeEntrySet(pTrans,heapId);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}
	return !bRet1&&!bRet2&&!bRet3;
}
bool test_split_func_3_111_2_111()
{
	bool bRet1 = true;
	bool bRet2 = true;
	bool bRet3 = true;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();

		//create heap and index,insert data
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		const uint32 colid_heap = GetColId();
		SetMyColInfoHeap(colid_heap,split_test_dll_heap_3_313);
		const uint32 colid_index = GetColId();
		SetMyColInfoIndex_3(colid_index);
		EntrySetID heapId = pSE->createEntrySet(pTrans,colid_heap);
		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,heapId);
		EntrySetID indexId = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,colid_index);
		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,indexId);

		std::string strData = "123abcdefABC";
		std::vector<std::string> vData;
		vData.push_back(strData);
		vData.push_back("abcdef123456");
		vData.push_back("EFRTYU123456");
		vData.push_back("abcdefASDFGH");
		vData.push_back("!@#$%^&*()00");

		std::vector<DataItem> vItem;
		for (std::vector<std::string>::iterator it = vData.begin(); it != vData.end(); it ++)
		{
			DataItem item;
			item.setData((void*)it->c_str());
			item.setSize(it->size());

			vItem.push_back(item);
		}

		pEntrySet->insertEntries(pTrans, vItem);
		pTrans->commit();

		std::vector<ScanCondition> vCon1;
		ScanCondition con1(1,ScanCondition::Equal,se_uint64("1cdA"),4,str_compare);
		vCon1.push_back(con1);
		std::set<std::string> set1;
		set1.insert(strData);
		bRet1 = CheckIndexCorrect(heapId, indexId,vCon1, set1);

		std::vector<ScanCondition> vCon2;
		ScanCondition con2(1,ScanCondition::Equal,se_uint64("2cdB"),4,str_compare);
		vCon2.push_back(con2);
		std::set<std::string> set2;
		set2.insert(strData);
		bRet2 = CheckIndexCorrect(heapId, indexId, vCon2, set2);

		std::vector<ScanCondition> vCon3;
		ScanCondition con3(1,ScanCondition::Equal,se_uint64("3cdC"),4,str_compare);
		vCon3.push_back(con3);
		std::set<std::string> set3;
		set3.insert(strData);
		bRet3 = CheckIndexCorrect(heapId, indexId, vCon3, set3);

		xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pSE->removeEntrySet(pTrans,heapId);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		return false;
	}
	return bRet1&&bRet2&&bRet3;
}
