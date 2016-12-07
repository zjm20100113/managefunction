#include "share_memory.h"

intptr_t
create_share_memory(shmm_t *shmm)
{
  if (shmm->size <= 0) {
    return ERROR;
  }

  shmm->addr = mmap(NULL, shmm->size, 
                    PROT_READ|PROT_WRITE,
                    MAP_ANON|MAP_SHARED, -1, 0);

  if (NULL == shmm->addr) {
    return ERROR;
  }

  memset(shmm->addr, 0, shmm->size * sizeof(char));
  return OK;
}


intptr_t
free_share_memory(shmm_t *shmm) 
{
  if (NULL == shmm->addr) {
    return OK;
  }

  if (munmap((void *) shmm->addr, shmm->size) == -1) {
    return ERROR;
  }

  return OK;
}

