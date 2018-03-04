#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/xpressive/xpressive_dynamic.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <Configs.h>
#include "attr_setter.h"

template<PARAM_TYPE t> struct ParamType;
#define MAKE_TYPE(t,rt)	\
	template<>	\
struct ParamType<t>\
{\
	typedef rt type;\
}

MAKE_TYPE(EBool,bool);
MAKE_TYPE(EInt,int);
MAKE_TYPE(EUInt,unsigned int);
MAKE_TYPE(EDouble,double);
MAKE_TYPE(EFloat,float);
MAKE_TYPE(EString,char*);

typedef std::pair<PARAM_TYPE,int> AttrInfo;

template<PARAM_TYPE t>
struct AttrSetter
{
	typedef typename ParamType<t>::type type;
	static void set(storage_params& params,long nOffset,const std::string& svalue)
	{
		std::stringstream s(svalue);
		type& value = *((type*)((char*)(&params) + nOffset));
		s>>value;
	}
};

template<>
struct AttrSetter<EString>
{
	typedef char* type;
	static void set(storage_params& params,long nOffset,const std::string& svalue)
	{

		type* value = (type*)(((char*)(&params)) + nOffset);
		*value = new char[svalue.length() + 1];
		std::memcpy(*value,svalue.c_str(),svalue.length());
		(*value)[svalue.length()] = '\0';
	}
};

storage_params* GetStorageParam(std::string& strConfigFile)
{
	std::ifstream configFile(strConfigFile.c_str());
	if (!configFile.is_open())
	{
		std::cout<<"Error: Can not open " + strConfigFile + " for read!"<<std::endl;
		return NULL;
	}
	static storage_params params;

	using namespace boost::xpressive;
	std::string strLine;
	std::memset(&params,0,sizeof(storage_params));

	while (getline(configFile,strLine))
	{
		boost::trim(strLine);
		if(strLine.length() > 0)
		{
			sregex commRegex = sregex::compile("^\\s*#.*");
			sregex valueRegex = sregex::compile("^\\s*(\\w+)\\s*=\\s*(.+)");

			smatch commWhat,valueWhat;
			if (!regex_match(strLine.begin(),strLine.end(),commWhat,commRegex))
			{
				if (regex_search(strLine.begin(),strLine.end(),valueWhat,valueRegex))
				{
					try
					{
						std::string strAttrName = valueWhat[1];
						AttrInfo attrInfo = GetAttrInfo(strAttrName);

						switch (attrInfo.first)
						{
#define REPEAT_CASE(z,n,data)	\
case n:\
	AttrSetter<PARAM_TYPE(n)>::set(params,attrInfo.second,valueWhat[2]);\
	break;
							BOOST_PP_REPEAT(MAXPARAM,REPEAT_CASE,~)
#undef REPEAT_CASE
default:
	break;
						}
					}
					catch(...)
					{
						continue;
					}
				}
				else
				{
					std::cout<<"Syntax Error: "<<strLine<<std::endl;
				}
			} //if not comments
		}//if(strLine.length() > 0)
	}
	return &params;
}


