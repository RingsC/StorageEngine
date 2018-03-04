#ifndef _PG_UTILS_DLL_H
#define _PG_UTILS_DLL_H
#include <map>
#include <vector>
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include  <boost/preprocessor/repetition/enum.hpp>
#include <boost/timer.hpp>
#include <boost/assign.hpp>
#include "StorageEngine.h"
#include "PGSETypes.h"
using namespace FounderXDB::StorageEngineNS;

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;


#define CATCHEXCEPTION \
	catch(StorageEngineException &ex)\
{\
	user_abort_transaction();\
	cout << ex.getErrorNo() << endl;\
	cout << ex.getErrorMsg() << endl;\
	return false;\
}\

#define HEAP_RETURN_FALSE \
	penry_set->endEntrySetScan(entry_scan);\
	pStorageEngine->closeEntrySet(pTransaction,penry_set);\
	user_abort_transaction();\
	return false;

#define ENDSCAN_CLOSE_COMMIT \
	penry_set->endEntrySetScan(entry_scan);\
	pStorageEngine->closeEntrySet(pTransaction,penry_set);\
	commit_transaction();\

void commit_transaction();
void user_abort_transaction();
void get_new_transaction();
void command_counter_increment();

bool checkeid(EntryID eid);
bool check_status(int status);
bool heap_insert(EntrySetID heapId , int data_amount , int data_size , vector<EntryID> &data_eid);

void thread_heap_create(int *status);
void thread_heap_remove(EntrySetID heapId, int *status);
void thread_heap_insert(EntrySetID heap_id , int data_amount , int data_size , int *status ,vector<EntryID>&data_eid);
void thread_heap_delete(int *status , int thread_id , vector<vector<EntryID>> & data_eid);
void thread_heap_update(int *status , int thread_id ,  vector<vector<EntryID>> & data_eid);
Transaction* start_new_transaction(Transaction::IsolationLevel   level);

void initKeyVector(vector<ScanCondition> &keyVec,int col_number, ScanCondition::CompareOperation cmp_op,const char *keydata,int (*compare_func)(const char *, size_t , const char *, size_t) );

extern ColumnInfo heap_colinfo;
extern ColumnInfo index_colinfo;

extern void form_heap_colinfo(ColumnInfo &colinfo);
int compare_str(const char *str1, size_t len1, const char *str2, size_t len2);

/************************************************************************** 
* @brief GetSingleColInfo 
* ����һ������ʹ�õ�columnInfo��Ӧ��id.
* Detailed description.
* @return uint32  
**************************************************************************/
uint32 GetSingleColInfo();


/************************************************************************** 
* @brief GetMultiCollInfo 
* ����һ������(��1��3���ַ�����2��2���ַ�����3��1���ַ�)
* Detailed description.
* @return uint32  
**************************************************************************/
uint32 GetMultiCollInfo();

/**************************************************************************
* struct TimerFixture
* ����ʱ��
* Detailed description.
**************************************************************************/
struct TimerFixture
{
	TimerFixture()
	{
#ifndef WIN32
		gettimeofday(&startTime,NULL);
#endif
	}

	~TimerFixture()
	{
#ifdef WIN32
		std::cout<<" cost "<<m_time.elapsed()<<std::endl;
#else
		const int RATE = 1000000L;
		gettimeofday(&endTime,NULL);
		double microsec = (endTime.tv_sec - startTime.tv_sec) * RATE + (endTime.tv_usec - startTime.tv_usec);
		std::cout<<" cost "<<(microsec / RATE)<<" seconds"<<std::endl;
#endif
	}
private:
#ifdef WIN32
	boost::timer m_time;
#else
	struct timeval startTime;
	struct timeval endTime;
#endif
};

inline int str_compare(const char *str1, size_t len1, const char *str2, size_t len2)
{
	size_t i = 0;
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
* ���������Եõ�һ��EntrySet��split�������ܹ��ж�����,����:StaticSpliter<3,2,8>
* ����size()�᷵��3,��Ϊ����3��,��������split������֤��һ����3���ַ����ڶ�����2��
* �ַ�����3����8���ַ�.
* Detailed description.
**************************************************************************/
template<BOOST_PP_ENUM(TIMES,NN,~)>
class StaticSpliter
{
public:

	static void split(RangeDatai& rangeData, const char *,int iIndexOfColumn, size_t len = 0)
	{
		static bool init = Init();
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
		std::cout<<"һ��"<<m_globalSize<<"��:"<<std::endl;
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

class MyColumnInfo
{
public:
	MyColumnInfo(const std::map<int,CompareCallbacki>& mapCompare,Spliti split);
	~MyColumnInfo();
	ColumnInfo& Get(void){return m_columnInfo;}
private:
	ColumnInfo m_columnInfo;
};

/**************************************************************************
* class EntrySetCollInfo
* ΪEntrySet����һ��columnInfo��id,����split������StaticSpliter���壬comp��
* ��Ϊstr_compare 
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
* ����һ��EntrySet,������Ҫ�����������У��õ�һ��ColumnInfo,����:
* EntrySetCollInfo<3,4,2,9> һ����4�У������ַ�����Ϊ3,4,2,9
* IndexCollInfo<EntrySetCollInfo<3,4,2,9>, 2,3>���Է���һ���ڵ�2�к͵�3����������ColumnInfo
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
* ����һ���ɱ䳤��ColumnInfo��Id
* Detailed description.
* @param[in] N0,N1...N9ָ������Щ���ϴ�������
* @return uint32  ColumnInfo��id. 
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

	void Add(int nColumn,EScanOp op,const string& str,int (*compare)(const char *, size_t , const char *, size_t));

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
* @brief RandomGenString 
*  ������������������ָ�����ȵĴ�����
* 
* @param[in/out] ���Է������ݣ����ɵ����ݻ���׷�ӵķ�ʽ����s����
* @param[in] nLen Ҫ���ɵ����ݳ���
**************************************************************************/
void RandomGenString(std::string& s,size_t nLen);

/**************************************************************************
class EntrySetCreator
* ��������һ����EntrySetTָ�����͵�EntrySet,EntrySet��EntrySetCollInfo��ʵ����
* ����EntrySetCreator<EntrySetCollInfo<3,8,2,4,9> >�ᴴ��һ������5��(ÿ�еĳ�
* �ȷֱ�Ϊ3,8,2,4,9)�ı�.����౾��Ӧ���Ǹ�����������ģ�庯����ģ�����������Ĭ��
* ֵ������д������
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
* ����������ָ�����ͱ��ϴ����ʹ��������࣬����Ӧ���Ǹ�����������ģ�庯���������ܲ�Ĭ��ֵ������д������
* ��: IndexCreator<EntrySetT,1,4,8>::get() �᷵��һ������EntrySetT�ĵ�1�У���4�к͵�8�н����������
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

#endif