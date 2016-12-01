#include "string_hash.h"

int cache_size = MACH_CACHE_LINE;
size_t cryptTable[0x500];

void 
prepare_crypt_table()  
{   

  size_t seed = 0x00100001, index1 = 0, index2 = 0, i;  

  for (index1 = 0; index1 < 0x100; index1++  ) {   

    for (index2 = index1, i = 0; i < 5; i++, index2 += 0x100  ) {   

      size_t temp1, temp2;  

      seed = (seed * 125 + 3) % 0x2AAAAB;  
      temp1 = (seed & 0xFFFF) << 0x10;  

      seed = (seed * 125 + 3) % 0x2AAAAB;  
      temp2 = (seed & 0xFFFF);  

      cryptTable[index2] = ( temp1 | temp2  );   

    }   

  }   

}  



size_t 
hash_string(char *lpszFileName, size_t dwHashType)  
{   

  unsigned char *key  = (unsigned char *)lpszFileName;  

  size_t seed1 = 0x7FED7FED;  

  size_t seed2 = 0xEEEEEEEE;  

  int ch;  



  while (*key != '\0') {   

    ch = toupper(*key++);  

    seed1 = cryptTable[(dwHashType << 8) + ch] ^ (seed1 + seed2);  
    seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;   
  }  

  return seed1;   

}  

void 
strtolower(u_char *dst, u_char *src, size_t n) 
{
  u_char c ;
  while (n) {
    c = *src;
    *dst = ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c);
    dst++;
    src++;
    n--;
  }

}

inline int 
hash_elt_size()
{
  int size = 0;
  /** size = (sizeof(void *) + align(sizeof(unsigned short) + sizeof(u_char *), sizeof(void *))); */
  size = (sizeof(void *) + sizeof(char *));
  return size;
}

/**comment: Initialize hash, Once that is done cannot be modified
 * param: hinit hash_init_t
 *        name  key_value members
 *        netls the number of key_value members
 * return: -2  the max count of key-value members is zero
 *         -3  the max size of the buckets is smaller than the size of the key-value member
 *         ERROR alloc memory failed
 *         OK  success*/
int
hash_init(hash_init_t *hinit, hash_key_t *name, int nelts)
{
  u_char             *elts = NULL;
  size_t              len = 0;
  unsigned short     *test;
  int                 i = 0, n = 0, key = 0, size = 0, start = 0, bucket_size = 0;
  hash_elt_t         *elt = NULL, **buckets = NULL;
  size_t              elt_size = hash_elt_size();

  if (hinit->max_size <= 0) {
    return -2;
  }

  if (hinit->bucket_size < elt_size + sizeof(void *)) {
    return -3;
  }

  test = malloc(hinit->max_size * sizeof(unsigned short));
  if (NULL == test) {
    return ERROR;
  }

  bucket_size = hinit->bucket_size - sizeof(void *);

  size = nelts * 2; //装填因子为0.5 超过0.7 冲突增加 效率降低
  size = size < hinit->max_size ? size : hinit->max_size;

  /**为每个bucket预留哨兵位置 */
  for (i = 0; i < size; i++) {
    test[i] = sizeof(void *);
  }

  for (n = 0; n < nelts; n++){
    if (name[n].key == NULL)
      continue;

    key = name[n].key_hash % size;
    test[key] = (unsigned short) (test[key] + elt_size);
  }

  len = 0;

  for (i = 0; i < size; i++) {
    if (test[i] == sizeof(void *)) {
      continue;
    }

    test[i] = (unsigned short)(align(test[i], cache_size));

    len += test[i];
  }

  /**申请空间用于存放桶  */
  if (NULL == hinit->hash) {
    hinit->hash = palloc(hinit->pool, sizeof(hash_t) + size * sizeof(hash_elt_t *));

    if (NULL == hinit->hash) {
      free(test);
      return ERROR;
    }

    buckets = (hash_elt_t **) ((u_char *)hinit->hash + sizeof(hash_t));
  } else {
    buckets = palloc(hinit->pool, size * sizeof(hash_elt_t *));

    if (NULL == buckets) {
      free (test);
      return ERROR;
    }

  }

  /**申请连续的存储空间用于存放所有元素 */
  elts =  (u_char *)palloc(hinit->pool,len + cache_size );

   if (NULL == elts) {
    free(test);
    return ERROR;
  }

  elts = (u_char *) align_ptr(elts, cache_size);

  /**每个桶指向各自的元素存储空间起始地址 */
  for (i = 0; i < size; i++) {
    if (test[i] == sizeof(void *)) {
      continue;
    }

    buckets[i] = (hash_elt_t *) elts;
    elts += test[i];
  } 

  for (i = 0; i < size; i++) {
    test[i] = 0;
  }

  for (n = 0; n < nelts; n++) {
    if (name[n].key == NULL) {
      continue;
    }

    key = name[n].key_hash % size;

    elt = (hash_elt_t *) ((u_char *)buckets[key] + test[key]);
    elt->value = name[n].value;
    elt->key_name = name[n].key;
    test[key] = (unsigned short) (test[key] + elt_size);
  } 

  for (i = 0; i < size; i++) {
    if (NULL == buckets[i]) {
      continue;
    }

    elt = (hash_elt_t *) ((u_char *) buckets[i] + test[i]);
    /**每个桶范围之间的哨兵 */
    elt->value = NULL; 
  }

  free(test); 
  hinit->hash->buckets = buckets;
  hinit->hash->size = size;

  return OK;
}


void *
hash_find(hash_init_t *hinit, char *name)
{
  uintptr_t     i;
  size_t        len = strlen(name);
  hash_elt_t   *elt = NULL;
  size_t key = 0;
  hash_t       *hash = NULL;
  size_t        hash_size = 0;

  if (NULL == hinit->key) {
    return NULL;
  }

  hash = hinit->hash;
  hash_size = hinit->hash->size;
  key = hinit->key(name, 0);

  if (0 >= hash_size) {
    return NULL;
  }

  i = key % hash_size;

  elt = hash->buckets[i];
  if (elt == NULL)  {
    return NULL;
  }

  while (elt->value)  {
    do {
      if (len != (size_t) strlen(elt->key_name)) {
        break;
      } 

      for (i = 0; i < len; i++) {
        if (name[i] != elt->key_name[i]) {
          break;
        }
      }

      return elt->value;
    
    } while(0); 
 
    elt = (hash_elt_t *)((u_char *)elt + hash_elt_size());

    continue;
  }

  return NULL;
}


size_t 
string_hash_key_char(char *data, size_t size)
{
  size_t key = 0;

  key = hash_string(data, 0);

  return key;
}

/**using BKDR algorithm */
size_t
string_hash_key_digital(char *data, size_t size) {
  size_t i = 0, key = 0;
  size_t ch = 0;

  for (i = 0; i < size; i++) {
    ch = (size_t)data[i];
    key = key * 31 + ch;
  }

  return key;
}
