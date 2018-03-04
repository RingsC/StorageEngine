#ifndef TEST_FRAME_H
#define TEST_FRAME_H
#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include "TestFrame/TestTask.h"
#include "TestFrame/TestThreadPool.h"
#include <set>

//开始注册测试用例
#define BEGIN_REGCASE() 	BOOST_AUTO_TEST_SUITE(TESTTRANTHREAD)

//注册测试用例完毕
#define END_REGCASE() 		BOOST_AUTO_TEST_SUITE_END()


//注册测试用例
#define REGCASE(FunCase)		\
		BOOST_FIXTURE_TEST_CASE(FunCase##_,UNUSED)\
		{\
			std::cout << "----------------TestCase:   " << #FunCase << " running ----------------\n";\
			boost::execution_monitor ex; \
			try \
			{\
				int FunCase##_RE = ex.execute(boost::unit_test::callback0<bool>(FunCase));\
				BOOST_CHECK(FunCase##_RE);\
				if (FunCase##_RE)\
					std::cout << "\n------------------- test success! -------------------\n\n";\
				else \
					std::cout << "\n------------------- test fail! -------------------\n\n";\
			}\
			catch(boost::execution_exception &e)\
			{\
				std::cout << e.what().begin(); \
				BOOST_CHECK(false);\
			}\
		}


//在测试用例中首先调用 ,进行事务ID的初始化
#define INITRANID()		int TestTranId=IncreaseTransId();
		 
//在测试用例中调用注册任务
//在注册任务，设定事务顺序；ROLE设定顺序号，值为1时最先执行,
//值越大执行顺序越晚，默认值为1；WAITID值是相关事务之间相互通
//信的唯一标识符号，不同序列的事务不能重复,默认值为-1;
#define REGTASK(Fun,ARG)	do{AddDequene(TestTranId,#Fun,Fun,ARG);}while(0)
#define REGTASKSEQ(Fun,ARG,ROLE,WAITID)	do{AddDequene(TestTranId,#Fun,Fun,ARG,ROLE,WAITID);}while(0)


//namespace Test
//{

class TestThreadPool;
class TestFrame
{
public:
	virtual ~TestFrame();
	void StartEngine();
	void Init();
	void StopEngine();
	void SetThreadNum(int num);
	void Run();
	void ThreadFunction(int threadid);
	void Join();
	bool HandleRoleTrans(TaskInfo& info);
	static TestFrame& Instance();
private:
	TestFrame();

	void AddPublicTask();
	//同步参数的变化
	void SynTask(int TranId,TaskInfo& info);
private:
	
	struct BExcute
	{
		BExcute(bool bStart,bool bEnd):bStartE(bStart),bEndE(bEnd){}
		BExcute(const BExcute &other);
		BExcute& operator = (const BExcute &other);
		void Set(bool bStart,bool bEnd);
		bool IsExute();
		bool bStartE;
		bool bEndE;
	};
	struct TranSequence
	{
		TranSequence(int Role,int Index,int TransID):m_Role(Role),m_Index(Index),
							m_TransID(TransID),m_Excute(false)
		{
			m_SetCount.insert(m_TransID);
		}
		TranSequence(const TranSequence &other);
		TranSequence& operator = (const TranSequence &other);
		//当前事务已执行结束
		bool m_Excute;
		//当前等级
		int m_Role;
		//不同事务在同一等级序号总数
		int m_Index;
		//当前事务ID
		int m_TransID;
		//若队列中有值表示当前事务已计算
		std::set<int> m_SetCount;
	};
	//线程执行时的公共队列
	std::deque<TaskInfo> m_PublicDeque;
	//公共队列中的任务执行标志
	std::map<int,BExcute*>	m_ExecuteFlag;
	//根据WaitId 记录需等待其他事务执行的事务的信息
	std::map<int,TranSequence*> m_RecordWait;
	
	boost::detail::spinlock m_Threadlock;
	boost::mutex m_ExecuteMutex;
	boost::mutex m_DequeMutex;
	//线程池
	TestThreadPool*  m_TestThreadPool;
	//线程数目
	int m_ThreadNum;
	//线程退出标志
	int m_ThreadFlag;
};
//}
#endif // !TEST_FRAME_H