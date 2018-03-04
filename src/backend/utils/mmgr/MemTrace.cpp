#include <stdio.h>
#include "utils/MemTrace.h"
#if defined(LOCK_WAIT_INFO_LOG) || defined(ALLOC_INFO)
#if defined(__WINDOWS__)
#include <DbgHelp.h>

#pragma comment(lib,"DbgHelp.Lib")
#else
#include <string.h>
#include <execinfo.h>
#endif
#endif

#define MAXPRINTLEVEL 60
#if defined(__WINDOWS__)
#define __THREAD  __declspec(thread)
#else 
#define __THREAD __thread
#endif


#if defined(LOCK_WAIT_INFO_LOG)
#if defined(__WINDOWS__)
void BackTrace(const int& MaxDepth,char*& pTrace)
{
	unsigned int   i;
	unsigned int   nLen;
	void         * stack[MAXPRINTLEVEL];
	unsigned short frames;
	SYMBOL_INFO  * symbol;
	HANDLE         process;

	if (NULL == pTrace)
		return;
	process = GetCurrentProcess();

	SymInitialize(process,NULL,TRUE);

	frames               = CaptureStackBackTrace(0, MAXPRINTLEVEL, stack, NULL );
	symbol               = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + (NAMELEN+1) * sizeof(char), 1 );
	symbol->MaxNameLen   = NAMELEN;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	if (frames > MaxDepth)
		frames = MaxDepth;

	for( i = 0; i < frames; i++ )
	{
		SymFromAddr(process,(DWORD64)(stack[i]),0,symbol);
		nLen = strlen(symbol->Name);
		if (nLen >= STRLENGTH)
			_snprintf_s(pTrace+(i*STRLENGTH),STRLENGTH,STRLENGTH-1,"%s",symbol->Name+(nLen-STRLENGTH+1));
		else
			_snprintf_s(pTrace+(i*STRLENGTH),STRLENGTH,STRLENGTH-1,"%s",symbol->Name);
	}

    free( symbol );
}
#endif
#elif defined(ALLOC_INFO)
#define	SIZE 8192
#define NAMELEN 255
#define	GetVoid(Mem)	(void**)((char*)Mem+sizeof(MemoryTrace::MemoryNode))
#define	NodeSize		(SIZE-sizeof(MemoryTrace::MemoryList))
#define	DataSize		(MAXPRINTLEVEL*sizeof(void*))

typedef boost::detail::spinlock::scoped_lock spin_scoped_lock;

/*! @function
********************************************************************************
Name:	MemoryTrace
Author:	Wanghao	
Param:	void
*******************************************************************************/
MemoryTrace::MemoryTrace():m_MaxFrame(MAXPRINTLEVEL),m_MemoryHead(NULL)
{
	#if defined(__WINDOWS__)
	m_process = GetCurrentProcess();
	m_bIsSymInitialized = false;
	#endif
	MemoryList* memory = (MemoryList*)malloc(SIZE);
	if (NULL != memory){
		memset(memory,0,SIZE);
		m_MemoryHead = memory;
		memory->bAlloc = false;
		memory->next = NULL;
	}
}

MemoryTrace::~MemoryTrace()
{
	while(NULL != m_MemoryHead)
	{
		MemoryList* Temp = m_MemoryHead;
		m_MemoryHead = m_MemoryHead->next;
		free(Temp);
	}
	#if defined(__WINDOWS__)
	SymCleanup(m_process);
	#endif
}

/*! @function
********************************************************************************
Name:		GetMemory
Author:		Wanghao	
Objective:	Get the Memory 
Param:		Depth
*******************************************************************************/
int MemoryTrace::GetMemory(unsigned long Hashkey,MemoryNode*& Meminfo,size_t MemSize)
{
	if (NULL == m_MemoryHead->next){
		MemAlloc:
		MemoryList* memory = (MemoryList*)malloc(SIZE);
		if (NULL == memory){
			Meminfo = NULL;
			return  -1;
		}else{
			memset(memory,0,SIZE);
			memory->bAlloc = false; //the set can allocate
			memory->mPos = 0;
			memory->next = NULL;
			MemoryNode* tempNode= (MemoryNode*)((char*)memory+sizeof(MemoryList));
			int nSize = (NodeSize)/(sizeof(MemoryNode)+DataSize);
			
			for (int i=0; i<nSize; i++){
				tempNode->bAlloc = false;
				tempNode->nPos = i;
				tempNode->mSet = (void*)memory;
				tempNode->data = GetVoid(tempNode);
				memory->mArray[i] = tempNode;
				tempNode = (MemoryNode*)((char*)tempNode+sizeof(MemoryNode)+DataSize);
			}
		}

		//set the m_MemoryFree position
		{
			spin_scoped_lock lock(m_MemoryHead->sLock);
			memory->next = m_MemoryHead->next;
			m_MemoryHead->next = memory;
			
			memory->sLock.lock();
		}

		memory->mArray[0]->bAlloc = true;
		memory->mArray[0]->nCount = 1;
		Meminfo = memory->mArray[0];
		return 0;
	}else{
		int nSize = (NodeSize)/(sizeof(MemoryNode)+DataSize);
		MemoryNode*	Temp = NULL;
		MemoryList*	pMemNext = m_MemoryHead->next;
		
		MemNext:
		int nCount = 0;
		bool bFlag = false;
		pMemNext->sLock.lock();
		//memory can allocate
		if (!pMemNext->bAlloc){
			int j=pMemNext->mPos;
			for (int i=j; i<nSize; i++){
				if (!pMemNext->mArray[i]->bAlloc){
					if (!bFlag){
						bFlag = true;
						//allocate the available Memory
						Temp = pMemNext->mArray[i];
						Temp->bAlloc = true;
						Temp->nCount = 1;
					}else {
						pMemNext->mPos = i;
						break;
					}
				}else {
					if ((MemSize == pMemNext->mArray[i]->nSize) && 
						(Hashkey == pMemNext->mArray[i]->nkey)){
						Meminfo = pMemNext->mArray[i];
						++(pMemNext->mArray[i]->nCount);
						return 1;
					}
					nCount++;
				}
			}

			if (nSize == (nCount+1+j)){
				pMemNext->bAlloc = true;
			}
		}
		
		if (NULL == Temp){
			if (NULL != pMemNext->next){
				MemoryList* pTemp = pMemNext->next;
				pMemNext->sLock.unlock();
				pMemNext = pTemp;
				goto MemNext;
			}else{
				pMemNext->sLock.unlock();
				goto MemAlloc;
			}
		}
		Meminfo = Temp;
		return 0;
	}	
}

void MemoryTrace::FreeMemory(MemoryTrace::MemoryNode *&node)
{
	MemoryList* Temp = (MemoryList*)node->mSet;
	if (node->nCount > 1)
	{
		spin_scoped_lock lock(Temp->sLock);
		--node->nCount;
	}
	else if (node->nCount == 1) 
	{
		spin_scoped_lock lock(Temp->sLock);
		if (Temp->bAlloc){
			Temp->bAlloc = false;
		}
		if (node->nPos < Temp->mPos)
			Temp->mPos = node->nPos;
		node->bAlloc = false;
		node->nCount = 0;
		node->nkey = 0;
		node->nDepth = 0;
		node->nSize = 0;
		
	}
}

/* @function
********************************************************************************
Name:		GetStackInfo
Author:		Wanghao	
Objective:	saved an array of pointers captured from the current stack trace
Date:		2013-3-21
Param:		info
********************************************************************************/
void MemoryTrace::GetStackInfo(MemoryTrace::MemoryNode** info, size_t nSize)
{
	unsigned int	nDepth = 0;
	unsigned long   nkey = 0;
	unsigned int	nValue = 0;
	void*			stack[MAXPRINTLEVEL];
	MemoryNode* 	pNode = NULL;

	#if defined(__WINDOWS__)
	nDepth = CaptureStackBackTrace(0,m_MaxFrame,stack,&nkey );
	#else
	nDepth = backtrace(stack,m_MaxFrame);
	CaluateHash(nkey,stack,nDepth);
	#endif
	
	nValue = GetMemory(nkey,pNode,nSize);
	if (NULL != pNode){
		MemoryList* pSet = (MemoryList*)pNode->mSet;
		if (nValue > 0){
			*info = pNode;
		}else{
			pNode->data = GetVoid(pNode);
			*info = pNode;
			for (unsigned int i=0; i<nDepth; i++){
				pNode->data[i] = stack[i];
			}
			(*info)->nDepth = nDepth;
			(*info)->nkey = nkey;
			(*info)->nSize = nSize;
			
		}
		pSet->sLock.unlock();
	}else{
	*info = NULL;
	}
}

void MemoryTrace::CaluateHash(unsigned long& Value,void** Array,unsigned int ArraySize)
{
	for(unsigned int i=0; i<ArraySize; i++){
		Value += (long)Array[i];
	}
}


/* @function
********************************************************************************
Name:		Print
Author:		Wanghao	
Objective:	Print the stack infomation
Date:		2013-3-22
Param(Out):	Infomation
Param(In):	Buf size (pPrint[Size*Depth])
Param(In):	Stack Depth
********************************************************************************/
size_t MemoryTrace::Print(MemoryTrace::MemoryNode** info, char* pPrint,
			const size_t Size, const unsigned int Depth, char *retractions)
{
	unsigned int i = 0;
	
	char* pbuf = pPrint;
	if ((NULL == *info) || (NULL == pPrint))
		return 0;

	unsigned int nPrint = (*info)->nDepth;
	if (Depth < (*info)->nDepth){
		nPrint = Depth;
	}
	
	MemoryList* pTemp = (MemoryList*)(*info)->mSet;
	memset(pPrint, 0, Size*MAXPRINTLEVEL);
#if defined(__WINDOWS__)
	SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
	if(!m_bIsSymInitialized)
	{
		if(!SymInitialize(m_process,NULL,TRUE))
		{
			DWORD error = GetLastError();
			printf("MemTrace.cpp | SymInitialize returned error : %d\n", error);
		}
		else
			m_bIsSymInitialized = true;
	}
	else
	{
		if(!SymRefreshModuleList(m_process))
		{
			DWORD error = GetLastError();
			printf("MemTrace.cpp | SymRefreshModuleList returned error : %d\n", error);
		}
	}
	IMAGEHLP_LINE64 Source;
	DWORD Displacement;
	SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + (NAMELEN + 1)*sizeof(char), 1 );
	symbol->MaxNameLen   = NAMELEN;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	memset(&Source, 0, sizeof(IMAGEHLP_LINE64));
	Source.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	
	spin_scoped_lock lock(pTemp->sLock);
	for (i=0; i < nPrint; i++)
	{
		if(!SymFromAddr(m_process,(DWORD64)((*info)->data[i]),0,symbol))
		{
			DWORD error = GetLastError();
			printf("MemTrace.cpp | SymFromAddr returned error : %d\n", error);
		}
		else
		{
			SymGetLineFromAddr64(m_process, (DWORD64)((*info)->data[i]), &Displacement, &Source);
			char* filename = strrchr(Source.FileName, '\\') + 1;
			int count = _snprintf_s(pbuf, Size, Size-1, "%s%s(%s:%d)\n",
							retractions, symbol->Name, filename, Source.LineNumber);
			pbuf = pbuf + count;
		}
	}
	free(symbol);
#else
	char** pString = NULL;

	spin_scoped_lock lock(pTemp->sLock);
	pString = backtrace_symbols((*info)->data,nPrint);
	for( i = 0; i < nPrint; i++ )
	{
		int count = snprintf(pbuf, Size, "%s%s\n", retractions, pString[i]);
		pbuf = pbuf + count;
	}
	free(pString);
#endif

	// replace < > by [ ]
	char *p = pPrint;
	while(p < pbuf)
	{
		if(*p == '<')
		{
			*p = '[';
		}
		else if(*p == '>')
		{
			*p = ']';
		}
		p++;
	}

	return pbuf - pPrint;
}

MemoryTrace& MemoryTrace::Instance()
{
	static MemoryTrace mem;
	return mem;
}
#endif


