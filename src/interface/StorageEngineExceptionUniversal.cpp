#include <string>
#include "interface/StorageEngineExceptionUniversal.h"
#include "interface/FDPGAdapter.h"
#include "postgres.h"
#include "utils/elog.h"

using std::string;
namespace FounderXDB{
namespace StorageEngineNS {
StorageEngineExceptionUniversal::StorageEngineExceptionUniversal()
:StorageEngineException(0)
{  
    char* errMsg = get_errno_errmsg(m_errorCode);
	append_info(errMsg);
    FlushErrorState();
}

StorageEngineExceptionUniversal::StorageEngineExceptionUniversal(int err, std::string errmsg)
:StorageEngineException(err,errmsg.c_str())
{

}

StorageEngineExceptionUniversal::StorageEngineExceptionUniversal(int err, const char* errmsg)
:StorageEngineException(err,errmsg)
{

}

StorageEngineExceptionUniversal::~StorageEngineExceptionUniversal()throw()
{
    
}
int StorageEngineExceptionUniversal::getErrorNo() const
{
    return getErrorCode();
}

const char* StorageEngineExceptionUniversal::getErrorMsg() const
{
    return const_cast<StorageEngineExceptionUniversal*>(this)->what();
}


void ThrowException( void )
{
	int errCode = 0;
    int level = get_exception_level();
    char* errMsg = get_errno_errmsg(errCode);
	std::string strMsg(errMsg);
	
	FlushErrorState();
	if (ERROR == level)
	{
        switch(errCode)
        {
        case ERRCODE_T_R_DEADLOCK_DETECTED:
            throw DeadLockException();
        case ERRCODE_T_R_SERIALIZATION_FAILURE:
            // FDPG_Transaction::fd_AbortCurrentTransaction();
            throw SerializationFailure();
		case ERRCODE_OBJECT_IN_USE:
			throw ObjectInUseException();
		case ERRCODE_ACCESS_OBJECT_NOT_EXIST:
			throw ObjectNotExistException();
		case ERRCODE_TUPLE_UPDATE_CONCURRENTLY:
			throw TupleUpdateConcurrent();
		case ERRCODE_DUPLICATE_OBJECT:
			throw ObjectAlreadyExist();
		case ERRCODE_XLOG_MAKE_NEW_WAL_DURING_RECOVERY:
		case ERRCODE_READ_ONLY_SQL_TRANSACTION:
			throw StandbyUpdateException(strMsg.c_str());
		case ERRCODE_REPHA_TWO_MASTERS:
			throw TwoMasterException();
		case ERRCODE_REPHA_GROUP_CREATOR_EXIST:
			throw HAGroupCreatorExistException(strMsg.c_str());
		case ERRCODE_REPHA_ERROR:
			throw ReplicationInitException(strMsg.c_str());
		case ERRCODE_UNIQUE_VIOLATION:
			throw UniqueViolationException(strMsg.c_str());
		case ERRCODE_OUT_OF_MEMORY:
			throw OutOfMemoryException();
		case ERRCODE_NOT_IN_TRANSACTION:
			throw NotInTransactionException();
		case ERRCODE_PROGRAM_LIMIT_EXCEEDED:
			throw ProgramLimitExceededException();
			
		case ERRCODE_RECOVERY_TARGET_TIMELINE_DISMATCH:
		case ERRCODE_RECOVERY_TARGET_XLOG_HAD_REMOVED:
		case ERRCODE_SYSID_DIFF_BETWEEN_PRIMARY_AND_STANDBY:
		case ERRCODE_RECOVERY_REQUEST_POINT_AHEAD_START:
			throw WalRcvException(strMsg.c_str());
			
		case ERRCODE_DATA_CORRUPTED:
			throw DataCorruptedException(strMsg.c_str());
        default:
            throw StorageEngineExceptionUniversal(errCode,strMsg);
        } 
	}
	if (FATAL == level || PANIC == level)
	{
	    throw StorageEngineFatalException(errCode,strMsg.c_str());
	}
}

} //StorageEngineNS
} //FounerXDB





