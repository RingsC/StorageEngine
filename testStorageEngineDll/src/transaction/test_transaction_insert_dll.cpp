/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.9.14		 测试事务原子性，插入部分数据时取消事务，另起一个事务检查是否正确回滚
001			许彦      创建		  2011.9.14		 测试事务原子性，插入全部数据完成后，另一个事务查询是否正确插入
002			许彦      创建		  2011.9.14		 测试事务原子性，插入部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚(未完成)
003			许彦      创建		  2011.9.20		 测试事务原子性，先插入一些数据并提交，再插入部分数据时取消事务，另起一个事务检查是否正确回滚

************************************************************************/
#include <boost/thread/thread.hpp>

#include <iostream>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "transaction/test_transaction_insert_dll.h"
using namespace FounderXDB::StorageEngineNS;

extern char *my_itoa(int value, char *string, int radix);

bool test_transaction_insert_dll_000()
{
	INTENT("测试事务原子性，插入部分数据时取消事务，另起一个事务检查是否正确回滚");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int i = TRANSACTION_ROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//该长度已包含'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			//插入部分数据后，中止当前事务
			if(ix == TRANSACTION_ROWS/2)
			{
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				free(insert_str);
				user_abort_transaction();
				
				break;
			}
			memcpy(insert_str, "testdata_", sizeof("testdata_"));//每次进入循环memcpy一次，保证每次都是从初始字符串开始连接字符
			my_itoa(ix,string,10);
			strcat(insert_str,string);
			memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
			DataItem *data = new DataItem;
			data->setData((void*)copy_insert_data[ix]);
			len = strlen((char*)insert_str);//长度不包含'\0'
			data->setSize(len+1);//为'\0'多分配一个字节
			dvec.push_back(data);
			pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//			cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
		}

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//如果事务正确回滚，应该查询不到数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{
			++counter;
		}
		CHECK_BOOL(counter == 0 );
		if(counter != 0)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		
		for(ix = 0; ix != TRANSACTION_ROWS/2; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_insert_dll_001()
{
	INTENT("测试事务原子性，插入全部数据完成后，另一个事务查询是否正确插入");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int i = TRANSACTION_ROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//该长度已包含'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			memcpy(insert_str, "testdata_", sizeof("testdata_"));//每次进入循环memcpy一次，保证每次都是从初始字符串开始连接字符
			my_itoa(ix,string,10);
			strcat(insert_str,string);
			memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
			DataItem *data = new DataItem;
			data->setData((void*)copy_insert_data[ix]);
			len = strlen((char*)insert_str);//长度不包含'\0'
			data->setSize(len+1);//为'\0'多分配一个字节
			dvec.push_back(data);
			pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//			cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;
		ix = 0;
		//如果事务正确完成，应该查询到全部插入数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{
			data_flag = memcmp(dvec[ix]->getData(),pScanData->getData(),pScanData->getSize());
			//CHECK_BOOL(data_flag == 0);
			if(data_flag != 0)
			{
				cout << "插入数据与查询数据内容不一致!" << endl;
				cout << "插入数据为" << (char*)dvec[ix]->getData() << endl;	
				cout << "查询数据为" << (char*)pScanData->getData() << endl;
				HEAP_RETURN_FALSE
			}

			size_flag = dvec[ix]->getSize() == pScanData->getSize() ? 0 : -1;//相等返回0.不等返回-1
			//CHECK_BOOL(size_flag == 0);
			if(size_flag != 0)
			{
				cout << "插入数据与查询数据长度不一致!" << endl;
				cout << "插入数据长度为" << dvec[ix]->getSize() << endl;
				cout << "查询数据长度为" << pScanData->getSize() << endl;
				HEAP_RETURN_FALSE
			}					
			++counter;	
			++ix;	
		}
		CHECK_BOOL(counter == TRANSACTION_ROWS );
		if(counter != TRANSACTION_ROWS)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_insert_dll_002_step1()
{
	INTENT("测试事务原子性，插入部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int i = TRANSACTION_ROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//该长度已包含'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			//插入部分数据后，中止当前事务
			if(ix == TRANSACTION_ROWS/2)
			{
				free(insert_str); 
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				exit(1);
			}
			memcpy(insert_str, "testdata_", sizeof("testdata_"));//每次进入循环memcpy一次，保证每次都是从初始字符串开始连接字符
			my_itoa(ix,string,10);
			strcat(insert_str,string);
			memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
			DataItem *data = new DataItem;
			data->setData((void*)copy_insert_data[ix]);
			len = strlen((char*)insert_str);//长度不包含'\0'
			data->setSize(len+1);//为'\0'多分配一个字节
			dvec.push_back(data);
			pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//			cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
		}
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_insert_dll_002_step2()
{
	INTENT("测试事务原子性，插入部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//如果事务正确回滚，应该查询不到数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{
			++counter;
		}
		CHECK_BOOL(counter == 0 );
		if(counter != 0)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}
	
		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_insert_dll_003()
{
	INTENT("测试事务原子性，先插入一些数据并提交，再插入部分数据时取消事务，另起一个事务检查是否正确回滚");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int i = TRANSACTION_ROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//该长度已包含'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			memcpy(insert_str, "testdata_", sizeof("testdata_"));//每次进入循环memcpy一次，保证每次都是从初始字符串开始连接字符
			my_itoa(ix,string,10);
			strcat(insert_str,string);
			memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
			DataItem *data = new DataItem;
			data->setData((void*)copy_insert_data[ix]);
			len = strlen((char*)insert_str);//长度不包含'\0'
			data->setSize(len+1);//为'\0'多分配一个字节
			dvec.push_back(data);
			pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//			cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务

		get_new_transaction();
		pEntrySet = open_entry_set();
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			//插入部分数据后，中止当前事务
			if(ix == TRANSACTION_ROWS/2)
			{
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				user_abort_transaction();
				break;
			}
			memcpy(insert_str, "testdata_", sizeof("testdata_"));//每次进入循环memcpy一次，保证每次都是从初始字符串开始连接字符
			my_itoa(ix,string,10);
			strcat(insert_str,string);
			memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
			DataItem *data = new DataItem;
			data->setData((void*)copy_insert_data[ix]);
			len = strlen((char*)insert_str);//长度不包含'\0'
			data->setSize(len+1);//为'\0'多分配一个字节
			dvec.push_back(data);
			pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//			cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
		}

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
//		pEntrySet = open_entry_set();
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//如果事务正确回滚，应该查询不到第2次插入的数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{
			++counter;
		}
		CHECK_BOOL(counter == TRANSACTION_ROWS );
		if(counter != TRANSACTION_ROWS)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str);
		for(ix = 0; ix != TRANSACTION_ROWS+TRANSACTION_ROWS/2; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

DLL_SIMPLE_EXTERN_FUNC

int test_trans_persistence_insert()
{
	INTENT("创建表并插入若干数据。提交事务并结束进程。"
		"重启进程，打开表查询插入的数据以测试事务的"
		"持久性。");

	extern EntrySetID EID;

	DLL_PREPARE_TEST();

	SpliterGenerater sg;
	ColumnInfo *colinfo = sg.buildHeapColInfo(1, 3);
	int col_id = get_col_id();
	setColumnInfo(col_id, colinfo);
	char data[][DATA_LEN] = 
	{
		"data1",
		"data2",
		"data3",
		"aaaaaaatest"
	};

	/*
	* 准备数据
	*/
	SHUTDOWN_TEST_STEP_1(TransPersistence)
	{
		EntrySetID ei;
		DLL_SIMPLE_CREATE_HEAP(ei, col_id);
		SAVE_HEAP_ID_FOR_NEXT(ei);
		int insert_data = 0;
		/* 插入若干数据，等待下次引擎启动做检查 */
		DLL_SIMPLE_INSERT_DATA(ei, data, ARRAY_LEN_CALC(data), insert_data);
		return true;
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
		int test_success = true;
		vector<string> v_src, v_det;
		for(int i = 0; i < ARRAY_LEN_CALC(data); ++i)
		{
			v_src.push_back(data[i]);
		}
		ShutdownMap m_hi;
		GET_RELATION_INFO(m_hi);
		Transaction *trans = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *es = (EntrySet*)pStorageEngine->openEntrySet(trans, EntrySet::OPEN_EXCLUSIVE, m_hi.begin()->first, NULL);
		vector<ScanCondition> vc;
		EntrySetScan *ess = es->startEntrySetScan(trans,BaseEntrySet::SnapshotMVCC,vc);
		EntryID ei;
		DataItem di;
		/* 扫描entryset，将所有内容放入vector中 */
		while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
		{
			v_det.push_back((char*)di.getData());
		}
		es->endEntrySetScan(ess);
		pStorageEngine->closeEntrySet(trans, es);
		pStorageEngine->removeEntrySet(trans, es->getId());
		trans->commit();

		/* 比较数据是否都正确 */
		sort(v_src.begin(), v_src.end());
		sort(v_det.begin(), v_det.end());

		test_success = (v_src == v_det);
		CLEAN_SHUTDOWN_TEST();
		return test_success;
	END_SHUTDOWN_TEST()
}
