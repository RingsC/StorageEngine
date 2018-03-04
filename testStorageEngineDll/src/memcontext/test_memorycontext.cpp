/**
* @file test_memorycontext.cpp
* @brief 
* @author 李书淦
* @date 2011-12-27 14:24:48
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/

#include "MemoryContext.h"
#include "memcontext/test_memcontext.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "sequence/utils.h"
#include "utils/utils_dll.h"
#include <boost/assign.hpp>
#include "XdbSequence.h"




using namespace FounderXDB::StorageEngineNS;
bool test_getMemoryContext( void )
{
	MemoryContext *pTransactionContext = NULL;
	const int ALLOC_SIZE = 32;

	try
	{
		if(NULL != pTransactionContext)
		{
		    pTransactionContext = get_txn_mem_cxt();
			pTransactionContext->alloc(ALLOC_SIZE);
			CHECK_BOOL(false);   //should throw an exception
		}
	}
	catch (StorageEngineException& /*ex*/)
	{

	}

	try
	{
		get_new_transaction();
		{
		    pTransactionContext = get_txn_mem_cxt();
			CHECK_BOOL(NULL != pTransactionContext);

			char *pBuffer = (char*)pTransactionContext->alloc(ALLOC_SIZE);
			memset(pBuffer, 'c', ALLOC_SIZE);// now we don't support init-byte for performance reasons
			for (int i = 0; i < ALLOC_SIZE;++i)
			{
				pBuffer[i] == 'c';
			}

			pTransactionContext->deAlloc(pBuffer);
		}
		commit_transaction();
	}
	catch(StorageEngineException& ex)
	{
		user_abort_transaction();
		std::cout<<ex.getErrorMsg()<<std::endl;
	}

	try
	{
        pTransactionContext = get_txn_mem_cxt();
		CHECK_BOOL(pTransactionContext == 0);   //should throw an exception
	}
	catch (StorageEngineException& /*ex*/)
	{
		
	}
	
	return true;
}

bool test_ReAlloc( void )
{
	MemoryContext *pTransactionContext = 0;
	const int ALLOC_SIZE = 32;
	try
	{
		get_new_transaction();
		{
		    pTransactionContext = get_txn_mem_cxt();
			CHECK_BOOL(NULL != pTransactionContext);

			char *pBuffer = (char*)pTransactionContext->alloc(1);
			pBuffer = (char*)pTransactionContext->reAlloc(pBuffer,ALLOC_SIZE);
            memset(pBuffer,'c',ALLOC_SIZE);
			for (int i = 0; i < ALLOC_SIZE;++i)
			{
				pBuffer[i] == 'c';
			}

			pTransactionContext->deAlloc(pBuffer);
		}
		commit_transaction();
	}
	catch(StorageEngineException& ex)
	{
		user_abort_transaction();
		std::cout<<ex.getErrorMsg()<<std::endl;
		return false;
	}
    return true;
}

//extern StorageEngine* pStorageEngine;
//extern void free_colinfo(ColumnInfo &colinfo);
extern int sort_cmp_func(const char *data1, size_t len1, const char *data2, size_t len2);
bool test_operator_new_delete_trans( void )
{
	using namespace boost::assign;
	INTENT("测试new/delete事务对象");
	try
	{
		Transaction *transaction = NULL;
		FXTransactionId tid = 0;
		form_heap_colinfo(heap_colinfo);
		form_index_colinfo(index_colinfo);

		setColumnInfo(1, &heap_colinfo);
		setColumnInfo(2, &index_colinfo);

		//create heap and index
		StorageEngine *storageEngine= StorageEngine::getStorageEngine();

		while(true)
		{		
			printf("getTransaction\n");
			transaction = storageEngine->getTransaction(tid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);//new(cxt) pgtransaction
			printf("createEntrySet\n");
			TableDroper::entryId = storageEngine->createEntrySet(transaction,1);
			printf("openEntrySet\n");
			EntrySet* pEntrySet = (EntrySet*)storageEngine->openEntrySet(transaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);//new(cxt) PGEntrySet
			printf("createIndexEntrySet\n");
			TableDroper::indexId = storageEngine->createIndexEntrySet(transaction,pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,2);
			printf("openIndexEntrySet\n");
			IndexEntrySet *index_entry = (IndexEntrySet *)storageEngine->openIndexEntrySet(transaction,
				pEntrySet,EntrySet::OPEN_EXCLUSIVE,TableDroper::indexId);//new(cxt) PGIndexEntrySet

			//插入一些数据
			std::vector<std::string> vInsertData;
			vInsertData += "1","2","3";
			InsertData(transaction,pEntrySet,vInsertData);
			
			//Sequence
			printf("getSequenceFactory\n");
			SequenceFactory *seqFactory = SequenceFactory::getSequenceFactory();
			printf("createOneSequence\n");
			SeqID seqID = seqFactory->createSequence(1);
			XdbSequence *seq = seqFactory->openSequence(seqID);

			//构造查询
			SearchCondition scanCondition;
			scanCondition.Add(1,LessEqual,"7",str_compare);
			printf("startEntrySetScan\n");
			IndexEntrySetScan *pScan = (IndexEntrySetScan *)(index_entry->startEntrySetScan(transaction,
				BaseEntrySet::SnapshotMVCC,scanCondition.Keys()));//new(cxt) PGIndexEntrySetScan

			const int ROW = 10;	
			printf("createSortEntrySet\n");
			SortEntrySet *pSortEntrySet = storageEngine->createSortEntrySet(transaction, EntrySet::SST_GenericDataTuples, 
				ROW * DATA_LEN, true, sort_cmp_func);//new(cxt) PGSortEntrySet
			printf("createTempEntrySet\n");
			EntrySet *pTempEntrySet = storageEngine->createTempEntrySet(transaction, EntrySet::SST_HeapTuples,
				0, ROW * DATA_LEN, false, true);//new(cxt) PGTempEntrySet
			vector<ScanCondition> v_sc_temp;
			printf("startEntrySetScan  Temp\n");
			EntrySetScan *ess_temp = pTempEntrySet->startEntrySetScan(transaction,BaseEntrySet::SnapshotMVCC,v_sc_temp);//new(cxt) TempEntrySetScan
//			EntrySet *pSort = storageEngine->openEntrySet(transaction,EntrySet::OPEN_EXCLUSIVE,TableDroper::entryId);//new(cxt) PGEntrySet
			vector<ScanCondition> v_sc;
			printf("startEntrySetScan   sort\n");
			EntrySetScan *ess = pSortEntrySet->startEntrySetScan(transaction,BaseEntrySet::SnapshotMVCC,v_sc);//new(cxt) PGSortEntrySetScan

			storageEngine->endStatement();
			printf("delete TempEntrySetScan  \n");
			pTempEntrySet->endEntrySetScan(ess_temp);//delete TempEntrySetScan
			printf("delete sortEntrySetScan  \n");
			pSortEntrySet->endEntrySetScan(ess);//delete PGSortEntrySetScan
			printf("delete PGIndexEntrySetScan  \n");
			index_entry->endEntrySetScan(pScan);//delete PGIndexEntrySetScan
//			storageEngine->closeEntrySet(transaction,pSort);
			storageEngine->closeEntrySet(transaction,index_entry);
			storageEngine->closeEntrySet(transaction,pEntrySet);
			storageEngine->removeEntrySet(transaction,TableDroper::entryId);
			
			//printf("delete PGTempEntrySet  \n");
			//delete pTempEntrySet;// delete PGTempEntrySet
			//printf("delete PGSortEntrySet  \n");
			//delete pSortEntrySet;//delete PGSortEntrySet
			//printf("delete PGIndexEntrySet  \n");
			//delete index_entry;//delete PGIndexEntrySet
			//printf("delete PGEntrySet  \n");
			//delete pEntrySet;//delete PGEntrySet 事务提交前显示delete掉在事务中new出来的对象
			
			transaction->commit();
			printf("delete transaction  \n");
			delete transaction;//delete pgtransaction
		}	
		//释放colunminfo中malloc空间。
		free_colinfo(heap_colinfo);
		free_colinfo(index_colinfo);
	}
	catch(StorageEngineException& ex)
	{
		user_abort_transaction();
		std::cout<<ex.getErrorMsg()<<std::endl;
		return false;
	}
	return true;
}