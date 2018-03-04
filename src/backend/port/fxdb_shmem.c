
#include "postgres.h"

#include "port/fxdb_shmem.h"

/* Assumed system buffer align size */
#define ALIGNOF_PAGE		ALIGNOF_BUFFER	

/* Structure of shared memory control */
typedef struct ShmemBlockStruct{
    char*   addr;       /* pointer to the memory chunk   */
    char*   name;       /* name of the chunk, NULL is ok */
}ShmemBlockStruct;

/* Is it a valid shmem block */
#define AssertValidShmemBlock(id)    \
(	\
	AssertMacro((id) >= 0 && (id) < N_SHMEMBLOCK),   \
    AssertMacro(ShmemBlock[(id)].name && ShmemBlock[(id)].addr)	\
)


#define N_SHMEMBLOCK              18

static ShmemBlockStruct ShmemBlock[N_SHMEMBLOCK] = {
	{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
	{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, 
	{NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL}
};

/* internal functions */
static int memoryAlloc(Size size, const char* name);
static int memoryLocateName(const char* name);
static void* memoryGetAddr(int id);

static int
memoryAlloc(Size size, const char* name)
{
    uint32	id;
    
	Assert(name);
    for (id = 0; id < N_SHMEMBLOCK; id++)
	{
		ShmemBlockStruct *shb = ShmemBlock + id;

        if (shb->addr == (char *)NULL)
        {
			char	*addr;
            Assert(!shb->name);

			/*
			 * SYSV shmget() guaranted the returned address is at least page aligned
			 * but malloc() does not. So we emulate it by malloc some more memory and
			 * align it by hand. Hope ALIGNOF_PAGE is not too big. 
			 */
			if ((addr = (char *)malloc(size + ALIGNOF_PAGE)) == NULL)
				ereport(ERROR, 
					(errcode(ERRCODE_OUT_OF_MEMORY), 
					errmsg("out of memory at memory allocation"))); 
			addr = (char *)TYPEALIGN(ALIGNOF_PAGE, addr);

			/* release at proc exit */
			shb->addr = addr;
            shb->name = strdup(name);	

			/* use index as the key */
			return id;
        }
	}
    
    /* Haven't reserved enough shmem blocks? Should never happen */
    ereport(PANIC, 
        (errcode(ERRCODE_INTERNAL_ERROR), 
        errmsg_internal("not enough memory slots reserved for memory"))); 

	/* keep compiler quiet */
    return -1;
}

static int 
memoryLocateName(const char* name)
{
    int     id;

    Assert(name != NULL);
    for (id = 0; id < N_SHMEMBLOCK; id++)
	{
        if (ShmemBlock[id].name != NULL && 
			!strcmp(ShmemBlock[id].name, name))
        {
            AssertValidShmemBlock(id);

			/* use index as the key */
            return id;
        }
	}

    return -1;
}

static void* 
memoryGetAddr(int id)
{
    AssertValidShmemBlock(id);
    return ShmemBlock[id].addr;
}

/* SYSV shmget() eumulation */

int			
fxdb_shmget(int key, Size size, int flag)
{
    int     id = -1;

	if (IPC_PRIVATE == key)
    {   
		id = memoryAlloc(size, NULL);
    }    
	else
	{
		char	name[64];

		sprintf(name, "%d", key);

		if (flag & IPC_CREAT)
        {
            if (flag & IPC_EXCL)
               /* if the key already exists, shmget() should fail */
                id = memoryLocateName(name) >= 0? 
                            -1: memoryAlloc(size, name);
            else
                id = memoryAlloc(size, name);
        }
		else
			id = memoryLocateName(name);
	}
    
    return  id;
}


/* SYSV shmdt() emulation */
int			
fxdb_shmdt(const void *shmaddr)
{
    return 0;
}

/* SYSV shmat() emulation */
void *       
fxdb_shmat(int memId, const void *shmaddr, int flag)
{
	void *addr;

    Assert(!flag);
    addr = (void *)memoryGetAddr(memId);

	/* check shmat() force-mapping semantics */
	if (shmaddr != NULL && addr != shmaddr)
		return (void *)-1;
	return addr;
}

/* SYSV shmctl() emulation */
int			
fxdb_shmctl(int memId, int cmd, struct shmid_ds * buf)
{
	if (IPC_RMID == cmd)
	{
        /* 
         * Don't free this memory till the last reference. Here we
         * just leave the memory till the end of main process. This
         * is ok since all memory is allocated by postmaster or 
         * standalone backend.
         */
        return 0;
	}
	else if (IPC_STAT == cmd)
		buf->shm_nattch = 0;

    return 0;	
}

