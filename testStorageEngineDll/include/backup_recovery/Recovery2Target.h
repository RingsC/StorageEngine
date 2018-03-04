/**
* @file BRTest.h
* @brief 
* @author ¿Ó È‰∆
* @date 2012-3-06 9:23:47
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _RECOVERY2TARGET_H
#define _RECOVERY2TARGET_H
#include <boost/thread.hpp>
#include <set>
namespace FounderXDB
{
	namespace StorageEngineNS 
	{
		class EntrySet;
		class Transaction;
	}
}
using namespace FounderXDB::StorageEngineNS;
class StorageData
{
public:
	StorageData(const std::string& strFile,EntrySet* pEntrySet = NULL,Transaction* pTrans = NULL);
	~StorageData( void );

	void Load(std::string const & strFile);
	void Save(std::string const & strFile);
	void InsertData(const std::string& strData,EntrySet* pEntrySet = NULL,Transaction* pTrans = NULL);
	void UpdateData(const std::string& strFrom,const std::string& strTo,EntrySet* pEntrySet = NULL,Transaction* pTrans = NULL);
	void DeleteData(const std::string& strDelete,EntrySet* pEntrySet = NULL,Transaction* pTrans = NULL);

	bool CheckIsSame( void );

private:
	Transaction* GetTransaction(Transaction *pTrans) 
	{
		return NULL == pTrans ? m_pTrans : pTrans;
	}

	EntrySet* GetEntrySet(EntrySet* pEntrySet)
	{
		return NULL == pEntrySet ? r_pEntrySet : pEntrySet;
	}

	std::set<std::string> m_setDatas;
	EntrySet* r_pEntrySet;
	boost::mutex m_mutex;
	Transaction* m_pTrans;
};

class PharseFile
{
public:
	PharseFile(const std::string& strFile);
	~PharseFile( void );

	int Get( void )const { return m_nPharse;}

	void SetEntrySetId(unsigned int idEntrySet) { m_idEntrySet = idEntrySet;}
	unsigned int GetEntrySetID( void ) { return m_idEntrySet;}
private:
	std::string m_strPath;
	unsigned int  m_idEntrySet;
	int m_nPharse;
	static const int TOTAL_PHARSE = 6;
};
bool CreateOnTable(unsigned int& id );    //time1
bool DoBaseBackup( unsigned int id );     //time2 
bool MakeMoreChanges( unsigned int id );  //time3
bool Recovery2Time2( unsigned int id );
bool MakeSomeChanges(unsigned int id);
bool Recovery2Time3( unsigned int id );


bool TestRecovery2Target( void );
void ShowEntrySetData(Transaction* pTran, EntrySet* pEntrySet);
#endif
