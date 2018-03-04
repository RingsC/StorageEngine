/**
* @file test_dump_load.h
* @brief 
* @author 李书淦
* @date 2012-2-8 14:23:16
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_DUMP_LOAD_H
#define _TEST_DUMP_LOAD_H


/************************************************************************** 
* @brief test_dump_load 
* 
* Detailed description.
INTENT("1.创建EntrySet,插入一些数据;\n"
"2. dump到一个文件中test.dump\n"
"3. 删除entryset\n"
"4. 重新创建这个entryset\n"
"5. 从test.dump中load数据\n"
"6.查询插入结果是否正确.\n");
* @param[in] void 
* @return bool  
**************************************************************************/
bool test_dump_load( void );
#endif 
