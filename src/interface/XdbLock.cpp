#include "postgres.h"
#include "storage/lock.h"
#include "interface/FDPGAdapter.h"
#include "XdbLock.h"
#include "interface/PGXdbLock.h"

using namespace FounderXDB::StorageEngineNS;

namespace FounderXDB{
namespace StorageEngineNS {

XdbLock::~XdbLock()
{

}
XdbLock *XdbLock::createLock(LockType lt, MemoryContext *cxt)
{
	return PGXdbLock::CreatePGLock(lt, cxt);
}

bool XdbLock::transactionTryLock(uint32 dbid, uint32 classid, uint32 objid, uint16 objsubid, LockMode lm)
{
	LOCKTAG tag;
	tag.locktag_field1 = dbid;
	tag.locktag_field2 = classid;
	tag.locktag_field3 = objid;
	tag.locktag_field4 = objsubid;
	tag.locktag_type = LOCKTAG_OBJECT;
	tag.locktag_lockmethodid = DEFAULT_LOCKMETHOD;

	return (FDPG_Lock::fd_lock_acqurie(&tag,lm == XdbLock::Exclusive ? ExclusiveLock : ShareLock,false,true) == LOCKACQUIRE_OK);

}

void XdbLock::transactionLock(uint32 dbid, uint32 classid, uint32 objid, uint16 objsubid, LockMode lm)
{
	LOCKTAG tag;
	tag.locktag_field1 = dbid;
	tag.locktag_field2 = classid;
	tag.locktag_field3 = objid;
	tag.locktag_field4 = objsubid;
	tag.locktag_type = LOCKTAG_OBJECT;
	tag.locktag_lockmethodid = DEFAULT_LOCKMETHOD;

	FDPG_Lock::fd_lock_acqurie(&tag,lm == XdbLock::Exclusive ? ExclusiveLock : ShareLock,false,false);
}

void XdbLock::transactionUnlock(uint32 dbid, uint32 classid, uint32 objid, uint16 objsubid, LockMode lm)
{
	LOCKTAG tag;
	tag.locktag_field1 = dbid;
	tag.locktag_field2 = classid;
	tag.locktag_field3 = objid;
	tag.locktag_field4 = objsubid;
	tag.locktag_type = LOCKTAG_OBJECT;
	tag.locktag_lockmethodid = DEFAULT_LOCKMETHOD;
	
	FDPG_Lock::fd_lock_release(&tag,lm == XdbLock::Exclusive ? ExclusiveLock : ShareLock,false);
}

ScopedXdbLock::ScopedXdbLock(XdbLock *l, XdbLock::LockMode lm) : lock_(l)
{
    if (lock_)
        lock_->lock(lm);
}

ScopedXdbLock::~ScopedXdbLock()
{
    if (lock_)
        lock_->unlock();
}
    
}
}