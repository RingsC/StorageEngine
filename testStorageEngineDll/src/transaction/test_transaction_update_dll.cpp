/************************************************************************


���Ե��б�
���		��д��      �޸�����      �޸�ʱ��		�޸�����		
000			����      ����		  2011.9.14		 ��������ԭ���ԣ�����ȫ��������ɺ󣬸���ȫ�����ݣ���һ�������ѯ�Ƿ���ȷ����
001			����      ����		  2011.9.14		 ��������ԭ���ԣ��Ȳ���һЩ���ݲ��ύ�����²�������ʱȡ����������һ���������Ƿ���ȷ�ع�
002			����      ����		  2011.9.14		 ��������ԭ���ԣ����벿������ʱǿ��ͣ����ǰ���̣������ʱδ�ύ������һ�����̼�������Ƿ���ȷ�ع�(δ���)

************************************************************************/

#include <iostream>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "utils/utils_dll.h"
#include "transaction/test_transaction_update_dll.h"

using namespace FounderXDB::StorageEngineNS;

#define TRANSACTION_ROWS_UPDATE 10
#define TRANSACTION_DATA_UPDATE "testdata_"


//����������λ����Ϊ�˷�����ʵ��ڴ��С
int numLenth(int num)
{
	int num_len = 0;
	for(int i = num ; i!=0 ; i/=10)
	{
		++num_len;
	}
	return num_len;
}

char *formData(int num,char *str)
{
	int num_len = numLenth(num);
	int len = strlen(str)+1;//�ó��Ȱ���'\0'
	char *insert_str = (char*)malloc(len+num_len);
	return insert_str;
}

extern char *my_itoa(int value, char *string, int radix);

void insertData(EntrySet *pEntrySet,char *insert_str,vector<DataItem*> &dvec,char copy_insert_data[][20],uint32 nData,char *str)
{
	EntryID insert_tid;
	char string[20];
	vector<DataItem*>::size_type ix;
	int len;
	for( ix = 0; ix != nData; ++ix)
	{
		memcpy(insert_str, str, strlen(str)+1);//ÿ�ν���ѭ��memcpyһ�Σ���֤ÿ�ζ��Ǵӳ�ʼ�ַ�����ʼ�����ַ�
		my_itoa(ix,string,10);
		strcat(insert_str,string);
		memcpy(copy_insert_data[ix],insert_str,strlen(insert_str)+1);
		DataItem *data = new DataItem;
		data->setData((void*)copy_insert_data[ix]);
		len = strlen((char*)insert_str);//���Ȳ�����'\0'
		data->setSize(len+1);//Ϊ'\0'�����һ���ֽ�
		dvec.push_back(data);
		pEntrySet->insertEntry(pTransaction, insert_tid, *dvec[ix]);
//		cout << (char*)dvec[ix]->getData() << "  size = " << dvec[ix]->getSize() << endl;
	}
}

bool test_transaction_update_dll_000()
{
	INTENT("��������ԭ���ԣ�����ȫ��������ɺ󣬸���ȫ�����ݣ���һ�������ѯ�Ƿ���ȷ����");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(TRANSACTION_ROWS_UPDATE,TRANSACTION_DATA_UPDATE);
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char copy_insert_data[TRANSACTION_ROWS_UPDATE][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,TRANSACTION_ROWS_UPDATE,TRANSACTION_DATA_UPDATE);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pUpdateData = new DataItem;
		EntryID update_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		//��������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,update_tid,*pUpdateData) == 0 )//*pΪdataitem���ͣ��������
		{
			pEntrySet->updateEntry(pTransaction,update_tid,*dvec[0]);
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
		int data_flag,size_flag;
		//�������������ȷִ�У�Ӧ�ò�ѯ�����º������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,scan_tid,*pScanData) == 0 )//*pΪdataitem���ͣ��������
		{
			data_flag = memcmp(dvec[0]->getData(),pScanData->getData(),pScanData->getSize());
			//CHECK_BOOL(data_flag == 0);
			if(data_flag != 0)
			{
				cout << "�����������ѯ�������ݲ�һ��!" << endl;
				cout << "��������Ϊ" << (char*)dvec[0]->getData() << endl;	
				cout << "��ѯ����Ϊ" << (char*)pScanData->getData() << endl;
				HEAP_RETURN_FALSE
			}

			size_flag = dvec[0]->getSize() == pScanData->getSize() ? 0 : -1;//��ȷ���0.���ȷ���-1
			//CHECK_BOOL(size_flag == 0);
			if(size_flag != 0)
			{
				cout << "�����������ѯ���ݳ��Ȳ�һ��!" <<  size_flag <<endl;
				cout << "�������ݳ���Ϊ" << dvec[0]->getSize() << endl;
				cout << "��ѯ���ݳ���Ϊ" << pScanData->getSize() << endl;
				HEAP_RETURN_FALSE
			}	
			++counter;	
		}
		CHECK_BOOL(counter == TRANSACTION_ROWS_UPDATE );
		if(counter != TRANSACTION_ROWS_UPDATE)
		{
			cout << "�����������ѯ����������һ��!" << endl;
			cout << "��ѯ��������counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS_UPDATE; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		delete pUpdateData;
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//�ύ����
	} 
	CATCHEXCEPTION
		return true;
}

bool test_transaction_update_dll_001()
{
	INTENT("��������ԭ���ԣ��Ȳ���һЩ���ݲ��ύ�����²�������ʱȡ����������һ���������Ƿ���ȷ�ع�");
	try
	{
		get_new_transaction();
		EntrySet *pEntrySet = open_entry_set();
		char *insert_str = formData(TRANSACTION_ROWS_UPDATE,TRANSACTION_DATA_UPDATE);
		vector<DataItem*> dvec;
		vector<DataItem*>::size_type ix;
		char copy_insert_data[TRANSACTION_ROWS_UPDATE][20];
		insertData(pEntrySet,insert_str,dvec,copy_insert_data,TRANSACTION_ROWS_UPDATE,TRANSACTION_DATA_UPDATE);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pUpdateData = new DataItem;
		EntryID update_tid;
		EntrySetScan *pHeapScan = heap_begin_scan(pEntrySet);
		ix = 0;
		//��������
		while ( pHeapScan->getNext(EntrySetScan::NEXT_FLAG,update_tid,*pUpdateData) == 0 )//*pΪdataitem���ͣ��������
		{
			//���²������ݺ���ֹ��ǰ����
			if(ix == TRANSACTION_ROWS_UPDATE/2)
			{
				pEntrySet->endEntrySetScan(pHeapScan);
				pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
				user_abort_transaction();
				break;
			}
			pEntrySet->updateEntry(pTransaction,update_tid,*dvec[0]);
			++ix;
		}

		get_new_transaction();
		pEntrySet = open_entry_set();
		DataItem *pScanData = new DataItem;
		EntryID scan_tid;
		pHeapScan = heap_begin_scan(pEntrySet);
		int counter = 0;
		int data_flag,size_flag;
		ix = 0;
		//�������������ȷ�ع���Ӧ�ò�ѯ�������º������
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
				cout << "�����������ѯ���ݳ��Ȳ�һ��!" <<  size_flag <<endl;
				cout << "�������ݳ���Ϊ" << dvec[ix]->getSize() << endl;
				cout << "��ѯ���ݳ���Ϊ" << pScanData->getSize() << endl;
				HEAP_RETURN_FALSE
			}	
			++counter;	
			++ix;
		}
		CHECK_BOOL(counter == TRANSACTION_ROWS_UPDATE );
		if(counter != TRANSACTION_ROWS_UPDATE)
		{
			cout << "�����������ѯ����������һ��!" << endl;
			cout << "��ѯ��������counter=" << counter << endl;
			HEAP_RETURN_FALSE
		}

		free(insert_str); 
		for(ix = 0; ix != TRANSACTION_ROWS_UPDATE; ++ix)
		{
			delete dvec[ix];
//			cout << "delete suc" << endl;
		}
		delete pScanData; 
		delete pUpdateData;
		pEntrySet->endEntrySetScan(pHeapScan);
		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
		commit_transaction();//�ύ����
	} 
	CATCHEXCEPTION
		return true;
}