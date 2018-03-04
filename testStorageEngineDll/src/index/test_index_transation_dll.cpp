#include "iostream"
#include "vector"
#include "map"
#include <string>

#include "utils/utils_dll.h"
#include "index/test_index_dll.h"
#include "PGSETypes.h"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"

using namespace FounderXDB::StorageEngineNS;

typedef int (*PToCompareFunc)(const char*, size_t, const char*, size_t);
extern EntrySet *open_entry_set();
extern void my_scankey_init(char scankey_data[][20], 
														const ScanCondition::CompareOperation co[],
														const int fieldno[],
														PToCompareFunc compare_func[],
														const int col_count,
														std::vector<ScanCondition> &vc_scancondition);


extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
extern EntrySetID EID;

const int entry_colid = 100;
int current_index_entry_colid = 101;
ColumnInfo *current_entryset_colinfo = NULL;
ColumnInfo *current_index_entryset_colinfo = NULL;

//�������ݳ��ȣ�����"\0"��
unsigned int data_len_calc(const char data[], unsigned int len)
{
	--len;
	while(len != 0 && data[len--] == '\0');
	return len != 0 ? len += 3 : 1;
}

//init scan condition
void new_scankey_init(const char datum_data[][20], 
										 const ScanCondition::CompareOperation co[],
										 const int fieldno[],
										 PToCompareFunc const compare_func[],
										 const int col_count,
										 std::vector<ScanCondition> &vc)
{
	ScanCondition sc;
	for(int i = 0; i < col_count; ++i)
	{
		sc.compare_func = compare_func[i];
		sc.fieldno = fieldno[i];
		sc.argument = (se_uint64)datum_data[i];
		sc.arg_length = data_len_calc(datum_data[i], 20)-1;
		sc.compop = co[i];
		vc.push_back(sc);
	}
}

void close_entryset(EntrySet **pEntrySet, Transaction *transation = pTransaction)
{
	pStorageEngine->closeEntrySet(transation, (*pEntrySet));
}

void close_entryset(IndexEntrySet **pEntrySet, Transaction *transation = pTransaction)
{
	pStorageEngine->closeEntrySet(transation, (*pEntrySet));
}

void close_all(std::map<EntrySetID, BaseEntrySet**> &vc_entryset,
							 std::map<EntrySetID, std::pair<EntrySetID, BaseEntrySet**> > &vc_index_entryset)
{
	std::map<EntrySetID, BaseEntrySet**>::iterator it_entryset = vc_entryset.begin();
	while(it_entryset != vc_entryset.end())
	{
		close_entryset((EntrySet**)it_entryset->second);	
		++it_entryset;
	}
	std::map<EntrySetID, 
					 std::pair<EntrySetID, BaseEntrySet**> >::iterator it_index_entryset = 
	vc_index_entryset.begin();
	while(it_index_entryset != vc_index_entryset.end())
	{
		close_entryset((IndexEntrySet**)(it_index_entryset->second).second);	
		++it_index_entryset;
	}
}

void remove_index_entryset(std::map<EntrySetID, std::pair<EntrySetID, BaseEntrySet**> > &vc_index_entryset)
{
	std::map<EntrySetID, 
		std::pair<EntrySetID, BaseEntrySet**> >::iterator it_index_entryset = 
		vc_index_entryset.begin();
	while(it_index_entryset != vc_index_entryset.end())
	{
		pStorageEngine->removeIndexEntrySet(pTransaction, 
																				it_index_entryset->second.first,
																				it_index_entryset->first);	
		++it_index_entryset;
	}
}

#define WHEN_EXCEPTION_CATCH() \
	close_all(vc_entryset, vc_index_entryset); \
	commit_transaction(); \
	cout << ex.getErrorNo() << endl; \
	cout << ex.getErrorMsg() << endl; \
	return false

#define WHEN_TEST_END() \
	close_all(vc_entryset, vc_index_entryset);

/*
* ����һ��entryset��map��
* index_entryset��map
* ���ڷ��������Դ���ڲ���
* ������ʱ�������ͷź�ɾ��
*/
#define BEGIN_TEST() \
	map<EntrySetID, BaseEntrySet**> vc_entryset; \
	map<EntrySetID, pair<EntrySetID, BaseEntrySet**> > vc_index_entryset;

/*
* pushһ��index_entryset
*/
#define INDEX_ENTRY_SET_PUSH(b_entryset, p_id) \
	vc_index_entryset[(*(b_entryset))->getId()] = \
		pair<EntrySetID, BaseEntrySet**>(p_id, (BaseEntrySet**)b_entryset);

/*
* pushһ��entryset
*/
#define ENTRY_SET_PUSH(b_entryset) \
	vc_entryset[(*(b_entryset))->getId()] = (BaseEntrySet**)b_entryset;

/*
*��ȡentryset�ĸ����г�����Ϣ������col_len_array�ĳ���
*/
int get_col_len_array(int const **len_array)
{
	static const int col_len_array[] = {1, 3, 3, 2, 1, 4};
	*len_array = col_len_array;
	return sizeof(col_len_array) / sizeof(col_len_array[0]);
}

/*
* entryset_spilt_to_any:
* ��entryset�ָ�Ϊ�κ���������
* ���ι�����еĳ��ȷֱ�Ϊ: 1 3 3 2 1 4
*/
void entryset_spilt_to_any(RangeDatai& rd, const char* str, int col, size_t len =0)
{
	while(NULL != str)
	{
		int const *len_array;
		int len = get_col_len_array(&len_array);
		for(int i = 1; i <= len; ++i)
		{
			if(col == i)
			{
				int sum = 0;
				for(int j = 0; j < i - 1; ++j)
				{
					sum += len_array[j];
				}
				rd.start = sum;
				rd.len = len_array[i - 1];
				break;
			}
		}
		break;
	}
}

void create_entryset()
{
	using namespace std;
	try
	{
		get_first_transaction();
		ColumnInfo *colinfo = (ColumnInfo*)malloc(sizeof(ColumnInfo));
		colinfo->keys = 0;
		colinfo->col_number = NULL;
		colinfo->rd_comfunction = NULL;
		colinfo->split_function = entryset_spilt_to_any;
		setColumnInfo(entry_colid, colinfo);
		current_entryset_colinfo = colinfo;
		EID = pStorageEngine->createEntrySet(pTransaction, 
			entry_colid,
			0);
		commit_transaction();
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorMsg() << endl;
		cout << ex.getErrorNo() << endl;
	}
}

void remove_entryset()
{
	using namespace std;
	try
	{
		get_new_transaction();
		pStorageEngine->removeEntrySet(pTransaction, EID);
		commit_transaction();
	}
	catch(StorageEngineException &ex)
	{
		user_abort_transaction();
		cout << ex.getErrorMsg() << endl;
		cout << ex.getErrorNo() << endl;
	}
}

IndexEntrySet* create_index_entryset(const std::string name, 
																		const int col_id, 
																		EntrySet **pEntrySet,
																		Transaction *transaction = pTransaction)
{
	EntrySetID ieid = pStorageEngine->createIndexEntrySet(transaction,
		*pEntrySet,
		BTREE_INDEX_ENTRY_SET_TYPE,
		col_id,
		0,
		NULL);
	IndexEntrySet *pIEntrySet = pStorageEngine->openIndexEntrySet(transaction, *pEntrySet, EntrySet::OPEN_EXCLUSIVE, ieid, NULL);
	//pStorageEngine->closeEntrySet(transaction, pIEntrySet);
	return pIEntrySet;
}

BaseEntrySet *open_index_entryset(EntrySet **pEntrySet,
																	 EntrySetID name,
																	 Transaction *transation = pTransaction,
																	 EntrySet::EntrySetOpenFlags op = EntrySet::OPEN_EXCLUSIVE,
																	 void *keybuild = NULL,
																	 void *compare = NULL)
{
	return pStorageEngine->openIndexEntrySet(transation, *pEntrySet, op, 
																					 name, NULL);
}

void insert_data(char data[][200], 
								 const int data_len,
								 EntrySet **pEntrySet,
								 Transaction *transaction = pTransaction)
{
	EntryID eid;
	DataItem di;
	for(int i = 0; i < data_len; ++i)
	{
		di.setData(&data[i]);
		di.setSize(data_len_calc(data[i], 200));
		(*pEntrySet)->insertEntry(transaction, eid, di);
	}
}

ColumnInfo *alloc_fill_index_colinfo(const int nkeys, 
													const Spliti spi_func,
													const unsigned int col_num[],
													const int col_num_len,
													const CompareCallbacki ccb[],
													const int ccb_len)
{
	ColumnInfo *ci = (ColumnInfo*)malloc(sizeof(ColumnInfo));
	ci->keys = nkeys;
	ci->split_function = spi_func;
	ci->col_number = (size_t*)malloc(sizeof(size_t) * nkeys);
	for(int i = 0; i < col_num_len; ++i)
	{
		ci->col_number[i] = col_num[i];
	}
	ci->rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki) * nkeys);
	for(int i = 0; i < ccb_len; ++i)
	{
		ci->rd_comfunction[i] = ccb[i];
	}
	return ci;
}

int set_col_info(ColumnInfo **ci)
{
	setColumnInfo(current_index_entry_colid, *ci);
	return current_index_entry_colid++;
}

int compare_str(const char *str1, size_t len1, const char *str2, size_t len2)
{
	if(len1 > len2 || len2 > len1)
	{
		return len1 > len2 ? 1 : -1;
	}
	for(int i = 0 ; ; ++i)
	{
		if(i == len1)
		{
			break;
		}
		if(str1[i] > str2[i] || str2[i] > str1[i])
		{
			return str1[i] > str2[i] ? 1 : -1;
		}
	}
	return 0;
}

bool all_data_equal(IndexEntrySet **pIndexEntrySet,
										std::vector<ScanCondition> &vc_scancondition,
										char data[][200],
										const int data_len,
										Transaction *transation = pTransaction,
										const EntrySet::SnapshotTypes opt = BaseEntrySet::SnapshotMVCC,
										const EntrySetScan::CursorMovementFlags scanTo = EntrySetScan::NEXT_FLAG)
{
	EntrySetScan *ess = (*pIndexEntrySet)->startEntrySetScan(transation,opt,vc_scancondition);
	DataItem di;
	EntryID eid;
	char *tmp;
	while(ess->getNext(scanTo, eid, di) == 0)
	{
		tmp = (char*)di.getData();
		for(int i = 0; i < data_len; ++i)
		{
			if(memcmp(data[i], tmp, strlen(tmp)) == 0)
			{
				memset(&data[i][0], 0, 200);
				break;
			}
		}
	}
	(*pIndexEntrySet)->endEntrySetScan(ess);
	for(int i = 0; i < data_len; ++i)
	{
		if(strlen(data[i]) != 0)
		{
			return false;
		}
	}
	return true;
}

#define LEN_CALC(data, div) sizeof(data)/sizeof(div)

extern EntrySetScan *heap_begin_scan(EntrySet *pEntrySet);

int test_index_transaction_001()
{
	using namespace std;
	BEGIN_TEST();
	INTENT("������һ���������Ȳ�����������Ȼ��������"
				 "����һ���µ�����������ɨ�裬�鿴�����Ƿ�ͬ"
				 "�����¡�");
	EntrySet *pEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet = NULL;
	try
	{	
		//��������
		char data[5][200] = 
		{
			"abcdefghijklmn",
			"abcdesssjklmn",
			"abcdeaaajklmn",
			"abdddfghijklmn",
			"abcdefghfgggmn"
		};

		get_new_transaction();
		pEntrySet = open_entry_set();
		ENTRY_SET_PUSH(&pEntrySet);
		insert_data(data, LEN_CALC(data, data[0]), &pEntrySet);
		command_counter_increment();

		//�ڵ�һ�к͵������Ͻ�����
		unsigned int keys[] = {1, 3};
		CompareCallbacki ccb[] = 
		{
			compare_str,
			compare_str
		};
		ColumnInfo *ci = alloc_fill_index_colinfo(2, 
												entryset_spilt_to_any, 
												keys, 
												LEN_CALC(keys, keys[0]), 
												ccb,
												LEN_CALC(ccb, ccb[0]));
		int col_id = set_col_info(&ci);
		pIndexEntrySet = create_index_entryset("my_index1",col_id,&pEntrySet);
		EntrySetID idxid = pIndexEntrySet->getId();																			 		 
		INDEX_ENTRY_SET_PUSH(&pIndexEntrySet, pEntrySet->getId());
		close_entryset(&pEntrySet);
		commit_transaction();
		/*
		* �ر�entryset���ύ����
		* ����һ���µ����񣬴�֮ǰ�������д���������
		* ������scanconditionɨ��entryset��
		*/
		get_new_transaction();
		pEntrySet = open_entry_set();

		pIndexEntrySet = static_cast<IndexEntrySet *>(open_index_entryset(&pEntrySet, idxid));
		//ɨ�����е�����
		char scankey_data[][20] = 
		{
			"a",
			"aaa"
		};
		ScanCondition::CompareOperation co[] = 
		{
			ScanCondition::GreaterEqual,
			ScanCondition::GreaterEqual
		};
		int fieldno[] = {1, 2};
		PToCompareFunc compare_func[] = 
		{
			compare_str,
			compare_str
		};
		std::vector<ScanCondition> vc_scancondition;
		new_scankey_init(scankey_data, co, fieldno, compare_func, 2, vc_scancondition);
		//scankeyɨ�����е����ݣ��鿴�����Ƿ�ͬ������
		bool is_equal = all_data_equal(&pIndexEntrySet, vc_scancondition, data, LEN_CALC(data, data[0]));
		WHEN_TEST_END();
		commit_transaction();
		//ɨ�赽���е����ݣ����Գɹ�
		if(is_equal)
		{		
			return true;
		}else
		{
			return false;
		}
	}
	catch (StorageEngineException &ex) 
	{
		WHEN_EXCEPTION_CATCH();
	}
}


int test_index_transaction_002()
{
	using namespace std;
	BEGIN_TEST();
	INTENT("������һ�������н�������Ȼ���ύ����"
				 "����һ���µ����񣬲����������ݣ�������"
				 "����ɨ�裬�鿴�����Ƿ�ͬ�����¡�");
	EntrySet *pEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet = NULL;
	try
	{	
		//��������
		char data[5][200] = 
		{
			"abcdefghijklmn",
			"abcdesssjklmn",
			"abcdeaaajklmn",
			"abdddfghijklmn",
			"abcdefghfgggmn"
		};

		get_new_transaction();
		pEntrySet = open_entry_set();
		ENTRY_SET_PUSH(&pEntrySet);

		//�ڵ�һ�к͵ڶ����Ͻ�����
		unsigned int keys[] = {1, 2};
		CompareCallbacki ccb[] = 
		{
			compare_str,
			compare_str
		};
		ColumnInfo *ci = alloc_fill_index_colinfo(2, 
			entryset_spilt_to_any, 
			keys, 
			LEN_CALC(keys, keys[0]), 
			ccb,
			LEN_CALC(ccb, ccb[0]));
		int col_id = set_col_info(&ci);
		pIndexEntrySet = create_index_entryset("my_index2",
			col_id,
			&pEntrySet);
	    EntrySetID idxid = pIndexEntrySet->getId();	
	    
		INDEX_ENTRY_SET_PUSH(&pIndexEntrySet, pEntrySet->getId());
		close_entryset(&pEntrySet);
		commit_transaction();
		/*
		* �ر�entryset���ύ����
		* ����һ���µ����񣬲����������ݡ�
		* ����scanconditionɨ��entryset��
		*/
		get_new_transaction();
		pEntrySet = open_entry_set();
		//�����������
		insert_data(data, LEN_CALC(data, data[0]), &pEntrySet);
		command_counter_increment();
		pIndexEntrySet = static_cast<IndexEntrySet *>(open_index_entryset(&pEntrySet, idxid));
		//open_index_entryset(&pEntrySet, pIndexEntrySet->getId());
		//ɨ�����е�����
		char scankey_data[][20] = 
		{
			"a",
			"aaa"
		};
		ScanCondition::CompareOperation co[] = 
		{
			ScanCondition::GreaterEqual,
			ScanCondition::GreaterEqual
		};
		int fieldno[] = {1, 2};
		PToCompareFunc compare_func[] = 
		{
			compare_str,
			compare_str
		};
		std::vector<ScanCondition> vc_scancondition;
		new_scankey_init(scankey_data, co, fieldno, compare_func, 2, vc_scancondition);
		//scankeyɨ�����е����ݣ��鿴�����Ƿ�ͬ������
		bool is_equal = all_data_equal(&pIndexEntrySet, vc_scancondition, data, LEN_CALC(data, data[0]));
		WHEN_TEST_END();
		commit_transaction();
		//ɨ�赽���е����ݣ����Գɹ�
		if(is_equal)
		{		
			return true;
		}else
		{
			return false;
		}
	}
	catch (StorageEngineException &ex) 
	{
		WHEN_EXCEPTION_CATCH();
	}
}

EntryID find_single_data_use_seq_scan(const char *dest_data,
																			 EntrySet *pEntrySet, 
																			 std::vector<ScanCondition> &vc_scancondition,
																			 bool &found,
																			 EntrySetScan::CursorMovementFlags scanTo = EntrySetScan::NEXT_FLAG,
																			 Transaction *transcation = pTransaction,
																			 EntrySet::SnapshotTypes opt = EntrySet::SnapshotMVCC)
{
	found = false;
	EntrySetScan *ess = pEntrySet->startEntrySetScan(transcation,opt,vc_scancondition);
	EntryID eid, return_eid;
	memset(&eid, 0, sizeof(eid));
	memset(&return_eid, 0, sizeof(return_eid));
	DataItem di;
	while(ess->getNext(scanTo, eid, di) == 0)
	{
		if(memcmp(dest_data, di.getData(), di.getSize()) == 0)
		{
			return_eid = eid;
			found = true;
		}
	}
	pEntrySet->endEntrySetScan(ess);
	return return_eid;
}

bool is_found_single_data_use_index(IndexEntrySet **pIndexEntrySet,
																std::vector<ScanCondition> &vc_scancondition,
																const char *dest_data,
																Transaction *transaction = pTransaction,
																EntrySetScan::CursorMovementFlags opt = EntrySetScan::NEXT_FLAG)
{
	EntrySetScan *ess = (*pIndexEntrySet)->startEntrySetScan(transaction,BaseEntrySet::SnapshotMVCC,vc_scancondition);
	int found_count = -1;
	DataItem di;
	EntryID eid;
	while(ess->getNext(opt, eid, di) == 0)
	{
		if(memcmp(dest_data, di.getData(), di.getSize()) == 0)
		{
			found_count = 1;
		}
	}
	(*pIndexEntrySet)->endEntrySetScan(ess);
	if(1 == found_count)
	{
		return true;
	}else
	{
		return false;
	}
}

int test_index_transaction_003()
{
	using namespace std;
	BEGIN_TEST();
	INTENT("������һ�������н������������������ݲ��ύ����"
				 "�����µ�����ɾ���������ݲ��ύ����"
				 "�����µ���������scanconditionɨ�衣");
	EntrySet *pEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet = NULL;
	try
	{	
		//��������
		char data[5][200] = 
		{
			"abcdefghijklmn",
			"abcdesssjklmn",
			"abcdeaaajklmn",
			"abdddfghijklmn",
			"abcdefghfgggmn"
		};

		get_new_transaction();
		pEntrySet = open_entry_set();
		ENTRY_SET_PUSH(&pEntrySet);
		insert_data(data, LEN_CALC(data, data[0]), &pEntrySet);
		command_counter_increment();

		//�ڵڶ��к͵������Ͻ�����
		unsigned int keys[] = {2, 5};
		CompareCallbacki ccb[] = 
		{
			compare_str,
			compare_str
		};
		ColumnInfo *ci = alloc_fill_index_colinfo(2, 
			entryset_spilt_to_any, 
			keys, 
			LEN_CALC(keys, keys[0]), 
			ccb,
			LEN_CALC(ccb, ccb[0]));
		int col_id = set_col_info(&ci);
		pIndexEntrySet = create_index_entryset("my_index3",
			col_id,
			&pEntrySet);
		EntrySetID idxid = pIndexEntrySet->getId();
		INDEX_ENTRY_SET_PUSH(&pIndexEntrySet, pEntrySet->getId());
		close_entryset(&pEntrySet);
		commit_transaction();
		/*
		* �ر�entryset���ύ����
		* ����һ���µ�����ɾ���������ݡ�
		*/
		get_new_transaction();
		pEntrySet = open_entry_set();
		//�������� abdddfghijklmn ��ɾ��
		std::vector<ScanCondition> vc_scancondition;
		bool found;
		EntryID delete_eid = find_single_data_use_seq_scan("abdddfghijklmn", 
																											 pEntrySet,
																											 vc_scancondition,
																											 found);
		if(found)
		{
			pEntrySet->deleteEntry(pTransaction, delete_eid);
		}else//û���ҵ� abdddfghijklmn ����ʧ��
		{
			WHEN_TEST_END();
			commit_transaction();
			return false;
		}
		close_entryset(&pEntrySet);
		commit_transaction();
		/*
		* ����һ���µ�����������������ɾ���˵����ݡ�
		*/
		get_new_transaction();
		pEntrySet = open_entry_set();
		pIndexEntrySet = static_cast<IndexEntrySet *>(open_index_entryset(&pEntrySet, idxid));
		//open_index_entryset(&pEntrySet, pIndexEntrySet->getId());
		/*
		* ������������ɨ�� a bdd dfg hi j klmn
		* �ڵڶ��͵�������ʹ��scankey
		*/
		char scankey_data[][20] = 
		{
			"bdd",
			"j"
		};
		ScanCondition::CompareOperation co[] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		int fieldno[] = {1, 2};
		PToCompareFunc compare_func[] = 
		{
			compare_str,
			compare_str
		};
		new_scankey_init(scankey_data, co, fieldno, compare_func, 2, vc_scancondition);
		bool is_found = is_found_single_data_use_index(&pIndexEntrySet, vc_scancondition, "abdddfghijklmn");
		WHEN_TEST_END();
		commit_transaction();
		if(is_found/*�ҵ������ʧ��*/)
		{
			return false;
		}else
		{
			return true;
		}
	}
	catch (StorageEngineException &ex) 
	{
		WHEN_EXCEPTION_CATCH();
	}
}

int test_index_transaction_004()
{
	using namespace std;
	BEGIN_TEST();
	INTENT("��һ��entryset�Ͻ�һ��������Ȼ������������ݲ��ύ����"
				 "�����µ����񲢽��ڶ����������ٲ����������ݣ����ύ����"
				 "�������������ֱ�ɨ�衣");
	EntrySet *pEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet2 = NULL;
	try
	{	
		get_new_transaction();
		pEntrySet = open_entry_set();
		ENTRY_SET_PUSH(&pEntrySet);


		/*
		* ������һ�����������������ڵڶ��к�
		* ��������
		*/
		unsigned int idnex1_col_num[] ={2, 6};
		CompareCallbacki index1_cco[] = 
		{
			compare_str,
			compare_str
		};
		ColumnInfo *index1_ci = 
			alloc_fill_index_colinfo
			(
				2,
				entryset_spilt_to_any,
				idnex1_col_num,
				LEN_CALC(idnex1_col_num, idnex1_col_num[0]),
				index1_cco,
				LEN_CALC(index1_cco, index1_cco[0])
			);
		int col_id = set_col_info(&index1_ci);
		pIndexEntrySet = create_index_entryset
			(
				"my_index41",
				col_id,
				&pEntrySet
			);
		EntrySetID idxid = pIndexEntrySet->getId();
		INDEX_ENTRY_SET_PUSH(&pIndexEntrySet, pEntrySet->getId());
		command_counter_increment();
		

		//��������1�Ĳ�������
		char index1_data[][200] = 
		{
			"testdata1",
			"testdata5",
			"dddddddddddddddddddddddddd",
			"pppppppdwwwwwwww",
			"ddlsmmmmswosk",
			"aaaa\0aaaaaaaaa",
			"ddddoemsma\0,smdfe",
			"dwoodmekkssma,s"
		};
		insert_data(index1_data, LEN_CALC(index1_data, index1_data[0]), &pEntrySet);
		close_entryset(&pEntrySet);
		commit_transaction();


		/*
		* �����µ����񲢽��ڶ�������
		* �ڶ����������ڵ����к͵�������
		* �ٲ����������ݣ����ύ����
		*/
		get_new_transaction();
		pEntrySet = open_entry_set();
		unsigned int index2_col_num[] ={4, 6};
		CompareCallbacki index2_cco[] = 
		{
			compare_str,
			compare_str
		};
		ColumnInfo *index2_ci = 
			alloc_fill_index_colinfo
			(
			2,
			entryset_spilt_to_any,
			index2_col_num,
			LEN_CALC(index2_col_num, index2_col_num[0]),
			index2_cco,
			LEN_CALC(index2_cco, index2_cco[0])
			);
		col_id = set_col_info(&index2_ci);
		pIndexEntrySet2 = create_index_entryset
			(
				"my_index42",
				col_id,
				&pEntrySet
			);
		EntrySetID idxid2 = pIndexEntrySet2->getId();
		INDEX_ENTRY_SET_PUSH(&pIndexEntrySet2, pEntrySet->getId());
		command_counter_increment();

		
		//��������2�Ĳ�������
		char index2_data[][200] = 
		{
			"testdata2",
			"mmma",
			"\0",
			"testdata10",
			"&&&&&&&\0\0!\0,sm",
			"-----+++++____|||---",
			"iphone 5 = 5 ge pingguo"
		};
		insert_data(index2_data, LEN_CALC(index2_data, index2_data[0]), &pEntrySet);
		close_entryset(&pEntrySet);
		commit_transaction();


		/*
		* �����µ�����ʹ�ô��������������ֱ�ɨ��
		*/
		get_new_transaction();
		pEntrySet = open_entry_set();
		pIndexEntrySet = static_cast<IndexEntrySet *>(open_index_entryset(&pEntrySet, idxid));
		pIndexEntrySet2 = static_cast<IndexEntrySet *>(open_index_entryset(&pEntrySet, idxid2));

		
		/*
		* ��������1��scankey
		* ������Ҫ���� d ddd oem sm a \0,sm dfe
		*/
		vector<ScanCondition> vc_index1_scancondition;
		char index1_scankey[][20] = {"ddd", "\0,sm"};
		ScanCondition::CompareOperation index1_co[] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		int index1_fieldno[] = {1, 2};
		PToCompareFunc index1_cmp_func[] =
		{
			compare_str,
			compare_str
		};
		new_scankey_init
			(
				index1_scankey,
				index1_co,
				index1_fieldno,
				index1_cmp_func,
				2,
				vc_index1_scancondition
			);
		bool is_found = is_found_single_data_use_index
			(
				&pIndexEntrySet,
				vc_index1_scancondition,
				"ddddoemsma\0,smdfe"
			);
		if(!is_found)
		{
			WHEN_TEST_END();
			commit_transaction();
			return false;
		}
		

		/*
		* ��������2��scankey
		* ������Ҫ���� & &&& &&& \0\0 ! \0,sm
		*/
		vector<ScanCondition> vc_index2_scancondition;
		char index2_scankey[][20] = {"\0\0", "\0,sm"};
		ScanCondition::CompareOperation index2_co[] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		int index2_fieldno[] = {1, 2};
		PToCompareFunc index2_cmp_func[] =
		{
			compare_str,
			compare_str
		};
		new_scankey_init
			(
			index2_scankey,
			index2_co,
			index2_fieldno,
			index2_cmp_func,
			2,
			vc_index2_scancondition
			);
		is_found = is_found_single_data_use_index
			(
			&pIndexEntrySet2,
			vc_index2_scancondition,
			"&&&&&&&\0\0!\0,sm"
			);
		if(!is_found)
		{
			WHEN_TEST_END();
			commit_transaction();
			return false;
		}
	}
	catch (StorageEngineException &ex) 
	{
		WHEN_EXCEPTION_CATCH();
	}
	WHEN_TEST_END();
	commit_transaction();
	return true;
}

int test_index_transaction_005()
{
	using namespace std;
	BEGIN_TEST();
	INTENT("���Բ���հ��ַ�\0������heap��˳�����"
				 "���������ҡ��հ��ַ�Ӧ��Ҳ���ҵ���");
	EntrySet *pEntrySet = NULL;
	IndexEntrySet *pIndexEntrySet = NULL;
	try
	{	
		get_new_transaction();
		pEntrySet = open_entry_set();
		ENTRY_SET_PUSH(&pEntrySet);


		/*
		* ������������
		*/
		const char *dest_data = "++++\0\0\0\0\0__||";
		char data[][200] = 
		{
			"can not found data",
			"",
			"di tie 10 hao xian",
			"dong che",
			"++++\0\0\0\0\0__||", //����Ҫ�ҵ�����
			"dkei&&*^*&^asldf",
			"adfklendke"
		};
		insert_data(data, LEN_CALC(data, data[0]), &pEntrySet);
		command_counter_increment();


		/*
		* ���������ڵ����к͵������Ͻ�����(1 3 3 2 1 4)
		*/
		unsigned int index_col_num[] ={3, 4};
		CompareCallbacki index_cco[] = 
		{
			compare_str,
			compare_str
		};
		ColumnInfo *index_ci = 
			alloc_fill_index_colinfo
			(
			2,
			entryset_spilt_to_any,
			index_col_num,
			LEN_CALC(index_col_num, index_col_num[0]),
			index_cco,
			LEN_CALC(index_cco, index_cco[0])
			);
		int col_id = set_col_info(&index_ci);
		pIndexEntrySet = create_index_entryset("my_index5", col_id, &pEntrySet);
		EntrySetID idxid = pIndexEntrySet->getId();
		INDEX_ENTRY_SET_PUSH(&pIndexEntrySet, pEntrySet->getId());
		close_entryset(&pEntrySet);
		commit_transaction();

		/*
		* ��������������˳��ɨ�������ɨ�����Ŀ������
		*/

		get_new_transaction();
		pEntrySet = open_entry_set();
		open_index_entryset(&pEntrySet, idxid);
		vector<ScanCondition> vc_scancondition;
		bool is_found = false;
		find_single_data_use_seq_scan(dest_data, pEntrySet, vc_scancondition, is_found);
		if(!is_found/*û���ҵ�dest_data,����ʧ��*/)
		{
			WHEN_TEST_END();
			commit_transaction();
			return false;
		}
		char index_scankey_data[][20] = {"\0\0\0", "\0\0"};
		ScanCondition::CompareOperation index_co[] = 
		{
			ScanCondition::Equal,
			ScanCondition::Equal
		};
		int index_fieldno[] = {1, 2};
		PToCompareFunc index_filedno_cmp_func[] = 
		{
			compare_str,
			compare_str
		};
		new_scankey_init
			(
				index_scankey_data, 
				index_co, 
				index_fieldno, 
				index_filedno_cmp_func, 
				2, 
				vc_scancondition
			);
		is_found = false;
		is_found = is_found_single_data_use_index(&pIndexEntrySet, vc_scancondition, dest_data);
		if(!is_found/*û���ҵ�dest_data,����ʧ��*/)
		{
			WHEN_TEST_END();
			commit_transaction();
			return false;
		}
	}
	catch (StorageEngineException &ex) 
	{
		WHEN_EXCEPTION_CATCH();
	}
	WHEN_TEST_END();
	commit_transaction();
	return true;
}