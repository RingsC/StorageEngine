/************************************************************************


���Ե��б�
���		��д��      �޸�����      �޸�ʱ��		�޸�����		
000			����      ����		  2011.9.14		 ��������ԭ���ԣ����벿������ʱȡ����������һ���������Ƿ���ȷ�ع�
001			����      ����		  2011.9.14		 ��������ԭ���ԣ�����ȫ��������ɺ���һ�������ѯ�Ƿ���ȷ����
002			����      ����		  2011.9.14		 ��������ԭ���ԣ����벿������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�(δ���)
003			����      ����		  2011.9.20		 ��������ԭ���ԣ��Ȳ���һЩ���ݲ��ύ���ٲ��벿������ʱȡ����������һ���������Ƿ���ȷ�ع�

************************************************************************/
#include <boost/thread/thread.hpp>

#include <iostream>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "transaction/test_transaction_insert_dll.h"
using namespace FounderXDB::StorageEngineNS;

extern char *my_itoa(int value, char *string, int radix);

bool test_transaction_insert_dll_000()
{
	INTENT("��������ԭ���ԣ����벿������ʱȡ����������һ���������Ƿ���ȷ�ع�");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//�����DATAROWS������λ����Ϊ�˷�����ʵ��ڴ��С
		for(int i = TRANSACTION_ROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//�ó����Ѱ���'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			//���벿�����ݺ���ֹ��ǰ����
			if(ix == TRANSACTION_ROWS/2)
			{
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				free(insert_str);
				user_abort_transaction();
				
				break;
			}
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

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//���������ȷ�ع���Ӧ�ò�ѯ��������
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

		
		for(ix = 0; ix != TRANSACTION_ROWS/2; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//�ύ����
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_insert_dll_001()
{
	INTENT("��������ԭ���ԣ�����ȫ��������ɺ���һ�������ѯ�Ƿ���ȷ����");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//�����DATAROWS������λ����Ϊ�˷�����ʵ��ڴ��С
		for(int i = TRANSACTION_ROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//�ó����Ѱ���'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
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
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;
		ix = 0;
		//���������ȷ��ɣ�Ӧ�ò�ѯ��ȫ����������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
		{
			data_flag = memcmp(dvec[ix]->getData(),pScanData->getData(),pScanData->getSize());
			//CHECK_BOOL(data_flag == 0);
			if(data_flag != 0)
			{
				cout << "�����������ѯ�������ݲ�һ��!" << endl;
				cout << "��������Ϊ" << (char*)dvec[ix]->getData() << endl;	
				cout << "��ѯ����Ϊ" << (char*)pScanData->getData() << endl;
				HEAP_RETURN_FALSE
			}

			size_flag = dvec[ix]->getSize() == pScanData->getSize() ? 0 : -1;//��ȷ���0.���ȷ���-1
			//CHECK_BOOL(size_flag == 0);
			if(size_flag != 0)
			{
				cout << "�����������ѯ���ݳ��Ȳ�һ��!" << endl;
				cout << "�������ݳ���Ϊ" << dvec[ix]->getSize() << endl;
				cout << "��ѯ���ݳ���Ϊ" << pScanData->getSize() << endl;
				HEAP_RETURN_FALSE
			}					
			++counter;	
			++ix;	
		}
		CHECK_BOOL(counter == TRANSACTION_ROWS );
		if(counter != TRANSACTION_ROWS)
		{
			cout << "�����������ѯ����������һ��!" << endl;
			cout << "��ѯ��������counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//�ύ����
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_insert_dll_002_step1()
{
	INTENT("��������ԭ���ԣ����벿������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//�����DATAROWS������λ����Ϊ�˷�����ʵ��ڴ��С
		for(int i = TRANSACTION_ROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//�ó����Ѱ���'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			//���벿�����ݺ���ֹ��ǰ����
			if(ix == TRANSACTION_ROWS/2)
			{
				free(insert_str); 
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				exit(1);
			}
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
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_insert_dll_002_step2()
{
	INTENT("��������ԭ���ԣ����벿������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//���������ȷ�ع���Ӧ�ò�ѯ��������
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
	
		delete pScanData; 
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//�ύ����
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_insert_dll_003()
{
	INTENT("��������ԭ���ԣ��Ȳ���һЩ���ݲ��ύ���ٲ��벿������ʱȡ����������һ���������Ƿ���ȷ�ع�");
	try 
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();

		int num_counter = 0;
		//�����DATAROWS������λ����Ϊ�˷�����ʵ��ڴ��С
		for(int i = TRANSACTION_ROWS; i!=0; i/=10)
		{
			++num_counter;
		}

		int len = sizeof("testdata_");//�ó����Ѱ���'\0'
		char *insert_str = (char*)malloc(len+num_counter);

		EntryID insert_tid;
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char string[20];
		char copy_insert_data[TRANSACTION_ROWS][20];
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
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
		commit_transaction();//�ύ����

		get_new_transaction();
		pEntrySet = open_entry_set();
		for( ix = 0; ix != TRANSACTION_ROWS; ++ix)
		{
			//���벿�����ݺ���ֹ��ǰ����
			if(ix == TRANSACTION_ROWS/2)
			{
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				user_abort_transaction();
				break;
			}
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

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
//		pEntrySet = open_entry_set();
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		//���������ȷ�ع���Ӧ�ò�ѯ������2�β��������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
		{
			++counter;
		}
		CHECK_BOOL(counter == TRANSACTION_ROWS );
		if(counter != TRANSACTION_ROWS)
		{
			cout << "�����������ѯ����������һ��!" << endl;
			cout << "��ѯ��������counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str);
		for(ix = 0; ix != TRANSACTION_ROWS+TRANSACTION_ROWS/2; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
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

int test_trans_persistence_insert()
{
	INTENT("�����������������ݡ��ύ���񲢽������̡�"
		"�������̣��򿪱��ѯ����������Բ��������"
		"�־��ԡ�");

	extern EntrySetID EID;

	DLL_PREPARE_TEST();

	SpliterGenerater sg;
	ColumnInfo *colinfo = sg.buildHeapColInfo(1, 3);
	int col_id = get_col_id();
	setColumnInfo(col_id, colinfo);
	char data[][DATA_LEN] = 
	{
		"data1",
		"data2",
		"data3",
		"aaaaaaatest"
	};

	/*
	* ׼������
	*/
	SHUTDOWN_TEST_STEP_1(TransPersistence)
	{
		EntrySetID ei;
		DLL_SIMPLE_CREATE_HEAP(ei, col_id);
		SAVE_HEAP_ID_FOR_NEXT(ei);
		int insert_data = 0;
		/* �����������ݣ��ȴ��´�������������� */
		DLL_SIMPLE_INSERT_DATA(ei, data, ARRAY_LEN_CALC(data), insert_data);
		return true;
	}
	/*
	* �������
	*/
	SHUTDOWN_TEST_STEP_2()
		int test_success = true;
		vector<string> v_src, v_det;
		for(int i = 0; i < ARRAY_LEN_CALC(data); ++i)
		{
			v_src.push_back(data[i]);
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
