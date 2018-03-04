#ifndef STORAGE_ENGINE_EXECPTION_UNIVERSAL_HPP
#define STORAGE_ENGINE_EXECPTION_UNIVERSAL_HPP

///@file StorageEngineExceptionUniversal
///@brief derived from StorageEngineException and can instance
#include "StorageEngineException.h"
#include <string>

namespace FounderXDB{
namespace StorageEngineNS {

///@ingroup StorageEngine
/// StorageEngineExceptionUniversal define exception for storage engine.
class  StorageEngineExceptionUniversal : virtual public StorageEngineException
{
public:
///constuctors
    StorageEngineExceptionUniversal();
    StorageEngineExceptionUniversal(int err, std::string errmsg);
    StorageEngineExceptionUniversal(int err, const char *errmsg);

/// overload operator = 

/// destructor
    virtual ~StorageEngineExceptionUniversal() throw ();

        //// Get error number
    virtual int getErrorNo() const;

    /// Get error message
    virtual const char* getErrorMsg() const;
};

void ThrowException( void );

} //StorageEngineNS
} //FounerXDB

#endif 