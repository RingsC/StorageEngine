#ifndef META_DATA_HPP
#define META_DATA_HPP
#ifdef WIN32
#include <hash_map>
using namespace stdext;
#else
#include<ext/hash_map>
using namespace __gnu_cxx;
#endif

//#include "PGSETypes.h"

#include "utils/rel.h"



// save colinfo in cache. g_col_info_cache is inited in start_engine
extern hash_map<Oid, Colinfo> g_col_info_cache;


// insert a colid and *pcol_info into g_col_info_cache;
extern void setColInfo(Oid colid, Colinfo pcol_info);

//get Colinfo* from g_col_info_cache by colid.
extern  Colinfo getColInfo(Oid colid);


#endif //META_DATA_HPP
