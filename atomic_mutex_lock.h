#ifndef _ATOMIC_MUTEX_LOCK_H_INCLUDE_
#define _ATOMIC_MUTEX_LOCK_H_INCLUDE_

#include <unistd.h>
#include <sched.h>
#include <inttypes.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>

#ifndef  OK
#define  OK  0
#endif

#ifndef  ERROR
#define  ERROR  -1
#endif

typedef volatile unsigned long atomic_t;

typedef struct {
  atomic_t   *lock;
  atomic_t   *wait;
  uintptr_t   semaphore;
  sem_t       sem;         
  uintptr_t   spin;
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


