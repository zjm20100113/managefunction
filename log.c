#include "log.h"

void log_open(log_t *log)
{
  int fd = -1;

  if (NULL == log) {
    fprintf(stderr, "the argv of log is empty, exit \n");
    exit(-2);
  }

  do {
    if (!strncasecmp(log->file.file_path.data, "stdout", 6)) {
      fd = fileno(stdout);
      break;
    }else if (!strncasecmp(log->file.file_path.data, "stderr", 6)) {
      fd = fileno(stderr);
      break;
    }else if(!strncasecmp(log->file.file_path.data, "/dev/null", 9)) {
      fd = fileno(NULL);
      break;
    }

    if(strlen(log->file.file_path.data) == 0 || 
        log->file.file_path.len == 0) {
      fd = fileno(stderr);
    }

    /** f = fopen(log->file.file_path.data, "a+"); */
    /**  */
    /** if (NULL == f) { */
    /**   fprintf(stderr, "can not open file %s, exit \n", log->file.file_path.data); */
    /**   exit(-2); */
    /** } */
    /**  */
    /** fd = fileno(f); */

    fd = open(log->file.file_path.data, O_WRONLY|O_APPEND|O_CREAT, 0644);
    
  } while(0);  

  log->file.fd = fd;

}

u_char *
log_sprintf(u_char *buf, u_char *last, const char *fmt, ...)
{
  u_char *p;
  va_list args;

  va_start(args, fmt);
  p = log_vslprintf(buf, last, fmt, args);
  va_end(args);

  return p;
}

void
log_error_core(log_t *log, int err,
    const char *fmt, ...)
{
  va_list        args;
  u_char        *p, *last;
  u_char         errstr[MAX_ERROR_STR];
  struct timeval tv;
  struct tm     *ptr;
  time_t         lt;
  char           time_str[128]; 
  pid_t          pid; 

  last = errstr + MAX_ERROR_STR;

  pid = getpid();

  p = log_sprintf(errstr, last, "[%P]", pid);

  gettimeofday(&tv, NULL);
  lt = tv.tv_sec;
  ptr = localtime(&lt);
  sprintf(time_str, "%02d%02d%02d:%02d%02d%02d.%06d",
      ptr->tm_year-100, ptr->tm_mon+1, ptr->tm_mday,
      ptr->tm_hour, ptr->tm_min, ptr->tm_sec, (int)tv.tv_usec);


  p = log_sprintf(p, last, " [%s][errno:%d]", time_str, err);

  va_start(args, fmt);
  p = log_vslprintf(p, last, fmt, args);
  va_end(args);

  if (p > last - 1) {
     p = last -1;
  }

  *p++ = '\n';

  write(log->file.fd, errstr, p - errstr);
}

u_char *
log_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args)
{
  u_char                *p, zero;
  int                    d;
  double                 f;
  size_t                 len, slen;
  int64_t                i64;
  uint64_t               ui64, frac;
  uint_t                 width, sign, hex, max_width, frac_width, scale, n;
  str_t                 *v;

  while (*fmt && buf < last) {

    /*
     * "buf < last" means that we could copy at least one character:
     * the plain character, "%%", "%c", and minus without the checking
     */

    if (*fmt == '%') {

      i64 = 0;
      ui64 = 0;

      zero = (u_char) ((*++fmt == '0') ? '0' : ' ');
      width = 0;
      sign = 1;
      hex = 0;
      max_width = 0;
      frac_width = 0;
      slen = (size_t) -1;

      while (*fmt >= '0' && *fmt <= '9') {
        width = width * 10 + *fmt++ - '0';
      }


      for ( ;; ) {
        switch (*fmt) {

          case 'u':
            sign = 0;
            fmt++;
            continue;

          case 'm':
            max_width = 1;
            fmt++;
            continue;

          case 'X':
            hex = 2;
            sign = 0;
            fmt++;
            continue;

          case 'x':
            hex = 1;
            sign = 0;
            fmt++;
            continue;

          case '.':
            fmt++;

            while (*fmt >= '0' && *fmt <= '9') {
              frac_width = frac_width * 10 + *fmt++ - '0';
            }

            break;

          case '*':
            slen = va_arg(args, size_t);
            fmt++;
            continue;

          default:
            break;
        }

        break;
      }


      switch (*fmt) {

        case 'V':
          v = va_arg(args, str_t *);

          len =min(((size_t) (last - buf)), v->len);
          buf = cpymem(buf, v->data, len);
          fmt++;

          continue;

        case 's':
          p = va_arg(args, u_char *);

          if (slen == (size_t) -1) {
            while (*p && buf < last) {
              *buf++ = *p++;
            }

          } else {
            len = min(((size_t) (last - buf)), slen);
            buf = cpymem(buf, p, len);
          }

          fmt++;

          continue;

        case 'O':
          i64 = (int64_t) va_arg(args, off_t);
          sign = 1;
          break;

        case 'P':
          i64 = (int64_t) va_arg(args, pid_t);
          sign = 1;
          break;

        case 'T':
          i64 = (int64_t) va_arg(args, time_t);
          sign = 1;
          break;

        case 'z':
          if (sign) {
            i64 = (int64_t) va_arg(args, ssize_t);
          } else {
            ui64 = (uint64_t) va_arg(args, size_t);
          }
          break;

        case 'i':
          if (sign) {
            i64 = (int64_t) va_arg(args, int_t);
          } else {
            ui64 = (uint64_t) va_arg(args, uint_t);
          }

          if (max_width) {
            width = sizeof("-2147483648") - 1;
          }

          break;

        case 'd':
          if (sign) {
            i64 = (int64_t) va_arg(args, int);
          } else {
            ui64 = (uint64_t) va_arg(args, u_int);
          }
          break;

        case 'l':
          if (sign) {
            i64 = (int64_t) va_arg(args, long);
          } else {
            ui64 = (uint64_t) va_arg(args, u_long);
          }
          break;

        case 'D':
          if (sign) {
            i64 = (int64_t) va_arg(args, int32_t);
          } else {
            ui64 = (uint64_t) va_arg(args, uint32_t);
          }
          break;

        case 'L':
          if (sign) {
            i64 = va_arg(args, int64_t);
          } else {
            ui64 = va_arg(args, uint64_t);
          }
          break;

        case 'f':
          f = va_arg(args, double);

          if (f < 0) {
            *buf++ = '-';
            f = -f;
          }

          ui64 = (int64_t) f;
          frac = 0;

          if (0 == frac_width) {//默认精度为6
            frac_width = 6;
          }

          if (frac_width) {

            scale = 1;
            for (n = frac_width; n; n--) {
              scale *= 10;
            }

            frac = (uint64_t) ((f - (double) ui64) * scale + 0.5);

            if (frac == scale) {
              ui64++;
              frac = 0;
            }
          }

          buf = sprintf_num(buf, last, ui64, zero, 0, width);

          if (frac_width) {
            if (buf < last) {
              *buf++ = '.';
            }

            buf = sprintf_num(buf, last, frac, '0', 0, frac_width);
          }

          fmt++;

          continue;

        case 'p':
          ui64 = (uintptr_t) va_arg(args, void *);
          hex = 2;
          sign = 0;
          zero = '0';
          width = PTR_SIZE * 2;
          break;

        case 'c':
          d = va_arg(args, int);
          *buf++ = (u_char) (d & 0xff);
          fmt++;

          continue;

        case 'Z':
          *buf++ = '\0';
          fmt++;

          continue;

        case '%':
          *buf++ = '%';
          fmt++;

          continue;

        default:
          *buf++ = *fmt++;

          continue;
      }

      if (sign) {
        if (i64 < 0) {
          *buf++ = '-';
          ui64 = (uint64_t) -i64;

        } else {
          ui64 = (uint64_t) i64;
        }
      }

      buf = sprintf_num(buf, last, ui64, zero, hex, width);

      fmt++;

    } else {
      *buf++ = *fmt++;
    }
  }

  return buf;
}

u_char *
sprintf_num(u_char *buf, u_char *last, uint64_t ui64, u_char zero,
    uint_t hexadecimal, uint_t width)
{
  u_char         *p, temp[INT64_LEN + 1];
  /*
   * we need temp[INT64_LEN] only,
   * but icc issues the warning
   */
  size_t          len;
  uint32_t        ui32;
  static u_char   hex[] = "0123456789abcdef";
  static u_char   HEX[] = "0123456789ABCDEF";

  p = temp + INT64_LEN;

  if (hexadecimal == 0) {

    if (ui64 <= (uint64_t) MAX_UINT32_VALUE) {

      ui32 = (uint32_t) ui64;

      do {
        *--p = (u_char) (ui32 % 10 + '0');
      } while (ui32 /= 10);

    } else {
      do {
        *--p = (u_char) (ui64 % 10 + '0');
      } while (ui64 /= 10);
    }

  } else if (hexadecimal == 1) {

    do {
      /* the "(uint32_t)" cast disables the BCC's warning */
      *--p = hex[(uint32_t) (ui64 & 0xf)];

    } while (ui64 >>= 4);

  } else { /* hexadecimal == 2 */

    do {

      /* the "(uint32_t)" cast disables the BCC's warning */
      *--p = HEX[(uint32_t) (ui64 & 0xf)];

    } while (ui64 >>= 4);
  }

  /* zero or space padding */

  len = (temp + INT64_LEN) - p;

  while (len++ < width && buf < last) {
    *buf++ = zero;
  }

  /* number safe copy */

  len = (temp + INT64_LEN) - p;

  if (buf + len > last) {
    len = last - buf;
  }

  return cpymem(buf, p, len);
}



