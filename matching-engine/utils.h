#ifndef UTILS_H
#define UTILS_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MIN(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })

#define MAX(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })

#define CLAMP(x, min, max) (MIN(max, MAX(x, min)))

uint64_t timestamp_nanos() {
#if __APPLE__
  return clock_gettime_nsec_np(CLOCK_REALTIME);
#elif __linux__
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return ts.tv_nsec;
#else
#error "Unknown compiler"
#endif
}

void to_upper(char* str) {
  while (*str) {
    *str = toupper(*str);
    str++;
  }
}

char* sig_to_string(const int sig) {
  char* name = (char*)malloc(sizeof(char) * 10);
  char* signame = strsignal(sig);
  to_upper(signame);
  snprintf(name, 10, "SIG%s", signame);
  return name;
}

#endif /* UTILS_H */
