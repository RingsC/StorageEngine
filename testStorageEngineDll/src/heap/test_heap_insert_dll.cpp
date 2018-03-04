/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.9.9		 测试插入一行数据是否正确
001			许彦	  创建		  2011.9.12		 测试插入多行数据是否正确
002			许彦	  创建		  2011.9.13		 测试插入数据长度较大时是否正确
003			许彦	  创建		  2011.9.13		 测试插入数据中包含空串的情况
004			许彦	  创建		  2011.9.13		 设置数据长度参数为小于或大于数据实际长度，测试插入数据是否正确
005			许彦	  创建		  2011.9.13		 测试插入大量数据行是否正确
006 		许彦	  创建		  2012.1.4		 连续多次打开同一个entryset，再多次关闭


************************************************************************/

#include <iostream>

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "PGSETypes.h"
#include "heap/test_heap_insert_dll.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;

using namespace FounderXDB::StorageEngineNS;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
EntrySetID EID = 0;

void my_spilt(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
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
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 1;
	}
}

void createTable()
{
	try
	{
		get_new_transaction();
		string entrySetName("test");
		EntrySet *pEntrySet = NULL;
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt;
		uint32 colid = 5;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		EID = pStorageEngine->createEntrySet(pTransaction, colid, 0,NULL);//创建一个heap
		commit_transaction();
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorMsg() << endl;
		cout << ex.getErrorNo() << endl;
	}
}

void dropTable()
{
	try
	{
		get_new_transaction();
		pStorageEngine->removeEntrySet(pTransaction,EID);//删除heap
		commit_transaction();
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorMsg() << endl;
		cout << ex.getErrorNo() << endl;
	}
}

EntrySet *open_entry_set()
{
	return static_cast<EntrySet *>(pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,EID));
}

EntrySetScan *heap_begin_scan(EntrySet *pEntrySet)
{
	return pEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,(vector<ScanCondition>) NULL);
}

char *my_itoa(int value, char *string, int radix)
{
	char tmp[33];
	char *tp = tmp;
	int i;
	unsigned v;
	int sign;
	char *sp;

	if (radix > 36 || radix <= 1)
	{
//		__set_errno(EDOM);
		return 0;
	}

	sign = (radix == 10 && value < 0);
	if (sign)
		v = -value;
	else
		v = (unsigned)value;
	while (v || tp == tmp)
	{
		i = v % radix;
		v = v / radix;
		if (i < 10)
			*tp++ = i+'0';
		else
			*tp++ = i + 'a' - 10;
	}

	if (string == 0)
		string = (char *)malloc((tp-tmp)+sign+1);
	sp = string;

	if (sign)
		*sp++ = '-';
	while (tp > tmp)
		*sp++ = *--tp;
	*sp = 0;
	return string;
}



bool test_heap_insert_dll_000()
{
	INTENT("测试插入一行数据是否正确");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		DataItem *pInsertData = new DataItem;
		int len = sizeof("testdata_1");
		char *insert_str = (char *)malloc(len);
		memcpy(insert_str, "testdata_1", len);
		pInsertData->setData((void *)insert_str);
		pInsertData->setSize(len);

		EntryID insert_tid;
		pEntrySet->insertEntry(pTransaction, insert_tid, *pInsertData);//data为const dataitem类型，输入参数
		command_counter_increment();

		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{
			++counter;
			data_flag = memcmp(pInsertData->getData(),pScanData->getData(),pScanData->getSize());
			//CHECK_BOOL(data_flag == 0);
			if(data_flag != 0)
			{
				cout << "插入数据与查询数据内容不一致!" << endl;
				cout << "插入数据为" << (char*)pInsertData->getData() << endl;
				cout << "查询数据为" << (char*)pScanData->getData() << endl;
				HEAP_RETURN_FALSE
			}
			
			size_flag = pInsertData->getSize() == pScanData->getSize() ? 0 : -1;//相等返回0.不等返回-1
			//CHECK_BOOL(size_flag == 0);
			if(size_flag != 0)
			{
				cout << "插入数据与查询数据长度不一致!" << endl;
				cout << "插入数据长度为" << pInsertData->getSize() << endl;
				cout << "查询数据长度为" << pScanData->getSize() << endl;
				HEAP_RETURN_FALSE
			}					
		}
		CHECK_BOOL(counter == 1);
		if(counter != 1)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			HEAP_RETURN_FALSE
		}

 		free(insert_str);
		delete pInsertData;  		
 		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
 	CATCHEXCEPTION
	return true;
}

#define DATAROWS 10
bool test_heap_insert_dll_001()
{
	INTENT("测试插入多行数据是否正确");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		
		int num_counter = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int i = DATAROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//该长度已包含'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[DATAROWS][20];
		for( ix = 0; ix != DATAROWS; ++ix)
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
		
		command_counter_increment();

		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;
		ix = 0;

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
		CHECK_BOOL(counter == DATAROWS );
		if(counter != DATAROWS)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE			
		}

		free(insert_str);
 		for(ix = 0; ix != DATAROWS; ++ix)
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

bool test_heap_insert_dll_002()
{
	INTENT("测试插入数据长度较大时是否正确");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		DataItem *pInsertData = new DataItem;
		int len = sizeof(test_insert_long_data_len_1020);
		char *insert_str = (char *)malloc(len);
		memcpy(insert_str, test_insert_long_data_len_1020, len);
		pInsertData->setData((void *)insert_str);
		pInsertData->setSize(len);

		EntryID insert_tid;
		pEntrySet->insertEntry(pTransaction, insert_tid, *pInsertData);//data为const dataitem类型，输入参数

		command_counter_increment();

		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{
			++counter;
			data_flag = memcmp(pInsertData->getData(),pScanData->getData(),pScanData->getSize());
			//CHECK_BOOL(data_flag == 0);
			if(data_flag != 0)
			{
				cout << "插入数据与查询数据内容不一致!" << endl;
				cout << "插入数据为" << (char*)pInsertData->getData() << endl;
				cout << "查询数据为" << (char*)pScanData->getData() << endl;
				HEAP_RETURN_FALSE
			}

			size_flag = pInsertData->getSize() == pScanData->getSize() ? 0 : -1;//相等返回0.不等返回-1
			//CHECK_BOOL(size_flag == 0);
			if(size_flag != 0)
			{
				cout << "插入数据与查询数据长度不一致!" << endl;
				cout << "插入数据长度为" << pInsertData->getSize() << endl;
				cout << "查询数据长度为" << pScanData->getSize() << endl;
				HEAP_RETURN_FALSE
			}					
		}
		CHECK_BOOL(counter == 1);
		if(counter != 1)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str);
		delete pInsertData; 
		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

bool test_heap_insert_dll_003()
{
	INTENT("测试插入数据中包含空串的情况");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		char insert_data[20][20] = {"", "\0\0\0\0\0", "\0\0abcde\0\0", "abc\0\0de\0\0"};

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix = 0;
			
		for( int i = 0; i < 4; ++ix,++i)
		{
			DataItem *data = new DataItem;
			data->setData((void*)insert_data[i]);
			data->setSize(sizeof(insert_data[i]));  //长度为20
			dvec.push_back(data);
			pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//			cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
		}

		command_counter_increment();

		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;
		ix = 0;
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
				cout << "插入数据与查询数据长度不一致!" <<endl;
				cout << "插入数据长度为" << dvec[ix]->getSize() << endl;
				cout << "查询数据长度为" << pScanData->getSize() << endl;
				HEAP_RETURN_FALSE
			}					
			++counter;	
			++ix;
		}			
		
		CHECK_BOOL(counter == 4 );
		if(counter != 4)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}
	
		for(ix = 0; ix != 4; ++ix)
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

bool test_heap_insert_dll_004()
{
	INTENT("设置数据长度参数为小于或大于数据实际长度，测试插入数据是否正确");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		DataItem *pInsertData = new DataItem;
		int len = sizeof("testdata_1");
		char *insert_str = (char *)malloc(len);
		memcpy(insert_str, "testdata_1", len);
		pInsertData->setData((void *)insert_str);
		pInsertData->setSize(len-5); //设置数据长度小于实际长度

		DataItem *pInsertData2 = new DataItem;
		int len2 = sizeof("testdata_2");
		char *insert_str2 = (char *)malloc(len2);
		memcpy(insert_str2, "testdata_2", len2);
		pInsertData2->setData((void *)insert_str2);
		pInsertData2->setSize(len+5); //设置数据长度大于实际长度

		EntryID insert_tid;
		pEntrySet->insertEntry(pTransaction, insert_tid, *pInsertData);//data为const dataitem类型，输入参数
		pEntrySet->insertEntry(pTransaction, insert_tid, *pInsertData2);

		command_counter_increment();

		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;

		pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData);//*p为dataitem类型，输出参数
//		data_flag = memcmp(pInsertData->getData(),pScanData->getData(),pScanData->getSize());
		data_flag = memcmp(pInsertData->getData(),pScanData->getData(),len-5);//比较应该相等
		CHECK_BOOL(data_flag == 0);
		if(data_flag != 0)
		{
			cout << "插入数据与查询数据内容不一致!" << endl;
			cout << "插入数据为" << (char*)pInsertData->getData() << endl;
			cout << "查询数据为" << (char*)pScanData->getData() << endl;
			HEAP_RETURN_FALSE
		}
		data_flag = memcmp(pInsertData->getData(),pScanData->getData(),len-4);//比较应该不相等
		CHECK_BOOL(data_flag != 0);
		if(data_flag == 0)
		{
			cout << "插入数据与查询数据内容不一致!" << endl;
			cout << "插入数据为" << (char*)pInsertData->getData() << endl;
			cout << "查询数据为" << (char*)pScanData->getData() << endl;
			HEAP_RETURN_FALSE
		}
		size_flag = pInsertData->getSize() == pScanData->getSize() ? 0 : -1;//相等返回0.不等返回-1
		CHECK_BOOL(size_flag == 0);
		if(size_flag != 0)
		{
			cout << "插入数据与查询数据长度不一致!" << endl;
			cout << "pInsertData插入数据长度为" << pInsertData->getSize() << endl;
			cout << "查询数据长度为" << pScanData->getSize() << endl; 
			HEAP_RETURN_FALSE
		}					
		
		pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData);
//		data_flag = memcmp(pInsertData2->getData(),pScanData->getData(),pScanData->getSize());
//		data_flag = memcmp(pInsertData2->getData(),pScanData->getData(),len+5);//应该相等
		data_flag = memcmp(pInsertData2->getData(),pScanData->getData(),len);//应该相等
		CHECK_BOOL(data_flag == 0);
		if(data_flag != 0)
		{
			cout << "插入数据与查询数据内容不一致!" << endl;
			cout << "插入数据为" << (char*)pInsertData2->getData() << endl;
			cout << "查询数据为" << (char*)pScanData->getData() << endl;
			HEAP_RETURN_FALSE
		}
		data_flag = memcmp(pInsertData2->getData(),pScanData->getData(),len+6);//应该不等
		CHECK_BOOL(data_flag != 0);
		if(data_flag == 0)
		{
			cout << "插入数据与查询数据内容不一致!" << endl;
			cout << "插入数据为" << (char*)pInsertData2->getData() << endl;
			cout << "查询数据为" << (char*)pScanData->getData() << endl;
			HEAP_RETURN_FALSE
		}
		size_flag = pInsertData2->getSize() == pScanData->getSize() ? 0 : -1;//相等返回0.不等返回-1
		CHECK_BOOL(size_flag == 0);
		if(size_flag != 0)
		{
			cout << "插入数据与查询数据长度不一致!" << endl;
			cout << "pInsertData2插入数据长度为" << pInsertData2->getSize() << endl;
			cout << "查询数据长度为" << pScanData->getSize() << endl;
			HEAP_RETURN_FALSE
		}			

		free(insert_str);
		delete pInsertData;  
		delete pInsertData2;  
		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}

#define MANYROWS 10000
bool test_heap_insert_dll_005()
{
	INTENT("测试插入大量数据行是否正确");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//计算宏DATAROWS的数字位数，为了分配合适的内存大小
		for(int i = MANYROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//该长度已包含'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[MANYROWS][20];
		for( ix = 0; ix != MANYROWS; ++ix)
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

		command_counter_increment();

		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;
		ix = 0;

		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*p为dataitem类型，输出参数
		{			
			data_flag = memcmp(dvec[ix]->getData(),pScanData->getData(),pScanData->getSize());
			if(data_flag != 0)
			{
				cout << "插入数据与查询数据内容不一致!" << endl;
				cout << "插入数据为" << (char*)dvec[ix]->getData() << endl;	
				cout << "查询数据为" << (char*)pScanData->getData() << endl;
				HEAP_RETURN_FALSE
			}

 			size_flag = dvec[ix]->getSize() == pScanData->getSize() ? 0 : -1;//相等返回0.不等返回-1
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
		CHECK_BOOL(counter == MANYROWS );
		if(counter != MANYROWS)
		{
			cout << "插入数据与查询数据行数不一致!" << endl;
			cout << "查询数据行数counter=" << counter << endl;
			HEAP_RETURN_FALSE			
		}

		free(insert_str);
		for(ix = 0; ix != MANYROWS; ++ix)
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

void insert_data_dll(EntrySet *es, Transaction *trans, const char data[][DATA_LEN], const int row)
{
	for(int i = 0; i < row; ++i)
	{
		DataItem di;
		EntryID ei;
		di.setData((void*)&data[i]);
		di.setSize(DATA_LEN);
		es->insertEntry(trans, ei, di);
	}
}

void thread_insert_dll(const EntrySetID eid, const char data[][DATA_LEN], const int row, int *sta)
{
	using namespace FounderXDB::StorageEngineNS;

	pStorageEngine->beginThread();
	Transaction* trans = NULL;
	EntrySet *es = NULL;
	bool fail = false;
	try {
		trans = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *es = (EntrySet*)pStorageEngine->openEntrySet(trans,EntrySet::OPEN_EXCLUSIVE,eid);
		insert_data_dll(es, trans, data, row);
		pStorageEngine->closeEntrySet(trans, es);
		trans->commit();
		*sta = 1;
	} catch (StorageEngineException &ex) {
		cout << ex.getErrorMsg() << endl;
		trans->abort();
		*sta = -1;
	}
	pStorageEngine->endThread();
}

bool test_heap_insert_dll_006()
{
	INTENT("连续多次打开同一个entryset，再多次关闭");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		for(int i = 0;i<10;++i)
		{
			EntrySet *pEntrySet = open_entry_set();
			pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//提交事务
	} 
	CATCHEXCEPTION
		return true;
}