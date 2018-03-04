#include <fstream>
#include "heap/test_heap_create.h"
//#include "pthread.h"
#include "DeadLock/deadlock.h"
#include "boost/thread.hpp"
/************************************************************************/
/* 首先，创建两个表rel1、rel2，
然后，两个事务trans1、trans2在不同的线程中要求同时打开两个表，
线程thread1获得了rel1要求打开rel2，同时，线程thread2打开了rel2要求打开
rel1，
此时，出现HardEdge死锁。
*/
/************************************************************************/

EntrySetID DeadLockHardEdgePara::rel_id1 = InvalidEntrySetID;
EntrySetID DeadLockHardEdgePara::rel_id2 = InvalidEntrySetID;

bool deadlock_create_two_heaps_task(ParamBase* arg)
{
	DeadLockPara* pArgs = (DeadLockPara*)arg;

	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		switch(pArgs->edge_flag)
		{
		case DeadLockPara::DEAD_LOCK_HARD_EDGE:
			DeadLockHardEdgePara::rel_id1 = pStorageEngine->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());
			DeadLockHardEdgePara::rel_id2 = pStorageEngine->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());
			break;
		case DeadLockPara::DEAD_LOCK_SOFT_EDGE:
			DeadLockSoftEdgePara::rel_id1 = pStorageEngine->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());
			DeadLockSoftEdgePara::rel_id2 = pStorageEngine->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());
			break;
		default:
			break;
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	MySleep(1000);
	return true;
}
bool deadlock_drop_two_heaps_task(ParamBase* arg)
{
	MySleep(2000);
	DeadLockPara* pArgs = (DeadLockPara*)arg;

	try
	{
		StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();
		switch (pArgs->edge_flag)
		{
		case DeadLockPara::DEAD_LOCK_HARD_EDGE:
			pStorageEngine->removeEntrySet(pArgs->GetTransaction(),DeadLockHardEdgePara::rel_id1);
			pStorageEngine->removeEntrySet(pArgs->GetTransaction(),DeadLockHardEdgePara::rel_id2);
			break;
		case DeadLockPara::DEAD_LOCK_SOFT_EDGE:
			while (!IsAllFinished()){ }//等待所有的任务都结束
			pStorageEngine->removeEntrySet(pArgs->GetTransaction(),DeadLockSoftEdgePara::rel_id1);
			pStorageEngine->removeEntrySet(pArgs->GetTransaction(),DeadLockSoftEdgePara::rel_id2);
			break;
		default:
			break;
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}

bool bTrans1 = false;
bool bTrans2 = false;
bool bDeadLock = false;

bool heap_level_hard_edge_deadlock_1_task_1(ParamBase* arg)
{
	DeadLockHardEdgePara* pArgs = (DeadLockHardEdgePara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();
		EntrySet* pEntrySet1 = pSE->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,DeadLockHardEdgePara::rel_id1);
		if (!pEntrySet1)
		{
			pArgs->SetSuccessFlag(false);
			return false;
		}

		bTrans2 = true;//通知trans2，trans1已经获得rel1的排他锁
	}
    EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool heap_level_hard_edge_deadlock_1_task_2(ParamBase* arg)
{
	DeadLockHardEdgePara* pArgs = (DeadLockHardEdgePara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();
		
		while (!bTrans1)
		{
			MySleep(100);
		}

		EntrySet* pEntrySet2 = pSE->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,DeadLockHardEdgePara::rel_id2);

		if (!bDeadLock)
		{
			pArgs->SetSuccessFlag(false);
		}

		if (!pEntrySet2)
		{
			pArgs->SetSuccessFlag(false);
		}
	}
	catch(DeadLockException &ex)
	{
		if (bDeadLock)
		{
			pArgs->SetSuccessFlag(false);
			return false;
		}
		bDeadLock = true;
		std::cout<<ex.getErrorMsg()<<std::endl;
	}

	return true;
}


bool heap_level_hard_edge_deadlock_2_task_1(ParamBase* arg)
{
	DeadLockHardEdgePara* pArgs = (DeadLockHardEdgePara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();
		EntrySet* pEntrySet2 = pSE->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,DeadLockHardEdgePara::rel_id2);
		if (!pEntrySet2)
		{
			pArgs->SetSuccessFlag(false);
			return false;
		}

		bTrans1 = true;//通知trans1，trans2已经获得rel2的排他锁
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool heap_level_hard_edge_deadlock_2_task_2(ParamBase* arg)
{
	DeadLockHardEdgePara* pArgs = (DeadLockHardEdgePara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();
		while (!bTrans2)
		{
			MySleep(100);
		}

		//打开rel_id2
		EntrySet* pEntrySet1 = pSE->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,DeadLockHardEdgePara::rel_id1);

		if (!bDeadLock)
		{
			pArgs->SetSuccessFlag(false);
		}

		if (!pEntrySet1)
		{
			pArgs->SetSuccessFlag(false);
		}
	}
	catch(DeadLockException &ex)
	{
		if(bDeadLock)
		{
			pArgs->SetSuccessFlag(false);
			return false;
		}
		bDeadLock = true;
		std::cout<<ex.getErrorMsg()<<std::endl;
	}

	return true;
}

const int TRANS_WAIT_DEADLOCK = GetTransWaitId();
bool deadlock_create_two_heaps()
{
	INITRANID()
	DeadLockHardEdgePara* arg = new DeadLockHardEdgePara;
	arg->edge_flag = DeadLockPara::DEAD_LOCK_HARD_EDGE;
	REGTASKSEQ(deadlock_create_two_heaps_task,arg,1,TRANS_WAIT_DEADLOCK);
	return true;
}
bool heap_level_hard_edge_deadlock_1()
{
	INITRANID()
	DeadLockHardEdgePara* arg = new DeadLockHardEdgePara;
	REGTASKSEQ(heap_level_hard_edge_deadlock_1_task_1,arg,2,TRANS_WAIT_DEADLOCK);
	REGTASKSEQ(heap_level_hard_edge_deadlock_1_task_2,arg,2,TRANS_WAIT_DEADLOCK);
	return true;
}
bool heap_level_hard_edge_deadlock_2()
{
	INITRANID()
	DeadLockHardEdgePara* arg = new DeadLockHardEdgePara;
	REGTASKSEQ(heap_level_hard_edge_deadlock_2_task_1,arg,2,TRANS_WAIT_DEADLOCK);
	REGTASKSEQ(heap_level_hard_edge_deadlock_2_task_2,arg,2,TRANS_WAIT_DEADLOCK);
	return true;
}
bool deadlock_drop_two_heaps()
{
	INITRANID()
	DeadLockHardEdgePara* arg = new DeadLockHardEdgePara;
	arg->edge_flag = DeadLockPara::DEAD_LOCK_HARD_EDGE;
	REGTASKSEQ(deadlock_drop_two_heaps_task,arg,3,TRANS_WAIT_DEADLOCK);
	return true;
}

/************************************************************************/
/*                        Soft-Edge DeadLock                            */

/*三个事务trans1、trans2、trans3，两个已经存在的表rel1、rel2，
  事务在不同的线程并发执行，此时，trans1持有rel1的排他锁，trans3持有rel2的共享锁，
  trans3申请rel1的排他锁，trans1、trans2同时申请rel2的共享锁、排他锁，
  而且trans1排在trans2后面，
  这样就会出现带soft-edge的环，通过拓扑排序可以解除死锁.
                                                                        */
/************************************************************************/
EntrySetID DeadLockSoftEdgePara::rel_id1 = InvalidEntrySetID;
EntrySetID DeadLockSoftEdgePara::rel_id2 = InvalidEntrySetID;

bool trans1_rel1 = false;//表示trans1已经取得rel1
bool trans2_rel2_will = false;//表示trans2开始申请rel2
bool trans3_rel2 = false;//表示trans3已经取得rel2
int nSoftLockTask = 0;
void IncreaseTaskFinishCount()
{
	boost::mutex mu;
	mu.lock();
	nSoftLockTask ++;
	mu.unlock();
}
bool IsAllFinished()
{
	if (5 == nSoftLockTask)
		return true;
	return false;
}
bool deadlock_soft_edge_trans1_task_1(ParamBase* arg)
{
	DeadLockSoftEdgePara* pArgs = (DeadLockSoftEdgePara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();
		EntrySet* pEntrySet1 = pSE->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,DeadLockSoftEdgePara::rel_id1);
		if (!pEntrySet1)
		{
			std::cout<<"open rel1 failed !"<<std::endl;
			pArgs->SetSuccessFlag(false);
			return true;
		}

		trans1_rel1 = true;
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)
	IncreaseTaskFinishCount();

	return true;
}
bool deadlock_soft_edge_trans1_task_2(ParamBase* arg)
{
	DeadLockSoftEdgePara* pArgs = (DeadLockSoftEdgePara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();
		
		while (!trans3_rel2 && !trans2_rel2_will)//trans3持有rel2，trans2已经申请rel2
		{
			MySleep(100);
		}

		//持有rel1后，再申请rel2，此时rel2被trans3持有
		EntrySet* pEntrySet2 = pSE->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_SHARED,DeadLockSoftEdgePara::rel_id2);
		if (!pEntrySet2)
		{
			pArgs->SetSuccessFlag(false);
			return false;
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)
    IncreaseTaskFinishCount();
	return true;
}
bool deadlock_soft_edge_trans2_task(ParamBase* arg)
{
	DeadLockSoftEdgePara* pArgs = (DeadLockSoftEdgePara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();
		while (!trans1_rel1 && !trans3_rel2)
		{
			//MySleep(10);
		}

		trans2_rel2_will = true;

		EntrySet* pEntrySet2 = pSE->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,DeadLockSoftEdgePara::rel_id2);
		if (!pEntrySet2)
		{
			std::cout<<"open rel1 failed !"<<std::endl;
			pArgs->SetSuccessFlag(false);
			return true;
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	IncreaseTaskFinishCount();
	return true;
}
bool deadlock_soft_edge_trans3_task_1(ParamBase* arg)
{
	DeadLockSoftEdgePara* pArgs = (DeadLockSoftEdgePara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();
		EntrySet* pEntrySet2 = pSE->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_SHARED,DeadLockSoftEdgePara::rel_id2);
		if (!pEntrySet2)
		{
			std::cout<<"open rel1 failed !"<<std::endl;
			pArgs->SetSuccessFlag(false);
			return true;
		}

		trans3_rel2 = true;
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	IncreaseTaskFinishCount();
	return true;
}
bool deadlock_soft_edge_trans3_task_2(ParamBase* arg)
{
	DeadLockSoftEdgePara* pArgs = (DeadLockSoftEdgePara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();

		//持有rel2后，再申请rel1，此时rel1被trans1持有
		EntrySet* pEntrySet1 = pSE->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,DeadLockSoftEdgePara::rel_id1);
		if (!pEntrySet1)
		{
			pArgs->SetSuccessFlag(false);
			return false;
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	IncreaseTaskFinishCount();
	return true;
}

const int TRANS_WAIT_DEAD_LOCK_SOFT_EDGE = GetTransWaitId();
bool deadlock_soft_edge_create_heap()
{
	INITRANID()
	DeadLockSoftEdgePara* arg = new DeadLockSoftEdgePara;
	arg->edge_flag = DeadLockPara::DEAD_LOCK_SOFT_EDGE;
	REGTASKSEQ(deadlock_create_two_heaps_task,arg,1,TRANS_WAIT_DEAD_LOCK_SOFT_EDGE);

	return true;
}
bool deadlock_soft_edge_trans1()
{
	INITRANID()
	DeadLockSoftEdgePara* arg = new DeadLockSoftEdgePara;
	arg->edge_flag = DeadLockPara::DEAD_LOCK_SOFT_EDGE;
	REGTASKSEQ(deadlock_soft_edge_trans1_task_1,arg,2,TRANS_WAIT_DEAD_LOCK_SOFT_EDGE);
	REGTASKSEQ(deadlock_soft_edge_trans1_task_2,arg,2,TRANS_WAIT_DEAD_LOCK_SOFT_EDGE);
	return true;
}
bool deadlock_soft_edge_trans2()
{
	INITRANID()
	DeadLockSoftEdgePara* arg = new DeadLockSoftEdgePara;
	arg->edge_flag = DeadLockPara::DEAD_LOCK_SOFT_EDGE;
	REGTASKSEQ(deadlock_soft_edge_trans2_task,arg,2,TRANS_WAIT_DEAD_LOCK_SOFT_EDGE);
	return true;
}
bool deadlock_soft_edge_trans3()
{
	INITRANID()
	DeadLockSoftEdgePara* arg = new DeadLockSoftEdgePara;
	arg->edge_flag = DeadLockPara::DEAD_LOCK_SOFT_EDGE;
	REGTASKSEQ(deadlock_soft_edge_trans3_task_1,arg,2,TRANS_WAIT_DEAD_LOCK_SOFT_EDGE);
	REGTASKSEQ(deadlock_soft_edge_trans3_task_2,arg,2,TRANS_WAIT_DEAD_LOCK_SOFT_EDGE);
	return true;
}
bool dead_lock_soft_edge_drop_heap()
{
	INITRANID()
	DeadLockSoftEdgePara* arg = new DeadLockSoftEdgePara;
	arg->edge_flag = DeadLockPara::DEAD_LOCK_SOFT_EDGE;
	REGTASKSEQ(deadlock_drop_two_heaps_task,arg,3,TRANS_WAIT_DEAD_LOCK_SOFT_EDGE);

	return true;
}

/************************************************************************/
/* 创建大量的表，然后随机的让不同的事务，不同的任务，打开不同的表       */
/************************************************************************/
const int HEAP_COUNT = 10;
std::vector<EntrySetID> DeadLockPara::vEntrySetId;
//pthread_mutex_t thread_mutex;
int nTaskFinishCount = 0;

//生产随机数
int MyRandom(int nBegin, int nEnd)
{
	//MySleep(100);
	srand((unsigned int)time(NULL));

	//return nBegin + (nEnd - nBegin)*rand()/(RAND_MAX + 1);
	return nBegin + rand()%(nEnd - nBegin);
}
bool deadlock_create_heaps_task(ParamBase* arg)
{
	DeadLockPara* pArgs = (DeadLockPara*)arg;
	try
	{
		DeadLockPara::vEntrySetId.reserve(HEAP_COUNT);

		StorageEngine* pSE = StorageEngine::getStorageEngine();

		for (int nIndex = 0; nIndex < HEAP_COUNT; nIndex++)
		{
			EntrySetID eid = InvalidEntrySetID;
			eid = pSE->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());
			DeadLockPara::vEntrySetId.push_back(eid);
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool deadlock_drop_heaps_task(ParamBase* arg)
{
	while (nTaskFinishCount != 100){}

	DeadLockPara* pArgs = (DeadLockPara*)arg;

	std::vector<EntrySetID>::iterator it;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();

		for (it = DeadLockPara::vEntrySetId.begin(); it != DeadLockPara::vEntrySetId.end(); it ++)
		{
			EntrySetID eid = *it;
			pSE->removeEntrySet(pArgs->GetTransaction(),eid);
		}

		DeadLockPara::vEntrySetId.clear();
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
const std::string strBase = "a";
const int nTupleCount = 100;
const int nTupleMaxLen = 100;
bool deadlock_heap_random_task_insert(EntrySet* pEntrySet, Transaction* pTrans)
{
	std::vector<DataItem> vData;
	for (int nIndex1 = 0; nIndex1 < nTupleCount; nIndex1 ++)
	{
		int nTupleLen = MyRandom(0,nTupleMaxLen);
		std::string strData;
		for (int nIndex = 0; nIndex < nTupleLen; nIndex ++)
		{
			strData += strBase;
		}
		DataItem data;
		data.setData((void*)strData.c_str());
		data.setSize(strData.size());
		vData.push_back(data);
	}

	pEntrySet->insertEntries(pTrans, vData);

	return true;
}
bool deadlock_heap_random_task_query(EntrySet* pEntrySet, Transaction* pTrans)
{
	std::vector<ScanCondition> condition;
	EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,condition);
	DataItem item;
	EntryID eid;
	while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
	{
		std::string str = (char*)item.getData();
	}
	pEntrySet->endEntrySetScan(pScan);
	return true;
}
bool deadlock_heap_random_task_update(EntrySet* pEntrySet, Transaction* pTrans)
{
	int nTupleLen = MyRandom(0,nTupleMaxLen);
	std::string strUpDate;
	for (int nIndex = 0; nIndex < nTupleMaxLen; nIndex ++)
	{
		strUpDate += strBase;
	}

	std::vector<ScanCondition> condition;
	EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,condition);
	
	bool found = false;
	EntryID eid;
	DataItem item;
	while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
	{
		std::string strSrc = (char*)item.getData();
		if (strSrc == strUpDate)
		{
			found = true;
			break;
		}
	}

	pEntrySet->endEntrySetScan(pScan);

	if (found)
        pEntrySet->updateEntry(pTrans,eid,item);

	return true;
}
bool deadlock_heap_random_task_delete(EntrySet* pEntrySet, Transaction* pTrans)
{
	int nTupleLen = MyRandom(0,nTupleMaxLen);
	std::string strUpDate;
	for (int nIndex = 0; nIndex < nTupleMaxLen; nIndex ++)
	{
		strUpDate += strBase;
	}

	std::vector<ScanCondition> condition;
	EntrySetScan* pScan = pEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,condition);

	std::list<EntryID> listId;
	EntryID eid;
	DataItem item;
	while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
	{
		std::string strSrc = (char*)item.getData();
		if (strSrc == strUpDate)
		{
			listId.push_back(eid);
		}
	}
	pEntrySet->endEntrySetScan(pScan);

	pEntrySet->deleteEntries(pTrans,listId);

	return true;
}

bool deadlock_open_heap_random_task(ParamBase* arg)
{
    DeadLockPara* pArgs = (DeadLockPara*)arg;
	try
	{
		StorageEngine* pSE = StorageEngine::getStorageEngine();

		int nPos = MyRandom(0,HEAP_COUNT);
		if (nPos < HEAP_COUNT)
		{
			EntrySet::EntrySetOpenFlags flag = EntrySet::OPEN_EXCLUSIVE;
			if (0 == nPos%2)
				flag = EntrySet::OPEN_SHARED;//奇数用排它锁打开，偶数用共享锁打开
			
			EntrySet* pEntrySet = pSE->openEntrySet(pArgs->GetTransaction(), flag, DeadLockPara::vEntrySetId.at(nPos));

			//插入数据
			deadlock_heap_random_task_insert(pEntrySet,pArgs->GetTransaction());
			//查询数据
			deadlock_heap_random_task_query(pEntrySet,pArgs->GetTransaction());
			//更新数据
            deadlock_heap_random_task_update(pEntrySet,pArgs->GetTransaction());
			//删除数据
			deadlock_heap_random_task_delete(pEntrySet,pArgs->GetTransaction());			

			static bool nFlag = false;

			int nid = (int)DeadLockPara::vEntrySetId.at(nPos);
			char charId[100] = {0};
			sprintf(charId,"open heap id : %d",nid);
			std::string strName = "e:/deadlock_open_heap_list.txt";
			std::fstream fWrite(strName.c_str(),std::ios_base::app|std::ios_base::out|std::ios_base::in);
			if (fWrite.is_open())
			{
				if (!nFlag)
				{
					nFlag = true;
					fWrite<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"<<std::endl;

				}
				fWrite<<charId<<std::endl;
			}
		}
	}
	catch(DeadLockException &ex)
	{
		std::cout<<ex.getErrorMsg()<<std::endl;
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	{
		boost::mutex mu;
		boost::mutex::scoped_lock lock(mu);
		nTaskFinishCount ++;
	}

	//pthread mutex lock method
	//pthread_mutex_init(&thread_mutex,NULL);
	//pthread_mutex_lock(&thread_mutex);
	//nTaskFinishCount ++;
	//pthread_mutex_unlock(&thread_mutex);
	//pthread_mutex_destroy(&thread_mutex);
	return true;
}
const int TRANS_WAIT_DEAD_LOCK_MANY = GetTransWaitId();
bool deadlock_create_heaps()
{
	INITRANID()
	DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_create_heaps_task,arg,1,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans1()
{
	INITRANID()
	DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans2()
{
	INITRANID()
		DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans3()
{
	INITRANID()
		DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans4()
{
	INITRANID()
		DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans5()
{
	INITRANID()
		DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans6()
{
	INITRANID()
		DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans7()
{
	INITRANID()
		DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans8()
{
	INITRANID()
		DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans9()
{
	INITRANID()
		DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}
bool deadlock_trans10()
{
	INITRANID()
		DeadLockPara* arg = new DeadLockPara;
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	REGTASKSEQ(deadlock_open_heap_random_task,arg,2,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}

bool deadlock_drop_heaps()
{
	INITRANID()
    DeadLockPara* arg = new DeadLockPara;
    REGTASKSEQ(deadlock_drop_heaps_task,arg,3,TRANS_WAIT_DEAD_LOCK_MANY);
	return true;
}