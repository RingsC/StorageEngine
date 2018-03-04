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
	//�������ID
	inline int GetTranId()const {return m_TransId;}
	inline ParamBase*& GetParam() {return m_Param;}
	inline void SetParam(ParamBase*& para){m_Param = para;}
	//�Ƿ�Ҫ�ж��Ժ������
	bool AbortFlag();
	//���к���
	void Invoke();
public:
	//�Ƿ����һ������
	bool m_bLast;
	//�Ƿ��ǵ�һ������
	bool m_bFirst;
	//��ɫ�ȼ���ʶ����ߵȼ�ʱΪ1��Сһ�ȼ�m_Roleֵ��1
	int m_Role;
	//�ȴ���ʶID,-1ʱ�޵ȴ���ɫ
	int m_WaitTranId;
	//��������
	std::string m_Name;
private:
	//ִ�н��
	bool m_Result;
	//��������
	ParamBase* m_Param;
	//��¼�������ID
	int m_TransId;
	//����
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
	//�Ƿ��ж��������
	bool m_babort;
	//������� ID
	int m_TransId;
	//task deque
	std::deque<TaskInfo>	m_taskF;
};

void AddDequene(int TranId, std::string name,F Fun,ParamBase* arg,int Role=1,int Waitid=-1);
void TraverseDequene(TestTask** ptask);
int IncreaseTransId();

//}
#endif // TEST_TASK_H