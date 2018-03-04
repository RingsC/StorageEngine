/*
*   It's important to know when you turn on 'DEBUG_OPERATOR_NEW_DELETE', all of the memory operation will be recored and output the console,
*   it makes the system very very slow, very very pool performance, the main works use to do screen operation. and you can replace the print with file operation to record
*   output information to log file.
*/
#include <cstdlib>
#include <cassert>
#include <algorithm>
#include "interface/utils.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/ErrNo.h"

#include "MemoryContext.h"

#define ALLOC_HEADER_SIZE  MAXALIGN(sizeof(AllocHeader))
namespace FounderXDB
{
namespace StorageEngineNS
{

class PGSE_DLLEXTERN Slock{
public:
    Slock();
public:
    slock_t m_slock;
};

class PGSE_DLLEXTERN ScopedSpinLock{
public:
    ScopedSpinLock(Slock &slock);

    ~ScopedSpinLock();
private:
    ScopedSpinLock(const ScopedSpinLock&);
    const ScopedSpinLock&operator=(const ScopedSpinLock&);
    Slock m_slock;
};

Slock::Slock()
{
    FDPG_Lock::fd_spinlock_init(m_slock);
}

ScopedSpinLock::ScopedSpinLock(Slock &slock) : m_slock(slock) 
{
	FDPG_Lock::fd_spinlock_acquire(m_slock.m_slock);
}

ScopedSpinLock::~ScopedSpinLock() 
{ 
	FDPG_Lock::fd_spinlock_release(m_slock.m_slock); 
}

void MemoryContext::MemoryPool::SetObjectDestroyed(void *basep)
{
    AllocHeader *hdr = (AllocHeader *)basep;
    hdr->flags &= ~AllocHeader::IS_OBJECT_FLAG;// remove the isobj bit, because the object is about to be destroyed.
}

MemoryContext::MemoryPool::MemoryPool(MemoryContext *memcxt)
{
    assert(memcxt != 0);
    this->context = memcxt;
}

#ifdef DEBUG_OPERATOR_NEW_DELETE
std::vector<void *> v;
#endif

void MemoryContext::MemoryPool::doDelete(void *p, const char* filename, uint32 lineno )
{
	bool releaseMemory = true, isobj = true;
    void *base = NULL;              
    base = MemoryContext::MemoryPool::GetChunkInfo(p, releaseMemory, isobj);   
    assert(base != NULL);                      
    if (releaseMemory)
        MemoryContext::deAlloc(base);
    else                
        MemoryContext::MemoryPool::SetObjectDestroyed(base);
#ifdef DEBUG_OPERATOR_NEW_DELETE
//	printf("delete %p\n",p);	
//	std::vector<void *>::iterator result = find(v.begin(),v.end(),p);
//	v.erase(std::find(v.begin(),v.end(),p));
	FDPG_Memory::fd_printf("\n\n---------------------\n\n");
	std::vector<void *>::iterator result_temp;
	for(std::vector<void *>::iterator result = v.begin();
		result != v.end(); ++result)
	{
		if(*result == p)
		{
			//FDPG_Memory::fd_printf("delete %p@%s:%n\n",*result, filename, lineno);
			printf("delete %p@%s:%n\n",*result, filename, lineno);
			result_temp = result;
//			v.erase(result);		
//			break;
		}
		if(*result != p && result != v.end())
		{
			//FDPG_Memory::fd_printf("%p\n",*result);
			printf("delete %p\n",*result);
		}
		if(result == v.end())
			//FDPG_Memory::fd_printf("error: %p not found\n",p); 
			printf("error: %p not found\n",p); 
	}
	v.erase(result_temp);
//	v.erase(std::find(v.begin(),v.end(),p));
#endif
}

void *MemoryContext::MemoryPool::AllocateMem(size_t nbytes, bool releaseOnDelete, const char* filename, uint32 lineno)
{
    bool isobject = true;// now this class is only used to allocate object memory.
    if (!isobject)
        releaseOnDelete = true;
    void *p = context->alloc(MAXALIGN(nbytes + ALLOC_HEADER_SIZE));
    //memset(p, 0, ALLOC_HEADER_SIZE); already zerofied
    void *retp = AllocHeader::BasePtrToUserPtr(p);// Make offset for the header.
    AllocHeader *hdr = (AllocHeader *)p;
    Assert(hdr == p);
    
    hdr->flags = AllocHeader::MAGIC_BYTE;
    hdr->flags |= releaseOnDelete ? AllocHeader::RELEASE_ON_DELETE_FLAG : 0;
    hdr->flags |= isobject ? AllocHeader::IS_OBJECT_FLAG : 0;
#ifdef DEBUG_OPERATOR_NEW_DELETE
	//FDPG_Memory::fd_printf("new %p@%s:%n\n",retp, filename, lineno);
	printf ("new %p@%s:%n \n",retp, filename, lineno);
	v.push_back(retp);
#endif
    return retp;
}

void *MemoryContext::MemoryPool::GetChunkInfo(void *p, bool &releaseOnDelete, bool &isobject)
{
    void *base = AllocHeader::UserPtrToBasePtr(p);// get the allocated base address
    AllocHeader *hdr = (AllocHeader *)base;
    Assert(hdr == base);
    
    releaseOnDelete = hdr->isReleaseOnDelete();
    isobject = hdr->isObject();
    assert(isobject == true);// this class is now only used to allocate object memory.
    return base;
}

void *MemoryContext::MemoryPool::AllocHeader::UserPtrToBasePtr(void *p) { return (char *)p - ALLOC_HEADER_SIZE;}
void *MemoryContext::MemoryPool::AllocHeader::BasePtrToUserPtr(void *p) { return (char *)p + ALLOC_HEADER_SIZE;}
        
} // StorageEngineNS
} // FounderXDB
