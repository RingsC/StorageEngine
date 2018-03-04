#include <iostream>
#include "TestFrame/TestFrame.h"
#include "Configs.h"
#include "StorageEngine.h"
#include "test_utils/test_utils.h"


extern storage_params* GetStorageParam(std::string& strConfigFile);
extern std::map<std::string,std::string> ProgramOpts;
storage_params *get_param( void )
{
	storage_params* pRet = NULL;
	if (ProgramOpts.find("-conf") != ProgramOpts.end())
	{
		return GetStorageParam(ProgramOpts["-conf"]);
	}
	else if (ProgramOpts.find("-c") != ProgramOpts.end())
	{
		if ("archive" == ProgramOpts["-c"])
		{
			static storage_params para;
			para.doRecovery = false;
			para.XLogArchiveMode = true;	
			//para.XLogArchiveCommand = "python copy.py \"%p\" \"../archive/%f\"";
			para.XLogArchivePath = "../archive";
			pRet = &para;
		}
		else if ("recovery" == ProgramOpts["-c"])
		{
			static storage_params para;
			para.doRecovery = true;
			//para.recoveryRestoreCommand = "python copy.py  \"../archive/%f\" \"%p\"";
            para.recoveryRestorePath = "../archive";
			pRet = &para;
		}
	}
    return pRet;
}

//namespace Test
//{

using namespace FounderXDB::StorageEngineNS;

extern std::map<int,TestTask*> gMapDequene;
extern void TraverseDequene(TestTask** ptask,std::map<int,TestTask*>::iterator iter);
extern std::map<int,TestTask*>::iterator EarseDequene(std::map<int,TestTask*>::iterator iter);
extern std::map<int,TestTask*>::iterator GetBegin();
extern std::map<int,TestTask*>::iterator GetEnd();
extern void GetDequene(TestTask** ptask,int TranId);
extern bool IsEmpty();

TestFrame::TestFrame():m_ThreadNum(0),m_ThreadFlag(0)
{
	m_TestThreadPool = new TestThreadPool;
}

TestFrame::~TestFrame()
{
	if (m_TestThreadPool)
	{
		m_TestThreadPool->JoinAll();
		delete m_TestThreadPool;
	}
	m_PublicDeque.clear();
	
	std::map<int,BExcute*>::iterator iter;
	for (iter=m_ExecuteFlag.begin(); iter!=m_ExecuteFlag.end();++iter)
	{
		if (iter->second != NULL)
			delete iter->second;
	}
	m_ExecuteFlag.clear();
	
	std::map<int,TranSequence*>::iterator it;
	for (it=m_RecordWait.begin(); it!=m_RecordWait.end();++it)
	{
		if (it->second != NULL)
			delete it->second;
	}
	m_RecordWait.clear();
}

StorageEngine *pStorageEngine = NULL;
void TestFrame::StartEngine()
{
	extern std::string g_strDataDir;
	pStorageEngine = StorageEngine::getStorageEngine();
//	pStorageEngine->bootstrap("D:\\dataDll");
	pStorageEngine->initialize(const_cast<char*>(g_strDataDir.c_str()), 80, get_param()); //start engine
}

TestFrame& TestFrame::Instance()
{
	static TestFrame instance;
	return instance;
}

void TestFrame::StopEngine()
{
	pStorageEngine->shutdown();//stop engine
}

void TestFrame::SetThreadNum(int num)
{
	m_ThreadNum = num;
}

//处理需要等待其他事务的事务
//::计算不同事务在同一执行顺序等级的事务总数
//(同一事务中的任务不做计算 )
bool TestFrame::HandleRoleTrans(TaskInfo & info)	
{
	int TranId = info.GetTranId();
	std::set<int>::iterator iter;
	std::map<int,TranSequence*>::iterator Seqiter;
	if (info.m_WaitTranId != -1)
	{
		Seqiter = m_RecordWait.find(info.m_WaitTranId);
		if (Seqiter == m_RecordWait.end())
		{
			if (info.m_Role == 1)
			{
				TranSequence* pTranSeq = new TranSequence(1,1,TranId);
				m_RecordWait.insert(std::pair<int,TranSequence*>(info.m_WaitTranId,pTranSeq));
				return true;
			}
		}
		else 
		{
			if (info.m_Role > (Seqiter->second->m_Role) && 
			(1 == abs(info.m_Role-Seqiter->second->m_Role)))
			{
				if (Seqiter->second->m_Excute)
				{
					Seqiter->second->m_Excute= false;
					if (info.m_bFirst)
					{
						//更新新的事务等级,并将事务总数先设为1
						Seqiter->second->m_Role = info.m_Role;
						Seqiter->second->m_Index = 1;
						iter = Seqiter->second->m_SetCount.find(TranId);
						if (iter == Seqiter->second->m_SetCount.end())
						{
							Seqiter->second->m_SetCount.insert(TranId);
							Seqiter->second->m_TransID = TranId;
							return true;
						}
					}
				}
			}
			//等级差大于1的话，不加入公共队列
			else if(info.m_Role > (Seqiter->second->m_Role) && 
				(1 < abs(info.m_Role - Seqiter->second->m_Role)))
			{
				return false;
			}
			else if (info.m_Role == Seqiter->second->m_Role)
			{
				if (Seqiter->second->m_TransID != TranId)
				{
					iter = Seqiter->second->m_SetCount.find(TranId);
					if (iter == Seqiter->second->m_SetCount.end())
					{
						//新的不同事物，但执行等级相同
						Seqiter->second->m_Index++;
						Seqiter->second->m_SetCount.insert(TranId);
						Seqiter->second->m_TransID = TranId;
						return true;
					}
					else //事务已经被计算过，是另外的任务
						return true;
				}
				else //事务已经被计算过，是另外的任务
					return true;
			}
		}
	}
	else
		return true;
	return false;
}
//每次从事务队列中取出一个任务加入公共队列
//这个任务完成后再次取出一个加入公共队列
void TestFrame::AddPublicTask()
{
	std::map<int,TestTask*>::iterator iter;
	TestTask* pTask = NULL;
	TaskInfo mInfo(NULL,NULL,"",0,true,false,true,-1);
	iter = GetBegin();
	while(iter != GetEnd())
	{
		TraverseDequene(&pTask,iter);
		assert(pTask!=NULL);
		if(!pTask->IsEmpty() && !pTask->IsAbort())
		{
			pTask->OutPutTask(mInfo);
			std::map<int,BExcute*>::iterator it;
			boost::mutex::scoped_lock lk(m_DequeMutex);
			it = m_ExecuteFlag.find(mInfo.GetTranId());
			if (it != m_ExecuteFlag.end())
			{
				if (it->second->IsExute())
				{
					pTask->RemoveTask();
					if (pTask->OutPutTask(mInfo))
					{
						if (!HandleRoleTrans(mInfo))
						{
							iter++;
							continue;
						}
						m_PublicDeque.push_back(mInfo);
					}
					it->second->Set(false,false);
				}
			}
			else
			{
				if (!HandleRoleTrans(mInfo))
				{
					iter++;
					continue;
				}
				m_PublicDeque.push_back(mInfo);
			}
			iter++;
		}
		else
		{
			iter = EarseDequene(iter);
		}
	}
}

void TestFrame::SynTask(int TranId,TaskInfo& info)
{
	TestTask* pTask = NULL;
	GetDequene(&pTask,TranId);
	assert(pTask!=NULL);
	//pTask->OutPutTask(mInfo);
	pTask->SynParam(info);
}
void TestFrame::Init()
{
	AddPublicTask();
}

//处理公共队列中的任务
void TestFrame::ThreadFunction(int threadid)
{
	pStorageEngine->beginThread();
	TaskInfo FInfo(NULL,NULL,"",0,true,false,true,-1);
	bool bBreak = true;
	BExcute* pThExcute = new BExcute(false,false);
	std::map<int,BExcute*>::iterator pos;
	std::map<int,TranSequence*>::iterator iter;
	int TranId = -1;
	int LastTrId = -1;
	while(NULL != pThExcute)
	{
		{
			m_Threadlock.lock();
			//队列为空就退出s
			if (m_PublicDeque.empty())
			{
				if (IsEmpty())
				{
					if (bBreak)
					{
						m_ThreadFlag++;
						bBreak = false;
					}
					
					if (m_ThreadFlag>=m_ThreadNum)
					{
						m_Threadlock.unlock();
						break;
					}
					else
					{
						m_Threadlock.unlock();
						MySleep(500);
						continue;
					}
				}
				else
				{
					AddPublicTask();
					if (m_PublicDeque.empty())
					{
						m_Threadlock.unlock();
						MySleep(500);
						continue;
					}
				}
			}

			FInfo = m_PublicDeque.front();
			TranId = FInfo.GetTranId();
			
			//标识开始执行标志
			pos = m_ExecuteFlag.find(TranId);
			if (pos != m_ExecuteFlag.end())
				pos->second->Set(true,false);
			else
			{
				BExcute* pExcute = new BExcute(true,false);
				m_ExecuteFlag.insert(std::pair<int,BExcute*>(TranId,pExcute));
			}
			m_PublicDeque.pop_front();
			printf("\n----------%s running on the %dth thread ----------\n",FInfo.m_Name.c_str(),threadid);
			m_Threadlock.unlock();
		}

		//执行任务
		FInfo.Invoke();
		
		//切换事务的状态
		pStorageEngine->detachThread();

		SynTask(TranId,FInfo);
	
		{
			boost::mutex::scoped_lock lk(m_ExecuteMutex);
			//标识已执行标志
			pos = m_ExecuteFlag.find(TranId);
			if (pos != m_ExecuteFlag.end())
				pos->second->Set(true,true);
			
			//处理需要等待其他事务的事务,
			//在任务执行后的处理
			iter = m_RecordWait.find(FInfo.m_WaitTranId);
			if (iter != m_RecordWait.end())
			{
				if (FInfo.m_bLast && iter->second->m_Index > 0)
				{
					iter->second->m_SetCount.erase(TranId);
					--iter->second->m_Index;
				}
				
				if (iter->second->m_Index == 0)
					iter->second->m_Excute = true;
			}
		}
	}
	delete pThExcute;
	pThExcute = NULL;
	pStorageEngine->endThread();
	printf("\n-----------------the %dth thread end-----------------\n",threadid);
	return;
}

void TestFrame::Run()
{
	for (int i=0; i<m_ThreadNum; ++i)
	{
		m_TestThreadPool->AddThread(boost::bind(&TestFrame::ThreadFunction,this,i));
	}
}

void TestFrame::Join()
{
	m_TestThreadPool->JoinAll();
}

TestFrame::BExcute::BExcute(const TestFrame::BExcute &other)
{
	bStartE= other.bStartE;
	bEndE= other.bEndE;
}

/// overload operator =
TestFrame::BExcute& TestFrame::BExcute::operator = (const TestFrame::BExcute &other)
{
	bStartE= other.bStartE;
	bEndE= other.bEndE;
	return *this;
}

void TestFrame::BExcute::Set(bool bStart,bool bEnd)
{
	bStartE= bStart;
	bEndE= bEnd;
}

bool TestFrame::BExcute::IsExute()
{
	if (bStartE && bEndE)
		return true;
	return false;
}

TestFrame::TranSequence::TranSequence(const TestFrame::TranSequence &other)
{
	m_Role = other.m_Role;
	m_Excute = other.m_Excute;
	m_Index = other.m_Index;
	m_TransID = other.m_TransID;
	m_SetCount = other.m_SetCount;
}

/// overload operator =
TestFrame::TranSequence& TestFrame::TranSequence::operator = (const TestFrame::TranSequence &other)
{
	m_Role = other.m_Role;
	m_Excute = other.m_Excute;
	m_Index = other.m_Index;
	m_TransID = other.m_TransID;
	m_SetCount = other.m_SetCount;
	return *this;
}

int start_engine_()
{
	TestFrame::Instance().StartEngine();
	return 1;
}

int stop_engine_()
{
	TestFrame::Instance().StopEngine();
	return 1;
}
//}
