#include "boost/thread.hpp" 

#include "pg/pg_insert.h"
#include "EntrySet.h"
#include "StorageEngine.h"
#include "Transaction.h"
#include "DataItem.h"
#include "utils/performance_timer.h"


#include <iostream>
using std::cout;
using std::endl;


using boost::thread_group;
using boost::thread;
using boost::bind;

using namespace FounderXDB::StorageEngineNS;

namespace FounderXDB{
namespace PgBdbPerformanceComp
{
	ColumnInfo colinfo(false);
	ColumnInfo indexColinfo(false);
	
	int PgStorage::num = 0;
	void PgStorage::batchInsert(void *data, uint32 len, uint32 nums)
	{
		EntrySet *table = NULL;
		IndexEntrySet *index_table = NULL;
		Transaction *pTransaction = NULL;

		DataItem value(data, len);
		std::vector<DataItem> datas;
		for(uint32 i = 0; i < nums; ++i)
		{
			datas.push_back(value);
		}
		size_t size = datas.size();
		
		string msg("PgBatchInsert");
		if(m_hasIndex)
		msg += "WithIndex";

		try {
			PerformanceTimer timer(msg.c_str(), m_count, len);
			pTransaction = getNewTransaction();
			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);


			table->insertEntries(pTransaction, datas);

			command_counter_increment();
			m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			m_pStorageEngine->closeEntrySet(pTransaction, table);
			pTransaction->commit();

			
		}catch (StorageEngineException &)
		{
			if (NULL == table)
			{
				pTransaction->abort();
			} else if(NULL == index_table){  //insert exception
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				pTransaction->abort();
			}else{
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				m_pStorageEngine->closeEntrySet(pTransaction, table);
				pTransaction->abort();
			}

			throw StorageEngineException(insertError, "insert entry failed");

		} //catch

		return;	
	}

	void PgStorage::insertFirstIndexLast(void *data, uint32 len)
	{
		EntrySet *table = NULL;
		IndexEntrySet *index_table = NULL;
		Transaction *pTransaction = NULL;
		DataItem content;
		formDataItem(content, data, len);


		formIndexColinfo(indexColinfo);
		setColumnInfo(3, &indexColinfo);


		string msg("PgInsertFirstIndexLast");
		

		try {
			PerformanceTimer timer(msg.c_str(),m_count, len);
			pTransaction = getNewTransaction();
			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);


			EntryID eid = {0};

			for (uint32 i = 0; i < m_count; i++) {
				table->insertEntry(pTransaction, eid, content); 
			}

			m_index_tableId = m_pStorageEngine->createIndexEntrySet(pTransaction, table, BTREE_INDEX_ENTRY_SET_TYPE, 3);
						
			m_pStorageEngine->closeEntrySet(pTransaction, table);
			pTransaction->commit();

			
		}catch (StorageEngineException &)
		{
			if (NULL == table)
			{
				pTransaction->abort();
			} 

			if(table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, table);
			}
			throw StorageEngineException(insertError, "insert entry failed");

		} //catch

		return;		
	}

	void PgStorage::insert(void *data, uint32 len, void *lessData, uint32 lessLen)
	{

		EntrySet *table = NULL;
		IndexEntrySet *index_table = NULL;
		Transaction *pTransaction = NULL;
		DataItem content;
		formDataItem(content, data, len);

		DataItem value;
		formDataItem(value, lessData, lessLen);

		
		string msg("PgInsert");
		if(m_hasIndex)
		msg += "WithIndex";

		try {
			PerformanceTimer timer(msg.c_str(),m_count, len);
			pTransaction = getNewTransaction();
			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);


			EntryID eid = {0};

			for (uint32 i = 0; i < m_count; i++) {
				table->insertEntry(pTransaction, eid, content); 
			}

			Transaction *pTransaction2 = getNewTransaction();
			EntrySet *table2 = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);
			for(uint32 i = 0; i < m_lessNums; ++i){
				table2->insertEntry(pTransaction, eid, value);
			}
			command_counter_increment();
			m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			m_pStorageEngine->closeEntrySet(pTransaction, table);
			pTransaction->abort();
			pTransaction2->commit();
			//pTransaction->abort();

			
		}catch (StorageEngineException &)
		{
			if (NULL == table)
			{
				pTransaction->abort();
			} else if(NULL == index_table){  //insert exception
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				pTransaction->abort();
			}else{
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				m_pStorageEngine->closeEntrySet(pTransaction, table);
				pTransaction->abort();
			}

			throw StorageEngineException(insertError, "insert entry failed");

		} //catch

		return;
	}

	void PgStorage::update(void *data, uint32 len)
	{
		EntrySet *table = NULL;
		IndexEntrySet *index_table = NULL;
		Transaction *pTransaction = NULL;
		try 
		{
			pTransaction = getNewTransaction();
			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);
			index_table = m_pStorageEngine->openIndexEntrySet(pTransaction, table, EntrySet::OPEN_EXCLUSIVE , m_index_tableId);
			command_counter_increment();

			DataItem updataData;
			formDataItem(updataData, data, len);
			
			if(!m_hasIndex){
				PerformanceTimer timer("PgUpdate", m_count, len);	
				EntrySetScan *entry_scan = table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
				
				DataItem getData;
				EntryID tid = {0};
				while (true)
				{
					int flag;
					flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getData);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}
					
					size_t getSize = getData.getSize();
					uint32 size = getSize > len ? len : getSize;
					if(0 == memcmp(data, getData.getData(), size))
					{
						table->updateEntry(pTransaction, tid, updataData);
					}
				}
				table->endEntrySetScan(entry_scan);
			}else{

				PerformanceTimer timer("PgUpdateWithIndex", m_count, len);
				ScanCondition cond(1, ScanCondition::Equal, (se_uint64)data, keyLen, heap_char_cmp);
				std::vector<ScanCondition> vscans;
				vscans.push_back(cond);
				EntrySetScan *pscan = NULL;
				pscan = index_table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, vscans);

				DataItem getData;
				EntryID tid = {0};
				while (true)
				{
					int flag;
					flag=pscan->getNext(EntrySetScan::NEXT_FLAG,tid,getData);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}

					table->updateEntry(pTransaction, tid, updataData);
				}
			} //else
			command_counter_increment();
			m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			m_pStorageEngine->closeEntrySet(pTransaction, table);
			pTransaction->commit();
			//pTransaction->abort();
		}catch (StorageEngineException &)
		{
			if (NULL == table)
			{
				pTransaction->abort();
			} else if(NULL == index_table){  //insert exception
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				pTransaction->abort();
			}else{
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				m_pStorageEngine->closeEntrySet(pTransaction, table);
				pTransaction->abort();
			}

			throw StorageEngineException(insertError, "update entry failed");

		}
		return;	
	}

	void PgStorage::scan(void *data, uint32 len)
	{

		EntrySet *table = NULL;
		IndexEntrySet *index_table = NULL;
		Transaction *pTransaction = NULL;
		try 
		{
			pTransaction = getNewTransaction();
			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);
			index_table = m_pStorageEngine->openIndexEntrySet(pTransaction, table, EntrySet::OPEN_EXCLUSIVE , m_index_tableId);
			

			DataItem updataData;
			formDataItem(updataData, data, len);
			
			if(!m_hasIndex){
				PerformanceTimer timer("PgScan", m_count, len);	
				EntrySetScan *entry_scan = table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
				
				DataItem getData;
				EntryID tid = {0};

				while(entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getData) == 0)
				{
					uint32 size = getData.getSize();
					uint32 cmpSize = size > len ? len : size;

					if(memcmp(data, getData.getData(), cmpSize) == 0)
					{
						continue;
					}
				}

	
				table->endEntrySetScan(entry_scan);
			}else{

				PerformanceTimer timer("PgScanWithIndex", m_count, len);
				ScanCondition cond(1, ScanCondition::Equal, (se_uint64)data, keyLen, heap_char_cmp);
				std::vector<ScanCondition> vscans;
				vscans.push_back(cond);
				EntrySetScan *pscan = NULL;
				pscan = index_table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, vscans);

				DataItem getData;
				EntryID tid = {0};
				while(pscan->getNext(EntrySetScan::NEXT_FLAG,tid,getData) == 0);
				index_table->endEntrySetScan(pscan);
#if 0
					char *p = (char *)getData.getData();
					cout << p << endl;
					char *p0 = (char *)data;
					cout << p0;
#endif 
				
			} //else
			command_counter_increment();
			m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			m_pStorageEngine->closeEntrySet(pTransaction, table);
			pTransaction->commit();
		}catch (StorageEngineException &)
		{
			if (NULL == table)
			{
				pTransaction->abort();
			} else if(NULL == index_table){  //insert exception
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				pTransaction->abort();
			}else{
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				m_pStorageEngine->closeEntrySet(pTransaction, table);
				pTransaction->abort();
			}

			throw StorageEngineException(insertError, "scan failed");

		}
		return;	

	}

//m_table->deleteEntry(pTransaction,tid);
	void PgStorage::del(void *data, uint32 len)
	{
		EntrySet *table = NULL;
		IndexEntrySet *index_table = NULL;
		Transaction *pTransaction = NULL;
		try 
		{
			pTransaction = getNewTransaction();
			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);
			index_table = m_pStorageEngine->openIndexEntrySet(pTransaction, table, EntrySet::OPEN_EXCLUSIVE , m_index_tableId);
			command_counter_increment();

			DataItem updataData;
			formDataItem(updataData, data, len);
			
			if(!m_hasIndex){
				PerformanceTimer timer("PgDel", m_count, len);	
				EntrySetScan *entry_scan = table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
				
				DataItem getData;
				EntryID tid = {0};
				while (true)
				{
					int flag;
					flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getData);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}
		
					table->deleteEntry(pTransaction,tid);

				}
				table->endEntrySetScan(entry_scan);
			}else{

				PerformanceTimer timer("PgDelWithIndex", m_count, len);
				ScanCondition cond(1, ScanCondition::Equal, (se_uint64)data, keyLen, heap_char_cmp);
				std::vector<ScanCondition> vscans;
				vscans.push_back(cond);
				EntrySetScan *pscan = NULL;
				pscan = index_table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, vscans);

				DataItem getData;
				EntryID tid = {0};
				while (true)
				{
					int flag;
					flag=pscan->getNext(EntrySetScan::NEXT_FLAG,tid,getData);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}

					table->deleteEntry(pTransaction,tid);
				}
			} //else
			command_counter_increment();
			m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			m_pStorageEngine->closeEntrySet(pTransaction, table);
			pTransaction->commit();
		}catch (StorageEngineException &)
		{
			if (NULL == table)
			{
				pTransaction->abort();
			} else if(NULL == index_table){  //insert exception
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				pTransaction->abort();
			}else{
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				m_pStorageEngine->closeEntrySet(pTransaction, table);
				pTransaction->abort();
			}

			throw StorageEngineException(insertError, "del entry failed");

		}
		return;	
	}

	void PgStorage::threadBatchInsert(void *data, uint32 len, uint32 nums)
	{
		thread_group tg;
		string msg("PgThreadBatchInsert");
		if(m_hasIndex){
			msg += "WithIndex";
		}

		//m_pStorageEngine->beginThread();
		//Transaction* pTransaction = NULL;
		//pTransaction = getNewTransaction();	

		double *time = new double[m_thread_nums];
		memset((void *)time, 0, sizeof(double) * m_thread_nums);

		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&PgStorage::threadBatchInsertFunc, this, data, len, nums, (time + i)));
		}
		tg.join_all();

		///calculate time
		for (uint32 i = 0; i < m_thread_nums; ++i)
		{
			*time += *(time + i);
		}
		*time = *time / m_thread_nums;	
		writeResToFile(msg.c_str(),*time, len, m_thread_nums, m_count);
		delete [] time;
		time = NULL;

		//pTransaction->commit();
		
		return ;
	
	}


	void PgStorage::threadInsertFirstIndexLast(void *data, uint32 len)
	{
		thread_group tg;
		string msg("PgThreadInsertFirstIndexLast");


		double *time = new double[m_thread_nums];
		memset((void *)time, 0, sizeof(double) * m_thread_nums);

		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&PgStorage::threadInsertFirstIndexLastFunc, this, data, len, (time + i)));
		}
		tg.join_all();

		///calculate time
		for (uint32 i = 0; i < m_thread_nums; ++i)
		{
			*time += *(time + i);
		}
		*time = *time / m_thread_nums;	
		writeResToFile(msg.c_str(),*time, len, m_thread_nums, m_count);
		delete [] time;
		time = NULL;

		//pTransaction->commit();
		
		return ;	
	}

	void PgStorage::threadInsert(void *data, uint32 len, void *lessData, uint32 lessLen)
	{
		thread_group tg;
		string msg("PgThreadInsert");
		if(m_hasIndex){
			msg += "WithIndex";
		}

		//m_pStorageEngine->beginThread();
		//Transaction* pTransaction = NULL;
		//pTransaction = getNewTransaction();	

		double *time = new double[m_thread_nums];
		memset((void *)time, 0, sizeof(double) * m_thread_nums);

		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&PgStorage::threadInsertFunc, this, data, len, lessData, lessLen, (time + i)));
		}
		tg.join_all();

		///calculate time
		for (uint32 i = 0; i < m_thread_nums; ++i)
		{
			*time += *(time + i);
		}
		*time = *time / m_thread_nums;	
		writeResToFile(msg.c_str(),*time, len, m_thread_nums, m_count);
		delete [] time;
		time = NULL;

		//pTransaction->commit();
		
		return ;

	}


	void PgStorage::threadUpdate(void *data, uint32 len)
	{
		thread_group tg;
		string msg("PgThreadUpdate");
		if(m_hasIndex){
			msg += "WithIndex";
		}


		double *time = new double[m_thread_nums];
		memset((void *)time, 0, sizeof(double) * m_thread_nums);

		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&PgStorage::threadUpdateFunc, this, data, len, (time + i )));
		}
		tg.join_all();

		for (uint32 i = 0; i < m_thread_nums; ++i)
		{
			*time += *(time + i);
		}
		*time = *time / m_thread_nums;
		writeResToFile(msg.c_str(), *time, len, m_thread_nums, m_count);
		delete [] time;
		time = NULL;
	
		return ;	
	}


	void PgStorage::threadScan(void *data, uint32 len)
	{
		thread_group tg;
		string msg("PgThreadScan");
		if(m_hasIndex){
			msg += "WithIndex";
		}


		double *time = new double[m_thread_nums];
		memset((void *)time, 0, sizeof(double) * m_thread_nums);

		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&PgStorage::threadScanFunc, this, data, len, (time + i)));
		}
		tg.join_all();

		for (uint32 i = 0; i < m_thread_nums; ++i)
		{
			*time += *(time + i);
		}
		*time = *time / m_thread_nums;
		writeResToFile(msg.c_str(), *time, len, m_thread_nums, m_count);
		delete [] time;
		time = NULL;
	
		return ;
	}

	void PgStorage::threadDel(void*data, uint32 len)
	{
		thread_group tg;
		string msg("PgThreadDel");
		if(m_hasIndex){
			msg += "WithIndex";
		}

		double *time = new double[m_thread_nums];
		memset((void *)time, 0, sizeof(double) * m_thread_nums);


		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&PgStorage::threadDelFunc, this, data, len, (time + i)));
		}
		tg.join_all();

		for (uint32 i = 0; i < m_thread_nums; ++i)
		{
			*time += *(time + i);
		}
		*time = *time / m_thread_nums;
		writeResToFile(msg.c_str(), *time, len, m_thread_nums, m_count);
		delete [] time;
		time = NULL;		
	
		return ;		
	}

	void PgStorage::threadBatchInsertFunc(void *data, uint32 len, uint32 nums, double *timeOut)
	{
		Transaction* pTransaction = NULL;

		PerformanceTimer timer("pgThreadBatchInsertWithIndex", 0, 0, 0,false);
		try {		
			m_pStorageEngine->beginThread();
			pTransaction = getNewTransaction();
			threadBatchInsertFuncInner(pTransaction, data, len, nums);
			
			pTransaction->commit();
			m_pStorageEngine->endThread();
		} catch (StorageEngineException &ex) {		
			if (pTransaction != NULL) {
				pTransaction->abort();
			}
			std::cerr<< ex.getErrorMsg() << endl;
		}
		*timeOut = timer.getElapse();
		return;	
	}

	void PgStorage::threadInsertFirstIndexLastFunc(void *data, uint32 len, double *timeOut)
	{
		Transaction* pTransaction = NULL;

		PerformanceTimer timer("pgThreadInsertFirstIndexLast", 0, 0, 0,false);
		try {		
			m_pStorageEngine->beginThread();
			pTransaction = getNewTransaction();
			threadInsertFirstIndexLastFuncInner(pTransaction, data, len);
			
			pTransaction->commit();
			m_pStorageEngine->endThread();
		} catch (StorageEngineException &ex) {		
			if (pTransaction != NULL) {
				pTransaction->abort();
			}
			std::cerr<< ex.getErrorMsg() << endl;
		}
		*timeOut = timer.getElapse();
		return;	
	}

	void PgStorage::threadInsertFunc(void *data, uint32 len, void *lessData, uint32 lessLen, double *timeOut)
	{
		Transaction* pTransaction = NULL;

		PerformanceTimer timer("pgThreadInsertWithIndex", 0, 0, 0,false);
		try {		
			m_pStorageEngine->beginThread();
			pTransaction = getNewTransaction();
			threadInsertFuncInner(pTransaction, data, len, lessData, lessLen);
			
			pTransaction->commit();
			m_pStorageEngine->endThread();
		} catch (StorageEngineException &ex) {		
			if (pTransaction != NULL) {
				pTransaction->abort();
			}
			std::cerr<< ex.getErrorMsg() << endl;
		}
		*timeOut = timer.getElapse();
		return;
	}

	void PgStorage::threadDelFunc(void *data, uint32 len, double *timeOut)
	{
		Transaction* pTransaction = NULL;
		PerformanceTimer timer("pgThreadDel", 0, 0, 0, false);
		try {		
			m_pStorageEngine->beginThread();
			pTransaction = getNewTransaction();
			threadDelFuncInner(pTransaction, data, len);
			
			pTransaction->commit();
			m_pStorageEngine->endThread();
		} catch (StorageEngineException &ex) {	
			if (pTransaction != NULL) {
				pTransaction->abort();
			}
			std::cerr << ex.getErrorMsg() << endl;
		}
		*timeOut = timer.getElapse();
	}


	void PgStorage::threadUpdateFunc(void *data, uint32 len, double *timeOut)
	{
		Transaction* pTransaction = NULL;
		PerformanceTimer timer("PgThreadUpdate", 0, 0, 0, false);
		try {		
			m_pStorageEngine->beginThread();
			pTransaction = getNewTransaction();

			threadUpdateFuncInner(pTransaction, data, len);

			pTransaction->commit();
			m_pStorageEngine->endThread();
		} catch (StorageEngineException &ex) {
			std::cerr << ex.getErrorMsg() << endl;

		}
		*timeOut = timer.getElapse();
		return;

	}


	void PgStorage::threadScanFunc(void *data, uint32 len, double *timeOut)
	{
		Transaction* pTransaction = NULL;
		PerformanceTimer timer("PgThreadScanWithIndex", 0, 0, 0, false);
		try {		
			m_pStorageEngine->beginThread();
			pTransaction = getNewTransaction();

			threadScanFuncInner(pTransaction, data, len);

			pTransaction->commit();
			m_pStorageEngine->endThread();	
		} catch (StorageEngineException &ex) {
			std::cerr<< ex.getErrorMsg() << endl;

		}

		*timeOut = timer.getElapse();
		return;

	}

	void PgStorage::threadBatchInsertFuncInner(Transaction *pTransaction, void *data, uint32 len, uint32 nums)
	{
		EntrySet *table = NULL;

		try {
			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);

			DataItem content(data, len);
			std::vector<DataItem> datas;

			uint32 count = m_count / m_thread_nums;
			for (uint32 i = 0; i < count; i++) {
				datas.push_back(content);
			}
			table->insertEntries(pTransaction,datas);

			m_pStorageEngine->closeEntrySet(pTransaction, table);

		}catch (StorageEngineException &)
		{
			if(pTransaction != NULL)
			{
				pTransaction->abort();
			}
			if(table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, table);
			}
			throw StorageEngineException(insertError, "insert entry failed");

		}//catch
	
	}

	void PgStorage::threadInsertFirstIndexLastFuncInner(Transaction *pTransaction, void *data, uint32 len)
	{
		EntrySet *table = NULL;
		DataItem content;
		formDataItem(content, data, len);


		formIndexColinfo(indexColinfo);
		setColumnInfo(3, &indexColinfo);


		try {
			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);

			EntryID eid = {0};
			uint32 count = m_count / m_thread_nums;
			for (uint32 i = 0; i < count; i++) {
				table->insertEntry(pTransaction, eid, content); 
			}

			m_index_tableId = m_pStorageEngine->createIndexEntrySet(pTransaction, table, BTREE_INDEX_ENTRY_SET_TYPE, 3);
						
			m_pStorageEngine->closeEntrySet(pTransaction, table);
			
		}catch (StorageEngineException &)
		{
			if (NULL == table)
			{
				pTransaction->abort();
			} 

			if(table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, table);
			}
			throw StorageEngineException(insertError, "insert entry failed");

		} //catch

		return;			
	}

	void PgStorage::threadInsertFuncInner(Transaction *pTransaction, void *data, uint32 len, void *lessData, uint32 lessLen)
	{
		EntrySet *table = NULL;

		try {
			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);

			DataItem content;
			formDataItem(content, data, len);

			DataItem value;
			formDataItem(value, lessData, lessLen);
		
			uint32 count = m_count / m_thread_nums;
			EntryID eid = {0};
			for (uint32 i = 0; i < count; i++) {
				table->insertEntry(pTransaction, eid, content);
			}
			for(uint32 i = 0; i < m_lessNums; ++i){
				table->insertEntry(pTransaction, eid, value);
			}

			m_pStorageEngine->closeEntrySet(pTransaction, table);

		}catch (StorageEngineException &)
		{
			if(pTransaction != NULL)
			{
				pTransaction->abort();
			}
			if(table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, table);
			}
			throw StorageEngineException(insertError, "insert entry failed");

		}//catch
		//pTransaction->commit();
	}

	void PgStorage::threadUpdateFuncInner(Transaction *pTransaction, void *data, uint32 len)
	{
		EntrySet *table = NULL;
		IndexEntrySet *index_table = NULL;

		try 
		{
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);
			
			DataItem updataData;
			formDataItem(updataData, data, len);
			
			if(!m_hasIndex)
			{
				EntrySetScan *entry_scan = table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
				
				DataItem getData;
				EntryID tid = {0};
				while (true)
				{
					int flag;
					flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getData);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}
		
					size_t getSize = getData.getSize();
					uint32 size = getSize > len ? len : getSize;
					if(0 == memcmp(data, getData.getData(), size))
					{
						table->updateEntry(pTransaction, tid, updataData);
					}

				}
				table->endEntrySetScan(entry_scan);
			}else{
				index_table = m_pStorageEngine->openIndexEntrySet(pTransaction, table, EntrySet::OPEN_EXCLUSIVE , m_index_tableId);

				ScanCondition cond(1, ScanCondition::Equal, (se_uint64)data, keyLen, heap_char_cmp);
				std::vector<ScanCondition> vscans;
				vscans.push_back(cond);
				EntrySetScan *pscan = NULL;
				pscan = index_table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, vscans);

				DataItem getData;
				EntryID tid = {0};
				while (true)
				{
					int flag;
					flag=pscan->getNext(EntrySetScan::NEXT_FLAG,tid,getData);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}

					table->updateEntry(pTransaction, tid, updataData);
				}
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			} //else

			m_pStorageEngine->closeEntrySet(pTransaction, table);
	
		}catch (StorageEngineException &)
		{
			if(table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, table);
			}
			if(index_table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			}

			throw StorageEngineException(insertError, "update entry failed");

		}
		return;		
	}


	void PgStorage::threadDelFuncInner(Transaction *pTransaction, void *data, uint32 len)
	{
		EntrySet *table = NULL;
		IndexEntrySet *index_table = NULL;
		try 
		{
				
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);

			DataItem updataData;
			formDataItem(updataData, data, len);
			
			if(!m_hasIndex){

				EntrySetScan *entry_scan = table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
				
				DataItem getData;
				EntryID tid = {0};
				while (true)
				{
					int flag;
					flag=entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getData);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}
		
					table->deleteEntry(pTransaction,tid);

				}
				table->endEntrySetScan(entry_scan);
			}else{

				index_table = m_pStorageEngine->openIndexEntrySet(pTransaction, table, EntrySet::OPEN_EXCLUSIVE , m_index_tableId);

				ScanCondition cond(1, ScanCondition::Equal, (se_uint64)data, keyLen, heap_char_cmp);
				std::vector<ScanCondition> vscans;
				vscans.push_back(cond);
				EntrySetScan *pscan = NULL;
				pscan = index_table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, vscans);

				DataItem getData;
				EntryID tid = {0};
				while (true)
				{
					int flag;
					flag=pscan->getNext(EntrySetScan::NEXT_FLAG,tid,getData);
					if (flag==NO_DATA_FOUND)
					{
						break;
					}

					table->deleteEntry(pTransaction,tid);
				}
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			} //else
			command_counter_increment();
			
			m_pStorageEngine->closeEntrySet(pTransaction, table);
			pTransaction->commit();
		}catch (StorageEngineException &)
		{
			if(table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, table);
			}
			if(index_table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			}

			throw StorageEngineException(insertError, "del entry failed");

		}
		return;		
	}

	void PgStorage::threadScanFuncInner(Transaction *pTransaction, void *data, uint32 len)
	{
		EntrySet *table = NULL;
		IndexEntrySet *index_table = NULL;
		try 
		{			
			table = m_pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);

			DataItem updataData;
			formDataItem(updataData, data, len);
			
			if(!m_hasIndex){
				EntrySetScan *entry_scan = table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
				
				DataItem getData;
				EntryID tid = {0};

				while(entry_scan->getNext(EntrySetScan::NEXT_FLAG,tid,getData) == 0)
				{
					uint32 size = getData.getSize();
					uint32 cmpSize = size > len ? len :size;
					if(memcmp(data, getData.getData(), cmpSize) == 0)
					{
						continue;
					}
				}

		
	
				table->endEntrySetScan(entry_scan);
			}else{
				index_table = m_pStorageEngine->openIndexEntrySet(pTransaction, table, EntrySet::OPEN_EXCLUSIVE , m_index_tableId);
				ScanCondition cond(1, ScanCondition::Equal, (se_uint64)data, keyLen, heap_char_cmp);
				std::vector<ScanCondition> vscans;
				vscans.push_back(cond);
				EntrySetScan *pscan = NULL;
				pscan = index_table->startEntrySetScan(pTransaction, BaseEntrySet::SnapshotMVCC, vscans);

				DataItem getData;
				EntryID tid = {0};
				while(pscan->getNext(EntrySetScan::NEXT_FLAG,tid,getData) == 0);
				//static int s_i = 0;
				//while(pscan->getNext(EntrySetScan::NEXT_FLAG,tid,getData) == 0)
				//{
				//	s_i++;
				//}
				index_table->endEntrySetScan(pscan);
#if 0
					char *p = (char *)getData.getData();
					cout << p << endl;
					char *p0 = (char *)data;
					cout << p0;
#endif 

					m_pStorageEngine->closeEntrySet(pTransaction, index_table);
				
			} //else
			command_counter_increment();

			m_pStorageEngine->closeEntrySet(pTransaction, table);

		}catch (StorageEngineException &)
		{
			if(table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, table);
			}
			if(index_table != NULL)
			{
				m_pStorageEngine->closeEntrySet(pTransaction, index_table);
			}

			throw StorageEngineException(insertError, "scan failed");

		}
		return;
	}


	void PgStorage::command_counter_increment()
	{
		m_pStorageEngine->endStatement();
	}

	int PgStorage::compDataItem(const DataItem &data1, const DataItem &data2)
	{
		uint32 len1  = data1.getSize();
		uint32 len2 = data2.getSize();
		uint32 len = len1 > len2 ? len2: len1;

		return memcmp(data1.getData(), data2.getData(), len);	
	}

	int heap_char_cmp(const char *data1, size_t len1, const char *data2, size_t len2)
	{
		size_t size = len1 > len2 ? len2 :len1;

		return memcmp(data1,data2,size);
	}
	
	void heap_single_split(RangeDatai& rangeData, const char* str, int col, size_t len)
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
			rangeData.len = keyLen;
		}
		if(col == 2)
		{
			rangeData.start = keyLen;
			rangeData.len = len;
		}
	}


	void index_split(RangeDatai& rangeData, const char* str, int col, size_t len)
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
			rangeData.len = keyLen;
		}
		if(col == 2)
		{
			rangeData.start = keyLen;
			rangeData.len = len;
		}
	}


	PgStorage::PgStorage(uint32 count, StorageEngine *storage, uint32 thread_nums, bool hasIndex, uint32 lessNums)
	{
		m_count = count;
		m_hasIndex = hasIndex;
		m_thread_nums = thread_nums;
		m_lessNums = lessNums;


		Transaction *m_pTransaction = NULL;
		EntrySet *table = NULL;
		try{
			m_pStorageEngine = storage;

			///run a new transaction
			m_pTransaction = getNewTransaction();

			formHeapColinfo(colinfo);
			setColumnInfo(2, &colinfo);

			m_tableId = 0;
			m_tableId = m_pStorageEngine->createEntrySet(m_pTransaction, 2);

			//m_tableId = 1085;
			//table = m_pStorageEngine->openEntrySet(m_pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);


			if(hasIndex)
			{
				table = m_pStorageEngine->openEntrySet(m_pTransaction, EntrySet::OPEN_EXCLUSIVE, m_tableId);
				formIndexColinfo(indexColinfo);
				setColumnInfo(3, &indexColinfo);
				m_index_tableId = m_pStorageEngine->createIndexEntrySet(m_pTransaction, table,BTREE_INDEX_ENTRY_SET_TYPE, 3);
				command_counter_increment();
				m_pStorageEngine->closeEntrySet(m_pTransaction, table);
			}

		} catch (StorageEngineException &e)
		{
			//std::cout <<e.what() << std::endl;
			if(0 == m_tableId){
				m_pTransaction->abort();
			} else {
				m_pStorageEngine->closeEntrySet(m_pTransaction, table);
				m_pTransaction->abort();
			}
			throw;

		}

		m_pTransaction->commit();
		++num;

	}

	PgStorage::~PgStorage()
	{

		Transaction *m_pTransaction = getNewTransaction();
		try {
			m_pStorageEngine->removeEntrySet(m_pTransaction, m_tableId);  
		} catch (StorageEngineException &)
		{
			m_pTransaction->abort();
		}
		m_pTransaction->commit();
		--num;
		if(0 == num){
			StorageEngine::releaseStorageEngine(m_pStorageEngine);
		}
		freeColinfo(colinfo);
		if(m_hasIndex)
		{
			freeColinfo(indexColinfo);
		}

	}


	Transaction * PgStorage::getNewTransaction()
	{
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		return m_pStorageEngine->getTransaction(invalid_transaction, Transaction::SERIALIZABLE_ISOLATION_LEVEL);//获得一个事务
	}




	void PgStorage::formHeapColinfo(ColumnInfo &colInfo)
	{
		colInfo.col_number = NULL;
		colInfo.keys = 2;  // columns of heap
		colInfo.rd_comfunction = (CompareCallbacki *)malloc(2*sizeof(CompareCallbacki));

		colInfo.rd_comfunction[0] = heap_char_cmp;
		colInfo.rd_comfunction[1] = heap_char_cmp;
		colInfo.split_function = heap_single_split;
	}

	void PgStorage::formIndexColinfo(ColumnInfo &colInfo)
	{
		colInfo.col_number = (size_t*)malloc(sizeof(size_t));
		colInfo.col_number[0] = 1;
		colInfo.keys = 1;
		colInfo.rd_comfunction = (CompareCallbacki *)malloc(sizeof(CompareCallbacki));
		colInfo.rd_comfunction[0] = heap_char_cmp;
		colInfo.split_function = index_split;		
	}

	void PgStorage::freeColinfo(ColumnInfo &colInfo)
	{
	    if(colInfo.rd_comfunction){
			free(colInfo.rd_comfunction);
			colInfo.rd_comfunction = NULL;
		}
		if(colInfo.col_number)
		{
			free(colInfo.col_number);
			colInfo.col_number = NULL;
		}
	}

	void PgStorage::formDataItem(DataItem &dataItem, void *data, uint32 len)
	{
		dataItem.setData(data);
		dataItem.setSize(len);
	}


	//int PgStorage::heap_char_cmp(const char *data1, size_t len1, const char *data2, size_t len2)
	//{
	//	size_t size = len1 > len2 ? len2 :len1;
	//	return memcmp(data1,data1,size);
	//}
	//
	//void PgStorage::heap_single_split(RangeDatai& rangeData, const char* str, int col, size_t len)
	//{
	//	rangeData.len = 0;
	//	rangeData.start = 0;
	//	if(str == NULL)
	//	{
	//		return ;
	//	}
	//	if (col == 1)
	//	{
	//		rangeData.start = 0;
	//		rangeData.len = len;
	//	}
	//}
} //namespace PgBdbPerformanceComp
} //namespace FounderXdb

