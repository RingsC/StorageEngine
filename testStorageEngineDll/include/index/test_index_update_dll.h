/**
* @file test_index_update_dll.h
* @brief 测试index update的测试用例
* @author 李书淦
* @date 2011-9-23 9:58:05
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_INDEX_UPDATE_DLL_H
#define _TEST_INDEX_UPDATE_DLL_H
/************************************************************************** 
* @brief testIndexUpdate_SingleColumn_DLL 
* 插入单列的数据，update,然后检查结果是否正确
* Detailed description.
* @return bool  测试成功返回true,否则返回false.
**************************************************************************/
bool testIndexUpdate_SingleColumn_DLL();


/************************************************************************** 
* @brief testIndexUpdate_MultiColumn 
* 插入多列的数据，update,然后检查结果是否正确
* Detailed description.
* @return bool  测试成功返回true,否则返回false.
**************************************************************************/
bool testIndexUpdate_MultiColumn_DLL();

/************************************************************************** 
* @brief testIndexUpdate_InAnotherTrans_DLL 
* 插入一些数据到entry set中，commit后再update,然后检查结果是否正确
* Detailed description.
* @return bool  测试成功返回true,否则返回false.
**************************************************************************/
bool testIndexUpdate_InAnotherTrans_DLL();
#endif 
