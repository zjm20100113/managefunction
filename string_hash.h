#ifndef _STRING_HASH_H_INCLUDE_
#define _STRING_HASH_H_INCLUDE_
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "palloc.h"

#define hash(key, c)   ((int) key * 31 + c) 

#ifndef ERROR
#define ERROR -1
#endif

#ifndef OK
#define OK     1
#endif

#ifndef MACH_CACHE_LINE
#define MACH_CACHE_LINE  32
#endif

typedef struct {
  void             *value;
  // unsigned short    len;
  char             *key_name;
}hash_elt_t;


typedef struct {
  hash_elt_t   **buckets;
  int           size;
}hash_t;

typedef struct {
  char               *key;
  size_t              key_hash;
  void               *value;
}hash_key_t;

typedef size_t (*hash_key_pt)(char *data, size_t len);

typedef struct {
 hash_t       *hash;
 hash_key_pt   key;
 int           max_size; // 将要添加成员的最大个数
 int           bucket_size; // 每个存储成员的hash桶的大小, 至少是一个成员的大小
 char         *name;
 pool_t       *pool;
}hash_init_t;



void 
prepare_crypt_table();

size_t 
hash_string(char *lpszFileName, size_t dwHashType);

void 
strtolower(u_char *dst, u_char *src, size_t n);

inline int 
hash_elt_size();

int
hash_init(hash_init_t *hinit, hash_key_t *name, int nelts);

void *
hash_find(hash_init_t *hinit, char *name);

size_t 
string_hash_key_char(char *data, size_t size);

size_t
string_hash_key_digital(char *data, size_t size);

#endif

