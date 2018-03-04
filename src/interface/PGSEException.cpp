#include "StorageEngineException.h"
namespace FounderXDB{
namespace StorageEngineNS{
// TODO: the impl of the two classes are over simplified, make them more robust later.
ObjectNotDestroyedException::ObjectNotDestroyedException(size_t nobjs)
{
    m_nobjs = nobjs;
}

//OutOfMemoryException::OutOfMemoryException(size_t nbytes)
//{
//    m_nbytes = nbytes;
//}

}
}