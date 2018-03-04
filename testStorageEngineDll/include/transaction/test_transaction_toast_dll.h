/**
* @file test_transaction_toast_dll.h
* @brief ����Toast�ı��ڻع�ʱ��ԭ����
* @author ������
* @date 2011-9-26 14:18:03
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANSACTION_TOAST_DLL_H
#define _TEST_TRANSACTION_TOAST_DLL_H
/************************************************************************** 
* @brief testToastTrans_InsertRollback_DLL 
* 1. ����в���һЩ���ݣ������д����ݣ�;
* 2. �ύ����
* 3. �ٲ�������һЩ����(����������)
* 4. �ع�
* 5  ���е�����
* @param[in] void 
* @return bool  ����ع��ɹ�������true,���򷵻�false
**************************************************************************/
bool testToastTrans_InsertRollback_DLL( void );

/************************************************************************** 
* @brief testToastTrans_UpdateRollback_DLL 
* 1. ����в���һЩ���ݣ������д����ݣ�;
* 2. �ύ����
* 3. Update
* 4. �ع�
* 5  �����е�����
* @return bool  ����ع��ɹ�������true,���򷵻�false
**************************************************************************/
bool testToastTrans_UpdateRollback_DLL(void);

/************************************************************************** 
* @brief testToastTrans_DeleteRollback_DLL 
* 1. ����в���һЩ���ݣ������д����ݣ�;
* 2. �ύ����
* 3. deleteһЩ����
* 4. �ع�
* 5  ���е�����
* Detailed description.
* @return bool  ����ع��ɹ�������true,���򷵻�false
**************************************************************************/
bool testToastTrans_DeleteRollback_DLL(void);
#endif 
