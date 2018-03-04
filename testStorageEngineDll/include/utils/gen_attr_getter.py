#!/usr/bin/env python

import os
import sys
import re
type_dict = {
'bool':0
,'int':1
,'unsigned int':2
,'double':3
,'float':4
,'char':5
}

def main():
    _data = '''
/*This file is generated by gen_attr_getter.py accord Configs.h
 *if you changed the Configs.h, please run the gen_atttr_getter.py
 *to update this file
 */
#ifndef _ATTR_SETTER_HPP
#define _ATTR_SETTER_HPP
#include <string>
#include <stdexcept>
#include "Configs.h"
#include <boost/algorithm/string.hpp>
enum PARAM_TYPE
{
        EBool = 0,
        EInt,
        EUInt,
        EDouble,
        EFloat,
        EString
};
#define MAXPARAM 6

typedef std::pair<PARAM_TYPE,int> AttrInfo;
storage_params* GetStorageParam(std::string& strConfigFile);
inline AttrInfo GetAttrInfo(std::string& strAttr)
{
    AttrInfo attrInfo;
    boost::to_lower(strAttr);
    //std::cout<<strAttr<<std::endl;
'''
    f = open('../../../../../include/StorageEngine/Configs.h');
    commRe = re.compile(r'^\s*//.*')
    attrRe = re.compile(r'^\s*(\w+)\s*(\*?)\s*(\w+).*')

    begin = False
    for line in f:
	if not begin:
	    if -1 != line.find('{'):
		begin = True
	else:
	    if -1 != line.find('}'):
		break;

	    if not re.match(commRe,line):
		result = re.match(attrRe,line)
		if not result:
		    print line
		else:
		    result = result.groups()
		    #print result
		    _data += '    if(strAttr == std::string("' + result[2].lower() + '"))\n'
		    _data += '    {\n'
		    _data += '        attrInfo.first = PARAM_TYPE(' +  str(type_dict[result[0]]) + ');\n'
		    _data += '        attrInfo.second = offsetof(storage_params,' + result[2] +');\n'
		    _data += '    }\n'
                    _data += '    else '
	    else:
		print 'comment: '+ line

    _data += '''
    {
        throw std::runtime_error(std::string("no such configure item"));
    }
    return attrInfo;
}
#endif

'''
    f = open('attr_setter.h','w')
    f.write(_data)
    f.close()


if __name__ == '__main__':
    main()
