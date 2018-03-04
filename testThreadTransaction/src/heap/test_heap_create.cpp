#include "heap/test_heap_create.h"

void my_split(RangeDatai &rangeData, const char* str, int col, size_t len = 0)
{

	rangeData.len = 0;
	rangeData.start = 0;
	if(str == NULL)
	{
		return;
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
	return;
}

bool test_heap_create_task(ParamBase* arg)
{
	HeapBase *pArgs = (HeapBase*)arg;
	std::cout<<">>>>>>>>>>>>>"<<pArgs->strCaseName<<"<<<<<<<<<<<<"<<std::endl;
	try
	{
		if (pArgs->isMultiCol)
		{
			pArgs->entrysetId = StorageEngine::getStorageEngine()->createEntrySet(pArgs->GetTransaction(),GetMultiCollInfo());
		}
		else
		{
			pArgs->entrysetId = StorageEngine::getStorageEngine()->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	//MemLog.Print("Create EntrySet ID : %d \n",pArgs->entrysetId);

	return true;
}
bool test_heap_create_drop_task(ParamBase* arg)
{
	HeapBase *pArgs = (HeapBase*)arg;
	//std::cout<<">>>>>>>>>>>>"<<pArgs->strCaseName<<"<<<<<<<<<<<<<"<<std::endl;
	try
	{
		StorageEngine::getStorageEngine()->removeEntrySet(pArgs->GetTransaction(),pArgs->entrysetId);
	}
	catch(StorageEngineException &ex)
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;

		//MemLog.Print(">>>>>>>>>>>>>>>>>>Drop failure heap id : %d \n",pArgs->entrysetId);

		//MemLog.Print("Print End !\n");
		//MemLog.Save("E:\\log.txt");

		arg->SetSuccessFlag(false);
	}

	return true;
}

#define testdata "testdata_1"
bool test_heap_create_insert_task(ParamBase* arg)
{
	HeapBase* pArgs = (HeapBase*)arg;
	try
	{
		EntrySet *pEntrySet = NULL;
		pEntrySet=StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);//打开表,transaction传NULL
		DataItem data;
		int len = sizeof(testdata);
		char *pstr = (char *)malloc(len);//记得释放空间...free(pstr);
		memcpy(pstr, testdata, len);//构建DataItem 
		data.setData((void *)pstr);//pstr中的字符及参数传给data
		data.setSize(len);

		EntryID tid;
		pEntrySet->insertEntry(pArgs->GetTransaction(), tid, data);//插入数据
		command_counter_increment();
		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_create_query_task(ParamBase* arg)
{
	HeapBase *pArgs = (HeapBase*)arg;
	try
	{
		EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(),EntrySet::OPEN_EXCLUSIVE,pArgs->entrysetId);
		EntrySetScan *pEntrySetScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(), BaseEntrySet::SnapshotMVCC, (std::vector<ScanCondition>)NULL);//开始扫描
		DataItem getdata;
		
		unsigned int length;
		EntryID eid;
		pEntrySetScan->getNext(EntrySetScan::NEXT_FLAG,eid,getdata);//暂时通过getdata判断后面是否还有数据，以后可以通过返回的int是否为NULL判断
		char *str = (char*)getdata.getData();//getdata中的数据给str
		length=getdata.getSize();//大小

		pArgs->SetSuccessFlag(length!=0);
		if (length==0)
		{
			std::cout<<"查询的数据长度为0，出错!"<<std::endl;
		}
		int flag = 0;
		flag=memcmp(testdata,str,sizeof(str));
		pArgs->SetSuccessFlag(flag==0);
		if (flag!=0)
		{
			std::cout<<"查询的数据与预测的不同，出错!"<<std::endl;
		}
		pEntrySet->endEntrySetScan(pEntrySetScan);
		StorageEngine::getStorageEngine()->closeEntrySet(pArgs->GetTransaction(),pEntrySet);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_heap_create()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
	HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_1()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
	HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_2()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
		HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_3()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
		HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_4()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
		HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_5()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
		HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_6()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
		HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_7()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
		HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_8()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
		HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_9()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
	HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}
bool test_heap_create_10()
{
	//通过插入数据并查出来确认表已经创建成功

	INITRANID()
		HeapBase *arg = new HeapBase;
	arg->strCaseName = __FUNCTION__;
	REGTASK(test_heap_create_task,arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task,arg);

	return true;
}

bool test_heap_drop()
{
	INITRANID()

	HeapBase *arg = new HeapBase;
	REGTASK(test_heap_create_task, arg);
	REGTASK(test_heap_create_insert_task,arg);
	REGTASK(test_heap_create_query_task,arg);
	REGTASK(test_heap_create_drop_task, arg);

	return true;
}

bool create_multi_heap_task(ParamBase* arg)
{
	HeapMulti *pArgs = (HeapMulti*)arg;
	std::cout<<">>>>>>>>>>>>>"<<pArgs->strCaseName<<"<<<<<<<<<<<<"<<std::endl;
	try
	{
		for (int nIndex = 0; nIndex < pArgs->nCount; nIndex ++)
		{
			EntrySetID entrysetId = InvalidEntrySetID;
			if (pArgs->isMultiCol)
			{
				entrysetId = StorageEngine::getStorageEngine()->createEntrySet(pArgs->GetTransaction(),GetMultiCollInfo());
			}
			else
			{
				entrysetId = StorageEngine::getStorageEngine()->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());
			}
			if (entrysetId)
				pArgs->vEntrySetID.push_back(entrysetId);
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}

bool heap_multi_insert(ParamBase* arg)
{
	HeapMulti* pArgs = (HeapMulti*)arg;
	try
	{
		std::vector<EntrySetID>::iterator it;
		for (it = pArgs->vEntrySetID.begin(); it != pArgs->vEntrySetID.end(); it++)
		{
			std::vector<std::string> vData;
			vData.push_back("123abc");
			vData.push_back("123efg");
			vData.push_back("123rty");
			vData.push_back("567gghert");
			vData.push_back("ddderty");
			vData.push_back("qqqwert");

			EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(), EntrySet::OPEN_EXCLUSIVE,*it);
			InsertData(pArgs->GetTransaction(),pEntrySet,vData);
		}
		pArgs->mapWillUpdateData.insert(std::make_pair("123abc","abc123"));
		pArgs->mapWillUpdateData.insert(std::make_pair("123rty","rty123"));

		pArgs->vAfterUpdateData.insert("abc123");
		pArgs->vAfterUpdateData.insert("123efg");
		pArgs->vAfterUpdateData.insert("rty123");
		pArgs->vAfterUpdateData.insert("567gghert");
		pArgs->vAfterUpdateData.insert("ddderty");
		pArgs->vAfterUpdateData.insert("qqqwert");

		pArgs->vWillDeleData.push_back("123efg");
		pArgs->vWillDeleData.push_back("rty123");

		pArgs->vAfterDeleData.insert("abc123");
		pArgs->vAfterDeleData.insert("567gghert");
		pArgs->vAfterDeleData.insert("ddderty");
		pArgs->vAfterDeleData.insert("qqqwert");
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}

bool FoundDeleteData(const std::string& strDele, const std::vector<std::string>& vData)
{
	bool bRet = false;
	std::string strTmp = strDele;
	std::vector<std::string>::const_iterator it;
	for (it = vData.begin(); it != vData.end(); it++)
	{
		if (strTmp == *it)
		{
			bRet = true;
			break;
		}
	}

	return bRet;
}
bool multi_heap_delete(ParamBase* arg)
{
	HeapMulti* pArgs = (HeapMulti*)arg;
	try
	{
		for (int nIndex = 0; nIndex < pArgs->vEntrySetID.size(); nIndex ++)
		{
			std::vector<EntryID> vDeleEntryId;
			EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(), EntrySet::OPEN_EXCLUSIVE,pArgs->vEntrySetID.at(nIndex));
			EntrySetScan* pScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(),BaseEntrySet::SnapshotMVCC,std::vector<ScanCondition>(NULL));
			EntryID eid;
			DataItem item;
			while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
			{
				std::string strTmp = (char*)item.getData();
				std::string strUpdate;
				if (FoundDeleteData(strTmp,pArgs->vWillDeleData))
					vDeleEntryId.push_back(eid);
			}
			pEntrySet->endEntrySetScan(pScan);

			DeleteData(pArgs->GetTransaction(),pEntrySet,vDeleEntryId);	
			std::vector<std::string> vResult;
			GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);
			std::set<std::string> setResult(vResult.begin(),vResult.end());
			bool bFlag = (setResult == pArgs->vAfterDeleData);
			pArgs->SetSuccessFlag(bFlag);
			if (!bFlag)
				break;
		}

	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool FouondUpdateData(const std::string& strElement, const std::map<std::string,std::string>& vData, std::string& strUpdate)
{
	bool bRet = false;
	std::string strTmp = strElement;
	std::map<std::string,std::string>::const_iterator it;
	for (it = vData.begin(); it != vData.end(); it++)
	{
		if (strTmp == it->first)
		{
			bRet = true;
			strUpdate = it->second;
			break;
		}
	}
	return bRet;
}
bool multi_heap_update(ParamBase* arg)
{
	HeapMulti* pArgs = (HeapMulti*)arg;
	try
	{
		for (int nIndex = 0; nIndex < pArgs->vEntrySetID.size(); nIndex++)
		{
			std::map<EntryID,std::string> mapUpdate;
			EntrySet* pEntrySet = StorageEngine::getStorageEngine()->openEntrySet(pArgs->GetTransaction(), EntrySet::OPEN_EXCLUSIVE,pArgs->vEntrySetID.at(nIndex));
			EntrySetScan* pScan = pEntrySet->startEntrySetScan(pArgs->GetTransaction(),BaseEntrySet::SnapshotMVCC,std::vector<ScanCondition>(NULL));
			EntryID eid;
			DataItem item;
			while (NO_DATA_FOUND != pScan->getNext(EntrySetScan::NEXT_FLAG,eid,item))
			{
				std::string strTmp = (char*)item.getData();
				std::string strUpdate;
				if (FouondUpdateData(strTmp,pArgs->mapWillUpdateData,strUpdate))
					mapUpdate.insert(make_pair(eid,strUpdate));
			}

			pEntrySet->endEntrySetScan(pScan);

			UpdateData(pArgs->GetTransaction(),pEntrySet,mapUpdate);	
			std::vector<std::string> vResult;
			GetDataFromEntrySet(vResult,pArgs->GetTransaction(),pEntrySet);
			std::set<std::string> setResult(vResult.begin(),vResult.end());
			bool bFlag = (setResult == pArgs->vAfterUpdateData);
			pArgs->SetSuccessFlag(bFlag);
			if (!bFlag)
				break;			
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool drop_multi_heap_task(ParamBase* arg)
{
	HeapMulti* pArgs = (HeapMulti*)arg;
	try
	{
		std::vector<EntrySetID>::iterator it;
		for (it = pArgs->vEntrySetID.begin(); it != pArgs->vEntrySetID.end(); it++)
		{
			StorageEngine::getStorageEngine()->removeEntrySet(pArgs->GetTransaction(),*it);
		}
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}

bool create_one_heap_task(ParamBase* arg)
{
	HeapMulti* pArgs = (HeapMulti*)arg;
	try
	{
		EntrySetID entrySetId = InvalidEntrySetID;
		if (pArgs->isMultiCol)
		{
			entrySetId = StorageEngine::getStorageEngine()->createEntrySet(pArgs->GetTransaction(),GetMultiCollInfo());
		}
		else
		{
			entrySetId = StorageEngine::getStorageEngine()->createEntrySet(pArgs->GetTransaction(),GetSingleColInfo());
		}
		if (entrySetId)
			pArgs->vEntrySetID.push_back(entrySetId);
	}
	EXCEPTION_CATCH_SET_FLAG(pArgs)

	return true;
}
bool test_create_multi_heap_one_transaction()
{
	INITRANID()

	HeapMulti* arg = new HeapMulti;
	arg->nCount = 3;
	REGTASK(create_multi_heap_task, arg);

	REGTASK(create_one_heap_task, arg);

	REGTASK(heap_multi_insert, arg);

	//update
	REGTASK(multi_heap_update, arg);

	//delete
	REGTASK(multi_heap_delete, arg);

	REGTASK(drop_multi_heap_task, arg);

	return true;
}