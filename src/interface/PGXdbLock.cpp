#include "postgres.h"
#include "storage/lock.h"
#include "interface/PGXdbLock.h"
#include "interface/FDPGAdapter.h"
#include "storage/lock.h"
#include "interface/StorageEngineExceptionUniversal.h"

using namespace FounderXDB::StorageEngineNS;

namespace FounderXDB{
namespace StorageEngineNS {

PGXdbLock::PGXdbLock(LockType lt):m_locktypeid(lt)
{
	if (lt==SpinLock)
		FDPG_Lock::fd_spinlock_init(m_spinlock);
	else if (lt==Mutex)
		m_lockid = FDPG_Lock::fd_LWLockAssign();
}
PGXdbLock::~PGXdbLock()

{
	if(m_locktypeid == Mutex)
	{
		FDPG_Lock::fd_DestroyLock(m_lockid);
	}
}
PGXdbLock* PGXdbLock::CreatePGLock(LockType lt, MemoryContext *cxt)
{
	return new (*cxt) PGXdbLock(lt);
}
#include "exception/XdbExceptions.h"
bool PGXdbLock::tryLock(LockMode lm)//������ʵ�ִ���try lock����˼��˵��lock���ǲ��ȴ�����޷����̵õ�����ô���̷��ز��ȴ�ؽ����false���õ�����ô���ؽ����true��
{
	if (m_locktypeid == Mutex)
	{
		LWLockMode mode;
		if (lm == Shared)
			mode = LW_SHARED;
		else
			mode = LW_EXCLUSIVE;
		return FDPG_Lock::fd_LWLockConditionalAcquire(m_lockid,mode);
	}
	else if (m_locktypeid == SpinLock)
	{
		throw FounderXDB::EXCEPTION::NotSupportedException("\nCan't call XdbLock::tryLock against spin locks.");
		//return false;
	}
	else
	{
		Assert(false);// no other types of locks.
		return false;
	}
}

void PGXdbLock::lock(LockMode lm)
{
	if (m_locktypeid == SpinLock)
	{
		FDPG_Lock::fd_spinlock_acquire(m_spinlock);
	}
	else if (m_locktypeid == Mutex)
	{
		LWLockMode mode;
		if (lm == Shared)
			mode = LW_SHARED;
		else
			mode = LW_EXCLUSIVE;
		FDPG_Lock::fd_LWLockAcquire(m_lockid,mode);
	}
}

void PGXdbLock::unlock()
{
	if (m_locktypeid == SpinLock)
	{
		FDPG_Lock::fd_spinlock_release(m_spinlock);
	}
	else if (m_locktypeid == Mutex)
	{
		FDPG_Lock::fd_LWLockRelease(m_lockid);
	}
}

}
}