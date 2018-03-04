#include "TestFrame/TestThreadPool.h"

//namespace Test
//{
TestThreadPool::~TestThreadPool()
{
	delete m_ThreadPool;
}
void TestThreadPool::JoinAll()
{
	m_ThreadPool->join_all();
}
int TestThreadPool::GetThreadNum()const
{
	return m_ThreadNum;
}

void TestThreadPool::RemoveThread(boost::thread* thd)
{
	boost::mutex::scoped_lock lock(m_ThreadMutex);
	m_ThreadNum--;

	m_ThreadPool->remove_thread(thd);
}
//}
