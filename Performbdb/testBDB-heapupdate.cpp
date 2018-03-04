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
#include <sstream>
#include <vector>
#include "bdb_utils.h"
#include "testBDB-heapupdate.h"

#ifdef _WIN32
extern "C" {
	extern int getopt(int, char * const *, const char *);
	extern int optind;
}
#else
#include <unistd.h>
#endif

#include <db_cxx.h>

#define MAX_RAM_STR_SIZE 2000

using std::cin;
using std::cout;
using std::cerr;
using std::vector;
using std::string;

using namespace std;

bool test_performbdb_heap_update()
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

		/* �ڻ�����֮ǰ���ɵ�����ʽΪdbenv->set_XXX()�����ɺ������û���*/
		/* ֪ͨDBʹ��Rijndael�����㷨���ο�����4�������ݽ��д���*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* ����DB�Ļ���Ϊ512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		print_error(ret);
		ret = dbenv->set_lk_max_objects(dbenv, 20000);
		ret = dbenv->set_lk_max_locks(dbenv, 20000);
		//ret = dbenv->set_lk_max_lockers(dbenv, 2000000);
		print_error(ret);
		/* ����DB�������ݿ��ļ���Ŀ¼*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* �����ݿ⻷����ע����ĸ���־�ֱ�ָʾDB ������־�����������桢��������ϵ
		ͳ*/

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

		cout<<"BDB�޸�(����/����)...����:";
		cin>>count;
		init_DBT(&key, &data);
		data.data = &name;
		data.size = sizeof("test");
		for (int i=0;i<count;i++)
		{
			//Dbt key(&i,sizeof(i));
			key.data = &i;
			key.size = sizeof(int);
			ret = db->put(db, txn1, &key, &data, NULL);//DB_NOOVERWRITE����Ƿ�data/key pair�Ѿ�����
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
				while((ret = cur1->c_get(cur1, &key, &data, DB_SET)) == 0)
				{
					ret = cur1->c_del(cur1, 0);
					break;
				}
			}
			init_DBT(&key, &data);
			data.data = name_new;
			data.size = sizeof("test_new");

			for (int i = 0;i<count;i++)
			{
				key.data = &i;
				key.size = sizeof(int);

				ret = db->put(db, txn1, &key, &data, DB_NOOVERWRITE);
			}
			if(ret == 0)
				printf("MODIFY SUCCESS!\n");
			else
				printf("MODIFY FAILED!\n");
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
		//cout << " �������ݿ�ʧ��: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

bool test_performbdb_heap_update_special_random()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		DB_TXN *txn1 = NULL;
		DBC *cur1;

		char name_new[10] = {'\0'};
		memcpy(name_new,"test",sizeof("test"));

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* �ڻ�����֮ǰ���ɵ�����ʽΪdbenv->set_XXX()�����ɺ������û���*/
		/* ֪ͨDBʹ��Rijndael�����㷨���ο�����4�������ݽ��д���*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* ����DB�Ļ���Ϊ512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		print_error(ret);
		ret = dbenv->set_lk_max_objects(dbenv, 200000);
		ret = dbenv->set_lk_max_locks(dbenv, 200000);
		//ret = dbenv->set_lk_max_lockers(dbenv, 2000000);
		print_error(ret);
		/* ����DB�������ݿ��ļ���Ŀ¼*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* �����ݿ⻷����ע����ĸ���־�ֱ�ָʾDB ������־�����������桢��������ϵ
		ͳ*/

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

		cout<<"BDB�޸Ĳ�������(����/����)������������󳤶�ΪMAX_RAM_STR_SIZE...����:";
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
			ret = db->put(db, txn1, &key, &data, NULL);//DB_NOOVERWRITE����Ƿ�data/key pair�Ѿ�����
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
				while((ret = cur1->c_get(cur1, &key, &data, DB_SET)) == 0)
				{
					ret = cur1->c_del(cur1, 0);
					break;
				}
			}
			init_DBT(&key, &data);
			data.data = name_new;
			data.size = sizeof("test");

			for (int i = 0;i<count;i++)
			{
				key.data = &i;
				key.size = sizeof(int);

				ret = db->put(db, txn1, &key, &data, DB_NOOVERWRITE);
				print_error(ret);
			}
			if(ret == 0)
				printf("MODIFY SUCCESS!\n");
			else
				printf("MODIFY FAILED!\n");
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
		//cout << " �������ݿ�ʧ��: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

bool test_performbdb_heap_update_large_data()
{
	try 
	{
		DB_ENV *dbenv;
		int ret;
		DB *db;
		DBT key, data;
		DB_TXN *txn1 = NULL;
		DBC *cur1;

		char name_new[10] = {'\0'};
		memcpy(name_new,"test",sizeof("test"));

		ret = db_env_create(&dbenv, 0);
		print_error(ret);

		/* �ڻ�����֮ǰ���ɵ�����ʽΪdbenv->set_XXX()�����ɺ������û���*/
		/* ֪ͨDBʹ��Rijndael�����㷨���ο�����4�������ݽ��д���*/
		//ret = dbenv->set_encrypt(dbenv, "encrypt_string", DB_ENCRYPT_AES);
		/* ����DB�Ļ���Ϊ512M */
		ret = dbenv->set_cachesize(dbenv, 0, 512*1024*1024, 0);
		print_error(ret);
		ret = dbenv->set_lk_max_objects(dbenv, 200000);
		ret = dbenv->set_lk_max_locks(dbenv, 200000);
		//ret = dbenv->set_lk_max_lockers(dbenv, 2000000);
		print_error(ret);
		/* ����DB�������ݿ��ļ���Ŀ¼*/
		//ret = dbenv->set_data_dir(dbenv,"./");
		/* �����ݿ⻷����ע����ĸ���־�ֱ�ָʾDB ������־�����������桢��������ϵ
		ͳ*/

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

		cout<<"BDB�޸Ĳ�������(����/����)...����:";
		cin>>count;

		cout<<endl<<"�������ݵĴ�С(��������Ϊ2��N�η��ֽ�)"<<endl<<"10->1k  15->32K  20->1M  25->32M 30->1G:";
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
			ret = db->put(db, txn1, &key, &data, NULL);//DB_NOOVERWRITE����Ƿ�data/key pair�Ѿ�����
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
				while((ret = cur1->c_get(cur1, &key, &data, DB_SET)) == 0)
				{
					ret = cur1->c_del(cur1, 0);
					break;
				}
			}
			init_DBT(&key, &data);
			data.data = name_new;
			data.size = sizeof("test");

			for (int i = 0;i<count;i++)
			{
				key.data = &i;
				key.size = sizeof(int);

				ret = db->put(db, txn1, &key, &data, DB_NOOVERWRITE);
				print_error(ret);
			}
			if(ret == 0)
				printf("MODIFY SUCCESS!\n");
			else
				printf("MODIFY FAILED!\n");
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
		//cout << " �������ݿ�ʧ��: " ;
		cout << e.what() << endl;
	} 

	return   0 ;
}

#endif