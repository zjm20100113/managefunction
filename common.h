#ifndef _COMMON_H_INCLUDE_
#define _COMMON_H_INCLUDE_

#ifndef  OK
#define  OK      0
#endif

#ifndef  ERROR
#define  ERROR  -1
#endif

#ifndef  TRUE
#define  TRUE    1
#endif

#ifndef  FALSE
#define  FALSE   0
#endif

#define align(d, a)          (((d) + (a - 1)) & ~(a - 1))
#define align_ptr(p, a)      \
  (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


#endif
