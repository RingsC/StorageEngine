/**
* @file deadlocktest.cpp
* @brief 
* @author 李书淦
* @date 2012-4-25 9:54:42
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <boost/assign.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <fstream>
#include "utils/util.h"
#include "access/xact.h"
#include "test_fram.h"
#include "interface/FDPGAdapter.h"
#include "interface/StorageEngineExceptionUniversal.h"
#include "deadlock/deadlocktest.h"

using namespace FounderXDB::StorageEngineNS;


Oid relid1 = 0;
Oid relid2 = 0;

pthread_t tid1 = 0;

bool wakeUp1 = false;
bool canWakeup1 = false;
bool hardDeadLockDected = false;
void wakeupThread1( int )
{
    wakeUp1 = true;
}
void Thread1( void )
{
	BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);
	free(param);
	try
	{
		tid1 = pthread_self();
		pg_signal(SIGINT,wakeupThread1);
     	begin_transaction();

		FDPG_Heap::fd_heap_open(relid1,AccessExclusiveLock);

		canWakeup1 = true;
		while (!wakeUp1)
		{
			pg_sleep(100L);
		}

		FDPG_Heap::fd_heap_open(relid2,AccessExclusiveLock);
		commit_transaction();
	}
	catch(DeadLockException &ex)
	{
		hardDeadLockDected = true;
	    std::cout << ex.getErrorNo() << std::endl;
	    std::cout << ex.getErrorMsg() << std::endl;
	    user_abort_transaction();
	}
}

void Thread2( void )
{
	BackendParameters *param = (BackendParameters *) malloc(sizeof(BackendParameters));
	param->MyThreadType = backend;
	fxdb_SubPostmaster_Main(param);
	free(param);
	try
	{
		begin_transaction();

		FDPG_Heap::fd_heap_open(relid2,AccessExclusiveLock);

		while (!canWakeup1)
		{
			pg_sleep(100L);
		}
		pgkill(tid1,SIGINT);

		FDPG_Heap::fd_heap_open(relid1,AccessExclusiveLock);
		commit_transaction();
	}
	catch(DeadLockException &ex)
	{
		hardDeadLockDected = true;
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		user_abort_transaction();
	}
}

bool HardLockTest( void )
{
	INTENT("1. 创建两个表:1和2;\n"
		   "2. 启动线程1： 打开A->sleep->打开B;\n"
		   "3. 启动线程2:  打开B->wakup 线程1->打开A;\n");

	//创建表1,2
	relid1 = OIDGenerator::instance().GetHeapID();
	relid2 = OIDGenerator::instance().GetHeapID();

	static ColinfoData pColInfo;
	pColInfo.col_number = 0;
	pColInfo.col_number = NULL;
	pColInfo.rd_comfunction = NULL;
	pColInfo.split_function =  FixSpliter::split;
	setColInfo(relid1,&pColInfo);
	setColInfo(relid2,&pColInfo);
	try
	{
		begin_transaction();
        FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID,relid1,11967,relid1);
		FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID,relid2,11967,relid2);
		commit_transaction();

        
		boost::thread_group thgrp;
		thgrp.create_thread(Thread1);
		thgrp.create_thread(Thread2);

		thgrp.join_all();

		CHECK_BOOL(hardDeadLockDected);

		begin_transaction();
		FDPG_Heap::fd_heap_drop(relid1);
		FDPG_Heap::fd_heap_drop(relid2);
		commit_transaction();

		return hardDeadLockDected;
	}
	CATCHEXCEPTION
    
	return false;
}


/////////////////=======================Soft lock test =========================
Oid relidX = 0;
Oid relidY = 0;

void ThreadA( void )
{

}

void ThreadB( void )
{

}

void ThreadC( void )
{

}
bool SoftLockTest( void )
{return true;
	INTENT("1. 创建两个表:X和Y;\n"
		   "2. 启动线程A： 打开X->sleep->wakeup C;\n"
		   "3. 启动线程B:  打开Y->wakup 线程1->打开A;\n"
		   "4. 启动线程C:  ");
	//创建表1,2
	relidX = OIDGenerator::instance().GetHeapID();
	relidY = OIDGenerator::instance().GetHeapID();

	static ColinfoData pColInfo;
	pColInfo.col_number = 0;
	pColInfo.col_number = NULL;
	pColInfo.rd_comfunction = NULL;
	pColInfo.split_function =  FixSpliter::split;
	setColInfo(relidX,&pColInfo);
	setColInfo(relidY,&pColInfo);

	try
	{
		begin_transaction();
		FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID,relidX,11967,relidX);
		FDPG_Heap::fd_heap_create(DEFAULTTABLESPACE_OID,relidY,11967,relidY);
		commit_transaction();


		boost::thread_group thgrp;
		thgrp.create_thread(ThreadA);
		thgrp.create_thread(ThreadB);
		thgrp.create_thread(ThreadC);

		thgrp.join_all();

		CHECK_BOOL(hardDeadLockDected);

		begin_transaction();
		FDPG_Heap::fd_heap_drop(relidX);
		FDPG_Heap::fd_heap_drop(relidY);
		commit_transaction();

		return hardDeadLockDected;
	}
	CATCHEXCEPTION
    return true;
}