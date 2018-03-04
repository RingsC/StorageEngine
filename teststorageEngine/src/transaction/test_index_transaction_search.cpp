#include <iostream>
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "catalog/xdb_catalog.h"
#include "access/xact.h"
#include "utils/rel.h"
#include "utils/util.h"
#include "test_fram.h"
#include "catalog/metaData.h"
#define SEARCH_ALL "a"
#define SEARCH_HAFT "k"

#ifndef UNIQUE_DATA
#define UNIQUE_DATA "reboot"
#endif 

extern int RELID;
extern int INDEX_ID;
extern Colinfo heap_cf;
extern Colinfo index_cf;

extern void initHeapAndIndexColinfo();
extern int compareCol1(const char *str1, size_t len1, const char *str2, size_t len2);
extern ItemPointerData findIPD(char *data, Relation rel, Relation indrel, int &sta, int &count, 
															 char *c_data = "op", StrategyNumber sn = BTLessStrategyNumber);
extern void freeHeapAndIndexColinfo();
extern void beginTestNotStartEngine();
extern void heapSplit(RangeData& rangeData, const char *str, int col, size_t len = 0);

//将index分割成两列
void indexSplit_2(RangeData& rd, const char *str, int col, size_t len = 0)
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

//初始化两列的heap和两列的index
void initHeapAndIndexColinfo_2()
{
	freeHeapAndIndexColinfo();
	heap_cf = (Colinfo)malloc(sizeof(ColinfoData));
	heap_cf->keys = 2;
	heap_cf->col_number = NULL;
	heap_cf->rd_comfunction = NULL;
	heap_cf->split_function = heapSplit;
	index_cf = (Colinfo)malloc(sizeof(ColinfoData));
	index_cf->keys = 2;
	index_cf->col_number = (size_t*)malloc(sizeof(size_t) * index_cf->keys);
	index_cf->col_number[0] = 1;
	index_cf->col_number[1] = 2;
	index_cf->rd_comfunction = (CompareCallback*)malloc(sizeof(CompareCallback) * index_cf->keys);
	index_cf->rd_comfunction[0] = compareCol1;
	index_cf->rd_comfunction[1] = compareCol1;
	index_cf->split_function = indexSplit_2;
}

//创建表，创建多列索引
void benginIndexTest_2()
{
	Relation indrel;
	Relation rel;
	initHeapAndIndexColinfo_2();
	beginTestNotStartEngine();
	begin_transaction();
	Oid relid = RELID;
	Oid indid = INDEX_ID;
	Oid tablespace = DEFAULTTABLESPACE_OID;
	SAVE_INFO_FOR_DEBUG();
	setColInfo(indid,index_cf);
	rel = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
	FDPG_Index::fd_index_create(rel,BTREE_TYPE,indid,indid);
	CommandCounterIncrement();
	indrel = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);
	FDPG_Index::fd_index_close(indrel, AccessShareLock);
	FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	commit_transaction();
}

//根据给定的数据查找元组
ItemPointerData findIPD_2(char *data, Relation rel, Relation indrel, int &sta,
													int &count, char *c_data1 = "a", char *c_data2 = "z", 
													StrategyNumber sn1 = BTGreaterEqualStrategyNumber,
													StrategyNumber sn2 = BTLessEqualStrategyNumber)
{
	assert(rel && indrel);
	using namespace FounderXDB::StorageEngineNS;
	ItemPointerData ipd;
	memset(&ipd, 0, sizeof(ItemPointerData));
	sta = -1;
	count = 0;
	//创建两列上的比较字符
	Datum *value = (Datum*)palloc(sizeof(Datum) * 2);
	value[0] = fdxdb_string_formdatum(c_data1, strlen(c_data1));
	value[1] = fdxdb_string_formdatum(c_data2, strlen(c_data2));
	ScanKeyData key[2];
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], 1, sn1, compareCol1, value[0]);
	Fdxdb_ScanKeyInitWithCallbackInfo(&key[1], 2, sn2, compareCol1, value[1]);
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

//打开索引表和数据表
void openRel(Relation *rel, Relation *indrel)
{
	initHeapAndIndexColinfo();
	if(rel && *rel == NULL)
	{
		SAVE_INFO_FOR_DEBUG();
		*rel = FDPG_Heap::fd_heap_open(RELID,RowExclusiveLock, MyDatabaseId);
	}
	if(indrel && *indrel == NULL)
	{
		SAVE_INFO_FOR_DEBUG();
		*indrel = FDPG_Index::fd_index_open(INDEX_ID, RowExclusiveLock, MyDatabaseId);
	}
}

//关闭索引表和数据表
void closeRel(Relation *rel, Relation *indrel)
{
	if(rel && *rel)
	{
		SAVE_INFO_FOR_DEBUG();
		FDPG_Heap::fd_heap_close(*rel, RowExclusiveLock);	
		*rel = NULL;
	}
	if(indrel && *indrel)
	{
		SAVE_INFO_FOR_DEBUG();
		FDPG_Index::fd_index_close(*indrel, RowExclusiveLock);
		*indrel = NULL;
	}
}

int testIndexSimpleSearch()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("在一个事务中打开一个索引表并扫描、关闭。"
				 "随后在打开另外一个事务做同样的事情。"
				 "如果报异常，则测试失败。");
	Relation rel = NULL;
	Relation indrel = NULL;
	try
	{
		int sta = 0;
		int count = 0;
		{
			begin_transaction();
			openRel(&rel, &indrel);
			//查找一个数据表中不存在的数据以保证扫描整个表
			SAVE_INFO_FOR_DEBUG();
			findIPD("abcd", rel, indrel, sta, count, SEARCH_ALL, BTGreaterEqualStrategyNumber);
			//找到abcd，测试失败
			if(sta == 1)
			{
				closeRel(&rel, &indrel);
				user_abort_transaction();
				return 0;
			}
			closeRel(&rel, &indrel);
			commit_transaction();
		}
		/*  关闭第一次扫描的事务，开启第二次扫描的事务  */
		{
			begin_transaction();
			openRel(&rel, &indrel);
			SAVE_INFO_FOR_DEBUG();
			findIPD("xyzs", rel, indrel, sta, count, SEARCH_HAFT, BTGreaterEqualStrategyNumber);
			//找到abcd，测试失败
			if(sta == 1)
			{
				closeRel(&rel, &indrel);
				user_abort_transaction();
				return 0;
			}
			closeRel(&rel, &indrel);
			commit_transaction();
		}
		return 1;
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		std::cout << se.getErrorNo() << std::endl;
		std::cout << se.getErrorMsg() << std::endl;
		return 0;
	}
}

int testIndexSearch001()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("在第一个事务中打开索引表并向前扫描三步。关闭事务（不关闭扫描）。"
				 "在另外一个事务中再次扫描该索引表。如果报异常说明测试失败。");
	Relation rel = NULL;
	Relation indrel = NULL;
	try
	{
		{
			begin_transaction();
			openRel(&rel, &indrel);
			SAVE_INFO_FOR_DEBUG();
			Datum *value = (Datum*)palloc(sizeof(Datum));
			//扫描所有元组
			value[0] = fdxdb_string_formdatum(SEARCH_ALL, strlen(SEARCH_ALL));
			ScanKeyData skd[1];
			Fdxdb_ScanKeyInitWithCallbackInfo(&skd[0], 1, BTGreaterEqualStrategyNumber, 
				compareCol1, value[0]);
			IndexScanDesc isd = FDPG_Index::fd_index_beginscan(rel, indrel, SnapshotNow, 1, 0);
			FDPG_Index::fd_index_rescan(isd, skd, 1, NULL, 0);
			int step = 0;
			while(step++ != 3)
			{
				FDPG_Index::fd_index_getnext(isd, ForwardScanDirection);
			}
			FDPG_Index::fd_index_endscan(isd);
			closeRel(&rel, &indrel);
			commit_transaction();
		}
		/* 提交第一个事务，打开第二个事务，做同样的事情 */
		{
			begin_transaction();
			indrel = NULL;
			openRel(&rel, &indrel);
			SAVE_INFO_FOR_DEBUG();
			Datum *value = (Datum*)palloc(sizeof(Datum));
			//扫描所有元组
			value[0] = fdxdb_string_formdatum(SEARCH_ALL, strlen(SEARCH_ALL));
			ScanKeyData skd[1];
			Fdxdb_ScanKeyInitWithCallbackInfo(&skd[0], 1, BTGreaterEqualStrategyNumber, 
																				compareCol1, value[0]);
			IndexScanDesc isd = FDPG_Index::fd_index_beginscan(rel, indrel, SnapshotNow, 1, 0);
			FDPG_Index::fd_index_rescan(isd, skd, 1, NULL, 0);
			int step = 0;
			while(step++ != 3)
			{
				FDPG_Index::fd_index_getnext(isd, ForwardScanDirection);
			}
			FDPG_Index::fd_index_endscan(isd);
			closeRel(&rel, &indrel);
			commit_transaction();
		}
	}catch (StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		std::cout << se.getErrorNo() << std::endl;
		std::cout << se.getErrorMsg() << std::endl;
		return 0;
	}
	return 1;
}

int testIndexSearch002()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试在一个事务中连着两次beginscan和连着两次rescan。"
				 "如果报异常则测试失败。");
	Relation rel = NULL;
	Relation indrel = NULL;
	try
	{
		begin_transaction();
		initHeapAndIndexColinfo();
		rel = FDPG_Heap::fd_heap_open(RELID, RowExclusiveLock, MyDatabaseId);

		indrel = FDPG_Index::fd_index_open(INDEX_ID,RowExclusiveLock, MyDatabaseId);

		Datum *value = (Datum*)palloc0(sizeof(Datum));
		value[0] = fdxdb_string_formdatum(SEARCH_ALL, strlen(SEARCH_ALL));
		ScanKeyData key[1];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], 
																			1, 
																			BTGreaterEqualStrategyNumber, 
																			compareCol1, 
																			value[0]);

		//连着两次beginscan
		IndexScanDesc scan = FDPG_Index::fd_index_beginscan(rel, indrel, SnapshotNow, 1, 0);
		SAVE_INFO_FOR_DEBUG();
		scan = FDPG_Index::fd_index_beginscan(rel, indrel, SnapshotNow, 1, 0);
		FDPG_Index::fd_index_rescan(scan, key, 1, NULL, 0);
		SAVE_INFO_FOR_DEBUG();
		FDPG_Index::fd_index_rescan(scan, key, 1, NULL, 0);
		FDPG_Index::fd_index_endscan(scan);
		closeRel(&rel, &indrel);
		commit_transaction();
		return 1;
	}catch (StorageEngineExceptionUniversal &se)
	{
		closeRel(&rel, &indrel);
		user_abort_transaction();
		std::cout << se.getErrorNo() << std::endl;
		std::cout << se.getErrorMsg() << std::endl;
		return 0;
	}
}
int testIndexSearch003()
{
	INTENT("测试通过两列比较查找元组。");
	using namespace FounderXDB::StorageEngineNS;
	Relation rel = NULL;
	Relation indrel = NULL;
	try
	{
		/************************************************************************/
		/*          ------               表内数据如下            ---------      */
		/*          "testdata_1", "testdata_2", "testdata_3", "testdata_1"      */
		/*					"testdata_2", "testdata_1", "apple", "reboot", "apple"      */
		/************************************************************************/
		begin_transaction();
		rel = FDPG_Heap::fd_heap_open(RELID,RowExclusiveLock, MyDatabaseId);
		indrel = FDPG_Index::fd_index_open(INDEX_ID,RowExclusiveLock, MyDatabaseId);
		//比较字符为第一列小于“aqq”且第二列大于“la”的元组
		//这里的目标查找元组为两个“apple”
		int sta = 0;
		int count = 0;
		int success_count = 0;
		//在范围内查找apple
		findIPD_2("apple", rel, indrel, sta, count, "aqq", "la",
							BTLessEqualStrategyNumber, BTGreaterEqualStrategyNumber);
		//找到两个apple，测试成功
		if(count == 2)
		{
			++success_count;
		}
		//在范围外查找apple
		findIPD_2("apple", rel, indrel, 
							sta, count, "aqq", "la",
							BTGreaterStrategyNumber, 
							BTLessStrategyNumber);
		//没找到任何的apple
		if(count == 0)
		{
			++success_count;
		}
		//上面两次查找得到正确结果
		if(success_count == 2)
		{
			closeRel(&rel, &indrel);
			commit_transaction();
			return 1;
		}
		closeRel(&rel, &indrel);
		commit_transaction();
		return 0;
	}catch(StorageEngineExceptionUniversal &se)
	{
		closeRel(&rel, &indrel);
		user_abort_transaction();
		std::cout << se.getErrorNo() << std::endl;
		std::cout << se.getErrorMsg() << std::endl;
		return 0;
	}
}

int testIndexSearch004()
{
	INTENT("测试通过两列比较查找元组。");
	using namespace FounderXDB::StorageEngineNS;
	const int heap_id = 8888;
	const int index_id = 9999;
	const int index_id2 = 7777;
	Relation rel = NULL;
	Relation indrel = NULL;
	Relation indrel2 = NULL;
	try
	{
		begin_first_transaction();

		/*
		*建表，建索引
		*/
		FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID, heap_id);
		CommandCounterIncrement();

		initHeapAndIndexColinfo_2();
	    setColInfo(index_id,index_cf);
		rel = FDPG_Heap::fd_heap_open(heap_id,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(rel,BTREE_TYPE,index_id,index_id);
		CommandCounterIncrement();


		/*
		*插入数据
		*/
		indrel = FDPG_Index::fd_index_open(index_id,RowExclusiveLock, MyDatabaseId);
		char data[20][20] = {"testdata_1", "testdata_2", "testdata_3", "testdata_1",
												 "testdata_2", "testdata_1", "apple", "reboot", "apple"};
		HeapTuple tuple = NULL;
		Datum values[1];
		bool    isnull[1];
		isnull[0] = false;
		for(int i = 0; i < 9; ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(data[i], 20);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			values[0] = fdxdb_form_index_datum(rel, indrel, tuple);
			FDPG_Index::fd_index_insert(indrel, values, isnull, &(tuple->t_self), rel, UNIQUE_CHECK_NO);
		}
		FDPG_Index::fd_index_close(indrel, RowExclusiveLock);
		CommandCounterIncrement();


		/*
		*检查rd_am和rd_aminfo属性
		*/
		indrel = FDPG_Index::fd_index_open(index_id,RowExclusiveLock, MyDatabaseId);
		//rd_am和rd_aminfo为空则测试失败
		if(NULL == indrel->rd_am || NULL == indrel->rd_aminfo)
		{
			closeRel(&rel, &indrel);
			FDPG_Index::fd_index_drop(heap_id, DEFAULTTABLESPACE_OID, index_id, MyDatabaseId);
			FDPG_Heap::fd_heap_drop(heap_id, MyDatabaseId);
			commit_transaction();
			return false;
		}
		closeRel(&rel, &indrel);

		commit_transaction();

		/*
		*开启新的事务
		*/
		begin_transaction();
		
		/*
		*创建第二个索引
		*/
		initHeapAndIndexColinfo_2();
		setColInfo(index_id2,index_cf);
		rel = FDPG_Heap::fd_heap_open(heap_id,RowExclusiveLock, MyDatabaseId);
		FDPG_Index::fd_index_create(rel,BTREE_TYPE,index_id2,index_id2);
		CommandCounterIncrement();


		/*
		*打开索引检查rd_am和rd_aminfo属性
		*/
		indrel = FDPG_Index::fd_index_open(index_id,RowExclusiveLock, MyDatabaseId);
		//rd_am和rd_aminfo为空则测试失败
		if(NULL == indrel->rd_am || NULL == indrel->rd_aminfo)
		{
			closeRel(&rel, &indrel);
			FDPG_Index::fd_index_drop(heap_id, DEFAULTTABLESPACE_OID, index_id, MyDatabaseId);
			FDPG_Index::fd_index_drop(heap_id, DEFAULTTABLESPACE_OID, index_id2, MyDatabaseId);
			FDPG_Heap::fd_heap_drop(heap_id, MyDatabaseId);
			commit_transaction();
			return false;
		}

	}catch(StorageEngineExceptionUniversal &se)
	{
		closeRel(&rel, &indrel);
		FDPG_Index::fd_index_drop(heap_id, DEFAULTTABLESPACE_OID, index_id, MyDatabaseId);
		FDPG_Index::fd_index_drop(heap_id, DEFAULTTABLESPACE_OID, index_id2, MyDatabaseId);
		FDPG_Heap::fd_heap_drop(heap_id, MyDatabaseId);
		commit_transaction();
		std::cout << se.getErrorNo() << std::endl;
		std::cout << se.getErrorMsg() << std::endl;
		return false;
	}

	closeRel(&rel, &indrel);
	FDPG_Index::fd_index_drop(heap_id, DEFAULTTABLESPACE_OID, index_id, MyDatabaseId);
	FDPG_Index::fd_index_drop(heap_id, DEFAULTTABLESPACE_OID, index_id2, MyDatabaseId);
	FDPG_Heap::fd_heap_drop(heap_id, MyDatabaseId);
	commit_transaction();
	return true;
}
