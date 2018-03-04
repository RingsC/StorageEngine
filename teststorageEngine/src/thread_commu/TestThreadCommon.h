/**
* @file TestThreadCommon.h
* @brief 
* @author 李书淦
* @date 2011-11-1 16:58:05
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TESTTHREADCOMMON_H
#define _TESTTHREADCOMMON_H
#include <vector>
#include <map>
#include <set>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <pthread.h>
#include "stdlib.h"

#define Random(x) (rand()%x)
#define Delay 1
#define Ndelay 0
/**************************************************************************
* @class RecverEx
* @brief 一个信号接收者类
* 
* Detailed description.
* 
**************************************************************************/
class RecverEx
{
public:
	RecverEx(bool randflag = FALSE):
	  m_pConfig(new std::set<int>())
		  ,m_tid(new pthread_t(0))
		  ,m_sRecvedSigs(new std::set<int>())
		  ,randflag(new bool(randflag))
	  {
	  }

	  /************************************************************************** 
	  * @brief Init 用来配置this可接收的信号
	  * 
	  * Detailed description.
	  * @param[in] pConfigs 可接收信号的集合
	  **************************************************************************/
	  RecverEx& operator+= (int nSig)
	  {
		  m_pConfig->insert(nSig);
		  return *this;
	  }

	  RecverEx& operator, (int nSig)
	  {
		  m_pConfig->insert(nSig);
		  return *this;
	  }

	  bool IsAllRecevied( void )
	  {
		  return *m_pConfig == *m_sRecvedSigs;
	  }
	  /**************************************************************************
	  * @class void Run( void );
	  * @brief 接收线程的入口函数
	  * 
	  * Detailed description.
	  **************************************************************************/
	  void operator()( void );


	  const std::set<int>& GetRecvedSigs()const {return *m_sRecvedSigs;}
	  pthread_t GetTID()const {return *m_tid;}
private:

	void SigHandler(int sig)
	{
		m_sRecvedSigs->insert(sig);
	}
	boost::shared_ptr<std::set<int> > m_pConfig;
	boost::shared_ptr<std::set<int> > m_sRecvedSigs;
	boost::shared_ptr<pthread_t> m_tid;
	bool randflag;

};


/**************************************************************************
* @class SenderEx
* @brief 可配置的信号发送者,可配置的意思是可以配置发送信号给谁，以及发送什么给谁
* 
* Detailed description.
**************************************************************************/
class SenderEx
{
public:
	SenderEx(bool randflag = FALSE):
	  m_pConfig(new std::multimap<RecverEx*,int>()),randflag(new bool(randflag))
	  {
	  }

	  /************************************************************************** 
	  * @brief Init 
	  * 
	  * Detailed description.
	  * @param[in] std::map<RecverEx* 
	  * @param[in] pConfigs 
	  **************************************************************************/
	  SenderEx& operator()(RecverEx* pRecv,int nSig)
	  {
		  m_pConfig->insert(std::make_pair(pRecv,nSig));
		  return *this;
	  }

	  void operator() ( void );
private:
	std::multimap<RecverEx*,int>*  m_pConfig;
	bool randflag;
};

class ThreadInfoSaver
{
public:
	ThreadInfoSaver();

	~ThreadInfoSaver();
};

#define SAVE_THREADINFO ThreadInfoSaver ___saver;
typedef void (*SigHandler)( int );
template<typename T,void (T::*fun) ( int )>
struct HandleMaker
{
private:
	T* m_pObj;
	static THREAD_LOCAL HandleMaker *g_pinstance;
	HandleMaker(T* obj ):m_pObj(obj){}
	void Call( int sig) {(m_pObj->*fun)(sig);}


public:
	static HandleMaker& getInstance(T* pObj = NULL)
	{
		if(NULL == g_pinstance)
		{
			g_pinstance = new HandleMaker(pObj);
		}

		return *g_pinstance;
	}
	static void Handle( int sig )
	{
		getInstance().Call(sig);
	}


};
template<typename T,void (T::*fun) ( int )>
THREAD_LOCAL HandleMaker<T,fun> *HandleMaker<T,fun>::g_pinstance = NULL;

template<typename T,void (T::*fun)(int)>
SigHandler MakeSigHandle(T* obj)
{
	HandleMaker<T,fun>::getInstance(obj);
	return HandleMaker<T,fun>::Handle;
}

#endif 
