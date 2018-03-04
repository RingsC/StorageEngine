#include "interface/PGDump.h"
#include "utils/pg_crc.h"
#include "interface/ErrNo.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/FDPGAdapter.h"
namespace FounderXDB
{
namespace StorageEngineNS 
{

static const int DUMPMAGIC = 0x64756d70;
DumpHeader::DumpHeader(TransactionId idTrans,EntrySetID entrySetId)
:magicNum(DUMPMAGIC)
,version(1)
,numTuples(0)
,dumpWhen(0)
,currentTransactionId(idTrans)
,entrySetId(entrySetId)
,chksum(0)
{
    INIT_CRC32(chksum);
}

DumpFile::DumpFile( void )
:m_pBufferHead(NULL)
,m_nLastAllocBuf(-1)
,m_nIdxFreeBuf(0)
,m_nFileSize(0)
,m_bWriteMode(false)
{

}

DumpFile::~DumpFile( void )
{
    delete m_pBufferHead;
	if(NULL != m_dumpFile)
	{
		Flush();
		fclose(m_dumpFile);
	}
}

void DumpFile::Open(const std::string& strFilePath,bool bWriteMode)
{
	if(bWriteMode)
	{
		m_dumpFile = FDPG_File::fd_fopen(strFilePath.c_str(),"w+");
	}
	else
	{
        m_dumpFile = FDPG_File::fd_fopen(strFilePath.c_str(),"r");
	}

	//int nErr = ferror(m_dumpFile);
	if (NULL == m_dumpFile)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, strFilePath + " doesn't exists on disk!");
	}
	fseek(m_dumpFile,0,SEEK_END);
	m_nFileSize = ftell(m_dumpFile);
    m_bWriteMode = bWriteMode;
	Init();
}

void DumpFile::Init( void )
{
    if(NULL == m_pBufferHead)
	{
		m_pBufferHead = new char[g_nBufferSize * g_nBufferCnt];
        char *buffer = m_pBufferHead;
		for (int idx = 0; idx < g_nBufferCnt; ++idx)
		{
			m_pBuffers[idx] = buffer;
			buffer += g_nBufferSize;
		}
	}
}

void DumpFile::WriteBuffer2Disk(int nCnt)
{
    if(!m_bWriteMode)
	{
		return;
	}
	assert(nCnt >= 0);
	//write 
	for(int nIdx = 0; nIdx < nCnt; ++nIdx)//最好可以一次性写入，不要分页写入。 为什么中间可能有clean的页面？
	{
		if (m_bufferDesc[nIdx].dirty)
		{
			fseek(m_dumpFile,m_bufferDesc[nIdx].offset,SEEK_SET);
			fwrite(m_pBuffers[nIdx],1,g_nBufferSize,m_dumpFile);
			m_bufferDesc[nIdx].dirty = false;
		}
	}

	m_nIdxFreeBuf = 0;
	m_nLastAllocBuf = -1;
}

void DumpFile::Flush( void )
{
    WriteBuffer2Disk(g_nBufferCnt);
}

char* DumpFile::GetBuffer(int nOffset,int& freeSpace,bool bSync)//不需要这么多buffer，可以使用N个pagesize（默认8K）的大buffer，装满了就写入文件即可。
{
	if (!m_bWriteMode && nOffset > m_nFileSize)
	{
		return NULL;
	}

	char* pBuffer = NULL;
	int nResultBufIdx;
    if(m_nLastAllocBuf >= 0
		&& (nOffset < m_bufferDesc[0].offset
		|| nOffset >= (m_bufferDesc[m_nLastAllocBuf].offset + 2 * g_nBufferSize)))
	{
		WriteBuffer2Disk(m_nLastAllocBuf + 1);
	}

    if (m_nLastAllocBuf >= 0 && (nOffset >= m_bufferDesc[0].offset)
		&& (nOffset < m_bufferDesc[m_nLastAllocBuf].offset + g_nBufferSize))
	{
		nResultBufIdx = (nOffset - m_bufferDesc[0].offset) / g_nBufferSize;
		pBuffer = m_pBuffers[nResultBufIdx];
	}
	else
	{
		if (0 == m_nIdxFreeBuf && (m_nLastAllocBuf == (g_nBufferCnt - 1)))
		{
			WriteBuffer2Disk(g_nBufferCnt);
		}

		nResultBufIdx = m_nIdxFreeBuf;
		pBuffer = m_pBuffers[nResultBufIdx];
		m_bufferDesc[nResultBufIdx].dirty = m_bWriteMode;
        m_bufferDesc[nResultBufIdx].offset = nOffset;

		if (!m_bWriteMode || bSync)
		{
			fseek(m_dumpFile,m_bufferDesc[nResultBufIdx].offset,SEEK_SET);
			int n = (int)fread(pBuffer,1,g_nBufferSize,m_dumpFile);
			++n;
		}
        
		m_nLastAllocBuf = m_nIdxFreeBuf;
		m_nIdxFreeBuf = (m_nIdxFreeBuf + 1) % g_nBufferCnt;
	}

	if (NULL != pBuffer)
	{
		int nPageOffset = nOffset & (g_nBufferSize - 1);
		freeSpace = g_nBufferSize - nPageOffset;
		return pBuffer + nPageOffset;
	}
    return NULL;
}

void DumpFile::Write(int nOffset,int nSize,void* data,bool bSync)
{
	int freeSpace(0);
	size_t nWriteSize(0);
	while (nSize > 0)
	{
		char* buffer = GetBuffer(nOffset,freeSpace,bSync);
		nWriteSize = nSize < freeSpace ? nSize : freeSpace;
        memcpy(buffer,data,nWriteSize);

		nSize -=(int)nWriteSize;
		nOffset +=(int) nWriteSize;
		data = (char*)data + nWriteSize;
	}
}

int DumpFile::Read(int nOffset,int nSize,void* data)
{
	int nNeedRead = nSize;
	int freeSpace(0);
	size_t nReadSize(0);
	while (nSize > 0)
	{
		char* buffer = GetBuffer(nOffset,freeSpace);
		if (NULL == buffer)
		{
			break;
		}

		nReadSize = nSize < freeSpace ? nSize : freeSpace;
		memcpy(data,buffer,nReadSize);

		nSize -= (int)nReadSize;
		nOffset +=(int) nReadSize;
		data = (char*)data + nReadSize;
	} 

	return (nNeedRead - nSize);
}

DumpSaver::DumpSaver(const char* dir,EntrySetID entrySetId,TransactionId idTrans)
:m_dumpHeader(idTrans,entrySetId)
,m_nOffset(0)
,m_strPath(std::string(dir))

{
	r_dumpBuffers.Open(m_strPath,true);
	Write(&m_dumpHeader,sizeof(DumpHeader));
}

void DumpSaver::Write(void* pData,int len)
{
	r_dumpBuffers.Write(m_nOffset,len,pData);
	m_nOffset += len;
}

void DumpSaver::WriteTuple(void* pData,int len)
{
    assert(NULL != pData && 0 < len);
	++m_dumpHeader.numTuples;
	//COMP_CRC32(m_dumpHeader.chksum, &len,sizeof(int));//这次调用不需要，只需要计算数据的crc chksum即可。
    Write(&len,sizeof(int));
	COMP_CRC32(m_dumpHeader.chksum, pData, len);
    Write(pData,len);
}

DumpSaver::~DumpSaver()
{
	FIN_CRC32(m_dumpHeader.chksum);
    r_dumpBuffers.Write(0,sizeof(DumpHeader),&m_dumpHeader,true);
	r_dumpBuffers.Flush();
}

DumpLoader::DumpLoader(const char* dir,EntrySetID entrySetId)
:m_dumpHeader(0,entrySetId)
,m_nOffset(0)
,m_nTuplesReaded(0)
,m_strPath(std::string(dir))
,m_itemData(NULL)
,m_itemDataSize(0)
{
	r_dumpBuffers.Open(m_strPath);

}

DumpLoader::~DumpLoader()
{
    if(0 != m_itemData)
	{
		free(m_itemData);
	}
}

DumpLoader::Iterator DumpLoader::Begin( void )
{
    r_dumpBuffers.Read(0,sizeof(DumpHeader),&m_dumpHeader);
	m_nOffset += sizeof(DumpHeader);
	if (DUMPMAGIC != (int)m_dumpHeader.magicNum)
	{
		throw StorageEngineExceptionUniversal(LOGIC_ERR, m_strPath + " is not an valid dump file!");
	}
    INIT_CRC32(m_chksum);
    return Next();
}

bool DumpLoader::ReadItem( void )
{
	if ((uint32)m_nTuplesReaded == m_dumpHeader.numTuples)
	{
		FIN_CRC32(m_chksum);
		if (!EQ_CRC32(m_chksum,m_dumpHeader.chksum))
		{
            throw StorageEngineExceptionUniversal(LOGIC_ERR, m_strPath + "  is damaged!");
		}
		return false;
	}

	if(sizeof(int) == r_dumpBuffers.Read(m_nOffset,sizeof(int),&m_dataItem.size))
	{
		//COMP_CRC32(m_chksum, &m_dataItem.size,sizeof(int));//不需要对长度做校验
		m_nOffset += sizeof(int);
		ExtendData((int)m_dataItem.size);
		if((int)m_dataItem.getSize() == r_dumpBuffers.Read(m_nOffset,(int)m_dataItem.getSize(),m_dataItem.data))
		{
			COMP_CRC32(m_chksum, m_dataItem.data,(uint32)m_dataItem.size);
			m_nOffset += (int)m_dataItem.getSize();
			++m_nTuplesReaded;
			return true;
		}
	}

	throw StorageEngineExceptionUniversal(LOGIC_ERR, m_strPath + "  is damaged!");
}

DumpLoader::Iterator DumpLoader::Next( void )
{
	if (ReadItem())
	{
		return &m_dataItem;
	}

	return NULL;
}

DumpLoader::Iterator DumpLoader::End( void )
{
    return NULL;
}

void DumpLoader::ExtendData(int nHint)
{
    if (m_itemDataSize < nHint)
    {
		m_itemDataSize = nHint;
		m_itemData = (char*)realloc(m_itemData,m_itemDataSize);
		m_dataItem.setData(m_itemData);
    }
}
}
}
