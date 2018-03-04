#ifndef TEST_UTILS_DLL_H
#define TEST_UTILS_DLL_H
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include  <boost/preprocessor/repetition/enum.hpp>
#include <boost/assign.hpp>
#include "StorageEngine.h"
#include "PGSETypes.h"
//#include "StorageEngineException.h"


//extern int THREAD_NUM_10;
#define THREAD_NUM_10 10
#define DATA_LEN  200
#define LOOP_TIMES 200
#define HEAP_CNT 1
#define DATA "hello world"
#define INT_LEN 20


using std::vector;
using std::string;
using namespace FounderXDB::StorageEngineNS;
//////mulit thread test //////
extern vector<EntrySetID> entrySetIdVec;
// heap create////
extern Transaction* start_new_transaction(Transaction::IsolationLevel   level);
extern bool thrfun_heap_create();
extern bool thrfun_heap_close(Transaction* txn,EntrySetID heap_id);
extern bool thrfun_index_create(Transaction *txn);
extern bool thrfun_index_create_del();
extern bool thrfun_index_create_indid(EntrySetID &indid);
extern bool thrfun_index_create_indid_heapid(EntrySetID &heapid, EntrySetID &indid);
extern bool thrfun_index_open_indid(EntrySetID &indid);
extern bool thrfun_index_open_indid_heapid(EntrySetID &heapid, EntrySetID &indid);
//extern void thread_heap_create( int &i);
extern void thread_heap_create( int *i);
extern void thread_heap_create( EntrySetID *heapId, int *i);
extern void thread_heap_close(EntrySetID heap_id, int *i);
extern void thread_heap_del(EntrySetID heap_id, int *i);
extern void thread_heap_insert(EntrySetID heap_id, int *i);
extern void thread_heap_insert_large(EntrySetID heap_id, int len, int *i);
extern void thread_heap_update_large_find(EntrySetID heap_id, int len, int *i);
extern void thread_heap_insert_large_find(EntrySetID heap_id, int len, int *i);

extern void thread_heap_dele(EntrySetID heap_id,EntryID eid, int *i);
extern void thread_heap_update(EntrySetID heap_id,EntryID eid, int *i);
extern void thread_heap_update_large(EntrySetID heap_id,int len, EntryID *eid, int *i);
extern void thread_heap_find(EntrySetID heap_id,EntryID eid, int *i);
extern void thread_heap_find_large(EntrySetID heap_id,EntryID eid, DataItem *data, int *i);
extern void thread_heap_find_chars(EntrySetID heap_id,EntryID eid, int *i);
extern void thread_heap_getNext(EntrySetID heap_id,EntryID eid, int *i);
extern void thread_heap_getNext_large(EntrySetID heap_id,EntryID eid, DataItem *data, int *i);

extern void thread_index_find(EntrySetID heap_id,EntrySetID index_id, EntryID eid, int *i);
extern void thread_index_getNext(EntrySetID heap_id,EntrySetID index_id, EntryID eid, int *i);
extern void thread_index_getNext_large(EntrySetID heap_id,EntrySetID index_id, EntryID eid, DataItem *data, int *i);

//extern void thread_transaction_create(int &i);
extern void thread_transaction_create(int *i);
extern void thread_transaction_create_noncommit(int *i);
extern void thread_index_create_indid(EntrySetID *indid, int *i);
extern void thread_index_create_indid_heapid(EntrySetID *heapid, EntrySetID *indid, int *i);
extern void thread_index_create_open(EntrySetID &indid, int *i);
extern void thread_index_create_open_heapid(EntrySetID &heapid, EntrySetID &indid, int *i);

extern void thread_index_create(int *i);
extern void thread_index_create(EntrySetID *heapId, EntrySetID *indexId, int *i);
extern void thread_index_create_check_valid(EntrySetID heapId, EntrySetID *indexId, int *i);
extern void thread_index_create_check_valid_1(EntrySetID heapId, int *i);
//extern void thread_index_create_unique(EntrySetID *heapId,int *i);
extern void thread_index_create_check_unique(EntrySetID *heapId,int *i);
extern void thread_index_create_check_unique_1(EntrySetID heapId,int *i);
extern void thread_index_del(EntrySetID heapId, EntrySetID indexId, int *i);
extern void thread_index_create_del(int *i);
extern void thread_open_heap(EntrySetID heapId, int *i);
extern void heap_create_pointer(EntrySetID *heap_id, int *i);
extern void heap_create_insert_data(EntrySetID *heap_id, EntryID *eid, int *i);
extern void heap_create_insert_data_large(EntrySetID *heap_id, EntryID *eid, DataItem *data,int *i);
extern void heap_index_create_insert_data(EntrySetID *heap_id, EntrySetID *index_id, EntryID *eid, int *i);
extern void heap_index_create_insert_data_large(EntrySetID *heap_id, EntrySetID *index_id, EntryID *eid, DataItem *data, int *i);
extern void heap_create(EntrySetID &heap_id, int &i);
extern void heap_remove(EntrySetID heapId, int *i);
extern void thread_del_index(EntrySetID heapid, EntrySetID indexid, int *i);
extern void heap_remove_poi(EntrySetID heapId, int *i);
extern void index_remove_poi(EntrySetID heapId, EntrySetID indexId, int *i);
extern bool heap_insert(Transaction* txn,EntrySetID heapId);
extern bool heap_insert_large(Transaction* txn,EntrySetID heapId, int len);
extern bool heap_insert_large_find(Transaction* txn,EntrySetID heapId, int len);
extern bool heap_insert_update_large_find(Transaction* txn,EntrySetID heapId, int len);
extern bool heap_dele(Transaction* txn,EntrySetID heapId, EntryID eid);
extern bool heap_update(Transaction* txn,EntrySetID heapId, EntryID eid);
extern bool heap_update_large(Transaction* txn,EntrySetID heapId, int len, EntryID &eid);
extern bool heap_find(Transaction* txn,EntrySetID heapId, EntryID eid);
extern bool heap_find_chars(Transaction* txn,EntrySetID heapId, EntryID eid);
extern bool heap_find_large(Transaction* txn,EntrySetID heapId, DataItem &data, EntryID eid);
extern bool heap_getNext(Transaction* txn,EntrySetID heapId, EntryID eid);
extern bool heap_getNext_large(Transaction* txn,EntrySetID heapId, DataItem &data, EntryID eid);
extern bool index_find(Transaction* txn,EntrySetID heapId, EntrySetID indexId, EntryID eid);
extern bool index_getNext(Transaction* txn,EntrySetID heapId, EntrySetID indexId, EntryID eid);
extern bool index_getNext_large(Transaction* txn,EntrySetID heapId, EntrySetID indexId, DataItem &data, EntryID eid);
extern bool heap_insert_eid(Transaction* txn,EntrySetID heapId, EntryID &eid);
extern bool heap_insert_eid_large(Transaction* txn,EntrySetID heapId, EntryID &eid, DataItem &data);

////heap open ////
extern bool thrfun_index_open(EntrySetID heapId = 0);

extern bool cmpdata(void* first, void* sec, int len);
extern bool checkeid(EntryID eid);
extern bool cheakEid(EntryID eid, EntryID eid_find);
extern bool cheakData(int a, int b);
extern void formDataItem(int len, DataItem &data);
extern void deformDataItem(DataItem *data);
extern bool checkDataItem_part(DataItem *data1, DataItem *data2);
extern bool checkDataItem_full(DataItem *data1, DataItem *data2);





//////end mulit thread test ///////////




//extern Transaction *pTransaction;
extern void get_first_transaction();
extern void commit_transaction();
extern void user_abort_transaction();
extern void get_new_transaction();
extern MemoryContext* get_txn_mem_cxt();

extern void command_counter_increment();
int start_engine_();
int stop_engine_();
void startEngineAndCreateTable();
void dropTableAndStopEngine();

class MyColumnInfo
{
public:
	MyColumnInfo(const std::map<int,CompareCallbacki>& mapCompare,Spliti split);
	~MyColumnInfo();
	ColumnInfo& Get(void){return m_columnInfo;}
private:
	ColumnInfo m_columnInfo;
};
int str_compare(const char *str1, size_t len1, const char *str2, size_t len2);
int vstr_compare(const char *str1, size_t len1, const char *str2, size_t len2);


/************************************************************************** 
* @brief varSplit 
* 产生可变长度属性的split函数,属性之间用\000分隔
* Detailed description.
* @param[in] pszNeedSplit 待分隔的字符数据
* @param[in] iIndexOfColumn 列序号，从1开始
* @return RangeDatai  返回第iIndexOfColumn的起始位置和长度
**************************************************************************/
void VarSplit(RangeDatai& rd, const char *pszNeedSplit,int iIndexOfColumn, size_t len = 0);


#define CATCHEXCEPTION \
	catch(StorageEngineException &ex)\
{\
	user_abort_transaction();\
	cout << ex.getErrorNo() << endl;\
	cout << ex.getErrorMsg() << endl;\
	return false;\
}\

#define CATCHEXCEPTION1(pTrans) \
	catch(StorageEngineException &ex)\
{\
	pTrans->abort();\
	delete pTrans;\
	pTrans = NULL;\
	cout << ex.getErrorNo() << endl;\
	cout << ex.getErrorMsg() << endl;\
	return false;\
	}\

#define HEAP_RETURN_FALSE \
	pEntrySet->endEntrySetScan(pHeapScan);\
	pStorageEngine->closeEntrySet(pTransaction,pEntrySet);\
	user_abort_transaction();\
	return false;

#define HEAP_RETURN_FALSE_HS \
	penry_set->endEntrySetScan(entry_scan);\
	pStorageEngine->closeEntrySet(pTransaction,penry_set);\
	user_abort_transaction();\
	return false;

#define VOID_CATCHEXCEPTION(pTrans) \
	catch(StorageEngineException &ex)\
{\
	cout << ex.getErrorNo() << endl;\
	cout << ex.getErrorMsg() << endl;\
	pTrans->abort();\
	delete pTrans;\
	pTrans = NULL;\
}

class FixSpliter
{
public:
	explicit FixSpliter(const std::vector<int> &vec);
	static void split(RangeDatai& rangeData, const char *pszNeedSplit,int iIndexOfColumn, size_t len = 0);
private:
	static std::vector<int> g_vecOfSplitPos;
};

#define NN(z,n,data) int N##n = 0
#define TIMES  10
#define ACCUMULATOR(z,n,data) m_globalTable[BOOST_PP_ADD(n,1)] = m_globalTable[n] + N##n

/**************************************************************************
class StaticSpliter
* 用这个类可以得到一个EntrySet的split函数和总共有多少列,例如:StaticSpliter<3,2,8>
* 它的size()会返回3,因为它有3列,它产生的split函数保证第一列有3个字符，第二列有2个
* 字符，第3列有8个字符.
* Detailed description.
**************************************************************************/
static int used_to_remove_waring = 0;
template<BOOST_PP_ENUM(TIMES,NN,~)>
class StaticSpliter
{
public:

	static void split(RangeDatai& rangeData,const char *,int iIndexOfColumn, size_t len = 0)
	{
		static bool init = Init();
		used_to_remove_waring += len + init ? 1 : 0;
		rangeData.start = m_globalTable[iIndexOfColumn - 1];
		rangeData.len = m_globalTable[iIndexOfColumn] - m_globalTable[iIndexOfColumn - 1];
	}

	static int size()
	{
		static bool init = Init();
		return m_globalSize;
	}

	static void print()
	{
		std::cout<<"一共"<<m_globalSize<<"列:"<<std::endl;
		for(int i = 1;i <= m_globalSize;++i)
		{
			RangeDatai rangeData;
			memset(&rangeData, 0, sizeof(rangeData));
			split(rangeData, NULL,i);

			std::cout<<"("<<rangeData.start<<","<<rangeData.len<<")";
		}
		std::cout<<std::endl;
	}
private:
	static bool Init()
	{
		BOOST_PP_ENUM(TIMES,ACCUMULATOR,~);
		m_globalSize = TIMES;

		for(int i = 2;i <= TIMES + 1;++i)
		{
			if (m_globalTable[i] == m_globalTable[i - 1])
			{
				m_globalSize = i - 1;
				break;
			}
		}
		return true;
	}
	static int m_globalTable[TIMES + 1];
	static int m_globalSize;
};
template<BOOST_PP_ENUM_PARAMS(TIMES,int n)>
int StaticSpliter<BOOST_PP_ENUM_PARAMS(TIMES,n)>::m_globalTable[TIMES + 1] = {0};

template<BOOST_PP_ENUM_PARAMS(TIMES,int n)>
int StaticSpliter<BOOST_PP_ENUM_PARAMS(TIMES,n)>::m_globalSize = 0;

/**************************************************************************
* class EntrySetCollInfo
* 为EntrySet返回一个columnInfo的id,它的split函数由StaticSpliter定义，comp函
* 数为str_compare 
* Detailed description.
**************************************************************************/
extern uint32 EntryColId;
template<BOOST_PP_ENUM(TIMES,NN,~)>
class EntrySetCollInfo
{
public:
	static uint32 get(void)
	{
		static uint32 colid = Init();
		return colid;
	}

private:
	static uint32 Init(void)
	{
		typedef StaticSpliter<BOOST_PP_ENUM_PARAMS(TIMES,N)> SplitT;

		using namespace boost::assign;
		std::map<int,CompareCallbacki> mapComp;
		for (int i = 1; i <= SplitT::size();++i)
		{
			mapComp[i] = str_compare;
		}

		static MyColumnInfo columnInfo(mapComp,SplitT::split);

		EntryColId += 3;
		setColumnInfo(EntryColId,&columnInfo.Get());
#if 0
		SplitT::print();
#endif
		return EntryColId;
	}
};

template<typename T,int> struct CharNum{const static int value = 0;};
#define DEFINE_COLUMN_SIZE(z,n,data)\
template<BOOST_PP_ENUM_PARAMS(TIMES,int N)>\
struct CharNum<EntrySetCollInfo<BOOST_PP_ENUM_PARAMS(TIMES,N)>,BOOST_PP_INC(n)> \
{\
	const static int value = BOOST_PP_CAT(N,n);\
};

BOOST_PP_REPEAT(TIMES,DEFINE_COLUMN_SIZE,~)
#undef DEFINE_COLUMN_SIZE

/**************************************************************************
* @class ate<typename EnsetT,BOOST_PP_ENUM(TIMES,NN,~)>
* 给定一个EntrySet,及其需要创建索引的列，得到一个ColumnInfo,例如:
* EntrySetCollInfo<3,4,2,9> 一共有4列，各列字符个数为3,4,2,9
* IndexCollInfo<EntrySetCollInfo<3,4,2,9>, 2,3>可以返回一个在第2列和第3列上索引的ColumnInfo
**************************************************************************/
extern uint32 IndexId;
template<typename EnsetT,BOOST_PP_ENUM(TIMES,NN,~)>
class IndexCollInfo
{
public:
	static uint32 get(void)
	{
		static uint32 colid = Init();
		return colid;
	}
private:
	static uint32 Init(void)
	{
#define GET_COLUMN_SIZE(z,n,data) CharNum<EnsetT,BOOST_PP_CAT(N,n)>::value
#define SET_COLUMN(z,n,data) mapComp[BOOST_PP_CAT(N,n)] = str_compare
		typedef StaticSpliter<BOOST_PP_ENUM(TIMES,GET_COLUMN_SIZE,~)> SplitT;
		using namespace boost::assign;
		std::map<int,CompareCallbacki> mapComp;
		BOOST_PP_ENUM(TIMES,SET_COLUMN,~);				
		mapComp.erase(mapComp.find(0));

		static MyColumnInfo columnInfo(mapComp,SplitT::split);
#if 0
		SplitT::size();
		SplitT::print();
#endif
		IndexId += 3;
		setColumnInfo(IndexId,&columnInfo.Get());
		return IndexId;
#undef SET_COLUMN
#undef GET_COLUMN_SIZE
	}
};

/************************************************************************** 
* @brief getVarLenColumnInfo 
* 返回一个可变长的ColumnInfo的Id
* Detailed description.
* @param[in] N0,N1...N9指定在哪些列上创建索引
* @return uint32  ColumnInfo的id. 
**************************************************************************/
extern uint32 VarColId ;
template<BOOST_PP_ENUM(TIMES,NN,~)>
class VarLenColumnInfo
{
public:
	static uint32 get(void)
	{
		static uint32 varId = 0;
		if (0 == varId)
		{
#define SET_COMPARE(z,n,data) mapComp[BOOST_PP_CAT(N,n)]= vstr_compare
			varId = VarColId += 3;
            std::map<int,CompareCallbacki> mapComp;
			BOOST_PP_ENUM(TIMES,SET_COMPARE,~);
			mapComp.erase(mapComp.find(0));

			static MyColumnInfo columnInfo(mapComp,VarSplit);
			setColumnInfo(varId,&columnInfo.Get());
		}

		return varId;
	}
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

	void Add(int nColumn,EScanOp op,const char* pszValue,int (*compare)(const char *, size_t , const char *, size_t));

	std::vector<ScanCondition>& Keys(void){return m_vConditions;}

	int Count( void ){ return m_nCount;}
private:
	ScanCondition::CompareOperation GetOption(EScanOp op);

	int m_nCount;
	uint32 m_values[32];

	std::vector<ScanCondition> m_vConditions;

	friend std::ostream& operator<<(std::ostream& os,const SearchCondition& search);
};

/************************************************************************** 
* @brief GetEntryId 
* 
* Detailed description.
* @param[in] pTrans 当前事务
* @param[in] pEntrySet 
* @param[in] vData 要查找的数据
* @param[in] vIds 返回与vData中对应的id
**************************************************************************/
void GetEntryId(Transaction *pTrans
				,EntrySet* pEntrySet
				,const std::vector<std::string>& vData
				,std::vector<EntryID>& vIds);

/************************************************************************** 
* @brief InsertData 
* Help Function :将vData中的数据插入到pEntrySet,返回对数据的EntryID
* Detailed description.
* @param[in] pTrans   当前事务
* @param[in] pEntrySet 用来插入数据的表
* @param[in] vData 待插入的数据
* @param[out] pvIds Defaults to NULL. 插入以后与vData对应的entry id
**************************************************************************/
void InsertData(Transaction * pTrans 
				,EntrySet *pEntrySet
				,const std::vector<std::string>& vData
				,std::vector<EntryID>* pvIds = NULL);

/************************************************************************** 
* @brief UpdateData 
* Help Function: 将pEntrySet中指定的数据进行更新
* Detailed description.
* @param[in] pTrans 当前事务 
* @param[in] pEntrySet 需要更新数据的entryset
* @param[in] vData <id,data>对将id对应的数据更新为data
**************************************************************************/
void UpdateData(Transaction * pTrans
				,EntrySet *pEntrySet
				,const std::map<EntryID,std::string>& vData);

/************************************************************************** 
* @brief DeleteData 
* 删除pEntrySet中由vIds指定的数据项
* Detailed description.
* @param[in] pTrans 当前事务
* @param[in] pEntrySet 数据所在的EntrySet
* @param[in] vIds 要删除数据的EntryId
**************************************************************************/
void DeleteData(Transaction * pTrans
				,EntrySet *pEntrySet
				,const std::vector<EntryID>& vIds);

bool operator < (const EntryID& lhs,const EntryID& rhs);

/************************************************************************** 
* @brief CompareVector 
* 比较lhs与rhs中的数据是否对应相等
* Detailed description.
* @param[in] lhs 
* @param[in] rhs 
* @return bool  如果lhs与rhs中的数据对应相等返回true,否则返回false.
**************************************************************************/
bool CompareVector(std::vector<std::string>& lhs,std::vector<std::string>& rhs);

/************************************************************************** 
* @brief GetDataFromEntrySet 
* 返回EntrySet中所有的数据
* Detailed description.
* @param[out] sResult 返回值
* @param[in] pTrans  当前事务
* @param[in] pEntrySet 数据所在的EntrySet
**************************************************************************/
void GetDataFromEntrySet(std::vector<std::string>& sResult,Transaction * pTrans,EntrySet *pEntrySet);

/************************************************************************** 
* @brief GetIndexScanResults 
* 使用pIndex查询所有满足条件的值
* Detailed description.
* @param[out] sResult 返回查询结果
* @param[in] pTrans 当前事务
* @param[in] pIndex 使用的index
* @param[in] vConditions 查询条件
**************************************************************************/
void GetIndexScanResults(std::vector<std::string>& sResult
						 ,Transaction * pTrans 
						 ,IndexEntrySet * pIndex
						 ,std::vector<ScanCondition>& vConditions) ;


/************************************************************************** 
* @brief GetSingleColInfo 
* 返回一个单列使用的columnInfo对应的id.
* Detailed description.
* @return uint32  
**************************************************************************/
uint32 GetSingleColInfo();


/************************************************************************** 
* @brief GetMultiCollInfo 
* 返回一个三列(第1列3个字符，第2列2个字符，第3列1个字符)
* Detailed description.
* @return uint32  
**************************************************************************/
uint32 GetMultiCollInfo();

/************************************************************************** 
* @brief RandomGenString 
* 随机生成nLen升序的字符串
* Detailed description.
* @param[in] s 
* @param[in] nLen 
**************************************************************************/
void RandomGenString(std::string& s,size_t nLen);



class TableDroper
{
public:
	static EntrySetID entryId;
	static EntrySetID indexId;

	static void Drop(void);
};

#define HEAP_RETURN_TRUE_HS \
	catch(StorageEngineException &ex)\
{\
	cout << ex.getErrorMsg() << endl;\
	penry_set->endEntrySetScan(entry_scan);\
	pStorageEngine->closeEntrySet(pTransaction,penry_set);\
	commit_transaction();\
	return true;\
}\

#define INDEX_COMMIT_HS \
pStorageEngine->closeEntrySet(pTransaction, pEntrySet);\
pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);\
pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet->getId());\
commit_transaction();\

#define INDEX_RETURN_FALSE_HS \
pStorageEngine->closeEntrySet(pTransaction, pEntrySet);\
pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet);\
pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet->getId());\
user_abort_transaction();\
return false;\

#define ENDSCAN_CLOSE_COMMIT \
	penry_set->endEntrySetScan(entry_scan);\
	pStorageEngine->closeEntrySet(pTransaction,penry_set);\
	commit_transaction();\

#define ARRAY_LEN_CALC(ptr) sizeof(ptr)/sizeof(ptr[0])

#define DLL_SIMPLE_EXTERN_FUNC \
	extern void thread_create_heap_dll(const int col_id, EntrySetID *id); \
	extern void thread_insert_dll(const EntrySetID eid, const char data[][DATA_LEN], const int row, int *sta); \
	extern void thread_update_dll(const EntrySetID eid, const char *det_data, const int det_len, \
																const char *src_data, const int src_len, int *sta); \
	extern void thread_delete_dll(const EntrySetID eid, const char *det_data, const int det_len, int *sta);

#define DLL_SIMPLE_CREATE_HEAP(eid, col_id) \
	tg.create_thread(bind(&thread_create_heap_dll, col_id, &eid));\
	tg.join_all();

#define DLL_SIMPLE_INSERT_DATA(eid, data, row, insert_sta) \
	tg.create_thread(bind(&thread_insert_dll, eid, data, row, &insert_sta));\
	tg.join_all();

#define DLL_SIMPLE_UPDATE_DATA(eid, det_data, det_len, src_data, src_len, sta) \
	tg.create_thread(bind(&thread_update_dll, eid, det_data, det_len, src_data, src_len, &sta));\
	tg.join_all();

#define DLL_SIMPLE_DELETE_DATA(eid, det_data, det_len, sta) \
	tg.create_thread(bind(&thread_delete_dll, eid, det_data, det_len, &sta));\
	tg.join_all();

/*#define DLL_SIMPLE_INSERT_DATA_WITH_INDEX(hir, unique_check, data, data_len) \
	param = get_param();\
	SAVE_PARAM(param);\
	tg.create_thread(bind(&thread_insert_with_index, data, data_len, ARRAY_LEN_CALC(data), &hir, param, unique_check));\
	tg.join_all();

#define DLL_SIMPLE_DROP_HEAP(relid, drop_sta) \
	param = get_param();\
	SAVE_PARAM(param);\
	tg.create_thread(bind(&thread_heap_drop, relid, param, &drop_sta));\
	++relid; \
	tg.join_all();

#define DLL_SIMPLE_CREATE_INDEX(heap_id, ind_id, heap_colinfo, ind_colinfo, create_sta) \
	param = get_param();\
	SAVE_PARAM(param);\
	tg.create_thread(bind(&thread_index_create, heap_id, heap_colinfo, ind_colinfo, ind_id, false, param, &create_sta));\
	tg.join_all();

#define DLL_SIMPLE_DROP_INDEX(heap_id, ind_id, drop_sta) \
	param = get_param();\
	SAVE_PARAM(param);\
	tg.create_thread(bind(&thread_index_drop, heap_id, ind_id, param, &drop_sta));\
	++ind_id; \
	tg.join_all();*/

#define DLL_PREPARE_TEST() \
	using namespace std; \
	using namespace FounderXDB::StorageEngineNS; \
	using namespace boost; \
	thread_group tg;

#define DLL_GET_THREAD_GROUP() tg

class SpliterGenerater
{
	/*============= public =============*/
public:
	SpliterGenerater();
	virtual ~SpliterGenerater();
	ColumnInfo* buildHeapColInfo(const int col_count, ...);
	ColumnInfo* buildIndexColInfo(const int col_count, 
		const int *col_number, 
		const CompareCallbacki *cmp_func_array,
		const Spliti split_func);
public:
	/*
	* 定义五个模板函数，能将索引分为最多5个列
	*/
	template<int heap_col_1>
	static
	void index_split_to_any(RangeDatai &rd, const char *str, int col, size_t data_len)
	{

		rd.start = 0;
		rd.len = SpliterGenerater::m_vc_col_array[heap_col_1 - 1];
		return ;
	}

	template<int heap_col_1, int heap_col_2>
	static
	void index_split_to_any(RangeDatai &rd, const char *str, int col, size_t data_len)
	{

		int col_array[] = {heap_col_1, heap_col_2};
		for(int i = 0; i < col; ++i)
		{
			rd.start += rd.len;
			rd.len = SpliterGenerater::m_vc_col_array[col_array[i] - 1];
		}
		return;
	}

	template<int heap_col_1, int heap_col_2, int heap_col_3>
	static
	void index_split_to_any(RangeDatai &rd, const char *str, int col, size_t data_len)
	{

		int col_array[] = {heap_col_1, heap_col_2, heap_col_3};
		for(int i = 0; i < col; ++i)
		{
			rd.start += rd.len;
			rd.len = SpliterGenerater::m_vc_col_array[col_array[i] - 1];
		}
		return rd;
	}

	template<int heap_col_1, int heap_col_2, int heap_col_3, int heap_col_4>
	static
	void index_split_to_any(RangeDatai &rd, const char *str, int col, size_t data_len)
	{

		int col_array[] = {heap_col_1, heap_col_2, heap_col_3, heap_col_4};
		for(int i = 0; i < col; ++i)
		{
			rd.start += rd.len;
			rd.len = SpliterGenerater::m_vc_col_array[col_array[i] - 1];
		}
		return;
	}

	template<int heap_col_1, int heap_col_2, int heap_col_3, int heap_col_4, int heap_col_5>
	static
	void index_split_to_any(RangeDatai &rd, const char *str, int col, size_t data_len)
	{

		int col_array[] = {heap_col_1, heap_col_2, heap_col_3, heap_col_4, heap_col_5};
		for(int i = 0; i < col; ++i)
		{
			rd.start += rd.len;
			rd.len = SpliterGenerater::m_vc_col_array[col_array[i] - 1];
		}
		return;
	}

public:
	static std::vector<int> m_vc_col_array;

	/*============= private =============*/
private:
	SpliterGenerater(const SpliterGenerater&);

private:
	ColumnInfo* m_pheap_cf;
	ColumnInfo* m_pindex_cf;
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

int get_col_id();
/**************************************************************************
class EntrySetCreator
* 创建并打开一个由EntrySetT指定类型的EntrySet,EntrySet是EntrySetCollInfo的实例、
* 例如EntrySetCreator<EntrySetCollInfo<3,8,2,4,9> >会创建一个具有5列(每列的长
* 度分别为3,8,2,4,9)的表.这个类本来应该是个函数，但是模板函数的模板参数不能有默认
* 值，所以写成了类
* Detailed description.
**************************************************************************/
template<typename EntrySetT>
class EntrySetCreator
{
public:
	static EntrySet* createAndOpen(Transaction *pTrans,const std::string& /*name*/)
	{
		EntrySetID id = StorageEngine::getStorageEngine()->createEntrySet(pTrans
			,EntrySetT::get());

		EntrySet* pEntrySet = (EntrySet *)StorageEngine::getStorageEngine()->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,id);
		return pEntrySet;
	}
};


/**************************************************************************
*class IndexCreator
* @brief 
* 用来创建在指定类型表上创建和打开索引的类，本来应该是个函数，可是模板函数参数不能不默认值，所以写成了类
* 例: IndexCreator<EntrySetT,1,4,8>::get() 会返回一个可以EntrySetT的第1列，第4列和第8列建立组合索引
* Detailed description.
**************************************************************************/
template<typename EntrySetT,BOOST_PP_ENUM(TIMES,NN,~)>
class IndexCreator
{
public:
	static IndexEntrySet* createAndOpen(Transaction *pTrans,EntrySet * pEntrySet,const std::string& /*name*/)
	{
		EntrySetID idIndex = StorageEngine::getStorageEngine()->createIndexEntrySet(pTrans
			,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,IndexCollInfo<EntrySetT,BOOST_PP_ENUM_PARAMS(TIMES,N)>::get());

		IndexEntrySet *pIndex = (IndexEntrySet *)StorageEngine::getStorageEngine()->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,idIndex);
		return pIndex;
	}
};

void MySleep(long microsec );

extern bool check_test_result(vector<string> *v_data, EntrySetID relid, EntrySetID idxid, vector<string> &v_key);
#endif //TEST_UTILS_DLL_H
