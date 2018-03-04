/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.9.19		 测试事务原子性，先插入数据，然后删除全部数据，另一个事务查询是否正确删除
001			许彦      创建		  2011.9.19		 测试事务原子性，先插入数据，然后删除部分数据时取消事务，另起一个事务检查是否正确回滚
002			许彦      创建		  2011.9.19		 测试事务原子性，先插入数据，然后删除部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚(未完成)

************************************************************************/

#include <boost/thread/thread.hpp>

#include <iostream>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "transaction/test_transaction_delete_dll.h"

using namespace FounderXDB::StorageEngineNS;

extern char *my_itoa(int value, char *string, int radix);

#define TRANSACTION_ROWS_DELETE 10
bool test_transaction_delete_dll_000()
{
	INTENT("测试删除事务原子性，删除全部数据完成后，另一个事务查询是否正确删除。");
	try
 	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int i = TRANSACTION_ROWS_DELETE; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//该长度已包含'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS_DELETE][20];
		for( ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
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
		DataItem *pDeleteData = new DataItem;
		EntryID delete_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		//删除数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,delete_tid,*pDeleteData) == 0 )//*p为dataitem类型，输出参数
		{
			pEntrySet->deleteEntry(pTransaction,delete_tid);
		}
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();
	
		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//如果删除事务正确执行，应该查询不到数据
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
		
		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		delete pDeleteData;
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_delete_dll_001()
{
	INTENT("测试事务原子性，先插入数据，然后删除部分数据时取消事务，另起一个事务检查是否正确回滚");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int i = TRANSACTION_ROWS_DELETE; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//该长度已包含'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS_DELETE][20];
		for( ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
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
		DataItem *pDeleteData = new DataItem;
		EntryID delete_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		ix = 0;
		//删除数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,delete_tid,*pDeleteData) == 0 )//*p为dataitem类型，输出参数
		{
			//删除部分数据后，中止当前事务
			if(ix == TRANSACTION_ROWS_DELETE/2)
			{
				pEntrySet->endEntrySetScan(pHeapScan);
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				user_abort_transaction();
				break;
			}
			pEntrySet->deleteEntry(pTransaction,delete_tid);
			++ix;
		}
		
		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//如果删除事务正确回滚，应该查询到全部插入数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{
			++counter;
		}
		CHECK_BOOL( counter == TRANSACTION_ROWS_DELETE );
		if( counter != TRANSACTION_ROWS_DELETE )
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData;
		delete pDeleteData;
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_delete_dll_002_step1()
{
	INTENT("测试事务原子性，先插入数据，然后删除部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int i = TRANSACTION_ROWS_DELETE; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//该长度已包含'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS_DELETE][20];
		for( ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
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
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pDeleteData = new DataItem;
		EntryID delete_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		ix = 0;
		//删除数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,delete_tid,*pDeleteData) == 0 )//*p为dataitem类型，输出参数
		{
			//删除部分数据后，中止当前进程
			if(ix == TRANSACTION_ROWS_DELETE/2)
			{
				pEntrySet->endEntrySetScan(pHeapScan);
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				exit(1);
			}
			pEntrySet->deleteEntry(pTransaction,delete_tid);
			++ix;
		}
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_delete_dll_002_step2()
{
	INTENT("测试事务原子性，先插入数据，然后删除部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		
		//如果删除事务正确回滚，应该查询到全部插入数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{
			++counter;
		}
		CHECK_BOOL( counter == TRANSACTION_ROWS_DELETE );
		if( counter != TRANSACTION_ROWS_DELETE )
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

DLL_SIMPLE_EXTERN_FUNC

int test_trans_persistence_update()
{
	INTENT("建表并插入若干数据，随后删除部分数据，结束进程。"
				 "重启进程，测试是否数据正确删除，没被删除的数据是"
				 "否还存在。");

	DLL_PREPARE_TEST();

	const string T_DATA = "diedurmjdifjrm%%%$$$###@@@!!!**&&djeimd";
	const string DET_DATA = "det$$$$$$$$$$$$$JJ";

#undef ARR_LEN 
#define ARR_LEN 100

	SpliterGenerater sg;
	ColumnInfo *colinfo = sg.buildHeapColInfo(2, 1, 3);
	int col_id = get_col_id();
	setColumnInfo(col_id, colinfo);
	char data[ARR_LEN][DATA_LEN];
	memset(data, 0, ARR_LEN * DATA_LEN);

	/* 构建测试数据 */
	for(int i = 0; i < ARR_LEN; ++i)
	{
		memcpy(data[i], T_DATA.c_str(), T_DATA.length());
	}

	/*
	* 准备数据
	* 插入数据并退出进程
	*/
	SHUTDOWN_TEST_STEP_1(TransPersistence)
	{ 
		EntrySetID ei;
		DLL_SIMPLE_CREATE_HEAP(ei, col_id);
		SAVE_HEAP_ID_FOR_NEXT(ei);
		int insert_data = 0;
		/* 插入数据 */
		DLL_SIMPLE_INSERT_DATA(ei, data, ARRAY_LEN_CALC(data), insert_data);
		int update_data = 0;
		/* 更新数据，将所有的数据更新成DET_DATA */
		DLL_SIMPLE_UPDATE_DATA(ei, DET_DATA.c_str(), DET_DATA.length(), T_DATA.c_str(), T_DATA.length(), update_data);
		return true;
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
	int test_success = false;
	vector<string> v_src, v_det;
	for(int i = 0; i < ARR_LEN; ++i)
	{
		v_src.push_back(DET_DATA);
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

int test_trans_persistence_delete()
{
	INTENT("创建表并插入数据，随后删除部分数据，结束进程。"
				 "重启进程，扫描表，测试未删除的数据是否还存在。");

	DLL_PREPARE_TEST();

	const string D_DATA = "delete me";
	const string N_DATA = "do not delete me";

#undef ARR_LEN 
#define ARR_LEN 100

	SpliterGenerater sg;
	ColumnInfo *colinfo = sg.buildHeapColInfo(2, 1, 3);
	int col_id = get_col_id();
	setColumnInfo(col_id, colinfo);
	char data[ARR_LEN][DATA_LEN];
	memset(data, 0, ARR_LEN * DATA_LEN);

	/* 构建测试数据,插入ARR_LEN - 2条，留下两条保留给未删除的数据 */
	for(int i = 0; i < ARR_LEN - 2; ++i)
	{
		memcpy(data[i], D_DATA.c_str(), D_DATA.length());
	}

	/* 构建未删除数据 */
	memcpy(data[ARR_LEN - 2], N_DATA.c_str(), N_DATA.length());
	memcpy(data[ARR_LEN - 1], N_DATA.c_str(), N_DATA.length());

	/*
	* 准备数据
	* 插入数据并退出进程
	*/
	SHUTDOWN_TEST_STEP_1(TransPersistence)
	{ 
		EntrySetID ei;
		DLL_SIMPLE_CREATE_HEAP(ei, col_id);
		SAVE_HEAP_ID_FOR_NEXT(ei);
		int insert_data = 0;
		/* 插入数据 */
		DLL_SIMPLE_INSERT_DATA(ei, data, ARRAY_LEN_CALC(data), insert_data);
		int delete_data = 0;
		/* 删除所有的D_DATA,保留所有的N_DATA */
		DLL_SIMPLE_DELETE_DATA(ei, D_DATA.c_str(), D_DATA.length(), delete_data);
		return true;
	}
	/*
	* 检查数据
	*/
	SHUTDOWN_TEST_STEP_2()
	int test_success = false;
	vector<string> v_src, v_det;
	for(int i = 0; i < 2; ++i)
	{
		v_src.push_back(N_DATA);
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