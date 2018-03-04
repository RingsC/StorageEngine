/**
* @file test_trans_multi_index_update.cpp
* @brief 
* @author 李书淦
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
* 检查在一个只有固定字段的表上进行update时回滚是否满足事务的原子性
* Detailed description.
* @param[in] pTrans 当前事务
* @param[in] pIndex1 在第一列和第三列上建立的组合索引
* @param[in] pIndex2 在和三列和第五列上建立的组合索引
* @param[in] pIndex3 在第五列上建立的索引
**************************************************************************/
static void CheckIndexScan(Transaction *pTrans
						   ,IndexEntrySet* pIndex1
						   ,IndexEntrySet* pIndex2
						   ,IndexEntrySet *pIndex3)
{
	//在第一列上查询并检查结果(可使用Index1: "qe9" <= 第一列 < "qp1")
	SearchCondition search1;
	search1.Add(1,LessThan,"qp1",str_compare);
	search1.Add(1,GreaterEqual,"qe9",str_compare);

	set<string> sExpectResult1;
	sExpectResult1 += "qeliowrt2923484jwrt7qt440t0qte","qe9iowrt2823484jwrt1qt44050qte","qeliowrw2923484ewrt7qt4n0t0qte";
	vector<string> vResultUsingIndex1_1;
	GetIndexScanResults(vResultUsingIndex1_1,pTrans,pIndex1,search1.Keys());

	set<string> sResultUsingIndex1_1(vResultUsingIndex1_1.begin(),vResultUsingIndex1_1.end());
	CHECK_BOOL(sResultUsingIndex1_1 == sExpectResult1);

	//在第三列上查询并检查结果(可以使用Index1和Index2: "t2" <= 第三列)
	SearchCondition search2_1,search2_2;
	search2_1.Add(2,GreaterEqual,"t2",str_compare); //for index1 第3列在index1中是第2列
	search2_2.Add(1,GreaterEqual,"t2",str_compare); //for index2 第3列在index3中是第1列
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

	//在第5列上查询(可使用的有Index2和Index3:)
	SearchCondition search3_1,search3_2;
	search3_1.Add(2,GreaterEqual,"rt0qg4",str_compare);   //for index2 第5列在index2中是第2列
	search3_1.Add(2,LessThan,"rt1qt4",str_compare);       //for index2

	search3_2.Add(1,GreaterEqual,"rt0qg4",str_compare);   //for index3  第5列在index3中是第1列
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

	//在第1列和第3列上查询(可使用的索引有Index1)
	SearchCondition search4;
	search4.Add(1,LessThan,"qe4",str_compare);
	search4.Add(2,GreaterThan,"t1",str_compare);

	std::set<string> sExpected4;
	sExpected4.insert("qe3iowrt2923484jw2t7qt740t0qte");

	std::vector<string> vResult4;
	GetIndexScanResults(vResult4,pTrans,pIndex1,search4.Keys());

	set<string> sResult4(vResult4.begin(),vResult4.end());
	CHECK_BOOL(sExpected4 == sResult4);

	//在第3列和第5列上查询（可使用的索引有Index2)
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
	INTENT("1. 创建一个有6列的表;"
		   "2. 在其上创建3个索引;"
		   "3. 提交后开启另一事务;"
		   "4. update表中的数据后回滚;"
		   "5. 另启一事务检查事务的原子性.");


	//申明一个有6列,各列依次有3个，4个,2个,8个,6个,7个字符
	typedef EntrySetCollInfo<3,4,2,8,6,7> EntrySetT;

	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;
	EntrySetID  entryId,index1Id,index2Id,index3Id;

	//1. 创建表和index，并插入一些数据
    try
    {
		//创建一个EntrySet并插入一些数据
		pTransaction = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = EntrySetCreator<EntrySetT>::createAndOpen(pTransaction,"test");
		entryId =  pEntrySet->getId();

		vector<string> vInserted1;
		vInserted1 += "qe3iowrt0923484owrt0qt4q0t0qte","qeliowrt2923484jwrt7qt440t0qte";
		InsertData(pTransaction,pEntrySet,vInserted1,&vIds);

		//在第1列和第3列上建立组合索引,插入另外一些数据
		IndexEntrySet *pIndex1 = IndexCreator<EntrySetT,1,3>::createAndOpen(pTransaction,pEntrySet,"index1");
		index1Id = pIndex1->getId();

		vector<string> vInserted2;
		vInserted2 += "qpliowr40923484o2rt0qt4q0t0qte","qe3iowrt2923484jw2t7qt740t0qte";
		InsertData(pTransaction,pEntrySet,vInserted2,&vIds);


		//在第三列和第5列建立组合索引,插入另外一些数据
		IndexEntrySet *pIndex2 = IndexCreator<EntrySetT,3,5>::createAndOpen(pTransaction,pEntrySet,"index2");
		index2Id = pIndex2->getId();

		vector<string> vInserted3;
		vInserted3 += "qe7io7rt0922484owrt0qg4q0t0qte","qe9iowrt2823484jwrt1qt44050qte";
		InsertData(pTransaction,pEntrySet,vInserted3,&vIds);

		//在第5列上建立索引,插入另外一些数据
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

	//2.更新后回滚
	try
	{
		//打开表和index
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

    //检查事务的原子性
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
* 创建并打开一个具有变长字段的Entryset
* Detailed description.
* @param[in] pTrans 当前事务
* @param[in] strName Entryset的名称 
* @return EntrySet*  返回创建的entry
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
* 在pEntrySet的指定列上创建并打开索引
* Detailed description.
* @param[in] pTrans 当前事务
* @param[in] pEntrySet 需要创建index的EntrySet
* @param[in] strName 索引的名称
* @param[in] N0...N9 索引所在的列
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
* 用来检查在具有变长属性的表上有关更新回滚时事务的原子性是否满足
* Detailed description.
* @param[in] pTrans 当前事务
* @param[in] pIndex1 第一列和第三列上建立的组合索引
* @param[in] pIndex2 第三列和第五列上建立的组合索引
* @param[in] pIndex3 第五列上建立的索引
**************************************************************************/
static void CheckVarIndexScan(Transaction* pTrans
							  ,IndexEntrySet* pIndex1
							  ,IndexEntrySet* pIndex2
							  ,IndexEntrySet *pIndex3)
{
	//在第一列上查询并检查结果(可使用Index1: "qe" < 第一列 < "qe5")
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

	//在第三列上查询并检查结果(可以使用Index1和Index2: "a" < 第三列 <= "rt2")
	SearchCondition search2_1,search2_2;
	search2_1.Add(2,GreaterThan,"a",vstr_compare); //for index1 第3列在index1中是第2列
	search2_1.Add(2,LessEqual,"rt2",vstr_compare); 

	search2_2.Add(1,GreaterEqual,"a",vstr_compare); //for index2 第3列在index3中是第1列
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

	//在第5列上查询(可使用的有Index2和Index3:)
	SearchCondition search3_1,search3_2;
	search3_1.Add(2,LessThan,"35",vstr_compare);   //for index2 第5列在index2中是第2列
	search3_2.Add(1,LessThan,"35",vstr_compare);   //for index3  第5列在index3中是第1列

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

	//在第1列和第3列上查询(可使用的索引有Index1)
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

	//在第3列和第5列上查询（可使用的索引有Index2)
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
	INTENT("1. 创建一个有6列的表(插入的数据是变长的);"
		"2. 在其上创建3个索引;"
		"3. 提交后开启另一事务;"
		"4. update表中的数据后回滚;"
		"5. 另启一事务检查事务的原子性.");


	StorageEngine *pStorageEngine = StorageEngine::getStorageEngine();
	TransactionId transId = 0; 
	vector<EntryID> vIds;
	EntrySetID  entryId,index1Id,index2Id,index3Id;

	//1. 创建表和index，并插入一些数据
	try
	{
		//创建一个EntrySet并插入一些数据
		Transaction* pTrans = pStorageEngine->getTransaction(transId,Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		EntrySet *pEntrySet = createOpenEntrySet(pTrans,"test");
		entryId =  pEntrySet->getId();

		vector<string> vInserted1;
		vInserted1 += "qe3|iow||rt0|923|484","qe|liow|rt2|92|3484|jwr";
		InsertData(pTrans,pEntrySet,vInserted1,&vIds);

		//在第1列和第3列上建立组合索引,插入另外一些数据
		IndexEntrySet *pIndex1 = VarIndexCreator<1,3>::create(pTrans,pEntrySet,"index1");
		index1Id = pIndex1->getId();

		vector<string> vInserted2;
		vInserted2 += "q|pliow|r40|923484o|2rt0q|t4","qe3i|ow|rt292|3484j|w2t|7q";
		InsertData(pTrans,pEntrySet,vInserted2,&vIds);


		//在第三列和第5列建立组合索引,插入另外一些数据
		IndexEntrySet *pIndex2 = VarIndexCreator<3,5>::create(pTrans,pEntrySet,"index1");
		index2Id = pIndex2->getId();

		vector<string> vInserted3;
		vInserted3 += "|qe|7io7r|t0922|484o|wrt0","qe|9iow|rt28|234|84jw|rt";
		InsertData(pTrans,pEntrySet,vInserted3,&vIds);

		//在第5列上建立索引,插入另外一些数据
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

	//2.更新后回滚
	try
	{
		//打开表和index
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

	//检查事务的原子性
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