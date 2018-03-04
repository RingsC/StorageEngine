/**
* @file test_toast_dll.h
* @brief 
* @author 李书淦
* @date 2011-9-23 15:54:57
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TOAST_DLL_H
#define _TEST_TOAST_DLL_H

/************************************************************************** 
* @brief test_toast_insert 
* 向表中插入大数据，检查插入是否正确无误
* Detailed description.
* @return bool  
**************************************************************************/
bool test_toast_insert(void);

/************************************************************************** 
* @brief test_toast_update 
* 更新表中的大数据，检查更新是否成功
* Detailed description.
* @return bool  
**************************************************************************/
bool test_toast_update(void);

/************************************************************************** 
* @brief test_toast_delete 
* 删除表中的一些大数据，检查是否删除成功
* Detailed description.
* @return bool  
**************************************************************************/
bool test_toast_delete(void);

bool test_toast_vacuum(void);

#endif 
