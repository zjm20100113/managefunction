#include "atomic_mutex_lock.h"
#include "log.h"
extern log_t *flog;

static void 
mutex_wakeup(mutex_t *mtx);

unsigned long atomic_cmp_set(atomic_t *lock, uintptr_t value, uintptr_t set)
{
  return __sync_bool_compare_and_swap(lock, value, set);
}

intptr_t
mutex_create(mutex_t *mtx, atomic_t *lock, atomic_t *wait)
{
  mtx->semaphore = 0;
  mtx->lock = lock;

  if ((uintptr_t) - 1 == mtx->spin) {
    return OK;
  }

  mtx->spin = 2048;
  
  mtx->wait = wait;

  if (sem_init(mtx->sem, 1, 0)) { /** the semaphore is to be shared between process */
    return ERROR;
  }

  mtx->semaphore = 1;

  return OK;
}

intptr_t 
mutex_destroy(mutex_t *mtx)
{
  if (sem_destroy(mtx->sem)) {
    return ERROR;
  }

  return OK;
}

intptr_t
mutex_trylock(mutex_t *mtx, pid_t pid)
{
  if (0 == *mtx->lock && atomic_cmp_set(mtx->lock, 0, pid)) {
    return OK;
  }

  return ERROR;
}


intptr_t
mutex_lock(mutex_t *mtx, pid_t pid)
{
  uintptr_t  i = 0;
  uintptr_t  n = 0;

  uintptr_t  cpu_n = sysconf(_SC_NPROCESSORS_ONLN);

  for ( ;; ) {
    
    if (0 == *mtx->lock && atomic_cmp_set(mtx->lock, 0, pid)) {
      return OK;
    }

    if (cpu_n > 1) {
      
      for (n = 1; n < mtx->spin; n <<= 1) {
        
        for (i = 0; i < n; i++) {
          __asm__ ("pause");
        }

        if (0 == *mtx->lock && atomic_cmp_set(mtx->lock, 0, pid)) {
          return OK;
        }

      }

    }
     
    if (mtx->semaphore) {
      log_msg(flog, 0, "before add wait value:%ul", *mtx->wait);
      __sync_fetch_and_add(mtx->wait, 1);

      if (0 == *mtx->lock && atomic_cmp_set(mtx->lock, 0, pid)) {
        __sync_fetch_and_add(mtx->wait, -1);
        return OK;
      }
 
      log_msg(flog, 0, "sem waiting, wait:%ul", *mtx->wait);
      while (sem_wait(mtx->sem) == -1) {

        log_msg(flog, 0, "sem wait return -1");
        if (errno != EINTR) {
          break;
        }
      }

      log_msg(flog, 0, "semaphore wake up");
      continue;
    }

    sched_yield(); /** let thread that has the high priority run firstly */
  }

  return OK;
}

intptr_t
mutex_unlock(mutex_t *mtx, pid_t pid)
{
  if (atomic_cmp_set(mtx->lock, pid, 0)) {
    mutex_wakeup(mtx);
    return OK;
  }

  return OK;
}

static void 
mutex_wakeup(mutex_t *mtx) 
{
  unsigned long wait;

  if (!mtx->semaphore) {
    return ;
  }

  for ( ;; ) {

    wait = *mtx->wait;

    if ((long)wait <= 0) {
      return;
    }

    if (atomic_cmp_set(mtx->wait, wait, wait - 1)) {
      break;
    }
  }

  sem_post(mtx->sem);

  log_msg(flog, 0, "semaphore post done");
}
