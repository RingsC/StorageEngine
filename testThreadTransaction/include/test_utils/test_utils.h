#ifndef _TEST_UTILS_H
#define _TEST_UTILS_H
#include <iostream>
#include <map>
#include <vector>
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include  <boost/preprocessor/repetition/enum.hpp>
#include <boost/assign.hpp>
#include "TestFrame/TestFrame.h"
#include "TestFrame/TestTask.h"
#include "TestFrameCommon/TestFrameCommon.h"
#include "PGSETypes.h"
#include "postgres.h"
#include "utils/rel.h"
#include "access/skey.h"
#include "StorageEngine.h"


using namespace FounderXDB::StorageEngineNS;

//�쳣ʱ����ʧ�ܲ���
#define EXCEPTION_CATCH_SET_FLAG(arg) \
	catch(StorageEngineException &ex)\
{\
	std::cout << ex.getErrorNo() << std::endl;\
	std::cout << ex.getErrorMsg() << std::endl;\
	arg->SetSuccessFlag(false);\
	return false;\
}\

int str_compare(const char *str1, size_t len1, const char *str2, size_t len2);

class MyColumnInfo
{
public:
	MyColumnInfo(const std::map<int,CompareCallbacki>& mapCompare,Spliti split);
	~MyColumnInfo();
	ColumnInfo& Get(void){return m_columnInfo;}
private:
	ColumnInfo m_columnInfo;
};


class FixSpliter
{
public:
	explicit FixSpliter(const std::vector<int> &vec);
	static void split(RangeDatai& rangeData, const char *pszNeedSplit,int iIndexOfColumn, size_t len=0);
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
static int used_to_remove_waring = 0;
template<BOOST_PP_ENUM(TIMES,NN,~)>
class StaticSpliter
{
public:

	static void split(RangeDatai& rangeData, const char *,int iIndexOfColumn, size_t len = 0)
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

#define DATA_LEN 1024

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
		if(pos >= m_row_num)
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

//����һ�����б�ID�Ļ���
class HeapBase :public ParamBase
{
public:
	HeapBase() : isMultiCol(false){}
	EntrySetID entrysetId;
	bool isMultiCol;
	std::string strCaseName;
};

/************************************************************************** 
* @brief varSplit 
* �����ɱ䳤�����Ե�split����,����֮����\000�ָ�
* Detailed description.
* @param[in] pszNeedSplit ���ָ����ַ�����
* @param[in] iIndexOfColumn ����ţ���1��ʼ
* @return RangeDatai  ���ص�iIndexOfColumn����ʼλ�úͳ���
**************************************************************************/
void VarSplit(RangeDatai& rangeData, const char *pszNeedSplit,int iIndexOfColumn, size_t len = 0);

/************************************************************************** 
* @brief RandomGenString 
* �������nLen������ַ���
* Detailed description.
* @param[in] s 
* @param[in] nLen 
**************************************************************************/
void RandomGenString(std::string& strLine,size_t nLen);

/************************************************************************** 
* @brief GetDataFromEntrySet 
* ����EntrySet�����е�����
* Detailed description.
* @param[out] sResult ����ֵ
* @param[in] pTrans  ��ǰ����
* @param[in] pEntrySet �������ڵ�EntrySet
**************************************************************************/
void GetDataFromEntrySet(std::vector<std::string>& sResult,Transaction * pTrans,EntrySet *pEntrySet);

/************************************************************************** 
* @brief GetIndexScanResults 
* ʹ��pIndex��ѯ��������������ֵ
* Detailed description.
* @param[out] sResult ���ز�ѯ���
* @param[in] pTrans ��ǰ����
* @param[in] pIndex ʹ�õ�index
* @param[in] vConditions ��ѯ����
**************************************************************************/
void GetIndexScanResults(std::vector<std::string>& sResult
						 ,Transaction * pTrans 
						 ,IndexEntrySet * pIndex
						 ,std::vector<ScanCondition>& vConditions) ;



uint32 GetSingleColInfo();

/************************************************************************** 
* @brief GetMultiCollInfo 
* ����һ������(��1��3���ַ�����2��2���ַ�����3��1���ַ�)
* Detailed description.
* @return uint32  
**************************************************************************/
uint32 GetMultiCollInfo();


/************************************************************************** 
* @brief InsertData 
* Help Function :��vData�е����ݲ��뵽pEntrySet,���ض����ݵ�EntryID
* Detailed description.
* @param[in] pTrans   ��ǰ����
* @param[in] pEntrySet �����������ݵı�
* @param[in] vData �����������
* @param[out] pvIds Defaults to NULL. �����Ժ���vData��Ӧ��entry id
**************************************************************************/
void InsertData(Transaction * pTrans 
				,EntrySet *pEntrySet
				,const std::vector<std::string>& vData
				,std::vector<EntryID>* pvIds = NULL);
void UpdateData(Transaction * pTrans ,EntrySet *pEntrySet ,const std::map<EntryID ,std::string>& vData);
void DeleteData(Transaction * pTrans ,EntrySet *pEntrySet ,const std::vector<EntryID>& vIds);
void command_counter_increment();

/************************************************************************** 
* @brief CompareVector 
* �Ƚ�lhs��rhs�е������Ƿ��Ӧ���
* Detailed description.
* @param[in] lhs 
* @param[in] rhs 
* @return bool  ���lhs��rhs�е����ݶ�Ӧ��ȷ���true,���򷵻�false.
**************************************************************************/
bool CompareVector(std::vector<std::string>& lhs,std::vector<std::string>& rhs);
void MySleep(long millsec);
int GetTransWaitId();
#endif//_TEST_UTILS_H
