#include "utils/utils_dll.h"
#include "index/test_index_cmp_dll.h"
#include "PGSETypes.h"
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "test_fram.h"
#include "vector"
#include <string>
#include <iostream>
#include <boost/assign.hpp>

using namespace FounderXDB::StorageEngineNS;

extern StorageEngine *pStorageEngine;
extern Transaction *pTransaction;
extern EntrySet* open_entry_set();

ColumnInfo *colinfo;
extern EntrySetID EID;

extern void my_spilt(RangeDatai& rangeData, const char* str, int col, size_t len = 0);

typedef int (*PToCompareFunc)(const char*, size_t, const char*, size_t);

#define startnum 120000
#define endnum 130000

#define predictdata "012562"
#define predictdata1 "012345"

char storeData[20000][20]={"\0"};

void free_col()
{
    delete (colinfo->rd_comfunction);
    delete (colinfo->col_number);
    delete colinfo;
    colinfo = NULL;
}

//init scan condition
void my_scankey(const char datum_data[][20], 
        const ScanCondition::CompareOperation co[],
        const int fieldno[],
        PToCompareFunc compare_func[],
        const int col_count,
        std::vector<ScanCondition> &vc)
{
    ScanCondition sc;
    for(int i = 0; i < col_count; ++i)
    {
        sc.compare_func = compare_func[i];
        sc.fieldno = fieldno[i];
        sc.argument = (se_uint64)datum_data[i];
        sc.arg_length = strlen(datum_data[i]);
        sc.compop = co[i];
        vc.push_back
            (
             ScanCondition
             (
              fieldno[i], 
              co[i], 
              (se_uint64)datum_data[i], 
              strlen(datum_data[i]), 
              compare_func[i]
             )
            );
    }
}
int my_compare(const char *str1, size_t len1, const char *str2, size_t len2)
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
typedef EntrySetCollInfo<3,2> EntrySetT;

void predict_data_largeMount()
{
    int i;	
    int datacols=0;//Ԥ�����ݴ���к�
    for (i=startnum;i<endnum;i++)//�洢Ԥ������
    {	
        int flag;
        char baseData[20]={"\0"};
//(i,baseData,10);
        char baseData_3[3]={"\0"};
        char baseData_2[2]={"\0"};
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
}

bool test_indexscan_LargeMount_dll()
{
    using namespace std;
    IndexEntrySet *pIndexEntrySet = NULL;
    EntrySet *pEntrySet = NULL;
    INTENT("ɨ��������ݣ������ַ����Ƚϲ�ȷ��������ȷ"
            "�ȽϵĲ���Ϊǰ�����ַ���123С�������������ַ���45��"
            "���������Ϊ120000��130000��Ԥ�����120460��120999,121460��121999�Դ�����");
    try
    {	
        predict_data_largeMount();//����Ԥ������

        get_new_transaction();
        pEntrySet = open_entry_set();
        DataItem di;
        EntryID eid;
        char data[20]={"\0"};
        //insert data into pEntrySet
        for(int i = startnum; i < endnum; i++)
        {
//(i,data,10);
            di.setData(data);
            di.setSize(strlen(data)+1);
            pEntrySet->insertEntry(pTransaction, eid, di);
        }
        command_counter_increment();

        //create a index on testRelation
        EntrySetID idxIndex = pStorageEngine->createIndexEntrySet(pTransaction, pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetT::get());
        pIndexEntrySet = pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,idxIndex);
        command_counter_increment();

        //begin index scan
        char datum_data[20][20] = {"123", "45"};
        int fieldno[2] = {1, 2};
        ScanCondition::CompareOperation co[2] = 
        {
            ScanCondition::LessThan,
            ScanCondition::GreaterThan
        };
        PToCompareFunc compare_func[2] = {my_compare, my_compare};
        vector<ScanCondition> vc;
        my_scankey(datum_data,co,fieldno,compare_func,2,vc);
        EntrySetScan *iess = pIndexEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
        memset(&eid, 0, sizeof(eid));
        memset(&di, 0, sizeof(di));
        char *temp;
        int flag = 65536;
        int storeRow=0;
        while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
        {
            temp = (char*)di.getData();
            flag = memcmp(temp,storeData[storeRow],sizeof(temp));
            CHECK_BOOL(flag==0);
            if(flag != 0)
            {			
                cout<<"��ѯ��������Ԥ�����ݲ�һ�£�����!"<<endl;
                INDEX_RETURN_FALSE_HS;
            }
            storeRow++;
        }
        pIndexEntrySet->endEntrySetScan(iess);
        INDEX_COMMIT_HS;		
    }
    CATCHEXCEPTION;
    return true;
}

bool test_indexCreatefirst_thenInsert()
{
    using namespace std;
    IndexEntrySet *pIndexEntrySet = NULL;
    EntrySet *pEntrySet = NULL;
    INTENT("�����ߴ��������ٲ������ݣ����Գɹ����������ڲ��ߵĳ����֧�ǲ�һ�µ�");
    try
    {	

        get_new_transaction();
        pEntrySet = open_entry_set();
        EntrySetID index_id = pStorageEngine->createIndexEntrySet(pTransaction, pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,GetSingleColInfo());
        char data[20][20] = {"123456", "123789", "012562", "012120", "012456"};
        DataItem di;
        EntryID eid;
        //insert data into pEntrySet
        for(int i = 0; i < 5; i++)
        {
            di.setData(&data[i]);
            di.setSize(strlen(data[i])+1);
            pEntrySet->insertEntry(pTransaction, eid, di);
        }

        command_counter_increment();

        //create a index on testRelation

        pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,index_id);
        command_counter_increment();

        //begin index scan
        char datum_data[20][20] = {"123", "45"};
        int fieldno[2] = {1, 2};
        ScanCondition::CompareOperation co[2] = 
        {
            ScanCondition::LessThan,
            ScanCondition::GreaterThan
        };
        PToCompareFunc compare_func[2] = {my_compare, my_compare};
        vector<ScanCondition> vc;
        my_scankey(datum_data,co,fieldno,compare_func,2,vc);
        EntrySetScan *iess = pIndexEntrySet->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
        memset(&eid, 0, sizeof(eid));
        memset(&di, 0, sizeof(di));
        char *temp;
        int flag = 65536;
        while(iess->getNext(EntrySetScan::NEXT_FLAG, eid, di) == 0)
        {
            temp = (char*)di.getData();
            flag = memcmp(temp,predictdata,sizeof(temp));
        }
        pIndexEntrySet->endEntrySetScan(iess);
        CHECK_BOOL(flag == 0);
        if(flag != 0)
        {			
            cout<<"��ѯ��������Ԥ�����ݲ�һ�£�����!"<<endl;
            INDEX_RETURN_FALSE_HS;
        }
        INDEX_COMMIT_HS;		
    }
    CATCHEXCEPTION;
    return true;
}

bool test_indexMult_SameCol_dll()
{
    using namespace std;
    IndexEntrySet *pIndexEntrySet1 = NULL;
    IndexEntrySet *pIndexEntrySet2 = NULL;
    IndexEntrySet *pIndexEntrySet3 = NULL;
    EntrySet *pEntrySet = NULL;
    INTENT("�����������������ͬ������");
    try
    {	
        get_new_transaction();
        pEntrySet = open_entry_set();
        //insert data into pEntrySet
        char data[20][20] = {"123456", "123789", "012562", "012120", "012456"};
        DataItem di;
        EntryID eid;
        //insert data into pEntrySet
        for(int i = 0; i < 5; i++)
        {
            di.setData(&data[i]);
            di.setSize(strlen(data[i])+1);
            pEntrySet->insertEntry(pTransaction, eid, di);
        }

        command_counter_increment();

        //create a index on testRelation
        pStorageEngine->createIndexEntrySet(pTransaction, pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetT::get());
        pStorageEngine->createIndexEntrySet(pTransaction, pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetT::get());
        pStorageEngine->createIndexEntrySet(pTransaction, pEntrySet,BTREE_INDEX_ENTRY_SET_TYPE,EntrySetT::get());
		pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,pIndexEntrySet1->getId());
        pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,pIndexEntrySet2->getId());
        pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,pIndexEntrySet3->getId());
        command_counter_increment();

        //begin index scan
        char datum_data[20][20] = {"123", "45"};
        int fieldno[2] = {1, 2};
        ScanCondition::CompareOperation co[2] = 
        {
            ScanCondition::LessThan,
            ScanCondition::GreaterThan
        };
        PToCompareFunc compare_func[2] = {my_compare, my_compare};
        vector<ScanCondition> vc;
        my_scankey(datum_data,co,fieldno,compare_func,2,vc);
        EntrySetScan *iess1 = pIndexEntrySet1->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
        memset(&eid, 0, sizeof(eid));
        memset(&di, 0, sizeof(di));
        char *temp;
        int flag = 65536;
        while(iess1->getNext(EntrySetScan::PREV_FLAG, eid, di) == 0)
        {
            temp = (char*)di.getData();
            flag = memcmp(temp,predictdata,sizeof(temp));
        }
        pIndexEntrySet1->endEntrySetScan(iess1);
        CHECK_BOOL(flag == 0);
        if(flag != 0)
        {			
            cout<<"��ѯ��������Ԥ�����ݲ�һ�£�����!"<<endl;
            pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
            pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet1);
            pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet1->getId());
            user_abort_transaction();
            return false;
        }

        EntrySetScan *iess2 = pIndexEntrySet2->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
        memset(&eid, 0, sizeof(eid));
        memset(&di, 0, sizeof(di));
        while(iess2->getNext(EntrySetScan::PREV_FLAG, eid, di) == 0)
        {
            temp = (char*)di.getData();
            flag = memcmp(temp,predictdata,sizeof(temp));
        }
        pIndexEntrySet2->endEntrySetScan(iess2);
        CHECK_BOOL(flag == 0);
        if(flag != 0)
        {			
            cout<<"��ѯ��������Ԥ�����ݲ�һ�£�����!"<<endl;
            pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
            pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet2);
            pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet2->getId());
            user_abort_transaction();
            return false;
        }

        EntrySetScan *iess3 = pIndexEntrySet3->startEntrySetScan(pTransaction,BaseEntrySet::SnapshotMVCC,vc);
        memset(&eid, 0, sizeof(eid));
        memset(&di, 0, sizeof(di));
        while(iess3->getNext(EntrySetScan::PREV_FLAG, eid, di) == 0)
        {
            temp = (char*)di.getData();
            flag = memcmp(temp,predictdata,sizeof(temp));
        }
        pIndexEntrySet3->endEntrySetScan(iess3);
        CHECK_BOOL(flag == 0);
        if(flag != 0)
        {			
            cout<<"��ѯ��������Ԥ�����ݲ�һ�£�����!"<<endl;
            pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
            pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet3);
            pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet3->getId());
            user_abort_transaction();
            return false;
        }


        pStorageEngine->closeEntrySet(pTransaction, pEntrySet);
        pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet1);
        pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet2);
        pStorageEngine->closeEntrySet(pTransaction, pIndexEntrySet3);
        pStorageEngine->removeIndexEntrySet(pTransaction, pEntrySet->getId(), pIndexEntrySet1->getId());
        commit_transaction();		
    }
    CATCHEXCEPTION;
    return true;
}

void heap_split_uniqe_01_dll(RangeDatai& rang, const char*, int i, size_t len)
{
    rang.start = 0;
    rang.len = 2;
}

void index_split_uniqe_01_dll(RangeDatai& rang, const char*, int i , size_t len)
{
    rang.start = 0;
    rang.len = 2;
}

void form_heap_colinfo_uniqe_01_dll(ColumnInfo &colinfo)
{
    colinfo.keys = 1;
    colinfo.col_number = NULL;
    colinfo.split_function = heap_split_uniqe_01_dll;
    colinfo.rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki));
    colinfo.rd_comfunction[0] = my_compare;
}

void form_index_colinfo_uniqe_01_dll(ColumnInfo &colinfo)
{
    colinfo.keys = 1;
    size_t *p = (size_t*)malloc(sizeof(size_t));
    *p = 1;
    colinfo.col_number = p;
    colinfo.split_function = index_split_uniqe_01_dll;
    colinfo.rd_comfunction = (CompareCallbacki*)malloc(sizeof(CompareCallbacki));
    colinfo.rd_comfunction[0] = my_compare;
}

EntrySet *heap_entry = NULL;
IndexEntrySet *index_entry = NULL;
Transaction *transaction = NULL;
bool test_index_uniqe_01_dll()
{
    StorageEngine *storageEngine= StorageEngine::getStorageEngine();

        using namespace boost::assign;
        EntrySetID HEAP_ID; 
        EntrySetID INDEX_ID ;
        //���Դ���Ψһ�����������������Ȼ��ͨ��gettype���type
        form_heap_colinfo_uniqe_01_dll(heap_colinfo);
        form_index_colinfo_uniqe_01_dll(index_colinfo);

        setColumnInfo(1, &heap_colinfo);
        setColumnInfo(2, &index_colinfo);//strlen���������"/0"�ĳ���

        FXTransactionId tid = 0;
        
        transaction = storageEngine->getTransaction(tid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);



        HEAP_ID = storageEngine->createEntrySet(transaction,1);
        //index_entry = storageEngine->createIndexEntrySet(transaction, heap_entry, BTREE_INDEX_ENTRY_SET_TYPE,"index", 0, 2);//������
        storageEngine->endStatement();

        //HEAP_ID = heap_entry->getId();
        //INDEX_ID = index_entry->getId();

        heap_entry = (EntrySet*)storageEngine->openEntrySet(transaction,EntrySet::OPEN_EXCLUSIVE,HEAP_ID);
        //index_entry = static_cast<IndexEntrySet *>(storageEngine->openIndexEntrySet(transaction, heap_entry, 0, INDEX_ID, NULL, NULL));GetSingleColInfo()

        std::vector<std::string> vInsertData;
        vInsertData += "19","28";
        InsertData(transaction,heap_entry,vInsertData);

        storageEngine->endStatement();
        
        INDEX_ID = storageEngine->createIndexEntrySet(transaction, heap_entry,  UNIQUE_BTREE_INDEX_ENTRY_SET_TYPE,2);//������
        index_entry = static_cast<IndexEntrySet *>(storageEngine->openIndexEntrySet(transaction, heap_entry, EntrySet::OPEN_EXCLUSIVE, INDEX_ID,NULL));
        std::vector<std::string> vResult1;
        SearchCondition scanCondition1;
        GetIndexScanResults(vResult1,transaction,index_entry,scanCondition1.Keys());
        try
        {       
        vInsertData.push_back("19"); //������ͬ�����ݼ���Ψһ����
        InsertData(transaction,heap_entry,vInsertData);// catch the problem

        storageEngine->endStatement();

		index_entry = static_cast<IndexEntrySet *>(storageEngine->openIndexEntrySet(transaction, heap_entry, EntrySet::OPEN_EXCLUSIVE, INDEX_ID, NULL));

        index_entry->getType();

        //����һ����ѯ
        SearchCondition scanCondition;
        scanCondition.Add(1,LessEqual,"9",str_compare);

        std::vector<std::string> vResult;
        GetIndexScanResults(vResult,transaction,index_entry,scanCondition.Keys());

        //�����
        std::set<std::string> sResult(vResult.begin(),vResult.end());
        std::set<std::string> sDesired;
        sDesired += "19","28","19";

        assert(sResult == sDesired);

        storageEngine->closeEntrySet(transaction,heap_entry);
        storageEngine->closeEntrySet(transaction,index_entry);
        storageEngine->removeIndexEntrySet(transaction, HEAP_ID, INDEX_ID, NULL);
        storageEngine->removeEntrySet(transaction,HEAP_ID,NULL);
        transaction->commit();

    }
    catch (StorageEngineException &ex) 
    {
        std::cout << ex.getErrorNo() << std::endl;
        std::cout << ex.getErrorMsg() << std::endl;
        transaction->abort();
        return true;
    }
    return false;
}
