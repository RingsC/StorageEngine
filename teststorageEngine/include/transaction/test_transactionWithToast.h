/**
* @file test_transactionWithToast.h
* @brief ���Դ����ݵĲ��룬ɾ�������µĻع�
* @author ������
* @date 2011-9-20 14:06:33
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANSACTIONWITHTOAST_H
#define _TEST_TRANSACTIONWITHTOAST_H

/************************************************************************** 
* @brief testToastTrans_InsertRollback 
* 1. ����в���һЩ���ݣ������д����ݣ�;
* 2. �ύ����
* 3. �ٲ�������һЩ����(����������)
* 4. �ع�
* 5  ���е�����
* @param[in] void 
* @return bool  ����ع��ɹ�������true,���򷵻�false
**************************************************************************/
bool testToastTrans_InsertRollback( void );

/************************************************************************** 
* @brief testToastTrans_UpdateRollback 
* 1. ����в���һЩ���ݣ������д����ݣ�;
* 2. �ύ����
* 3. Update
* 4. �ع�
* 5  �����е�����
* @return bool  ����ع��ɹ�������true,���򷵻�false
**************************************************************************/
bool testToastTrans_UpdateRollback(void);

/************************************************************************** 
* @brief testToastTrans_DeleteRollback 
* 1. ����в���һЩ���ݣ������д����ݣ�;
* 2. �ύ����
* 3. deleteһЩ����
* 4. �ع�
* 5  ���е�����
* Detailed description.
* @return bool  ����ع��ɹ�������true,���򷵻�false
**************************************************************************/
bool testToastTrans_DeleteRollback(void);
#endif 
