#ifndef TEST_THREAD_POOL_H
#define TEST_THREAD_POOL_H
#include <deque>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include "PGSETypes.h"

//namespace Test
//{

class TestThreadPool
{
public:
	TestThreadPool():m_ThreadNum(0){m_ThreadPool = new boost::thread_group();}
	virtual ~TestThreadPool();
	void JoinAll();
	//得到线程数目
	int GetThreadNum() const;
    //增加一个工作线程
    template <typename TaskFunc>
	inline boost::thread* AddThread(TaskFunc func)
    {
		boost::mutex::scoped_lock lock(m_ThreadMutex);
        m_ThreadNum++;
        boost::thread* thd = m_ThreadPool->create_thread(func);
		return thd;
    }
    
    //移除一个工作线程
    void RemoveThread(boost::thread* thd);
private:
	int m_ThreadNum;
	boost::mutex m_ThreadMutex;
	boost::thread_group* m_ThreadPool;
};
//}
#endif // TEST_THREAD_POOL_H