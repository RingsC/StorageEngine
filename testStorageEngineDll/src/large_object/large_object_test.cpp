#ifndef WIN32
#define _FILE_OFFSET_BITS 64
#endif

#include <fstream>

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "LargeObject.h"
#include "utils/utils_dll.h"
#include "test_fram.h"

#include "large_object/large_object_test.h"

int64 myfseek(FILE *fd, int64 offset, int org)
{
#ifdef WIN32
	return _fseeki64(fd, offset, org);
#else
	return fseeko(fd, offset, org);
#endif //WIN32
}

int64 myftell(FILE *fd)
{
#ifdef WIN32
	return _ftelli64(fd);
#else
	return ftello(fd);
#endif //WIN32
}

static
uint64 write_lo(FILE *in, uint64 buf_size, 
							FounderXDB::StorageEngineNS::LargeObject *lo,
							FounderXDB::StorageEngineNS::Transaction *txn)
{
	using namespace std;

	char *buf = new char[buf_size];
	uint64 count = 0;
	uint64 file_len;
	uint64 totalSize = 0;
	uint64 curpos = 0;
	uint64 next_len = 0;

	myfseek(in, 0, SEEK_END);
	file_len = myftell(in);
	myfseek(in, 0, SEEK_SET);

	time_t t = time(NULL);

	while(!feof(in))
	{	
		if(totalSize == file_len)
		{
			break;
		}else if((file_len - totalSize) < buf_size)
		{
			fread(buf, file_len - totalSize, 1, in);
		} else 
		{
			fread(buf, buf_size, 1, in);
		}
		next_len = myftell(in);
		totalSize += lo->write(txn, buf, next_len - curpos);
		curpos = next_len;
		++count;
	}
	
	time_t t2 = time(NULL);

	std::cout << "写入 " << totalSize << " 字节耗费 " << difftime(t2, t) << " 秒.\n";

	delete[] buf;

	return totalSize;
}

static
uint64 read_lo( FounderXDB::StorageEngineNS::Transaction *txn,
						 FounderXDB::StorageEngineNS::LargeObject *lo, 
						 std::string path)
{
	using namespace std;
	const uint64 BUFSIZE = 17000;

	FILE *out = fopen(path.c_str(), "wb+");

	char *buf = new char[BUFSIZE];
	uint64 count = 0;
	uint64 size = 0;
	uint64 totalSize = 0;
	
	time_t t = time(NULL);

	while((size = lo->read(txn, buf, BUFSIZE)) > 0)
	{
		totalSize += size;
		fwrite(buf, size, 1, out);
	}

	time_t t2 = time(NULL);

	std::cout << "读出 " << totalSize << " 字节耗费 " << difftime(t2, t) << " 秒.\n";

	fclose(out);

	delete[] buf;

	return totalSize;
}

static
bool cmp_file(std::string path1, std::string path2)
{
	using namespace std;

	bool return_sta = true;

	FILE *in1 = fopen(path1.c_str(), "rb+");
	FILE *in2 = fopen(path2.c_str(), "rb+");
	
	const uint64 BUFSIZE = 50000; 
	char *buf1 = new char[BUFSIZE];
	char *buf2 = new char[BUFSIZE];

	myfseek(in1, 0, SEEK_END);
	const uint64 FILELEN = myftell(in1);
	myfseek(in1, 0, SEEK_SET);

	uint64 countSize = 0;
	uint64 readSize = 0;

	while(true)
	{
		if(FILELEN == countSize)
		{
			break;
		}
		else if((FILELEN - countSize) < BUFSIZE)
		{
			readSize = FILELEN - countSize;
		}else
		{
			readSize = BUFSIZE;
		}

		fread(buf1, readSize, 1, in1);
		fread(buf2, readSize, 1, in2);

		if(memcmp(buf1, buf2, readSize))
		{
			return_sta = false;
			break;
		}
		countSize += readSize;
	}
	fclose(in1);
	fclose(in2);
	delete[] buf1;
	delete[] buf2;

	return return_sta;
}

bool test_simple_large_object_dll()
{
	using namespace FounderXDB::StorageEngineNS;
	using namespace std;

	bool return_sta = true;
	const int SIZEBUF = 8192;

	StorageEngine *storageEngine= StorageEngine::getStorageEngine();
	FXTransactionId tid = 0;
	Transaction *txn = NULL;
	LargeObjectStore *los = NULL;
	LargeObject *lo = NULL;
	uint64 totalSize = 0;
	LargeObjectID lo_id = 0;
	try
	{
		do 
		{
			string path = get_my_arg("test_simple_large_object_dll");
			if(path.length() <= 0)
			{
				return true;
			}

			char *loName = "测试大对象";
			
			/* 写入大对象 */
			txn = storageEngine->getTransaction(tid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			los = LargeObjectStore::getLargeObjectStore(0);
			char *extraData = "time=\"123456\"";
			lo_id = los->create(txn, loName, extraData, strlen("time=\"123456\""));

			lo = los->open(txn, lo_id, LargeObjectStore::READ);

			LargeObject *tmpLo = los->getRecentOpenLO(txn, loName);

			std::string tname = los->loIDGetName(lo_id);
			LargeObjectID tloid = los->nameGetLoID(loName);

			if(tname != std::string(loName) || tloid != lo_id)
			{
				return_sta = false;
			}

			if(tmpLo != lo)
			{
				return_sta = false;
			}

			los->close(lo);

			lo = los->open(txn, lo_id, LargeObjectStore::WRITE);

			tmpLo = los->getRecentOpenLO(txn, loName);
			if(tmpLo != lo)
			{
				return_sta = false;
			}

			FILE *in = fopen(path.c_str(), "rb+");

			if(!in)
			{
				cout << "打开文件" << path << "失败！\n";
				return_sta = false;
				break;
			}
			totalSize = write_lo(in, SIZEBUF, lo, txn);
			fclose(in);

			/* 大对象写入完毕，关闭大对象 */
			//los->close(lo);

			lo = NULL;
			los = NULL;
			txn->commit();

			/* 现在读取大对象，并在同级目录下生成新文件 */
			tid = 0;
			txn = storageEngine->getTransaction(tid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			los = LargeObjectStore::getLargeObjectStore(0);
			lo = los->open(txn, loName, LargeObjectStore::WRITE);
			los->close(lo);

			lo = los->open(txn, loName, LargeObjectStore::READ);
      DataItem di;
      lo->getExtraData(di);
			size_t len = di.getSize();
			char *cmp_data = (char*)di.getData();
			if(memcmp(cmp_data, extraData, len) != 0)
			{
				return_sta = false;
			}
			cmp_data = (char*)lo->getName();
			if(strcmp(cmp_data, loName) != 0)
			{
				return_sta = false;
			}
			LargeObjectID tmpid = lo->getId();
			if(tmpid != lo_id)
			{
				return_sta = false;
			}

			string path1 = path;
			path1.append(".1");
			read_lo(txn, lo, path1);

			char *update_extra_data = "abcdefgaaaaaatest";
			DataItem olddi;
			lo->updateExtraData(update_extra_data, strlen(update_extra_data), olddi);

			lo->getExtraData(di);
			cmp_data = (char*)di.getData();
			return_sta = (return_sta ? di.getSize() == strlen(update_extra_data) : false);
			return_sta = (return_sta ? (memcpy(cmp_data, update_extra_data, di.getSize())) : false);

			/* 大对象读取完毕，关闭大对象 */
			los->close(lo);

			lo = NULL;
			los = NULL;
			txn->commit();

			/* 比较两个文件的内容前后是否完全一致 */
			return_sta = (return_sta ? cmp_file(path, path1) : false);
			remove(path1.c_str());

			if(return_sta)
			{
				/* 这一步测试truncate功能 */
				const uint64 TRUNCATESIZE = totalSize * 0.6;
				tid = 0;
				txn = storageEngine->getTransaction(tid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
				los = LargeObjectStore::getLargeObjectStore(0);
				lo = los->open(txn, loName, LargeObjectStore::READWRITE);
				DataItem di;
        lo->getExtraData(di);
				cmp_data = (char*)di.getData();
				len = di.getSize();
				if(memcmp(cmp_data, update_extra_data, len) != 0)
				{
					return_sta = false;
				}

				lo->truncate(txn, TRUNCATESIZE);
				/* 检查大对象大小 */
				string path2 = path;
				path2.append(".2");
				uint64 tsize = read_lo(txn, lo, path2);
				los->close(lo);

				lo = NULL;
				los = NULL;
				txn->commit();
				return_sta = (return_sta ? ((tsize == TRUNCATESIZE) ? true : false) : false);
				remove(path2.c_str());
			}

		} while (0);
	}
	catch (StorageEngineException &ex) 
	{
		std::cout << ex.getErrorNo() << std::endl;
		std::cout << ex.getErrorMsg() << std::endl;
		return_sta = false;
	}

	/* 
	 * 这一步测试drop大对象功能
	 */
	if(return_sta)
	{
		txn = storageEngine->getTransaction(tid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		try
		{
			los = LargeObjectStore::getLargeObjectStore(0);
			los->drop(txn, lo_id);
			lo = los->open(txn, "first_lo2", LargeObjectStore::READ);
			return_sta = false;
		} catch(StorageEngineException &)
		{
			/* drop成功后再打开会抛出异常 */
			return_sta = true;
		}
		los = NULL;
		txn->commit();
		txn = NULL;
	}

	if(los)
	{
		los->close(lo);
	}

	if(txn)
	{
		txn->commit();
	}

	return return_sta;
}

static
void copy_file(FILE *out, FILE *in, const uint64 size)
{
	const uint64 READ_COUNT = 50000;
	uint64 read_size;
	uint64 currentPos = myftell(in);
	char *data = new char[READ_COUNT];

	while(currentPos < size)
	{
		if(size - currentPos > READ_COUNT)
		{
			read_size = READ_COUNT;
		} else 
		{
			read_size = size - currentPos;
		}

		fread(data, read_size, 1, in);
		fwrite(data, read_size, 1, out);

		currentPos += read_size;
	}
}

bool test_lo_seek_tell_dll()
{
	INTENT("测试大对象的seek和tell方法。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace std;

	bool return_sta = true;
	const int SIZEBUF = 8192;

	StorageEngine *storageEngine= StorageEngine::getStorageEngine();
	FXTransactionId tid = 0;
	Transaction *txn = NULL;
	LargeObjectStore *los = NULL;
	LargeObject *lo = NULL;
	uint64 totalSize = 0;
	LargeObjectID lo_id = 0;

	string path = get_my_arg("test_lo_seek_tell_dll");
	string path1 = path;
	path1.append(".1");
	string path3 = path;
	path3.append(".3");

	if(path.length() <= 0)
	{
		return true;
	}

	do 
	{
		try
		{
			/* 写入大对象 */
			txn = storageEngine->getTransaction(tid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);

			los = LargeObjectStore::getLargeObjectStore(0);
			lo_id = los->create(txn, "first_lo", NULL, 0);

			lo = los->open(txn, lo_id, LargeObjectStore::WRITE);
			FILE *in = fopen(path.c_str(), "rb+");
			if(!in)
			{
				cout << "打开文件 " << path << " 失败！\n";
				return_sta = false;
				break;
			}
			totalSize = write_lo(in, SIZEBUF, lo, txn);

			if(totalSize <= 0)
			{
				return_sta = false;
				break;
			}

			fclose(in);
			/* 关闭大对象 */
			los->close(lo);
			lo = NULL;
			los = NULL;
			txn->commit();
			txn = NULL;

			/* 该大对象的大小为totalSize,接下来我们测试seek和tell方法 */
			txn = storageEngine->getTransaction(tid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			los = LargeObjectStore::getLargeObjectStore(0);
			lo = los->open(txn, lo_id, LargeObjectStore::READ);

			/* 现在只读取该大对象的后%40的数据 */
			/* 这里将大对象的读取起始位置设置到60%的位置 */
			lo->seek(txn, totalSize * 0.6, LargeObject::SET);
			uint64 currentPos = lo->tell();

			return_sta = (return_sta ? 
				(currentPos == (uint64)(totalSize * 0.6)) : false);

			/* 读大对象后40%的数据 */
			read_lo(txn, lo, path1);
			/* 此时大对象已经读取完毕，offset应该停留在最后 */
			uint64 offset = lo->tell();

			return_sta = (return_sta ? offset == totalSize : false);

			los->close(lo);
 			los->drop(txn, "first_lo");
			txn->commit();
			txn = NULL;

			/* 拷贝完整大对象的后40%的数据至一个新的文件 */
			FILE *out = fopen(path3.c_str(), "wb");
			in = fopen(path.c_str(), "rb+");
			myfseek(in, totalSize * 0.6, SEEK_SET);
			copy_file(out, in, totalSize);
			fclose(in);
			fclose(out);

			/* 
			 * 比较文件，两个文件分别是: 
			 * 1.直接读取的大对象后40%的文件
			 * 2.通过拷贝完整源文件文件后40%内容的文件
			 */

			return_sta = (return_sta ? cmp_file(path1, path3) : false);

		} catch(StorageEngineException &ex)
		{
			std::cout << ex.getErrorNo() << std::endl;
			std::cout << ex.getErrorMsg() << std::endl;
			return_sta = false;
		} catch(...) 
		{
			return_sta = false;
		}
	} while (0);

	remove(path1.c_str());
	remove(path3.c_str());

	if(txn)
	{
		txn->abort();
	}

	return return_sta;

}

bool test_DataBase()
{
	using namespace FounderXDB::StorageEngineNS;
	FXTransactionId tid = 0;
	Transaction *txn = NULL;
	Transaction *txn2 = NULL;
	Transaction *txn3 = NULL;
	bool result = true;
	bool txnAbort = true;
	bool txn2Abort = true;
	bool txn3Abort = true;
	try
	{
		StorageEngine *storageEngine= StorageEngine::getStorageEngine();

		txn = storageEngine->getTransaction(tid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		storageEngine->createDatabase(txn,"test1","defaulttablespace",0,0);
		txn->commit();
		txnAbort = false;

		FXTransactionId tid2 = 0;
		txn2 = storageEngine->getTransaction(tid2, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		storageEngine->dropDatabase(txn2,"test1");
		txn2->commit();
		txn2Abort = false;

		FXTransactionId tid3 = 0;
		txn3 = storageEngine->getTransaction(tid3, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
		storageEngine->createDatabase(txn3,"test2","defaulttablespace",0,0);
		storageEngine->dropDatabase(txn3,"test2");
		txn3->commit();
		txn3Abort = false;
	}
	catch(StorageEngineException &se)
	{
		if (txnAbort &&  NULL != txn)
			txn->abort();
		if (txn2Abort &&  NULL != txn2)
			txn2->abort();
		if (txn3Abort &&  NULL !=txn3)
			txn2->abort();
		printf("%s\n", se.getErrorMsg());
		result = false;
	}
	
	return result;
}

static
void lo_database_switch_write(const char dbname[][1024], 
															const char loname[][1024],
															const int arrlen,
															DataGenerater *dg,
															FounderXDB::StorageEngineNS::Transaction *trans)
{
	using namespace FounderXDB::StorageEngineNS;

	const int WRITE_LEN = 8192;
	int count = 0;
	const int WRITE_COUNT = dg->getDataLen() / WRITE_LEN;
	StorageEngine *se = StorageEngine::getStorageEngine();
	LargeObjectStore *lostore = LargeObjectStore::getLargeObjectStore();
	se->setCurrentDatabase(dbname[0]);
	LargeObject *lo1 = lostore->open(trans, loname[0], LargeObjectStore::WRITE);
	se->setCurrentDatabase(dbname[1]);
	LargeObject *lo2 = lostore->open(trans, loname[1], LargeObjectStore::WRITE);

	for (int i = 0; i < WRITE_COUNT; ++i)
	{
		se->setCurrentDatabase(dbname[0]);
		lo1->write(trans, dg->operator [](0) + i * WRITE_LEN, WRITE_LEN);

		se->setCurrentDatabase(dbname[1]);
		lo2->write(trans, dg->operator [](1) + i * WRITE_LEN, WRITE_LEN);
	}
	lostore->close(lo1);
	lostore->close(lo2);
}

static
char *read_lo_data(const char *loname, FounderXDB::StorageEngineNS::Transaction *trans, int &readlen)
{
	using namespace FounderXDB::StorageEngineNS;

	StorageEngine *se = StorageEngine::getStorageEngine();
	LargeObjectStore *lostore = LargeObjectStore::getLargeObjectStore();
	LargeObject *lo = lostore->open(trans, loname, LargeObjectStore::READ);
	char *data = (char *)malloc(1024 * 1024);
	readlen = lo->read(trans, data, 1024 * 1024);
	assert(readlen == 1024 * 1024);
	lostore->close(lo);
	return data;
}

bool test_database_large_object_dll()
{
	INTENT("测试在多个数据库的情况下创建同名大对象并操作大对象。");

	using namespace FounderXDB::StorageEngineNS;
	using namespace std;

	bool return_sta = true;
	const int DLEN = 1024 * 1024;
	const char dbname[2][1024] = {"db1", "db2"};
	const char loname[2][1024] = {"lo1", "lo1"};
	DataGenerater dg(2, DLEN);
	dg.dataGenerate();

	StorageEngine *se = StorageEngine::getStorageEngine();
	FXTransactionId xid = InvalidTransactionID;
	Transaction *trans = NULL;

	do 
	{
		try
		{
			/* 启动事务并创建两个数据库 */
			trans = se->getTransaction(xid, Transaction::READ_COMMITTED_ISOLATION_LEVEL);
			se->createDatabase(trans, dbname[0], "defaulttablespace");
			se->createDatabase(trans, dbname[1], "defaulttablespace");
			/* 切换到A数据库创建同名大对象 */
			se->setCurrentDatabase(dbname[0]);
			LargeObjectID loid1 = LargeObjectStore::getLargeObjectStore()->create(trans, loname[0]);
			/* 切换到B数据库创建同名大对象 */
			se->setCurrentDatabase(dbname[1]);
			LargeObjectID loid2 = LargeObjectStore::getLargeObjectStore()->create(trans, loname[1]);
			/* 接下来在不同数据库之间切换并且写入这两个大对象 */
			lo_database_switch_write(dbname, loname, 2, &dg, trans);
			/* 大对象写入完毕，读出大对象数据并且比较结果 */
			int readlen1, readlen2;
			se->setCurrentDatabase(dbname[0]);
			char *data1 = read_lo_data(loname[0], trans, readlen1);
			se->setCurrentDatabase(dbname[1]);
			char *data2 = read_lo_data(loname[1], trans, readlen2);
			if (memcmp(data1, dg[0], readlen1) != 0 ||
					memcmp(data2, dg[1], readlen2 != 0))
			{
				return_sta = false;
			}
			free(data1);
			free(data2);
			trans->abort();
		} catch(StorageEngineException &ex)
		{
			std::cout << ex.getErrorNo() << std::endl;
			std::cout << ex.getErrorMsg() << std::endl;
			return_sta = false;
		} catch(...) 
		{
			return_sta = false;
		}
	} while (0);

	return return_sta;
}
	