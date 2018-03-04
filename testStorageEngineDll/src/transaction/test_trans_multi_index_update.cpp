/**
* @file test_trans_multi_index_update.cpp
* @brief 
* @author ������
* @date 2011-9-27 15:50:42
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <boost/assign.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include "test_fram.h"
#include "StorageEngine.h"
#include "utils/utils_dll.h"
#include "transaction/test_trans_multi_index_update.h"
using namespace std;
using namespace boost::assign;
using namespace FounderXDB::StorageEngineNS;



/************************************************************************** 
* @brief CheckIndexScan 
* �����һ��ֻ�й̶��ֶεı��Ͻ���updateʱ�ع��Ƿ����������ԭ����
* Detailed description.
* @param[in] pTrans ��ǰ����
* @param[in] pIndex1 �ڵ�һ�к͵������Ͻ������������
* @param[in] pIndex2 �ں����к͵������Ͻ������������
* @param[in] pIndex3 �ڵ������Ͻ���������
**************************************************************************/
static void CheckIndexScan(Transaction *pTrans
						   ,IndexEntrySet* pIndex1
						   ,IndexEntrySet* pIndex2
						   ,IndexEntrySet *pIndex3)
{
	//�ڵ�һ���ϲ�ѯ�������(��ʹ��Index1: "qe9" <= ��һ�� < "qp1")
	SearchCondition search1;
	search1.Add(1,LessThan,"qp1",str_compare);
	search1.Add(1,GreaterEqual,"qe9",str_compare);

	set<string> sExpectResult1;
	sExpectResult1 += "qeliowrt2923484jwrt7qt440t0qte","qe9iowrt2823484jwrt1qt44050qte","qeliowrw2923484ewrt7qt4n0t0qte";
	vector<string> vResultUsingIndex1_1;
	GetIndexScanResults(vResultUsingIndex1_1,pTrans,pIndex1,search1.Keys());

	set<string> sResultUsingIndex1_1(vResultUsingIndex1_1.begin(),vResultUsingIndex1_1.end());
	CHECK_BOOL(sResultUsingIndex1_1 == sExpectResult1);

	//�ڵ������ϲ�ѯ�������(����ʹ��Index1��Index2: "t2" <= ������)
	SearchCondition search2_1,search2_2;
	search2_1.Add(2,GreaterEqual,"t2",str_compare); //for index1 ��3����index1���ǵ�2��
	search2_2.Add(1,GreaterEqual,"t2",str_compare); //for index2 ��3����index3���ǵ�1��
	set<string> sExpectResult2;
	sExpectResult2 += "qeliowrt2923484jwrt7qt440t0qte"
		,"qe3iowrt2923484jw2t7qt740t0qte"
		,"qe9iowrt2823484jwrt1qt44050qte"
		,"qe8iowrt5923484owht0qt420t0qte"
		,"qeliowrw2923484ewrt7qt4n0t0qte";

	vector<string> vResultUsingIndex1_2,vResultUsingIndex2_1;
	GetIndexScanResults(vResultUsingIndex1_2,pTrans,pIndex1,search2_1.Keys());
	GetIndexScanResults(vResultUsingIndex2_1,pTrans,pIndex2,search2_2.Keys());

	set<string> sResultUsingIndex1_2(vResultUsingIndex1_2.begin(),vResultUsingIndex1_2.end());
	set<string> sResultUsingIndex2_1(vResultUsingIndex2_1.begin(),vResultUsingIndex2_1.end());

	CHECK_BOOL(sExpectResult2 == sResultUsingIndex1_2);
	CHECK_BOOL(sExpectResult2 == sResultUsingIndex2_1);

	//�ڵ�5���ϲ�ѯ(��ʹ�õ���Index2��Index3:)
	SearchCondition search3_1,search3_2;
	search3_1.Add(2,GreaterEqual,"rt0qg4",str_compare);   //for index2 ��5����index2���ǵ�2��
	search3_1.Add(2,LessThan,"rt1qt4",str_compare);       //for index2

	search3_2.Add(1,GreaterEqual,"rt0qg4",str_compare);   //for index3  ��5����index3���ǵ�1��
	search3_2.Add(1,LessThan,"rt1qt4",str_compare);       //for index3

	set<string> sExpectResult3;
	sExpectResult3 += "qe3iowrt0923484owrt0qt4q0t0qte",
		"qpliowr40923484o2rt0qt4q0t0qte",
		"qe7io7rt0922484owrt0qg4q0t0qte";

	vector<string> vResultUsingIndex2_2,vResultUsingIndex3_1;
	GetIndexScanResults(vResultUsingIndex2_2,pTrans,pIndex2,search3_1.Keys());
	GetIndexScanResults(vResultUsingIndex3_1,pTrans,pIndex3,search3_2.Keys());

	set<string> sResultUsingIndex2_2(vResultUsingIndex2_2.begin(),vResultUsingIndex2_2.end());
	set<string> sResultUsingIndex3_1(vResultUsingIndex3_1.begin(),vResultUsingIndex3_1.end());
	CHECK_BOOL(sResultUsingIndex2_2 == sExpectResult3);
	CHECK_BOOL(sResultUsingIndex3_1 == sExpectResult3);

	//�ڵ�1�к͵�3���ϲ�ѯ(��ʹ�õ�������Index1)
	SearchCondition search4;
	search4.Add(1,LessThan,"qe4",str_compare);
	search4.Add(2,GreaterThan,"t1",str_compare);

	std::set<string> sExpected4;
	sExpected4.insert("qe3iowrt2923484jw2t7qt740t0qte");

	std::vector<string> vResult4;
	GetIndexScanResults(vResult4,pTrans,pIndex1,search4.Keys());

	set<string> sResult4(vResult4.begin(),vResult4.end());
	CHECK_BOOL(sExpected4 == sResult4);

	//�ڵ�3�к͵�5���ϲ�ѯ����ʹ�õ�������Index2)
	SearchCondition search5;
	search5.Add(1,GreaterThan,"t2",str_compare);
	search5.Add(2,LessThan,"rt7qt4",str_compare);

	std::set<string> sExpected5;
	sExpected5.insert("qe8iowrt5923484owht0qt420t0qte");

	std::vector<string> vResult5;
	GetIndexScanResults(vResult5,pTrans,pIndex2,search5.Keys());

	set<string> sResult5(vResult5.begin(),vResult5.end());
	CHECK_BOOL(sExpected5 == sResult5);
}

static void CloseAll(Transaction *pTrans,EntrySet *pEntrySet,IndexEntrySet *pIndex1,IndexEntrySet *pIndex2,IndexEntrySet *pIndex3)
{
	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	if (NULL != pEntrySet)
	{
         pStorageEngine->closeEntrySet(pTrans,pEntrySet);
	}

	if (NULL != pIndex1)
	{
		pStorageEngine->closeEntrySet(pTrans,pIndex1);
	}

	if (NULL != pIndex2)
	{
		pStorageEngine->closeEntrySet(pTrans,pIndex2);
	}

	if (NULL != pIndex3)
	{
		pStorageEngine->closeEntrySet(pTrans,pIndex3);
	}
}
extern Transaction* pTransaction;
bool test_trans_multi_index( void )
{
	INTENT("1. ����һ����6�еı�;"
		   "2. �����ϴ���3������;"
		   "3. �ύ������һ����;"
		   "4. update���е����ݺ�ع�;"
		   "5. ����һ�����������ԭ����.");


	//����һ����6��,����������3����4��,2��,8��,6��,7���ַ�
	typedef EntrySetCollInfo<3,4,2,8,6,7> EntrySetT;

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;
	EntrySetID  entryId,index1Id,index2Id,index3Id;

	//1. �������index��������һЩ����
    try
    {
		//����һ��EntrySet������һЩ����
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = EntrySetCreator<EntrySetT>::createAndOpen(pTransaction,"test");
		entryId =  pEntrySet->getId();

		vector<string> vInserted1;
		vInserted1 += "qe3iowrt0923484owrt0qt4q0t0qte","qeliowrt2923484jwrt7qt440t0qte";
		InsertData(pTransaction,pEntrySet,vInserted1,&vIds);

		//�ڵ�1�к͵�3���Ͻ����������,��������һЩ����
		IndexEntrySet *pIndex1 = IndexCreator<EntrySetT,1,3>::createAndOpen(pTransaction,pEntrySet,"index1");
		index1Id = pIndex1->getId();

		vector<string> vInserted2;
		vInserted2 += "qpliowr40923484o2rt0qt4q0t0qte","qe3iowrt2923484jw2t7qt740t0qte";
		InsertData(pTransaction,pEntrySet,vInserted2,&vIds);


		//�ڵ����к͵�5�н����������,��������һЩ����
		IndexEntrySet *pIndex2 = IndexCreator<EntrySetT,3,5>::createAndOpen(pTransaction,pEntrySet,"index2");
		index2Id = pIndex2->getId();

		vector<string> vInserted3;
		vInserted3 += "qe7io7rt0922484owrt0qg4q0t0qte","qe9iowrt2823484jwrt1qt44050qte";
		InsertData(pTransaction,pEntrySet,vInserted3,&vIds);

		//�ڵ�5���Ͻ�������,��������һЩ����
		IndexEntrySet *pIndex3 = IndexCreator<EntrySetT,5>::createAndOpen(pTransaction,pEntrySet,"index3");
		index3Id = pIndex3->getId();
		vector<string> vInserted4;
		vInserted4 += "qe8iowrt5923484owht0qt420t0qte","qeliowrw2923484ewrt7qt4n0t0qte";
		InsertData(pTransaction,pEntrySet,vInserted4,&vIds);


        CheckIndexScan(pTransaction,pIndex1,pIndex2,pIndex3);
        
		CloseAll(pTransaction,pEntrySet,pIndex1,pIndex2,pIndex3);
        pTransaction->commit();
    }
    CATCHEXCEPTION

	//2.���º�ع�
	try
	{
		//�򿪱��index
		transId = 0; 
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,entryId);
    /*    IndexEntrySet *pIndex1 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,0,index1Id,NULL,NULL);
		IndexEntrySet *pIndex2 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,0,index2Id,NULL,NULL);
		IndexEntrySet *pIndex3 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,0,index3Id,NULL,NULL);
*/
		map<EntryID,string> updateData;
		updateData[vIds[1]] = "qe3iowrt1723484jwrt1rt44050qte";
		updateData[vIds[2]] = "qb3iow3t1723484jwrm1ct44050qte";
        UpdateData(pTransaction,pEntrySet,updateData);
        

		pStorageEngine->closeEntrySet(pTransaction,pEntrySet);
       // CloseAll(pTransaction,pEntrySet,pIndex1,pIndex2,pIndex3);
		pTransaction->abort();
	}
	CATCHEXCEPTION

    //��������ԭ����
	try
	{
		transId = 0; 
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = (EntrySet *)pStorageEngine->openEntrySet(pTransaction,EntrySet::OPEN_EXCLUSIVE,entryId);
		IndexEntrySet *pIndex1 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,index1Id);
		IndexEntrySet *pIndex2 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,index2Id);
		IndexEntrySet *pIndex3 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTransaction,pEntrySet,EntrySet::OPEN_EXCLUSIVE,index3Id);

		CheckIndexScan(pTransaction,pIndex1,pIndex2,pIndex3);
		CloseAll(pTransaction,pEntrySet,pIndex1,pIndex2,pIndex3);
		pTransaction->commit();
	}
	CATCHEXCEPTION

	//Clean up
	try
	{
		transId = 0; 
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pStorageEngine->removeEntrySet(pTransaction,entryId);
		pTransaction->commit();
	}
	CATCHEXCEPTION
	return true;
}

/************************************************************************** 
* @brief createOpenEntrySet 
* ��������һ�����б䳤�ֶε�Entryset
* Detailed description.
* @param[in] pTrans ��ǰ����
* @param[in] strName Entryset������ 
* @return EntrySet*  ���ش�����entry
**************************************************************************/
static EntrySet* createOpenEntrySet(Transaction* pTrans,string const& strName)
{
	EntrySetID id = StorageEngine::getStorageEngine()->createEntrySet(pTrans
		,VarLenColumnInfo<>::get());

	EntrySet *pEntrySet = (EntrySet *)StorageEngine::getStorageEngine()->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,id);
	return pEntrySet;
}

/************************************************************************** 
* @brief VarIndexCreator 
* ��pEntrySet��ָ�����ϴ�����������
* Detailed description.
* @param[in] pTrans ��ǰ����
* @param[in] pEntrySet ��Ҫ����index��EntrySet
* @param[in] strName ����������
* @param[in] N0...N9 �������ڵ���
* @return IndexEntrySet*  
**************************************************************************/
template<BOOST_PP_ENUM(TIMES,NN,~)>
class VarIndexCreator
{
public:
	static IndexEntrySet* create(Transaction* pTrans
		,EntrySet* pEntrySet
		,string const& strName)
	{
		EntrySetID idIndex = StorageEngine::getStorageEngine()->createIndexEntrySet(pTrans
			,pEntrySet
			,BTREE_INDEX_ENTRY_SET_TYPE
			,VarLenColumnInfo<BOOST_PP_ENUM_PARAMS(TIMES,N)>::get());

		IndexEntrySet *pIndex = (IndexEntrySet *)StorageEngine::getStorageEngine()->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,idIndex);
		return pIndex;
	}

};

/************************************************************************** 
* @brief CheckVarIndexScan 
* ��������ھ��б䳤���Եı����йظ��»ع�ʱ�����ԭ�����Ƿ�����
* Detailed description.
* @param[in] pTrans ��ǰ����
* @param[in] pIndex1 ��һ�к͵������Ͻ������������
* @param[in] pIndex2 �����к͵������Ͻ������������
* @param[in] pIndex3 �������Ͻ���������
**************************************************************************/
static void CheckVarIndexScan(Transaction* pTrans
							  ,IndexEntrySet* pIndex1
							  ,IndexEntrySet* pIndex2
							  ,IndexEntrySet *pIndex3)
{
	//�ڵ�һ���ϲ�ѯ�������(��ʹ��Index1: "qe" < ��һ�� < "qe5")
	SearchCondition search1;
	search1.Add(1,LessThan,"qe5",vstr_compare);
	search1.Add(1,GreaterThan,"qe",vstr_compare);
cout<<"========================================================="<<endl;
	set<string> sExpectResult1;
	sExpectResult1 += "qe3|iow||rt0|923|484","qe3i|ow|rt292|3484j|w2t|7q";
	vector<string> vResultUsingIndex1_1;
	GetIndexScanResults(vResultUsingIndex1_1,pTrans,pIndex1,search1.Keys());

	set<string> sResultUsingIndex1_1(vResultUsingIndex1_1.begin(),vResultUsingIndex1_1.end());
	CHECK_BOOL(sResultUsingIndex1_1 == sExpectResult1);

	//�ڵ������ϲ�ѯ�������(����ʹ��Index1��Index2: "a" < ������ <= "rt2")
	SearchCondition search2_1,search2_2;
	search2_1.Add(2,GreaterThan,"a",vstr_compare); //for index1 ��3����index1���ǵ�2��
	search2_1.Add(2,LessEqual,"rt2",vstr_compare); 

	search2_2.Add(1,GreaterEqual,"a",vstr_compare); //for index2 ��3����index3���ǵ�1��
	search2_2.Add(1,LessEqual,"rt2",vstr_compare); 
	set<string> sExpectResult2;
	sExpectResult2 += "q|eli|owr"
		,"q|pliow|r40|923484o|2rt0q|t4"
		,"qe|liow|rt2|92|3484|jwr";

	vector<string> vResultUsingIndex1_2,vResultUsingIndex2_1;
	GetIndexScanResults(vResultUsingIndex1_2,pTrans,pIndex1,search2_1.Keys());
	GetIndexScanResults(vResultUsingIndex2_1,pTrans,pIndex2,search2_2.Keys());

	set<string> sResultUsingIndex1_2(vResultUsingIndex1_2.begin(),vResultUsingIndex1_2.end());
	set<string> sResultUsingIndex2_1(vResultUsingIndex2_1.begin(),vResultUsingIndex2_1.end());

	CHECK_BOOL(sExpectResult2 == sResultUsingIndex1_2);
	CHECK_BOOL(sExpectResult2 == sResultUsingIndex2_1);

	//�ڵ�5���ϲ�ѯ(��ʹ�õ���Index2��Index3:)
	SearchCondition search3_1,search3_2;
	search3_1.Add(2,LessThan,"35",vstr_compare);   //for index2 ��5����index2���ǵ�2��
	search3_2.Add(1,LessThan,"35",vstr_compare);   //for index3  ��5����index3���ǵ�1��

	set<string> sExpectResult3;
	sExpectResult3 += "q|eli|owr",
		"q|pliow|r40|923484o|2rt0q|t4",
		"qe|liow|rt2|92|3484|jwr";

	vector<string> vResultUsingIndex2_2,vResultUsingIndex3_1;
	GetIndexScanResults(vResultUsingIndex2_2,pTrans,pIndex2,search3_1.Keys());
	GetIndexScanResults(vResultUsingIndex3_1,pTrans,pIndex3,search3_2.Keys());

	set<string> sResultUsingIndex2_2(vResultUsingIndex2_2.begin(),vResultUsingIndex2_2.end());
	set<string> sResultUsingIndex3_1(vResultUsingIndex3_1.begin(),vResultUsingIndex3_1.end());
	CHECK_BOOL(sResultUsingIndex2_2 == sExpectResult3);
	CHECK_BOOL(sResultUsingIndex3_1 == sExpectResult3);

	//�ڵ�1�к͵�3���ϲ�ѯ(��ʹ�õ�������Index1)
	SearchCondition search4;
	search4.Add(1,LessEqual,"q",vstr_compare);
	search4.Add(1,GreaterThan,"owr",vstr_compare);
	search4.Add(2,LessThan,"r41",vstr_compare);
	search4.Add(2,GreaterThan,"owr",vstr_compare);

	std::set<string> sExpected4;
	sExpected4.insert("q|pliow|r40|923484o|2rt0q|t4");

	std::vector<string> vResult4;
	GetIndexScanResults(vResult4,pTrans,pIndex1,search4.Keys());

	set<string> sResult4(vResult4.begin(),vResult4.end());
	CHECK_BOOL(sExpected4 == sResult4);

	//�ڵ�3�к͵�5���ϲ�ѯ����ʹ�õ�������Index2)
	SearchCondition search5;
	search5.Add(1,LessEqual,"7io7r",vstr_compare);
	search5.Add(2,LessEqual,"484o",vstr_compare);

	std::set<string> sExpected5;
	sExpected5.insert("|qe|7io7r|t0922|484o|wrt0");

	std::vector<string> vResult5;
	GetIndexScanResults(vResult5,pTrans,pIndex2,search5.Keys());

	set<string> sResult5(vResult5.begin(),vResult5.end());
	CHECK_BOOL(sExpected5 == sResult5);
}

bool test_trans_multi_index_nullvalue( void )
{
	INTENT("1. ����һ����6�еı�(����������Ǳ䳤��);"
		"2. �����ϴ���3������;"
		"3. �ύ������һ����;"
		"4. update���е����ݺ�ع�;"
		"5. ����һ�����������ԭ����.");


	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;
	EntrySetID  entryId,index1Id,index2Id,index3Id;

	//1. �������index��������һЩ����
	try
	{
		//����һ��EntrySet������һЩ����
		Transaction* pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = createOpenEntrySet(pTrans,"test");
		entryId =  pEntrySet->getId();

		vector<string> vInserted1;
		vInserted1 += "qe3|iow||rt0|923|484","qe|liow|rt2|92|3484|jwr";
		InsertData(pTrans,pEntrySet,vInserted1,&vIds);

		//�ڵ�1�к͵�3���Ͻ����������,��������һЩ����
		IndexEntrySet *pIndex1 = VarIndexCreator<1,3>::create(pTrans,pEntrySet,"index1");
		index1Id = pIndex1->getId();

		vector<string> vInserted2;
		vInserted2 += "q|pliow|r40|923484o|2rt0q|t4","qe3i|ow|rt292|3484j|w2t|7q";
		InsertData(pTrans,pEntrySet,vInserted2,&vIds);


		//�ڵ����к͵�5�н����������,��������һЩ����
		IndexEntrySet *pIndex2 = VarIndexCreator<3,5>::create(pTrans,pEntrySet,"index1");
		index2Id = pIndex2->getId();

		vector<string> vInserted3;
		vInserted3 += "|qe|7io7r|t0922|484o|wrt0","qe|9iow|rt28|234|84jw|rt";
		InsertData(pTrans,pEntrySet,vInserted3,&vIds);

		//�ڵ�5���Ͻ�������,��������һЩ����
		IndexEntrySet *pIndex3 = VarIndexCreator<5>::create(pTrans,pEntrySet,"index1");
		index3Id = pIndex3->getId();
		vector<string> vInserted4;
		vInserted4 += "qe8i|owrt|5923|484o|wht|","q|eli|owr";
		InsertData(pTrans,pEntrySet,vInserted4,&vIds);


		CheckVarIndexScan(pTrans,pIndex1,pIndex2,pIndex3);

		CloseAll(pTrans,pEntrySet,pIndex1,pIndex2,pIndex3);
		pTrans->commit();
	}
	CATCHEXCEPTION

	//2.���º�ع�
	try
	{
		//�򿪱��index
		transId = 0; 
		Transaction* pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = (EntrySet *)pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,entryId);
		/*    IndexEntrySet *pIndex1 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTrans,pEntrySet,0,index1Id,NULL,NULL);
		IndexEntrySet *pIndex2 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTrans,pEntrySet,0,index2Id,NULL,NULL);
		IndexEntrySet *pIndex3 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTrans,pEntrySet,0,index3Id,NULL,NULL);
		*/
		map<EntryID,string> updateData;
		updateData[vIds[1]] = "qe|3iowrt|17234=|84jw|rt1rt|44050qte";
		updateData[vIds[2]] = "q|b3i|ow3|t1|7234|84j";
		UpdateData(pTrans,pEntrySet,updateData);


		pStorageEngine->closeEntrySet(pTrans,pEntrySet);
		// CloseAll(pTrans,pEntrySet,pIndex1,pIndex2,pIndex3);
		pTrans->abort();
	}
	CATCHEXCEPTION

	//��������ԭ����
	try
	{
		transId = 0; 
		Transaction* pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = (EntrySet *)pStorageEngine->openEntrySet(pTrans,EntrySet::OPEN_EXCLUSIVE,entryId);
		IndexEntrySet *pIndex1 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,index1Id);
		IndexEntrySet *pIndex2 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,index2Id);
		IndexEntrySet *pIndex3 = (IndexEntrySet *)pStorageEngine->openIndexEntrySet(pTrans,pEntrySet,EntrySet::OPEN_EXCLUSIVE,index3Id);

		CheckVarIndexScan(pTrans,pIndex1,pIndex2,pIndex3);
		CloseAll(pTrans,pEntrySet,pIndex1,pIndex2,pIndex3);
		pTrans->commit();
	}
	CATCHEXCEPTION

	//Clean up
	try
	{
		transId = 0; 
		Transaction* pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		pStorageEngine->removeEntrySet(pTrans,entryId);
		pTrans->commit();
	}
	CATCHEXCEPTION
	return true;
}