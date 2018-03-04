/**
* @file toast_test_utils.cpp
* @brief help function的实现
* @author 李书淦
* @date 2011-9-15 13:55:28
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#include <ctime>
#include <fstream>
#include <boost/smart_ptr.hpp>
#include "utils/toast_test_utils.h"
#include "interface/FDPGAdapter.h"
bool readfromfile = true;
void RandomGenString(std::string& strLine,size_t nLen)
{
	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::ifstream inFile(szFileName);
		if (inFile.is_open())
		{
			inFile.seekg(0,std::ios::end);
			size_t length = inFile.tellg();
			inFile.seekg(0,std::ios::beg);
			boost::shared_ptr<char> psz(new char[length]);
			inFile.read(psz.get(),length);
			strLine.append(psz.get(),length);
			return;
		}
	}

	std::string s;
	srand((unsigned)time( NULL ));
	size_t nGenerated = 0;
	for (nGenerated = 0;nGenerated < 10;++nGenerated)
	{
		char c = 0;
		s += c;
	}
	for (nGenerated = 10; nGenerated < nLen; ++nGenerated)
	{
		char c = (rand()) % 26;
		s += 'a' + c;
	}
	char c = 0;
	s += c;
	strLine.append(s);

	if (readfromfile)
	{
		char szFileName[32];
		sprintf(szFileName,"%d",nLen);
		std::fstream outFile(szFileName,std::ios_base::out);
		if(outFile.is_open())
		{
			outFile<<s;
		}
	}

}

static Oid test_toast_get_tmp_heap_id(void)
{
	return GetTempObjectId();
}

static Oid test_toast_get_tmp_index_id(void)
{
	return GetTempObjectId();
}

Oid test_toast_get_heap_id(bool temp)
{
	if(!temp){
		return OIDGenerator::instance().GetHeapID();
	}else{
		return test_toast_get_tmp_heap_id();
	}
}

Oid test_toast_get_index_id(bool temp)
{
	if(!temp){
		return OIDGenerator::instance().GetIndexID();
	}else{
		return test_toast_get_tmp_index_id();
	}
}

