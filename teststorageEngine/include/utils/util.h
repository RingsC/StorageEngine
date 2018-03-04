#ifndef TEST_UTILS_H
#define TEST_UTILS_H
#include <set>
#include <vector>
#include <map>
#include <bitset>
#include <string>
#include "postgres.h"
#include "utils/rel.h"
#include "access/heapam.h"
#include "access/genam.h"
#include "postmaster/postmaster.h"
using std::string;
#define ARRAY_LEN_CALC(ptr) sizeof(ptr)/sizeof(ptr[0])
#define DATA_LEN 1024

extern void setColInfo(Oid colid, Colinfo pcol_info);

#define EXTERN_SIMPLE_FUNCTION \
	extern void thread_create_heap(const int rel_id, BackendParameters *param, int *sta);\
	extern void thread_insert(const char data[][DATA_LEN], \
													const int array_len, \
													const int data_len, \
													const Oid rel_id, \
													BackendParameters *param, \
													int *sta);\
	extern void thread_heap_drop(int rel_id, BackendParameters *param, int *sta);\
	extern void thread_index_create(const int heap_id, \
																const Colinfo heap_colinfo, \
																const Colinfo ind_colinfo, \
																const int ind_id, \
																const bool unique,\
																BackendParameters *param,\
																int *sta);\
	extern void thread_index_drop(const int heap_id,\
																const int ind_id,\
																BackendParameters *param,\
																int *sta);\
	extern void thread_insert_with_index(const char data[][DATA_LEN], \
															const int data_len, \
															const int array_len, \
															const HeapIndexRelation *hir,\
															BackendParameters *GET_PARAM(), \
															IndexUniqueCheck iuc);

#define SIMPLE_CREATE_HEAP(relid, create_sta) \
	param = get_param();\
	SAVE_PARAM(param);\
	setColInfo(relid, heap_info); \
	tg.create_thread(bind(&thread_create_heap, relid, param, &create_sta));\
	tg.join_all();

#define SIMPLE_INSERT_DATA(relid, data, data_len, insert_sta) \
	param = get_param();\
	SAVE_PARAM(param);\
	tg.create_thread(bind(&thread_insert, data, ARRAY_LEN_CALC(data), data_len, relid, param, &insert_sta));\
	tg.join_all();

#define SIMPLE_INSERT_DATA_WITH_INDEX(hir, unique_check, data, data_len) \
	param = get_param();\
	SAVE_PARAM(param);\
	tg.create_thread(bind(&thread_insert_with_index, data, data_len, ARRAY_LEN_CALC(data), &hir, param, unique_check));\
	tg.join_all();

#define SIMPLE_DROP_HEAP(relid, drop_sta) \
	param = get_param();\
	SAVE_PARAM(param);\
	tg.create_thread(bind(&thread_heap_drop, relid, param, &drop_sta));\
	++relid; \
	tg.join_all();

#define SIMPLE_CREATE_INDEX(heap_id, ind_id, heap_colinfo, ind_colinfo, create_sta) \
	param = get_param();\
	SAVE_PARAM(param);\
	setColInfo(ind_id, ind_colinfo); \
	tg.create_thread(bind(&thread_index_create, heap_id, heap_colinfo, ind_colinfo, ind_id, false, param, &create_sta));\
	tg.join_all();

#define SIMPLE_DROP_INDEX(heap_id, ind_id, drop_sta) \
	param = get_param();\
	SAVE_PARAM(param);\
	tg.create_thread(bind(&thread_index_drop, heap_id, ind_id, param, &drop_sta));\
	++ind_id; \
	tg.join_all();


#define PREPARE_TEST() \
	thread_group tg;\
	BackendParameters *param = NULL;\
	PREPARE_PARAM(BackendParameters *);

#define GET_PARAM() param
#define GET_THREAD_GROUP() tg

extern int RELID;
extern int INDEX_ID;
extern BackendId BID;
extern int RID;
extern void createTable(const int rel_id = RID, const Colinfo heap_info = 0);
extern void dropTable();

extern bool win32_thread_bgwriter(void*);
extern void start_engine(const char *, const uint32, bool, const storage_params *);
extern void stop_engine(int);
extern void begin_first_transaction();
extern void commit_transaction();
extern void user_abort_transaction();
extern void begin_transaction();

extern void begin_subtransaction();
extern void commit_subtransaction();
extern void abort_subtransaction();

extern void create_heap(const int rel_id = RELID, const Colinfo heap_info = NULL);
extern void create_index(int heap_id, Colinfo heap_colinfo, Colinfo ind_colinfo, int ind_id = INDEX_ID, bool unique = false);
extern void remove_heap(int &rel_id = RELID);
extern void remove_index(const int heap_id=30000, int &ind_id = INDEX_ID);
extern void insert_data(const char insert_data[][DATA_LEN], 
												const int array_len, 
												const int data_len,
												const int rel_id = RELID);
extern void insert_data(const std::vector<std::string> &insert_data,
												const int rel_id = RELID,
												std::vector<ItemPointerData> *v_it = NULL);
extern void delete_data_(const std::vector<ItemPointerData> &delete_data,
												const int rel_id = RELID);
extern void form_index(const char insert_data[][DATA_LEN], 
											 const int array_len, 
											 const int data_len,
											 const int rel_id,
											 const int index_id,
											 Colinfo heap_info,
											 Colinfo ind_info,
											 IndexUniqueCheck iuc);
extern void insert_large_data(char *pdataArr[], 
							  const int array_len, 
							  const int data_len,
							  const int rel_id);
extern void calc_tuple_num(Oid relid, int &count);
extern char *make_data(unsigned int size);
extern char *make_unique_data(unsigned int size,unsigned int row);
ItemPointerData findTuple(const char *data, Relation rel, int &sta, int &count, int cmp_len = 0);
bool check_equal_all(const int rel_id, const char data[][DATA_LEN], const int array_len);
bool check_all_in_hash_tbl(const int *relid, const int len);
bool check_all_in_array(char src[][DATA_LEN], 
												char det[][DATA_LEN],
												unsigned int src_array_len,
												unsigned int det_array_len, 
												unsigned int data_len);
bool check_array_equal(const char array1[][DATA_LEN],
											 const int array1_len,
											 const char array2[][DATA_LEN],
											 const int array2_len);
bool check_array_equal(char src[][DATA_LEN], 
					   char det[][DATA_LEN],
					   unsigned int array_len,
					   unsigned int array_row);//检测两个数组是否相同
extern void exit_test_save_data(void *data, int len, const char *filename);
extern void *exit_test_read_data(const char *filename);
extern void exit_test_save_data(std::vector<std::string> &v_data, const char *filename);
extern void exit_test_read_data(const char *filename, std::vector<std::string> &v_data, int read_count);

#define EXIT_TEST_SAVE_LARGE_DATA(v_data) \
	exit_test_save_data(v_data, __FUNCTION__);

#define EXIT_TEST_READ_LARGE_DATA(v_data, read_count) \
	exit_test_read_data(__FUNCTION__, v_data, read_count);

#define EXIT_TEST_SAVE_DATA(data, len) \
	exit_test_save_data((data), (len), __FUNCTION__);

#define EXIT_TEST_READ_DATA() \
	exit_test_read_data(__FUNCTION__);

extern std::vector<std::string> *ran_do_insert(const Oid rel_id, bool &is_deadlock, int least_len = 100);
extern std::vector<std::string> *ran_do_delete(const Oid rel_id, bool &is_deadlock);
extern std::vector<std::pair<std::string, std::string> > *ran_do_update(const Oid rel_id, bool &is_deadlock, int least_len = 100);

std::pair<char*, int> tuple_get_string_and_len(HeapTuple tuple);

int start_engine_();
int start_thread_engine_();
Oid get_heap_id();
Oid get_index_id();
Oid get_remove_heap_id();
Oid get_remove_index_id();
void clear_heap_id();
void clear_index_id();
void clear_all();
BackendParameters *
get_param(BackendId bid = BID, ThreadType type = backend);

int stop_engine_();

int exit_proc();

#define CHECK_ALL_TRUE(array, size, _sta) \
	std::bitset<size> _tm(0xFFFF); \
	_sta = ((array & _tm) == _tm);

#define PREPARE_PARAM(type) \
	std::vector<type> vc_backend_param

#define SAVE_PARAM(param) \
	vc_backend_param.push_back(param)

#define FREE_PARAM(type) \
	std::vector<type>::iterator it = vc_backend_param.begin(); \
	while(it != vc_backend_param.end()) \
	{ \
		free(*it); \
		++it; \
	} \
	vc_backend_param.clear()

typedef struct
{
public:
	unsigned int ncol;
	Datum *cmp_values;
	StrategyNumber *cmp_strategy;
	CompareCallback *cmp_func;
	unsigned int *col_array;
	ScanKeyData *scan_key;
} IndexScanInfo;

typedef struct  
{
public:
	unsigned int ncol;/* 比较多少个列 */
	unsigned int *col_array;/* heap每列长度 */
	unsigned int *col_num;/* 在第几列上比较 */
	char **cmp_data;	/* 比较字符串 */
	//unsigned int *cmp_len; /* 比较长度 */
	StrategyNumber *cmp_stn;/* 比较策略 */
} CheckResultInfo;

typedef struct 
{
	Oid heap_id;
	Oid *index_id;
	Colinfo heap_info;
	Colinfo *index_info;
	unsigned int index_array_len;
} HeapIndexRelation;

void get_result_in_array(char src[][DATA_LEN],
												 const unsigned int src_array_len,
												 const CheckResultInfo &cri,
												 char det[][DATA_LEN],
												 unsigned int &return_len);

void init_scan_key(IndexScanInfo &isi);
void free_scan_key(IndexScanInfo &isi);
void alloc_scan_space(const unsigned int size, IndexScanInfo &isi);
void free_scan_space(IndexScanInfo &isi);

template<typename T>
class VectorMgr
{
public:
	VectorMgr(){}
	void vPush(T t)
	{
		v_t.push_back(t);
	}
	std::vector<T>& getVMgr()
	{
		return v_t;
	}
	virtual ~VectorMgr()
	{
		for(int i = 0; i < v_t.size(); ++i)
		{
			delete (v_t[i]);
		}
	}
private:
	std::vector<T> v_t;
};

class DataGenerater
{
public:
	DataGenerater(const unsigned int size, const unsigned int data_len);
	DataGenerater(const DataGenerater&);
	DataGenerater(){}
	virtual ~DataGenerater();

public:
	void dataGenerate();
	void dataToDataArray2D(char data[][DATA_LEN]);
	void dataToDataArray2D(const int size, char data[][DATA_LEN]);
	void dataGenerate(const char *src_data, const int data_len, int pos);
	inline unsigned int getRowNum()
	{
		return m_row_num;
	}
	inline unsigned int getDataLen()
	{
		return m_data_len;
	}
	inline unsigned int getSize()
	{
		return m_size;
	}
	inline void setSize(unsigned int size)
	{
		m_size = size;
	}
	inline char* operator[](int pos)
	{
		if(pos >= (int)m_row_num)
		{
			return NULL;
		}
		return &m_rand_data[pos * m_data_len];
	}

private:
	void space_increment();

private:
	char *m_rand_data;
	unsigned int m_row_num;
	unsigned int m_data_len;
	unsigned int m_size;
};


class SpliterGenerater
{
	/*============= public =============*/
public:
	SpliterGenerater();
	virtual ~SpliterGenerater();
	Colinfo buildHeapColInfo(const int col_count, ...);
	Colinfo buildIndexColInfo(const int col_count, 
							const int *col_number, 
							const CompareCallback *cmp_func_array,
							const Split split_func);
public:
	/*
	* 定义五个模板函数，能将索引分为最多5个列
	*/
	template<int heap_col_1>
	static
	void index_split_to_any(RangeData& rd, const char *str, int col, size_t data_len)
	{
		memset(&rd, 0, sizeof(rd));
		rd.start = 0;
		rd.len = SpliterGenerater::m_vc_col_array[heap_col_1 - 1];
	}

	template<int heap_col_1, int heap_col_2>
	static
	void index_split_to_any(RangeData& rd, const char *str, int col, size_t data_len)
	{
		memset(&rd, 0, sizeof(rd));
		int col_array[] = {heap_col_1, heap_col_2};
		for(int i = 0; i < col; ++i)
		{
			rd.start += rd.len;
			rd.len = SpliterGenerater::m_vc_col_array[col_array[i] - 1];
		}
	}

	template<int heap_col_1, int heap_col_2, int heap_col_3>
	static
	void index_split_to_any(RangeData& rd, const char *str, int col, size_t data_len)
	{
		memset(&rd, 0, sizeof(rd));
		int col_array[] = {heap_col_1, heap_col_2, heap_col_3};
		for(int i = 0; i < col; ++i)
		{
			rd.start += rd.len;
			rd.len = SpliterGenerater::m_vc_col_array[col_array[i] - 1];
		}
	}

	template<int heap_col_1, int heap_col_2, int heap_col_3, int heap_col_4>
	static
	void index_split_to_any(RangeData& rd, const char *str, int col, size_t data_len)
	{
		memset(&rd, 0, sizeof(rd));
		int col_array[] = {heap_col_1, heap_col_2, heap_col_3, heap_col_4};
		for(int i = 0; i < col; ++i)
		{
			rd.start += rd.len;
			rd.len = SpliterGenerater::m_vc_col_array[col_array[i] - 1];
		}
	}

	template<int heap_col_1, int heap_col_2, int heap_col_3, int heap_col_4, int heap_col_5>
	static
	void index_split_to_any(RangeData& rd, const char *str, int col, size_t data_len)
	{
		memset(&rd, 0, sizeof(rd));
		int col_array[] = {heap_col_1, heap_col_2, heap_col_3, heap_col_4, heap_col_5};
		for(int i = 0; i < col; ++i)
		{
			rd.start += rd.len;
			rd.len = SpliterGenerater::m_vc_col_array[col_array[i] - 1];
		}
	}

public:
	static std::vector<int> m_vc_col_array;

	/*============= private =============*/
private:
	SpliterGenerater(const SpliterGenerater&);

private:
	Colinfo m_pheap_cf;
	Colinfo m_pindex_cf;
};

class FixSpliter
{
public:
	explicit FixSpliter(const std::vector<int> &vec);
	static void split(RangeData& rangeData, const char *pszNeedSplit,int iIndexOfColumn, size_t len=0);
private:
	static std::vector<int> g_vecOfSplitPos;
};

class VarSpliter
{
public:
	explicit VarSpliter(const char cSep)
	{
		g_cSep = cSep;
	}

	static void split(RangeData& rangeData, char *pszNeedSplit,int iIndexOfColumn);

private:
	static char g_cSep;
};
class SimpleIndex;
class SimpleIndex;
class SimpleHeap
{
public:
	SimpleHeap(Oid idRelTableSpace,Oid idRelation,Split spliter,bool needCreate = false,bool needDrop = true);

	void Open(LOCKMODE lockMode);

	void Insert(const std::string& strInsert);

	void Delete(const std::string& strDelete);

	ItemPointer Find(const std::string& pszValue);

	std::set<std::string>& GetAll(void);

	void Update( const std::string& szOldValue,const string& szNewValue);

	~SimpleHeap();

	Relation Get( void )
	{
		return m_relation;
	}

	Oid GetRelTableSpaceId( void )
	{
		return m_idReltableSpace;
	}

	Oid GetRelationId( void )
	{
		return m_idReleation;
	}

private:
	Oid m_idReltableSpace;
	Oid m_idReleation;
	Relation  m_relation;
	LOCKMODE m_lockMode;
	bool m_bNeedDrop;
	friend class SimpleIndex;
	std::vector<SimpleIndex*> m_vecIndexs;
	std::set<std::string> m_setData;
};

class SimpleIndex
{
public:
	SimpleIndex(SimpleHeap& heap,
		Oid idIndex,
		const std::map<int,CompareCallback> &vecKeys,
		bool bCreate = true,
		bool needDrop = true);

	std::set<string>&  Find(class SearchCondition& scanCondition);
	std::vector<string>& Find(class SearchCondition& scanCondition,int hold);

	void Insert(HeapTuple updatedTuple);

	~SimpleIndex();

	Relation Get( void ){return m_indexRelation;}
private:
	SimpleIndex& operator=(const SimpleIndex& );

	SimpleHeap& r_heap;
	Relation m_indexRelation;
	Oid m_idIndex;
	Colinfo m_pCollInfo;
	bool m_bNeedDrop;
	std::set<string> m_setResult;
	std::vector<string> m_vResult;
};

enum EScanOp
{
	LessThan = 1,
	LessEqual,
	Equal,
	GreaterEqual,
	GreaterThan
};

class SearchCondition
{
public:
	SearchCondition();
	~SearchCondition();

	void Add(AttrNumber nColumn,EScanOp op,const char* pszValue,CompareCallback compare);

	ScanKey Keys(void){return m_keys;}

	int Count( void ){ return m_nCount;}
private:
	int m_nCount;
	ScanKeyData m_keys[32];
	Datum m_values[32];
};

class OIDGenerator
{
public:
	static OIDGenerator& instance();

	Oid GetTableSpaceID( void );
	Oid GetHeapID( void );
	Oid GetIndexID( void );
private:
	OIDGenerator(){}
	OIDGenerator(const OIDGenerator&);
};

#define CATCHEXCEPTION \
catch(StorageEngineExceptionUniversal &ex)\
{\
	std::cout << ex.getErrorNo() << std::endl;\
	std::cout << ex.getErrorMsg() << std::endl;\
	user_abort_transaction();\
	return false;\
}\
catch(std::runtime_error& stdex)\
{\
	std::cout<<stdex.what()<<std::endl;\
	user_abort_transaction();\
	return false;\
}

#define CATCHNORETURN \
	catch(StorageEngineExceptionUniversal &ex)\
{\
	std::cout << ex.getErrorNo() << std::endl;\
	std::cout << ex.getErrorMsg() << std::endl;\
	user_abort_transaction();\
}\
	catch(std::runtime_error& stdex)\
{\
	std::cout<<stdex.what()<<std::endl;\
	user_abort_transaction();\
}

#define END_TRANSACTION \
{\
	commit_transaction();\
	printf("test success！\n");\
}\
else\
{\
	user_abort_transaction();\
	printf("tset failed！\n");\
}

#define CATCH_EXCEPTION_AND_ABORT \
	catch ( StorageEngineExceptionUniversal &ex )\
{\
	user_abort_transaction();\
	cout << ex.getErrorNo() << endl;\
	cout << ex.getErrorMsg() << endl;\
	dropTable();\
	return false;\
}

void index_scan_hs(    const Oid index_id, 
				   const Colinfo ind_info, 
				   const Oid heap_id, 
				   const Colinfo heap_info,
				   const IndexScanInfo *isi, 
				   DataGenerater *dg,
				   unsigned int *array_len);
int str_compare(const char *str1, size_t len1, const char *str2, size_t len2);
void insert_with_index(const char data[][DATA_LEN], 
					   const int data_len,
					   const int array_len,  
					   const HeapIndexRelation *hir, 
					   IndexUniqueCheck iuc);
void OutPutToastErrorInter(std::set<std::string>& sDesired,std::set<std::string>& sResult,const char* pFile,size_t nLen);
void FreeColiInfo(Colinfo pInfo);
#define OutPutToastError(s,d) OutPutToastErrorInter(s,d,__FILE__,__LINE__)

struct ScanKeyInfo
{
	int				colid;
	char			argument[10];
	StrategyNumber	cmpStrategy;
};

class HeapFuncTest
{
public:
	HeapFuncTest();

	//heap column initial
	void buildHeapColInfo();
	void buildHeapColInfo( int colnum, const int length[] );

	//heap create
	void createHeap( const int rel_id, const Colinfo heap_info );

	//open db
	void openHeap( LOCKMODE lockmode );

	//insert data
	void insertRangeTuples( const int begin, const int end, char header[], const int headerLen );
	void insertTuples( char (*data)[20], const int amount );
	void insert( char data[], const int dataLen );

	//delete data
	void deleteTupleById( ItemPointer itemPointer );

	//scan data
	void scanRangeTuplesWithHeader( const int amount, char header[], const int headerLen );
	void scanTupleInserted( const int amount, char data[], const int dataLen );
	void scanTuplesInserted( char (*data)[20], const int amount, const int dataLen );
	void scanWithKey( struct ScanKeyInfo *scanKey, int nkeys );
	void scanDeleteTuple( char *data, int dataLen, int amount );

	//GetOffset
	ItemPointer GetOffsetByNo( const int Num );

	//construct scan key
	void constructKeys( struct ScanKeyInfo *scanKey, int nkeys );

	//remove table
	void dropHeap();

	//tool function
	bool resultCheck( char *scanData, struct ScanKeyInfo *scanKey, const int nkeys ); //扫描key查询结果检查
	int getDigitLength( int digit ); //获取数字位数

	~HeapFuncTest();

public:
	bool m_bSuccess;
	static std::vector<int> m_heap_col_array;

protected:
	Colinfo m_pHeap;
	Relation m_pRelation;
	Oid m_nRelid;
	ScanKeyData *m_pKey;
};

class IndexFuncTest:public HeapFuncTest
{
public:
	IndexFuncTest();

	//index column initial
	void buildIndexColInfo( int colnum, const int length[], struct ScanKeyInfo *scanKey );
	//create index
	void createIndex( int indid, Colinfo indCol );
	//index scan
	void scanIndex( ScanKeyInfo *pKeys, const int nkeys );

	~IndexFuncTest();

public:
	static std::vector<int> m_index_col_array;

private:
	Oid m_nIndid;
	Colinfo m_pIndex;
};

#endif //TEST_UTILS_H
