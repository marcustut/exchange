#ifndef UTILS_H
#define UTILS_H

#include <ctype.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <log/log.h>

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

#define CHECK_ERR(predicate, ret, ...) \
  if (predicate) {                     \
    log_error(__VA_ARGS__);            \
    ret;                               \
  }

#define CHECK_LOG(level, predicate, ret, ...)        \
  if (predicate) {                                   \
    log_log(level, __FILE__, __LINE__, __VA_ARGS__); \
    ret;                                             \
  }

/*
 * The follow macros is a hack to utilize the compiler to count the number of
 * arguments passed to a variadic macro at compile-time.
 * See:
 * https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments
 */
#define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) PP_128TH_ARG(__VA_ARGS__)
#define PP_128TH_ARG(                                                          \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16,     \
    _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, \
    _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, \
    _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, \
    _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, _72, _73, _74, _75, _76, \
    _77, _78, _79, _80, _81, _82, _83, _84, _85, _86, _87, _88, _89, _90, _91, \
    _92, _93, _94, _95, _96, _97, _98, _99, _100, _101, _102, _103, _104,      \
    _105, _106, _107, _108, _109, _110, _111, _112, _113, _114, _115, _116,    \
    _117, _118, _119, _120, _121, _122, _123, _124, _125, _126, _127, N, ...)  \
  N
#define PP_RSEQ_N()                                                            \
  127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113,   \
      112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100, 99, 98, \
      97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80,  \
      79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62,  \
      61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44,  \
      43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26,  \
      25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, \
      6, 5, 4, 3, 2, 1, 0

void _free(size_t n, ...) {
  va_list args;
  va_start(args, n);
  for (size_t i = 0; i < n; i++) {
    void* vp = va_arg(args, void*);
    free(vp);
  }
  va_end(args);
}
#define FREE(...) _free(PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define FREE_RETURN(ret, ...) \
  {                           \
    FREE(__VA_ARGS__);        \
    return ret;               \
  }

uint64_t timestamp_nanos() {
#if __APPLE__
  return clock_gettime_nsec_np(CLOCK_REALTIME);
#elif __linux__
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return (uint64_t)(ts.tv_sec) * (uint64_t)1000000000 + (uint64_t)(ts.tv_nsec);
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

#define SIGNAMEANDNUM(s) \
  { #s, s }

static struct {
  const char* name;
  int value;
} SIGNALS[] = {
    SIGNAMEANDNUM(SIGINT),
    SIGNAMEANDNUM(SIGTERM),
    SIGNAMEANDNUM(SIGPIPE),
};

const char* sig_to_string(int s) {
  const char* name = NULL;

  for (int i = 0; i < sizeof(SIGNALS) / sizeof(*SIGNALS); i++) {
    if (s == SIGNALS[i].value) {
      name = SIGNALS[i].name;
      break;
    }
  }

  return name;
}

typedef struct {
  const char* market;
  const char* topic;
  const char* symbol;
} channel_t;

channel_t* channel_from_str(char* str) {
  channel_t* channel = (channel_t*)malloc(sizeof(channel_t));

  const char* token = strtok(str, ":");
  if (token != NULL)
    channel->market = token;
  else
    goto err;

  token = strtok(NULL, ":");
  if (token != NULL)
    channel->topic = token;
  else
    goto err;

  token = strtok(NULL, ":");
  if (token != NULL)
    channel->symbol = token;
  else
    goto err;

  return channel;

err:
  free(channel);
  return NULL;
}

#define MILLISECOND 1000000

void sleep_ns(uint64_t duration_ns) {
  struct timespec ts = {.tv_sec = 0, .tv_nsec = duration_ns};
  nanosleep(&ts, NULL);
}

#endif /* UTILS_H */
