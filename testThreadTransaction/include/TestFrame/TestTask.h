#ifndef TEST_TASK_H
#define TEST_TASK_H
#include <deque>
#include <string>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/shared_ptr.hpp>
#include "PGSETypes.h"
#include "TestFrameCommon/TestFrameCommon.h"

//namespace Test
//{


typedef bool (*F) (ParamBase* arg);
struct UNUSED
{};

struct TaskInfo
{
public:
	TaskInfo(F Fun,ParamBase* arg,std::string name,int tranid,bool bfirst,bool blast,
				int Role,int Waitid):m_Fun(Fun),m_Name(name),m_Result(false),m_TransId(tranid),
				m_Param(arg),m_bFirst(bfirst),m_bLast(blast),m_Role(Role),m_WaitTranId(Waitid)
	{
	}
	TaskInfo(const TaskInfo &other);
    /// overload operator =
    TaskInfo& operator = (const TaskInfo &other);
	//输出事务ID
	inline int GetTranId()const {return m_TransId;}
	inline ParamBase*& GetParam() {return m_Param;}
	inline void SetParam(ParamBase*& para){m_Param = para;}
	//是否要中断以后的任务
	bool AbortFlag();
	//运行函数
	void Invoke();
public:
	//是否最后一个任务
	bool m_bLast;
	//是否是第一个任务
	bool m_bFirst;
	//角色等级标识，最高等级时为1，小一等级m_Role值加1
	int m_Role;
	//等待标识ID,-1时无等待角色
	int m_WaitTranId;
	//函数名字
	std::string m_Name;
private:
	//执行结果
	bool m_Result;
	//函数参数
	ParamBase* m_Param;
	//记录事务队列ID
	int m_TransId;
	//函数
	F m_Fun;
};
class TestTask
{
public:
	TestTask(int TranId,std::string name,F Fun,ParamBase* arg,bool bfirst,
				bool blast,int Role,int Waitid):m_TransId(TranId),
				m_babort(false)
	{
		
		m_taskF.push_back(TaskInfo(Fun,arg,name,TranId,bfirst,blast,Role,Waitid));
	}
	TestTask(const TestTask &other);
    /// overload operator =
    TestTask& operator = (const TestTask &other);
	virtual ~TestTask();
	void AddTask(std::string name,int TranId,F Fun,ParamBase* arg,bool bfirst,bool blast,int Role,int Waitid);
	bool RemoveTask();
	bool OutPutTask(TaskInfo& info); 
	void SynParam(TaskInfo& info);
	bool IsEmpty();
	bool IsAbort();
private:
	//是否中断任务队列
	bool m_babort;
	//事务队列 ID
	int m_TransId;
	//task deque
	std::deque<TaskInfo>	m_taskF;
};

void AddDequene(int TranId, std::string name,F Fun,ParamBase* arg,int Role=1,int Waitid=-1);
void TraverseDequene(TestTask** ptask);
int IncreaseTransId();

//}
#endif // TEST_TASK_H