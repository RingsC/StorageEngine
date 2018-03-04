#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include "StorageEngine.h"
#include "TestFrameCommon/TestFrameCommon.h"
#include "StorageEngineException.h"

using namespace std;
using namespace FounderXDB::StorageEngineNS;

extern StorageEngine *pStorageEngine;
extern boost::mutex gprintMutex;
void ParamBase::begin()
{
	try
	{
		FXTransactionId invalid_transaction = ((TransactionId) 0);
		m_Trans = pStorageEngine->getTransaction(invalid_transaction, Transaction::READ_COMMITTED_ISOLATION_LEVEL);//获得一个事务
		if (NULL != m_Trans)
		m_TransID = m_Trans->getTransactionId();
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
	}
}

void ParamBase::commit()
{
	try
	{
		m_Trans->commit();//提交事务	
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		m_Trans->abort();
	}
	delete m_Trans;
	m_Trans = NULL;
}

void ParamBase::abort()
{
	if (NULL != m_Trans)
	{
		m_Trans->abort();//取消事务
		delete m_Trans;
		m_Trans = NULL;
	}
}

ParamBase::ParamBase(const ParamBase &other)
{
	m_FlagCommit = other.m_FlagCommit;
	m_TransID = other.m_TransID;
	m_Trans = other.m_Trans;
}

/// overload operator =
ParamBase& ParamBase::operator = (const ParamBase &other)
{
	m_FlagCommit = other.m_FlagCommit;
	m_TransID = other.m_TransID;
	m_Trans = other.m_Trans;
	return *this;
}

void ParamBase::SetTranState()
{
	try
	{
		m_Trans = pStorageEngine->getTransaction(m_TransID, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
	}
}



ParamIndex::ParamIndex(const ParamIndex &other)
{
	m_vEntryID = other.m_vEntryID;
	m_nHeapEntryID = other.m_nHeapEntryID;
	m_nIndexEntryID = other.m_nIndexEntryID;
	m_nIndexEntryID2 = other.m_nIndexEntryID2;
	m_nIndexEntryID3 = other.m_nIndexEntryID3;
}

/// overload operator =
ParamIndex& ParamIndex::operator = (const ParamIndex &other)
{
	m_vEntryID = other.m_vEntryID;
	m_nHeapEntryID = other.m_nHeapEntryID;
	m_nIndexEntryID = other.m_nIndexEntryID;
	m_nIndexEntryID2 = other.m_nIndexEntryID2;
	m_nIndexEntryID3 = other.m_nIndexEntryID3;
	return *this;
}

LargeObject * ParamLOControler::getLO()
{
	return m_pLO;
}

void ParamLOControler::pushLO(LargeObject *lo)
{
	m_pLO = lo;
}

void ParamLOControler::pushFileDir(const std::string dir)
{
	m_sLOFile = dir;
}

void ParamLOControler::pushRFileDir(const std::string dir)
{
	m_sLORFile = dir;
}

std::string ParamLOControler::getFileDir()
{
	return m_sLOFile;
}

std::string ParamLOControler::getRFileDir()
{
	return m_sLORFile;
}

void ParamLOControler::setWriteSize(unsigned int size)
{
	m_iWriteSize = size;
}

unsigned int ParamLOControler::getWriteSize()
{
	return m_iWriteSize;
}

unsigned int ParamLOControler::countTask()
{
	extern int64 myftell(FILE *);
	extern int64 myfseek(FILE *fd, int64 offset, int org);

	assert(m_iWriteSize > 0 && m_sLOFile.length() > 0);

	FILE *fd = fopen(m_sLOFile.c_str(), "rb+");

	assert(fd != NULL);
	
	myfseek(fd, 0, SEEK_END);

	int64 fileSize = myftell(fd);

	assert(fileSize > m_iWriteSize);

	fclose(fd);

	double retCount = ((double)fileSize) / m_iWriteSize;
	unsigned int i_count = retCount;

	if(retCount == i_count)
		retCount = i_count;
	else
		retCount = i_count + 1;

	return retCount;
}

bool ParamLOControler::eof()
{
	extern int64 myftell(FILE *);

	if(m_fd == NULL)
		return true;

	return (myftell(m_fd) >= m_iFileSize);
}

void ParamLOControler::close()
{
	if(m_fd != NULL)
		fclose(m_fd);

	m_fd = NULL;
}

void ParamLOControler::setFileToWrite()
{
	assert(m_fd == NULL);

	m_fd = fopen(m_sLOFile.c_str(), "wb+");

	assert(m_fd != NULL);
}


void ParamLOControler::writeBuffer(char *buf, unsigned int size)
{
	if(m_fd == NULL)
	{
		assert(m_sLORFile.length() > 0);

		m_fd = fopen(m_sLORFile.c_str(), "wb+");

		assert(m_fd != NULL);
	}

	fwrite(buf, size, 1, m_fd);
}

unsigned int ParamLOControler::getBuffer(char *buf)
{
	extern int64 myftell(FILE *);
	extern int64 myfseek(FILE *fd, int64 offset, int org);

	if(m_fd == NULL)
	{
		assert(m_sLOFile.length() > 0);

		m_fd = fopen(m_sLOFile.c_str(), "rb+");
		
		assert(m_fd != NULL);

		myfseek(m_fd, 0, SEEK_END);
		m_iFileSize = myftell(m_fd);
		myfseek(m_fd, 0, SEEK_SET);
	}

	assert(buf != NULL);

	int64 next_ = myftell(m_fd);
	unsigned int write = 0;

	if(m_iFileSize - next_ >= m_iWriteSize)
	{
		write = m_iWriteSize;
	} else {
		write = m_iFileSize - next_;
	}

	if(feof(m_fd))
		return 0;
	
	fread(buf, write, 1, m_fd);

	return write;
}

ParamLOControler::ParamLOControler() : ParamBase()
{
	m_fd = NULL;
	m_pLO = NULL;
	m_iWriteSize = 0;
	m_iFileSize = 0;
	m_sLOFile = "";
}

ParamLOControler::~ParamLOControler()
{
	if(m_fd != NULL)
		fclose(m_fd);
}
