#ifndef SHARE_MEMORY_H_INCLUDE_
#define SHARE_MEMORY_H_INCLUDE_
#include <sys/mman.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"


typedef struct {
  u_char      *addr;
  size_t       size;
} shmm_t;

intptr_t
create_share_memory(shmm_t *shmm);
intptr_t
free_share_memory(shmm_t *shmm);

#endif
