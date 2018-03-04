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

			//������0��100ΪOid����relation
			int test_heap_create_by_oid_asc()
			{
			INTENT("������0��100ΪOid����relation");
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
			//�������Ҫ��PG�Լ���try-catch����catch��Ҫthrowһ���쳣
			//����PG��ʹ���Լ��Ĵ���ʽ�����ս���������
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
extern bool can_call_free;				//����pToFreeFunc������һ�ε���
extern char *current_file_name;		//��¼��ǰ�ļ�
extern unsigned int line_num;			//��¼��ǰ����������
extern unsigned int test_num;			//��¼test������
extern unsigned int test_success;	//��¼test�ɹ�������
extern unsigned int test_fail;		//��¼testʧ�ܵ�����
extern FILE *lpf;									//��־�ļ����
extern FILE *flpf;								//����ʧ�ܵ���־�ļ����
extern const char *log_dir;
extern const char *log_path;
extern const char *log_fail_path;
static std::string g_strNameOfTestCase;
//��־�ṹ
//��־�ļ�������Ŀ�ļ��µ�log_�ļ���
struct TestLog
{
	TestLog() : 
		split_line("==============================================================")
	{
		//do nothing
	}
	const char * const split_line;
	char test_function_path[1024];//���Ժ���λ��
	char test_time[64];						//����ִ��ʱ��
	char test_name[256];					//���Ժ�����
	bool test_status;							//����״̬
	char test_intent[2048];				//������ͼ
	char test_crash_site[1024];		//���Ա���·��
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
extern std::list<TestLog> list_fail_test;//��¼����ʧ�ܲ��Ե���Ϣ

extern void on_test_exit_at_fornt(OnTestExitFunc, void*);
extern void on_test_exit_at_tail(OnTestExitFunc, void*);
extern void run_test_exit();

void setFunc_(void (*p1)(void), void (*p2)(void));

//���ʧ�ܵĲ�����Ϣ
void insertFailTestInfo();

//��ӡ����ʧ�ܲ�����Ϣ
void printfFailTestInfo();

//��ʽ��TestLog��test_crash_site
void formatCrashSite(char *file_name, int line_num);

//��ʼ����־����
void initLog();

//��¼��������
void logToday();

//д����־����
void log_out(FILE *p);

//д�����ʧ�ܵ���־����
void log_out_fail(FILE *p);

//�ر���־
void close_log();

//�жϵ�ǰ�����Ƿ�����������ִ��
int runMultiProcess();

//���������ִ�еı���ļ�
void createMultiProcessFile();

//ɾ�������ִ�еı���ļ�
void deleteMultiProcessFile();

//����һ���µĽ���
void runNewProcess(char *func_name, int wait);

//��ȡ�����в������ж�Ҫִ�еĲ�������
int isExecuteTest(char *func_name);

//��¼�ڶ��������µĲ��Գɹ�����ʧ����
void logSFNum(int sta);

//ɾ������̲��Գɹ�����ʧ�ܵļ����ļ�
void deleteLogSFNum();

//��������̲��Գɹ�����ʧ�ܵļ����ļ�
void createLogSFNum();

//���²�������
void updateSF();

//��������ʧ�ܵĲ�����Ϣ
void insertProcessFailTestInfo();

void doNothing();


/* ��ȡ�����в��� */
std::string get_argv();
int get_argc();

/* ���������в��� */
bool run_test_parse(const char *func_name, const RUN_TYPE rt);
bool is_run_myself(const char *func_name);
void run_begin(const char *func_name);
void run_remove(const char *func_name);
void save_heap_id(const unsigned int heap_id, const char *func_name);
void save_heap_and_index(const unsigned int heap_id, const unsigned int index_id, const char *func_name);
void get_relation_info(const char *func_name, std::map<unsigned int, std::set<unsigned int> > &m_hi);
void check_for_release();

/* ���ò����Ƿ���� */
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

//��¼������ͼ
#define INTENT(intent) \
	strcpy(tl.test_intent, intent);

//��¼�ļ����к�
//��ÿһ�����ܳ����ڲ������ĺ�������ǰһ�е���
#define SAVE_INFO_FOR_DEBUG() \
	line_num = __LINE__ + 1;\
	current_file_name = __FILE__;\
	StorageEngineFixture::bEnded = false;

BOOST_GLOBAL_FIXTURE(StorageEngineFixture);
//���Կ�ʼ������Ϊ����֮�����ִ�еĺ�����
#define BEGIN_DEBUG(init_func) \
	BOOST_FIXTURE_TEST_SUITE(t_##init##_##func##_,EmptyFixture) 

//���Խ���������Ϊ�������֮��ִ�еĺ�����
#define END_DEBUG(free_func) \
	BOOST_AUTO_TEST_SUITE_END()

//ע��һ�����Ժ������ú������뷵��һ��int���ͣ�����Ϊ������
//�����REGIST_FUNC�������һ��BEGIN_DEBUG��END_DEBUG���Χ
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

//�����������е�������������
//�������need_wait_other��������ָ���������Ƿ���Ҫ�ȴ��´�������
//����ִ����ϲŻ������
//�ڶ���̲��������£�����ÿһ��(�����ǵ����̻��Ƕ����)��������
//Ӧ���ṩ�Լ���start_engine����Ӧ��stop_engine��ͬʱ��һ��������
//���д������ɸ�����̲���������ʱ����BEGIN_DEBUG��END_DEBUG��
//��Ӧ���ṩstart_engine��stop_engine��Ӧ��ΪNULL(������0)
//����ڶ���̲���������ʱ����������û�н��е����������c��
//�µ�process_fail.txt��sf_sta.txt��sta�ļ��Ƿ���ڣ�������ھ�ɾ��
//��������ǰһ�����׼�û�������������µ�
//����̵�beginTest��endTest�����������̵���÷ֿ�д
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

//�о߲��ԡ�
//every_init_funcΪÿ�β�������ִ�п�ʼǰ���õĺ�����
//every_free_funcΪÿ�β�������ִ�н�������õĺ�����
//init_funcΪȫ�ֲ��Կ�ʼǰ���õĺ�����
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
*����ΪXXX_YYY��ʽ�ĺ�
*XXX���������ͣ��ֱ�ΪCHECK��WARN��REQUIRE
*CHECKΪ��鼶������Ϊfalse�����������ӣ����Լ���
*WARNΪ���漶�𣬲���������Ҳ���жϳ���
*REQUIREΪ��߼�������Ϊfalse����ֹ�������еĲ��ԣ��������
*YYY�����ͷֱ�ΪEQUAL��GE/GT/LT/LE/NE��MESSAGE
*EQUAL���ڱȽ����������Ƿ����
*GE/GT/LT/LE/NE�ֱ��Ӧ�ڲ�����">=",">","<","<=","!="
*MESSAGE������Ϊfalseʱ���������Ϣ
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

//CHECK_BOOL���ڼ򵥵��ж�����������
#define CHECK_BOOL(_bool) BOOST_CHECK(_bool)

#endif //TEST_FRAM