#ifndef BDB_INSERT_HPP
#define BDB_INSERT_HPP

#include <string>

#include <db_cxx.h>

#include "boost/thread.hpp" 
/// @file pg_insert.h
/// @brief PgStorage class definition.
///

#include "utils/pgbdbutils.h"
#include "PGSETypes.h"

using boost::thread_group;
using boost::thread;
using boost::bind;

namespace FounderXDB
{




namespace PgBdbPerformanceComp
{
	class BdbStorage{
	public:
		/// Insert a data in bdb .
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  isindex if true this heap has index;false no index on this heap
		void insert(void *data, uint32 len, void *lessData, uint32 lessLen);

		/// Insert batch datas in bdb .
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  nums the number of data
		/// @param[in]  isindex if true this heap has index;false no index on this heap
		void batchInsert(void *data, uint32 len, uint32 nums);	

		int insert(void *data, uint32 len, bool isC);

		/// del a data in pg
		/// @param[in] data  the data wanted to delete
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		void del(void *data, uint32 len);

		/// del a data in pg
		/// @param[in] data  the data wanted to update
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		/// @param[in] DescData destenation data
		/// @param[in] descLen len of DescData
		void update(void *data, uint32 len, void *DescData = NULL, uint32 descLen =0);

		/// scan a data in pg, record the max time for a single tuple
		/// @param[in] data  the data wanted to scan
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		void scan(void *data, uint32 len);

		/// Insert a data in multi threads
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  isindex if true this heap has index;false no index on this heap
		void threadInsert(void *data, uint32 len, void *lessData, uint32 lessLen);

		/// Insert batch datas in multi threads
		/// @param[in]  data     data
		/// @param[in]  len    len of data
		/// @param[in]  nums the number of data
		void threadBatchInsert(void *data, uint32 len, uint32 nums);

		/// update the data in multi thread
		/// @param[in] data  the data wanted to update
		/// @param[in] len   the length of data
		/// @param[in] isindex if true this heap has index;false no index on this heap
		void threadUpdate(void *data, uint32 len, void *DescData = NULL, uint32 descLen = 0);

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
		BdbStorage(uint32 count = 1,  uint32 thread_nums =1, bool isIndex = false, Db *m_dbp = NULL, DbEnv *m_envp = NULL, uint32 lessCount = 0);
		/// drop the heap table
		~BdbStorage();

	private:

		
	int openDb(Db **, const char *, const char *, DbEnv *, uint32);
	void init();
	void destroy();
	void openDB(DbTxn *txn);
	void openDBWithMultiThread();

	void closeDB();

	void index_insert(Dbt &data, Dbt &key, Dbt &lessData, Dbt &lessKey);
	void batch_insert(Dbt &data, Dbt &key);

	void index_scan(Dbt &data, Dbt &key);

	void index_update(Dbt &data, Dbt &key);
	
	void heap_update(Dbt &data, Dbt &DesData, Dbt &key);


	void index_del(Dbt &data, Dbt &key);
	void heap_del(Dbt &data, Dbt &key);

	void threadInsertFuncWithIndex(Dbt &data, Dbt &key, Dbt &lessData, Dbt &lessKey, double *timeOut);
	void threadBatchInsertFuncWithIndex(Dbt &data, Dbt &key, double *timeOut);

	void threadScanFuncWithIndex(Dbt &data, Dbt &key, double *timeOut);

	void threadUpdateFuncWithIndex(Dbt &data, Dbt &key, double *timeOut);
	void threadUpdateFunc(Dbt &data, Dbt &desData, Dbt &key, double *timeOut);

	void threadDelFuncWithIndex(Dbt &data, Dbt &key, double *timeOut);
	void threadDelFunc(Dbt &data, Dbt &key, double *timeOut);


	//void writeResToFile(const std::string &op, double timeOut, uint32 len);
	private:
		std::string fileName;
		std::string dataBase;
		uint32 m_count;
		uint32 m_thread_nums;
		uint32 m_less_count;
		bool m_has_index;
		Db *m_dbp;
		DbEnv *m_envp;

		boost::mutex m_mutex;


		

		//DbTxn *m_txn;
		//EntrySet *m_table;
		//IndexEntrySet *m_index_table;
		//Transaction *m_pTransaction;
		


	};




extern int heap_char_cmp(const char *data1, size_t len1, const char *data2, size_t len2);
//extern  void threadInsertFunc(void *data, uint32 len);
}//PgBdbPerformanceComp

} //FounderXDB


#endif
