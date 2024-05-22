#ifndef MAP_UTILS_H
#define MAP_UTILS_H

#include <stdint.h>

static const uint64_t DEFAULT_EMPTY_KEY = 0;
static const uint64_t TOMBSTONE_KEY = UINT64_MAX;
static const uint32_t DEFAULT_PRIME_FACTOR = 109345121;
static const uint32_t DEFAULT_CAPACITY = 17;
static const uint32_t DEFAULT_MAX_LOAD_FACTOR = 50;  // 50%

/**
 * Hash function to get the index of the _price_limit_map_hash table given a
 * key.
 *
 * @ref
 * https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
 */
uint64_t map_hash(uint64_t key);

#define MAP_PROBE(map, key)                          \
  int free = -1;                                     \
  uint64_t hashed_i = map_hash(key) % map->capacity; \
  uint64_t i = hashed_i;                             \
  do {                                               \
    if (map->table[i].key == DEFAULT_EMPTY_KEY) {    \
      free = i;                                      \
      break;                                         \
    } else if (map->table[i].key == key)             \
      return i;                                      \
    i = (i + 1) % map->capacity;                     \
  } while (i != hashed_i);                           \
  return -(free + 1);

#endif
