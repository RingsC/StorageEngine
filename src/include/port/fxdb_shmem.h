
#ifndef FXDB_SHMEM_H
#define FXDB_SHMEM_H

#ifdef HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif

#ifdef WIN32
  struct shmid_ds
{
	int			dummy;
	int			shm_nattch;
};
#endif

extern int	fxdb_shmget(int key, Size size, int flag);
extern int	fxdb_shmdt(const void *shmaddr);
extern void *fxdb_shmat(int memId, const void *shmaddr, int flag);
extern int fxdb_shmctl(int memId, int cmd, struct shmid_ds * buf);

#endif /* FXDB_SHMEM_H */
