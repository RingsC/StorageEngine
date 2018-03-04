#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>
#include "utils/util.h"

#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "utils/tuplestore.h"
#include "utils/tuplesort.h"
#include "utils/fmgroids.h"

#include "test_fram.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"

#include "heap_sort_store/heap_store.h"


using namespace FounderXDB::StorageEngineNS;

extern int RELID;

int test_simple_tuple_store()
{
	INTENT("建表并插入若干数据。扫描表并将tuple放入Tuplestorestate中。"
				 "遍历Tuplestorestate并且应该能找到扫描出来的所有tuple。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace std;

	bool return_sta = true;
	vector<string> v_data;
	++RELID;
	create_heap();

#undef DATA_ARR_LEN
#define DATA_ARR_LEN 100

	/* 构造数据 */
	char data[DATA_ARR_LEN][DATA_LEN];
	DataGenerater dg(DATA_ARR_LEN, DATA_LEN);
	dg.dataGenerate();
	dg.dataToDataArray2D(data);

	/* 插入数据 */
	insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN, RELID);

	/* 扫描所有的tuple并放入Tuplestorestate中 */
	PG_TRY();
	{
		Tuplestorestate *tss = NULL;

		{
			tss = tuplestore_begin_heap(false, true, DATA_ARR_LEN * DATA_LEN);
			begin_transaction();
			Relation rel = FDPG_Heap::fd_heap_open(RELID, AccessShareLock);
			HeapScanDesc hsd = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
			HeapTuple tuple = NULL;
			while((tuple = FDPG_Heap::fd_heap_getnext(hsd, ForwardScanDirection)) != NULL)
			{
				/* 将tuple放入tss */
				tuplestore_puttuple(tss, tuple);
			}
			heap_endscan(hsd);
			heap_close(rel, RowExclusiveLock);
			commit_transaction();
		}

		{
			/* tss中现在已经存有所有的tuple，开始遍历tss。 */
			MinimalTuple m_tuple = NULL;
			HeapTuple tuple = NULL;
			bool should_free;
			while((m_tuple = (MinimalTuple)tuplestore_gettuple(tss, true, &should_free)) != NULL)
			{
				char *data = NULL;
				tuple = heap_tuple_from_minimal_tuple(m_tuple);
				data = fxdb_tuple_to_chars(tuple);
				v_data.push_back(data);
				pfree(data);
				pfree(tuple);
				if(should_free)
				{
					pfree(m_tuple);
				}
			}
			if(v_data.size() != DATA_ARR_LEN)
			{
				return_sta = false;
			}
		}

		/* 检查结果 */
		{
			sort(v_data.begin(), v_data.end());
			vector<string> v_cmp_data;
			for(int i = 0; i < DATA_ARR_LEN; ++i)
			{
				v_cmp_data.push_back(data[i]);
			}
			sort(v_cmp_data.begin(), v_cmp_data.end());
			return_sta = (v_data == v_cmp_data ? true : false);
		}

		tuplestore_end(tss);
	}
	PG_CATCH();
	{
		user_abort_transaction();
		StorageEngineExceptionUniversal se;
		printf("%s\n", se.getErrorMsg());
		return_sta = false;
	}
	PG_END_TRY();

	remove_heap();
	return return_sta;
}

static int sort_compare(char *data1, int len1, char *data2, int len2)
{
	if(len1 != len2)
	{
		return (len1 > len2) ? 1 : -1;
	}

	return memcmp(data1, data2, len1);
}

int test_simple_tuple_sort()
{
	INTENT("建表并插入若干数据。扫描表并将tuple放入Tuplestorestate中。"
		"执行排序算法，检测Tuplestorestate中所有元组是否正确排序.");

	using namespace FounderXDB::StorageEngineNS;
	using namespace std;

	bool return_sta = true;
	vector<string> v_data;
	vector<string> v_data2;
	++RELID;
	create_heap();

#undef DATA_ARR_LEN
#define DATA_ARR_LEN 5

	/* 构造数据 */
	char data[DATA_ARR_LEN][DATA_LEN] = 
	{
		"decax", ///4
		"bacde", ///2
		"abcde", ///1
		"cdiek", ///3
		"esdie"  ///5
	};

	/* 插入数据 */
	insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN, RELID);

	/* 扫描所有的tuple并放入Tuplestorestate中 */
	PG_TRY();
	{
		extern TupleDesc single_attibute_tupDesc;

		Tuplesortstate *tss = NULL;
		Tuplesortstate *tss2 = NULL;
		{
			tss = tuplesort_begin_data(DATA_ARR_LEN * DATA_LEN, false, (void*)sort_compare);
			tss2 = tuplesort_begin_heap(single_attibute_tupDesc, DATA_ARR_LEN * DATA_LEN, true, (void*)sort_compare);
			begin_transaction();
			Relation rel = FDPG_Heap::fd_heap_open(RELID, AccessShareLock);
			HeapScanDesc hsd = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
			HeapTuple tuple = NULL;
			while((tuple = FDPG_Heap::fd_heap_getnext(hsd, ForwardScanDirection)) != NULL)
			{
				/* 将tuple放入tss */
				int len = 0;
				char *data = NULL;
				data = fdxdb_tuple_to_chars_with_len(tuple, len);
				tuplesort_putdata(tss, (void*)data, len);
				tuplesort_putheaptuple(tss2, tuple);
			}
			heap_endscan(hsd);
			heap_close(rel, RowExclusiveLock);
			commit_transaction();
		}

		/// 执行排序算法
		tuplesort_performsort(tss);
		tuplesort_performsort(tss2);

		{
			/* tss中现在已经存有所有的tuple，开始遍历tss。 */
			void *data;
			int len = 0;
			while(tuplesort_getdata(tss, true, &data, &len))
			{
				v_data.push_back((char*)data);
			}
			if(v_data.size() != DATA_ARR_LEN)
			{
				return_sta = false;
			}
		}

		{
			/* tss中现在已经存有所有的tuple，开始遍历tss。 */
			bool should_free = false;
			HeapTuple tup = NULL;
			tuplesort_markpos(tss2);
			while(tup = tuplesort_getheaptuple(tss2, true, &should_free));
			tuplesort_restorepos(tss2);
			while(tup = tuplesort_getheaptuple(tss2, true, &should_free))
			{
				v_data2.push_back(fxdb_tuple_to_chars(tup));
			}
			if(v_data2.size() != DATA_ARR_LEN)
			{
				return_sta = false;
			}
		}
		tuplesort_end(tss);
		tuplesort_end(tss2);

		/* 检查结果 */
		{
			sort(v_data.begin(), v_data.end());
			sort(v_data2.begin(), v_data2.end());
			vector<string> v_cmp_data;
			for(int i = 0; i < DATA_ARR_LEN; ++i)
			{
				v_cmp_data.push_back(data[i]);
			}
			sort(v_cmp_data.begin(), v_cmp_data.end());
			return_sta = (v_data == v_cmp_data && v_data2 == v_cmp_data ? true : false);
		}
	}
	PG_CATCH();
	{
		user_abort_transaction();
		StorageEngineExceptionUniversal se;
		printf("%s\n", se.getErrorMsg());
		return_sta = false;
	}
	PG_END_TRY();

	remove_heap();
	return return_sta;
}

static int int_compare(char *data1, int len1, char *data2, int len2)
{
	int a = atoi(data1);
	int b = atoi(data2);
	if(a == b)
	    return 0;
	return a < b ? -1 : 1;
}

int test_sorted_tuple_sort()
{
    INTENT("对已经有序的排序表排序.");

	using namespace FounderXDB::StorageEngineNS;
	using namespace std;

	bool return_sta = true;
	vector<string> v_data;
	vector<string> v_data2;
	++RELID;
	create_heap();
    
#undef DATA_ARR_LEN
#define DATA_ARR_LEN 100
	/* 插入数据 */
	using namespace FounderXDB::StorageEngineNS;
	begin_transaction();
	Oid relspace = MyDatabaseTableSpace;
	Relation rel = FDPG_Heap::fd_heap_open(RELID, RowExclusiveLock, MyDatabaseId);
	HeapTuple tuple = NULL;
	try
	{
	    int data_len = 0;
	    char insert_data[10];//10为最大int的字符串长度
	    extern char *my_itoa(int value, char *string, int radix);
		for(int i = 0; i < DATA_ARR_LEN; ++i)
		{
		    memset(insert_data, 0, 10);
			SAVE_INFO_FOR_DEBUG();
		    my_itoa(i, insert_data, 10);
		    data_len = strlen(insert_data);
			tuple = fdxdb_heap_formtuple(insert_data, data_len);
			FDPG_Heap::fd_simple_heap_insert(rel, tuple);
			CommandCounterIncrement();
		}
	}catch(StorageEngineExceptionUniversal &se)
	{
		heap_close(rel, RowExclusiveLock);
		user_abort_transaction();
		printf("%s\n", se.getErrorMsg());
		throw StorageEngineExceptionUniversal(se);
	}
	heap_close(rel, RowExclusiveLock);
	commit_transaction();
	
	/* 扫描所有的tuple并放入Tuplestorestate中 */
	PG_TRY();
	{
		extern TupleDesc single_attibute_tupDesc;
        
        
		Tuplesortstate *tss = NULL;
		Tuplesortstate *tss2 = NULL;
		{
			tss = tuplesort_begin_heap(single_attibute_tupDesc, 5*1024, true, (void*)int_compare);
			tss2 = tuplesort_begin_heap(single_attibute_tupDesc, 5*1024, true, (void*)int_compare);
			begin_transaction();
			Relation rel = FDPG_Heap::fd_heap_open(RELID, AccessShareLock);
			HeapScanDesc hsd = FDPG_Heap::fd_heap_beginscan(rel, SnapshotNow, 0, NULL);
			HeapTuple tuple = NULL;
			while((tuple = FDPG_Heap::fd_heap_getnext(hsd, ForwardScanDirection)) != NULL)
			{
				tuplesort_putheaptuple(tss, tuple);
			}
			heap_endscan(hsd);
			heap_close(rel, RowExclusiveLock);
		}

		tuplesort_performsort(tss);

		{
			/* tss中现在已经存有所有的tuple，开始遍历tss。 */
			bool should_free = false;
			HeapTuple tup = NULL;
			tuplesort_markpos(tss);
			while(tup = tuplesort_getheaptuple(tss, true, &should_free));
			tuplesort_restorepos(tss);
			int ret_count = 0;
			
			while(tup = tuplesort_getheaptuple(tss, true, &should_free))
			{
				++ret_count;
				tuplesort_putheaptuple(tss2, tup);
			}
			if(ret_count != DATA_ARR_LEN)
			{
				return_sta = false;
			}
		}
		tuplesort_end(tss);
		
		//调用者确认已经有序的内容，为了避免再次排序，设置sorted状态。
		tuplesort_set_sorted(tss2);
		//tuplesort_performsort(tss2);
		
		{
		    int ret_count = 0;
		    HeapTuple tup = NULL;
		    bool should_free = false;
		    while(tup = tuplesort_getheaptuple(tss2, true, &should_free))
		    {
		        ++ret_count;
		    }
		    if(ret_count != DATA_ARR_LEN)
		    {
			    return_sta = false;
		    }
		}
		commit_transaction();
	}
	PG_CATCH();
	{
		user_abort_transaction();
		StorageEngineExceptionUniversal se;
		printf("%s\n", se.getErrorMsg());
		return_sta = false;
	}
	PG_END_TRY();

	remove_heap();
	return return_sta;
}

void tuplesort_datum_split_heap(RangeData& range,const char* str,int col,size_t len)
{
	range.start = 0;
	range.len = 0;
	if (!str)
		return;

	if (1 == col)
	{
		range.len = 2;
	}
	if (2 == col)
	{
		range.start = 2;
		range.len = 3;
	}
	if (3 == col)
	{
		range.start = 5;
		range.len = 5;
	}
}
void tuplesort_datum_split_index(RangeData& range,const char* str,int col,size_t len)
{
	range.start = 0;
	range.len = 0;
	if (!str)
		return;

	if (1 == col)
	{
		range.len = 2;
	}
	if (2 == col)
	{
		range.start = 2;
		range.len = 5;
	}
}

std::string GenerateRandomString(uint32 len)
{
	std::string strRet;

	char chDict[] = {"123456789abcdefghijklmnopqrstuvwxyz!@#$%^&*()_+|.~=-/"};
	uint32 arr_len = sizeof(chDict);

	for (uint32 i = 0; i < len; i ++)
	{
		uint32 index = rand() / (RAND_MAX/(arr_len-1));
		if (index < arr_len)
		{
			char chTmp[2] = {0};
			memcpy(chTmp,chDict + index,1);
			std::string strTmp(chTmp);
			strRet += strTmp;
		}
	}

	return strRet;
}
struct TestData 
{
	TestData()
	{
		tuple_count = 0;
		tuple_len = 0;
		pData = NULL;
	}

	void Clear_Mem()
	{
		if (pData)
		{
			free(pData);
			pData = NULL;
		}
	}
	uint32 tuple_count;//tuple count in mem pData
	uint32 tuple_len;//single tuple len in mem pData
	char* pData;
	~TestData()
	{
		Clear_Mem();		
	}
};
void GetTestDataPtr(TestData& data)
{
	if (data.tuple_count == 0 || data.tuple_len == 0)
		return;

	char* pData = data.pData;
	if (!pData)
	{
		pData = (char*)malloc(data.tuple_count * data.tuple_len + 1);
		memset(pData,0,data.tuple_count * data.tuple_len + 1);
		data.pData = pData;
	}

	if (pData)
	{
		memset(pData,0,data.tuple_count * data.tuple_len);
		char* pTmp = pData;
		srand((int)time(0));
		for (uint32 i = 0; i < data.tuple_count; i ++)
		{
			std::string strRandom = GenerateRandomString(data.tuple_len);
			memcpy(pTmp,strRandom.c_str(),data.tuple_len);
			pTmp += data.tuple_len;
		}
	}

	assert(pData);
}
//3 cols in heap ,each col len : 2, 3, 5, total len :10
void SetColInfo_TupleSort_Heap(const Oid colid)
{
	Colinfo col = new ColinfoData;
	col->keys = 0;
	col->col_number = NULL;
	col->rd_comfunction = NULL;
	col->split_function = tuplesort_datum_split_heap;
	setColInfo(colid,col);
}
void SetColInfo_TupleSort_Index(const Oid colid)
{
	Colinfo col = new ColinfoData;
	col->keys = 2;
	col->col_number = new size_t[2];
	col->col_number[0] = 1;
	col->col_number[1] = 3;
	col->rd_comfunction = new CompareCallback[col->keys];
	col->rd_comfunction[0] = str_compare;
	col->rd_comfunction[1] = str_compare;
	col->split_function = tuplesort_datum_split_index;
	setColInfo(colid,col);
}
bool tuplesort_create_heap(Oid& heapId, Oid& indexId)
{
	try
	{
		begin_transaction();

		const Oid COLLUM_ID = 10001;
		SetColInfo_TupleSort_Heap(COLLUM_ID);
		heapId = FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID,InvalidOid,DEFAULTDATABASE_OID,COLLUM_ID);

		Relation rel = FDPG_Heap::fd_heap_open(heapId,ShareLock,DEFAULTDATABASE_OID);

		const Oid COLLUM_INDEX_ID = 10002;
		SetColInfo_TupleSort_Index(COLLUM_INDEX_ID);
		indexId = FDPG_Index::fd_index_create(rel,BTREE_TYPE,InvalidOid,COLLUM_INDEX_ID);

		commit_transaction();
	}
	catch(StorageEngineExceptionUniversal& ex)
	{
		heapId = InvalidOid;
		indexId = InvalidOid;
		std::cout<<ex.getErrorMsg()<<std::endl;
		user_abort_transaction();
		return false;
	}

	return true;
}

class TupleId
{
public:
	uint16		bi_hi;
	uint16		bi_lo;
	uint16      ip_posid;

	bool operator<(const TupleId& tid)
	{
		return bi_hi < tid.bi_hi && bi_lo < tid.bi_lo && ip_posid < tid.ip_posid;

	}
protected:
private:
};
void heap_remove_sort(EntrySetID heapId)
{
	try 
	{
		begin_transaction();
		FDPG_Heap::fd_heap_drop(heapId);
		commit_transaction();

	}
	catch (StorageEngineException &ex)
	{
		std::cout << ex.getErrorMsg() << std::endl;
		user_abort_transaction();		
	}
}
bool test_tuplesort_datum()
{
	bool bRet = false;
	Oid relid = InvalidOid;
	Oid indexid = InvalidOid;
	try
	{


		std::cout<<"test1"<<std::endl;
		tuplesort_create_heap(relid,indexid);

		begin_transaction();		

		Relation rel = FDPG_Heap::fd_heap_open(relid,AccessShareLock);
		Relation indexRel = FDPG_Index::fd_index_open(indexid,AccessShareLock);

		std::cout<<"test2"<<std::endl;
		Tuplesortstate* pState = NULL;
		pState = tuplesort_begin_index_datum(1024,true,indexRel);

		//tuple total len = 10 ,col1 len = 2,col2 len = 3, col3 len = 5
		//col1 and col3 have index 
		std::vector<std::string> vKeys;
		vKeys.push_back("g2g4567");
		vKeys.push_back("d2a4567");
		vKeys.push_back("1234567");
		vKeys.push_back("a2b4567");
		vKeys.push_back("c2a4567");
		vKeys.push_back("e2f4567");
		vKeys.push_back("1234567");
		vKeys.push_back("a2c4566");

		std::map<int,std::string> mapTidKey;
		ItemPointer pTid = (ItemPointer)palloc0(vKeys.size() * sizeof(ItemPointerData));
		for (int i = 0; i < vKeys.size(); ++i)
		{
			pTid[i].ip_blkid.bi_hi = 0;
			pTid[i].ip_blkid.bi_lo = 0;
			pTid[i].ip_posid = i;

			mapTidKey.insert(std::make_pair(i,vKeys.at(i)));

			Datum dat = fdxdb_string_formdatum(vKeys.at(i).c_str(),vKeys.at(i).size());
			bool bIsNull = false;
			tuplesort_putdatum(pState,dat,&pTid[i]);
		}

		std::cout<<"test3"<<std::endl;
		tuplesort_performsort(pState);

		std::cout<<"test4"<<std::endl;

		std::vector<std::string> vCmpKeys;
		std::map<int,std::string> mapCmpTidKey;
		bool bNull = false;
		Datum val;
		ItemPointerData tid;
		while (tuplesort_getdatum(pState,true,&val,&tid))
		{
			char* p = VARDATA_ANY(val);
			int nLen = VARSIZE_ANY_EXHDR(val);
			std::string str(p,nLen);
			vCmpKeys.push_back(str);
			std::cout<<str<<std::endl;

			int key = tid.ip_posid;
			mapCmpTidKey.insert(std::make_pair(key,str));
		}

		std::cout<<"test5"<<std::endl;
		tuplesort_end(pState);
		commit_transaction();

		std::sort(vKeys.begin(),vKeys.end());
		bool bKeys = (vKeys == vCmpKeys);
		bool bTid = (mapCmpTidKey == mapTidKey);

		bRet = bKeys && bTid;
	}
	catch(StorageEngineExceptionUniversal& ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
		user_abort_transaction();
		bRet = false;
	}

	std::cout<<"test5"<<std::endl;
	heap_remove_sort(relid);
	return bRet;
}