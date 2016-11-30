#include "palloc.h"

/**comment:����ʽ��ϵͳ�����ڴ�
 * parm: alignment ���뵥λ�Ĵ�С2��������
 *       size ������ڴ��С
 *return:success ���ص�ַ
 *       failed: ����NULL ͨ��errno��ȡ������Ϣ
 *       */
void * 
mem_align(size_t alignment, size_t size) 
{
  void *p = NULL;
  int   err = -1;
  
  err = posix_memalign(&p, alignment, size * sizeof(char));

  if (err) {
    p = NULL;
  }
  
  return p;
}


/** 
 * comment: �����ڴ��
 * param:size �ڴ�صĳ�ʼ����С
 * return:success �����ڴ���׵�ַ
 *        failed  ����NULL errno��ȡ������Ϣ
 * */
pool_t * 
create_pool(size_t size) 
{
  pool_t *p;
  
  p = mem_align(POOL_ALIGNMENT, size);
  
  if (p == NULL) {
    return  NULL;
  }
  
  p->d.last = (u_char *) p + sizeof(pool_t);
  p->d.end = (u_char *) p + size;
  p->d.next = NULL;
  p->d.failed = 0;
  
  size = size - sizeof(pool_t);
  
  p->max = (size < MAX_ALLOC_FROM_POOL) ? size : MAX_ALLOC_FROM_POOL;

  p->current = p;
  p->large = NULL;
  p->cleanup = NULL;

  return p;
}

/** 
 * comment: �����ڴ��
 * param: pool �ڴ���׵�ַ
 * return:void
 * */
void 
destroy_pool(pool_t *pool)
{
  pool_t         *p = NULL, *n = NULL;
  pool_large_t   *l = NULL;
  pool_cleanup_t *c = NULL;

  for(c = pool->cleanup; c; c = c->next) {
    if (c->handler) {
      c->handler(c->data);
    }
  }

  for (l = pool->large; l ; l = l->next) {
    if (l->alloc) {
      free(l->alloc);
    }
  }

  for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
    free(p);

    if (n == NULL) {
      break;
    }
  }
  
}

/**
 * comment:�����ڴ��,�ͷ�ԭ�еĴ���ڴ�
 * param:�ڴ�ص��׵�ַ
 * return:void
 * */
void 
reset_pool(pool_t * pool) 
{
  pool_t         *p = NULL;
  pool_large_t   *l = NULL;
  
  for (l = pool->large; l; l = l->next) {
    if (l->alloc) {
      free(l->alloc);
    }
  }
  
  for (p = pool; p; p = p->d.next) {
    p->d.last  = (u_char *) p  + sizeof(pool_t);
    p->d.failed = 0;
  }
  
  p->current = pool;
  p->large = NULL;
}

/**comment:���ڴ�������ڴ�
 * param:pool �ڴ���׵�ַ
 *       size �����ڴ�Ĵ�С
 * return:success ���ؿ��õ�ַ
 *        failed NULL
 * */
void * 
palloc(pool_t *pool, size_t size) 
{
  u_char *m = NULL;
  pool_t *p = NULL;
  
  if (size <= pool->max) {
    p = pool->current;
    
    do {
      m = align_ptr(p->d.last, PTR_ALIGNMENT);
      
      if ((size_t) (p->d.end - m) >= size) {
        p->d.last = m + size;
        
        return m;
      }
      
      p = p->d.next;
    } while (p);
    
    return palloc_block(pool, size);
  }
  
  return palloc_large(pool, size);
}

/**comment: ���ڴ�������ַ �ɹ����ڴ���ʼ��Ϊ0
 * param: pool �ڴ���׵�ַ
 *        size �����ڴ�Ĵ�С
 *return: success ���õĵ�ַ
 *        failed  NULL
 * */
void *
pcalloc(pool_t *pool, size_t size) //���䲢��ʼ��Ϊ��
{
    void *p;

    p = palloc(pool, size);
    if (p) {
      (void)memset(p, 0, size * sizeof(char));
    }

    return p;
}

/**comment:��ϵͳ���벢�����µ��ڴ��
 * param: pool �ڴ���׵�ַ
 *        size �ڴ��Ĵ�С
 * return:success ���ڴ���׵�ַ
 *        failed  NULL
 *        */
void * 
palloc_block(pool_t *pool, size_t size)
{
  u_char     *m = NULL;
  size_t      psize = 0;
  pool_t     *p = NULL, *new = NULL;
  
  psize = (size_t) (pool->d.end - (u_char *)pool);
  
  m = mem_align(POOL_ALIGNMENT, psize);
  
  if (m == NULL) {
    return NULL;
  }
  
  
  new = (pool_t *) m;
  
  new->d.end = m + psize;
  new->d.next = NULL;
  new->d.failed = 0;
  
  m += sizeof(pool_data_t);
  m = align_ptr(m, PTR_ALIGNMENT);
  new->d.last = m + size;
  
  for (p = pool->current; p->d.next; p = p->d.next) {
    if (p->d.failed++ > 4) {
      pool->current = p->d.next;
    }
  }

  p->d.next = new;

  return m;
}

/**comment:��ϵͳ�������ڴ�,������Ǽ����ڴ����
 * param: pool �ڴ���׵�ַ
 *        size  ����ڴ�Ĵ�С
 * return:success �����ڴ���׵�ַ
 *        failed  NULL
 *        */
void *
palloc_large(pool_t *pool, size_t size)
{
  void          *p = NULL;
  unsigned int   n = 0;
  pool_large_t  *large = NULL;
  
  p = malloc(size);
  
  if (p == NULL) {
    return NULL;
  }
  
  n = 0;
  
  for (large = pool->large; large; large = large->next) {
    if (large->alloc == NULL) {
      large->alloc = p;
      return p;
    }

    if (n++ > 3)  {
      break;
    }
  }

  large = palloc(pool, sizeof(pool_large_t));
  if (large == NULL) {
    free(p);
    return NULL;
  }

  large->alloc = p;
  large->next = pool->large;
  pool->large = large;

  return p;
}







