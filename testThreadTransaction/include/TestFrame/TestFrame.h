#ifndef TEST_FRAME_H
#define TEST_FRAME_H
#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>
#include "TestFrame/TestTask.h"
#include "TestFrame/TestThreadPool.h"
#include <set>

//��ʼע���������
#define BEGIN_REGCASE() 	BOOST_AUTO_TEST_SUITE(TESTTRANTHREAD)

//ע������������
#define END_REGCASE() 		BOOST_AUTO_TEST_SUITE_END()


//ע���������
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


//�ڲ������������ȵ��� ,��������ID�ĳ�ʼ��
#define INITRANID()		int TestTranId=IncreaseTransId();
		 
//�ڲ��������е���ע������
//��ע�������趨����˳��ROLE�趨˳��ţ�ֵΪ1ʱ����ִ��,
//ֵԽ��ִ��˳��Խ��Ĭ��ֵΪ1��WAITIDֵ���������֮���໥ͨ
//�ŵ�Ψһ��ʶ���ţ���ͬ���е��������ظ�,Ĭ��ֵΪ-1;
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
	//ͬ�������ı仯
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
		//��ǰ������ִ�н���
		bool m_Excute;
		//��ǰ�ȼ�
		int m_Role;
		//��ͬ������ͬһ�ȼ��������
		int m_Index;
		//��ǰ����ID
		int m_TransID;
		//����������ֵ��ʾ��ǰ�����Ѽ���
		std::set<int> m_SetCount;
	};
	//�߳�ִ��ʱ�Ĺ�������
	std::deque<TaskInfo> m_PublicDeque;
	//���������е�����ִ�б�־
	std::map<int,BExcute*>	m_ExecuteFlag;
	//����WaitId ��¼��ȴ���������ִ�е��������Ϣ
	std::map<int,TranSequence*> m_RecordWait;
	
	boost::detail::spinlock m_Threadlock;
	boost::mutex m_ExecuteMutex;
	boost::mutex m_DequeMutex;
	//�̳߳�
	TestThreadPool*  m_TestThreadPool;
	//�߳���Ŀ
	int m_ThreadNum;
	//�߳��˳���־
	int m_ThreadFlag;
};
//}
#endif // !TEST_FRAME_H