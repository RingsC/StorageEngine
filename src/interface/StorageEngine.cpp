#include "postgres.h"
#include "utils/relcache.h"
#include "StorageEngine.h"
#include "postmaster/xdb_main.h"
#include "interface/PGStorageEngine.h"

using namespace FounderXDB::StorageEngineNS;


StorageEngine* StorageEngine::getStorageEngine()
{
    return PGStorageEngine::getInstance();
}

void StorageEngine::releaseStorageEngine(StorageEngine* instance)
{
    PGStorageEngine* pgInstance = static_cast<PGStorageEngine*>(instance);
    return PGStorageEngine::releaseStorageEngine(pgInstance);
}
RangeDatai::~RangeDatai()
{
    if (!ownMem)
        return;
    if (itemSpace)
        MemoryContext::deAlloc(itemSpace);
    if (items)
        MemoryContext::deAlloc(items);
}

void RangeDatai::addAddrLen(Transaction *txn, const void *addr1, size_t len1)
{
    if (items == 0 || itemSpace == 0 || nItems == 0 || szItems == 0 || curpos == 0) {
        assert(items == 0 && itemSpace == 0 && nItems == 0 && szItems == 0 && curpos == 0);
        szItems = 128;
        totalSpace = 4 * 1024;
        itemSpace = txn->getAssociatedMemoryContext()->alloc(totalSpace);// alloc 4K
        items = (AddrLen *)txn->getAssociatedMemoryContext()->alloc(szItems * sizeof(AddrLen));
        this->ownMem = true;
    }
    if (this->nItems >= szItems) {
        assert(ownMem);
        szItems *= 2;
        items = (AddrLen *)MemoryContext::reAlloc(items, szItems * sizeof(AddrLen));       
    }        
    if (curpos + len1 > totalSpace) {
        assert(ownMem);
        totalSpace *= 2;
        totalSpace += len1;
		void* oldspace = itemSpace;
        itemSpace = MemoryContext::reAlloc(itemSpace, totalSpace);
		if(itemSpace == NULL)
		{
			throw FounderXDB::EXCEPTION::OutOfMemoryException(len1, "Cannot allocate memory in addAddrLen");
		}
    }
    items[nItems].addr = (char *)itemSpace + curpos;
    items[nItems].len = len1;
    curpos += len1;
    memcpy(items[nItems].addr, addr1, len1);
    
    nItems++;
}

bool RangeDatai::isValid()const
{
    return (start == 0 && len == 0 && nItems != 0 && szItems != 0 && items != 0 && itemSpace != 0) || 
            (nItems == 0 && szItems == 0 && items == 0 && itemSpace == 0); 
}

RangeDatai::RangeDatai(){memset(this, 0, sizeof(RangeDatai)); }
RangeDatai::RangeDatai(const RangeDatai&obj) 
{ 
    memcpy(this, &obj, sizeof(obj)); 
    // ownMem = true; don't set this, because obj may not own mem either.
    obj.ownMem = false;/*transfer ownership*/
}
DBMetaInfo::~DBMetaInfo()
{
    if (metaData && ownMem_) 
        MemoryContext::deAlloc(metaData);
}

DBMetaInfo::DBMetaInfo(const DBMetaInfo&dbmi)
{
    metaData = dbmi.metaData;
    metaDataLen = dbmi.metaDataLen;
    dbid = dbmi.dbid;
	tbid = dbmi.tbid;
    dbname = dbmi.dbname;
    if (metaData) {
        dbmi.ownMem_ = false;// transfer ownership
        ownMem_ = true;
    }
}

const DBMetaInfo&DBMetaInfo::operator=(const DBMetaInfo&dbmi) //the two methods are needed, default is OK.
{
    if (metaData && ownMem_) 
        MemoryContext::deAlloc(metaData);
    metaData = dbmi.metaData;
    metaDataLen = dbmi.metaDataLen;
    dbid = dbmi.dbid;
	tbid = dbmi.tbid;
    dbname = dbmi.dbname;
    if (metaData) {
        dbmi.ownMem_ = false;// transfer ownership
        ownMem_ = true;
    }
    return dbmi;
}

void DBMetaInfo::setMetaData(void *data, size_t len)
{ 
    metaData = data; 
    metaDataLen = len; 
    ownMem_ = true;
}