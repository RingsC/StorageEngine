#ifndef XDB_MAIN_H
#define XDB_MAIN_H
#include "Configs.h"
#define DEFAULTTABLESPACE_OID 1664
#define DEFAULTDATABASE_OID 11967
#define GLOBALTABLESPACE_OID 1663
typedef enum {
	UNKNOWN_TYPE = 0,
	BTREE_TYPE = 1,
	HASH_TYPE = 2,
	BITMAP_TYPE = 3,
    BTREE_CLUSTER_TYPE = 4,
    LSM_TYPE = 5,
    LSM_SUBTYPE = 6,
    
	UINQUE_TYPE_BASE = 8,
	BTREE_UNIQUE_TYPE = UINQUE_TYPE_BASE,
	//HASH_UNIQUE_TYPE = 9,
    BITMAP_UNIQUE_TYPE = 10,
    BTREE_UNIQUE_CLUSTER_TYPE = 11
} IndexType;

#define  IS_CLUSTER_INDEX(indextype) ((indextype) == BTREE_UNIQUE_CLUSTER_TYPE || (indextype) == BTREE_CLUSTER_TYPE)

extern void stop_engine(int status);
extern void start_engine(const char *datadir, const uint32 thread_num, 
												 bool initDatadir, storage_params * params);
extern void interrupt_start_engine(int code);
extern void initdb(const char *datadir);
extern void setloglevel(int level);
extern void close_port(int code, Datum arg);
#endif //XDB_MAIN_H
