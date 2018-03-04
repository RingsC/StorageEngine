/*
*BEGIN_DEBUG(init function name) / BEGIN_FIXTURE_DEBUG(init func name, dest func name, global init func name)
*REGIST_FUNC(before function name, function name, after function name)
*REGIST_FUNC(before function name, function name, after function name)
*...
*END_DEBUG(free function name)

*case following:
			.........
			void start_engine_()
			{
			const char* pg_data = "C:\\storageEngine\\data44";
			start_engine(pg_data);
			}

			void stop_engine_()
			{
			stop_engine(1);
			}

			//测试由0至100为Oid创建relation
			int test_heap_create_by_oid_asc()
			{
			INTENT("测试由0至100为Oid创建relation");
			AbortOutOfAnyTransaction();
			StartTransactionCommand();
			BeginTransactionBlock();
			CommitTransactionCommand();
			Oid relid;
			Relation rel;
			Oid table_space = MyDatabaseTableSpace;
			char kind = RELKIND_RELATION;
			for(unsigned int i = 0; i <= 100; ++i)
			{
			//这里必须要用PG自己的try-catch，且catch块要throw一个异常
			//否则PG将使用自己的处理方式，最终将结束程序
			PG_TRY();
			{
			relid = i;
			SAVE_INFO_FOR_DEBUG();
			rel = fxdb_heap_create(table_space, relid, kind);
			}
			PG_CATCH();
			{
			throw;
			}
			PG_END_TRY();
			}
			return 1;
			}

			BEGIN_DEBUG(start_engine_)
			REGIST_FUNC(0, test_heap_create_by_oid_asc, 0)
			END_DEBUG(stop_engine_)
			.........
*/
//#define BOOST_TEST_MODULE TEST_POSTGRES
#ifndef TEST_FRAM 
#define TEST_FRAM

#include <time.h>

#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif //WIN32

#include <stdio.h>
#include <iostream>
#include <string>
#include <bitset>
#include <map>
#include <set>
#include <boost/timer.hpp>
#include <boost/test/execution_monitor.hpp>
#include <boost/test/unit_test.hpp>
#ifndef WIN32
#include "sys/time.h"
#endif
typedef void(*OnTestExitFunc)(void*);
extern void (*pToInitFunc)(void);
extern void (*pToFreeFunc)(void);
extern bool can_call_free;				//控制pToFreeFunc跳过第一次调用
extern char *current_file_name;		//记录当前文件
extern unsigned int line_num;			//记录当前函数所在行
extern unsigned int test_num;			//记录test的总数
extern unsigned int test_success;	//记录test成功的总数
extern unsigned int test_fail;		//记录test失败的总数
extern FILE *lpf;									//日志文件句柄
extern FILE *flpf;								//测试失败的日志文件句柄
extern const char *log_dir;
extern const char *log_path;
extern const char *log_fail_path;
static std::string g_strNameOfTestCase;
//日志结构
//日志文件存于项目文件下的log_文件夹
struct TestLog
{
	TestLog() : 
		split_line("==============================================================")
	{
		//do nothing
	}
	const char * const split_line;
	char test_function_path[1024];//测试函数位置
	char test_time[64];						//测试执行时间
	char test_name[256];					//测试函数名
	bool test_status;							//测试状态
	char test_intent[2048];				//测试意图
	char test_crash_site[1024];		//测试崩溃路径
private:
	TestLog& operator=(const TestLog&);
};

typedef enum
{
	TransPersistence,
	TransExit,
	LTTest
} RUN_TYPE;

extern TestLog tl;
extern std::list<TestLog> list_fail_test;//记录所有失败测试的信息

extern void on_test_exit_at_fornt(OnTestExitFunc, void*);
extern void on_test_exit_at_tail(OnTestExitFunc, void*);
extern void run_test_exit();

void setFunc_(void (*p1)(void), void (*p2)(void));

//添加失败的测试信息
void insertFailTestInfo();

//打印所有失败测试信息
void printfFailTestInfo();

//格式化TestLog的test_crash_site
void formatCrashSite(char *file_name, int line_num);

//初始化日志环境
void initLog();

//记录今天日期
void logToday();

//写入日志内容
void log_out(FILE *p);

//写入测试失败的日志内容
void log_out_fail(FILE *p);

//关闭日志
void close_log();

//判断当前进程是否脱离主进程执行
int runMultiProcess();

//创建多进程执行的标记文件
void createMultiProcessFile();

//删除多进程执行的标记文件
void deleteMultiProcessFile();

//启动一个新的进程
void runNewProcess(char *func_name, int wait);

//获取命令行参数，判断要执行的测试用例
int isExecuteTest(char *func_name);

//记录在多进程情况下的测试成功或者失败数
void logSFNum(int sta);

//删除多进程测试成功或者失败的计数文件
void deleteLogSFNum();

//创建多进程测试成功或者失败的计数文件
void createLogSFNum();

//更新测试数据
void updateSF();

//多进程添加失败的测试信息
void insertProcessFailTestInfo();

void doNothing();


/* 获取命令行参数 */
std::string get_argv();
int get_argc();

/* 分析命令行参数 */
bool run_test_parse(const char *func_name, const RUN_TYPE rt);
bool is_run_myself(const char *func_name);
void run_begin(const char *func_name);
void run_remove(const char *func_name);
void save_heap_id(const unsigned int heap_id, const char *func_name);
void save_heap_and_index(const unsigned int heap_id, const unsigned int index_id, const char *func_name);
void get_relation_info(const char *func_name, std::map<unsigned int, std::set<unsigned int> > &m_hi);
void check_for_release();

/* 检查该测例是否结束 */
bool is_time_out();
std::string get_my_arg(char *func_name);

#define SHUTDOWN_TEST_STEP_1(rt) \
	if(!run_test_parse(__FUNCTION__, rt)) \
	{ \
		return true; \
	} \
	if(!is_run_myself(__FUNCTION__)) { \
		run_begin(__FUNCTION__);

#define SHUTDOWN_TEST_STEP_2() \
	}else { \

#define CLEAN_SHUTDOWN_TEST() \
	run_remove(__FUNCTION__);

#define END_SHUTDOWN_TEST() \
		run_remove(__FUNCTION__); \
	}

#define SAVE_HEAP_ID_FOR_NEXT(heapid) \
	save_heap_id(heapid, __FUNCTION__);

#define SAVE_HEAP_AND_INDEX_FOR_NEXT(heapid, indexid) \
	save_heap_and_index(heapid, indexid, __FUNCTION__);

typedef std::map<unsigned int, std::set<unsigned int> > ShutdownMap;

#define GET_RELATION_INFO(map) \
	get_relation_info(__FUNCTION__, map);

#ifndef DONOTING
#define DONOTING doNothing
#endif

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
		std::cout<<g_strNameOfTestCase<<" cost "<<m_time.elapsed()<<std::endl;
#else
		const int RATE = 1000000L;
		gettimeofday(&endTime,NULL);
		double microsec = (endTime.tv_sec - startTime.tv_sec) * RATE + (endTime.tv_usec - startTime.tv_usec);
		std::cout<<g_strNameOfTestCase<<" cost "<<(microsec / RATE)<<" seconds"<<std::endl;
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
static TimerFixture *g_pTimer = NULL;
struct StorageEngineFixture
{
	StorageEngineFixture()
	{
		static bool bStarted = false;
		if (!bStarted)
		{
			g_pTimer = new TimerFixture();
			extern int start_engine_();

			initLog(); 
			if(!runMultiProcess()) { 
				deleteMultiProcessFile(); 
				deleteLogSFNum(); 
				list_fail_test.clear(); 
			} 

			start_engine_();
			std::cout<<"start engine"<<std::endl;	
			bStarted = true;
		}

	}
	~StorageEngineFixture()
	{
		if (!bEnded)
		{
			updateSF(); 
			std::cout << "n Total of " << test_num << " test : " 
				<< test_success << " success , " << test_fail << " fail.nn"; 
			printfFailTestInfo(); 
			deleteMultiProcessFile(); 
			deleteLogSFNum(); 
			if(!runMultiProcess()) 
			{ 
				close_log(); 
				//std::cout << "---- call stop engine ---- n"; 
			} 
			extern int stop_engine_();
			stop_engine_();
			g_strNameOfTestCase.clear();
			g_strNameOfTestCase += "\nTotally ";
			delete g_pTimer;
			bEnded = true;
		}}

	static bool bEnded;
};

struct EmptyFixture{};

struct Fixture
{
	Fixture()
	{
		if(pToInitFunc != NULL)
		{
			(*pToInitFunc)();
		}
	}
	~Fixture()
	{
		if(pToFreeFunc != NULL && can_call_free == true)
		{
			(*pToFreeFunc)();
		}else
		{
			can_call_free = true;
		}
	}
};

//记录测试意图
#define INTENT(intent) \
	strcpy(tl.test_intent, intent);

//记录文件、行号
//在每一个可能出现内部崩溃的函数调用前一行调用
#define SAVE_INFO_FOR_DEBUG() \
	line_num = __LINE__ + 1;\
	current_file_name = __FILE__;\
	StorageEngineFixture::bEnded = false;

BOOST_GLOBAL_FIXTURE(StorageEngineFixture);
//调试开始，参数为调试之间必须执行的函数名
#define BEGIN_DEBUG(init_func) \
	BOOST_FIXTURE_TEST_SUITE(t_##init##_##func##_,EmptyFixture) 

//调试结束，参数为调试完毕之后执行的函数名
#define END_DEBUG(free_func) \
	BOOST_AUTO_TEST_SUITE_END()

//注册一个测试函数，该函数必须返回一个int类型，参数为函数名
//任意个REGIST_FUNC宏必须由一对BEGIN_DEBUG、END_DEBUG宏包围
#define REGIST_FUNC(before_func, funcname, after_func) \
	BOOST_FIXTURE_TEST_CASE(funcname##_,TimerFixture) \
	{ \
			bool isRunArterFunc = false; \
	    g_strNameOfTestCase.clear();\
		g_strNameOfTestCase += #funcname;\
		if(!runMultiProcess()) { \
			check_for_release(); \
			++test_num; \
			memset(tl.test_crash_site, 0, sizeof(tl.test_crash_site)); \
			memset(tl.test_intent, 0, sizeof(tl.test_intent)); \
			strcpy(tl.test_name, #funcname); \
			strcpy(tl.test_function_path, __FILE__); \
			time_t t = time(0); \
			strftime(tl.test_time, sizeof(tl.test_time), "%Y-%m-%d %H:%M:%S", localtime(&t)); \
			std::cout << "---------------------- " << #funcname << " running ----------------------\n"; \
			boost::execution_monitor em; \
			try { \
			if(before_func != NULL) \
				before_func(); \
			int funcname##_return = em.execute(boost::unit_test::callback0<bool>(funcname)); \
			BOOST_CHECK(funcname##_return); \
			if(funcname##_return) \
			{ \
				++test_success; \
				tl.test_status = true; \
			} \
			else { \
				tl.test_status = false; \
				++test_fail; \
				strcpy(tl.test_crash_site, "return false."); \
			} \
			if(funcname##_return) { \
				std::cout << "\n---------------------- test success! ----------------------\n\n"; \
			} \
			else {\
				insertFailTestInfo(); \
				std::cout << "\n---------------------- test fail! ----------------------\n\n"; \
			} \
			log_out(lpf); \
			log_out_fail(flpf); \
			}catch(boost::execution_exception &e) \
			{	\
				++test_fail; \
				tl.test_status = false; \
				formatCrashSite(current_file_name, line_num); \
				insertFailTestInfo(); \
				log_out(lpf); \
				log_out_fail(flpf); \
				std::cout << current_file_name << "::\n"; \
				std::cout << "<-- fail to call " << #funcname << ",Crash on line:" << line_num << " -->\n"; \
				std::cout << e.what().begin(); \
				std::cout << "\n---------------------- test fail! ----------------------\n\n"; \
				if(after_func != NULL) { \
					isRunArterFunc = true; \
					after_func(); \
				} \
				BOOST_CHECK(false); \
			}	\
			if(!isRunArterFunc && after_func != NULL) \
				after_func(); \
		} \
	}

//创建进程运行单个测试用例：
//多出来的need_wait_other参数用于指定主进程是否需要等待新创建的子
//进程执行完毕才会继续。
//在多进程测试用例下，对于每一个(无论是单进程还是多进程)测试用例
//应该提供自己的start_engine和相应的stop_engine，同时当一个测试套
//件中存在若干个多进程测试用例的时候，其BEGIN_DEBUG和END_DEBUG宏
//不应该提供start_engine和stop_engine，应该为NULL(不能是0)
//如果在多进程测试启动的时候遇到测试没有进行的情况，请检查c盘
//下的process_fail.txt、sf_sta.txt和sta文件是否存在，如果存在就删除
//这是由于前一测试套件没有正常结束导致的
//多进程的beginTest和endTest函数跟单进程的最好分开写
#define MULTI_REGIST_FUNC(before_func, funcname, after_func, need_wait_other) \
	BOOST_AUTO_TEST_CASE(funcname##_) \
	{ \
		++test_num; \
		createLogSFNum(); \
		if(!runMultiProcess()) { \
			createMultiProcessFile(); \
			runNewProcess(#funcname, need_wait_other); \
		} \
		if(isExecuteTest(#funcname)) { \
			memset(tl.test_crash_site, 0, sizeof(tl.test_crash_site)); \
			memset(tl.test_intent, 0, sizeof(tl.test_intent)); \
			strcpy(tl.test_name, #funcname); \
			strcpy(tl.test_function_path, __FILE__); \
			time_t t = time(0); \
			strftime(tl.test_time, sizeof(tl.test_time), "%Y-%m-%d %H:%M:%S", localtime(&t)); \
			std::cout << "---------------------- " << #funcname << " running ----------------------\n"; \
			boost::execution_monitor em; \
			try { \
			if(before_func != NULL) \
				before_func(); \
			int funcname##_return = em.execute(boost::unit_test::callback0<bool>(funcname)); \
			BOOST_CHECK(funcname##_return); \
			if(funcname##_return) \
			{ \
				getchar(); \
				logSFNum(1); \
				tl.test_status = true; \
			} \
			else { \
				tl.test_status = false; \
				logSFNum(0); \
				formatCrashSite(current_file_name, line_num); \
				strcpy(tl.test_crash_site, "return false."); \
			} \
			if(funcname##_return) { \
				std::cout << "\n---------------------- test success! ----------------------\n\n"; \
			}else { \
				std::cout << "\n---------------------- test fail! ----------------------\n\n"; \
				insertProcessFailTestInfo(); \
			} \
			log_out(lpf); \
			log_out_fail(flpf); \
			}catch(boost::execution_exception &e) \
			{	\
				logSFNum(0); \
				tl.test_status = false; \
				formatCrashSite(current_file_name, line_num); \
				log_out_fail(flpf); \
				log_out(lpf); \
				insertProcessFailTestInfo(); \
				std::cout << current_file_name << "::\n"; \
				std::cout << "<-- fail to call " << #funcname << ",Crash on line:" << line_num << " -->\n"; \
				std::cout << e.what().begin(); \
				std::cout << "\n---------------------- test fail! ----------------------\n\n"; \
			}	\
			deleteMultiProcessFile(); \
			deleteMultiProcessFile(); \
			if(after_func != NULL) \
				after_func(); \
		} \
	}

//夹具测试。
//every_init_func为每次测试用例执行开始前调用的函数名
//every_free_func为每次测试用例执行结束后调用的函数名
//init_func为全局测试开始前调用的函数名
#define BEGIN_FIXTURE_DEBUG(every_init_func, every_free_func, init_func) \
	BOOST_FIXTURE_TEST_SUITE(Fixture_, Fixture) \
	BOOST_AUTO_TEST_CASE(every_init_func##_##every_free_func##_) \
	{ \
		if(!runMultiProcess()) { \
			initLog(); \
			list_fail_test.clear(); \
			init_func(); \
			std::cout << "---- call " << #init_func << "---- \n"; \
			setFunc_(every_init_func, every_free_func); \
		} \
	}

/*
*以下为XXX_YYY形式的宏
*XXX有三个类型，分别为CHECK、WARN、REQUIRE
*CHECK为检查级别，条件为false则错误计数增加，测试继续
*WARN为警告级别，不做计数，也不中断程序
*REQUIRE为最高级别，条件为false将终止往后所有的测试，程序结束
*YYY的类型分别为EQUAL、GE/GT/LT/LE/NE、MESSAGE
*EQUAL用于比较两个参数是否相等
*GE/GT/LT/LE/NE分别对应于操作符">=",">","<","<=","!="
*MESSAGE当条件为false时输出给定信息
*/
#define CHECK_EQUAL(l, r) BOOST_CHECK_EQUAL(l, r)
#define WARN_EQUAL(l, r) BOOST_WARN_EQUAL(l, r)
#define REQUIRE_EQUAL(l, r) BOOST_REQUIRE_EQUAL(l, r)

#define CHECK_GE(l, r) BOOST_CHECK_GE(l, r)
#define WARN_GE(l, r) BOOST_WARN_GE(l, r)
#define REQUIRE_GE(l, r) BOOST_REQUIRE_GE(l, r)

#define CHECK_GT(l, r) BOOST_CHECK_GT(l, r)
#define WARN_GT(l, r) BOOST_WARN_GT(l, r)
#define REQUIRE_GT(l, r) BOOST_REQUIRE_GT(l, r)

#define CHECK_LT(l, r) BOOST_CHECK_LT(l, r)
#define WARN_LT(l, r) BOOST_WARN_LT(l, r)
#define REQUIRE_LT(l, r) BOOST_REQUIRE_LT(l, r)

#define CHECK_LE(l, r) BOOST_CHECK_LE(l, r)
#define WARN_LE(l, r) BOOST_WARN_LE(l, r)
#define REQUIRE_LE(l, r) BOOST_REQUIRE_LE(l, r)

#define CHECK_NE(l, r) BOOST_CHECK_NE(l, r)
#define WARN_NE(l, r) BOOST_WARN_NE(l, r)
#define REQUIRE_NE(l, r) BOOST_REQUIRE_NE(l, r)

#define CHECK_MESSAGE(expr, r) BOOST_CHECK_MESSAGE(expr, r)
#define WARN_MESSAGE(expr, r) BOOST_WARN_MESSAGE(expr, r)
#define REQUIRE_MESSAGE(expr, r) BOOST_REQUIRE_MESSAGE(expr, r)

//CHECK_BOOL用于简单的判断条件并计数
#define CHECK_BOOL(_bool) BOOST_CHECK(_bool)

#endif //TEST_FRAM