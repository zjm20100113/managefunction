#ifndef _ATOMIC_MUTEX_LOCK_H_INCLUDE_
#define _ATOMIC_MUTEX_LOCK_H_INCLUDE_

#include <unistd.h>
#include <sched.h>
#include <inttypes.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>
#include "common.h"

typedef volatile unsigned long atomic_t;

typedef struct {
  atomic_t   *lock; /* should be located in the shared memory */
  atomic_t   *wait; /* should be located in the shared memory */
  uintptr_t   semaphore;
  sem_t      *sem;          // semaphore shared between process, so it is must be located the shared memory
  uintptr_t   spin; // if spin equal (uintptr_t) -1 then the semaphore will not be used
} mutex_t;

unsigned long 
atomic_cmp_set(atomic_t *lock, uintptr_t value, uintptr_t set);

intptr_t 
mutex_create(mutex_t *mtx, atomic_t *lock, atomic_t *wait);

intptr_t
mutex_trylock(mutex_t *mtx, pid_t pid);

intptr_t 
mutex_lock(mutex_t *mtx, pid_t pid);

intptr_t
mutex_unlock(mutex_t *mtx, pid_t pid);

intptr_t 
mutex_destroy(mutex_t *mtx);
#endif


