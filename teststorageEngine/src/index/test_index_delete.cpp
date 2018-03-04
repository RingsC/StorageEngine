#include <iostream>
#include <assert.h>
#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "catalog/metaData.h"
#include "test_fram.h"
#include "utils/util.h"
#include "transaction/test_transaction_update.h"
#include "utils/fmgroids.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "heap/test_delete.h"

extern int RELID;
extern int INDEX_ID;

Colinfo heap_cf = NULL;
Colinfo index_cf = NULL;

//第二列的比较函数
int compareCol1(const char *str1, size_t len1, const char *str2, size_t len2)
{
	return strcmp(str1, str2);
}

//将heap分成两列
void heapSplit(RangeData& rd,const char *str, int col, size_t len = 0)
{
	rd.len = 0;
	rd.start = 0;
	if(col == 1)
	{
		rd.start = 0;
		rd.len = 3;
	}
	if(col == 2)
	{
		rd.start = 3;
		rd.len = 2;
	}
}

//将index分割成一列
void indexSplit(RangeData& rd,const char *str, int col, size_t len = 0)
{
	rd.start = 0;
	rd.len = 0;

	if(col == 1)
	{
		rd.start = 0;
		rd.len = 2;
	}
}

void initHeapAndIndexColinfo()
{
	if(heap_cf == NULL)
	{
		heap_cf = (Colinfo)malloc(sizeof(ColinfoData));
		heap_cf->keys = 1;
		heap_cf->col_number = NULL;
		heap_cf->rd_comfunction = NULL;
		heap_cf->split_function = heapSplit;
	}
	if(index_cf == NULL)
	{
		index_cf = (Colinfo)malloc(sizeof(ColinfoData));
		index_cf->keys = 1;
		index_cf->col_number = (size_t*)malloc(sizeof(size_t) * index_cf->keys);
		index_cf->col_number[0] = 2;
		index_cf->rd_comfunction = (CompareCallback*)malloc(sizeof(CompareCallback) * index_cf->keys);
		index_cf->rd_comfunction[0] = compareCol1;
		index_cf->split_function = indexSplit;
	}
}

void freeHeapAndIndexColinfo()
{
	if(heap_cf && index_cf)
	{
		free(heap_cf);
		free(index_cf->col_number);
		free(index_cf->rd_comfunction);
		free(index_cf);
		heap_cf = NULL;
		index_cf = NULL;
	}
}

//创建表，创建单列索引
void benginIndexTest()
{
	Relation indrel;
	Relation rel;
	beginTestNotStartEngine();
	begin_transaction();
	Oid relid = RELID;
	Oid indid = INDEX_ID;
	Oid tablespace = DEFAULTTABLESPACE_OID;
	SAVE_INFO_FOR_DEBUG();
	initHeapAndIndexColinfo();
	setColInfo(indid, index_cf);
	rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
	FDPG_Index::fd_index_create(rel,BTREE_TYPE,indid,indid);
	CommandCounterIncrement();
	indrel = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);
	FDPG_Index::fd_index_close(indrel, AccessShareLock);
	FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	commit_transaction();
}

//删除表,删除索引
void endIndexTest()
{
	begin_transaction();
	FDPG_Index::fd_index_drop(RELID, DEFAULTTABLESPACE_OID, INDEX_ID, MyDatabaseId);
	commit_transaction();
	endTestNotStopEngine();
	++INDEX_ID;
}


ItemPointerData findIPD(char *data, Relation rel, Relation indrel, int &sta, int &count, 
												char *c_data = "op", StrategyNumber sn = BTLessStrategyNumber)
{
	assert(rel && indrel);
	using namespace FounderXDB::StorageEngineNS;
	ItemPointerData ipd;
	memset(&ipd, 0, sizeof(ItemPointerData));
	sta = -1;
	count = 0;
	Datum *value = (Datum*)palloc(sizeof(Datum));
	value[0] = fdxdb_string_formdatum(c_data, strlen(c_data));
	ScanKeyData key[1];
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], 1, sn, compareCol1, value[0]);
	SAVE_INFO_FOR_DEBUG();
	IndexScanDesc indscan = FDPG_Index::fd_index_beginscan(rel, indrel, SnapshotNow, 1, 0);
	index_rescan(indscan, key, 1, NULL, 0);
	HeapTuple tuple;
	char tmp_data[200];
	while((tuple = FDPG_Index::fd_index_getnext(indscan, ForwardScanDirection)) != NULL)
	{
		//memcpy_s(tmp_data, 200, fxdb_tuple_to_chars(tuple), 200);
		memcpy(tmp_data, fxdb_tuple_to_chars(tuple), 200);
		if(strcmp(tmp_data, data) == 0)
		{
			ipd = tuple->t_self;
			sta = 1;
			++count;
		}
	}
	FDPG_Index::fd_index_endscan(indscan);
	return ipd;
}

int testIndexDelete001()
{
	INTENT("测试用索引的方式查找到reboot元组并删除。"
				 "删除后查看heap，测试成功的条件是reboot"
				 "元组不存在。");
	using namespace FounderXDB::StorageEngineNS;
	Relation indrel, rel;
	try
	{
		begin_transaction();
		Oid indid = INDEX_ID;
		Oid relid = RELID;
		initHeapAndIndexColinfo();
		rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		indrel = FDPG_Index::fd_index_open(indid,RowExclusiveLock, MyDatabaseId);
		int sta = 0;
		int count = 0;
		//查找元组reboot
		SAVE_INFO_FOR_DEBUG();
		ItemPointerData ipd = findIPD("reboot", rel, indrel, sta, count);
		if(sta == 1)//找到reboot，删除
		{
			FDPG_Heap::fd_simple_heap_delete(rel, &ipd);
			CommandCounterIncrement();
		}else				//找不到reboot，测试失败
		{
			FDPG_Index::fd_index_close(indrel, RowExclusiveLock);
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			commit_transaction();
			return 0;
		}
		//再次查找reboot元组，如果存在，测试失败
		findIPD("reboot", rel, indrel, sta, count);
		if(sta == 1)//找到reboot，测试失败
		{
			FDPG_Index::fd_index_close(indrel, RowExclusiveLock);
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			commit_transaction();
			return 0;
		}
		FDPG_Index::fd_index_close(indrel, RowExclusiveLock);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
		return 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		std::cout << se.getErrorMsg() << std::endl;
		std::cout << se.getErrorNo() << std::endl;
		return 0;
	}
}

int testIndexDelete002()
{
	INTENT("测试删除目标表中所有的元组。"
				 "测试成功的条件为表中不再含有数据。");
	using namespace FounderXDB::StorageEngineNS;
	Relation indrel, rel;
	try
	{
		begin_transaction();
		//表中的所有元组
		char data[20][20] = {"testdata_1", "testdata_2", "testdata_3", "testdata_1",\
			"testdata_2", "testdata_1", "apple", "reboot", "apple"};
		int i = 0;
		ItemPointerData ipd;
		int sta = 0;
		int count = 0;
		initHeapAndIndexColinfo();
		SAVE_INFO_FOR_DEBUG();
		rel = FDPG_Heap::fd_heap_open(RELID,RowExclusiveLock, MyDatabaseId);
		SAVE_INFO_FOR_DEBUG();
		indrel = FDPG_Index::fd_index_open(INDEX_ID,RowExclusiveLock, MyDatabaseId);
		while(strlen(data[i]) != 0)
		{
			//由于目标数据表中的所有数据均为小写字母开头，
			//所以这里用字符a为比较字符，大于等于为比较策
			//略，以保证能找到所有的元组
			SAVE_INFO_FOR_DEBUG();
			ipd = findIPD(data[i], rel, indrel, sta, count, "a", BTGreaterEqualStrategyNumber);
			SAVE_INFO_FOR_DEBUG();
			if(sta == 1)
			{
				FDPG_Heap::fd_simple_heap_delete(rel, &ipd);
				CommandCounterIncrement();
			}
			++i;
		}
		//再次扫描目标数据表，如果表中还存在数据则测试失败
		Datum *value = (Datum*)palloc(sizeof(Datum));
		value[0] = fdxdb_string_formdatum("a", 1);
		ScanKeyData key[1];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], 1, BTGreaterEqualStrategyNumber, compareCol1, value[0]);
		SAVE_INFO_FOR_DEBUG();
		IndexScanDesc indscan = FDPG_Index::fd_index_beginscan(rel, indrel, SnapshotNow, 1, 0);
		index_rescan(indscan, key, 1, NULL, 0);
		i = 0;
		while(FDPG_Index::fd_index_getnext(indscan, ForwardScanDirection) != NULL)
		{
			++i;
		}
		//表中有数据，测试失败
		if(i > 0)
		{
			FDPG_Index::fd_index_endscan(indscan);
			FDPG_Index::fd_index_close(indrel, RowExclusiveLock);
			FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
			user_abort_transaction();
			return 0;
		}
		FDPG_Index::fd_index_endscan(indscan);
		FDPG_Index::fd_index_close(indrel, RowExclusiveLock);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		commit_transaction();
		return 1;
	}catch (StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		std::cout << se.getErrorMsg() << std::endl;
		std::cout << se.getErrorNo() << std::endl;
		return 0;
	}
}
