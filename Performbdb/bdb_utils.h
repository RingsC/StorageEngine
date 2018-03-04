#include <boost/timer.hpp>
//#include <sys/types.h>
#include <iostream>
//#include <iomanip>
#include <errno.h>
#include <db.h>
//#include <pthread.h>
#include <stdlib.h>
#include <string.h>

void init_DBT(DBT * key, DBT * data);
void print_error(int ret);
bool check_status(int status);

bool BDB_thread_heap_create(int *status);
bool BDB_thread_heap_remove(int *status);
bool BDB_thread_heap_delete(int thread_id , int count ,int *status);
bool BDB_thread_heap_insert(int count , int data_size , int *status);

struct TimerFixture
{
	TimerFixture()
	{
#ifndef WIN32
		gettimeofday(&startTime,NULL);
#endif
	}

	~TimerFixture()
	{
#ifdef WIN32
		std::cout<<" cost "<<m_time.elapsed()<<std::endl;
#else
		const int RATE = 1000000L;
		gettimeofday(&endTime,NULL);
		double microsec = (endTime.tv_sec - startTime.tv_sec) * RATE + (endTime.tv_usec - startTime.tv_usec);
		std::cout<<" cost "<<(microsec / RATE)<<" seconds"<<std::endl;
#endif
	}
private:
#ifdef WIN32
	boost::timer m_time;
#else
	struct timeval startTime;
	struct timeval endTime;
#endif
};

/************************************************************************** 
* @brief RandomGenString 
*  ������������������ָ�����ȵĴ�����
* 
* @param[in/out] ���Է������ݣ����ɵ����ݻ���׷�ӵķ�ʽ����s����
* @param[in] nLen Ҫ���ɵ����ݳ���
**************************************************************************/
void RandomGenString(std::string& s,size_t nLen);