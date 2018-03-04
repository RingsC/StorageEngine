
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
    if(strAttr == std::string("bufferPoolSize"))
    {
        attrInfo.first = PARAM_TYPE(1);
        attrInfo.second = offsetof(storage_params,bufferPoolSize);
    }
	else	 if(strAttr == std::string("workMem"))
	{
	    attrInfo.first = PARAM_TYPE(1);
        attrInfo.second = offsetof(storage_params,work_mem);	
	}
	else	 if(strAttr == std::string("mainTenanceWorkMem"))
	{
	    attrInfo.first = PARAM_TYPE(1);
        attrInfo.second = offsetof(storage_params,maintenance_work_mem);	
	}	
    else     if(strAttr == std::string("xlogarchivemode"))
    {
        attrInfo.first = PARAM_TYPE(0);
        attrInfo.second = offsetof(storage_params,XLogArchiveMode);
    }
    else     if(strAttr == std::string("xlogarchivepath"))
    {
        attrInfo.first = PARAM_TYPE(5);
        attrInfo.second = offsetof(storage_params,XLogArchivePath);
    }
    else     if(strAttr == std::string("dorecovery"))
    {
        attrInfo.first = PARAM_TYPE(0);
        attrInfo.second = offsetof(storage_params,doRecovery);
    }
    else     if(strAttr == std::string("recoveryrestorepath"))
    {
        attrInfo.first = PARAM_TYPE(5);
        attrInfo.second = offsetof(storage_params,recoveryRestorePath);
    }
    else     if(strAttr == std::string("recoveryendcommand"))
    {
        attrInfo.first = PARAM_TYPE(5);
        attrInfo.second = offsetof(storage_params,recoveryEndCommand);
    }
    else     if(strAttr == std::string("archivecleanupcommand"))
    {
        attrInfo.first = PARAM_TYPE(5);
        attrInfo.second = offsetof(storage_params,archiveCleanupCommand);
    }
    else     if(strAttr == std::string("recoverytargetxid"))
    {
        attrInfo.first = PARAM_TYPE(1);
        attrInfo.second = offsetof(storage_params,recoveryTargetXid);
    }
    else     if(strAttr == std::string("recoverytargetname"))
    {
        attrInfo.first = PARAM_TYPE(5);
        attrInfo.second = offsetof(storage_params,recoveryTargetName);
    }
    else     if(strAttr == std::string("recoverytargettime"))
    {
        attrInfo.first = PARAM_TYPE(5);
        attrInfo.second = offsetof(storage_params,recoveryTargetTime);
    }
    else     if(strAttr == std::string("postportnumber"))
    {
        attrInfo.first = PARAM_TYPE(1);
        attrInfo.second = offsetof(storage_params,PostPortNumber);
    }
    else     if(strAttr == std::string("max_wal_senders"))
    {
        attrInfo.first = PARAM_TYPE(1);
        attrInfo.second = offsetof(storage_params,max_wal_senders);
    }
    else     if(strAttr == std::string("walsnddelay"))
    {
        attrInfo.first = PARAM_TYPE(1);
        attrInfo.second = offsetof(storage_params,WalSndDelay);
    }
    else     if(strAttr == std::string("replication_timeout"))
    {
        attrInfo.first = PARAM_TYPE(1);
        attrInfo.second = offsetof(storage_params,replication_timeout);
    }
    else     if(strAttr == std::string("masterenablestandby"))
    {
        attrInfo.first = PARAM_TYPE(1);
        attrInfo.second = offsetof(storage_params,masterEnableStandby);
    }
    else     if(strAttr == std::string("syncrepstandbynames"))
    {
        attrInfo.first = PARAM_TYPE(5);
        attrInfo.second = offsetof(storage_params,SyncRepStandbyNames);
    }
    else     if(strAttr == std::string("standbymode"))
    {
        attrInfo.first = PARAM_TYPE(0);
        attrInfo.second = offsetof(storage_params,StandbyMode);
    }
    else     if(strAttr == std::string("slaveenablehotstandby"))
    {
        attrInfo.first = PARAM_TYPE(0);
        attrInfo.second = offsetof(storage_params,slaveEnableHotStandby);
    }
    else     if(strAttr == std::string("primaryconninfo"))
    {
        attrInfo.first = PARAM_TYPE(5);
        attrInfo.second = offsetof(storage_params,PrimaryConnInfo);
    }
    else     if(strAttr == std::string("triggerfile"))
    {
        attrInfo.first = PARAM_TYPE(5);
        attrInfo.second = offsetof(storage_params,TriggerFile);
    }
    else     if(strAttr == std::string("fileincrement"))
    {
        attrInfo.first = PARAM_TYPE(3);
        attrInfo.second = offsetof(storage_params,fileIncrement);
    }
    else 
    {
        throw std::runtime_error(std::string("no such configure item"));
    }
    return attrInfo;
}
#endif

