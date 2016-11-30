#ifndef __PALLOC_H_INCLUDE_
#define __PALLOC_H_INCLUDE_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>

#define DEFAULT_POOL_SIZE   (16 * 1024)
#define POOL_ALIGNMENT       16
#define PTR_ALIGNMENT        sizeof(unsigned long)    /* platform word */
#define align(d, a)          (((d) + (a - 1)) & ~(a - 1))
#define align_ptr(p, a)      \
  (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define MIN_POOL_SIZE        \
  align((sizeof(pool_t) + 2 * sizeof(pool_large_t)), POOL_ALIGNMENT)

#define MAX_ALLOC_FROM_POOL  (getpagesize() - 1)

typedef struct pool_cleanup_s  pool_cleanup_t;
typedef struct pool_s          pool_t;
typedef struct pool_large_s    pool_large_t;
typedef struct pool_data_s     pool_data_t;
typedef void (*pool_cleanup_pt)(void *data);

struct pool_cleanup_s {
  pool_cleanup_pt   handler;
  void              *data;
  pool_cleanup_t    *next;
};



struct pool_large_s {
  pool_large_t     *next;
  void             *alloc;
};


struct pool_data_s{
  u_char               *last;
  u_char               *end;
  pool_t               *next;
  unsigned int         failed;
};


struct pool_s {
  pool_data_t       d;
  size_t            max;
  pool_t           *current;
  pool_large_t     *large;
  pool_cleanup_t   *cleanup;
};



pool_t * 
create_pool(size_t size);
void 
destroy_pool(pool_t *pool);
void 
reset_pool(pool_t * pool);
void * 
palloc(pool_t *pool, size_t size);
void *
pcalloc(pool_t *pool, size_t size);
void *
palloc_block(pool_t *pool, size_t size);
void *
palloc_large(pool_t *pool, size_t size);
void * 
mem_align(size_t alignment, size_t size);
#endif

