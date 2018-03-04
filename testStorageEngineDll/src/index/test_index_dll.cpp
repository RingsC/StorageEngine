/************************************************************************


测试点列表：
编号		编写者      修改类型      修改时间		修改内容		
000			许彦      创建		  2011.8.18		 两字段索引，仅向前扫描，索引列为第1列和第2列
001			许彦      创建		  2011.8.22		 两字段索引，仅向后扫描，改变索引列为第2列和第3列
002			许彦      创建		  2011.8.22		 两字段索引，仅向前扫描，改变索引列顺序为先后列再前列，如索引列为先第2列后第1列
003			许彦      创建		  2011.8.24		 五字段索引，仅向前扫描，索引列为2,3,5,4,1,使用5种不同策略
004			许彦      创建		  2011.8.24		 32字段索引，仅向前扫描，索引列为1-32，插入大量数据


************************************************************************/

#include "iostream"
#include "vector"
#include <string>

#include "utils/utils_dll.h"
#include "index/test_index_dll.h"
#include "PGSETypes.h"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"

using namespace FounderXDB::StorageEngineNS;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
extern EntrySet* open_entry_set();
extern unsigned int data_len_calc(const char data[], unsigned int len);

extern EntrySetID EID;

extern void my_spilt(RangeDatai& rangeData, const char* str, int col, size_t len = 0);

typedef int (*PToCompareFunc)(const char*, size_t, const char*, size_t);

void my_split_heap_321321(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
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
	if (col == 4)
	{
		rangeData.start = 6;
		rangeData.len = 3;
	}
	if (col == 5)
	{
		rangeData.start = 9;
		rangeData.len = 2;
	}
	if (col == 6)
	{
		rangeData.start = 11;
		rangeData.len = 1;
	}
}

void my_split_heap_100(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	for(int i = 0;i<100;i++)
	{
		if (col == i+1)
		{
			rangeData.start = i;
			rangeData.len = 1;
		}
	}
}

void index_create_table_split_to_6()
{
	using namespace std;
	try
	{
		get_new_transaction();
		std::string entrySetName("test");
		EntrySet *pEntrySet = NULL;
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_split_heap_321321;
		uint32 colid = 6;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		EID = pStorageEngine->createEntrySet(pTransaction,colid);//创建一个heap
		commit_transaction();
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorMsg() << endl;
		cout << ex.getErrorNo() << endl;
	}
}

void index_create_table_split_to_100()
{
	using namespace std;
	try
	{
		get_new_transaction();
		std::string entrySetName("test");
		EntrySet *pEntrySet = NULL;
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 0;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_split_heap_100;
		uint32 colid = 7;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		EID = pStorageEngine->createEntrySet(pTransaction,colid);//创建一个heap
		commit_transaction();
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorMsg() << endl;
		cout << ex.getErrorNo() << endl;
	}
}

void index_create_table()
{
	using namespace std;
	try
	{
		get_new_transaction();
		std::string entrySetName("test");
		EntrySet *pEntrySet = NULL;
		ColumnInfo colinfo;
		colinfo.col_number = NULL;
		colinfo.keys = 3;
		colinfo.rd_comfunction = NULL;
		colinfo.split_function = my_spilt;
		uint32 colid = 5;
		ColumnInfo* p_colinfo=(ColumnInfo* )malloc(sizeof(ColumnInfo));
		memcpy(p_colinfo,&colinfo,sizeof(ColumnInfo));
		setColumnInfo(colid, p_colinfo);
		EID = pStorageEngine->createEntrySet(pTransaction,colid);//创建一个heap
		commit_transaction();
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorMsg() << endl;
		cout << ex.getErrorNo() << endl;
	}
}

void index_drop_table()
{
	using namespace std;
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

void my_split_index_12(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
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
}

void my_split_heap_321(RangeDatai& rangeData, char *str, int col)
{
	rangeData.start = 0;
	rangeData.len = 0;
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
void my_split_index_23(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 1;
	}
}

void my_split_index_21(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 3;
	}
}

void my_split_index_23541(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 1;
	}
	if (col == 3)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 4)
	{
		rangeData.start = 5;
		rangeData.len = 3;
	}
	if (col == 5)
	{
		rangeData.start = 8;
		rangeData.len = 3;
	}
}

void my_split_index_100(RangeDatai& rangeData, const char* str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	for(int i = 0;i<100;i++)
	{
		if (col == i+1)
		{
			rangeData.start = i;
			rangeData.len = 1;
		}
	}
}

int my_compare_str(const char *str1, size_t len1, const char *str2, size_t len2)
{
	int i = 0;
	while(i < len1 && i < len2)
	{
		if(str1[i] < str2[i])
			return -1;
		else if(str1[i] > str2[i])
			return 1;
		else i++;

	}
	if(len1 == len2)
		return 0;
	if(len1 > len2)
		return 1;
	return -1;
}

ColumnInfo *set_col_info(const int nkeys = 2, const int colid = 20)
{
	ColumnInfo *co = (ColumnInfo *)malloc(sizeof(ColumnInfo));
	co->keys = nkeys;
	co->col_number = (size_t*)malloc(co->keys * sizeof(size_t));
	co->col_number[0] = 1;
	co->col_number[1] = 2;
	co->rd_comfunction = (CompareCallbacki*)malloc(co->keys * sizeof(CompareCallbacki));
	co->rd_comfunction[0] = my_compare_str;
	co->rd_comfunction[1] = my_compare_str;
	co->split_function =  my_split_index_12;
	setColumnInfo(colid, co);
	return co;
}

//init scan condition
void my_scankey_init(const char datum_data[][20], 
										 const ScanCondition::CompareOperation co[],
										 const int fieldno[],
										 PToCompareFunc compare_func[],
										 const int col_count,
										 std::vector<ScanCondition> &vc)
{
	ScanCondition sc;
	for(int i = 0; i < col_count; ++i)
	{
		sc.compare_func = compare_func[i];
		sc.fieldno = fieldno[i];
		sc.argument = (se_uint64)datum_data[i];
		sc.arg_length = strlen(datum_data[i]);
		sc.compop = co[i];
		vc.push_back
			(
				ScanCondition
				(
				fieldno[i], 
				co[i], 
				(se_uint64)datum_data[i], 
				strlen(datum_data[i]), 
				compare_func[i]
				)
			);
	}
}

bool test_indexscan_dll_000()
{
	using namespace std;
	IndexEntrySet *pIndexEntrySet = NULL;
	EntrySet *pEntrySet = NULL;
	INTENT("两字段索引，仅向前扫描，索引列为第1列和第2列");
	try
	{	
		get_new_transaction();

		//open entryset
		pEntrySet = open_entry_set();

		char data[20][20] = {"123456", "123789", "012562", "012120", "012456"};
		DataItem di;
		EntryID eid;
		//insert data into pEntrySet
		for(int i = 0; i < 5; i++)
		{
			di.setData(&data[i]);
			di.setSize(strlen(data[i])+1);
			pEntrySet->insertEntry(pTransaction, eid, di);
		}
		command_counter_increment();
		set_col_info(2, 10);

		//create a index on testRelation
		EntrySetID indexID = pStorageEngine->createIndexEntrySet(pTransaction, 
																				pEntrySet,
																				BTREE_INDEX_ENTRY_SET_TYPE,												
																				10);
		pIndexEntrySet = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(pTransaction,
																			pEntrySet,
																			EntrySet::OPEN_EXCLUSIVE,
																			indexID));
		command_counter_increment();

		//begin index scan
		char datum_data[20][20] = {"123", "45"};
		int fieldno[2] = {1, 2};
		ScanCondition::CompareOperation co[2] = 
		{
			ScanCondition::LessThan,
			ScanCondition::GreaterThan
		};
		PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,
										co,
										fieldno,
										compare_func,
										2,
										vc);
		EntrySetScan *iess = pIndexEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));
		char *temp;
		int flag = 65536;
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			temp = (char*)di.getData();
			flag = strcmp(temp, "012562");
		}
		pIndexEntrySet->endEntrySetScan(iess);
		CHECK_BOOL(flag == 0);
		if(flag != 0)
		{			
			pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
			pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
			pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet->getId());
			commit_transaction();
			return false;
		}
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexEntrySet->getId();
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
		commit_transaction();		
	}
	catch (StorageEngineException &ex) 
	{
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexEntrySet->getId();
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
		commit_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}
	return true;
}

bool test_indexscan_dll_001()
{
	using namespace std;
	EntrySet *pEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet = NULL;
	INTENT("两字段索引，仅向前扫描，改变索引列为第2列和第3列");
	try
	{	
		get_new_transaction();

		//open entrySet and insert some data
		pEntrySet = open_entry_set();

		char data[20][20] = {"123456", "123789", "012562", "012120", "012456"};
		DataItem di;
		EntryID eid;
		//insert data into pEntrySet
		for(int i = 0; i < 5; i++)
		{
			di.setData(&data[i]);
			di.setSize(strlen(data[i])+1);
			pEntrySet->insertEntry(pTransaction, eid, di);
		}
		command_counter_increment();

		//create a index on pEntrySet
		ColumnInfo *colinfo = set_col_info(2, 11);
		colinfo->col_number[0] = 2;
		colinfo->col_number[1] = 3;
		colinfo->split_function = my_split_index_23;
		EntrySetID indexID = pStorageEngine->createIndexEntrySet(pTransaction,
																				pEntrySet,
																				BTREE_INDEX_ENTRY_SET_TYPE
																			
																				,11);
		pIndexEntrySet = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(pTransaction,
																			pEntrySet,
																			EntrySet::OPEN_EXCLUSIVE,
																			indexID));
		command_counter_increment();

		//select use scankey
		char datum_data[20][20] = {"45", "2"};
		int fieldno[2] = {1, 2};
		ScanCondition::CompareOperation co[2] = 
		{
			ScanCondition::LessThan,
			ScanCondition::LessThan
		};
		PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,
			co,
			fieldno,
			compare_func,
			2,
			vc);

		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));

		//begin index scan use scankey
		EntrySetScan *iess = pIndexEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
		char *temp;
		int flag = 65536;
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			temp = (char*)di.getData();
			flag = strcmp(temp, "012120");
		}
		pIndexEntrySet->endEntrySetScan(iess);
		CHECK_BOOL(flag == 0);
		
		//test failed if flag != 0
		if(flag != 0)
		{
			EntrySetID entid = pEntrySet->getId(); 
			EntrySetID indxid = pIndexEntrySet->getId();
			pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
			pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
			pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
			commit_transaction();
			return false;
		}
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexEntrySet->getId();
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
		commit_transaction();		
	}
	catch (StorageEngineException &ex) 
	{
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexEntrySet->getId();
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
		commit_transaction();
		return false;
	}	
	return true;
}

bool test_indexscan_dll_002()
{
	using namespace std;
	INTENT("两字段索引，仅向前扫描，改变索引列顺序为先后列再前列，如索引列为先第2列后第1列");
	EntrySet *pEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet = NULL;
	try
	{	
		get_new_transaction();

		//open heap and insert some tuple
		pEntrySet = open_entry_set();

		char data[20][20] = {"123456", "123789", "012562", "012120", "012456"};
		DataItem di;
		EntryID eid;
		//insert data into pEntrySet
		for(int i = 0; i < 5; i++)
		{
			di.setData(&data[i]);
			di.setSize(strlen(data[i])+1);
			pEntrySet->insertEntry(pTransaction, eid, di);
		}
		command_counter_increment();

		//create a index on pEntrySet
		ColumnInfo *colinfo = set_col_info(2, 12);
		colinfo->col_number[0] = 2;
		colinfo->col_number[1] = 1;
		colinfo->split_function = my_split_index_21;
		EntrySetID indexID = pStorageEngine->createIndexEntrySet(pTransaction,
			pEntrySet,
			BTREE_INDEX_ENTRY_SET_TYPE
			,
			12);
		pIndexEntrySet = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(pTransaction,
																			pEntrySet,
																			EntrySet::OPEN_EXCLUSIVE,
																			indexID,NULL));
		command_counter_increment();

		//select use scankey
		char datum_data[20][20] = {"45", "123"};
		int fieldno[2] = {1, 2};
		ScanCondition::CompareOperation co[2] = 
		{
			ScanCondition::GreaterThan,
			ScanCondition::LessThan
		};
		PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,
			co,
			fieldno,
			compare_func,
			2,
			vc);

		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));
		
		//begin scan use scankey
		EntrySetScan *ess = pIndexEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
		char *temp;
		int flag = 65536;
		while(ess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			temp = (char*)di.getData();
			flag = strcmp(temp, "012562");
		}
		pIndexEntrySet->endEntrySetScan(ess);
		CHECK_BOOL(flag == 0);
		
		if(flag != 0)
		{
			EntrySetID entid = pEntrySet->getId(); 
			EntrySetID indxid = pIndexEntrySet->getId();
			pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
			pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
			pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
			commit_transaction();
			return false;
		}
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexEntrySet->getId();
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
		commit_transaction();			
	}
	catch (StorageEngineException &ex) 
	{
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexEntrySet->getId();
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
		commit_transaction();
		return false;
	}	

	return true;
}

bool test_indexscan_dll_003()
{
	using namespace std;
	INTENT("五字段索引，仅向前扫描，索引列为2,3,5,4,1，使用5种不同策略");
	EntrySet *pEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet = NULL;
	try
	{	
		get_new_transaction();

		//open heap and insert some tuple
		pEntrySet = open_entry_set();

		char data[20][20] = {"123206jim456", "123789jim789", "012562tom562", "012120tom120",
			"255456ann456","123456ann458","255206ann568"};
		//2 3 5 4 1
		//"123 20 6 jim 45 6" 
		//"123 78 9 jim 78 9"
		//"012 56 2 tom 56 2"
		//"012 12 0 tom 12 0" 
		//"255 45 6 ann 45 6"
		//"123 45 6 ann 45 8"
		//"255 20 6 ann 56 8
		DataItem di;
		EntryID eid;
		//insert data into pEntrySet
		for(int i = 0; i < 7; i++)
		{
			di.setData(&data[i]);
			di.setSize(strlen(data[i])+1);
			pEntrySet->insertEntry(pTransaction, eid, di);
		}
		command_counter_increment();

		//create a index on pEntrySet
		const int nkeys = 5;
		ColumnInfo *colinfo = set_col_info(nkeys, 13);
		colinfo->col_number[0] = 2;
		colinfo->col_number[1] = 3;
		colinfo->col_number[2] = 5;
		colinfo->col_number[3] = 4;
		colinfo->col_number[4] = 1;
		colinfo->rd_comfunction[0] = my_compare_str;
		colinfo->rd_comfunction[1] = my_compare_str;
		colinfo->rd_comfunction[2] = my_compare_str;
		colinfo->rd_comfunction[3] = my_compare_str;
		colinfo->rd_comfunction[4] = my_compare_str;
		colinfo->split_function = my_split_index_23541;
		EntrySetID indexID = pStorageEngine->createIndexEntrySet(pTransaction,
			pEntrySet,
			BTREE_INDEX_ENTRY_SET_TYPE,
			13);
		pIndexEntrySet = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(pTransaction,
																			pEntrySet,
																			EntrySet::OPEN_EXCLUSIVE,
																			indexID
																		     ));

		command_counter_increment();

		//select use scankey
		char datum_data[20][20] = {"20", "6", "56", "ann", "255"};
		int fieldno[nkeys] = {1,2,3,4,5};
		ScanCondition::CompareOperation co[nkeys] = 
		{
			ScanCondition::GreaterThan,
			ScanCondition::LessEqual,
			ScanCondition::LessThan,
			ScanCondition::Equal,
			ScanCondition::GreaterEqual
		};
		PToCompareFunc compare_func[nkeys] = 
		{
			my_compare_str,
			my_compare_str,
			my_compare_str,
			my_compare_str,
			my_compare_str
		};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,
			co,
			fieldno,
			compare_func,
			nkeys,
			vc);

		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));
		
		//begin scan use scankey
		EntrySetScan *ess = pIndexEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
		char *temp;
		int flag = 65536;
		while(ess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			temp = (char*)di.getData();
			flag = strcmp(temp, "255456ann456"); //255206ann568
		}
		pIndexEntrySet->endEntrySetScan(ess);
		CHECK_BOOL(flag == 0);

		if(flag != 0)
		{
			EntrySetID entid = pEntrySet->getId(); 
			EntrySetID indxid = pIndexEntrySet->getId();
			pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
			pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
			pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
			commit_transaction();
			return false;
		}
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexEntrySet->getId();
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
		commit_transaction();	
	}
	catch (StorageEngineException &ex) 
	{
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		EntrySetID entid = pEntrySet->getId(); 
		EntrySetID indxid = pIndexEntrySet->getId();
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, entid, indxid);
		commit_transaction();
		return false;
	}	

	return true;
}

#define index_cols 32
bool test_indexscan_dll_004()
{
	using namespace std;
	INTENT("32字段索引，仅向前扫描，索引列为1-32，插入大量数据");
	EntrySet *pEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet = NULL;
	try
	{	
		get_new_transaction();

		//open heap and insert some tuple
		pEntrySet = open_entry_set();

		const int rows = 10000;
		const int cols = index_cols;
		char data[rows][cols+1];
		char data_row1[cols+1];
		memset(data_row1,'a',sizeof(data_row1));
		char data_row2[cols+1];
		memset(data_row2,'b',sizeof(data_row2));
		char data_row3[cols+1];
		memset(data_row3,'c',sizeof(data_row3));

		data_row1[cols] = '\0';
		data_row2[cols] = '\0';
		data_row3[cols] = '\0';

		for(int i = 0;i<rows;i++)
		{
			if((i%3)==0)
			{
				memcpy(data[i],data_row1,sizeof(data_row1));
			}
			if((i%3)==1)
			{
				memcpy(data[i],data_row2,sizeof(data_row2));
			}
			if((i%3)==2)
			{
				memcpy(data[i],data_row3,sizeof(data_row3));
			}
		}

		int time_begin,time_end;
		time_begin = clock();
		DataItem di;
		EntryID eid;
		//insert data into pEntrySet
		for(int i = 0; i < rows; i++)
		{
			SAVE_INFO_FOR_DEBUG();
			di.setData(&data[i]);
			di.setSize(strlen(data[i])+1);
			pEntrySet->insertEntry(pTransaction, eid, di);
		}
		time_end = clock();
		printf("插入%d行数据耗时%dms\n",rows,time_end - time_begin);
		command_counter_increment();

		//create a index on testRelation
		ColumnInfo *colinfo = set_col_info(index_cols, 14);
		for(int i = 0; i < index_cols; i++)
		{
			colinfo->col_number[i] = i+1;
		}
		for(int i = 0; i < index_cols; i++)
		{
			colinfo->rd_comfunction[i] = my_compare_str;
		}
		colinfo->split_function = my_split_index_100;

		EntrySetID indexID = pStorageEngine->createIndexEntrySet(pTransaction,
			pEntrySet,
			BTREE_INDEX_ENTRY_SET_TYPE,
			14);


		pIndexEntrySet = pStorageEngine->openIndexEntrySet(pTransaction,
			pEntrySet,
			EntrySet::OPEN_EXCLUSIVE,
			indexID);
		command_counter_increment();

		//select use scankey
		//form datum
		//必须与索引列顺序保持一致
		char datum_data[index_cols][20];
		for(int i = 0; i < index_cols; i++)
		{
			for(int j = 0;j<1;j++)
			{
				datum_data[i][j] = 'b';
				datum_data[i][j+1] = '\0';		
			}
		}
		ScanCondition::CompareOperation co[index_cols];
		//index_cols个比较策略全用Equal
		for(int i = 0; i < index_cols; i++)
		{
			co[i] = ScanCondition::Equal;
		}

		int fieldno[index_cols];
		for(int i = 0; i < index_cols; i++)
		{
			fieldno[i] = i+1;
		}
		//所有的比较函数都为my_compare_str
		PToCompareFunc compare_func[index_cols];
		for(int i = 0; i < index_cols; ++i)
		{
			compare_func[i] = my_compare_str;
		}

		vector<ScanCondition> vc;
		my_scankey_init(datum_data,
			co,
			fieldno,
			compare_func,
			index_cols,
			vc);

		memset(&eid, 0, sizeof(eid));
		memset(&di, 0, sizeof(di));

		//begin scan use scankey
		EntrySetScan *ess = pIndexEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
		int flag = 65536;
		char *temp;
		int counter = 0;
		time_begin = clock();
		while(ess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			++counter;
			temp = (char*)di.getData();
			flag = strcmp(temp, data_row2); //255206ann568
		}
		time_end = clock();
		printf("索引列为32字段，查询%d行数据耗时%dms\n",rows,time_end);
		CHECK_BOOL(flag==0 && counter==rows/3);
		pIndexEntrySet->endEntrySetScan(ess);
		
		if(flag!=0 || counter!=rows/3)
		{
			pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
			pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
			pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet->getId());
			commit_transaction();
			return false;
		}
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet->getId());
		commit_transaction();		
	}
	catch (StorageEngineException &ex) 
	{
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet->getId());
		commit_transaction();
		return false;
	}	

	return true;
}

extern 
void insert_data(char data[][200], 
								 const int data_len,
								 EntrySet **pEntrySet,
								 Transaction *transaction = pTransaction);

bool test_indexscan_mark_restore()
{
	INTENT("测试索引扫描的mark/restore position功能是否正确。");

	using namespace std;
	IndexEntrySet *pIndexEntrySet = NULL;
	EntrySet *pEntrySet = NULL;
	bool return_sta = true;
	try
	{	
		get_new_transaction();

		pEntrySet = open_entry_set();

		const uint32 INSERT_ROW = 100;
		DataGenerater dg(INSERT_ROW, DATA_LEN);
		dg.dataGenerate();
		char data[INSERT_ROW][DATA_LEN];
		dg.dataToDataArray2D(data);
		insert_data(data, ARRAY_LEN_CALC(data), &pEntrySet);
		pStorageEngine->endStatement();
		set_col_info(2, 10);

		//create a index on testRelation
		EntrySetID esid = pStorageEngine->createIndexEntrySet(pTransaction, 
			pEntrySet,
			BTREE_INDEX_ENTRY_SET_TYPE,
			10,
			0,
			NULL);
		pIndexEntrySet = static_cast<IndexEntrySet *>(pStorageEngine->openIndexEntrySet(pTransaction,
			pEntrySet,
			EntrySet::OPEN_EXCLUSIVE,
			esid,
			NULL));
		pStorageEngine->endStatement();

		//begin index scan
		char datum_data[20][20] = {"AAA", "AA"};
		int fieldno[2] = {1, 2};
		ScanCondition::CompareOperation co[2] = 
		{
			ScanCondition::GreaterThan,
			ScanCondition::GreaterThan
		};
		PToCompareFunc compare_func[2] = {my_compare_str, my_compare_str};
		vector<ScanCondition> vc;
		my_scankey_init(datum_data,
			co,
			fieldno,
			compare_func,
			2,
			vc);
		EntrySetScan *iess = pIndexEntrySet->startEntrySetScan(pTransaction, EntrySet::SnapshotNOW, vc);
		EntryID eid;
		DataItem di;
		char *temp;
		vector<string> v_cmp1, v_cmp2;
		iess->markPosition();
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			v_cmp1.push_back((char*)di.getData());
		}
		/* 读取标记 */
		iess->restorePosition();
		while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
		{
			v_cmp2.push_back((char*)di.getData());
		}
		pIndexEntrySet->endEntrySetScan(iess);
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet->getId());
		commit_transaction();		
		sort(v_cmp1.begin(), v_cmp1.end());
		sort(v_cmp2.begin(), v_cmp2.end());

		return_sta = (v_cmp1 == v_cmp2) ? true : false;
	}
	catch (StorageEngineException &ex) 
	{
		pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
		pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);
		pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet->getId());
		commit_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return_sta = false;
	}
	return return_sta;
}