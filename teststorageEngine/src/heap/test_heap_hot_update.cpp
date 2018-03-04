/**
*@file	:test_heap_hot_update_dll.cpp
*@brief 	:test heap's hot update
*@author	:WangHao
*@date	:2012-5-18
*/
#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "postmaster/postmaster.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "utils/util.h"
#include "heap/test_heap_hot_update.h"
#include "test_fram.h"
#include <vector>

using namespace std;
using namespace FounderXDB::StorageEngineNS;
std::vector<ItemPointerData> vResult;
std::vector<ItemPointerData> vtoastResult;

extern int RID;
extern int INDEX_ID;
static Oid relid;
static Oid indid;
bool resultflag[3]={true,true,true};
int hot_str_compare(const char *bit1, size_t len1, const char *bit2, size_t len2)
{
	int i = 0;
	while(i < len1 && i < len2)
	{
		if(bit1[i] < bit2[i])
			return -1;
		else if(bit1[i] > bit2[i])
			return 1;
		else i++;
	}
	if(len1 == len2)
		return 0;
	if(len1 > len2)
		return 1;
	return -1;
}

void FillData(char* keyData,const char Fillchar, char* FillCData)
{
	memcpy(FillCData,keyData,3);
	memset(FillCData+3,Fillchar,3002);
}

void my_spilt_hot_index(RangeData& rangeData, const char* str, int col, size_t len = 0)
{
	memset(&rangeData, 0, sizeof(rangeData));
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
}

void create_Table(const int rel_id, const Colinfo heap_info)
{
	INTENT("insert data \n");
	try
	{
		begin_first_transaction();
		Oid relspace = MyDatabaseTableSpace;
		SAVE_INFO_FOR_DEBUG();
		setColInfo(rel_id, heap_info);
		FDPG_Heap::fd_heap_create(relspace, rel_id, 0, rel_id);
		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}
}

//删除测试用例创建的表
void drop_Table()
{
	try
	{
		begin_transaction();
		Oid re_lid = relid;
		FDPG_Heap::fd_heap_drop(re_lid, MyDatabaseId);
		commit_transaction();
	}	
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
	}	
}

bool test_hot_update_data()
{
	INTENT("hot update data \n");
	++RID;
	relid = RID;
	char *temp = NULL;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 1);
		create_Table(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;
		HeapTuple tuple = NULL;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		char insertdata[][10] = {"012345","123456","234567","345678","456789","56789a","6789ab"};
		for(int i=0;i<7;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(insertdata[i], sizeof(insertdata[i]))) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(testRelation, tuple) == 0);
			vResult.push_back(tuple->t_self);
			FDPG_Memory::fd_pfree(tuple);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();
		
		Relation indexRelation;
		++INDEX_ID;
		indid = INDEX_ID;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo->keys = 1;
		colinfo->col_number = (size_t*)malloc(sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->rd_comfunction = (CompareCallback*)malloc(sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = hot_str_compare;
		colinfo->split_function =  my_spilt_hot_index;
		setColInfo(indid, colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);
		FDPG_Transaction::fd_CommandCounterIncrement();
		//open index
		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);

		//进行hot 型更新数据,索引列是相同的
		HeapTuple tuple_new = FDPG_Heap::fd_heap_form_tuple("345abc", sizeof("345abc"));
		FDPG_Heap::fd_simple_heap_update(testRelation, &vResult[3], tuple_new);
		FDPG_Transaction::fd_CommandCounterIncrement();
		FDPG_Memory::fd_pfree(tuple_new);

		//索引查询
		IndexScanDesc index_scan;
		ScanKeyData key;
		int flag = 1;
		int infomask2 = 0;
		char datum_data[] = "345";
		Datum* values = (Datum *) FDPG_Memory::fd_palloc(sizeof(Datum)); //用几个datum就分配多大的空间
		*values = FDPG_Common::fd_string_formdatum(datum_data, strlen(datum_data));
		//init scankey
		FDPG_ScanKey::fd_ScanKeyInitWithCallbackInfo(&key,1,BTEqualStrategyNumber,hot_str_compare,*values);	
		FDPG_Memory::fd_pfree(values);
		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, 1, 0);
		FDPG_Index::fd_index_rescan(index_scan, &key, 1, NULL, 0);

		//因为插入时具有唯一性，所以每项查询只有一个
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			infomask2 = tuple->t_data->t_infomask2;
			temp = FDPG_Common::fd_tuple_to_chars(tuple);
			flag = strcmp(temp, "345abc");
			FDPG_Memory::fd_pfree(temp);
		}
		CHECK_BOOL(flag == 0);
		//如果infomask2不等于0x8000，hot更新不成功
		if(flag != 0 ||(flag == 0 && (infomask2&0x8000)!=0x8000))//HEAP_ONLY_TUPLE	0x8000
		{
			FDPG_Index::fd_index_endscan(index_scan);
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
			FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
			user_abort_transaction();
			drop_Table();
			return false;
		}
		
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
		commit_transaction();	
		
		drop_Table();

		FreeColiInfo(colinfo);
	}
	catch(StorageEngineExceptionUniversal &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		if (temp != NULL)
			FDPG_Memory::fd_pfree(temp);
		drop_Table();
		return false;
	}

	return true;
}

int indexquery(Relation heapRelation,	Relation indexRelation,	const char* querydata,
					int& infomask2,const char* Source,const int len)
{
	//索引查询
	IndexScanDesc index_scan;
	ScanKeyData key;
	int result = 1;
	Datum* values = (Datum *) FDPG_Memory::fd_palloc(sizeof(Datum)); //用几个datum就分配多大的空间
	*values = FDPG_Common::fd_string_formdatum(querydata, strlen(querydata));
	//init scankey
	FDPG_ScanKey::fd_ScanKeyInitWithCallbackInfo(&key,1,BTEqualStrategyNumber,hot_str_compare,*values);	
	FDPG_Memory::fd_pfree(values);
	index_scan = FDPG_Index::fd_index_beginscan(heapRelation, indexRelation, SnapshotNow, 1, 0);
	FDPG_Index::fd_index_rescan(index_scan, &key, 1, NULL, 0);

	//因为插入时具有唯一性，所以每项查询只有一个
	char *temp = NULL;
	HeapTuple tuple = NULL;
	while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
	{
		infomask2 = tuple->t_data->t_infomask2;
		temp = FDPG_Common::fd_tuple_to_chars(tuple);
		result = memcmp(temp, Source,len);
		FDPG_Memory::fd_pfree(temp);
	}
	FDPG_Index::fd_index_endscan(index_scan);
	return result;
}
bool test_hot_update_toast_data()
{
	INTENT("hot update toast data \n");
	++RID;
	relid = RID;
	try
	{
		SpliterGenerater sg;
		Colinfo heap_info = sg.buildHeapColInfo(3, 3, 2, 3000);
		create_Table(relid,heap_info);
		begin_transaction();
		Oid reltablespace = MyDatabaseTableSpace;
		Relation testRelation;
		HeapTuple tuple = NULL;
		testRelation = FDPG_Heap::fd_heap_open(relid,RowExclusiveLock, MyDatabaseId);
		char insertdata[][10] = {"012345","123456","234567","345678","456789"};
		for(int i=0;i<5;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(insertdata[i], sizeof(insertdata[i]))) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(testRelation, tuple) == 0);
			vtoastResult.push_back(tuple->t_self);
			FDPG_Memory::fd_pfree(tuple);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();

		//toast insert
		char toastinsert[2][3005];
		FillData("567",'z',toastinsert[0]);
		FillData("678",'w',toastinsert[1]);
		for(int i=0;i<2;i++)
		{
			SAVE_INFO_FOR_DEBUG();
			CHECK_BOOL((tuple = FDPG_Heap::fd_heap_form_tuple(toastinsert[i], sizeof(toastinsert[i]))) !=NULL);
			CHECK_BOOL(FDPG_Heap::fd_simple_heap_insert(testRelation, tuple) == 0);
			vtoastResult.push_back(tuple->t_self);
			FDPG_Memory::fd_pfree(tuple);
		}
		FDPG_Transaction::fd_CommandCounterIncrement();
		
		Relation indexRelation;
		++INDEX_ID;
		indid = INDEX_ID;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
		colinfo->keys = 1;
		colinfo->col_number = (size_t*)malloc(sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->rd_comfunction = (CompareCallback*)malloc(sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = hot_str_compare;
		colinfo->split_function =  my_spilt_hot_index;
		setColInfo(indid, colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);
		FDPG_Transaction::fd_CommandCounterIncrement();
		//open index
		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);

		//untoast ->toast
		//进行hot 型更新数据,索引列是相同的
		char uptoastdata[3005];
		int lentaost = sizeof(uptoastdata);
		FillData("345",'a',uptoastdata);
		HeapTuple tuple_new = FDPG_Heap::fd_heap_form_tuple(uptoastdata, 3005);
		FDPG_Heap::fd_simple_heap_update(testRelation, &vtoastResult[3], tuple_new);
		FDPG_Transaction::fd_CommandCounterIncrement();
		FDPG_Memory::fd_pfree(tuple_new);
		//索引查询
		int infomask2 = 0;
		char qutoastdata1[3005];
		FillData("345",'a',qutoastdata1);
		int flag = indexquery(testRelation,indexRelation,"345",infomask2,qutoastdata1,3005);
		CHECK_BOOL(flag == 0);
		//如果infomask2不等于0x8000，hot更新不成功
		if(flag != 0 ||(flag == 0 && (infomask2&0x8000)!=0x8000))//HEAP_ONLY_TUPLE	0x8000
		{
			resultflag[0]= false;
		}


		//toast ->untoast
		HeapTuple tuple_new2 = FDPG_Heap::fd_heap_form_tuple("567abd", sizeof("567abd"));
		FDPG_Heap::fd_simple_heap_update(testRelation, &vtoastResult[5], tuple_new2);
		FDPG_Transaction::fd_CommandCounterIncrement();
		FDPG_Memory::fd_pfree(tuple_new2);
		//索引查询
		int infomask21 = 0;
		int flag1 = indexquery(testRelation,indexRelation,"567",infomask21,"567abd",6);
		CHECK_BOOL(flag1 == 0);
		//如果infomask2不等于0x8000，hot更新不成功
		if(flag1 != 0 ||(flag1 == 0 && (infomask21&0x8000)!=0x8000))//HEAP_ONLY_TUPLE	0x8000
		{
			resultflag[1]= false;
		}

		
		//toast ->toast
		//进行hot 型更新数据,索引列是相同的
		char uptoastdata2[3005];
		FillData("678",'g',uptoastdata2);
		HeapTuple tuple_new3 = FDPG_Heap::fd_heap_form_tuple(uptoastdata2, 3005);
		FDPG_Heap::fd_simple_heap_update(testRelation, &vtoastResult[6], tuple_new3);
		FDPG_Transaction::fd_CommandCounterIncrement();
		FDPG_Memory::fd_pfree(tuple_new3);
		//索引查询
		int infomask22 = 0;
		char qutoastdata2[3005];
		FillData("678",'g',qutoastdata2);
		int flag2 = indexquery(testRelation,indexRelation,"678",infomask22,qutoastdata2,3005);
		CHECK_BOOL(flag2 == 0);
		//如果infomask2不等于0x8000，hot更新不成功
		if(flag2 != 0 ||(flag2 == 0 && (infomask22&0x8000)!=0x8000))//HEAP_ONLY_TUPLE	0x8000
		{
			resultflag[2]= false;
		}

		if (!resultflag[0] ||!resultflag[1] ||!resultflag[2])
		{
			FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
			FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
			FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
			user_abort_transaction();
			drop_Table();
			return false;
		}
		
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		FDPG_Index::fd_index_drop(relid, reltablespace,indid, MyDatabaseId);	
		commit_transaction();	

		drop_Table();

		FreeColiInfo(colinfo);
	}
	catch(StorageEngineExceptionUniversal &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		drop_Table();
		return false;
	}

	return true;
}

