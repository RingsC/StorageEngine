/**
* @file test_trans_multi_index_update.h
* @brief 测试多索引的update回滚的测试用例
* @author 李书淦
* @date 2011-9-27 15:50:16
* @version 1.0
* @copyright: founder.com
* @email: lisg@founder.com 
* All rights reserved.
*/
#ifndef _TEST_TRANS_MULTI_INDEX_UPDATE_H
#define _TEST_TRANS_MULTI_INDEX_UPDATE_H
/************************************************************************** 
* @brief test_trans_multi_index 
* 在表上创建多个索引，测试更新后回滚的情况下，事务是否是保持原子性的(表的各个属性是固定长度的)
* Detailed description.
* @param[in] void 
* @return bool  成功返回true,否则返回false
**************************************************************************/
bool test_trans_multi_index( void );

/************************************************************************** 
* @brief test_trans_multi_index_nullvalue 
*在表上创建多个索引，测试更新后回滚的情况下，事务是否是保持原子性的(表的各个属性是长度不等，所以index可能有null值)
* Detailed description.
* @param[in] void 
* @return bool  成功返回true,否则返回false
**************************************************************************/
bool test_trans_multi_index_nullvalue( void );
#endif 
