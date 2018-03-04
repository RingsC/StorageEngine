/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.9.14		 测试事务原子性，插入全部数据完成后，更新全部数据，另一个事务查询是否正确更新
001			许彦      创建		  2011.9.14		 测试事务原子性，先插入一些数据并提交，更新部分数据时取消事务，另起一个事务检查是否正确回滚
002			许彦      创建		  2011.9.14		 测试事务原子性，插入部分数据时强制停掉当前进程，事务此时未提交，另起一个进程检查事务是否正确回滚(未完成)

************************************************************************/

#include <iostream>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "transaction/test_transaction_update_dll.h"

using namespace FounderXDB::StorageEngineNS;

#define TRANSACTION_ROWS_UPDATE 10
#define TRANSACTION_DATA_UPDATE "testdata_"


//计算宏的数字位数，为了分配合适的内存大小
int numLenth(int num)
{
	int num_len = 0;
	for(int i = num ; i!=0 ; i/=10)
	{
		++num_len;
	}
	return num_len;
}

char *formData(int num,char *str)
{
	int num_len = numLenth(num);
	int len = strlen(str)+1;//该长度包含'\0'
	char *insert_str = (char*)malloc(len+num_len);
	return insert_str;
}

extern char *my_itoa(int value, char *string, int radix);

void insertData(EntrySet *pEntrySet,char *insert_str,vector<DataItem*> &dvec,char copy_insert_data[][20],uint32 nData,char *str)
{
	EntryID insert_tid;
	char string[20];
	vector<DataItem*>::size_type ix;
	int len;
	for( ix = 0; ix != nData; ++ix)
	{
		memcpy(insert_str, str, strlen(str)+1);//每次进入循环memcpy一次，保证每次都是从初始字符串开始连接字符
		my_itoa(ix,string,10);
		strcat(insert_str,string);
		memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
		DataItem *data = new DataItem;
		data->setData((void*)copy_insert_data[ix]);
		len = strlen((char*)insert_str);//长度不包含'\0'
		data->setSize(len+1);//为'\0'多分配一个字节
		dvec.push_back(data);
		pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//		cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
	}
}

bool test_transaction_update_dll_000()
{
	INTENT("测试事务原子性，插入全部数据完成后，更新全部数据，另一个事务查询是否正确更新");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(TRANSACTION_ROWS_UPDATE,TRANSACTION_DATA_UPDATE);
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char copy_insert_data[TRANSACTION_ROWS_UPDATE][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,TRANSACTION_ROWS_UPDATE,TRANSACTION_DATA_UPDATE);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pUpdateData = new DataItem;
		EntryID update_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		//更新数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,update_tid,*pUpdateData) == 0 )//*p为dataitem类型，输出参数
		{
			pEntrySet->updateEntry(pTransaction,update_tid,*dvec[0]);
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
		int data_flag,size_flag;
		//如果更新事务正确执行，应该查询到更新后的数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{
			data_flag = memcmp(dvec[0]->getData(),pScanData->getData(),pScanData->getSize());
			//CHECK_BOOL(data_flag == 0);
			if(data_flag != 0)
			{
				cout << "插入数据与查询数据内容不一致!" << endl;
				cout << "插入数据为" << (char*)dvec[0]->getData() << endl;	
				cout << "查询数据为" << (char*)pScanData->getData() << endl;
				HEAP_RETURN_FALSE
			}

			size_flag = dvec[0]->getSize() == pScanData->getSize() ? 0 : -1;//相等返回0.不等返回-1
			//CHECK_BOOL(size_flag == 0);
			if(size_flag != 0)
			{
				cout << "插入数据与查询数据长度不一致!" <<  size_flag <<endl;
				cout << "插入数据长度为" << dvec[0]->getSize() << endl;
				cout << "查询数据长度为" << pScanData->getSize() << endl;
				HEAP_RETURN_FALSE
			}	
			++counter;	
		}
		CHECK_BOOL(counter == TRANSACTION_ROWS_UPDATE );
		if(counter != TRANSACTION_ROWS_UPDATE)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS_UPDATE; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		delete pUpdateData;
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_update_dll_001()
{
	INTENT("测试事务原子性，先插入一些数据并提交，更新部分数据时取消事务，另起一个事务检查是否正确回滚");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(TRANSACTION_ROWS_UPDATE,TRANSACTION_DATA_UPDATE);
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char copy_insert_data[TRANSACTION_ROWS_UPDATE][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,TRANSACTION_ROWS_UPDATE,TRANSACTION_DATA_UPDATE);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pUpdateData = new DataItem;
		EntryID update_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		ix = 0;
		//更新数据
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,update_tid,*pUpdateData) == 0 )//*p为dataitem类型，输出参数
		{
			//更新部分数据后，中止当前事务
			if(ix == TRANSACTION_ROWS_UPDATE/2)
			{
				pEntrySet->endEntrySetScan(pHeapScan);
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				user_abort_transaction();
				break;
			}
			pEntrySet->updateEntry(pTransaction,update_tid,*dvec[0]);
			++ix;
		}

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;
		ix = 0;
		//如果更新事务正确回滚，应该查询不到更新后的数据
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
				cout << "插入数据与查询数据长度不一致!" <<  size_flag <<endl;
				cout << "插入数据长度为" << dvec[ix]->getSize() << endl;
				cout << "查询数据长度为" << pScanData->getSize() << endl;
				HEAP_RETURN_FALSE
			}	
			++counter;	
			++ix;
		}
		CHECK_BOOL(counter == TRANSACTION_ROWS_UPDATE );
		if(counter != TRANSACTION_ROWS_UPDATE)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS_UPDATE; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		delete pUpdateData;
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}