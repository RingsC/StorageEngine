#ifndef _PGDUMP_H
#define _PGDUMP_H
#include <string>
#include <stdio.h>
#include "c.h"
#include "EntrySet.h"
namespace FounderXDB{
	namespace StorageEngineNS {
typedef unsigned char uint8;	/* == 8 bits */
typedef unsigned short uint16;	/* == 16 bits */
typedef unsigned int uint32;	/* == 32 bits */
struct DumpHeader 
{
	DumpHeader(TransactionId idTrans,EntrySetID entrySetId);
	uint32 magicNum; //used to identify dumpfile, should be DUMP
	uint8  version;  //current should be 1.0
	uint32 numTuples;
	time_t dumpWhen;
	TransactionId currentTransactionId;
	uint32 entrySetId;
	uint32 chksum;
};

class DumpFile
{
public:
	DumpFile( void );
	~DumpFile( void );
	void Open(const std::string& strFilePath,bool bWriteMode = false);
	void Write(int nOffset,int nSize,void* data,bool bSync = false);
	int Read(int nOffset,int nSize,void* buffer);
    void Flush( void );
private:
	void Init( void );
	char* GetBuffer(int nOffset,int& freeSpace,bool bSync = false);
	void WriteBuffer2Disk(int nCnt);

	static const int g_nBufferSize = 8192;
	static const int g_nBufferCnt = 128;
    
	struct BufferDesc
	{
	    BufferDesc( void )
			:dirty(false)
			,offset(-1)
		{}
		bool dirty;
		int offset;
	}m_bufferDesc[g_nBufferCnt];

	char *m_pBuffers[g_nBufferCnt];
	char *m_pBufferHead;
	FILE *m_dumpFile;
	int m_nLastAllocBuf;
	int m_nIdxFreeBuf;
	int m_nFileSize;
	bool m_bWriteMode;
};

class DumpSaver
{
public:
	DumpSaver(const char* dir,EntrySetID entrySetId,TransactionId idTrans);
	~DumpSaver();

	void WriteTuple(void* pData,int len);

private:
	void Write(void* pData,int len);

	DumpHeader m_dumpHeader;
	int m_nOffset;
	DumpFile r_dumpBuffers;
	std::string m_strPath;
};

class DumpLoader
{
public:
	DumpLoader(const char* dir,EntrySetID entrySetId);
	~DumpLoader();
	
	typedef DataItem* Iterator;
	Iterator Begin( void );
	Iterator Next( void );
	Iterator End( void );
private:
	void ExtendData(int nHin);
	bool ReadItem( void );

	DumpHeader m_dumpHeader;
	DataItem   m_dataItem;
	DumpFile r_dumpBuffers;
	int m_nOffset;
	int m_nTuplesReaded;
	std::string m_strPath;
	char* m_itemData;
	int m_itemDataSize;
	uint32 m_chksum;
};
}
}
#endif
