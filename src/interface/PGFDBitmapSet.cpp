#include "postgres.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/PGFDBitmapSet.h"

namespace FounderXDB{
namespace StorageEngineNS {

#define THROW_CALL(FUNC,...)   \
	bool flag = false;\
	PG_TRY(); {\
	FUNC(__VA_ARGS__);\
} PG_CATCH(); { \
	flag=true;   \
} PG_END_TRY(); \
	if (flag)\
{\
	ThrowException(); \
}
	
PGBitmapSet::PGBitmapSet(Bitmapset* pgbitmapset,Transaction *txn):m_bitmapset(pgbitmapset),m_curtxn(txn)
{
	
}
PGBitmapSet::~PGBitmapSet()
{

}
BitmapSet *PGBitmapSet::duplicate() const
{
	MemoryContext* cxt = m_curtxn->getAssociatedMemoryContext();
	PGBitmapSet* bitset = new(*cxt) PGBitmapSet(m_bitmapset,m_curtxn);// 放到txn memcxt中
	return bitset;
}
bool PGBitmapSet::isEqual(const BitmapSet&other) const
{
	bool ret = false;
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(ret = bms_equal,m_bitmapset,pgother->m_bitmapset)
	return ret;
}

BitmapSet *PGBitmapSet::doUnion(const BitmapSet&other) const
{
	Bitmapset *temp = NULL;
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(temp = bms_union,m_bitmapset,pgother->m_bitmapset);
	MemoryContext* cxt = m_curtxn->getAssociatedMemoryContext();
	PGBitmapSet* bitset = new(*cxt) PGBitmapSet(temp,m_curtxn);// 放到txn memcxt中
	return bitset;
}
void PGBitmapSet::doUnion(const BitmapSet&other)
{
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(m_bitmapset = bms_add_members,m_bitmapset,pgother->m_bitmapset);
}
BitmapSet *PGBitmapSet::doIntersect(const BitmapSet&other) const
{
	Bitmapset *temp = NULL;
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(temp = bms_intersect,m_bitmapset,pgother->m_bitmapset);
	MemoryContext *cxt = m_curtxn->getAssociatedMemoryContext();
	PGBitmapSet* bitset = new(*cxt) PGBitmapSet(temp,m_curtxn);// 放到txn memcxt中
	return bitset;
}
void PGBitmapSet::doIntersect(const BitmapSet&other)
{
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(m_bitmapset = bms_int_members,m_bitmapset,pgother->m_bitmapset);
}
bool PGBitmapSet::isOverlap(const BitmapSet&other) const
{
	bool ret = false;
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(ret = bms_overlap,m_bitmapset,pgother->m_bitmapset)
	return ret;
}
BitmapSet *PGBitmapSet::doDifference(const BitmapSet&other) const
{
	Bitmapset *temp = NULL;
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(temp = bms_difference,m_bitmapset,pgother->m_bitmapset);
	MemoryContext* cxt = m_curtxn->getAssociatedMemoryContext();
	PGBitmapSet* bitset = new(*cxt) PGBitmapSet(temp,m_curtxn);// 放到txn memcxt中
	return bitset;
}
void PGBitmapSet::doDifference(const BitmapSet&other)
{
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(m_bitmapset = bms_del_members,m_bitmapset,pgother->m_bitmapset);
}
bool PGBitmapSet::hasNonEmptyDifference(const BitmapSet&other) const
{
	bool ret = false;
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(ret = bms_nonempty_difference,m_bitmapset,pgother->m_bitmapset)
	return ret;
}
void PGBitmapSet::join(BitmapSet&other)
{
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(m_bitmapset = bms_join,m_bitmapset,pgother->m_bitmapset);
}
	
bool PGBitmapSet::isSubsetOf (const BitmapSet&other) const
{
	bool ret = false;
	const PGBitmapSet* pgother = static_cast<const PGBitmapSet*>(&other);
	THROW_CALL(ret = bms_is_subset,m_bitmapset,pgother->m_bitmapset)
	return ret;
}
bool PGBitmapSet::isMyMember(int x) const
{
	bool ret = false;
	THROW_CALL(ret = bms_is_member,x,m_bitmapset);
	return ret;
}
size_t PGBitmapSet::numMembers() const
{
	size_t num = 0;
	THROW_CALL(num = bms_num_members,m_bitmapset);
	return num;
}

BitmapSet::BMS_Membership PGBitmapSet::membership() const
{
	::BMS_Membership temp = (::BMS_Membership)0;
	THROW_CALL(temp = bms_membership,m_bitmapset);
	if (temp == (::BMS_Membership)BMS_EMPTY_SET)
		return BitmapSet::BMS_EMPTY_SET;
	else if (temp == (::BMS_Membership)BMS_SINGLETON)
		return BitmapSet::BMS_SINGLETON;
	else 
		return BitmapSet::BMS_MULTIPLE;

}
bool PGBitmapSet::isEmpty() const
{
	bool ret = false;
	THROW_CALL(ret = bms_is_empty,m_bitmapset);
	return ret;
}
void PGBitmapSet::addMember(int x)
{
	THROW_CALL(m_bitmapset = bms_add_member,m_bitmapset,x);
}
void PGBitmapSet::delMember(int x)
{
	THROW_CALL(m_bitmapset = bms_del_member,m_bitmapset,x);
}
int PGBitmapSet::removeFirst()
{
	int ret = 0;
	THROW_CALL(ret = bms_first_member,m_bitmapset);
	return ret;
}

}
}