#include <iostream>
#include <fstream>


#include "pg/pg_insert.h"
#include "utils/performance_timer.h"

#include "StorageEngineException.h"
#include "StorageEngine.h"

#include "bdb/bdb_insert.h"

using FounderXDB::StorageEngineNS::StorageEngineException;
using FounderXDB::StorageEngineNS::StorageEngine;

using std::ifstream;


using std::cout;
using std::endl;
using std::cin;


using namespace FounderXDB::PgBdbPerformanceComp;



//const uint32 dataLen = 500;

int main()
{
//#ifdef WIN32
//	ifstream inFile("D:\\XMLDB\\indata.txt");
//#else
//	ifstream inFile("home/fd/indata.txt");
//#endif
//
//	///get input data
//	uint32 dataLen = 500;
//	cout << "input data len:";
//	cin >> dataLen;
//	char *data = new char[dataLen];	
//	memset((void *)data, '\0', dataLen);
//	
//	uint32 lessLen = sizeof("tesrteaawereqqewr");
//	char *lessData = new char[lessLen];
//	memcpy((void *)lessData, "tesrteaawereqqewr", lessLen);
//	
//	if(!inFile.eof()){
//		inFile.get(data, dataLen, 'A');
//	}
//
//	uint32 tuple_nums = 1;
//	cout << "input tuple numbers (1->1W):";
//	cin >> tuple_nums;
//	tuple_nums *= 10000;
//
//	uint32 thread_nums = 1;
//	cout << "input thread nums:";
//	cin >> thread_nums;
//
//
//	bool hasIndex = false;
//	uint32 lessNums = 10;
//	///// start StorageEngine 
//	try
//	{
//
//	uint32 thread_num = 80;
//	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
//#ifdef WIN32
//	pStorageEngine->initialize("D:\\XMLDB\\data", thread_num);
//#else
//	pStorageEngine->initialize("../data", thread_num);
//#endif
//
//
//		///// cost time of pg,table hasn't index
//		PgStorage pg(tuple_nums, pStorageEngine, thread_nums, false, lessNums);
//
//		pg.insert((void *)data, dataLen, (void *)lessData, lessLen);
//		pg.scan((void *)lessData, lessLen);
//		pg.scan((void*)data, dataLen);
//		pg.update((void *)lessData, lessLen);
//		pg.del((void*)data, dataLen);
//		pg.batchInsert((void *)data, dataLen, tuple_nums);
//    	pg.insertFirstIndexLast((void *)data, dataLen);
//
//		/// cost time of multi thread insert operation
//		pg.threadInsert((void *)data, dataLen,(void *)lessData, lessLen); 
//		pg.threadScan((void *)lessData, lessLen);
//		pg.threadScan((void *)data, dataLen);
//		pg.threadUpdate((void *)lessData, lessLen);
//		pg.threadDel((void *)data, dataLen);
//		pg.threadBatchInsert((void *)data, dataLen, tuple_nums);
//		pg.threadInsertFirstIndexLast((void *)data, dataLen);
//
//	///// cost time of pg,table with index
//		PgStorage pgIndex(tuple_nums, pStorageEngine, thread_nums, true, lessNums);
//		pgIndex.insert((void *)data, dataLen, (void *)lessData, lessLen);
//		pgIndex.scan((void *)lessData, lessLen);
//		pgIndex.scan((void*)data, dataLen);
//		pgIndex.update((void *)lessData, lessLen);
//		pgIndex.del((void*)data, dataLen);
//		pgIndex.batchInsert((void *)data, dataLen, tuple_nums);
//
//		/// cost time of multi thread insert operation	
//		pgIndex.threadInsert((void *)data, dataLen,(void *)lessData, lessLen); 
//		pgIndex.threadScan((void *)lessData, lessLen);
//		pgIndex.threadScan((void *)data, dataLen);
//		pgIndex.threadUpdate((void *)lessData, lessLen);
//		pgIndex.threadDel((void *)data, dataLen);
//		pgIndex.threadBatchInsert((void *)data, dataLen, tuple_nums);
//
//
///// bdb operation table has not index
//		BdbStorage bdb(tuple_nums, thread_nums, false, NULL, NULL,lessNums);
//		bdb.insert((void *)data, dataLen, (void *)lessData, lessLen);
//		bdb.scan((void *)lessData, lessLen);
//		bdb.scan((void *)data, dataLen);
//		bdb.update((void *)lessData, lessLen);
//		bdb.del((void *)data, dataLen);
//		
//		bdb.threadInsert((void *)data, dataLen, (void *)lessData, lessLen); 
//		bdb.threadScan((void *)lessData, lessLen);
//		bdb.threadScan((void *)data, dataLen);
//		bdb.threadUpdate((void *)lessData, lessLen);
//		bdb.threadDel((void *)data, dataLen);
//
//		/// bdb operation table has index
//		BdbStorage bdbIndex(tuple_nums, thread_nums, true, NULL, NULL,lessNums);		
//		bdbIndex.insert((void *)data, dataLen, (void *)lessData, lessLen);
//		bdbIndex.scan((void *)lessData, lessLen);
//		bdbIndex.scan((void *)data, dataLen);
//		bdbIndex.update((void *)lessData, lessLen);
//		bdbIndex.del((void *)data, dataLen);
//		bdbIndex.batchInsert((void *)data, dataLen, tuple_nums);
//			
//		bdbIndex.threadInsert((void *)data, dataLen, (void *)lessData, lessLen); 
//		bdbIndex.threadScan((void *)lessData, lessLen);
//		bdbIndex.threadScan((void *)data, dataLen);                              	
//		bdbIndex.threadUpdate((void *)lessData, lessLen);
//		bdbIndex.threadDel((void *)data, dataLen); 
//		bdbIndex.threadBatchInsert((void *)data, dataLen, tuple_nums);
//	} catch(DbException &e)
//	{
//		std::cerr << e.what() << endl;	
//	}
//
//	catch (StorageEngineException &e)
//	{
//		std::cerr << e.what() << endl;	
//	}
//
//	inFile.close();
//	std::cout << "end" <<std::endl;
	return 0;
}

