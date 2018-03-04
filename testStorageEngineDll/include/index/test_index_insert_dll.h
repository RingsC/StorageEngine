/**
* @file test_index_insert_dll.h
* @brief 
* @author ������
* @date 2011-9-21 15:48:34
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_INDEX_INSERT_DLL_H
#define _TEST_INDEX_INSERT_DLL_H

/************************************************************************** 
* @brief testIndexInsert_SingleColumn 
* ������������������в���һЩ���ݣ���������Ƿ���ȷ
* Detailed description.
* @param[in] void 
* @return bool  ������Գɹ�����true,���򷵻�false
**************************************************************************/
bool testIndexInsert_SingleColumn( void );

/************************************************************************** 
* @brief testIndexInsert_SameScaKey 
* ������������������в���һЩ���ݣ���������Ƿ���ȷ(��ѯʱ����ͬ��ʹ��������ͬ��scakkey)
* Detailed description.
* @param[in] void 
* @return bool  ������Գɹ�����true,���򷵻�false
**************************************************************************/
bool testIndexInsert_SameScaKey( void );

/************************************************************************** 
* @brief testIndex_InAnotherTrans 
* ��һ�������н������������������һЩ���ݣ�������һ�������м�����Ƿ���ȷ
* Detailed description.
* @return bool  ������Գɹ�����true,���򷵻�false
**************************************************************************/
bool testIndex_InAnotherTrans(void);


/************************************************************************** 
* @brief testIndex_Sort 
* ����һ������������������һ����ֵ��ȵ�ֵ����鷵�ؽ���Ƿ��յڶ��ؼ�������
* Detailed description.
* @return bool  ������Գɹ�����true,���򷵻�false
**************************************************************************/
bool testIndex_Sort();

/************************************************************************** 
* @brief test_cluster_index
* tests cluster index functionalities, including insert key-value entries, 
* and then scan the index using various scan conditions; 
***************************************************************************/
bool test_cluster_index();
#endif 


