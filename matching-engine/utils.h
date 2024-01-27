#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
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

#endif /* UTILS_H */
