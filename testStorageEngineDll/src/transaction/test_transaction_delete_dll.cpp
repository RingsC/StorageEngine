/************************************************************************


���Ե��б�
���		��д��      �޸�����      �޸�ʱ��		�޸�����		
000			����      ����		  2011.9.19		 ��������ԭ���ԣ��Ȳ������ݣ�Ȼ��ɾ��ȫ�����ݣ���һ�������ѯ�Ƿ���ȷɾ��
001			����      ����		  2011.9.19		 ��������ԭ���ԣ��Ȳ������ݣ�Ȼ��ɾ����������ʱȡ����������һ���������Ƿ���ȷ�ع�
002			����      ����		  2011.9.19		 ��������ԭ���ԣ��Ȳ������ݣ�Ȼ��ɾ����������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�(δ���)

************************************************************************/

#include <boost/thread/thread.hpp>

#include <iostream>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "transaction/test_transaction_delete_dll.h"

using namespace FounderXDB::StorageEngineNS;

extern char *my_itoa(int value, char *string, int radix);

#define TRANSACTION_ROWS_DELETE 10
bool test_transaction_delete_dll_000()
{
	INTENT("����ɾ������ԭ���ԣ�ɾ��ȫ��������ɺ���һ�������ѯ�Ƿ���ȷɾ����");
	try
 	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//�����DATAROWS������λ����Ϊ�˷�����ʵ��ڴ��С
		for(int i = TRANSACTION_ROWS_DELETE; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//�ó����Ѱ���'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS_DELETE][20];
		for( ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
		{
			memcpy(insert_str, "testdata_", sizeof("testdata_"));//ÿ�ν���ѭ��memcpyһ�Σ���֤ÿ�ζ��Ǵӳ�ʼ�ַ�����ʼ�����ַ�
			my_itoa(ix,string,10);
			strcat(insert_str,string);
			memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
			DataItem *data = new DataItem;
			data->setData((void*)copy_insert_data[ix]);
			len = strlen((char*)insert_str);//���Ȳ�����'\0'
			data->setSize(len+1);//Ϊ'\0'�����һ���ֽ�
			dvec.push_back(data);
			pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//			cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
 		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pDeleteData = new DataItem;
		EntryID delete_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		//ɾ������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,delete_tid,*pDeleteData) == 0 )//*pΪdataitem���ͣ��������
		{
			pEntrySet->deleteEntry(pTransaction,delete_tid);
		}
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();
	
		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//���ɾ��������ȷִ�У�Ӧ�ò�ѯ��������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
		{
			++counter;
		}
		CHECK_BOOL(counter == 0 );
		if(counter != 0)
		{
			cout << "�����������ѯ����������һ��!" << endl;
			cout << "��ѯ��������counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}
		
		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		delete pDeleteData;
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//�ύ����
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_delete_dll_001()
{
	INTENT("��������ԭ���ԣ��Ȳ������ݣ�Ȼ��ɾ����������ʱȡ����������һ���������Ƿ���ȷ�ع�");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//�����DATAROWS������λ����Ϊ�˷�����ʵ��ڴ��С
		for(int i = TRANSACTION_ROWS_DELETE; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//�ó����Ѱ���'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS_DELETE][20];
		for( ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
		{		
			memcpy(insert_str, "testdata_", sizeof("testdata_"));//ÿ�ν���ѭ��memcpyһ�Σ���֤ÿ�ζ��Ǵӳ�ʼ�ַ�����ʼ�����ַ�
			my_itoa(ix,string,10);
			strcat(insert_str,string);
			memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
			DataItem *data = new DataItem;
			data->setData((void*)copy_insert_data[ix]);
			len = strlen((char*)insert_str);//���Ȳ�����'\0'
			data->setSize(len+1);//Ϊ'\0'�����һ���ֽ�
			dvec.push_back(data);
			pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//			cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pDeleteData = new DataItem;
		EntryID delete_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		ix = 0;
		//ɾ������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,delete_tid,*pDeleteData) == 0 )//*pΪdataitem���ͣ��������
		{
			//ɾ���������ݺ���ֹ��ǰ����
			if(ix == TRANSACTION_ROWS_DELETE/2)
			{
				pEntrySet->endEntrySetScan(pHeapScan);
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				user_abort_transaction();
				break;
			}
			pEntrySet->deleteEntry(pTransaction,delete_tid);
			++ix;
		}
		
		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//���ɾ��������ȷ�ع���Ӧ�ò�ѯ��ȫ����������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
		{
			++counter;
		}
		CHECK_BOOL( counter == TRANSACTION_ROWS_DELETE );
		if( counter != TRANSACTION_ROWS_DELETE )
		{
			cout << "�����������ѯ����������һ��!" << endl;
			cout << "��ѯ��������counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData;
		delete pDeleteData;
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//�ύ����
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_delete_dll_002_step1()
{
	INTENT("��������ԭ���ԣ��Ȳ������ݣ�Ȼ��ɾ����������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//�����DATAROWS������λ����Ϊ�˷�����ʵ��ڴ��С
		for(int i = TRANSACTION_ROWS_DELETE; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//�ó����Ѱ���'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS_DELETE][20];
		for( ix = 0; ix != TRANSACTION_ROWS_DELETE; ++ix)
		{		
			memcpy(insert_str, "testdata_", sizeof("testdata_"));//ÿ�ν���ѭ��memcpyһ�Σ���֤ÿ�ζ��Ǵӳ�ʼ�ַ�����ʼ�����ַ�
			my_itoa(ix,string,10);
			strcat(insert_str,string);
			memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
			DataItem *data = new DataItem;
			data->setData((void*)copy_insert_data[ix]);
			len = strlen((char*)insert_str);//���Ȳ�����'\0'
			data->setSize(len+1);//Ϊ'\0'�����һ���ֽ�
			dvec.push_back(data);
			pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
		}
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pDeleteData = new DataItem;
		EntryID delete_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		ix = 0;
		//ɾ������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,delete_tid,*pDeleteData) == 0 )//*pΪdataitem���ͣ��������
		{
			//ɾ���������ݺ���ֹ��ǰ����
			if(ix == TRANSACTION_ROWS_DELETE/2)
			{
				pEntrySet->endEntrySetScan(pHeapScan);
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				exit(1);
			}
			pEntrySet->deleteEntry(pTransaction,delete_tid);
			++ix;
		}
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_delete_dll_002_step2()
{
	INTENT("��������ԭ���ԣ��Ȳ������ݣ�Ȼ��ɾ����������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		
		//���ɾ��������ȷ�ع���Ӧ�ò�ѯ��ȫ����������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
		{
			++counter;
		}
		CHECK_BOOL( counter == TRANSACTION_ROWS_DELETE );
		if( counter != TRANSACTION_ROWS_DELETE )
		{
			cout << "�����������ѯ����������һ��!" << endl;
			cout << "��ѯ��������counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		delete pScanData;
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//�ύ����
	} 
	CATCHEXCEPTION
		return true;
}

DLL_SIMPLE_EXTERN_FUNC

int test_trans_persistence_update()
{
	INTENT("���������������ݣ����ɾ���������ݣ��������̡�"
				 "�������̣������Ƿ�������ȷɾ����û��ɾ����������"
				 "�񻹴��ڡ�");

	DLL_PREPARE_TEST();

	const string T_DATA = "diedurmjdifjrm%%%$$$###@@@!!!**&&djeimd";
	const string DET_DATA = "det$$$$$$$$$$$$$JJ";

#undef ARR_LEN 
#define ARR_LEN 100

	SpliterGenerater sg;
	ColumnInfo *colinfo = sg.buildHeapColInfo(2, 1, 3);
	int col_id = get_col_id();
	setColumnInfo(col_id, colinfo);
	char data[ARR_LEN][DATA_LEN];
	memset(data, 0, ARR_LEN * DATA_LEN);

	/* ������������ */
	for(int i = 0; i < ARR_LEN; ++i)
	{
		memcpy(data[i], T_DATA.c_str(), T_DATA.length());
	}

	/*
	* ׼������
	* �������ݲ��˳�����
	*/
	SHUTDOWN_TEST_STEP_1(TransPersistence)
	{ 
		EntrySetID ei;
		DLL_SIMPLE_CREATE_HEAP(ei, col_id);
		SAVE_HEAP_ID_FOR_NEXT(ei);
		int insert_data = 0;
		/* �������� */
		DLL_SIMPLE_INSERT_DATA(ei, data, ARRAY_LEN_CALC(data), insert_data);
		int update_data = 0;
		/* �������ݣ������е����ݸ��³�DET_DATA */
		DLL_SIMPLE_UPDATE_DATA(ei, DET_DATA.c_str(), DET_DATA.length(), T_DATA.c_str(), T_DATA.length(), update_data);
		return true;
	}
	/*
	* �������
	*/
	SHUTDOWN_TEST_STEP_2()
	int test_success = false;
	vector<string> v_src, v_det;
	for(int i = 0; i < ARR_LEN; ++i)
	{
		v_src.push_back(DET_DATA);
	}
	ShutdownMap m_hi;
	GET_RELATION_INFO(m_hi);
	Transaction *trans = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	EntrySet *es = (EntrySet*)pStorageEngine->openEntrySet(trans, EntrySet::OPEN_EXCLUSIVE, m_hi.begin()->first, NULL);
	vector<ScanCondition> vc;
	EntrySetScan *ess = es->startEntrySetScan(trans,BaseEntrySet::SnapshotMVCC,vc);
	EntryID ei;
	DataItem di;
	/* ɨ��entryset�����������ݷ���vector�� */
	while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
	{
		v_det.push_back((char*)di.getData());
	}
	es->endEntrySetScan(ess);
	pStorageEngine->closeEntrySet(trans, es);
	pStorageEngine->removeEntrySet(trans, es->getId());
	trans->commit();

	/* �Ƚ������Ƿ���ȷ */
	sort(v_src.begin(), v_src.end());
	sort(v_det.begin(), v_det.end());
	test_success = (v_src == v_det);

	CLEAN_SHUTDOWN_TEST();
	return test_success;
	END_SHUTDOWN_TEST()
}

int test_trans_persistence_delete()
{
	INTENT("�������������ݣ����ɾ���������ݣ��������̡�"
				 "�������̣�ɨ�������δɾ���������Ƿ񻹴��ڡ�");

	DLL_PREPARE_TEST();

	const string D_DATA = "delete me";
	const string N_DATA = "do not delete me";

#undef ARR_LEN 
#define ARR_LEN 100

	SpliterGenerater sg;
	ColumnInfo *colinfo = sg.buildHeapColInfo(2, 1, 3);
	int col_id = get_col_id();
	setColumnInfo(col_id, colinfo);
	char data[ARR_LEN][DATA_LEN];
	memset(data, 0, ARR_LEN * DATA_LEN);

	/* ������������,����ARR_LEN - 2������������������δɾ�������� */
	for(int i = 0; i < ARR_LEN - 2; ++i)
	{
		memcpy(data[i], D_DATA.c_str(), D_DATA.length());
	}

	/* ����δɾ������ */
	memcpy(data[ARR_LEN - 2], N_DATA.c_str(), N_DATA.length());
	memcpy(data[ARR_LEN - 1], N_DATA.c_str(), N_DATA.length());

	/*
	* ׼������
	* �������ݲ��˳�����
	*/
	SHUTDOWN_TEST_STEP_1(TransPersistence)
	{ 
		EntrySetID ei;
		DLL_SIMPLE_CREATE_HEAP(ei, col_id);
		SAVE_HEAP_ID_FOR_NEXT(ei);
		int insert_data = 0;
		/* �������� */
		DLL_SIMPLE_INSERT_DATA(ei, data, ARRAY_LEN_CALC(data), insert_data);
		int delete_data = 0;
		/* ɾ�����е�D_DATA,�������е�N_DATA */
		DLL_SIMPLE_DELETE_DATA(ei, D_DATA.c_str(), D_DATA.length(), delete_data);
		return true;
	}
	/*
	* �������
	*/
	SHUTDOWN_TEST_STEP_2()
	int test_success = false;
	vector<string> v_src, v_det;
	for(int i = 0; i < 2; ++i)
	{
		v_src.push_back(N_DATA);
	}
	ShutdownMap m_hi;
	GET_RELATION_INFO(m_hi);
	Transaction *trans = start_new_transaction(Transaction::READ_COMMITTED_ISOLATION_LEVEL);
	EntrySet *es = (EntrySet*)pStorageEngine->openEntrySet(trans, EntrySet::OPEN_EXCLUSIVE, m_hi.begin()->first, NULL);
	vector<ScanCondition> vc;
	EntrySetScan *ess = es->startEntrySetScan(trans,BaseEntrySet::SnapshotMVCC,vc);
	EntryID ei;
	DataItem di;
	/* ɨ��entryset�����������ݷ���vector�� */
	while(ess->getNext(EntrySetScan::NEXT_FLAG, ei, di) == 0)
	{
		v_det.push_back((char*)di.getData());
	}
	es->endEntrySetScan(ess);
	pStorageEngine->closeEntrySet(trans, es);
	pStorageEngine->removeEntrySet(trans, es->getId());
	trans->commit();

	/* �Ƚ������Ƿ���ȷ */
	sort(v_src.begin(), v_src.end());
	sort(v_det.begin(), v_det.end());
	test_success = (v_src == v_det);

	CLEAN_SHUTDOWN_TEST();
	return test_success;
	END_SHUTDOWN_TEST()
}