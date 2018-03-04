#ifndef PG_FDBITMAPSET_H
#define PG_FDBITMAPSET_H
#include "nodes/bitmapset.h"
#include "FDBitmapSet.h"
#include "Transaction.h"

namespace FounderXDB{
namespace StorageEngineNS {
	
class PGBitmapSet: public BitmapSet
{
private:
    PGBitmapSet(const PGBitmapSet &other);
	const PGBitmapSet&operator=(const PGBitmapSet&);
public:
	PGBitmapSet(Bitmapset* pgbitmapset,Transaction* txn);
	PGBitmapSet(){};
	~PGBitmapSet();
	virtual BitmapSet *duplicate() const;
	virtual bool isEqual(const BitmapSet&other) const;
	
	// set operations
	virtual BitmapSet *doUnion(const BitmapSet&other)const;//bms_union. union result is returned as a new bitmapset
	virtual void doUnion(const BitmapSet&other);//bms_add_member. modifies this bitmapset
	virtual BitmapSet *doIntersect(const BitmapSet&other) const;//bms_intersect. intersect result is returned as a new bitmapset
	virtual void doIntersect(const BitmapSet&other);//bms_int_members. modifies this bitmapset
	virtual bool isOverlap(const BitmapSet&other) const;
	virtual BitmapSet *doDifference(const BitmapSet&other) const;//bms_difference. difference result is returned as a new bitmapset
	virtual void doDifference(const BitmapSet&other);//bms_del_members. modifies this bitmapset
	virtual bool hasNonEmptyDifference(const BitmapSet&other)const;
	virtual void join(BitmapSet&other);
	
	virtual bool isSubsetOf (const BitmapSet&other)const;
	virtual bool isMyMember(int x)const;
	virtual size_t numMembers()const;
	virtual BMS_Membership membership()const;
	virtual bool isEmpty()const;
	
	virtual void addMember(int x);
	virtual void delMember(int x);
	// remove 1st integer, used to do iteration. call duplicate() to duplicate a tmp,
	// then call tmp.removeFirst() to get its members one by one.
	virtual int removeFirst();
private:
	Bitmapset* m_bitmapset;
	Transaction *m_curtxn;
};
}
}
#endif //PG_FDBITMAPSET_H