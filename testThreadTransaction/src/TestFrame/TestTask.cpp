#include <boost/test/test_tools.hpp>
#include <iostream>
#include <stdexcept> 
#include "TestFrame/TestTask.h"
#include "StorageEngine.h"

//namespace Test
//{

using namespace FounderXDB::StorageEngineNS;

boost::mutex gMapMutex;
std::map<int,TestTask*> gMapDequene;
int gTransId = 0;
int IncreaseTransId()
{
	boost::mutex::scoped_lock lk(gMapMutex);
	gTransId++;
	return gTransId;
}

void AddDequene(int TranId, std::string name,F Fun,ParamBase* arg,int Role,int Waitid)
{
	boost::mutex::scoped_lock lk(gMapMutex);
	std::map<int,TestTask*>::iterator iter;
	iter = gMapDequene.find(TranId);
	TestTask* pTask = NULL;
	if (iter != gMapDequene.end())
	{
		pTask = iter->second;
		pTask->AddTask(name,TranId,Fun,arg,false,true,Role,Waitid);
	}
	else
	{
		pTask = new TestTask(TranId,name,Fun,arg,true,true,Role,Waitid);
		gMapDequene.insert(std::pair<int,TestTask*>(TranId,pTask));
		
	}
}

std::map<int,TestTask*>::iterator EarseDequene(std::map<int,TestTask*>::iterator iter)
{
	TestTask* pTask = NULL;
	boost::mutex::scoped_lock lk(gMapMutex);
	pTask = iter->second;
	delete pTask;
	pTask = NULL;
	gMapDequene.erase(iter++);
	return iter;
}
void TraverseDequene(TestTask** ptask,std::map<int,TestTask*>::iterator iter)
{
	boost::mutex::scoped_lock lk(gMapMutex);
	if (gMapDequene.empty())
	 	return;
	*ptask = iter->second;
}

std::map<int,TestTask*>::iterator GetBegin()
{
	boost::mutex::scoped_lock lk(gMapMutex);
	return gMapDequene.begin();
}

std::map<int,TestTask*>::iterator GetEnd()
{
	boost::mutex::scoped_lock lk(gMapMutex);
	return gMapDequene.end();
}

void GetDequene(TestTask** ptask,int TranId)
{
	boost::mutex::scoped_lock lk(gMapMutex);
	if (gMapDequene.empty())
	 	return;
	std::map<int,TestTask*>::iterator iter;
	iter = gMapDequene.find(TranId);
	*ptask = iter->second;
}

bool IsEmpty()
{
	boost::mutex::scoped_lock lk(gMapMutex);
	return gMapDequene.empty();
}

TaskInfo::TaskInfo(const TaskInfo &other)
{
	m_TransId = other.m_TransId;
	m_Param = other.m_Param;
	m_Fun = other.m_Fun;
	m_Name = other.m_Name;
	m_bFirst = other.m_bFirst;
	m_bLast = other.m_bLast;
	m_Role = other.m_Role;
	m_WaitTranId = other.m_WaitTranId;
}

/// overload operator =
TaskInfo& TaskInfo::operator = (const TaskInfo &other)
{
	m_TransId = other.m_TransId;
	m_Param = other.m_Param;
	m_Fun = other.m_Fun;
	m_Name = other.m_Name;
	m_bFirst = other.m_bFirst;
	m_bLast = other.m_bLast;
	m_Role = other.m_Role;
	m_WaitTranId = other.m_WaitTranId;
	return *this;
}

extern StorageEngine *pStorageEngine;

void TaskInfo::Invoke()
{
	if (NULL == m_Param)
	{
		std::cerr<<"\n parameter is null and Task fail\n";
		std::abort();
		return;
	}
	
	if (m_bFirst)
		m_Param->begin();
	else
		m_Param->SetTranState();
	
	m_Result = m_Fun(m_Param);
	pStorageEngine->endStatement(m_Param->m_Trans);

	if (!m_bLast && !m_Param->m_FlagCommit)
	{
		m_Param->abort();
		delete m_Param;
		m_Param = NULL;
	}

	if (m_bLast)
	{
		if (m_Param->m_FlagCommit)
			m_Param->commit();
		else
			m_Param->abort();
		delete m_Param;
		m_Param = NULL;
	}
	
	if (m_Result)
	{
		printf("\n--------%s  run success!--------\n",m_Name.c_str());
	}
	else
	{
		printf("\n----error occurs in %s----",m_Name.c_str());
		std::abort();
	}
	
}

bool TaskInfo::AbortFlag()
{
	if (NULL == m_Param)
		return true;
	return false;
}

TestTask::TestTask(const TestTask &other)
{
	m_TransId = other.m_TransId;
	m_taskF = other.m_taskF;
}

TestTask::~TestTask()
{
	m_taskF.clear();
}

/// overload operator =
TestTask& TestTask::operator = (const TestTask &other)
{
	m_TransId = other.m_TransId;
	m_taskF = other.m_taskF;
	return *this;
}

void TestTask::AddTask(std::string name,int TranId,F Fun,ParamBase* arg,bool bfirst,bool blast,
							int Role,int Waitid)
{
	m_taskF.back().m_bLast = false;
	m_taskF.push_back(TaskInfo(Fun,arg,name,TranId,bfirst,blast,Role,Waitid));
}

bool TestTask::RemoveTask()
{
	if (m_taskF.empty())
		return false;
	m_taskF.pop_front();
	return true;
}

bool TestTask::OutPutTask(TaskInfo& info)
{
	if (m_taskF.empty()|| m_babort)
		return false;
	info = m_taskF.front();
	if (info.AbortFlag())
		m_babort = true;
	return true;
}

void TestTask::SynParam(TaskInfo& info)
{
	if (!m_taskF.empty())
	{
		m_taskF.front().SetParam(info.GetParam());
		if (m_taskF.size()>=2)
			m_taskF.at(1).SetParam(info.GetParam());
	}
}

bool TestTask::IsEmpty()
{
	return m_taskF.empty();
}
bool TestTask::IsAbort()
{
	return m_babort;
}

//}
