#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_NO_MAIN 
#include <malloc.h>
#include <boost/test/unit_test.hpp>
//#include <boost/test/impl/framework.ipp>
//#include <boost/test/impl/unit_test_main.ipp>
#include "teamcity/teamcity_boost.h"
#include <iostream>
#ifdef TESTFRAME
#include "TestFrame.h"
#endif

//using namespace Test;
// Register TeamCity log formatter
// It will be actived only under TeamCity build
// (detected by presence of TEAMCITY_PROJECT_NAME variable)
bool init_unit_test() {
	if (JetBrains::underTeamcity()) {
		boost::unit_test::unit_test_log.set_formatter(new JetBrains::TeamcityBoostLogFormatter());
		boost::unit_test::unit_test_log.set_threshold_level(boost::unit_test::log_successful_tests);
	}

	return true;
}
extern std::string g_strDataDir = "";

int BOOST_TEST_CALL_DECL
main(int argc, char* argv[]) {

	/************************************************************************/
	/* 为命令行添加--detect_memory_leaks=0参数以关闭boost报内存泄露的信息   */
	/************************************************************************/
	extern void save_arg(const int argc, char **argv);
	{
		if(argc >= 2)
		{
			save_arg(argc, argv);
			g_strDataDir = argv[1];
			argv[1] = (char*)malloc(strlen("--detect_memory_leaks=0") + 1);
			strcpy(argv[1], "--detect_memory_leaks=0");

			#ifdef TESTFRAME
			int result=  boost::unit_test::unit_test_main(&init_unit_test, argc, argv );
			
			TestFrame& app = TestFrame::Instance();
			//app->StartEngine();
			if (argc>=3 && (atoi(argv[2])>0))
				app.SetThreadNum(atoi(argv[2]));
			else
				app.SetThreadNum(20);
			app.Init();
			app.Run();

			//int result=  boost::unit_test::unit_test_main(&init_unit_test, argc, argv );
			app.Join();
			//app.StopEngine();
	
			//return boost::unit_test::unit_test_main(&init_unit_test, argc, argv );
			return result;
			#else
			return boost::unit_test::unit_test_main(&init_unit_test, argc, argv );
			#endif
		}
	}
	save_arg(argc, argv);
	return boost::unit_test::unit_test_main(&init_unit_test, argc, argv );
}

#include "teamcity/teamcity_messages.cpp"
#include "teamcity/teamcity_boost.cpp"