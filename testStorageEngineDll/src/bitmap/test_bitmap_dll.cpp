/**
*@file	:test_bitmap_dll.cpp
*@brief 	:test bitmap
*@author	:WangHao
*@date	:2012-4-27
*/

#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <boost/assign.hpp>
#include "boost/thread.hpp" 
#include "PGSETypes.h"
#include "EntrySet.h"
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "bitmap/test_bitmap_dll.h"
#include "EntryIDBitmap.h"
#include "FDBitmapSet.h"


using namespace FounderXDB::StorageEngineNS;
using namespace std;
extern StorageEngine *pStorageEngine;
EntrySetID heapid;


char onebitmap[] = "testbitmapone";
char twobitmap[] = "testbitmaptwo";


int bitmap_compare(const char *bit1, size_t len1, const char *bit2, size_t len2)
{
	int i = 0;
	while(i < len1 && i < len2)
	{
		if(bit1[i] < bit2[i])
			return -1;
		else if(bit1[i] > bit2[i])
			return 1;
		else i++;
	}
	if(len1 == len2)
		return 0;
	if(len1 > len2)
		return 1;
	return -1;
}

bool TestTidBitmap()
{
	int sta[2] = {0};
	int found = 0;
	EntrySetCollInfo<13>::get();

	
	boost::thread_group tg;
	tg.create_thread(boost::bind(&test_tidbitmap_insert,&sta[0]));
	tg.join_all();
	if (sta[0])
	{
		++found;
	}
	
	tg.create_thread(boost::bind(&test_tidbitmap_test,&sta[1]));
	tg.join_all();
	if (sta[1])
	{
		++found;
	}

	if (2 == found)
		return true;
	return false;
}

void test_tidbitmap_insert(int* i)
{
	Transaction* pTransaction = NULL;
	try
	{
		pStorageEngine->beginThread();
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		pTransaction = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		heapid = pStorageEngine->createEntrySet(pTransaction,EntrySetCollInfo<13>::get());
		EntrySet* heapentry =static_cast<EntrySet* > (pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heapid));

		vector<DataItem> bitmapdata;
		DataItem onebititem;
		onebititem.setData((void*)onebitmap);
		onebititem.setSize(13);
		bitmapdata.push_back(onebititem);
		DataItem twobititem;
		twobititem.setData((void*)twobitmap);
		twobititem.setSize(13);
		bitmapdata.push_back(twobititem);
		
		heapentry->insertEntries(pTransaction,bitmapdata);

		pStorageEngine->closeEntrySet(pTransaction,heapentry);
		pTransaction->commit();
		*i = 1;
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		pTransaction->abort();
		*i = 0;
	}
	delete pTransaction;
	pStorageEngine->endThread();
}

void FetchEntryData(EntryIDBitmap* pbitmap,std::vector<DataItem>& vectData)
{	
	EntryID eid;
	DataItem item;
	EntryIDBitmapIterator * pBitmapiter= pbitmap->beginIterate();
	while(NO_DATA_FOUND !=pBitmapiter->getNext(eid,item))
	{
		vectData.push_back(item);
	}
	pbitmap->endIterate(pBitmapiter);
}
void test_tidbitmap_test(int* i)
{
	Transaction* pTransaction = NULL;
	try
	{
		pStorageEngine->beginThread();
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		pTransaction = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet* heapentry = static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,heapid));//打开表


		TableDroper::indexId = pStorageEngine->createIndexEntrySet(pTransaction,heapentry,BTREE_INDEX_ENTRY_SET_TYPE
			,EntrySetCollInfo<13>::get());
		IndexEntrySet *pindexEntry = pStorageEngine->openIndexEntrySet(pTransaction,heapentry,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);


		char onescan[]="testbitmapone";
		ScanCondition cond1(1, ScanCondition::Equal, (se_uint64)(onescan), 13, bitmap_compare);
        std::vector<ScanCondition> vscans1;
        vscans1.push_back(cond1);
		IndexEntrySetScan* pindexsan1 = (IndexEntrySetScan*) pindexEntry->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vscans1);


		char twoscan[]="testbitmaptwo";
		ScanCondition cond2(1, ScanCondition::Equal, (se_uint64)(twoscan), 13, bitmap_compare);
        std::vector<ScanCondition> vscans2;
        vscans2.push_back(cond2);
		IndexEntrySetScan* pindexsan2 = (IndexEntrySetScan*) pindexEntry->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vscans2);

		EntryIDBitmap* poneBitmap = pindexsan1->getBitmap(pTransaction);
		EntryIDBitmap* ptwoBitmap = pindexsan2->getBitmap(pTransaction);

		if (poneBitmap==NULL ||ptwoBitmap==NULL)
		{
			*i = 0;
		}
		else
		{
			poneBitmap->isEmpty();
			poneBitmap->doUnion(*ptwoBitmap);
			poneBitmap->doIntersect(*ptwoBitmap);

			EntryIDBitmap* poneBitmap2 = pindexsan1->getBitmap(pTransaction);
			EntryIDBitmap* ptwoBitmap2 = pindexsan2->getBitmap(pTransaction);
		
			std::vector<DataItem> vectDataItem1;
			FetchEntryData(poneBitmap2,vectDataItem1);
			std::vector<DataItem> vectDataItem2;
			FetchEntryData(ptwoBitmap2,vectDataItem2);

			
			//本次测试是一幅图片存入一个元组
			if ((memcmp((char*)vectDataItem1[0].getData(),onebitmap,13)==0)&&
				(memcmp((char*)vectDataItem2[0].getData(),twobitmap,13)==0))	
			{
				*i = 1;
			}
			else
			{
				*i = 0;
			}
		}
		pindexEntry->endEntrySetScan(pindexsan1);
		pindexEntry->endEntrySetScan(pindexsan2);
		pStorageEngine->closeEntrySet(pTransaction,pindexEntry);
		pStorageEngine->closeEntrySet(pTransaction,heapentry);
		pTransaction->commit();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		pTransaction->abort();
		*i = 0;
	}
	delete pTransaction;
	pStorageEngine->endThread();
}


//////////////////////////////////////////////////////////////////////////
//new bitmap scan test cases

void bitmap_scan_new_split(RangeDatai& range, const char* pSrc, int colnum, size_t len)
{
	range.start = 0;
	range.len = 0;

	if (1 == colnum)
	{
		range.len = 5;
	}
}

EntrySetID bitmap_scan_create_heap(const std::vector<std::string>& vInsertData)
{
	EntrySetID entrySetId = InvalidEntrySetID;
	StorageEngine* pSE = NULL;
	Transaction* pTrans = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		uint32 heap_colnum = 123456;
		ColumnInfo *pCol = new ColumnInfo;
		pCol->col_number = NULL;
		pCol->keys = 0;
		pCol->rd_comfunction = NULL;
		pCol->split_function = bitmap_scan_new_split;
		setColumnInfo(heap_colnum,pCol);

		entrySetId = pSE->createEntrySet(pTrans,heap_colnum);

		uint32 index_colnum = 1234567;
		ColumnInfo *pCol_index = new ColumnInfo;
		pCol_index->col_number = new size_t[1];
		pCol_index->col_number[0] = 1;
		pCol_index->keys = 1;
		pCol_index->rd_comfunction = new CompareCallbacki[1];
		pCol_index->rd_comfunction[0] = str_compare;
		pCol_index->split_function = bitmap_scan_new_split;
		setColumnInfo(index_colnum,pCol_index);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);
		EntrySetID indexId = pSE->createIndexEntrySet(pTrans,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,index_colnum);

		//insert data
		std::vector<DataItem> vItem;
		for (uint32 i = 0; i < vInsertData.size(); i ++)
		{
			DataItem item;
			item.setData((void*)vInsertData.at(i).c_str());
			item.setSize(vInsertData.at(i).size());
			vItem.push_back(item);			
		}
		pEntrySet->insertEntries(pTrans,vItem);

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		std::cout<<ex.getErrorMsg()<<std::endl;
	}

	return entrySetId;
}
bool bitmap_scan_delete_heap(const EntrySetID& entrySetId)
{
	bool bRet = true;
	Transaction* pTrans = NULL;
	StorageEngine* pSE = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		pSE->removeEntrySet(pTrans,entrySetId);

		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		std::cout<<ex.getErrorMsg()<<std::endl;
		bRet = false;
	}
	
	return bRet;
}
bool bitmap_scan_process(const EntrySetID& entrySetId,const std::vector<std::string>& vInsertData)
{
	bool bRet = true;
	Transaction* pTrans = NULL;
	StorageEngine* pSE = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		EntrySetID index_id = InvalidEntrySetID;
		{
			PGIndinfoData index_info;
			pEntrySet->getIndexInfo(pTrans,index_info);
			if (index_info.index_num > 0)
				index_id = index_info.index_array[0];
		}

		IndexEntrySet* pIndex = pSE->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_SHARED,index_id);

		//bitmap scan
		std::vector<ScanCondition> vCon1;
		ScanCondition con1(1,ScanCondition::LessEqual,se_uint64("10003"),5,str_compare);
		vCon1.push_back(con1);

		IndexEntrySetScan* pScan1 = (IndexEntrySetScan*)pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon1);
		EntryIDBitmap* pBitmap1 = pScan1->getBitmap(pTrans);
		EntryIDBitmapIterator* tbmIterator1 = pBitmap1->beginIterate();
		
		//检查scan1的结果
		std::set<std::string> set1_compare(vInsertData.begin(),vInsertData.begin()+3);
		std::set<std::string> set_query1;

		EntryID eid;
		DataItem item;
		while (NO_DATA_FOUND != tbmIterator1->getNext(eid,item))
		{
			std::string strTmp;
			strTmp = (char*)item.getData();
			set_query1.insert(strTmp);
		}

		pBitmap1->endIterate(tbmIterator1);

		if (set1_compare != set_query1)
			bRet = false;
		
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		pTrans->abort();
		std::cout<<ex.getErrorMsg()<<std::endl;
		bRet = false;
	}

	return bRet;
}
bool test_bitmap_scan_new()
{
	bool bRet = true;
	//insert data
	std::vector<std::string> vInsertData;
	vInsertData.push_back("10001abc");
	vInsertData.push_back("10002abd");
	vInsertData.push_back("10003bcd");
	vInsertData.push_back("10004def");
	vInsertData.push_back("10005efg");
	//create heap
	EntrySetID entrySetId = bitmap_scan_create_heap(vInsertData);

	std::cout<<"create heap and index succeed !\n"<<std::endl;

	//bitmap scan
	if (InvalidEntrySetID != entrySetId)
		bRet = bitmap_scan_process(entrySetId,vInsertData);

	//delete heap
	bRet = bitmap_scan_delete_heap(entrySetId);

	return bRet;
}

bool bitmap_scan_hot_update(const EntrySetID& entrySetId,std::string strUpdate)
{
	bool bRet = true;
	Transaction* pTrans = NULL;
	StorageEngine* pSE = NULL;
	try
	{
		pSE = StorageEngine::getStorageEngine();
		TransactionId xid = InvalidTransactionID;
		pTrans = pSE->getTransaction(xid,Transaction::READ_COMMITTED_ISOLATION_LEVEL);

		EntrySet* pEntrySet = pSE->openEntrySet(pTrans,EntrySet::OPEN_SHARED,entrySetId);

		std::vector<ScanCondition> vCon;
		EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vCon);
		DataItem item;
		EntryID eid;
		while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
		{
			char* pData = (char*)item.getData();
			memcpy(pData+5,strUpdate.c_str(),3);
			item.setData(pData);
			pEntrySet->updateEntry(pTrans,eid,item);
		}

		pEntrySet->endEntrySetScan(pScan);
		pTrans->commit();
	}
	catch(StorageEngineException& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		pTrans->abort();
		bRet = false;
	}

	return bRet;
}
bool test_bitmap_scan_hot_update()
{
	bool bRet = true;
	//insert data
	std::vector<std::string> vInsertData;
	vInsertData.push_back("10001abc");
	vInsertData.push_back("10002abd");
	vInsertData.push_back("10003bcd");
	vInsertData.push_back("10004def");
	vInsertData.push_back("10005efg");
	//create heap
	EntrySetID entrySetId = bitmap_scan_create_heap(vInsertData);

	//hot update
	std::string strUpdate = "ret";
	bitmap_scan_hot_update(entrySetId,strUpdate);
	strUpdate = "bro";
    bitmap_scan_hot_update(entrySetId,strUpdate);
	strUpdate = "app";
	bitmap_scan_hot_update(entrySetId,strUpdate);
	strUpdate = "yuo";
	bitmap_scan_hot_update(entrySetId,strUpdate);
	strUpdate = "fly";
	bitmap_scan_hot_update(entrySetId,strUpdate);

	vInsertData.clear();
	vInsertData.push_back("10001fly");
	vInsertData.push_back("10002fly");
	vInsertData.push_back("10003fly");
	vInsertData.push_back("10004fly");
	vInsertData.push_back("10005fly");

	//bitmap scan
	if (InvalidEntrySetID != entrySetId)
		bRet = bitmap_scan_process(entrySetId,vInsertData);

	//delete heap
	bRet = bitmap_scan_delete_heap(entrySetId);

	return bRet;
}