#ifndef MAP_UTILS_H
#define MAP_UTILS_H

#include <stdint.h>

static const uint64_t DEFAULT_EMPTY_KEY = 0;
static const uint64_t TOMBSTONE_KEY = UINT64_MAX;
static const uint32_t DEFAULT_PRIME_FACTOR = 109345121;
static const uint32_t DEFAULT_CAPACITY = 17;
static const uint32_t DEFAULT_MAX_LOAD_FACTOR = 50;  // 50%

#endif
