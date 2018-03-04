#include <fstream>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include "test_utils/test_utils.h"


extern uint32 EntryColId = 65535;
extern uint32 IndexId = 65536;
extern uint32 VarColId = 65537;

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::setw;

using namespace FounderXDB::StorageEngineNS;
MyColumnInfo::MyColumnInfo(const std::map<int,CompareCallbacki>& mapCompare,Spliti split) : m_columnInfo(false)
{
	m_columnInfo.keys = mapCompare.size();

	if (0 != m_columnInfo.keys)
	{
		m_columnInfo.col_number = new size_t[m_columnInfo.keys];
		m_columnInfo.rd_comfunction = new CompareCallbacki[m_columnInfo.keys];
	}

	m_columnInfo.split_function = split;
	int idx = 0;
	BOOST_FOREACH(BOOST_TYPEOF(*mapCompare.begin()) comp,mapCompare)
	{
		m_columnInfo.col_number[idx] = comp.first;
		m_columnInfo.rd_comfunction[idx++] = comp.second;
	}
}

MyColumnInfo::~MyColumnInfo()
{
	delete []m_columnInfo.col_number;
	m_columnInfo.col_number = NULL;
	delete []m_columnInfo.rd_comfunction;
	m_columnInfo.rd_comfunction = NULL;
}

std::vector<int> FixSpliter::g_vecOfSplitPos;
FixSpliter::FixSpliter(const std::vector<int> &vec)
{
	g_vecOfSplitPos.clear();
	for (std::vector<int>::const_iterator it = vec.begin();
		it != vec.end();
		++it)
	{
		g_vecOfSplitPos.push_back(*it + (g_vecOfSplitPos.size() > 0 ? g_vecOfSplitPos.back() : 0));
	}
}

void FixSpliter::split(RangeDatai& rangeData, const char *pszNeedSplit,int iIndexOfColumn, size_t len)
{
	pszNeedSplit;
	assert(iIndexOfColumn >= 1 && iIndexOfColumn <= (int)g_vecOfSplitPos.size());

	if (iIndexOfColumn > 1)
	{
		rangeData.start = g_vecOfSplitPos[iIndexOfColumn - 2];
		rangeData.len = g_vecOfSplitPos[iIndexOfColumn - 1] -  g_vecOfSplitPos[iIndexOfColumn - 2];
	}
	else
	{
		rangeData.len = g_vecOfSplitPos[iIndexOfColumn - 1];
	}
}


SearchCondition::SearchCondition():
m_nCount(0)
{

}

SearchCondition::~SearchCondition()
{

}

void SearchCondition::Add(int nColumn,EScanOp op,const char* pszValue,int (*compare)(const char *, size_t , const char *, size_t))
{
	m_vConditions.push_back(ScanCondition(nColumn,GetOption(op),(se_uint64)pszValue,std::strlen(pszValue),compare));
	++m_nCount;
}

std::ostream& operator<<(std::ostream& os,const SearchCondition& search)
{
	const char aOp[6][16] = {
		"<"
		,"<="
		,"=="
		,">="
		,">"
	};
	std::vector<ScanCondition>::const_iterator iter=search.m_vConditions.begin();
	for(; iter!=search.m_vConditions.end(); ++iter)
	{
		cout<<iter->fieldno<<setw(4)<<aOp[iter->compop-1]
			<<setw(4)<<std::string((const char*)iter->argument,iter->arg_length);
	}
	return os;
}

ScanCondition::CompareOperation SearchCondition::GetOption(EScanOp op)
{
	switch (op)
	{
	case LessThan:
		return ScanCondition::LessThan;
	case LessEqual:
		return ScanCondition::LessEqual;
	case Equal:
		return ScanCondition::Equal;
	case GreaterEqual:
		return ScanCondition::GreaterEqual;
	case GreaterThan:
		return ScanCondition::GreaterThan;
	default:
		return ScanCondition::InvalidOperation;
	}
}

bool readfromfile = true;
void RandomGenString(std::string& strLine,size_t nLen)
{
	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::ifstream inFile(szFileName);
		if (inFile.is_open())
		{
			inFile.seekg(0,std::ios::end);
			size_t length = inFile.tellg();
			inFile.seekg(0,std::ios::beg);
			boost::shared_ptr<char> psz(new char[length]);
			inFile.read(psz.get(),length);
			strLine.append(psz.get(),length);
			return;
		}
	}

	std::string s;
	srand((unsigned)time( NULL ));
	size_t nGenerated = 0;
	for (nGenerated = 0;nGenerated < 10;++nGenerated)
	{
		char c = 0;
		s += c;
	}
	for (nGenerated = 10; nGenerated < nLen - 1; ++nGenerated)
	{
		char c = (rand()) % 26;
		s += 'a' + c;
	}
	char c = 0;
	s += c;
	strLine.append(s);

	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::fstream outFile(szFileName,std::ios_base::out);
		if(outFile.is_open())
		{
			outFile<<s;
		}
	}

}

void GetIndexScanResults(std::vector<std::string>& sResult,Transaction * pTrans ,IndexEntrySet * pIndex, std::vector<ScanCondition>& vConditions) 
{
	IndexEntrySetScan *pScan = (IndexEntrySetScan *)(pIndex->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vConditions));

	DataItem outData;
	EntryID outId;
	while (0 == pScan->getNext(EntrySetScan::NEXT_FLAG,outId,outData))
	{
		sResult.push_back(std::string((char*)outData.getData(),outData.getSize()));
	}
	pIndex->endEntrySetScan(pScan);
}

uint32 GetSingleColInfo()
{
	static uint32 colid = 0;

	if (0 == colid)
	{
		colid = 1241234;
		using namespace boost::assign;
		std::map<int,CompareCallbacki> mapComp;
		insert(mapComp)(1,str_compare);

		std::vector<int> v;
		v.push_back(1);
		FixSpliter spliter(v);
		static MyColumnInfo columnInfo(mapComp,FixSpliter::split);

		setColumnInfo(colid,&columnInfo.Get());
	}

	return EntrySetCollInfo<1>::get();
}

uint32 GetMultiCollInfo()
{
	static uint32 colid = 0;

	if (0 == colid)
	{
		colid = 32589;
		using namespace boost::assign;
		std::map<int,CompareCallbacki> mapComp;
		insert(mapComp)(1,str_compare)(2,str_compare)(3,str_compare);

		std::vector<int> v;
		v += 3,2,1;
		FixSpliter spliter(v);
		static MyColumnInfo columnInfo(mapComp,FixSpliter::split);

		setColumnInfo(colid,&columnInfo.Get());
	}

	return EntrySetCollInfo<3,2,1>::get();
}
void GetDataFromEntrySet(std::vector<std::string>& sResult,Transaction * pTrans,EntrySet *pEntrySet)
{
	std::vector<ScanCondition> vConditions;
	EntrySetScan *pScan = (EntrySetScan*)(pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,vConditions));

	DataItem outData;
	EntryID outId;
	while (0 == pScan->getNext(EntrySetScan::NEXT_FLAG,outId,outData))
	{
		sResult.push_back(std::string((char*)outData.getData(),outData.getSize()));
	}

	pEntrySet->endEntrySetScan(pScan);
}

void InsertData(Transaction * pTrans 
				,EntrySet *pEntrySet
				,const std::vector<std::string>& vData
				,std::vector<EntryID>* pvIds)
{
	for (size_t iPos = 0; iPos < vData.size(); ++iPos)
	{
		DataItem data;
		data.setData((void*)vData[iPos].c_str());
		data.setSize(vData[iPos].length());
		EntryID eid;
		pEntrySet->insertEntry(pTrans,eid,data);

		if (NULL != pvIds)
		{
			pvIds->push_back(eid);
		}
	}
	command_counter_increment();
}

void DeleteData(Transaction * pTrans
				,EntrySet *pEntrySet
				,const std::vector<EntryID>& vIds)
{
	BOOST_FOREACH(BOOST_TYPEOF(*vIds.begin()) id,vIds)
	{
		pEntrySet->deleteEntry(pTrans,id);
	}
	command_counter_increment();
}
void UpdateData(Transaction * pTrans
				,EntrySet *pEntrySet
				,const std::map<EntryID
				,std::string>& vData)
{
	std::map<EntryID,std::string>::const_iterator iter;
	for(iter=vData.begin(); iter!=vData.end(); ++iter)
	{
		DataItem data;
		data.setData((void*)iter->second.c_str());
		data.setSize(iter->second.length());
		pEntrySet->updateEntry(pTrans,iter->first,data); 
	}
	command_counter_increment();
}

int str_compare(const char *str1, size_t len1, const char *str2, size_t len2)
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

void command_counter_increment()
{
	StorageEngine::getStorageEngine()->endStatement();
}

bool CompareVector(std::vector<std::string>& lhs,std::vector<std::string>& rhs)
{
	if (lhs.size() == rhs.size())
	{
		for (size_t iPos = 0; iPos < lhs.size(); ++iPos)
		{
			if(lhs[iPos] != rhs[iPos])
			{
				return false;
			}
		}
		return true;
	}
	return false;
}
DataGenerater::DataGenerater(const unsigned int row_num, const unsigned int data_len = DATA_LEN)
{
    int i = 0;
    //如果两次调用构造函数的时间间隔很短，该
    //循环可以避免产生相同的时间
    while(i++ < 1000 * 1000);
    srand((unsigned int)time(NULL));
    this->m_row_num = row_num;
    this->m_data_len = data_len;
    this->m_size = row_num;
    m_rand_data = NULL;
    m_rand_data = (char*)malloc(row_num * data_len);
	memset(m_rand_data, 0, row_num * data_len);
}

DataGenerater::DataGenerater(const DataGenerater& dg)
{
    m_row_num = dg.m_row_num;
    m_size = dg.m_size;
    m_data_len = dg.m_data_len;
    m_rand_data = (char*)malloc(m_row_num * m_data_len);
    memcpy(m_rand_data, dg.m_rand_data, m_row_num * m_data_len);
}

DataGenerater::~DataGenerater()
{
    free(m_rand_data);
    m_rand_data = NULL;
}

void DataGenerater::dataGenerate()
{	
	char *p = (char *)malloc(m_data_len);
    for(unsigned int j = 0; j < this->m_row_num; ++j)
    {
		memset(p, 0, m_data_len);
        for(unsigned int i = 0; i < m_data_len - 1; i++)
        {
            p[i] = (char)(rand() % 95 + 32);
        }
        p[m_data_len - 1] = '\0';
        memcpy(m_rand_data + j * m_data_len, p, m_data_len);
    }
	free(p);
}

void DataGenerater::dataGenerate(const char *src_data, const int data_len, int pos)
{
    if(m_size == m_row_num)
    {
        m_size = 0;
    }
    if(pos >= m_row_num)
    {
        space_increment();
    }
    memcpy(m_rand_data + pos * m_data_len, src_data, data_len);
    ++m_size;
}

void DataGenerater::dataToDataArray2D(char data[][DATA_LEN])
{
    m_row_num = m_size;
	memset(data, 0, m_row_num * DATA_LEN);
    for(unsigned int i = 0; i < m_row_num; ++i)
    {
        memcpy(data[i], m_rand_data + i * m_data_len, m_data_len);
    }
}

void DataGenerater::dataToDataArray2D(const int size, char data[][DATA_LEN])
{
    for(unsigned int i = 0; i < size; ++i)
    {
        memcpy(data[i], m_rand_data + i * m_data_len, m_data_len);
    }
}

void DataGenerater::space_increment()
{
    char *tmp_data = NULL;
    tmp_data = (char*)malloc(m_row_num * m_data_len);
    memcpy(tmp_data, m_rand_data, m_row_num * m_data_len);
    free(m_rand_data);
    m_rand_data = (char*)malloc((m_row_num + 10) * m_data_len);
    memcpy(m_rand_data, tmp_data, m_row_num * m_data_len);
    m_row_num += 10;
    free(tmp_data);
}

void MySleep(long millsec)
{
	boost::thread::sleep(boost::get_system_time() + boost::posix_time::milliseconds(millsec));
}

int GetTransWaitId()
{
	static int waitbase = 200;
	return ++waitbase;
}