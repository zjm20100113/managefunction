/* 单次支持写入4000个字节的日志, 若超出则截取和因字符数组越界导致不可预知的程序错误 */
#ifndef __LOG_H_INCLUDE_
#define __LOG_H_INCLUDE_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

#define log_msg(log, ...)     log_error_core(log, __VA_ARGS__)

#define cpymem(dst, src, n)   (((u_char *)  memcpy(dst, src, n)) + (n))
#define min(val1, val2)       ((val1 > val2) ? (val2) : (val1))
#define MAX_ERROR_STR         4096

#if ((__GNU__ == 2) && (__GNUC_MINOR__ < 8))
#define MAX_UINT32_VALUE      (uint32_t) 0xffffffffLL
#else
#define MAX_UINT32_VALUE      (uint32_t) 0xffffffff
#endif

#define INT64_LEN             (sizeof("-9223372036854775808") - 1)

#ifndef PTR_SIZE
#define PTR_SIZE              8
#endif

typedef intptr_t               int_t;
typedef uintptr_t              uint_t;
typedef struct log_s           log_t;
typedef struct open_file_s     open_file_t;

typedef struct {
  size_t      len;
  char       *data;
} str_t;

struct open_file_s {
  int              fd;
  str_t            file_path;
};
  
struct log_s {
  open_file_t      file;

  log_t           *next;
};
    

u_char *
log_sprintf(u_char *buf, u_char *last, const char *fmt, ...);
void
log_error_core(log_t *log, int err, const char *fmt, ...);
u_char *
log_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);
u_char *
sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero,
  uint_t hexadecimal, uint_t width);
void log_open(log_t *log);


#endif
