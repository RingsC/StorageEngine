#ifndef _ICQ_TYPES_H_
#define _ICQ_TYPES_H_
//#include <sys/types.h>
#include <iostream>
//#include <iomanip>
#include <errno.h>
#include <db.h>
#include <vector>
//#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "bdb_utils.h"
#include "testBDB-heapscan.h"


#ifdef _WIN32
extern "C" {
	extern int getopt(int, char * const *, const char *);
	extern int optind;
}
#else
#include <unistd.h>
#endif

#include <db_cxx.h>

#define random(x) (rand()%x)
#define FINDTIMES 100
#define MAX_RAM_STR_SIZE 2000

using std::cin;
using std::cout;
using std::cerr;
using std::string;
using std::vector;
using std::stringstream;

using namespace std;

bool test_performbdb_heap_scan()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		DB_TXN *txn1 = NULL;
		DBC *cur1;

		char name[10] = {'\0'};
		memcpy(name,"test",sizeof("test"));
		char name_new[10] = {'\0'};
		memcpy(name_new,"test_new",sizeof("test_new"));

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		ret = dbenv->set_lk_max_objects(dbenv, 20000);
		ret = dbenv->set_lk_max_locks(dbenv, 20000);
		//ret = dbenv->set_lk_max_lockers(dbenv, 2000000);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = dbenv->txn_begin(dbenv, NULL, &txn1, 0);
		print_error(ret);
		ret = db->open(db, txn1, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);
		ret = db->cursor(db, txn1, &cur1, 0);
		print_error(ret);


		int count;

		cout<<"BDB查找(数据/索引)...行数:";
		cin>>count;
		init_DBT(&key, &data);
		data.data = name;
		data.size = sizeof("test");
		for (int i=0;i<count;i++)
		{
			//Dbt key(&i,sizeof(i));
			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, txn1, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
			//print_error(ret);
		}

		if(ret == 0)
			printf("INSERT SUCCESS!\n");
		else
			printf("INSERT FAILED!\n");

		{
			TimerFixture e;

			init_DBT(&key, &data);
			data.data = &name;
			data.size = sizeof("test");
			for (int i = 0;i<count;i++)
			{
				key.data = &i;
				key.size = sizeof(int);
				while((ret = cur1->c_get(cur1, &key, &data, DB_NEXT)) == 0)
				{
					break;
				}
			}
			if(ret == 0)
				printf("SCAN SUCCESS!\n");
			else
				printf("SCAN FAILED!\n");
			ret = cur1->c_close(cur1);
			print_error(ret);
			ret = txn1->commit(txn1, 0);
			print_error(ret);

			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}

	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}


bool test_performbdb_heap_scan_special()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		DB_TXN *txn1 = NULL;
		DBC *cur1;

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		ret = dbenv->set_lk_max_objects(dbenv, 20000);
		ret = dbenv->set_lk_max_locks(dbenv, 20000);
		//ret = dbenv->set_lk_max_lockers(dbenv, 2000000);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = dbenv->txn_begin(dbenv, NULL, &txn1, 0);
		print_error(ret);
		ret = db->open(db, txn1, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);
		ret = db->cursor(db, txn1, &cur1, 0);
		print_error(ret);


		int count;

		cout<<"BDB查找(数据/索引)...行数:";
		cin>>count;
		init_DBT(&key, &data);

		std::string testdata ("test");
		for (int i=0;i<count;i++)
		{
			std::stringstream pstr;
			pstr << testdata << i;
			data.data = (void *)pstr.str().c_str();//pstr中的字符及参数传给data
			data.size = pstr.str().length();

			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, txn1, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
			//print_error(ret);
		}

		if(ret == 0)
			printf("INSERT SUCCESS!\n");
		else
			printf("INSERT FAILED!\n");

		{
			TimerFixture e;

			init_DBT(&key, &data);
			//data.data = &name;

			for (int i = 0; i< FINDTIMES ; i++)
			{
				srand((int)time(0));
				std::stringstream pstr_find;
				int find_key = random(count);
				pstr_find << testdata << find_key;
				data.data = (void *)pstr_find.str().c_str();//pstr_find中的字符及参数传给data
				data.size = pstr_find.str().length();
				key.data = &find_key;
				key.size = sizeof(int);

				while((ret = cur1->c_get(cur1, &key, &data, DB_NEXT)) == 0)
				{
					break;
				}
			}
			if(ret == 0)
				printf("SCAN SUCCESS!\n");
			else
				printf("SCAN FAILED!\n");
			ret = cur1->c_close(cur1);
			print_error(ret);
			ret = txn1->commit(txn1, 0);
			print_error(ret);

			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}

	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}


bool test_performbdb_heap_scan_special_random()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		DB_TXN *txn1 = NULL;
		DBC *cur1;

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		print_error(ret);
		ret = dbenv->set_lk_max_objects(dbenv, 200000);
		ret = dbenv->set_lk_max_locks(dbenv, 200000);
		//ret = dbenv->set_lk_max_lockers(dbenv, 2000000);
		print_error(ret);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = dbenv->txn_begin(dbenv, NULL, &txn1, 0);
		print_error(ret);
		ret = db->open(db, txn1, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);
		ret = db->cursor(db, txn1, &cur1, 0);
		print_error(ret);


		int count;

		cout<<"BDB查询出插入的随机(数据/索引)，插入数据最大长度为MAX_RAM_STR_SIZE...行数:";
		cin>>count;
		srand((unsigned)time( NULL ));
		init_DBT(&key, &data);
		vector <std::string>find_data;
		for (int i=0;i<count;i++)
		{
			std::string temp_data;
			RandomGenString(temp_data,(rand()%MAX_RAM_STR_SIZE+1));
			find_data.push_back(temp_data);
			data.data = (void *)temp_data.c_str();
			data.size = temp_data.length();
			//sizeof(temp_data);

			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, txn1, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
			print_error(ret);
		}

		if(ret == 0)
			printf("INSERT SUCCESS!\n");
		else
			printf("INSERT FAILED!\n");

		{
			TimerFixture e;

			init_DBT(&key, &data);

			for (int i = 0;i<count;i++)
			{
				data.data = &find_data[i];
				data.size = find_data[i].length();
				key.data = &i;
				key.size = sizeof(int);
				while((ret = cur1->c_get(cur1, &key, &data, DB_NEXT)) == 0)
				{
					break;
				}
				print_error(ret);
			}

			if(ret == 0)
				printf("SCAN SUCCESS!\n");
			else
				printf("SCAN FAILED!\n");
			ret = cur1->c_close(cur1);
			print_error(ret);
			ret = txn1->commit(txn1, 0);
			print_error(ret);

			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}


	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

bool test_performbdb_heap_scan_large_data()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		DB_TXN *txn1 = NULL;
		DBC *cur1;

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* 在环境打开之前，可调用形式为dbenv->set_XXX()的若干函数设置环境*/
		/* 通知DB使用Rijndael加密算法（参考资料4）对数据进行处理*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* 设置DB的缓存为512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		print_error(ret);
		ret = dbenv->set_lk_max_objects(dbenv, 200000);
		ret = dbenv->set_lk_max_locks(dbenv, 200000);
		//ret = dbenv->set_lk_max_lockers(dbenv, 2000000);
		print_error(ret);
		/* 设置DB查找数据库文件的目录*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* 打开数据库环境，注意后四个标志分别指示DB 启动日志、加锁、缓存、事务处理子系
		统*/

		ret = dbenv->open(dbenv,"./dbtestENV",DB_CREATE|DB_INIT_LOG|DB_INIT_LOCK|DB_INIT_MPOOL|DB_INIT_TXN,0);
		print_error(ret);
		ret = db_create(&db, dbenv, 0);
		print_error(ret);
		ret = dbenv->txn_begin(dbenv, NULL, &txn1, 0);
		print_error(ret);
		ret = db->open(db, txn1, "MyTest.db", NULL, DB_BTREE, DB_CREATE, 0);
		print_error(ret);
		ret = db->cursor(db, txn1, &cur1, 0);
		print_error(ret);


		int count,data_size;

		cout<<"BDB查询出插入的随机(数据/索引)...行数:";
		cin>>count;

		cout<<endl<<"插入数据的大小(生成数据为2的N次方字节)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
		cin>>data_size;

		//srand((unsigned)time( NULL ));
		init_DBT(&key, &data);
		vector <std::string>find_data;
		for (int i=0;i<count;i++)
		{
			std::string temp_data;
			RandomGenString(temp_data,1<<data_size);
			find_data.push_back(temp_data);
			data.data = (void *)temp_data.c_str();
			data.size = temp_data.length();
			//sizeof(temp_data);

			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, txn1, &key, &data, NULL);//DB_NOOVERWRITE检测是否data/key pair已经存在
			print_error(ret);
		}

		if(ret == 0)
			printf("INSERT SUCCESS!\n");
		else
			printf("INSERT FAILED!\n");

		{
			TimerFixture e;

			init_DBT(&key, &data);

			for (int i = 0;i<count;i++)
			{
				data.data = &find_data[i];
				data.size = find_data[i].length();
				key.data = &i;
				key.size = sizeof(int);
				while((ret = cur1->c_get(cur1, &key, &data, DB_NEXT)) == 0)
				{
					break;
				}
				print_error(ret);
			}

			if(ret == 0)
				printf("SCAN SUCCESS!\n");
			else
				printf("SCAN FAILED!\n");
			ret = cur1->c_close(cur1);
			print_error(ret);
			ret = txn1->commit(txn1, 0);
			print_error(ret);

			db->close(db, 0);
			dbenv->close(dbenv, 0);
		}


	} 
	catch  (DbException  & e)
	{
		//cout << " 创建数据库失败: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}
#endif