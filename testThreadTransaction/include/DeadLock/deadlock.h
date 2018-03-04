#ifndef DEAD_LOCK_CHECK_H
#define DEAD_LOCK_CHECK_H

#include "test_utils/test_utils.h"

class DeadLockPara : public ParamBase
{
public:
	static std::vector<EntrySetID> vEntrySetId;

	enum HARD_SOFT_EDGE
	{
		DEAD_LOCK_HARD_EDGE,
		DEAD_LOCK_SOFT_EDGE
	};

	HARD_SOFT_EDGE edge_flag;
};

class DeadLockHardEdgePara : public DeadLockPara
{
public:
	static EntrySetID rel_id1;
	static EntrySetID rel_id2;
};
class DeadLockSoftEdgePara : public DeadLockPara
{
public:
	static EntrySetID rel_id1;
	static EntrySetID rel_id2;
};
bool deadlock_create_two_heaps();
bool heap_level_hard_edge_deadlock_1();
bool heap_level_hard_edge_deadlock_2();
bool deadlock_drop_two_heaps();
bool deadlock_soft_edge_create_heap();
bool deadlock_soft_edge_trans1();
bool deadlock_soft_edge_trans2();
bool deadlock_soft_edge_trans3();
bool dead_lock_soft_edge_drop_heap();
//random deadlock test cases
bool deadlock_create_heaps();
bool deadlock_drop_heaps();
bool deadlock_trans1();
bool deadlock_trans2();
bool deadlock_trans3();
bool deadlock_trans4();
bool deadlock_trans5();
bool deadlock_trans6();
bool deadlock_trans7();
bool deadlock_trans8();
bool deadlock_trans9();
bool deadlock_trans10();
bool IsAllFinished();
#endif//DEAD_LOCK_CHECK_H