/**
* @file test_trans_toast_index.h
* @brief ����������toast�����Ͻ���index,Ȼ��rollback�Ĳ�������
* @author ������
* @date 2011-9-20 16:33:01
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANS_TOAST_INDEX_H
#define _TEST_TRANS_TOAST_INDEX_H


/************************************************************************** 
* @brief test_toastIndex_InsertRollabck 
* ��taost�����ϴ������������Բ���ع�������
* Detailed description.
* @return bool  ������Գɹ�����true,���򷵻�false
**************************************************************************/
bool test_toastIndex_InsertRollabck(void);


/************************************************************************** 
* @brief test_toastIndex_UpdateRollback 
* ��taost�����ϴ������������Ը��»ع�������
* Detailed description.
* @return bool  ������Գɹ�����true,���򷵻�false
**************************************************************************/
bool test_toastIndex_UpdateRollback(void);


/************************************************************************** 
* @brief test_toastIndex_DeleteRollback 
* ��taost�����ϴ�������������ɾ���ع�������
* Detailed description.
* @return bool  ������Գɹ�����true,���򷵻�false
**************************************************************************/
bool test_toastIndex_DeleteRollback(void);
#endif 
