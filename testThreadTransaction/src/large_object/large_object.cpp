#ifndef WIN32
#define _FILE_OFFSET_BITS 64
#endif

#include <fstream>

#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "LargeObject.h"
#include "test_fram.h"

#include "large_object/large_object_test.h"

#include "TestFrame/TestFrame.h"
#include "TestFrameCommon/TestFrameCommon.h"

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

			char *update_extra_data = "abcdefgaaaaaa";
			DataItem olddi;
			lo->updateExtraData(update_extra_data, strlen(update_extra_data), olddi);

			lo->getExtraData(di);
			cmp_data = (char*)di.getData();
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

static
bool test_LO_Create(ParamBase* para)
{
	using namespace FounderXDB::StorageEngineNS;

	ParamLOControler *paraLO = (ParamLOControler *)para;

	try
	{
		Transaction *trans = para->GetTransaction();
		LargeObjectStore *loStore = LargeObjectStore::getLargeObjectStore();

		LargeObjectID loid = loStore->create(trans, "test_lo", NULL, 0);
		
		LargeObject *lo = loStore->open(trans, loid, LargeObjectStore::WRITE);

		paraLO->pushLO(lo);
	} catch (StorageEngineException &se)
	{
		std::cout << se.getErrorMsg() << std::endl;
		paraLO->SetSuccessFlag(false);

		return false;
	}

	return true;
}

static
bool test_LO_read(ParamBase* para)
{
	using namespace FounderXDB::StorageEngineNS;

	ParamLOControler *paraLO = (ParamLOControler *)para;
	LargeObject *lo = paraLO->getLO();
	
	if(paraLO->getRFileDir().length() == 0)
	{
		std::string newFile = paraLO->getFileDir();
		newFile.append(".backup");
		paraLO->pushRFileDir(newFile);
	}

	unsigned int readSize = 0;

	try
	{
		unsigned int size = paraLO->getWriteSize();
		char *buf = new char[size];

		if((readSize = lo->read(paraLO->GetTransaction(), buf, size)) != 0)
		{
			paraLO->writeBuffer(buf, readSize);
		}

		delete []buf;

		if(paraLO->eof())
		{
			paraLO->close();
		}
	} catch (StorageEngineException &se)
	{
		std::cout << se.getErrorMsg() << std::endl;
		paraLO->SetSuccessFlag(false);

		return false;
	}

	return true;
}

static
bool test_LO_Write(ParamBase* para)
{
	using namespace FounderXDB::StorageEngineNS;

	ParamLOControler *paraLO = (ParamLOControler *)para;
	LargeObject *lo = paraLO->getLO();

	try
	{
		char *buf = new char[paraLO->getWriteSize()];

		unsigned int writeSize = paraLO->getBuffer(buf);

		if(writeSize != 0)
			lo->write(paraLO->GetTransaction(), buf, writeSize);

		delete []buf;

		if(paraLO->eof())
		{
			LargeObjectStore *loStore = LargeObjectStore::getLargeObjectStore();

			LargeObjectID loid = lo->getId();

			loStore->close(lo);

			lo = loStore->open(paraLO->GetTransaction(), loid, LargeObjectStore::READ);

			paraLO->close();

			paraLO->pushLO(lo);
		}
	} catch (StorageEngineException &se)
	{
		std::cout << se.getErrorMsg() << std::endl;
		paraLO->SetSuccessFlag(false);

		return false;
	}

	return true;
}

static
bool t_cmp_file(ParamBase* para)
{
	ParamLOControler *paraLO = (ParamLOControler *)para;

	bool result = cmp_file(paraLO->getFileDir(), paraLO->getRFileDir());

	remove(paraLO->getRFileDir().c_str());

	return result;
}

bool test_LargeObject_Write()
{
	INITRANID()

	ParamLOControler* pPara = new ParamLOControler();

	pPara->pushFileDir("e:\\600M");
	pPara->setWriteSize(8192000);
	unsigned int taskCount = pPara->countTask();

	REGTASK(test_LO_Create,pPara);

	for(int i = 0; i < taskCount; ++i)
	{
		REGTASK(test_LO_Write,pPara);
	}

	for(int i = 0; i < taskCount; ++i)
	{
		REGTASK(test_LO_read,pPara);
	}

	REGTASK(t_cmp_file, pPara);

	return true;
}