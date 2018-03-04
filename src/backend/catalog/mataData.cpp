#include <iostream>
#include <exception>
#include "postgres.h"
#include "postgres_ext.h"
#include "catalog/metaData.h"
#include "storage/spin.h"

using namespace std;

hash_map<Oid, Colinfo> g_col_info_cache;
slock_t g_spinlock = SpinLockInit(&g_spinlock);

void setColInfo(Oid colid, Colinfo pcol_info)
{
    if(pcol_info == NULL) {
        return;
    }
	try
	{
		SpinLockAcquire(&g_spinlock);
    	g_col_info_cache.insert(std::pair<Oid, Colinfo>(colid, pcol_info));
		SpinLockRelease(&g_spinlock);
	}
	#ifdef _DEBUG
	catch(exception &r)
	{
		ereport(WARNING, (errmsg("Set ColInfo:%s %s",__FUNCTION__,r.what())));
	}
    #else 
	catch(exception &/*r*/)
	{
	}
	#endif
}

Colinfo getColInfo(Oid colid)
{
    hash_map<Oid, Colinfo>::iterator iter;
	try
	{
		SpinLockAcquire(&g_spinlock);
	    iter = g_col_info_cache.find(colid);
	    if (iter != g_col_info_cache.end()) 
		{
			SpinLockRelease(&g_spinlock);
	        return iter->second;
	    }
		SpinLockRelease(&g_spinlock);
	}
#ifdef _DEBUG
	catch(exception &r)
	{
		ereport(WARNING, (errmsg("Get ColInfo:%s %s",__FUNCTION__,r.what())));
	}
#else 
	catch(exception &/*r*/)
	{
	}
#endif
    return NULL;
}

