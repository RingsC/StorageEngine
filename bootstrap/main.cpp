#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <ctime>
#include <iomanip>
#include "StorageEngine.h"
#include "StorageEngineException.h"
#include "utils/utils_dll.h"
using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::setw;
using namespace FounderXDB::StorageEngineNS;


int main(int argc,char** argv)
{
	try
	{
	    StorageEngine* pStorageEngine = StorageEngine::getStorageEngine();//得到一个storageEngine的实例
    	pStorageEngine->bootstrap(argv[1]);  // first then end main and comment this code now
	}
	catch (StorageEngineException& ex)
	{
		std::cout<<ex.getErrorNo()<<ex.getErrorMsg()<<std::endl;
	}

	return 1;
}
