#ifndef PG_INSERT_HPP
#define PG_INSERT_HPP

/// @file pg_insert.h
/// @brief PgStorage class definition.
///

#include "utils/pgbdbutils.h"
#include "PGSETypes.h"


namespace FounderXDB
{

namespace StorageEngineNS{
	class DataItem;
	class EntrySet;
	class IndexEntrySet;
	class ColumnInfo;
	class RangeDatai;
	class Transaction;
	class StorageEngine;
}
using StorageEngineNS::DataItem;
using StorageEngineNS::EntrySet;
using StorageEngineNS::ColumnInfo;
using StorageEngineNS::RangeDatai;
using StorageEngineNS::Transaction;
using StorageEngineNS::StorageEngine;
using StorageEngineNS::IndexEntrySet;



namespace PgBdbPerformanceComp
{
	class PgStorage{
	public:
		/// Insert a data in pg .
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  isindex if true this heap has index;false no index on this heap
		void insert(void *data, uint32 len, void *lessData = NULL, uint32 lessLen = 0);

		/// Insert datas in pg .
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  nums  the number of data
		void batchInsert(void *data, uint32 len, uint32 nums);

		/// Insert a data then create index.
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  nums  the number of data
		void insertFirstIndexLast(void *data, uint32 len);



		/// del a data in pg
		/// @param[in] data  the data wanted to delete
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		void del(void *data, uint32 len);

		/// del a data in pg
		/// @param[in] data  the data wanted to update
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		void update(void *data, uint32 len);

		/// scan a data in pg, record the max time for a single tuple
		/// @param[in] data  the data wanted to scan
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		void scan(void *data, uint32 len);

		/// Insert a data in multi threads
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  isindex if true this heap has index;false no index on this heap
		void threadInsert(void *data, uint32 len, void *lessData = NULL, uint32 lessLen = 0);

		/// Insert data first then index create in multi threads
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  nums  the number of data
		void threadInsertFirstIndexLast(void *data, uint32 len);

		/// Insert batch data in multi threads
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  nums  the number of data
		void threadBatchInsert(void *data, uint32 len, uint32 nums);

		/// update the data in multi thread
		/// @param[in] data  the data wanted to update
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		void threadUpdate(void *data, uint32 len);

		/// scan a data in multi threads, record the max time for a single tuple
		/// @param[in] data  the data wanted to scan
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		void threadScan(void *data, uint32 len);

		/// del a data in multi thread
		/// @param[in] data  the data wanted to delete
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		void threadDel(void*data, uint32 len);


	public:
		/// create a heap table
		PgStorage(uint32 count, StorageEngine *storage, uint32 thread_nums =1, bool isIndex = false, uint32 lessNums = 0);
		/// drop the heap table
		~PgStorage();

	private:
		/// form data item needed by entryset insert api
		/// @param[in] data  
		/// #param[in] len   len of data
		/// @param[out] dataItem  dataItem needed by pg entryset
		void formDataItem(DataItem &dataItem, void *data, uint32 len);

		/// compare DataItem
		/// @param[in] data1 want to compare 
		/// @param[in] data2 want to compare
		/// @return  0 equal; > 0 data1 > data2; < data1 < data2
		int compDataItem(const DataItem &data1, const DataItem &data2);

		void threadInsertFuncInner(Transaction *pTransaction, void *data, uint32 len, void *lessData, uint32 lessLen);
		void threadInsertFirstIndexLastFuncInner(Transaction *pTransaction, void *data, uint32 len);
		void threadBatchInsertFuncInner(Transaction *pTransaction, void *data, uint32 len, uint32 nums);

		void threadUpdateFuncInner(Transaction *pTransaction, void *data, uint32 len);

		void threadScanFuncInner(Transaction *pTransaction, void *data, uint32 len);

		void threadDelFuncInner(Transaction *pTransaction, void *data, uint32 len);


		void formHeapColinfo(ColumnInfo &colInfo);
		void freeColinfo(ColumnInfo &colInfo);

		void formIndexColinfo(ColumnInfo &colInfo);

		

		//int heap_char_cmp(const char *data1, size_t len1, const char *data2, size_t len2);
		//void heap_single_split(RangeDatai& rangeData, const char* str, int col, size_t len);

		void threadInsertFunc(void *data, uint32 len, void *lessData, uint32 lessLen, double *timeOut);
		void threadInsertFirstIndexLastFunc(void *data, uint32 len, double *timeOut);

		void threadBatchInsertFunc(void *data, uint32 len, uint32 nums, double *timeOut);

		void threadUpdateFunc(void *data, uint32 len, double *timeOut);

		void threadScanFunc(void *data, uint32 len, double *timeOut);

		void threadDelFunc(void *data, uint32 len, double *timeOut);
		

		void command_counter_increment();

	private:
		Transaction * getNewTransaction();

	private:
		bool m_hasIndex;
		uint32 m_count;
		uint32 m_thread_nums;
		StorageEngine *m_pStorageEngine;
		StorageEngineNS::EntrySetID m_tableId;
		StorageEngineNS::EntrySetID m_index_tableId;
		uint32 m_lessNums;

		static int num;
		//EntrySet *m_table;
		//IndexEntrySet *m_index_table;
		//Transaction *m_pTransaction;
		


	};

extern int heap_char_cmp(const char *data1, size_t len1, const char *data2, size_t len2);
//extern  void threadInsertFunc(void *data, uint32 len);
}//PgBdbPerformanceComp

} //FounderXDB

#endif
