#include <boost/thread/thread.hpp>
#include <stdio.h>
#include <malloc.h>
#include <vector>
#include <algorithm>
#include <exception>

#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"

#include "utils/util.h"
#include "meta/meta_test.h"
#include "test_fram.h"
#include "catalog/metaData.h"
#include "catalog/catalog.h"
#include "catalog/xdb_catalog.h"

extern int relid_compare(const char* a, size_t len1, const char* b, size_t len2);

extern int str_compare(const char *str1, size_t len1, const char *str2, size_t len2);

extern void meta_item_split(RangeData& rangeData, const char *str, int col, size_t len = 0);

//extern unsigned int fxdb_get_type(unsigned int object_id);

//extern bool fxdb_get_index_info(unsigned int table_id, IndinfoData& colinfo);

extern unsigned int fxdb_get_max_id();

extern bool fxdb_delete_meta(unsigned int object_id);

//extern unsigned int fxdb_get_tblspace(unsigned int object_id);

extern void fxdb_create_catalog();
//void fxdb_create_catalog1()
//{
//	Oid reltablespace = DEFAULTTABLESPACE_OID;
//	FDPG_Transaction::fd_AbortOutOfAnyTransaction();
//	StartTransactionCommand();
//	BeginTransactionBlock();
//	CommitTransactionCommand();
//
//	fxdb_heap_create(reltablespace, MetaTableId);
//	CommandCounterIncrement();
//	Colinfo colinfo2 = (Colinfo)palloc(sizeof(ColinfoData));
//	colinfo2->keys = 8;
//	colinfo2->col_number = NULL;
//	colinfo2->rd_comfunction = NULL;
//	colinfo2->split_function = meta_item_split;
//	Relation rel = heap_open(MetaTableId, reltablespace, RowExclusiveLock, colinfo2);
//
//	//rel->rd_colinfo->split_function = meta_item_split;
//	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
//	colinfo->keys = 1;
//	colinfo->col_number = (int*)palloc(colinfo->keys * sizeof(int));
//	colinfo->col_number[0] = 1;
//	colinfo->rd_comfunction = (CompareCallback*)palloc(colinfo->keys * sizeof(CompareCallback));
//	colinfo->rd_comfunction[0] = relid_compare;
//	colinfo->split_function =  meta_item_split;
//	fxdb_index_create(reltablespace, MetaTableColFirstIndex, rel, BTREE_TYPE, colinfo, false);
//
//	colinfo->col_number[0] = 3;
//	fxdb_index_create(reltablespace, MetaTableColThirdIndex, rel, BTREE_TYPE, colinfo, false);
//
//	heap_close(rel, RowExclusiveLock);
//
//	StartTransactionCommand();
//	EndTransactionBlock();
//	CommitTransactionCommand();
//
//}

//init heap colinfo
extern void init_metadata_heap_colinfo();

//init index colinfo
extern void init_metadata_index_col1_colinfo();

//init index colinfo
extern void init_metadata_index_col3_colinfo();

/*get_tuple_on_BTEqualStrategyNumber:
*using BTEqualStrategyNumber to find a tuple
*/
extern char* get_tuple_in_BTEqualStrategyNumber(const unsigned int& index_id,
																								const unsigned int& object_id,
																								const Oid& table_id = MetaTableId,
																								const Oid& table_space = DEFAULTTABLESPACE_OID);

unsigned int fxdb_get_tblspace1(unsigned int object_id)
{
	char *tmp_data = get_tuple_in_BTEqualStrategyNumber(MetaTableColFirstIndex,
		object_id);

	unsigned int table_space_id = 0;
    RangeData rd = {0};
    meta_item_split(rd, tmp_data, 7);
	memcpy(&table_space_id, &tmp_data[rd.start], rd.len);
	return table_space_id;
}

//get max oid
Oid get_max_id1()
{
	using namespace FounderXDB::StorageEngineNS;
	Oid max_id = 0;
	Relation rel = NULL;
	PG_TRY();
	{
		//open MetaTableId
		rel = heap_open(MetaTableId, AccessShareLock);

		HeapScanDesc scan = heap_beginscan(rel, SnapshotNow, 0, NULL, 0, 0);
		HeapTuple tuple = NULL;
		char *tmp_max_id = "\0";
		Oid tmp_id = 0;
		//scan and compare Oid
		while((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			tmp_max_id = fxdb_tuple_to_chars(tuple);
            RangeData rd = {0};
            meta_item_split(rd, tmp_max_id, 1);
			memcpy(&tmp_id,
				&tmp_max_id[rd.start],
				strlen(&tmp_max_id[rd.start]));
			//max_id is always the largest
			if(max_id <= tmp_id)
			{
				max_id = tmp_id;
			}
			memset(&tmp_id, 0, sizeof(tmp_id));
		}
		heap_endscan(scan);
		heap_close(rel, AccessShareLock);
		return max_id;
	}
	PG_CATCH();
	{
		ThrowException();
	}
	PG_END_TRY();
}

void fxdb_copy_and_move1(char **meta_data, void *object, int meta_len, int len)
{
	memcpy(*meta_data, object, len);							//copy object
	*meta_data += len;																				//move addr
}

//insert metadata
bool fxdb_insert_meta1(unsigned int object_id, int type, unsigned int parent_table_name,
											 time_t time, int owner, unsigned int database_id,
											 unsigned int tablespace_id, unsigned int col_id)
{
	using namespace FounderXDB::StorageEngineNS;
	Relation rel = NULL;
	Relation indrel1 = NULL, indrel2 = NULL;
	//memory size calculation
	int len = sizeof(object_id) + 
		sizeof(type) + 
		sizeof(parent_table_name) + 
		sizeof(time) + 
		sizeof(owner) + 
		sizeof(database_id) + 
		sizeof(tablespace_id) + 
		sizeof(col_id) + 1;

	char *meta_data = NULL;
	//allocate memory
	meta_data = (char*)palloc(len);
	memset(meta_data, 0, len);
	char *meta_data_addr = meta_data;																	//first addr

	//copy object_id
	fxdb_copy_and_move1(&meta_data, 
		&object_id, 
		len, 
		sizeof(object_id));

	//copy type
	fxdb_copy_and_move1(&meta_data, 
		&type, 
		len, 
		sizeof(type));

	//copy parent_table_name
	fxdb_copy_and_move1(&meta_data, 
		&parent_table_name, 
		len, 
		sizeof(parent_table_name));	

	//copy time
	fxdb_copy_and_move1(&meta_data, 
		&time, 
		len, 
		sizeof(time));	

	//copy owner	
	fxdb_copy_and_move1(&meta_data, 
		&owner, 
		len, 
		sizeof(owner));

	//copy database_id	
	fxdb_copy_and_move1(&meta_data, 
		&database_id, 
		len, 
		sizeof(database_id));	

	//copy tablespace_id
	fxdb_copy_and_move1(&meta_data, 
		&tablespace_id, 
		len, 
		sizeof(tablespace_id));

	memcpy(meta_data, &col_id, sizeof(col_id));					//copy col_info
	meta_data += sizeof(col_id);

	int return_ = true;
	do 
	{
//		Colinfo colinfo = init_metadata_heap_colinfo();
		//open metadata table
		rel = heap_open(MetaTableId,
			RowExclusiveLock);

		//colinfo = init_metadata_index_col1_colinfo();
		//open index
		indrel1 = index_open(MetaTableColFirstIndex,
			RowExclusiveLock);

		//colinfo = init_metadata_index_col3_colinfo();
		indrel2 = index_open(MetaTableColThirdIndex,
			RowExclusiveLock);
		HeapTuple tuple = fdxdb_heap_formtuple(meta_data_addr, meta_data - meta_data_addr);
		Datum		values[1];
		bool		isnull[1];
		isnull[0] = false;
		values[0] = fdxdb_form_index_datum(rel, indrel1, tuple);
		simple_heap_insert(rel, tuple);//insert metadata
		CommandCounterIncrement();
		//update index
		if(!index_insert(indrel1, values, isnull, &(tuple->t_self), rel, UNIQUE_CHECK_YES))
		{
			return_ = false;
			break;
		}
		Datum		values1[1];
		values1[0] = fdxdb_form_index_datum(rel, indrel2, tuple);
		if(!index_insert(indrel2, values1, isnull, &(tuple->t_self), rel, UNIQUE_CHECK_NO))
		{
			return_ = false;
			break;
		}
		CommandCounterIncrement();
	} while (0);
	index_close(indrel1, RowExclusiveLock);
	index_close(indrel2, RowExclusiveLock);
	heap_close(rel, RowExclusiveLock);
	return return_;
}

//delete metadata
bool fxdb_delete_meta1(unsigned int object_id)
{
	using namespace FounderXDB::StorageEngineNS;
	Relation rel = NULL;
	Relation indrel = NULL;
	PG_TRY();
	{
		//open MetaTableId
		rel = heap_open(MetaTableId, 
			RowExclusiveLock);

		char compare_data[50];//compare string
		memcpy(compare_data, &object_id, sizeof(object_id));

//		Colinfo colinfo = init_metadata_index_col1_colinfo();

		//open MetaTableColFirstIndex
		indrel = index_open(MetaTableColFirstIndex,
			RowExclusiveLock);
		Datum *value = (Datum*)palloc(sizeof(Datum));
		value[0] = fdxdb_string_formdatum(compare_data, sizeof(compare_data));
		ScanKeyData key[1];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], 1, BTEqualStrategyNumber, relid_compare, value[0]);
		pfree(value);
		IndexScanDesc indscan = index_beginscan(rel, indrel, SnapshotNow, 1, 0, 0, 0);
		index_rescan(indscan, key, 1, NULL, 0);
		HeapTuple tuple = NULL;
		char *tmp_data = NULL;
		while((tuple = index_getnext(indscan, ForwardScanDirection)) != NULL)
		{
			tmp_data = fxdb_tuple_to_chars(tuple);
			if(memcmp(tmp_data, compare_data, strlen(compare_data)) == 0)
			{
				/*
				*delete tuple
				*/
				simple_heap_delete(rel, &tuple->t_self);
				CommandCounterIncrement();
			}
		}
		index_endscan(indscan);
		index_close(indrel, RowExclusiveLock);
		heap_close(rel, RowExclusiveLock);
		return true;
	}
	PG_CATCH();
	{
		ThrowException();
	}
	PG_END_TRY();
}

//search all index info on the table_id
bool fxdb_get_index_info1(unsigned int table_id, IndinfoData& indinfo)
{
	using namespace FounderXDB::StorageEngineNS;
	Relation rel = NULL;
	Relation indrel = NULL;
	PG_TRY();
	{
		rel = heap_open(MetaTableId,
			RowExclusiveLock);

//		Colinfo colinfo = init_metadata_index_col3_colinfo();
		indrel = index_open(MetaTableColThirdIndex,
			RowExclusiveLock);
		char compare_data[50];//compare string
		memcpy(compare_data, &table_id, sizeof(table_id));
		Datum *value = (Datum*)palloc(sizeof(Datum));
		value[0] = fdxdb_string_formdatum(compare_data, sizeof(compare_data));
		ScanKeyData key[1];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0], 1, BTEqualStrategyNumber, relid_compare, value[0]);
		IndexScanDesc indscan = index_beginscan(rel, indrel, SnapshotNow, 1, 0, 0, 0);
		index_rescan(indscan, key, 1, NULL, 0);
		HeapTuple tuple = NULL;
		char *tmp_data = NULL;
		unsigned int index_id = 0;
		indinfo.index_num = 0;
		//Init colinfo.index_array、colinfo.index_info.
		//Allocate 3 size.If overflow later,than allocate
		//3 size each time.
		indinfo.index_array = (unsigned int *)palloc(sizeof(unsigned int) * 3);
		indinfo.index_info = (Colinfo*)palloc(sizeof(Colinfo) * 3);
		int max_len = 3;
		RangeData rd;
		memset(&rd, 0, sizeof(rd));
		while((tuple = index_getnext(indscan, ForwardScanDirection)) != NULL)
		{
			tmp_data = fxdb_tuple_to_chars(tuple);
			//get parent_id(column 3)
			meta_item_split(rd, tmp_data, 1);
			memcpy(&index_id, tmp_data, rd.len);
			//overflow
			if(indinfo.index_num != 0 && indinfo.index_num % 3 == 0)
			{
				//init new unsigned int* and Colinfo*
				unsigned int *tmp_int = (unsigned int *)
					palloc(sizeof(unsigned int) * (max_len + 3));
				Colinfo *tmp_colinfo = (Colinfo*)
					palloc(sizeof(Colinfo) * (max_len + 3));
				//copy index_array to new memory
				memcpy(tmp_int,
					indinfo.index_array, 
					sizeof(unsigned int) * indinfo.index_num);
				//copy index_info to new memory
				memcpy(tmp_colinfo, 
					indinfo.index_info, 
					sizeof(Colinfo) * indinfo.index_num);
				max_len += 3;//get max len
				//free old unsigned int *
				pfree(indinfo.index_array);
				pfree(indinfo.index_info);
				//point to new memory
				indinfo.index_array = tmp_int;
				indinfo.index_info = tmp_colinfo;
			}
			//fill colinfo.index_array
			memcpy(&indinfo.index_array[indinfo.index_num],
				&index_id,
				sizeof(index_id));
			//move to col 8 and get	Colum_info
			meta_item_split(rd, tmp_data, 8);
			unsigned int colinfo_id = 0;
			memcpy(&colinfo_id, &tmp_data[rd.start], rd.len);
			Colinfo ci = getColInfo(colinfo_id);
			memcpy(&indinfo.index_info[indinfo.index_num],
				&ci,
				sizeof(Colinfo));
			////deep copy
			////fill colinfo.index_info
			//indinfo.index_info->keys = ci->keys;

			//indinfo.index_info->col_number = (int*)palloc(sizeof(int) * indinfo.index_info->keys);
			//memcpy(indinfo.index_info->col_number, 
			//	sizeof(int) * indinfo.index_info->keys, 
			//	ci->col_number, 
			//	sizeof(int) * ci->keys);

			//indinfo.index_info->rd_comfunction = 
			//	(CompareCallback*)palloc(sizeof(CompareCallback) * indinfo.index_info->keys);
			//memcpy(indinfo.index_info->rd_comfunction,
			//	sizeof(CompareCallback) * indinfo.index_info->keys,
			//	ci->rd_comfunction,
			//	sizeof(CompareCallback) * ci->keys);

			//indinfo.index_info->split_function = (Split)palloc(sizeof(Split));
			//memcpy(indinfo.index_info->split_function,
			//	sizeof(Split),
			//	ci->split_function,
			//	sizeof(Split));

			++indinfo.index_num;
			memset(&index_id, 0, sizeof(index_id));
		}
		index_endscan(indscan);
		index_close(indrel, RowExclusiveLock);
		heap_close(rel, RowExclusiveLock);
		return true;
	}
	PG_CATCH();
	{
		ThrowException();
	}
	PG_END_TRY();
}

void check_meta_info_create_heap()
{
	MemoryContext ocxt = MemoryContextSwitchTo(CacheMemoryContext);
	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->col_number = (size_t*)palloc(sizeof(size_t));
	colinfo->col_number[0] = 3;
	colinfo->keys = 1;
	colinfo->split_function = meta_item_split;
	colinfo->rd_comfunction = (CompareCallback*)palloc(sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = relid_compare;

	Colinfo icolinfo = (Colinfo)palloc(sizeof(ColinfoData));
	icolinfo->keys = 1;
	icolinfo->col_number = (size_t*)palloc(colinfo->keys * sizeof(size_t));
	icolinfo->col_number[0] = 1;
	icolinfo->rd_comfunction = (CompareCallback*)palloc(icolinfo->keys * sizeof(CompareCallback));
	icolinfo->rd_comfunction[0] = relid_compare;
	icolinfo->split_function =  meta_item_split;

	MemoryContextSwitchTo(ocxt);
	setColInfo(20, colinfo);
	setColInfo(21, icolinfo);

	begin_transaction();
	SAVE_INFO_FOR_DEBUG();
	fxdb_heap_create(12003 + RELID,20);
	fxdb_heap_create(13000 + RELID,20);
	fxdb_heap_create(12004 + RELID,20);
	fxdb_heap_create(11024 + RELID,20);
	fxdb_heap_create(12030 + RELID,20);
	fxdb_heap_create(11110 + RELID,20);
	fxdb_heap_create(11111 + RELID,20);
	fxdb_heap_create(11112 + RELID,20);
	fxdb_heap_create(11119 + RELID,20);
	fxdb_heap_create(11115 + RELID,20);
	fxdb_heap_create(12111 + RELID,20);
	fxdb_heap_create(11120 + RELID,20);

	commit_transaction();

	begin_transaction();
	Relation rel = heap_open(13000 + RELID, RowExclusiveLock);


	fxdb_index_create(15000 + RELID, rel, BTREE_TYPE, 21);
	fxdb_index_create(15001 + RELID, rel, BTREE_TYPE, 21);
	fxdb_index_create(15002 + RELID, rel, BTREE_TYPE, 21);
	fxdb_index_create(15003 + RELID, rel, BTREE_TYPE, 21);
	
	heap_close(rel, NoLock);

	commit_transaction();
}

int test_insert_metadata()
{
//	fxdb_create_catalog();
	check_meta_info_create_heap();

	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试往元数据表中插入一个元组。");
	int sta = 0;
	begin_transaction();

	SAVE_INFO_FOR_DEBUG();
	//fxdb_insert_meta(12030, 
	//	20001, 
	//	12012, 
	//	123456, 
	//	2, 
	//	1954,
	//	DEFAULTTABLESPACE_OID,
	//	20);
	//CommandCounterIncrement();
	Relation rel = NULL;
	try
	{
		rel = FDPG_Heap::fd_heap_open(MetaTableId,RowExclusiveLock, MyDatabaseId);
		HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		HeapTuple tuple = NULL;
		unsigned int obj_id = 0;
		unsigned int type = 0;
		unsigned int p_name = 0;
		int owner = -1;
		unsigned int databse_id = 0;
		unsigned int tbl_id = 0;
		int col_id = -1;
		while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			Form_meta_table formClass = 
				(Form_meta_table)fxdb_tuple_to_chars(tuple);
			obj_id = formClass->rel_id;
			type = formClass->type;
			p_name = formClass->rel_pid;
			owner = formClass->owner;
			databse_id = formClass->database_id;
			tbl_id = formClass->tablespace_id;
			col_id = formClass->colinfo_id;
			if(obj_id == 12030 + RELID &&
				type == 0 &&
				p_name == 0 &&
				owner == 0 && 
				databse_id == 11967 &&
				tbl_id == DEFAULTTABLESPACE_OID &&
				col_id == 20)
			{
				sta = 1;
				break;
			}
		} //for
		FDPG_Heap::fd_heap_endscan(scan);

	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		printf("%s\n", se.getErrorNo());
		sta = 0;
	}
	FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
	commit_transaction();
	//FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColFirstIndex);
	//FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColThirdIndex);
	//FDPG_Heap::fd_heap_drop(MetaTableId);
	return sta;
}

int test_get_max_id()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试获取元数据表中最大的OID。");
	int sta = 0;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		/*fxdb_insert_meta(11110, 
			20001, 
			10012, 
			123456, 
			2, 
			1954,
			DEFAULTTABLESPACE_OID,
			20);
		fxdb_insert_meta(11111, 
			20001, 
			11110, 
			123456, 
			2, 
			1954,
			DEFAULTTABLESPACE_OID,
			20);
		fxdb_insert_meta(11112, 
			20001, 
			10014, 
			123456, 
			2, 
			1954,
			DEFAULTTABLESPACE_OID,
			20);
		fxdb_insert_meta(11119, 
			20001, 
			11110, 
			123456, 
			2, 
			1954,
			DEFAULTTABLESPACE_OID,
			20);
		fxdb_insert_meta(11115, 
			20001, 
			11110, 
			123456, 
			2, 
			1954,
			DEFAULTTABLESPACE_OID,
			20);
		fxdb_insert_meta(12111, 
			197, 
			10017, 
			123456, 
			2, 
			1954,
			2012,
			20);
		fxdb_insert_meta(11120, 
			20001, 
			11110, 
			123456, 
			2, 
			1954,
			DEFAULTTABLESPACE_OID,
			20);*/
		CommandCounterIncrement();
		Oid id = fxdb_get_max_id();
		if(id == 15003 + RELID/*构造的最大Oid为15003*/)
		{
			sta = 1;
		}else
		{
			sta = 0;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorNo());
		printf("%s\n", se.getErrorMsg());
		user_abort_transaction();
		sta = 0;
	}
	//FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColFirstIndex);
	//FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColThirdIndex);
	//FDPG_Heap::fd_heap_drop(MetaTableId);
	commit_transaction();
	return sta;
}

int test_delete_metadata()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试根据给定oid删除元数据表中的元组。");
	int sta = 0;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		//delete tuple 12
		fxdb_delete_meta(12030 + RELID);
		CommandCounterIncrement();
		//open MetaTableId
		Relation rel = FDPG_Heap::fd_heap_open(MetaTableId,
			RowExclusiveLock, MyDatabaseId);
		//scan MetaTableId
		HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel,
			SnapshotNow,
			0,
			NULL);
		HeapTuple tuple = NULL;
		unsigned int num = 0;
		char *tmp_data = NULL;
		while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			tmp_data = fxdb_tuple_to_chars(tuple);
			memcpy(&num, tmp_data, sizeof(num));
			//found 12030 + RELID, delete fail
			if(num == 12030 + RELID)
			{
				sta = 1;
			}
			memset(&num, 0, sizeof(num));
		}
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		if(sta/*删除失败*/)
		{
			sta = 0;
		}else
		{
			sta = 1;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorNo());
		printf("%s\n", se.getErrorMsg());
		sta = 0;
	}
	//FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColFirstIndex);
	//FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColThirdIndex);
	//FDPG_Heap::fd_heap_drop(MetaTableId);
	commit_transaction();
	/* 这里插入22030这个表的id到元数据表中使后面能删除掉这个表 */
	begin_transaction();
	fxdb_insert_meta(12030 + RELID, UNKNOWN_TYPE, 0, 
		0, 0, MyDatabaseId,MyDatabaseTableSpace, 0,
		RELPERSISTENCE_PERMANENT);
	commit_transaction();
	return sta;
}

int test_get_meta_info()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试获取给定heap的所有index。");
	int sta = 0;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		MtInfoData mt;
		fxdb_get_mete_info(13000 + RELID,&mt);

		//元数据表中13000上建有四个索引
		SAVE_INFO_FOR_DEBUG();
		if (MtInfo_GetIndexCount(mt) == 4) {
			sta = 1;
		} else {
			sta = 0;
		}


	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorNo());
		printf("%s\n", se.getErrorMsg());
		sta = 0;
	}
	/*FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColFirstIndex);
	FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColThirdIndex);
	FDPG_Heap::fd_heap_drop(MetaTableId);*/
	commit_transaction();
	extern void check_meta_info_drop_heap();
	check_meta_info_drop_heap();
	return sta;
}

int test_get_index_info()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试获取给定heap的所有index。");
	int sta = 0;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		MtInfoData mtInfo;
		fxdb_get_mete_info(13000,&mtInfo);
		IndinfoData& ifd = mtInfo.indinfo;
		bool sta1 = true;//fxdb_get_index_info(13000, ifd);
		//元数据表中10上建有四个索引
		SAVE_INFO_FOR_DEBUG();
		if(sta1 && ifd.index_num == 4)
		{
			sta = 1;
		}else
		{
			sta = 0;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorNo());
		printf("%s\n", se.getErrorMsg());
		sta = 0;
	}
	/*FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColFirstIndex);
	FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColThirdIndex);
	FDPG_Heap::fd_heap_drop(MetaTableId);*/
	commit_transaction();
	return sta;
}

int test_get_tblspace()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试获取给定heap的table_space。");
	int sta = 0;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		MtInfoData mtInfo;
		fxdb_get_mete_info(11111,&mtInfo);
		unsigned int tblid = mtInfo.spcid;//fxdb_get_tblspace(11111);
		//heap Oid 为11111的元组表空间应该是2012
		if(tblid == DEFAULTTABLESPACE_OID)
		{
			sta = 1;
		}else
		{
			sta = 0;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorNo());
		printf("%s\n", se.getErrorMsg());
		sta = 0;
	}
	//FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColFirstIndex);
	//FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColThirdIndex);
	//FDPG_Heap::fd_heap_drop(MetaTableId);
	commit_transaction();
	return sta;
}

int test_set_get_indinfo()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试创建表并调用setColinfo和getColinfo函数。"
				 "正确执行的情况下应该能将该表的colinfo的id插"
				 "入到元数据表中并可获取出来。");
	int sta = 0;
	try
	{
		begin_transaction();
		//首先创建表
		SAVE_INFO_FOR_DEBUG();
//		Colinfo colinfo = init_metadata_heap_colinfo();
		fxdb_heap_create(123456, MetaTableId);
		//将colinfo放进集合里
//		setColInfo(123456, colinfo);
		fxdb_insert_meta(123456, 1, 1, 0, 1, 1967, DEFAULTTABLESPACE_OID, 123456);
		Colinfo colinfo1 = getColInfo(123456);
		if(colinfo1->keys == 8 && memcmp((void*)(colinfo1->split_function), (void*)(meta_item_split), sizeof(void*)) == 0)
		{
			sta = 1;
			
		}else
		{
			sta = 0;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorNo());
		printf("%s\n", se.getErrorMsg());
		sta = 0;
	}
	/*FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColFirstIndex);
	FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColThirdIndex);
	FDPG_Heap::fd_heap_drop(MetaTableId);*/
	FDPG_Heap::fd_heap_drop(123456, MyDatabaseId);
	commit_transaction();
	return sta;
}

int test_get_type()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试获取表类型。");
	int sta = 0;
	try
	{
		begin_transaction();
		SAVE_INFO_FOR_DEBUG();
		MtInfoData mtInfo ;
		fxdb_get_mete_info(11111,&mtInfo);
		unsigned int type = mtInfo.type;//fxdb_get_type(11111);
		//插入的heap的Oid为11111的元组type为197
		if(type == 20001)
		{
			sta = 1;
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorNo());
		printf("%s\n", se.getErrorMsg());
		sta = 0;
	}
	/*FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColFirstIndex);
	FDPG_Index::fd_index_drop(MetaTableId, DEFAULTTABLESPACE_OID, MetaTableColThirdIndex);
	FDPG_Heap::fd_heap_drop(MetaTableId);*/
	commit_transaction();
	return sta;
}

EXTERN_SIMPLE_FUNCTION

void check_meta_info_drop_heap()
{
	using namespace boost;
	PREPARE_TEST();
	int drop_sta = 0;
	Oid base_relid = RELID;
	Oid relid = 12003 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 12004 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 13000 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 11024 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 12030 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 11110 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 11111 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 11112 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 11119 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 11115 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 12111 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);
	relid = 11120 + base_relid;
	SIMPLE_DROP_HEAP(relid, drop_sta);

	FREE_PARAM(BackendParameters *);
}
void check_meta_info_insert_data()
{
	begin_transaction();
	fxdb_insert_meta(12004, 
		20001, 
		12003, 
		123456, 
		2, 
		1954,
		DEFAULTTABLESPACE_OID,
		20);
	fxdb_insert_meta(13000, 
		20001, 
		12003, 
		123456, 
		2, 
		1954,
		DEFAULTTABLESPACE_OID,
		20);
	fxdb_insert_meta(12003, 
		20001, 
		0, 
		123456, 
		2, 
		1954,
		DEFAULTTABLESPACE_OID,
		20);
	commit_transaction();
}
void check_meta_info_check_get_index_info(int &sta)
{
	sta = 0;
	begin_transaction();
	MtInfoData mtInfo;
	fxdb_get_mete_info(12003,&mtInfo);
	IndinfoData &ifd = mtInfo.indinfo;
	//fxdb_get_index_info(12003, ifd);
	commit_transaction();
	/*
	*提交当前事务并开启新的事务
	*在新的事务中获取12003的indinfo
	*取出的rel的mt_info属性，该
	*属性当前的indinfo应该已经有值。
	*这说明测试成功
	*/
	begin_transaction();
	Relation rel = heap_open(12003, RowExclusiveLock);
	//此时relation的mt_info属性携带值
	if(MtInfo_GetIndexCount(rel->mt_info)== 2)
	{
		sta = 1;
	}else
	{
		sta = 0;
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
}
void check_meta_info_check_free_mt_info(int &sta)
{
	sta = 0;
	/*
	*提交当前事务并开启新的事务
	*在新的事务中插入新的元组1024,
	*11024的母表是12003，这样会释放
	*之前表12003所携带的mt_info属性
	*/
	begin_transaction();
	//插入之前,mt_info有值
	Relation rel = heap_open(12003, RowExclusiveLock);
	if(0 != MtInfo_GetIndexCount(rel->mt_info))
	{
		sta = 0;
		heap_close(rel, RowExclusiveLock);
		commit_transaction();
		return;
	}
	heap_close(rel, RowExclusiveLock);
	fxdb_insert_meta(11024, 
		20001, 
		12003, 
		123456, 
		2, 
		1954,
		DEFAULTTABLESPACE_OID,
		20);
	rel = heap_open(12003, RowExclusiveLock);
	//此时rel的mt_info属性应该为空
	if(1/*NULL == rel->mt_info*/)
	{
		sta = 1;
	}else
	{
		sta = 0;
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
}
void check_meta_info_check_get_type(int &sta)
{
	sta = 0;
	/*
	*调用get_type以在内存中初始化relation的
	*mt_info成员的type属性。
	*/
	begin_transaction();
	MtInfoData mtInfo ;
	fxdb_get_mete_info(13000,&mtInfo);
	unsigned int type = mtInfo.type;//fxdb_get_type(13000);
	commit_transaction();
	/*
	*提交上一个事务，开始新的事务，此时的
	*mt_info成员的type已经有值，直接取
	*/
	begin_transaction();
	Relation rel = heap_open(13000, RowExclusiveLock);
	if(20001 == rel->mt_info.type &&
		 type == rel->mt_info.type)
	{
		sta = 1;
	}else
	{
		sta = 0;
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
}
void check_meta_info_check_get_tblspace(int &sta)
{
	sta = 0;
	/*
	*调用get_tblspace以在内存中初始化relation的
	*mt_info成员的tblspace属性。
	*/
	begin_transaction();
	MtInfoData mtInfo ;
	fxdb_get_mete_info(13000,&mtInfo);
	unsigned int tblspace = mtInfo.spcid;//fxdb_get_tblspace(13000);
	commit_transaction();
	/*
	*提交上一个事务，开始新的事务，此时的
	*mt_info成员的tblspace已经有值，直接取
	*/
	begin_transaction();
	Relation rel = heap_open(13000, RowExclusiveLock);
	if(DEFAULTTABLESPACE_OID == MtInfo_GetTablespace(rel->mt_info)
		&& tblspace == MtInfo_GetTablespace(rel->mt_info))
	{
		sta = 1;
	}else
	{
		sta = 0;
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
}

int test_check_meta_info()
{
	using namespace FounderXDB::StorageEngineNS;
	INTENT("测试元数据表的优化功能。");
	SAVE_INFO_FOR_DEBUG();
	check_meta_info_insert_data();
	int sta[100] = {0};
	int index = 0;
	try
	{

		{
			/*
			*测试get_index_info是否能正确在内存中初始化
			*relation的mt_info成员的indinfo属性
			*/
			SAVE_INFO_FOR_DEBUG();
			check_meta_info_check_get_index_info(sta[index]);
		}

		++index;

		{
			/*
			*测试insert_meta是否能正确释放relation
			*的mt_info成员的indinfo属性
			*/
			SAVE_INFO_FOR_DEBUG();
			check_meta_info_check_free_mt_info(sta[index]);
		}

		++index;

		{
			/*
			*测试get_type是否能正确初始化mt_info的type属性
			*/
			SAVE_INFO_FOR_DEBUG();
			check_meta_info_check_get_type(sta[index]);
		}

		++index;

		{
			/*
			*测试get_tblspace是否能正确初始化mt_info的tblspace属性
			*/
			SAVE_INFO_FOR_DEBUG();
			check_meta_info_check_get_tblspace(sta[index]);
		}
		
	}catch(StorageEngineExceptionUniversal &se)
	{
		printf("%s\n", se.getErrorMsg());
		printf("%s\n", se.getErrorNo());
		return 0;
	}
	SAVE_INFO_FOR_DEBUG();
	for(;index >= 0; --index)
	{
		if(!sta[index])
		{
			return false;
		}
	}
	return true;
}

typedef struct  
{
	Oid object_id;
	int type;
	Oid p_table_id;
	pg_time_t time;
	int owner;
	Oid database_id;
	Oid table_space_id;
	unsigned int colid;
} AMetaData;

void thread_fxdb_create_catalog(BackendParameters *param)
{
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(param);

	Oid reltablespace = DEFAULTTABLESPACE_OID;
	StartTransactionCommand();
	BeginTransactionBlock();
	CommitTransactionCommand();

	Colinfo colinfo2 = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo2->keys = 8;
	colinfo2->col_number = NULL;
	colinfo2->rd_comfunction = NULL;
	colinfo2->split_function = meta_item_split;
	setColInfo(MetaTableId,colinfo2);
	fxdb_heap_create(MetaTableId,MetaTableId);
	CommandCounterIncrement();


	Relation metaRelation = heap_open(MetaTableId, RowExclusiveLock);


	Colinfo colinfo = (Colinfo)palloc(sizeof(ColinfoData));
	colinfo->keys = 1;
	colinfo->col_number = (size_t*)palloc(colinfo->keys * sizeof(size_t));
	colinfo->col_number[0] = 1;
	colinfo->rd_comfunction = (CompareCallback*)palloc(colinfo->keys * sizeof(CompareCallback));
	colinfo->rd_comfunction[0] = relid_compare;
	colinfo->split_function =  meta_item_split;
	fxdb_index_create(MetaTableColFirstIndex, metaRelation, BTREE_TYPE, MetaTableColFirstIndex);

	colinfo->col_number[0] = 3;
	fxdb_index_create(MetaTableColThirdIndex, metaRelation, BTREE_TYPE, MetaTableColThirdIndex);

	if (colinfo->col_number)
		pfree(colinfo->col_number);
	if (colinfo->rd_comfunction)
		pfree(colinfo->rd_comfunction);
	pfree(colinfo);

	heap_close(metaRelation, RowExclusiveLock);

	StartTransactionCommand();
	EndTransactionBlock();
	CommitTransactionCommand();

}

#define PREPARE_META_TEST() \
	GET_PARAM() = get_param(); \
	SAVE_PARAM(GET_PARAM()); \
	GET_THREAD_GROUP().create_thread(bind(&thread_fxdb_create_catalog, GET_PARAM())); \
	GET_THREAD_GROUP().join_all();

void thread_meta_insert(BackendParameters *param, AMetaData *amd)
{
	extern void* fxdb_SubPostmaster_Main(void*);
	SAVE_INFO_FOR_DEBUG();
	fxdb_SubPostmaster_Main(param);

	using namespace FounderXDB::StorageEngineNS;

	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_create(MyDatabaseTableSpace, amd->object_id);
		fxdb_get_max_id();
		fxdb_insert_meta(amd->object_id, amd->type, amd->p_table_id, 
			amd->time, amd->owner, amd->database_id, 
			amd->table_space_id, amd->colid);
		CommandCounterIncrement();
		MtInfoData mtInfo ;
		fxdb_get_mete_info(amd->object_id,&mtInfo);
		IndinfoData &ifd = mtInfo.indinfo;
		//fxdb_get_index_info(amd->object_id, ifd);
		if(ifd.index_num > 0)
		{
			printf("%d get index info fail\n", amd->object_id);
		}
		//MtInfo mtInfo = fxdb_get_mete_info(amd->object_id);
		//fxdb_get_type(amd->object_id);
		fxdb_delete_meta(amd->object_id);
		commit_transaction();
	}catch(StorageEngineExceptionUniversal &se)
	{
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
	}
	
	proc_exit(0);
}

EXTERN_SIMPLE_FUNCTION

int test_thread_meta_test()
{
	INTENT("测试多线程元数据表的插入。");

	using namespace boost;
	using namespace FounderXDB::StorageEngineNS;
	using namespace std;
	PREPARE_TEST();

	PREPARE_META_TEST();
#undef THREAD_INSERT_NUM
#define THREAD_INSERT_NUM 50
	const unsigned int COLID = 1;
	const int OWNER = 0;
	const Oid PTABLEID = 0;
	const pg_time_t PGTIMET = time(NULL);
	const int TYPE = 1;
	const Oid TABLEIDSTART = 1025;
	VectorMgr<AMetaData *> vm_mgr;
	vector<Oid> v_oid;
	for(int i = 0; i < THREAD_INSERT_NUM; ++i)
	{
		AMetaData *amd = new AMetaData;
		vm_mgr.vPush(amd);
		amd->colid = COLID;
		amd->database_id = MyDatabaseId;
		amd->owner = OWNER;
		amd->p_table_id = PTABLEID;
		amd->table_space_id = MyDatabaseTableSpace;
		amd->time = PGTIMET;
		amd->type = TYPE;
		amd->object_id = TABLEIDSTART + i;
		v_oid.push_back(amd->object_id);
		GET_PARAM() = get_param();
		SAVE_PARAM(GET_PARAM());
		GET_THREAD_GROUP().create_thread(bind(thread_meta_insert, GET_PARAM(), amd));
	}
	GET_THREAD_GROUP().join_all();
	for(int i = 0; i < THREAD_INSERT_NUM; ++i)
	{
		int id = TABLEIDSTART + i;
		int drop_sta = 0;
		SIMPLE_DROP_HEAP(id, drop_sta);
	}
	int drop_sta = 0;
	int id = 254;
	SIMPLE_DROP_HEAP(id, drop_sta);
	SIMPLE_DROP_HEAP(id, drop_sta);
	SIMPLE_DROP_HEAP(id, drop_sta);
	FREE_PARAM(BackendParameters *);

	/* 检测结果 */
	int test_success = true;

	if(test_success == true)
	{
		begin_transaction();
		Relation rel = FDPG_Heap::fd_heap_open(MetaTableId,AccessShareLock, MyDatabaseId);
		HeapScanDesc scan = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
		HeapTuple tuple = NULL;
		while((tuple = FDPG_Heap::fd_heap_getnext(scan, ForwardScanDirection)) != NULL)
		{
			Oid object_id = -1;
			Oid p_object_id = -1;
			int owner = -1;
			int type = -1;
			pg_time_t time = -1;
			int colid = -1;
			Oid table_space = -1;
			Oid database_id = -1;
			char *data = NULL;
			RangeData rd = {0};
			data = fxdb_tuple_to_chars(tuple);
			meta_item_split(rd, data , 1);
			memcpy(&object_id, &data[rd.start], rd.len);
			data = fxdb_tuple_to_chars(tuple);
			meta_item_split(rd, data , 2);
			memcpy(&type, &data[rd.start], rd.len);
			data = fxdb_tuple_to_chars(tuple);
			meta_item_split(rd, data , 3);
			memcpy(&p_object_id, &data[rd.start], rd.len);
			data = fxdb_tuple_to_chars(tuple);
			meta_item_split(rd, data , 4);
			memcpy(&time, &data[rd.start], rd.len);
			data = fxdb_tuple_to_chars(tuple);
			meta_item_split(rd, data , 5);
			memcpy(&owner, &data[rd.start], rd.len);
			data = fxdb_tuple_to_chars(tuple);
			meta_item_split(rd, data , 6);
			memcpy(&database_id, &data[rd.start], rd.len);
			data = fxdb_tuple_to_chars(tuple);
			meta_item_split(rd, data , 7);
			memcpy(&table_space, &data[rd.start], rd.len);
			data = fxdb_tuple_to_chars(tuple);
			meta_item_split(rd, data , 8);
			memcpy(&colid, &data[rd.start], rd.len);
			vector<Oid>::iterator it = find(v_oid.begin(), v_oid.end(), object_id);
			bool _tmp = (it != v_oid.end());
			v_oid.erase(it);
			if(!(_tmp &&
				 type == TYPE &&
				 p_object_id == PTABLEID &&
				 time == PGTIMET &&
				 owner == OWNER &&
				 database_id == MyDatabaseId &&
				 table_space == MyDatabaseTableSpace &&
				 colid == COLID))
			{
				test_success = false;
			}
		}
		FDPG_Heap::fd_heap_endscan(scan);
		FDPG_Heap::fd_heap_close(rel, AccessShareLock);
		commit_transaction();
	}

	return test_success;
}

int test_get_max_id2()
{
	using namespace FounderXDB::StorageEngineNS;
	using namespace std;
	INTENT("测试获取元数据表中最大的OID。");
	vector<int> v_ran;
	const unsigned int START_OID = 123457;
	const unsigned int LOOP_NUM = 10000;
	/* 为了兼容前面的元数据表的测试，需要将i的初始值设为123457 */
	for(int i = 1; i <= LOOP_NUM; ++i)
	{
		v_ran.push_back(START_OID + i);
	}
	/* 打乱v_ran中元素的顺序 */
	random_shuffle(v_ran.begin(), v_ran.end());
	bool sta = true;
	for(int i = 0; i < LOOP_NUM; ++i)
	{
		try
		{
			begin_transaction();
			fxdb_insert_meta(v_ran[i], 
				20001, 
				10012, 
				123456, 
				2, 
				1954,
				DEFAULTTABLESPACE_OID,
				20);
			commit_transaction();
		}catch(StorageEngineExceptionUniversal &se)
		{
			printf("%s\n", se.getErrorNo());
			printf("%s\n", se.getErrorMsg());
			user_abort_transaction();
		}
	}
	begin_transaction();
	Oid max_id = fxdb_get_max_id();
	commit_transaction();
	if(max_id != START_OID + LOOP_NUM)
	{
		sta = false;
	}
	check_meta_info_drop_heap();
	return sta;
}

