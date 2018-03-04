#ifndef __MEMORY_TRACE_H__
#define __MEMORY_TRACE_H__

#if defined(LOCK_WAIT_INFO_LOG)
#if defined(__WINDOWS__)
#include <windows.h>
#endif
#if defined(STRLENGTH)
#undef 	STRLENGTH
#define STRLENGTH	128
#else
#define STRLENGTH	128
#endif
//pTrace所持有的内存大小要等于STRLENGTH*MaxDepth
extern void BackTrace(const int& MaxDepth,char*& pTrace);

#elif defined(ALLOC_INFO)
#include <boost/smart_ptr/detail/spinlock.hpp>
#if defined(__WINDOWS__)
#include <windows.h>
#endif
#include <set>
#define MAXARRAYLEN	40

class MemoryTrace
{
public:
	struct MemoryNode
	{
		bool			bAlloc;	//the Memory is available
		unsigned int	nPos; 	//self position
		unsigned int	nCount; //ncount.
		unsigned int	nDepth; //stack frames
		size_t			nSize; 	//memory block size
		unsigned long   nkey; 	//stack hash
		void*			mSet;	//memory set
		void**			data;	//data address 
	};
	struct MemoryList
	{
		bool					bAlloc;					//the set can allocate
		unsigned int			mPos;					//start position
		MemoryNode* 			mArray[MAXARRAYLEN];	//max size = 16384/60*8  size=16384/60*4
		boost::detail::spinlock	sLock;
		struct MemoryList*		next;
	};
private:
	#if defined(__WINDOWS__)
	HANDLE		m_process;
	#endif
	const unsigned int	m_MaxFrame;
	MemoryList*			m_MemoryHead;
	bool                m_bIsSymInitialized;
public:
	MemoryTrace(void);
	~MemoryTrace(void);
	//max Depth 60
	void GetStackInfo(MemoryTrace::MemoryNode** info, size_t nSize);
	void FreeMemory(MemoryTrace::MemoryNode*& node);
	//the size of  the array (pPirnt) = Size*60
	size_t Print(MemoryTrace::MemoryNode** info, char* pPrint,
				const size_t Size, const unsigned int Depth, char *retractions);
	
	static MemoryTrace& Instance(void);
private:
	int GetMemory(unsigned long Hashkey,MemoryNode*& Meminfo,size_t MemSize);
	void CaluateHash(unsigned long& Value,void** Array,unsigned int ArraySize);
};


struct KeyValue{
	size_t		   				nSize; 	//memory block size
	unsigned long   			nKey; 	//stack hash
	KeyValue(){nSize=0; nKey=0;}
	KeyValue(size_t Size,unsigned long Key)
	{
		nSize=Size; 
		nKey=Key; 
	}
	KeyValue(const KeyValue& keyVal)
	{
		nSize = keyVal.nSize; 
		nKey = keyVal.nKey; 
	}
	KeyValue& operator= (const KeyValue& keyVal)
	{
		nSize = keyVal.nSize; 
		nKey = keyVal.nKey; 
		return *this;
	}
	bool operator== (const KeyValue& keyVal)const
	{
		if ((nSize == keyVal.nSize) && (nKey == keyVal.nKey))
			return true;
		else
			return false;
	}
	bool operator< (const KeyValue& keyVal)const
	{
		if ((nKey < keyVal.nKey) || ((nKey == keyVal.nKey) && (nSize < keyVal.nSize)))
			return true;
		else
			return false;
	}
};

struct DataValue{
	unsigned int						nCount; 	//ncount.
	std::set<MemoryTrace::MemoryNode*>	setpNode;	//memory set
	DataValue(){nCount=0; setpNode.clear();}
	DataValue(unsigned int Count,MemoryTrace::MemoryNode* pNode)
	{
		nCount=Count;
		setpNode.insert(pNode);
	}
	DataValue(const DataValue& DataVal)
	{
		nCount = DataVal.nCount; 
		setpNode = DataVal.setpNode;
	}
	DataValue& operator= (const DataValue& DataVal)
	{
		nCount = DataVal.nCount; 
		setpNode = DataVal.setpNode;
		return *this;
	}
};
#endif
#endif
