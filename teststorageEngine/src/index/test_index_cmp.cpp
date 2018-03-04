#include "postgres.h"
#include "access/heapam.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "catalog/xdb_catalog.h"
#include "postmaster/xdb_main.h"
#include "stdlib.h"
#include "catalog/metaData.h"
#include "test_fram.h"
#include "utils/util.h"
#include "index/test_index_cmp.h"
#include "utils/fmgroids.h"
#include "interface/FDPGAdapter.h"
#include "iostream"
#include "interface/StorageEngineExceptionUniversal.h"
#define startnum 100000
#define endnum 150000
using namespace std;
using namespace FounderXDB::StorageEngineNS;
extern char *my_itoa(int value, char *string, int radix);
int my_compare_index(const char *str1, size_t len1, const char *str2, size_t len2)
{
	int i = 0;
	while(i < len1 && i < len2)
	{
		if(str1[i] < str2[i])
			return -1;
		else if(str1[i] > str2[i])
			return 1;
		else i++;

	}
	if(len1 == len2)
		return 0;
	if(len1 > len2)
		return 1;
	return -1;
}

void my_split_index_OneCol_MultMethod(RangeData& rangeData, const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 3;
	}
}

void my_split_index(RangeData& rangeData, const char *str, int col, size_t len=0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 3;
	}
	if (col == 2)
	{
		rangeData.start = 3;
		rangeData.len = 2;
	}
	if (col == 3)
	{
		rangeData.start = 5;
		rangeData.len = 1;
	}
}

void my_split_index_number(RangeData& rangeData, const char *str, int col, size_t len = 0)
{
	rangeData.start = 0;
	rangeData.len = 0;
	if(str == NULL)
	{
		return ;
	}
	if (col == 1)
	{
		rangeData.start = 0;
		rangeData.len = 2;
	}
	if (col == 2)
	{
		rangeData.start = 2;
		rangeData.len = 2;
	}
}
bool test_indexscan_NumberCmp()
{
	INTENT("������ֵ���ݣ�index�����Ƚ�,int�ڴ����е�����ֱ�ӿ���char�ڴ���,"
		"ǰ��2���ַ��ֱ���Ϊkey��ͨ������Ƿ���������������������ȵıȽϲ��ԱȽ�"
		"���������Ϊ100000��150000��Ԥ�ڵĲ�ѯ��������Ϊ123456��");

	try
	{	
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,2,2);//�������colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//����

		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;
		begin_transaction();
		Relation testRelation;
		HeapTuple tup;
		int flag;//�Ƚϱ�־
		
		int s =123456;//Ԥ������
		char basechar[50];
		char cpychar1[2];
		char cpychar2[2];
		memcpy(basechar,&s,sizeof(s));//int�ڴ����е�����ֱ�ӿ���char�ڴ�����int32λ��char8λ��ռ��4��char
		memcpy(cpychar1,&s,2);//ǰ�����ַ�����cpychar1
		memcpy(cpychar2,&basechar[2],2);//�������ַ�����cpychar2

		Datum * values = (Datum *) palloc(sizeof(Datum)*2);//�ü���datum�ͳ˼�
		values[0] = fdxdb_string_formdatum(cpychar1, 2);//��������keys
		values[1] = fdxdb_string_formdatum(cpychar2, 2);
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId); //��heap
			
		unsigned int i;
		for (i=startnum;i<endnum;i++)//���������tuple����100000��150000
		{
		char insertData[50];
		memcpy(insertData, &i, sizeof(i));//i����insertData��
		tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//����Ԫ��
		FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//����Ԫ��
		}
		CommandCounterIncrement();
 		Relation indexrelation;
 		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));
 		colinfo->keys = 2;
 		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
 		colinfo->col_number[0] = 1;
 		colinfo->col_number[1] = 2;
 		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
 		colinfo->rd_comfunction[0] = my_compare_index;
 		colinfo->rd_comfunction[1] = my_compare_index;//�ȽϺ���
 		colinfo->split_function =  my_split_index_number;//index��split
		setColInfo(indid,colinfo);;
 		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//��������
 
 		IndexScanDesc index_scan;
 		ScanKeyData key[2];
 		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTEqualStrategyNumber, my_compare_index,values[0]); //��ʼ��scankey
 		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],2,BTEqualStrategyNumber, my_compare_index,values[1]);

		indexrelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//������

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexrelation, SnapshotNow, 2, 0);//��ʼ����ɨ��
		FDPG_Index::fd_index_rescan(index_scan, key, 2, NULL, 0);//ɨ������

 		int ss;
		char *tt;
 		while((tup = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
 		{
 			tt=fxdb_tuple_to_chars(tup);
 			ss=*(int*)tt;//ת��������
			flag=(ss-s);//sΪԤ������
			if (flag==0)
			{
				printf("������ȷ,����Ϊ:%d\n",ss);
			}
			else
			{
				printf("��������!\n");
				printf("����������Ϊd%\n",ss);
				user_abort_transaction();
				remove_index(rid,INDEX_ID);//++index_id
				remove_heap(THREAD_RID);//ɾ��
				return false;
			}
 		}
		
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexrelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//ɾ��
		if (colinfo->col_number)
 			free(colinfo->col_number);
 		if (colinfo->rd_comfunction)
 			free(colinfo->rd_comfunction);
 		free(colinfo);
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	
	return true;
}

bool test_indexscan_LargeMount()
{
	INTENT("ɨ��������ݣ������ַ����Ƚϲ�ȷ��������ȷ"
			"�ȽϵĲ���Ϊǰ�����ַ���123С�������������ַ���45��"
			"���������Ϊ100000��150000��Ԥ�����100460��100999,101460��101999�Դ�����");
	try
	{	
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//����

		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;//������
		Relation testRelation;
		HeapTuple tuple;
		int flag = 0;

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*2); //�ü���datum�ͳ˼�
		values[0] = fdxdb_string_formdatum("123", 3);
		values[1] = fdxdb_string_formdatum("45", 2);
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));//��ʼ��colinfo�ռ�
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_index;//heap��split
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		int i;
		int datacols=0;//Ԥ�����ݴ���к�
		char insertData[20]={0};
		char baseData[20]={0};
		char (*storeData)[20] = new char[50000][20];
		char string[20] = {0};
		for (i=startnum;i<endnum;i++)//�洢Ԥ������
		{
			my_itoa(i,baseData,10);
			char baseData_3[3]={0};
			char baseData_2[2]={0};
			memcpy(baseData_3,baseData,sizeof(baseData_3));//ǰ�����ַ�����baseData_3��
			memcpy(baseData_2,&baseData[3],sizeof(baseData_2));//���������ַ�����baseData_2��
			char *cpychar1="123";
			flag=memcmp(baseData_3,cpychar1,sizeof(baseData_3));//�Ƚ�ǰ�����ַ�
			if (flag<0)
			{
				char *cpychar2="45";
				flag=memcmp(baseData_2,cpychar2,sizeof(baseData_2));//�ȽϺ������ַ�
				if (flag>0)
				{
					memcpy(storeData[datacols],baseData,sizeof(baseData));//Ԥ�����ݴ���storeData��
					datacols++;
				}
			}		
		}
					
		for (i=startnum;i<endnum;i++)//�����������ַ�Ԫ��
		{
			my_itoa(i,string,10);
			memcpy(insertData,string,sizeof(string));
			tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//����Ԫ��
			FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//����Ԫ��
		}
		CommandCounterIncrement();

		Relation indexrelation;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));//��ʼ��colinfo�ռ�
		colinfo->keys = 2;//�������Ͻ�������
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->col_number[1] = 2;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_index;
		colinfo->rd_comfunction[1] = my_compare_index;//�ȽϺ���
		colinfo->split_function =  my_split_index;//index��split
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//��������

		IndexScanDesc index_scan;
		ScanKeyData key[2];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTLessStrategyNumber, my_compare_index,values[0]);	 //��ʼ��scankey
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],2,BTGreaterStrategyNumber, my_compare_index,values[1]);
		indexrelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//������

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexrelation, SnapshotNow, 2, 0);//��ʼ����ɨ��
		FDPG_Index::fd_index_rescan(index_scan, key, 2, NULL, 0);//ɨ������
		char *tt;//�洢��ȡscan��char
		int storeRow=0;
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			int tup_len = 0;
			tt = fdxdb_tuple_to_chars_with_len(tuple,tup_len);
			flag=memcmp(storeData[storeRow],tt,tup_len);
			storeRow++;
			if (flag!=0)
			{
				printf("��������!\n");
				printf("����ĵط����ַ�Ϊ:%s\n",tt);
				FDPG_Index::fd_index_endscan(index_scan);
				FDPG_Index::fd_index_close(indexrelation,AccessShareLock);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				remove_index(rid,INDEX_ID);//++index_id
				remove_heap(THREAD_RID);//ɾ��
				user_abort_transaction();
				delete[] storeData;
				return false;
			}
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexrelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//ɾ��
    delete[] storeData;

		if (colinfo->col_number)
			free(colinfo->col_number); 	
		if (colinfo->rd_comfunction)
			free(colinfo->rd_comfunction);
 		free(colinfo);
		free(colinfo0);
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	

	return true;
}

bool test_indexscan_OneCol_MultMethod()
{
	INTENT("ɨ��������ݣ������ַ����Ƚϲ�ȷ��������ȷ"
			"��һ���Ͻ��������������,����Ϊǰ�����ַ�����123������С��134"
			"��������Ϊ100000��150000��Ԥ���������Ϊ124000��133999");
	try
	{	
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//����

		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;//������
		Relation testRelation;
		HeapTuple tuple;
		int flag = 0;

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*2); //�ü���datum�ͳ˼�
		values[0] = fdxdb_string_formdatum("123", 3);
		values[1] = fdxdb_string_formdatum("134", 3);
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));//��ʼ��colinfo�ռ�
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_index_OneCol_MultMethod;//heap��split
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		int i,storeRow=0;
		int begintry=124000;//Ԥ����ʼ����
		int endtry=134000;//Ԥ����������
		char insertData[10]={"\0"};
		char storeData[50000][10]={"\0"};
		char string[10] = {"\0"};
		for (i=begintry;i<endtry;i++) //����Ԥ������
		{
			my_itoa(i,string,10);
			memcpy(storeData[storeRow],string,sizeof(string));
			storeRow++;
		}

		for (i=startnum;i<endnum;i++)//�����������ַ�Ԫ��
		{
			my_itoa(i,string,10);
			memcpy(insertData,string,sizeof(string));
			tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//����Ԫ��
			FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//����Ԫ��
		}
		CommandCounterIncrement();

		Relation indexrelation;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));//��ʼ��colinfo�ռ�
		colinfo->keys = 2;//�������Ͻ�������
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->col_number[1] = 1;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_index;
		colinfo->rd_comfunction[1] = my_compare_index;//�ȽϺ���
		colinfo->split_function =  my_split_index_OneCol_MultMethod;//index��split
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//��������

		IndexScanDesc index_scan;
		ScanKeyData key[2];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTGreaterStrategyNumber, my_compare_index,values[0]);	 //��ʼ��scankey
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],1,BTLessStrategyNumber, my_compare_index,values[1]);
		indexrelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//������

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexrelation, SnapshotNow, 2, 0);//��ʼ����ɨ��
		FDPG_Index::fd_index_rescan(index_scan, key, 2, NULL, 0);//ɨ������
		char *tt;//�洢��ȡscan��char
		storeRow=0;
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			int tup_len = 0;
			tt = fdxdb_tuple_to_chars_with_len(tuple,tup_len);
			flag=memcmp(storeData[storeRow],tt,tup_len);
			storeRow++;
			if (flag!=0)
			{
				printf("��������!\n");
				printf("����ĵط����ַ�Ϊ:%s\n",tt);
				FDPG_Index::fd_index_endscan(index_scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				FDPG_Index::fd_index_close(indexrelation, AccessShareLock);
				remove_index(rid,INDEX_ID);//++index_id
				remove_heap(THREAD_RID);//ɾ��
				user_abort_transaction();
				return false;
			}
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexrelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//ɾ��
		
		if (colinfo->col_number)
			free(colinfo->col_number); 	
		if (colinfo->rd_comfunction)
			free(colinfo->rd_comfunction);
		free(colinfo);
		free(colinfo0);//�ͷ������colinfo0�ռ�
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	
	return true;
}

bool test_indexscan_MultCol_MultMethod()
{
	INTENT("ɨ��������ݣ������ַ����Ƚϲ�ȷ��������ȷ"
		"�������Ϸֱ���������������,����Ϊǰ�����ַ�����123��"
		"����С��134�����������ַ�����45��С��78"
		"��������Ϊ100000��150000��"
		"Ԥ���������Ϊ124460��124779,125469��125460�Դ�������133779");
	try
	{	
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//����

		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;//������
		Relation testRelation;
		HeapTuple tuple;
		int flag = 0;

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*4); //�ü���datum�ͳ˼�
		values[0] = fdxdb_string_formdatum("123", 3);
		values[1] = fdxdb_string_formdatum("134", 3);
		values[2] = fdxdb_string_formdatum("45", 2);
		values[3] = fdxdb_string_formdatum("78", 2);
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));//��ʼ��colinfo�ռ�
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_index;//heap��split
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		int i;
		int k=0;
		int datacols=0;
		char insertData[20]={"\0"};
		char baseData[20]={"\0"};
		char storeData[20000][20]={"\0"};
		char string[20] = {"\0"};
		for (i=startnum;i<endnum;i++)//�洢Ԥ������
		{
			my_itoa(i,baseData,10);
			char baseData_3[3]={"\0"};
			char baseData_2[2]={"\0"};
			memcpy(baseData_3,baseData,sizeof(baseData_3));//ǰ�����ַ�����baseData_3��
			memcpy(baseData_2,&baseData[3],sizeof(baseData_2));//���������ַ�����baseData_2��
			char *cpychar1_1="123";
			flag=memcmp(baseData_3,cpychar1_1,sizeof(baseData_3));//�Ƚ�ǰ�����ַ�
			if (flag>0)
			{
				char *cpychar1_2="134";
				flag=memcmp(baseData_3,cpychar1_2,sizeof(baseData_3));
				if (flag<0)
				{	
					char *cpychar2_1="45";
					flag=memcmp(baseData_2,cpychar2_1,sizeof(baseData_2));//�ȽϺ������ַ�
					if (flag>0)
					{
						char *cpychar2_2="78";
						flag=memcmp(baseData_2,cpychar2_2,sizeof(baseData_2));
						if (flag<0)
						{
							memcpy(storeData[datacols],baseData,sizeof(baseData));//Ԥ�����ݴ���storeData��
							datacols++;
						}
					}
				}

			}		
			k++;
		}

		for (i=startnum;i<endnum;i++)//�����������ַ�Ԫ��
		{
			my_itoa(i,string,10);
			memcpy(insertData,string,sizeof(string));
			tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//����Ԫ��
			FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//����Ԫ��
		}
		CommandCounterIncrement();

		Relation indexRelation;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));//��ʼ��colinfo�ռ�
		colinfo->keys = 2;//�����Ͻ�������
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->col_number[1] = 2;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_index;
		colinfo->rd_comfunction[1] = my_compare_index;
		colinfo->split_function =  my_split_index;//index��split
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//��������
		CommandCounterIncrement();

		IndexScanDesc index_scan;
		ScanKeyData key[4];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTGreaterStrategyNumber, my_compare_index,values[0]);//��ʼ��scankey
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],1,BTLessStrategyNumber, my_compare_index,values[1]);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[2],2,BTGreaterStrategyNumber, my_compare_index,values[2]);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[3],2,BTLessStrategyNumber, my_compare_index,values[3]);
		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//������

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, 4, 0);//��ʼ����ɨ��
		FDPG_Index::fd_index_rescan(index_scan, key, 4, NULL, 0);//ɨ������
		char *tt;//�洢��ȡscan��char
		int storeRow=0;
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			int tup_len = 0;
			tt = fdxdb_tuple_to_chars_with_len(tuple,tup_len);
			flag=memcmp(storeData[storeRow],tt,tup_len);
			storeRow++;
 			if (flag!=0)
 			{
 				printf("��������!\n");
 				printf("����ĵط����ַ�Ϊ:%s\n",tt);
 				FDPG_Index::fd_index_endscan(index_scan);
 				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
 				FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
				remove_index(rid,INDEX_ID);//++index_id
				remove_heap(THREAD_RID);//ɾ��
 				user_abort_transaction();
 				return false;
 			}
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//ɾ��

		if (colinfo->col_number)
			free(colinfo->col_number); 	
		if (colinfo->rd_comfunction)
			free(colinfo->rd_comfunction);
		free(colinfo);
		free(colinfo0);//�ͷ������colinfo0�ռ�
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	
	return true;
}

bool test_indexscan_MultCol_Col1OneMethod_Col2MultMethod()
														  //����������һ���е�������Ч�ʸߣ����������������Ż������ܸ���
{
	INTENT("ɨ��������ݣ������ַ����Ƚϲ�ȷ��������ȷ"
		"�������Ͻ�����������,����Ϊǰ�����ַ�����123"
		",���ŵ������ַ�����23��С��78"
		"��������Ϊ100000��150000"
		"Ԥ���������Ϊ123240��123779");
	try
	{	
		using namespace FounderXDB::StorageEngineNS;
		using namespace boost;

		SpliterGenerater sg;
		const int COL_NUM = 2;

		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
		++THREAD_RID;
		int rid=THREAD_RID;
		setColInfo(rid, heap_info);
		create_heap(rid,heap_info);//����

		begin_transaction();
		Oid reltablespace = DEFAULTTABLESPACE_OID;
		++INDEX_ID;
		Oid indid = INDEX_ID;//������
		Relation testRelation;
		HeapTuple tuple;
		int flag = 0;
		int beginnum=123240;
		int finishnum=123780;

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*3); //�ü���datum�ͳ˼�
		values[0] = fdxdb_string_formdatum("123", 3);
		values[1] = fdxdb_string_formdatum("23", 2);
		values[2] = fdxdb_string_formdatum("78", 2);
		Colinfo colinfo0 = (Colinfo)malloc(sizeof(ColinfoData));//��ʼ��colinfo�ռ�
		colinfo0->col_number = NULL;
		colinfo0->rd_comfunction = NULL;
		colinfo0->split_function =  my_split_index;//heap��split
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		int i;
		int storeRow=0;
		char insertData[20]={"\0"};
		char storeData[1000][20]={"\0"};
		char string[10] = {"\0"};
		for (i=beginnum;i<finishnum;i++)//�洢Ԥ������
		{
			my_itoa(i,string,10);
			memcpy(storeData[storeRow],string,sizeof(string));//Ԥ���������Ϊ123240��123779"
			storeRow++;
		}

		for (i=startnum;i<endnum;i++)//�����������ַ�Ԫ��
		{
			my_itoa(i,string,10);
			memcpy(insertData,string,sizeof(string));
			tup = FDPG_Heap::fd_heap_form_tuple(insertData, sizeof(insertData));//����Ԫ��
			FDPG_Heap::fd_simple_heap_insert(testRelation,tup);//����Ԫ��

		}

		Relation indexRelation;
		Colinfo colinfo = (Colinfo)malloc(sizeof(ColinfoData));//��ʼ��colinfo�ռ�
		colinfo->keys = 2;//�����Ͻ�������
		colinfo->col_number = (size_t*)malloc(colinfo->keys * sizeof(size_t));
		colinfo->col_number[0] = 1;
		colinfo->col_number[1] = 2;
		colinfo->rd_comfunction = (CompareCallback*)malloc(colinfo->keys * sizeof(CompareCallback));
		colinfo->rd_comfunction[0] = my_compare_index;
		colinfo->rd_comfunction[1] = my_compare_index;
		colinfo->split_function =  my_split_index;//index��split
		setColInfo(indid,colinfo);
		FDPG_Index::fd_index_create(testRelation,BTREE_TYPE,indid,indid);//��������
		CommandCounterIncrement();

		IndexScanDesc index_scan;
		ScanKeyData key[3];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTEqualStrategyNumber, my_compare_index,values[0]);//��ʼ��scankey
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[1],2,BTGreaterStrategyNumber, my_compare_index,values[1]);
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[2],2,BTLessStrategyNumber, my_compare_index,values[2]);

		indexRelation = FDPG_Index::fd_index_open(indid,AccessShareLock, MyDatabaseId);//������

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, 3, 0);//��ʼ����ɨ��
		FDPG_Index::fd_index_rescan(index_scan, key, 3, NULL, 0);//ɨ������
		char *tt;//�洢��ȡscan��char
		storeRow=0;//��ʼ���洢��������
		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			int tup_len = 0;
			tt = fdxdb_tuple_to_chars_with_len(tuple, tup_len);
			flag=memcmp(storeData[storeRow],tt,tup_len);//indexɨ�������������Ԥ�����ݱȽ�
			storeRow++;
			if (flag!=0)
			{
			 	printf("��������!\n");
			 	printf("����ĵط����ַ�Ϊ:%s\n",tt);
				FDPG_Index::fd_index_endscan(index_scan);
				FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
				FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
				FDPG_Index::fd_index_drop(rid, reltablespace, indid, MyDatabaseId);
				user_abort_transaction();
				return false;
			}
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//ɾ��

		if (colinfo->col_number)
			free(colinfo->col_number); 	
		if (colinfo->rd_comfunction)
			free(colinfo->rd_comfunction);
		free(colinfo);
		free(colinfo0);//�ͷ������colinfo0�ռ�
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	
	return true;
}

bool test_index_uniqe_01()
{
	INTENT("����Ψһ����������в��������ͬ�����ݣ��������Ĺ��̴���" 
		"UNIQUE_CHECK_YES���б�Ψһ��⣬�����ᱻcatch��.");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;

	SpliterGenerater sg;
	const int COL_NUM = 3;

	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
	THREAD_RID += 10;
	int rid=THREAD_RID;
	INDEX_ID += 10;
	int index_id=INDEX_ID;
	setColInfo(rid, heap_info);
	create_heap(rid,heap_info);//����

	const unsigned int data_row = 10;
	const unsigned int data_len = 20;
	DataGenerater dg(data_row, data_len);
	dg.dataGenerate();

	char data[][DATA_LEN] = {"aqwevadr", "aqwevadr","aqwevadr"};//��ʼ������������
	insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN,rid);//����в�������

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//����index��colinfo
	try
	{
		create_index(rid, heap_info,index_info,index_id, UNIQUE_CHECK_YES);//������
	}
	catch(...)
	{
		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//ɾ��
		return TRUE;
	}
	return false;
}

bool test_index_uniqe_02()
{
	INTENT("����Ψһ����������в��������ͬ�����ݲ����������ڸ��������Ĺ����У�"
		"���õ�index_insert����UNIQUE_CHECK_YES �����ᱻcatchס��");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	char *update_src_1 =  "aqwevadr"; 
	char *update_det_1 =   "aqwevadr"; 
	SpliterGenerater sg;
	const int COL_NUM = 3;

	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
	++THREAD_RID;
	int rid=THREAD_RID;
	++INDEX_ID;
	int index_id=INDEX_ID;
	setColInfo(rid, heap_info);
	create_heap(rid,heap_info);//����

	const unsigned int data_row = 10;
	const unsigned int data_len = 20;
	DataGenerater dg(data_row, data_len);
	dg.dataGenerate();

	char data[][DATA_LEN] = {"aqwevadr", "aqwevadr","aqwevadr"};//��ʼ������������

	insert_data(data, ARRAY_LEN_CALC(data), DATA_LEN,rid);//����в�������

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//����index��colinfo
	create_index(rid, heap_info,index_info,index_id);//������

	Relation rel = NULL;
	begin_transaction();
	int count = 0;
	int found[DATA_LEN];
	Datum		values[1];
	bool		isnull[1];

	Relation indrel1 = NULL;
	rel = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);
	ItemPointerData ipd1 = findTuple(update_src_1, rel, found[1], count);//�ҵ�src
	HeapTuple tuple1 = fdxdb_heap_formtuple(update_det_1, sizeof("aqwevadr"));

	FDPG_Heap::fd_simple_heap_update(rel, &ipd1, tuple1);//���±�
	indrel1 = FDPG_Index::fd_index_open(index_id,AccessShareLock, MyDatabaseId);//��������
	values[0] = fdxdb_form_index_datum(rel, indrel1, tuple1);
	isnull[0] = false;
	try
	{
		FDPG_Index::fd_index_insert(indrel1, values, isnull, &(tuple1->t_self), rel, UNIQUE_CHECK_YES);//��������
	}
	catch(...)
	{
		FDPG_Index::fd_index_close(indrel1,AccessShareLock);//�ر�����
		FDPG_Heap::fd_heap_close(rel, RowExclusiveLock);
		CommandCounterIncrement();
		commit_transaction();
		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//ɾ��
		return TRUE;
	}
	commit_transaction();
	remove_index(rid,INDEX_ID);//++index_id
	remove_heap(THREAD_RID);//ɾ��
	return TRUE;
}
#define COUNT 100
#define ARRAY_IN "aqwev"
bool test_index_uniqe_03()
{
	INTENT("��һ�����н��ж�β���ͬһ�����ݣ�ͬʱ����������"
		"�����ܷ���ȷִ��,����������в����uniqe�����index_scanɨ��������");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	SpliterGenerater sg;
	int i=0;
	const int COL_NUM = 2;
	int data_len=sizeof(ARRAY_IN);
	char predict_data[COUNT][DATA_LEN]={"\0"};

	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
	++THREAD_RID;
	int rid=THREAD_RID;
	++INDEX_ID;
	int index_id=INDEX_ID;
	setColInfo(rid, heap_info);
	create_heap(rid,heap_info);//����

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//����index��colinfo
	create_index(rid, heap_info,index_info,index_id);//������

	HeapIndexRelation hir;
	hir.index_array_len = 1;
	hir.heap_id = rid;
	hir.heap_info = heap_info;
	hir.index_id = (Oid*)malloc(hir.index_array_len * sizeof(Oid));
	hir.index_id[0] = index_id;
	hir.index_info = (Colinfo*)malloc(hir.index_array_len * sizeof(Colinfo));
	hir.index_info[0] = index_info;

	for (i=0;i<COUNT;i++)
	{
		memcpy(predict_data[i],ARRAY_IN,data_len);//Ԥ������
	}

	char data[][DATA_LEN] = {ARRAY_IN};//��ʼ������������
	for (i=0;i<COUNT;i++)
	{
		insert_with_index(data,data_len,1,&hir,UNIQUE_CHECK_NO);//�������ݲ���������
	}

	IndexScanInfo isi;
	alloc_scan_space(INDEX_COL_NUM, isi);
	isi.cmp_values[0] = fdxdb_string_formdatum("aqw", strlen("aqw"));//��ʼ��scankey
	isi.cmp_values[1] = fdxdb_string_formdatum("ev", strlen("ev"));
	isi.cmp_func[0] = my_compare_str;
	isi.cmp_func[1] = my_compare_str;
	isi.cmp_strategy[0] = BTGreaterEqualStrategyNumber;
	isi.cmp_strategy[1] = BTGreaterEqualStrategyNumber;
	isi.col_array[0] = 1;
	isi.col_array[1] = 2;
	init_scan_key(isi);

	unsigned int array_len=0;
	char result_data[COUNT][DATA_LEN];
	DataGenerater found_dg(100, DATA_LEN);
	index_scan_hs(index_id, index_info, rid, heap_info,&isi, &found_dg,&array_len);
	//���ɶ�ά�ַ�������
	found_dg.dataToDataArray2D(array_len, result_data);
	bool sta = check_array_equal(predict_data, result_data, data_len, ARRAY_LEN_CALC(data));
	assert(sta == TRUE);

	remove_index(rid,INDEX_ID);//++index_id
	remove_heap(THREAD_RID);//ɾ��

	return TRUE;
}

bool test_index_update_multi()
{
	INTENT("��һ�����н��ж�β���һ��������ݣ�ͬʱ���������������ܷ���ȷִ�С�");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	SpliterGenerater sg;
	int i=0;
	int count=100;
	const int COL_NUM = 3;

	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
	++THREAD_RID;
	int rid=THREAD_RID;
	++INDEX_ID;
	int index_id=INDEX_ID;
	setColInfo(rid, heap_info);
	create_heap(rid,heap_info);//����

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//����index��colinfo
	create_index(rid, heap_info,index_info,index_id);//������

	for (i=0;i<count;i++)
	{
		const unsigned int data_row = 1;
		const unsigned int data_len = 6;
		DataGenerater dg(data_row, data_len);
		dg.dataGenerate();
		char data[data_row][DATA_LEN];
		dg.dataToDataArray2D(data);//��ʼ������������

		insert_data(data, 1, DATA_LEN,rid);//����в�������
		form_index(data, 1, data_len, rid, index_id, heap_info, index_info, UNIQUE_CHECK_NO);//��������
	}
	remove_index(rid,INDEX_ID);//++index_id
	remove_heap(THREAD_RID);//ɾ��
	
	return TRUE;
}

bool test_debug_001()
{
	try
	{

	INTENT("debug 001");

	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	SpliterGenerater sg;
	int i=0;
	const int COL_NUM = 3;
	HeapTuple tuple;
	++THREAD_RID;
	++INDEX_ID;
	int rid=THREAD_RID;
	int index_id=INDEX_ID;

	begin_transaction();

	Oid relspace = MyDatabaseTableSpace;
	SAVE_INFO_FOR_DEBUG();
	Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
	setColInfo(rid, heap_info);
	rid = FDPG_Heap::fd_heap_create(relspace, 0,0,rid);

	Relation heap_relation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock, MyDatabaseId);

	const int INDEX_COL_NUM = 2;
	int col_number[INDEX_COL_NUM] = {1,2};
	CompareCallback cmp_func[INDEX_COL_NUM] = 
	{
		my_compare_str,
		my_compare_str
	};
	Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//����index��colinfo
	setColInfo(index_id , index_info);

	FDPG_Index::fd_index_create(heap_relation, BTREE_UNIQUE_TYPE,index_id,index_id);

	Relation indexRelation = FDPG_Index::fd_index_open(index_id,AccessShareLock, MyDatabaseId);//������

	//CommandCounterIncrement();
	const unsigned int data_row = 10;
	const unsigned int data_len = 20;
	DataGenerater dg(data_row, data_len);
	dg.dataGenerate();

	char data[][DATA_LEN] = {"abcdefse","e3erefas","fasdferera","fefsdfe4543"
							 "aqwevadr","rer6u7y6","afhth656wgf","r3rasfe4wea"
							 "aqwdeeadr","sdfet45fsd","dfr3fsdaaf","ef3radsfee"};//��ʼ������������

	for(int i = 0; i <  ARRAY_LEN_CALC(data); ++i)
		{
			SAVE_INFO_FOR_DEBUG();
			tuple = fdxdb_heap_formtuple(data[i], data_len);
			FDPG_Heap::fd_simple_heap_insert(heap_relation, tuple);
		}
		CommandCounterIncrement();
	//commit_transaction();

	StorageEngineFixture::bEnded = true;
	on_exit_reset();
	exit(0);
	}
	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	
	return true;

}

bool test_debug_002()
{
	using namespace FounderXDB::StorageEngineNS;
	using namespace boost;
	SpliterGenerater sg;
	try
	{
		INTENT("debug 002");
		begin_transaction();
		HeapTuple tuple;
		int rid=513;
		++INDEX_ID;
		int index_id=INDEX_ID;
		Relation indexRelation;
		Relation testRelation;

		const int COL_NUM = 3;
		Colinfo heap_info = sg.buildHeapColInfo(COL_NUM,3,2);//�������colinfo
		setColInfo(rid, heap_info);

		const int INDEX_COL_NUM = 2;
		int col_number[INDEX_COL_NUM] = {1,2};
		CompareCallback cmp_func[INDEX_COL_NUM] = 
		{
			my_compare_str,
			my_compare_str
		};
		Colinfo index_info = sg.buildIndexColInfo(2, col_number, cmp_func, SpliterGenerater::index_split_to_any<1,2>);//����index��colinfo

		setColInfo(index_id , index_info);
		testRelation = FDPG_Heap::fd_heap_open(rid,RowExclusiveLock,MyDatabaseId);

		indexRelation = FDPG_Index::fd_index_open(index_id,AccessShareLock, MyDatabaseId);//������

		HeapTuple tup;
		Datum * values = (Datum *) palloc(sizeof(Datum)*1); //�ü���datum�ͳ˼�
		values[0] = fdxdb_string_formdatum("a", 1);

		IndexScanDesc index_scan;
		ScanKeyData key[1];
		Fdxdb_ScanKeyInitWithCallbackInfo(&key[0],1,BTGreaterEqualStrategyNumber, my_compare_index,values[0]);//��ʼ��scankey

		index_scan = FDPG_Index::fd_index_beginscan(testRelation, indexRelation, SnapshotNow, 1, 0);//��ʼ����ɨ��
		FDPG_Index::fd_index_rescan(index_scan, key, 1, NULL, 0);//ɨ������
		char *tt;//�洢��ȡscan��char

		while((tuple = FDPG_Index::fd_index_getnext(index_scan, ForwardScanDirection)) != NULL)
		{
			tt=fxdb_tuple_to_chars(tuple);
		}
		FDPG_Index::fd_index_endscan(index_scan);
		FDPG_Heap::fd_heap_close(testRelation, RowExclusiveLock);
		FDPG_Index::fd_index_close(indexRelation, AccessShareLock);
		commit_transaction();

		remove_index(rid,INDEX_ID);//++index_id
		remove_heap(THREAD_RID);//ɾ�� ++heap_id
	}

	catch (StorageEngineExceptionUniversal &ex) 
	{
		user_abort_transaction();
		cout << ex.getErrorNo() << endl;
		cout << ex.getErrorMsg() << endl;
		return false;
	}	

	return true;
}