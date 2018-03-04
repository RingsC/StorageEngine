#include <string>

#include "boost/thread.hpp" 

#include "utils/pgbdbutils.h"
#include "bdb/bdb_insert.h"
#include "utils/performance_timer.h"

#include <db_cxx.h>

using boost::thread_group;
using boost::thread;
using boost::bind;


using std::string;



namespace FounderXDB{
namespace PgBdbPerformanceComp

{

	BdbStorage::BdbStorage(uint32 count,  uint32 thread_nums, bool isIndex, Db *dbp, DbEnv *envp, uint32 lessCount):m_count(count), m_thread_nums(thread_nums), m_has_index(isIndex),
							m_dbp(dbp), m_envp(envp), m_less_count(lessCount)
	{
		init();

	}

	void BdbStorage::init()
	{
		    // Application name
		const char *progName = "BdbStorage";


		uint32 envFlags =
			DB_CREATE     |  // Create the environment if it does not exist
			DB_RECOVER    |  // Run normal recovery
			DB_INIT_LOCK  |  // Initialize the locking subsystem
			DB_INIT_LOG   |  // Initialize the logging subsystem
			DB_INIT_TXN   |  // Initialize the transactional subsystem. This
                       // also turns on logging.
			DB_INIT_MPOOL |  // Initialize the memory pool (in-memory cache)

			DB_THREAD;       // Cause the environment to be free-threaded

		try {
		
			// Create the environment
			m_envp = new DbEnv(0);
			//DbEnv m_envp(0);

			// Specify in-memory logging
			//m_envp->log_set_config(DB_LOG_IN_MEMORY, 1);

			// Specify the size of the in-memory log buffer.
			m_envp->set_lg_bsize(10 * 1024 * 1024);

			// Specify the size of the in-memory cache
			m_envp->set_cachesize(0, 135 * 1024 * 1024, 1);

			// Indicate that we want db to internally perform deadlock
			// detection.  Also indicate that the transaction with
			// the fewest number of write locks will receive the
			// deadlock notification in the event of a deadlock.
			m_envp->set_lk_detect(DB_LOCK_MINWRITE);

			//100W内存
			
			//m_envp->set_cache_max(1000000, 1000000);
			

			// Open the environment
#ifdef WIN32
			m_envp->open("D:\\XMLDB\\bdbHome", envFlags, 0);

#else
			m_envp->open("/home/fd/bdbHome/", envFlags, 0);
#endif


		}catch(DbException &e)
		{
		/*	if(m_txn != NULL){
				m_txn->abort();
			}*/
			std::cerr << "Error opening database environment: "
                  << std::endl;
			std::cerr << e.what() << std::endl;
		}		
	}

	void BdbStorage::openDBWithMultiThread()
	{
			uint32 openFlags;
	
			try {
				m_dbp = new Db(m_envp, 0);  //throw exception


				/// support duplicate    
				m_dbp->set_flags(DB_DUP);

				///set db cache size
				m_dbp->set_pagesize(8192);


								//// Now open the database */
				openFlags = DB_CREATE        | // Allow database creation
							DB_AUTO_COMMIT   |    // Allow autocommit
			                DB_THREAD; 


				///transaction auto committed because of DB_AUTO_COMMIT
				m_dbp->open(NULL,   // Txn pointer, protected by transaction because of DB_AUTO_COMMIT
					  "MyTest.db",   // File name
					NULL,       // Logical db name
					DB_BTREE,   // Database type (using btree)
					openFlags,  // Open flags
					0);       // File mode. Using defaults

			} catch (DbException &e) {
				/*if(m_txn != NULL)
				{
					m_txn->abort();
				}*/
				std::cerr<< ": openDb: db open failed:" << std::endl;
				std::cerr << e.what() << std::endl;

			}	
	}
	void BdbStorage::openDB(DbTxn *txn)
	{
			uint32 openFlags;
	
			try {
				m_dbp = new Db(m_envp, 0);  //throw exception


				/// support duplicate    
				m_dbp->set_flags(DB_DUP);

				/// Set the size of the pages used to hold items in the database, in bytes
				m_dbp->set_pagesize(8192);

								//// Now open the database */
				openFlags = DB_CREATE        | // Allow database creation
			                 DB_THREAD; 


				m_dbp->open(txn,   // Txn pointer, protected by transaction because of DB_AUTO_COMMIT
					  "MyTest.db",   // File name
					NULL,       // Logical db name
					DB_BTREE,   // Database type (using btree)
					openFlags,  // Open flags
					0);       // File mode. Using defaults


			} catch (DbException &e) {

				std::cerr<< ": openDb: db open failed:" << std::endl;
				std::cerr << e.what() << std::endl;

			}	
	}

	int BdbStorage::openDb(Db **dbpp, const char *progname, const char *fileName, DbEnv *envp, u_int32_t extraFlags)
	{
		uint32 openFlags;

		try {
			Db *dbp = new Db(envp, 0);


			// Point to the new'd Db
			*dbpp = dbp;
			/// Set the size of the pages used to hold items in the database, in bytes
			m_dbp->set_pagesize(8192);

			// Now open the database */
			openFlags = DB_CREATE; 

			dbp->open(NULL,   // Txn pointer
                  fileName,   // File name
                  NULL,       // Logical db name
                  DB_BTREE,   // Database type (using btree)
                  openFlags,  // Open flags
                  0);         // File mode. Using defaults
		} catch (DbException &e) {
			std::cerr << progname << ": openDb: db open failed:" << std::endl;
			std::cerr << e.what() << std::endl;
			return (EXIT_FAILURE);
		}
		return (EXIT_SUCCESS);

	}

	void BdbStorage::closeDB()
	{
		try {
			if (m_dbp != NULL)
			{
				int ret = m_dbp->close(0);
				//m_dbp->remove("MyTest.db", NULL, 0);
				//delete m_dbp;
				m_dbp = NULL;
			}

		} catch(DbException &e) {
			std::cerr << "Error closing database and environment."
                  << std::endl;
			std::cerr << e.what() << std::endl;			
		}		
	}

	void BdbStorage::destroy()
	{
		try {

			// Close our environment if it was opened.
			if (m_envp != NULL)
			{
				m_envp->close(0);
				//m_envp->remove("D:\\XMLDB\\bdbHome", 0);
				//delete m_envp;
				m_envp = NULL;
			}
		} catch(DbException &e) {
			std::cerr << "Error closing database and environment."
                  << std::endl;
			std::cerr << e.what() << std::endl;
			
		}	
	}

	BdbStorage::~BdbStorage()
	{
		destroy();

	}

	void BdbStorage::batchInsert(void *data, uint32 len, uint32 nums)
	{
		Dbt value;

		
		uint32 allLen = m_count * (len + 8) + 4;
		void *buf = malloc(allLen);
		value.set_data(buf);
		value.set_ulen(allLen);
		value.set_flags(DB_DBT_USERMEM);

		DbMultipleDataBuilder datas(value);
		for(uint32 i = 0; i < m_count; ++i)
		{
			datas.append(data, len);
		}

#if 0
		DbMultipleDataIterator iter(value);
		Dbt nextItem;
		while (iter.next(nextItem) == true);
#endif

		Dbt key;
		void *keyBuf = NULL;

		uint32 keylen = len > keyLen ? keyLen : len;
		uint32 keyAllLen = m_count  * (keylen + 8) +4;
		keyBuf = malloc(keyAllLen);
		key.set_data(keyBuf);
		key.set_ulen(keyAllLen);
		key.set_flags(DB_DBT_USERMEM);
		DbMultipleDataBuilder keyDatas(key);

		for(uint32 i = 0; i < m_count; ++i)
		{
			keyDatas.append(data, keylen);
		}

		PerformanceTimer timer("bdbBatchInsertWithIndex",m_count, len);
		try{
			batch_insert(value, key);
		}catch(DbException &){
			std::cerr << "bdb insert error" << std::endl;
			throw;
		}
		free(buf);
		free(keyBuf);
	}

	void BdbStorage::insert(void *data1, uint32 len, void *lessData, uint32 lessLen)
	{
		
		Dbt value(data1, len);
		Dbt lessValue(lessData, lessLen);

		string msg("bdbInsert");

		Dbt key;
		Dbt lessKey;
		if(m_has_index)
		{
			msg += "WithIndex";
			uint32 keylen = len > keyLen ? keyLen : len;
			key.set_data(data1);
			key.set_size(keylen);
			
			uint32 lessKeyLen = lessLen > keyLen ? keyLen : lessLen;
			lessKey.set_data(lessData);
			lessKey.set_size(lessKeyLen);
		}

		PerformanceTimer timer(msg.c_str(),m_count, len);
		try{
			index_insert(value, key, lessValue, lessKey);
		}catch(DbException &){
			std::cerr << "bdb insert error" << std::endl;
			throw;
		}

	}

void BdbStorage::scan(void *data, uint32 len)
{
	string msg("bdbScan");
	Dbt value;
	Dbt key;

	if(m_has_index)
	{
		msg += "WithIndex";
		uint32 keylen = len > keyLen ? keyLen : len;
		key.set_data(data);
		key.set_size(keylen);	
	}

	PerformanceTimer timer(msg.c_str(),m_count, len);
	try{
		index_scan(value, key);
	}catch(DbException &){
		throw;
	}

}

void BdbStorage::update(void *data, uint32 len, void *DescData, uint32 descLen)
{

	Dbt value(data, len);
	Dbt descValue(DescData, descLen);
	
	Dbt key;
	try{
		if(m_has_index)
		{			
			uint32 keylen = len > keyLen ? keyLen : len;
			key.set_data(data);
			key.set_size(keylen);

			PerformanceTimer timer("bdbUpdateWithIndex",m_count, len);
			index_update(descValue, key);
		}else
		{
			PerformanceTimer timer("bdbUpdate",m_count, len);
			heap_update(value, descValue, key);
		}
	}catch(DbException &)
	{
		throw;
	}
	
}

void BdbStorage::del(void *data, uint32 len)
{
	/// change data and form Dbt
	Dbt value(data, len);
	
	Dbt key;
	try{
		if(m_has_index)
		{
			uint32 keylen = len > keyLen ? keyLen : len;
			key.set_data(data);
			key.set_size(keylen);

			PerformanceTimer timer("bdbDelWithIndex",m_count, len);
			index_del(value, key);
		}else
		{
			PerformanceTimer timer("bdbDel",m_count, len);
			heap_del(value, key);
		}
	}catch(DbException &)
	{
		std::cerr << "bdb del error" << endl;
		throw;
	}

}

void BdbStorage::threadDel(void*data, uint32 len)
{
	/// no index
	Dbt value(data, len);


	string msg("BdbthreadDel");

	Dbt key;
	if(m_has_index)
	{
		msg += string("WithIndex");
		uint32 keylen = len > keyLen ? keyLen : len;
		key.set_data(data);
		key.set_size(keylen);
	}

	/// open table before start up transaction
	openDBWithMultiThread();

	double *time = new double[m_thread_nums];
	memset((void *)time, 0, sizeof(double) * m_thread_nums);
	thread_group tg;

	if(m_has_index){
	
		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&BdbStorage::threadDelFuncWithIndex, this, value, key, (time + i)));
		}
		tg.join_all();

	}else{
		//PerformanceTimer timer("BdbthreadInsert", m_count, len, m_thread_nums);
		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&BdbStorage::threadDelFunc, this, value, key, (time + i)));
		}
		tg.join_all();		
	}

	for (uint32 i = 0; i < m_thread_nums; ++i)
	{
		*time += *(time + i);
	}
	*time = *time / m_thread_nums;
	writeResToFile(msg.c_str(), *time, len, m_thread_nums, m_count);
	delete [] time;
	time = NULL;
	// close table
	closeDB();

	return ;
}

void BdbStorage::threadUpdate(void *data, uint32 len, void *DescData, uint32 descLen)
{
	/// no index
	Dbt value(data, len);
	Dbt desValue(DescData, descLen);

	string msg("BdbthreadUpdate");

	Dbt key;
	if(m_has_index)
	{
		msg += string("WithIndex");
		uint32 keylen = len > keyLen ? keyLen : len;
		key.set_data(data);
		key.set_size(keylen);
	}

	/// store time in memory
	double *time = new double[m_thread_nums];
	memset((void *)time, 0, sizeof(double) * m_thread_nums);

	/// open table before start up transaction
	openDBWithMultiThread();

	thread_group tg;

	if(m_has_index){
		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&BdbStorage::threadUpdateFuncWithIndex, this, value, key, (time + i)));
		}
		tg.join_all();
	}else{
		for(uint32 i = 0; i < m_thread_nums; ++i)
		{
			tg.create_thread(bind(&BdbStorage::threadUpdateFunc, this, value, desValue, key, (time + i)));
		}
		tg.join_all();
	}

	/// calculate time
	for (uint32 i = 0; i < m_thread_nums; ++i)
	{
		*time += *(time + i);
	}
	*time = *time / m_thread_nums;
	writeResToFile(msg.c_str(), *time, len, m_thread_nums, m_count);
	delete [] time;
	time = NULL;

	// close table
	closeDB();

	return ;
}

void BdbStorage::threadScan(void *data, uint32 len)
{

	string msg("BdbThreadScan");
	Dbt value;
	Dbt key;

	if(m_has_index)
	{
		msg += "WithIndex";
		uint32 keylen = len > keyLen ? keyLen : len;
		key.set_data(data);
		key.set_size(keylen);	
	}

	/// open table before start up transaction
	openDBWithMultiThread();

	thread_group tg;
	double *time = new double[m_thread_nums];
	memset((void *)time, 0, sizeof(double) * m_thread_nums);

	for(uint32 i = 0; i < m_thread_nums; ++i)
	{
		tg.create_thread(bind(&BdbStorage::threadScanFuncWithIndex, this, value, key, (time + i)));
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

	// close table
	closeDB();

	return ;
}

void BdbStorage::threadBatchInsert(void *data, uint32 len, uint32 nums)
{
	Dbt value;

	uint32 count = m_count / m_thread_nums;
	uint32 allLen = count * (len + 8) + 4;
	void *buf = malloc(allLen);
	value.set_data(buf);
	value.set_ulen(allLen);
	value.set_flags(DB_DBT_USERMEM);

	DbMultipleDataBuilder datas(value);
	for(uint32 i = 0; i < count; ++i)
	{
		datas.append(data, len);
	}

	Dbt key;
	void *keyBuf = NULL;
	uint32 keylen = len > keyLen ? keyLen : len;
	uint32 keyAllLen = count  * (keylen + 8) +4;
	keyBuf = malloc(keyAllLen);
	key.set_data(keyBuf);
	key.set_ulen(keyAllLen);
	key.set_flags(DB_DBT_USERMEM);
	DbMultipleDataBuilder keyDatas(key);

	for(uint32 i = 0; i < count; ++i)
	{
		keyDatas.append(data, keylen);
	}



	/// open table before start up transaction
	openDBWithMultiThread();

	thread_group tg;
	double *time = new double[m_thread_nums];
	memset((void *)time, 0, sizeof(double) * m_thread_nums);

	for(uint32 i = 0; i < m_thread_nums; ++i)
	{
		tg.create_thread(bind(&BdbStorage::threadBatchInsertFuncWithIndex, this, value, key, (time + i)));
	}
	tg.join_all();

	///calculate time
	for (uint32 i = 0; i < m_thread_nums; ++i)
	{
		*time += *(time + i);
	}
	*time = *time / m_thread_nums;	
	writeResToFile("BdbThreadBatchInsertWithIndex", *time, len, m_thread_nums, m_count);
	delete [] time;
	time = NULL;

	// close table
	closeDB();
	free(buf);
	buf = NULL;
	free(keyBuf);
	keyBuf = NULL;

	return ;
}

void BdbStorage::threadInsert(void *data, uint32 len, void *lessData, uint32 lessLen)
{

	Dbt value(data, len);
	Dbt lessValue(lessData, lessLen);
	string msg("BdbThreadInsert");

	Dbt key;
	Dbt lessKey;
	if(m_has_index)
	{
		msg += "WithIndex";
		uint32 keylen = len > keyLen ? keyLen : len;
		key.set_data(data);
		key.set_size(keylen);

		uint32 lessKeyLen = lessLen > keyLen ? keyLen : lessLen;
		lessKey.set_data(lessData);
		lessKey.set_size(lessKeyLen);
	}

	/// open table before start up transaction
	openDBWithMultiThread();

	thread_group tg;
	double *time = new double[m_thread_nums];
	memset((void *)time, 0, sizeof(double) * m_thread_nums);

	for(uint32 i = 0; i < m_thread_nums; ++i)
	{
		tg.create_thread(bind(&BdbStorage::threadInsertFuncWithIndex, this, value, key, lessValue, lessKey, (time + i)));
	}
	tg.join_all();

	///calculate time
	for (uint32 i = 0; i < m_thread_nums; ++i)
	{
		*time += *(time + i);
	}
	*time = *time / m_thread_nums;	
	writeResToFile(msg.c_str(), *time, len, m_thread_nums, m_count);
	delete [] time;
	time = NULL;

	// close table
	closeDB();

	return ;
}

void BdbStorage::threadDelFunc(Dbt &data, Dbt &key, double *timeOut)
{
	DbTxn *txn = NULL;
	Dbc *dbcp = NULL;
	Dbt content;

	PerformanceTimer timer("bdbThreadDel", 0, 0, 0, false);
	uint32 i = 0;
	try{
		
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);

		boost::mutex::scoped_lock lock(m_mutex);
		// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		uint32 lenData = data.get_size();
		int ret = dbcp->get(&key, &content, DB_SET);
		while (ret != DB_NOTFOUND)
		{
#if 0
			std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "data:" << (char *)content.get_data() << "len:" << content.get_size() << std::endl;
#endif

			uint32 lenContent = content.get_size();
			if(lenData != lenContent)
			{
				ret = dbcp->get(&key, &content, DB_NEXT);
				continue;
			}
			if(memcmp(content.get_data(), data.get_data(), lenData) == 0)
			{
				ret = dbcp->del(0);
				ret = dbcp->get(&key, &content, DB_NEXT);
			}
		}

#if 0
		/// verify deletion operation
		ret = dbcp->get(&key, &content, DB_SET);
		if (DB_NOTFOUND != ret)
		{
			std::cerr << "del fail" << std::endl;
		}
#endif

		dbcp->close();
		txn->commit(DB_TXN_SYNC);

		
		//dbcp->close();
		//closeDB();
	}catch(DbException &ee){

		if(txn != NULL){
			txn->abort();
		}

		if(dbcp != NULL)
		{
			dbcp->close();
		}
		
		std::cerr << ee.what() << std::endl;
	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}

		if(dbcp != NULL)
		{
			dbcp->close();

		}

		std::cerr << "Unknown exception: " << ee.what() << std::endl;

	}
	*timeOut = timer.getElapse();
	return ;
}


void BdbStorage::threadDelFuncWithIndex(Dbt &data, Dbt &key, double *timeOut)
{
	DbTxn *txn = NULL;
	Dbc *dbcp = NULL;
	Dbt content;

	PerformanceTimer timer("bdbThreadDelWithIndex", 0, 0, 0, false);
	uint32 i = 0;
	try{
		
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);

		boost::mutex::scoped_lock lock(m_mutex);
		// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		int ret = dbcp->get(&key, &content, DB_SET);
		while (ret != DB_NOTFOUND)
		{
#if 0
			std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "data:" << (char *)content.get_data() << "len:" << content.get_size() << std::endl;
#endif
			ret = dbcp->del(0);
			ret = dbcp->get(&key, &content, DB_NEXT_DUP);
		}

#if 0
		/// verify deletion operation
		ret = dbcp->get(&key, &content, DB_SET);
		if (DB_NOTFOUND != ret)
		{
			std::cerr << "del fail" << std::endl;
		}
#endif

		dbcp->close();
		txn->commit(DB_TXN_SYNC);

		
		//dbcp->close();
		//closeDB();
	}catch(DbException &ee){

		if(txn != NULL){
			txn->abort();
		}

		if(dbcp != NULL)
		{
			dbcp->close();
		}
		
		std::cerr << ee.what() << std::endl;
	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}

		if(dbcp != NULL)
		{
			dbcp->close();

		}

		std::cerr << "Unknown exception: " << ee.what() << std::endl;

	}
	*timeOut = timer.getElapse();
	return ;
}

void BdbStorage::threadUpdateFunc(Dbt &data, Dbt &DesData, Dbt &key, double *timeOut)
{
	DbTxn *txn = NULL;
	Dbc *dbcp = NULL;
	Dbt content;

	PerformanceTimer timer("bdbThreadUpdate", 0, 0, 0,false);
	uint32 i = 0;
	
	try{

		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);

		boost::mutex::scoped_lock lock(m_mutex);
		
		// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		uint32 lenData = data.get_size();
		int ret = dbcp->get(&key, &content, DB_SET);
		while (ret != DB_NOTFOUND)
		{
#if 0
			std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "src data:" << (char *)content.get_data() << "len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << "len:" << data.get_size() << std::endl;
#endif		
			uint32 lenContent = content.get_size();
			if(lenData != lenContent)
			{
				ret = dbcp->get(&key, &content, DB_NEXT);
				continue;
			}
			if(memcmp(content.get_data(), data.get_data(), lenData) == 0)
			{
				ret = dbcp->put(&key, &DesData, DB_CURRENT);
				ret = dbcp->get(&key, &content, DB_NEXT);
			}

		}
#if 0
		/// verify update operation
		ret = dbcp->get(&key, &content, DB_SET);
		int dataLen = data.get_size();
		int contentLen = content.get_size();
		int len = dataLen > contentLen ? contentLen : dataLen;
		if (memcmp(content.get_data(), data.get_data(), len) != 0)
		{
			std::cout << "src data:" << (char *)content.get_data() << ",len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << ",len:" << data.get_size() << std::endl;
			std::cerr << "updation fail" << std::endl;
		}
#endif

		dbcp->close();
		//g_mutex.unlock();

		txn->commit(DB_TXN_SYNC);


	}catch(DbException &ee){

		if(dbcp != NULL)
		{
			dbcp->close();
		}
		if(txn != NULL){
			txn->abort();
		}

		std::cerr <<"err:"<< ee.what() << std::endl;

	}catch(std::exception &ee){
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		if(txn != NULL){
			txn->abort();
		}

		std::cerr << "Unknown exception: " << ee.what() << std::endl;
		
	}
	*timeOut = timer.getElapse();
	return ;
}

void BdbStorage::threadUpdateFuncWithIndex(Dbt &data, Dbt &key, double *timeOut)
{
	DbTxn *txn = NULL;
	Dbc *dbcp = NULL;
	Dbt content;

	PerformanceTimer timer("bdbThreadUpdateWithIndex", 0, 0, 0,false);
	uint32 i = 0;
	
	try{

		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);

		boost::mutex::scoped_lock lock(m_mutex);
		
		// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		int ret = dbcp->get(&key, &content, DB_SET);
		while (ret != DB_NOTFOUND)
		{
#if 0
			std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "src data:" << (char *)content.get_data() << "len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << "len:" << data.get_size() << std::endl;
#endif
			ret = dbcp->put(&key, &data, DB_CURRENT);
			ret = dbcp->get(&key, &content, DB_NEXT_DUP);

		}
#if 0
		/// verify update operation
		ret = dbcp->get(&key, &content, DB_SET);
		int dataLen = data.get_size();
		int contentLen = content.get_size();
		int len = dataLen > contentLen ? contentLen : dataLen;
		if (memcmp(content.get_data(), data.get_data(), len) != 0)
		{
			std::cout << "src data:" << (char *)content.get_data() << ",len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << ",len:" << data.get_size() << std::endl;
			std::cerr << "updation fail" << std::endl;
		}
#endif

		dbcp->close();
		//g_mutex.unlock();

		txn->commit(DB_TXN_SYNC);


	}catch(DbException &ee){

		if(dbcp != NULL)
		{
			dbcp->close();
		}
		if(txn != NULL){
			txn->abort();
		}

		std::cerr <<"err:"<< ee.what() << std::endl;

	}catch(std::exception &ee){
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		if(txn != NULL){
			txn->abort();
		}


		std::cerr << "Unknown exception: " << ee.what() << std::endl;
		
	}
	*timeOut = timer.getElapse();
	return ;
}

void BdbStorage::threadScanFuncWithIndex(Dbt &data, Dbt &key, double *timeOut)
{
	DbTxn *txn = NULL;
	Dbc *dbcp;
	PerformanceTimer timer("bdbThreadScanWithIndex", 0, 0, 0,false);

	uint32 i = 0;
	try{
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		
				// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		int ret = dbcp->get(&key, &data, DB_SET);
		while (ret != DB_NOTFOUND)
		{


#if 0
			std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "date:" << (char *)data.get_data() << "len:" << data.get_size() << std::endl;
#endif
			ret = dbcp->get(&key, &data, DB_NEXT_DUP);
		}
		dbcp->close();

		txn->commit(DB_TXN_SYNC);
		//dbcp->close();
		//closeDB();
	}catch(DbException &ee){
		if(txn != NULL){
			txn->abort();
		}

		if(dbcp != NULL)
		{
			dbcp->close();
		}
		
		std::cerr << ee.what() << std::endl;

	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}

		std::cerr << "Unknown exception: " << ee.what() << std::endl;

	}
	*timeOut = timer.getElapse();
	return ;
}

void BdbStorage::threadBatchInsertFuncWithIndex(Dbt &data, Dbt &key, double *timeOut)
{
	PerformanceTimer timer("bdbThreadBatchInsertWithIndex", 0, 0, 0,false);

	DbTxn *txn = NULL;

	try{
		/// start up thread

		/// start up transaction
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		//openDB(txn);

		m_dbp->put(txn, &key, &data, DB_MULTIPLE);

		//closeDB();
		txn->commit(DB_TXN_SYNC);

	}catch(DbException &e){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}
		std::cerr << e.what() << std::endl;
		throw;
	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}


		std::cerr << "Unknown exception: " << ee.what() << std::endl;

		throw;
	}
	*timeOut = timer.getElapse();
	return ;
}

void BdbStorage::threadInsertFuncWithIndex(Dbt &data, Dbt &key, Dbt &lessData, Dbt &lessKey, double *timeOut)
{

	PerformanceTimer timer("bdbThreadInsertWithIndex", 0, 0, 0,false);

	DbTxn *txn = NULL;

	try{
		/// start up thread

		uint32 count = m_count / m_thread_nums;
		/// start up transaction
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		//openDB(txn);
		for(uint32 i = 0; i < count; ++i)
		{
			m_dbp->put(txn, &key, &data, 0);
		}
		for(uint32 i = 0; i< m_less_count; ++i)
		{
			m_dbp->put(txn, &lessKey, &lessData,0);
		}

		//closeDB();
		txn->commit(DB_TXN_SYNC);

	}catch(DbException &e){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}
		std::cerr << e.what() << std::endl;
		throw;
	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}


		std::cerr << "Unknown exception: " << ee.what() << std::endl;

		throw;
	}
	*timeOut = timer.getElapse();
	return ;
}


void BdbStorage::heap_update(Dbt &data, Dbt &DesData, Dbt &key)
{
	DbTxn *txn = NULL;
	Dbc *dbcp = NULL;
	Dbt content;

/// iterator updation from the first to the last
	uint32 i = 0;
	try{
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		openDB(txn);
	
				// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		uint32 lenData = data.get_size();
		int ret = dbcp->get(&key, &content, DB_SET);
		while (ret != DB_NOTFOUND)
		{
#if 0
			std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "src data:" << (char *)content.get_data() << "len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << "len:" << data.get_size() << std::endl;
#endif
			
			uint32 lenContent = content.get_size();
			if(lenData != lenContent)
			{
				ret = dbcp->get(&key, &content, DB_NEXT);
				continue;
			}
			if(memcmp(content.get_data(), data.get_data(), lenData) == 0)
			{
				ret = dbcp->put(&key, &DesData, DB_CURRENT);
				ret = dbcp->get(&key, &content, DB_NEXT);
			}
		} //while
#if 0
		/// verify update operation
		ret = dbcp->get(&key, &content, DB_SET);
		int dataLen = data.get_size();
		int contentLen = content.get_size();
		int len = dataLen > contentLen ? contentLen : dataLen;
		if (memcmp(content.get_data(), data.get_data(), len) != 0)
		{
			std::cout << "src data:" << (char *)content.get_data() << ",len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << ",len:" << data.get_size() << std::endl;
			std::cerr << "updation fail" << std::endl;
		}
#endif
		
		dbcp->close();
		closeDB();

		txn->commit(DB_TXN_SYNC);

	}catch(DbException &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp !=NULL)
		{
			closeDB();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		std::cerr << ee.what() << std::endl;

	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		if(m_dbp !=NULL)
		{
			closeDB();
		}

		std::cerr << "Unknown exception: " << ee.what() << std::endl;
		
	}
		return ;
}

void BdbStorage::index_update(Dbt &data, Dbt &key)
{
	DbTxn *txn = NULL;
	Dbc *dbcp = NULL;
	Dbt content;


	uint32 i = 0;
	try{
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		openDB(txn);
	
				// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		int ret = dbcp->get(&key, &content, DB_SET);
		while (ret != DB_NOTFOUND)
		{
#if 0
			std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "src data:" << (char *)content.get_data() << "len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << "len:" << data.get_size() << std::endl;
#endif
			ret = dbcp->put(&key, &data, DB_CURRENT);
			ret = dbcp->get(&key, &content, DB_NEXT_DUP);

		}
#if 0
		/// verify update operation
		ret = dbcp->get(&key, &content, DB_SET);
		int dataLen = data.get_size();
		int contentLen = content.get_size();
		int len = dataLen > contentLen ? contentLen : dataLen;
		if (memcmp(content.get_data(), data.get_data(), len) != 0)
		{
			std::cout << "src data:" << (char *)content.get_data() << ",len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << ",len:" << data.get_size() << std::endl;
			std::cerr << "updation fail" << std::endl;
		}
#endif
		
		dbcp->close();
		closeDB();

		txn->commit(DB_TXN_SYNC);

	}catch(DbException &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp !=NULL)
		{
			closeDB();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		std::cerr << ee.what() << std::endl;

	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		if(m_dbp !=NULL)
		{
			closeDB();
		}

		std::cerr << "Unknown exception: " << ee.what() << std::endl;
		
	}
		return ;
}


void BdbStorage::heap_del(Dbt &data, Dbt &key)
{
	DbTxn *txn = NULL;
	Dbc *dbcp = NULL;
	Dbt content;

/// iterator updation from the first to the last
	uint32 i = 0;
	try{
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		openDB(txn);
	
				// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		uint32 lenData = data.get_size();
		int ret = dbcp->get(&key, &content, DB_SET);
		while (ret != DB_NOTFOUND)
		{
#if 0
			std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "src data:" << (char *)content.get_data() << "len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << "len:" << data.get_size() << std::endl;
#endif
			
			uint32 lenContent = content.get_size();
			if(lenData != lenContent)
			{
				ret = dbcp->get(&key, &content, DB_NEXT);
				continue;
			}
			if(memcmp(content.get_data(), data.get_data(), lenData) == 0)
			{
				ret = dbcp->del(0);
				ret = dbcp->get(&key, &content, DB_NEXT);
			}
		}//while
#if 0
		/// verify update operation
		ret = dbcp->get(&key, &content, DB_SET);
		int dataLen = data.get_size();
		int contentLen = content.get_size();
		int len = dataLen > contentLen ? contentLen : dataLen;
		if (memcmp(content.get_data(), data.get_data(), len) != 0)
		{
			std::cout << "src data:" << (char *)content.get_data() << ",len:" << content.get_size() << std::endl;
			std::cout << "des data:" << (char *)data.get_data() << ",len:" << data.get_size() << std::endl;
			std::cerr << "updation fail" << std::endl;
		}
#endif
		
		dbcp->close();
		closeDB();

		txn->commit(DB_TXN_SYNC);

	}catch(DbException &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp !=NULL)
		{
			closeDB();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		std::cerr << ee.what() << std::endl;

	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		if(m_dbp !=NULL)
		{
			closeDB();
		}

		std::cerr << "Unknown exception: " << ee.what() << std::endl;
		
	}
		return ;
}


void BdbStorage::index_del(Dbt &data, Dbt &key)
{
	DbTxn *txn = NULL;
	Dbc *dbcp = NULL;
	Dbt content;


	uint32 i = 0;
	try{
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		openDB(txn);
				// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		int ret = dbcp->get(&key, &content, DB_SET);
		while (ret != DB_NOTFOUND)
		{
#if 0
			std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "data:" << (char *)content.get_data() << "len:" << content.get_size() << std::endl;
#endif
			ret = dbcp->del(0);
			ret = dbcp->get(&key, &content, DB_NEXT_DUP);
		}

#if 0
		/// verify deletion operation
		ret = dbcp->get(&key, &content, DB_SET);
		if (DB_NOTFOUND != ret)
		{
			std::cerr << "del fail" << std::endl;
		}
#endif

		dbcp->close();
		closeDB();
		txn->commit(DB_TXN_SYNC);
		//dbcp->close();
		//closeDB();
	}catch(DbException &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		std::cerr << ee.what() << std::endl;
		
		throw;
	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}

		std::cerr << "Unknown exception: " << ee.what() << std::endl;

		throw;
	}
		return ;
}

void BdbStorage::batch_insert(Dbt &data, Dbt &key)
{
	DbTxn *txn = NULL;
	try{
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		openDB(txn);
		//for(uint32 i = 0; i < m_count; ++i)
		//{
			m_dbp->put(txn, &key, &data, DB_MULTIPLE);
		//}

		txn->commit(DB_TXN_SYNC);
		closeDB();
	}catch(DbException &e){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}

		std::cerr << e.what() << std::endl;


		throw;
	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}


		std::cerr << "Unknown exception: " << ee.what() << std::endl;

		throw;
	}
		return ;
}

void BdbStorage::index_insert(Dbt &data, Dbt &key, Dbt &lessData, Dbt &lessKey)
{
	DbTxn *txn = NULL;
	try{
		
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		openDB(txn);
		for(uint32 i = 0; i < m_count; ++i)
		{
			m_dbp->put(txn, &key, &data, 0);
		}

		for(uint32 i = 0; i < m_less_count; ++i)
		{
			m_dbp->put(txn, &lessKey, &lessData, 0);
		}
		txn->commit(DB_TXN_SYNC);
		closeDB();
	}catch(DbException &e){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}

		std::cerr << e.what() << std::endl;


		throw;
	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}


		std::cerr << "Unknown exception: " << ee.what() << std::endl;

		throw;
	}
		return ;
}

void BdbStorage::index_scan(Dbt &data, Dbt &key)
{
	DbTxn *txn = NULL;
	Dbc *dbcp;


	uint32 i = 0;
	try{
		m_envp->txn_begin(NULL, &txn, DB_READ_COMMITTED);
		openDB(txn);
				// Acquire a cursor for the database.
		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);

		//int ret = dbcp->get(&key, &data, DB_SET);

		int ret = dbcp->get(&key, &data, DB_SET);
		while (ret != DB_NOTFOUND)
		{

#if 0
			//std::cout << "key:" << (char *)key.get_data() <<"len:" << key.get_size() << std::endl;
			std::cout << "date:" << (char *)data.get_data() << "len:" << data.get_size() << std::endl;
#endif
			ret = dbcp->get(&key, &data, DB_NEXT_DUP);
		}
		dbcp->close();
		closeDB();
		txn->commit(DB_TXN_SYNC);
		//dbcp->close();
		//closeDB();
	}catch(DbException &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}
		std::cerr<< ee.what() << std::endl;
		throw;
	}catch(std::exception &ee){
		if(txn != NULL){
			txn->abort();
		}
		if(m_dbp != NULL){
			closeDB();
		}
		if(dbcp != NULL)
		{
			dbcp->close();
		}

		std::cerr << "Unknown exception: " << ee.what() << std::endl;

		throw;
	}
		return ;
}

void init_DBT(DBT * key, DBT * data)
{
	memset(key, 0, sizeof(DBT));
	memset(data, 0, sizeof(DBT));
}

int BdbStorage::insert(void *data1, uint32 len, bool isC)
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;

		ret = db_env_create(&dbenv, 0);
		//print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为5M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024, 0);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/
		//dbenv->set_lk_detect(dbenv,DB_LOCK_MINWRITE);
	
		ret = dbenv->open(dbenv,"D:\\XMLDB\\bdbHome",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		//print_error(ret);
		ret = db_create(&db, dbenv, 0);
		//print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		//print_error(ret);


		init_DBT(&key, &data);
		data.data = data1;
		data.size = len;
		key.data = data1;
		key.size  = keyLen;


		{
			PerformanceTimer timer("bdbInsertWithIndex",m_count, 10);
			for (uint32 i = 0;i < m_count;i++)
			{
				//Dbt key(&i,sizeof(i));
				key.data = &i;
				key.size = sizeof(int);
				ret = db->put(db, NULL, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
				//print_error(ret);
			}

				if(ret == 0)
					printf("INSERT SUCCESS!\n");
				else
					printf("INSERT FAILED!\n");
				db->close(db, 0);
				dbenv->close(dbenv, 0);
		}
	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;	
}

//void BdbStorage::writeResToFile(const string &op,double timeOut, uint32 len)
//{
//	ofstream out("D:\\XMLDB\\res.txt", std::ios::out | std::ios::app);
//	if(!out.is_open())
//	{
//		cout << "open file failed" << endl;
//	}
//	out <<op<<":thread_nums:" << m_thread_nums<< "  ;lines:" << m_count << "  ,cost:" << timeOut  <<"  ,tupleLen:" <<len<< std::endl;
//}


//void BdbStorage::scan(void *data, uint32 len)
//	{
//	// We put a try block around this section of code
//	// to ensure that our database is properly closed
//	// in the event of an error.
//	//
//
//	try {
//		DbTxn *txn;
//		m_envp->txn_begin(NULL, &txn, 0);
//		// Acquire a cursor for the table.
//		Dbc *dbcp;
//		m_dbp->cursor(txn, &dbcp, DB_READ_COMMITTED);
//
//		// Walk through the table, printing the key/data pairs.
//		Dbt key;
//		Dbt data;
//		while (dbcp->get(&key, &data, DB_NEXT) == 0) {
//			char *key_string = (char *)key.get_data();
//			char *data_string = (char *)data.get_data();
//			cout << key_string << " : " << data_string << "\n";
//		}
//		dbcp->close();
//	}
//	catch (DbException &dbe) {
//		std::cerr << "AccessExample: " << dbe.what() << "\n";
//	}	
//}


} //namespace PgBdbPerformanceComp
} //namespace FounderXDB
