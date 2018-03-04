#include <list>
#include <bitset>
#include <fstream>
#include <time.h>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/regex.hpp>
#ifdef WINDOWS_MULTIPLE
#include <Windows.h>
#endif //WINDOWS_MULTIPLE
#include "test_twophase_frame.h"
extern char *my_itoa(int value, char *string, int radix);

void (*pToInitFunc)(void);
void (*pToFreeFunc)(void);
bool can_call_free;						//控制pToFreeFunc跳过第一次调用
char *current_file_name = "";	//记录当前文件
unsigned int line_num = 0;		//记录当前函数所在行
unsigned int test_num;				//记录test的总数
unsigned int test_success;		//记录test成功的总数
unsigned int test_fail;				//记录test失败的总数
FILE *lpf = NULL;							//日志文件句柄
FILE *flpf = NULL;						//测试失败的日志文件句柄
const char *log_dir = "../../../testdir/log_";
const char *log_path = "../../../testdir/log_/test_.log";
const char *log_fail_path = ".../../../testdir/log_/test_fail_.log";
TestLog tl;
std::list<TestLog> list_fail_test;//记录所有失败测试的信息
char *test_func_name = NULL;

static std::string t_argv;
static int t_argc;

bool StorageEngineFixture::bEnded;

/* 所有测试结束调用的函数列表 */
static std::list<std::pair<OnTestExitFunc, void*> > on_exit_func_list;

void on_test_exit_at_fornt(OnTestExitFunc func, void* arg_list)
{
	using namespace std;
	on_exit_func_list.push_front(pair<OnTestExitFunc, void*>(func, arg_list));
}

void on_test_exit_at_tail(OnTestExitFunc func, void* arg_list)
{
	using namespace std;
	on_exit_func_list.push_back(pair<OnTestExitFunc, void*>(func, arg_list));
}

void run_test_exit()
{
	using namespace std;

	list<pair<OnTestExitFunc, void*> >::iterator begin = on_exit_func_list.begin();
	while(begin != on_exit_func_list.end())
	{
		(*(*begin->first))(begin->second);
		++begin;
	}
	on_exit_func_list.clear();
}

void check_for_release()
{
	if(on_exit_func_list.size() > 100)
	{
		run_test_exit();
	}
}

bool is_time_out()
{
	using namespace std;

	static time_t start_time = time(NULL);
	static int minutes = 0;

	if(minutes == 0)
	{
		const int ARGC = get_argc();
		string str = get_argv();
		string::size_type pos = str.rfind(' ');
		string time = str.substr(++pos);

		boost::regex exampleregex("^[0-9]*"); 
		boost::cmatch result; 
		if(boost::regex_search(time.c_str(), result, exampleregex))
		{
			minutes = atoi(time.c_str());
		} else {
			return false;
		}
	}

	const time_t CURRENT_TIME = time(NULL);
	double diff_time = difftime(CURRENT_TIME, start_time);
	if(diff_time >= minutes * 60)
	{
		start_time = time(NULL);
		return false;
	} else {
		return true;
	}
}

std::string get_my_arg(char *func_name)
{
	using namespace std;

	const int ARGC = get_argc();
	string str = get_argv();

	string::size_type pos;

	pos = str.rfind(' ');
	string tmpArgv = str.substr(pos + 1);
	pos = tmpArgv.find(func_name);

	if(pos == string::npos || strlen(func_name) == tmpArgv.length())
	{
		return string("");
	}

	pos += strlen(func_name) + 1;
	if(pos == string::npos)
	{
		return string("");
	}
	tmpArgv = tmpArgv.substr(pos, tmpArgv.length() - pos);
	string::size_type posEnd = tmpArgv.find(";");
	tmpArgv = tmpArgv.substr(0, posEnd);

	return tmpArgv;
}

std::map<std::string,std::string> ProgramOpts;

void save_arg(const int argc, char **argv)
{
	t_argc = argc;
	for(int i = 0; i < t_argc; ++i)
	{
		t_argv.append(argv[i]);
		t_argv.append(" ");
	}
	t_argv.erase(t_argv.rfind(" "));

	//save program options
	using namespace std;
	using namespace boost::xpressive;
	sregex opReg = sregex::compile("(-\\w+)\\s+(\\S+)");
	smatch mach;
	std::string::const_iterator it = t_argv.begin(); 
	while (regex_search(it,t_argv.end(),mach,opReg))
	{
		ProgramOpts[mach[1]] = mach[2];
		it = mach[0].second;
	}

}

std::string get_argv()
{
	return t_argv;
}

int get_argc()
{
	return t_argc;
} 

/*
* 命令行参数类型：
*		-c [TA | TB | TC |...] --运行某种类型的测例，如TA, TB, TC为类型
*		-s [function name] --运行某个指定测例
*/
bool run_test_parse(const char *func_name, const RUN_TYPE rt)
{
	using namespace std;
	using namespace boost::xpressive;
	int argc = get_argc();
	if(argc <= 2)
	{
		return false;
	}
	string str = get_argv();

	RUN_TYPE t_rt;
	/* 运行某种类型测例 */
	string::size_type nStartPos = 0;
	sregex reg = sregex::compile("-c\\s+(\\w+)");
	smatch math;
	if (regex_search(str,math,reg))
	{
		string tmp = math[1];
		if(tmp.compare("TransPersistence") == 0)
		{
			t_rt = TransPersistence;
		} else if(tmp.compare("TransExit") == 0)
		{
			t_rt = TransExit;
		} else if(tmp.compare("LTTest"))
		{
			t_rt = LTTest;
		} else 
		{
			return false;
		}
		return t_rt == rt ? true : false;
	}
	//if((nStartPos = str.find("-c")) != string::npos)
	//{
	//	string::size_type nEndPos = str.find(" ",nStartPos + 3);
	//	if (nEndPos == string::npos)
	//	{
	//		nEndPos = str.length();
	//	}
	//	string tmp = str.substr(nStartPos + 3, nEndPos - pos);
	//	if(tmp.compare("TransPersistence") == 0)
	//	{
	//		t_rt = TransPersistence;
	//	}else if(tmp.compare("TransExit") == 0)
	//	{
	//		t_rt = TransExit;
	//	}else
	//	{
	//		return false;
	//	}
	//	return t_rt == rt ? true : false;
	//}

	if(str.find("-s") != -1)
	{
		int pos = str.rfind(" ") + 1;
		string tmp = str.substr(pos);	
		pos = tmp.find(func_name);
		return pos != string::npos ? true : false;
	}

	return false;
}

bool is_run_myself(const char *func_name)
{
	using namespace std;
	string path;
	path.append(func_name);
	int _return = false;
	{
		FILE *in = fopen(path.c_str(), "r");
		if(!in)
		{
			_return = false;	
		}else
		{
			_return = true;
			fclose(in);
		}
		return _return;
	}
}

void run_begin(const char *func_name)
{
	using namespace std;
	string path;;
	path.append(func_name);
	FILE *out = fopen(path.c_str(), "wb");
	fclose(out);
}

void run_remove(const char *func_name)
{
	using namespace std;
	string path;
	path.append(func_name);
	remove(path.c_str());
}

void save_heap_id(const unsigned int heap_id, const char *func_name)
{
	using namespace std;

	string path(func_name);
	ofstream out(path.c_str());
	if(out)
	{
		out << "h" << heap_id << "\n";
		out.close();
	}
}

void save_heap_and_index(const unsigned int heap_id, const unsigned int index_id, const char *func_name)
{
	using namespace std;

	string path(func_name);
	ofstream out(path.c_str());
	if(out)
	{
		out << "h" << heap_id << "i" << index_id << "\n";
		out.close();
	}
}

void get_relation_info(const char *func_name, std::map<unsigned int, std::set<unsigned int> > &m_hi)
{
	using namespace std;
	string path(func_name);
	ifstream in(path.c_str());
	if(in)
	{
		string str;
		while((in >> str) != NULL)
		{
			int start = str.find("h");
			unsigned int heap_id;
			++start;
			string tmp;
			tmp = str.substr(start, str.length() - start);
			heap_id = atoi(tmp.c_str());
			set<unsigned int> s_i;
			m_hi[heap_id] = s_i;
			int end = str.find("i");
			if(end != -1)
			{
				++end;
				tmp = str.substr(end, str.length() - end);
				unsigned int index_id;
				index_id = atoi(tmp.c_str());
				m_hi[heap_id].insert(index_id);
			}
		}
	}
}

//记录在多进程情况下的测试成功或者失败数
//记录中第一个数值为成功的数值，第二个为失败的数值
void logSFNum(int sta)
{
	FILE *sf_sta = fopen("c:/sf_sta.txt", "r+");
	assert(sf_sta);
	int success = 0;
	int fail = 0;
	fscanf(sf_sta, "%d", &success);
	fscanf(sf_sta, "%d", &fail);
	//如果测试成功
	if(sta)
	{
		++success;
	}else
	{
		++fail;
	}
	//定位于文件头，写回数据
	fseek(sf_sta, 0, SEEK_SET);
	fprintf(sf_sta, "%d\n", success);
	fprintf(sf_sta, "%d\n", fail);
	fclose(sf_sta);
}

//创建多进程测试成功或者失败的计数文件
void createLogSFNum()
{
	FILE *f = fopen("c:/sf_sta.txt", "r");
	//如果该文件不存在
	if(!f)
	{
		f = fopen("c:/sf_sta.txt", "w+");
		fprintf(f, "%d\n", 0);
		fprintf(f, "%d", 0);
	}
	fclose(f);
}

//删除多进程测试成功或者失败的计数文件
void deleteLogSFNum()
{
	remove("c:/sf_sta.txt");
	remove("c:/process_fail.txt");
}

//更新测试数据
void updateSF()
{
	FILE *sf_sta = fopen("c:/sf_sta.txt", "r+");
	//如果文件不存在，即没有多进程测试
	if(!sf_sta)
	{
		return;
	}
	int success = 0;
	int fail = 0;
	fscanf(sf_sta, "%d", &success);
	fscanf(sf_sta, "%d", &fail);
	test_success += success;
	test_fail += fail;
	fclose(sf_sta);
}

//启动一个新的进程
void runNewProcess(char *func_name, int wait)
{
	//拼接命令行参数
	char command[1024];
	strcpy(command, " ");
	strcat(command, "--detect_memory_leaks=0 ");
	strcat(command, func_name);
#ifdef WINDOWS_MULTIPLE
	STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;
	PROCESS_INFORMATION pi;
	CreateProcess("teststorageEngine\\debug\\testStorageEngineLib.exe", command, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
#endif //WINDOWS_MULTIPLE
	if(wait)
	{
		//等待所有子进程完成，等待时间为1小时
#ifdef WINDOWS_MULTIPLE
		WaitForMultipleObjects(1, &pi.hProcess, true, 1000 * 3600);
#endif //WINDOWS_MULTIPLE
	}
}

//获取命令行参数，判断要执行的测试用例
int isExecuteTest(char *func_name)
{
	char *cl = NULL;
#ifdef WINDOWS_MULTIPLE
	cl = GetCommandLine();
#endif //WINDOWS_MULTIPLE
	//第一个命令行参数为boost库的--detect_memory_leaks=0,忽略
	//这里获取第二个命令行参数，即函数名
	if(!test_func_name)
	{
		strtok(cl, " ");
		test_func_name = strtok(NULL, " ");
	}
	if(test_func_name == NULL)
	{
		return 0;
	}
	if(strcmp(test_func_name, func_name) == 0)
	{
		return 1;
	}else
		return 0;
}

//判断当前进程是否脱离主进程执行
int runMultiProcess()
{
	FILE *f_sta = fopen("c:/sta", "r+");
	if(f_sta)
	{
		fclose(f_sta);
		return 1;
	}
	return 0;
}

//创建多进程执行的标记文件
void createMultiProcessFile()
{
	FILE *f_sta = fopen("c:/sta", "w+");
	assert(f_sta);
	fclose(f_sta);
}

//删除多进程执行的标记文件
void deleteMultiProcessFile()
{
	remove("c:/sta");
}

void setFunc_(void (*p1)(void), void (*p2)(void))
{
	pToInitFunc = p1;
	pToFreeFunc = p2;
}

//添加失败的测试信息
void insertFailTestInfo()
{
	if(!tl.test_status)
	{
		list_fail_test.push_back(tl);
	}
}

//多进程添加失败的测试信息
void insertProcessFailTestInfo()
{
	if(!tl.test_status)
	{
		FILE *process_f = fopen("c:/process_fail.txt", "a+");
		assert(process_f);
		fprintf(process_f, "%s\n", tl.test_name);
		fprintf(process_f, "%s\n", tl.test_time);
		fprintf(process_f, "%s\n", tl.test_function_path);
		fprintf(process_f, "%s\n", tl.test_intent);
		fprintf(process_f, "%s\n", tl.test_crash_site);
		fclose(process_f);
	}
}

//初始化失败测试信息
void initFailTestInfo()
{
	FILE *process_f = fopen("c:/process_fail.txt", "r");
	if(process_f)
	{
		tl.test_status = false;
		char c[1024];
		while(fgets(c, 1024, process_f) != NULL)
		{
			strcpy(tl.test_name, c);
			fgets(c, 1024, process_f);
			strcpy(tl.test_time, c);
			fgets(c, 1024, process_f);
			strcpy(tl.test_function_path, c);
			fgets(c, 1024, process_f);
			strcpy(tl.test_intent, c);
			fgets(c, 1024, process_f);
			strcpy(tl.test_crash_site, c);
			list_fail_test.push_back(tl);
		}
		fclose(process_f);
	}
}

//打印所有失败测试信息
void printfFailTestInfo()
{
	initFailTestInfo();
	if(list_fail_test.size() == 0)
	{
		fprintf(stdout, "\r\r\n\t+++++++++++++++++没有失败测试.+++++++++++++++++\r\r\n");
		return;
	}
	fprintf(stdout, "\r\r\n\t+++++++++++++++++所有失败测试.+++++++++++++++++\r\r\n");
	std::list<TestLog>::iterator begin = list_fail_test.begin(), end = list_fail_test.end();
	while(begin != end)
	{
		fprintf(stdout, "%s\r\r\n", begin->split_line);
		fprintf(stdout, "\r\r\ntest_function_name:%s\r\r\n\r\r\n", begin->test_name);
		fprintf(stdout, "test_date:%s\r\r\n", begin->test_time);
		fprintf(stdout, "test_function_path:%s\r\r\n", begin->test_function_path);
		fprintf(stdout, "test_intent:%s\r\r\n", begin->test_intent);
		fprintf(stdout, "test_crash_site:%s\r\r\n\r\r\n", begin->test_crash_site);
		if(begin->test_status)
		{
			fprintf(stdout, "test_status:success\r\r\n");
		}else 
		{
			fprintf(stdout, "test_status:fail\r\r\n");
		}
		fprintf(stdout, "%s\r\r\n\r\r\n", begin->split_line);
		++begin;
	}
}

//格式化TestLog的test_crash_site
void formatCrashSite(char *file_name, int line_num)
{
	if(file_name && line_num)
	{
		strcpy(tl.test_crash_site, file_name);
		strcat(tl.test_crash_site, " , line ");
		char line_[5];
		sprintf(line_,"%d",line_num);
		strcat(tl.test_crash_site, line_);
	}
}

//初始化日志环境
void initLog()
{
#ifdef WIN32
	mkdir(log_dir);
#else
	mkdir(log_dir, S_IWRITE|S_IREAD|S_IRWXU);
#endif //WIN32

	if(lpf == NULL)
	{
		lpf = fopen(log_path, "a+");
		if(!lpf)
		{
			assert("can not open log file" == 0);
		}
	}
	if(flpf == NULL)
	{
		flpf = fopen(log_fail_path, "a+");
		if(!flpf)
		{
			fclose(lpf);
			assert("can not open fail log file" == 0);
		}
	}
}

//记录今天日期
void logToday()
{
	if(lpf)
	{
		char time_[64];
		time_t t = time(0);
		strftime(time_, sizeof(time_), "%Y-%m-%d %H:%M:%S", localtime(&t));
		fprintf(lpf, "test today : ");
		fprintf(lpf, "\t\t+++++++ today %s +++++++\n", time_);
	}
}

//写入日志内容
void log_out(FILE *p)
{
	if(p)
	{
		fprintf(p, "%s\n", tl.split_line);
		fprintf(p, "test_function_name:%s\n\n", tl.test_name);
		fprintf(p, "test_date:%s\n", tl.test_time);
		fprintf(p, "test_function_path:%s\n", tl.test_function_path);
		fprintf(p, "test_intent:%s\n", tl.test_intent);
		fprintf(p, "test_crash_site:%s\n\n", tl.test_crash_site);
		if(tl.test_status)
		{
			fprintf(p, "test_status:success\n");
		}else 
		{
			fprintf(p, "test_status:fail\n");
		}
		fprintf(p, "%s\n\n", tl.split_line);
	}
}

//写入测试失败的日志内容
void log_out_fail(FILE *p)
{
	if(p)
	{
		if(!tl.test_status)
		{
			log_out(p);
		}
	}
}

//关闭日志
void close_log()
{
	if(lpf)
	{
		fprintf(lpf, "\r\r\n\tcurrent test result : total of %d test , %d success and %d fail\r\r\n\r\r\n", \
			test_success+test_fail, test_success, test_fail);
		fclose(lpf);
		lpf = NULL;
	}
	if(flpf)
	{
		fclose(flpf);
		flpf = NULL;
	}
}

void doNothing()
{
	int i = 0;
}

