/**
* @file test_index_update_dll.h
* @brief ����index update�Ĳ�������
* @author ������
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
* ���뵥�е����ݣ�update,Ȼ�������Ƿ���ȷ
* Detailed description.
* @return bool  ���Գɹ�����true,���򷵻�false.
**************************************************************************/
bool testIndexUpdate_SingleColumn_DLL();


/************************************************************************** 
* @brief testIndexUpdate_MultiColumn 
* ������е����ݣ�update,Ȼ�������Ƿ���ȷ
* Detailed description.
* @return bool  ���Գɹ�����true,���򷵻�false.
**************************************************************************/
bool testIndexUpdate_MultiColumn_DLL();

/************************************************************************** 
* @brief testIndexUpdate_InAnotherTrans_DLL 
* ����һЩ���ݵ�entry set�У�commit����update,Ȼ�������Ƿ���ȷ
* Detailed description.
* @return bool  ���Գɹ�����true,���򷵻�false.
**************************************************************************/
bool testIndexUpdate_InAnotherTrans_DLL();
#endif 
