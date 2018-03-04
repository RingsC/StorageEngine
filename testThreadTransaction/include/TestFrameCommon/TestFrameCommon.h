#ifndef __TEST_FRAME_COMMON_H__
#define __TEST_FRAME_COMMON_H__

#include "Transaction.h"
#include "EntrySet.h"
#include "PGSETypes.h"
#include "LargeObject.h"

class ParamBase
{
public:
	friend struct TaskInfo;
	ParamBase():m_Trans(NULL),m_FlagCommit(true){}
	virtual ~ParamBase(){}
	//测试用例调用
	inline FounderXDB::StorageEngineNS::Transaction* GetTransaction(){return m_Trans;}
	inline void SetSuccessFlag(bool flag) {m_FlagCommit = flag;}
	ParamBase(const ParamBase &other);
    /// overload operator =
    ParamBase& operator = (const ParamBase &other);
	
private:
	//测试框架调用
	void begin();
	void commit();
	void abort();
	void SetTranState();
private:
	bool m_FlagCommit;
	FounderXDB::StorageEngineNS::FXTransactionId m_TransID;
    FounderXDB::StorageEngineNS::Transaction* m_Trans;
};

class ParamIndex: public ParamBase
{
public:
	ParamIndex():m_nHeapEntryID(0),m_nIndexEntryID(0),m_nIndexEntryID2(0),m_nIndexEntryID3(0){}
	virtual ~ParamIndex(){}
	ParamIndex(const ParamIndex &other);
    /// overload operator =
    ParamIndex& operator = (const ParamIndex &other);
	
public:
	//记录要在事务内部传递的值
	std::vector<FounderXDB::StorageEngineNS::EntryID>	m_vEntryID;
	FounderXDB::StorageEngineNS::EntrySetID m_nHeapEntryID;
	FounderXDB::StorageEngineNS::EntrySetID m_nIndexEntryID;
	FounderXDB::StorageEngineNS::EntrySetID m_nIndexEntryID2;
	FounderXDB::StorageEngineNS::EntrySetID m_nIndexEntryID3;
};


class ParamLOControler : public ParamBase
{
public :
	ParamLOControler();
	virtual ~ParamLOControler() ;

public:
	FounderXDB::StorageEngineNS::LargeObject *getLO();
	void pushLO(FounderXDB::StorageEngineNS::LargeObject *lo);
	void pushFileDir(const std::string dir);
	void pushRFileDir(const std::string dir);
	std::string getFileDir();
	std::string getRFileDir();
	void setWriteSize(unsigned int size);
	unsigned int getWriteSize();
	unsigned int countTask();
	unsigned int getBuffer(char *buf);
	void setFileToWrite();
	void writeBuffer(char *buf, unsigned int size);
	bool eof();
	void close();

private:
	FounderXDB::StorageEngineNS::LargeObject *m_pLO;
	std::string m_sLOFile, m_sLORFile;
	unsigned int m_iWriteSize;
	FILE *m_fd;
	int64 m_iFileSize;
};
#endif
