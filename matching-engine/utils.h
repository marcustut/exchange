#ifndef UTILS_H
#define UTILS_H

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

#define SIGNAMEANDNUM(s) \
  { #s, s }

static struct {
  const char* name;
  int value;
} SIGNALS[] = {
    SIGNAMEANDNUM(SIGINT),
    SIGNAMEANDNUM(SIGTERM),
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
  channel_t* channel = malloc(sizeof(channel_t));

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

#endif /* UTILS_H */
