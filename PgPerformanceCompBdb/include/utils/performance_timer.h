#ifndef PERFORMANCE_TIMER_HPP
#define PERFORMANCE_TIMER_HPP

#include <iostream>
#include <fstream>
#include <exception>
#include <string>
#include <ctime>
#include <boost/timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "utils/pgbdbutils.h"


using std::cout;
using std::endl;
using std::ofstream;
using std::string;
using std::exception;
using namespace boost::posix_time;

void writeResToFile(const std::string &op, double timeOut, uint32 len, uint32 thread_nums, uint32 count);


/**************************************************************************
* struct PerformanceTimer
* º∆À„ ±º‰
* Detailed description.
**************************************************************************/

struct PerformanceTimer
{
	PerformanceTimer()
	{
#ifdef WIN32
		out.open("D:\\XMLDB\\res.txt", std::ios::out | std::ios::app);
#else
		try {
		out.open("/home/fd/res.txt", std::ios::out | std::ios::app);
		} catch(exception &e)
		{
			cout << e.what() << endl;
		}
#endif //WIN32
	
		if(!out.is_open())
		{
			cout << "open file failed" << endl;
		}
		startTime = microsec_clock::local_time(); 
	}

	
	double getElapse()
	{
		endTime = microsec_clock::local_time();
		time_duration dur = endTime - startTime;
		return dur.total_milliseconds(); 
	}

	PerformanceTimer(string op, uint32 lines, uint32 tupleLen, uint32 thread_nums = 1, bool isWrite = true)
	{
		//out.open("D:\\XMLDB\\res.txt", std::ios::out | std::ios::app);
#ifdef WIN32
		out.open("D:\\XMLDB\\res.txt", std::ios::out | std::ios::app);
#else
		try {
		out.open("/home/fd/res.txt", std::ios::out | std::ios::app);
		} catch(exception &e)
		{
			cout << e.what() << endl;
		}
#endif //WIN32

		if(!out.is_open())
		{
			cout << "open file failed" << endl;
		}
		m_lines = lines;
		m_thread_nums = thread_nums;
		m_tuple_len = tupleLen;
		m_op = op;
		m_isWrite = isWrite;
	

		startTime = microsec_clock::local_time(); 
	}

	~PerformanceTimer()
	{
		if(m_isWrite)
		{
			out <<m_op<<":thread_nums:" << m_thread_nums<< "  ;lines:" << m_lines << "  ,cost:" << getElapse()  <<"  ,tupleLen:" <<m_tuple_len<< std::endl;
		}
		out.close();
	}
private:
	ofstream out;
	uint32 m_lines;
	uint32 m_thread_nums;
	uint32 m_tuple_len;
	string m_op;
	bool m_isWrite;


	ptime startTime;
	ptime endTime;
};



#endif //PERFORMANCE_TIMER_HPP
