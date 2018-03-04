#include <iostream>
#include <algorithm>

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "PGSETypes.h"
#include "utils/utils_dll.h"
#include "test_fram.h"

using namespace FounderXDB::EXCEPTION;
using namespace std;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;

extern int my_compare_str_index(const char *str1, size_t len1, const char *str2, size_t len2);

bool test_cluster_entryset_001()
{
	INTENT("测试cluster功能是否正常工作。");

	bool return_sta = true;
	const int DATA_ROWS = 1024;
	const int DATA_LENS = 8192;
	const int DATA_INSERT_NUM = 100;
	Transaction *trans = NULL;
	EntrySetID relid, idxid;
	SpliterGenerater sg;
	int relcolid = get_col_id();
	int idxcolid = get_col_id();

	/* 构造表和索引的colinfo */
	ColumnInfo *relinfo = sg.buildHeapColInfo(1, 10);
	int colnumber[] = {1};
	CompareCallbacki cmpfunc[] = {my_compare_str_index};
	ColumnInfo *idxinfo = sg.buildIndexColInfo(1, colnumber, cmpfunc, SpliterGenerater::index_split_to_any<1>);
	setColumnInfo(relcolid, relinfo);
	setColumnInfo(idxcolid, idxinfo);

	try
	{
		/* 创建表和索引 */
		get_new_transaction();
		relid = pStorageEngine->createEntrySet(pTransaction, relcolid);
		EntrySet *entryset = pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, relid);
		idxid = pStorageEngine->createIndexEntrySet(pTransaction, entryset, BTREE_INDEX_ENTRY_SET_TYPE, idxcolid);
		pStorageEngine->closeEntrySet(pTransaction, entryset);
		commit_transaction();

		/* 构建测试数据 */
		DataGenerater dg(DATA_ROWS + DATA_INSERT_NUM, DATA_LENS);
		dg.dataGenerate();

		vector<string> v_data;
		vector<EntryID> v_Ids;
		for (int i = 0; i < DATA_ROWS; ++i)
		{
			v_data.push_back(dg[i]);
		}

		get_new_transaction();

		entryset = pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, relid);
		InsertData(pTransaction, entryset, v_data, &v_Ids);
		pStorageEngine->endStatement();

		/* 删除前一半的数据 */
		vector<EntryID> v_deleteIds;
		vector<EntryID>::iterator begin = v_Ids.begin();
		int pos = (v_Ids.size() / 2);
		vector<EntryID>::iterator end = (begin + pos);
		v_deleteIds.assign(begin, end);
		DeleteData(pTransaction, entryset, v_deleteIds);
		commit_transaction();

		/* cluster */
		pStorageEngine->cluster(relid, idxid);
		
		get_new_transaction();
		entryset = pStorageEngine->openEntrySet(pTransaction, EntrySet::OPEN_SHARED, relid);
		/* 再次插入若干数据 */
		{
			vector<string> v_data2;
			for (int i = 0; i < DATA_INSERT_NUM; ++i)
			{
				v_data2.push_back(dg[i + DATA_ROWS]);
			}
			InsertData(pTransaction, entryset, v_data2);
		}

		/* 计算表中存在的数据 */
		{
			vector<string>::iterator begin = v_data.begin();
			pos = v_data.size() / 2;
			begin = begin + pos;
			vector<string> v_data2;
			v_data2.assign(begin, v_data.end());
			v_data = v_data2;
			for (int i = 0; i < DATA_INSERT_NUM; ++i)
			{
				v_data.push_back(dg[i + DATA_ROWS]);
			}
		}

		/* 检测结果 */
		pos = v_data.size() / 2;
		sort(v_data.begin(), v_data.end());
		string key = v_data[pos];
		key = key.substr(0, 10);
		vector<string> v_key;
		v_key.push_back(key);
		return_sta = check_test_result(&v_data, relid, idxid, v_key);

		pStorageEngine->closeEntrySet(pTransaction, entryset);

		commit_transaction();

		get_new_transaction();
		pStorageEngine->removeEntrySet(pTransaction, relid);
		commit_transaction();
	} catch (XdbBaseException &se)
	{
		cout << se.what() << endl;
		user_abort_transaction();
		return_sta = false;
	}

	return return_sta;
}
