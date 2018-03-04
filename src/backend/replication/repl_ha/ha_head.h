#ifndef __HA_HEAD_H__
#define __HA_HEAD_H__

#ifndef HA_SIMULATE_ENV
#include "postgres.h"
#include "libpq/libpq.h"
#include "libpq/ip.h"
#include "postmaster/postmaster.h"
#include "postmaster/xdb_main.h"
#include "access/xlogdefs.h"
#include "port.h"
#ifdef WIN32
#include <mstcpip.h>
#else
#include <ifaddrs.h>
#endif

#else
#include "simulate/include/ha_in_pg.h"
#endif



#include "ha_debug.h"

#define ha_Sleep(sleep_time_us) pg_sleep(sleep_time_us)

#define ha_malloc(size) palloc(size)
#define ha_free(ptr) pfree(ptr)

typedef struct XLogRecPtr ha_lsn_t;

#include "ha_net.h"	 
#include "ha_env.h"

#endif

