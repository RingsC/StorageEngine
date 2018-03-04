//#include <vld.h>
#define BOOST_TEST_ALTERNATIVE_INIT_API
#define BOOST_TEST_NO_MAIN 
//#define CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
#include <malloc.h>
#include <boost/test/unit_test.hpp>
//#include <boost/test/impl/framework.ipp>
//#include <boost/test/impl/unit_test_main.ipp>
#include "teamcity/teamcity_boost.h"
#include <iostream>

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
extern std::string g_strDataDir;

int BOOST_TEST_CALL_DECL
main(int argc, char* argv[]) {

	//_CrtDumpMemoryLeaks();
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
	//_CrtSetBreakAlloc(588);
	/************************************************************************/
	/* Ϊ����������--detect_memory_leaks=0�����Թر�boost���ڴ�й¶����Ϣ   */
	/************************************************************************/
	extern void save_arg(const int argc, char **argv);

	{
		if(argc >= 2)
		{
			save_arg(argc, argv);
			g_strDataDir = argv[1];
			argv[1] = (char*)malloc(strlen("--detect_memory_leaks=0") + 1);
			strcpy(argv[1], "--detect_memory_leaks=0");

			return boost::unit_test::unit_test_main(&init_unit_test, argc, argv );
		}
	}
	save_arg(argc, argv);
	return boost::unit_test::unit_test_main(&init_unit_test, argc, argv );
}

#include "teamcity/teamcity_messages.cpp"
#include "teamcity/teamcity_boost.cpp"