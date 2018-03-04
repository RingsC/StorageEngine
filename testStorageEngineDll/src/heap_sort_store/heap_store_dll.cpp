#include "test_fram.h"

#include "utils/utils_dll.h"
#include "heap_sort_store/heap_store_dll.h"
#include "PGSETypes.h"
#include "StorageEngine.h"
#include "StorageEngineException.h"
//#include "SortEntrySet.h"
#include "vector"
#include <string>
#include <iostream>
#include <algorithm>

bool test_heap_store_dll()
{
	INTENT("����PGTempEntrySet�ӿڵĻ������ܡ�");

	using namespace std;

	bool return_sta = true;
	try
	{
		StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		Transaction *pTrans = pStorageEngine->getTransaction(xid,	Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *entrySet = NULL;
		EntrySetID eid = pStorageEngine->createEntrySet(pTrans, GetSingleColInfo(), 0, NULL);
		entrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,eid);

		/* ����������� */
		vector<DataItem> v_di;
		/* 20+MB�������� */
		const int ROW = 100;
		DataGenerater dg(ROW, DATA_LEN);
		dg.dataGenerate();
		for(uint32 i = 0; i < ROW; ++i)
		{
			 DataItem di(dg[i], DATA_LEN);
			v_di.push_back(di);
		}
		entrySet->insertEntries(pTrans, v_di);
		pStorageEngine->endStatement();

		/* �򿪱�����ѯ���������ݷ���PGTempEntrySet�� */
		EntrySet *pTempEntrySet = pStorageEngine->createTempEntrySet(pTrans, EntrySet::SST_HeapTuples, 0, ROW * DATA_LEN, false, true);
		/// ʹ�õ�һ�ַ�ʽ��������������Ϊ����
		pTempEntrySet->insertEntries(pTrans, *entrySet);

		vector<ScanCondition> v_sc;
		EntrySetScan *ess = entrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotNOW,v_sc);
		EntryID ei;
		DataItem di;
		while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
		{
			/// ʹ�õڶ��ַ�ʽ��ʹ��һ��DataItem��Ϊ����
			pTempEntrySet->insertEntry(pTrans, ei, di);
		}
		entrySet->endEntrySetScan(ess);

		/// ʹ�õ����ַ�ʽ�� ʹ��һ��vector��Ϊ����
		pTempEntrySet->insertEntries(pTrans, v_di);
		pStorageEngine->closeEntrySet(pTrans, entrySet);
		pStorageEngine->removeEntrySet(pTrans, eid);

		/* ��ʱpTempEntrySet��ӵ����3��entrySet������ */
		ess = pTempEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotNOW,v_sc);
		vector<string> v_cmp1;
		vector<string> v_cmp_pos1, v_cmp_pos2;
		int32 index_pos = (ROW * 3 - 20);
		while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == SUCCESS)
		{
			v_cmp1.push_back(static_cast<char*>(di.getData()));
			if(index_pos == 0)
			{
				ess->markPosition();
			}
			if(index_pos < 0)
			{
				v_cmp_pos1.push_back(static_cast<char*>(di.getData()));
			}
			--index_pos;
		}
		ess->restorePosition();

		while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == SUCCESS)
		{
			v_cmp_pos2.push_back(static_cast<char*>(di.getData()));
		}
		pTempEntrySet->endEntrySetScan(ess);
		
		vector<string> v_cmp2;

		for(int i = 0; i < 3; ++i)
		{
			for(int j = 0; j < ROW; ++j)
			{
				v_cmp2.push_back(static_cast<char*>(v_di[j].getData()));
			}
		}
		sort(v_cmp1.begin(), v_cmp1.end());
		sort(v_cmp2.begin(), v_cmp2.end());
		sort(v_cmp_pos1.begin(), v_cmp_pos1.end());
		sort(v_cmp_pos2.begin(), v_cmp_pos2.end());
		return_sta = ( (v_cmp1 == v_cmp2 && v_cmp_pos1 == v_cmp_pos2) ? true : false);

		//delete pTempEntrySet;
		pTrans->commit();

	}catch(StorageEngineException &se)
	{
		printf("%s\n", se.getErrorMsg());
		return_sta = false;
	}

	return return_sta;
}

int sort_cmp_func(const char *data1, size_t len1, const char *data2, size_t len2)
{
	if(len1 != len2)
	{
		return (len1 > len2) ? 1 : -1;
	}

	return memcmp(data1, data2, len1);
}

bool test_heap_sort_dll()
{
	INTENT("����PGSortEntrySet�ӿڵĻ������ܡ�");

	using namespace std;

	bool return_sta = true;

	try
	{

		StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
		TransactionId xid = 0;
		Transaction *pTrans = pStorageEngine->getTransaction(xid,	Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *entrySet = NULL;
		EntrySetID eid = pStorageEngine->createEntrySet(pTrans, GetSingleColInfo(), 0, NULL);
		entrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,eid);

		/* ����������� */
		vector<DataItem> v_di;
		/* 20+MB�������� */
		const int ROW = 100;
		DataGenerater dg(ROW, DATA_LEN);
		dg.dataGenerate();
		for(uint32 i = 0; i < ROW; ++i)
		{
			DataItem di(dg[i], DATA_LEN);
			v_di.push_back(di);
		}
		entrySet->insertEntries(pTrans, v_di);
		pTrans->commit();

		//pSortEntrySet->initialize();
		xid = 0;
		pTrans = pStorageEngine->getTransaction(xid,	Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		/* �򿪱�����ѯ���������ݷ���PGSortEntrySet�� */
		SortEntrySet *pSortEntrySet = pStorageEngine->createSortEntrySet(pTrans, 
			EntrySet::SST_GenericDataTuples, 
			ROW * DATA_LEN, 
			true, 
			sort_cmp_func);
		SortEntrySet *pSortEntrySet2 = pStorageEngine->createSortEntrySet(pTrans,
			EntrySet::SST_HeapTuples,
			ROW * DATA_LEN,
			true,
			sort_cmp_func);
		entrySet = pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,eid);
		/// ʹ�õ�һ�ַ�ʽ��������������Ϊ����
		pSortEntrySet->insertEntries(pTrans, *entrySet);
		pSortEntrySet2->insertEntries(pTrans, *entrySet);
		pStorageEngine->closeEntrySet(pTrans, entrySet);

		/// ʹ�õڶ��ַ�ʽ������һ��vector��Ϊ����
		pSortEntrySet->insertEntries(pTrans, v_di);
		pSortEntrySet2->insertEntries(pTrans, v_di);

		/// ʹ�õ����ַ�ʽ������һ��DataItem��Ϊ����
		EntryID ei;
		for(int i = 0; i < v_di.size(); ++i)
		{
			pSortEntrySet->insertEntry(pTrans, ei, v_di[i]);
			pSortEntrySet2->insertEntry(pTrans, ei, v_di[i]);
		}

		/* ��ʱpSortEntrySetӵ��3��entrySet������ */
		pSortEntrySet->performSort(); /// ִ�������㷨
		pSortEntrySet2->performSort(); /// ִ�������㷨
		vector<ScanCondition> v_sc;
		vector<string> v_cmp1, v_cmp2, v_cmp3;
		vector<string> v_cmp_pos1, v_cmp_pos2;
		EntrySetScan *ess = pSortEntrySet->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,v_sc);
		DataItem di;
		int32 index_pos = ROW * 3 - 20;
		while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == SUCCESS)
		{
			v_cmp1.push_back(static_cast<char *>(di.getData()));
			if(index_pos == 0)
			{
				ess->markPosition();
			}
			if(index_pos < 0)
			{
				v_cmp_pos1.push_back(static_cast<char *>(di.getData()));
			}
			--index_pos;
		}
		EntrySetScan *ess2 = pSortEntrySet2->startEntrySetScan(pTrans,BaseEntrySet::SnapshotMVCC,v_sc);
		while(ess2->getNext(EntrySetScan::NEXT_FLAG, ei, di) == SUCCESS)
		{
			v_cmp3.push_back(static_cast<char *>(di.getData()));
		}

		ess->restorePosition();
		while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == SUCCESS)
		{
			v_cmp_pos2.push_back(static_cast<char *>(di.getData()));
		}
		pSortEntrySet->endEntrySetScan(ess);

		for(int i = 0; i < 3; ++i)
		{
			for(int j = 0; j < ROW; ++j)
			{
				v_cmp2.push_back(static_cast<char *>(v_di[j].getData()));
			}
		}
		sort(v_cmp2.begin(), v_cmp2.end());
		sort(v_cmp3.begin(), v_cmp3.end());
		sort(v_cmp_pos1.begin(), v_cmp_pos1.end());
		sort(v_cmp_pos2.begin(), v_cmp_pos2.end());
		return_sta = ( (v_cmp1 == v_cmp2 && 
										v_cmp1 == v_cmp3 && 
										v_cmp_pos1 == v_cmp_pos2) ? true : false);

		pStorageEngine->removeEntrySet(pTrans, eid);

		delete pSortEntrySet;
		pTrans->commit();

	}catch(StorageEngineException &se)
	{
		printf("%s\n", se.getErrorMsg());
		return_sta = false;
	}

	return return_sta;
}