#ifndef _ICQ_TYPES_H_
#define _ICQ_TYPES_H_
//#include <sys/types.h>
#include <iostream>
//#include <iomanip>
#include <errno.h>
#include <db.h>
//#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <fstream>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/timer.hpp>
#include "bdb_utils.h"
#include <db_cxx.h>

using std::cout;
using std::cin;
using std::endl;


void init_DBT(DBT * key, DBT * data)
{
	memset(key, 0, sizeof(DBT));
	memset(data, 0, sizeof(DBT));
}

void print_error(int ret)
{
	if(ret != 0)
		printf("ERROR: %s\n",db_strerror(ret));
}

bool readfromfile = true;
void RandomGenString(std::string& strLine,size_t nLen)
{
	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::ifstream inFile(szFileName);
		if (inFile.is_open())
		{
			inFile.seekg(0,std::ios::end);
			size_t length = inFile.tellg();
			inFile.seekg(0,std::ios::beg);
			boost::shared_ptr<char> psz(new char[length]);
			inFile.read(psz.get(),length);
			strLine.append(psz.get(),length);
			return;
		}
	}

	std::string s;
	srand((unsigned)time( NULL ));
	size_t nGenerated = 0;
	for (nGenerated = 0;nGenerated < 10;++nGenerated)
	{
		char c = 0;
		s += c;
	}
	for (nGenerated = 10; nGenerated < nLen - 1; ++nGenerated)
	{
		char c = (rand()) % 26;
		s += 'a' + c;
	}
	char c = 0;
	s += c;
	strLine.append(s);

	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::fstream outFile(szFileName,std::ios_base::out);
		if(outFile.is_open())
		{
			outFile<<s;
		}
	}

}

bool BDB_thread_heap_create(int *status)
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 设置DB的缓存为5M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		db->close(db, 0);
		dbenv->close(dbenv, 0);
		* status = 1;
		}
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		std::cout << e.what() << endl;
		* status = 0;
	} 

	return   0 ;
}

bool BDB_thread_heap_insert(int count , int data_size , int *status)
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为5M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		init_DBT(&key, &data);
		for (int i=0;i<count;i++)
		{
			std::string temp_data;
			RandomGenString(temp_data,1<<data_size);
			data.data = (void*)temp_data.c_str();
			data.size = temp_data.length();

			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, NULL, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
			print_error(ret);
		}

		db->close(db, 0);
		dbenv->close(dbenv, 0);
		
		*status = 1;
	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
		*status = 0;
	} 

	return   0 ;
}

bool BDB_thread_heap_delete(int thread_id , int count ,int *status)
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		boost::mutex mut____;
		{
		boost::unique_lock<boost::mutex> lock(mut____);
		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为5M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);

		
		init_DBT(&key, &data);

			for (int i=(thread_id*count) ; i<((thread_id+1)*count) ; i++)
			{
				key.data = &i;
				key.size = sizeof(int);
				ret = db->del(db, NULL, &key, 0);
				print_error(ret);
			}

			print_error(ret);

			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}

		*status = 1;
	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
		*status = 0;
	} 

	return   0 ;
}


bool BDB_thread_heap_remove(int *status)
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DB_TXN *txn1 = NULL;

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为5M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		//ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		//print_error(ret);
		//ret=dbenv->set_data_dir(dbenv, "./dbtestENV");
		print_error(ret);
		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = dbenv->txn_begin(dbenv, NULL, &txn1, 0);
		//ret = db_create(&db, dbenv, 0);
		//print_error(ret);
		//ret = db->open(db, NULL, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		//print_error(ret);
		//db->close(db, 0);
		//db->remove(db , "./dbtestENV" , "MyTest.db" , 0);
		db->close(db, 0);
		ret=dbenv->dbremove(dbenv,txn1, "../dbtestENV" , "MyTest.db" , 0);
		print_error(ret);
		ret = txn1->commit(txn1, 0);
		print_error(ret);
		
		dbenv->close(dbenv, 0);

		*status = 1;
	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
		*status = 0;
	} 

	return   0 ;

}

bool check_status(int status)
{
	if (status == 0)
	{
		cout<<"operation error!"<<endl;
	}
	return true;
}
#endif