#ifndef PG_XDB_LOCK_H
#define PG_XDB_LOCK_H
#include "XdbLock.h"
#include "storage/s_lock.h"
#include "Transaction.h"

namespace FounderXDB{
namespace StorageEngineNS {

class PGXdbLock: public XdbLock
{
private:
    PGXdbLock(LockType lt);
    PGXdbLock(const PGXdbLock &other);
public:
	// Spinlocks don't support lock mode, Mutex does.
	static PGXdbLock* CreatePGLock(LockType lt, MemoryContext*);
	virtual ~PGXdbLock();
	virtual void lock(LockMode lm);
	// try lock, return true if locked, false if unable to obtain lock.
	// spinlock does not support this method.
	virtual bool tryLock(LockMode lm);
	virtual void unlock();
private:
	slock_t m_spinlock;
	LWLockId m_lockid;
	LockType m_locktypeid;
};
}
}
#endif //PG_XDB_LOCK_H
