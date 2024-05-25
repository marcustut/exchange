#ifndef UINT64_HASHMAP_H
#define UINT64_HASHMAP_H

#include "limit.h"

#define UINT64_HASHMAP_DEFAULT_CAPACITY 8
#define UINT64_HASHMAP_MAX_LOAD_FACTOR 65  // 65%

struct uint64_hashmap_entry {
  uint64_t key;
  void* value;
};

struct uint64_hashmap {
  uint32_t size, capacity;
  struct uint64_hashmap_entry* table;
};

/**
 * A fast hash function dedicated for 64-bit integers, in this case `uint64_t`.
 *
 * @ref
 * https://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
 */
uint64_t uint64_hash(uint64_t key);

/**
 * Fast method of finding the next power of 2 greater than or equal to the
 * supplied value.
 *
 * If the value is <= 0 then 1 will be returned.
 *
 * This method is not suitable for `INT_MIN` or numbers greater than 2^30.
 * When provided then `INT_MIN` will be returned.
 *
 * @ref
 * https://github.com/real-logic/agrona/blob/master/agrona/src/main/java/org/agrona/BitUtil.java
 *
 * @param value from which to search for next power of 2.
 * @return The next power of 2 or the value itself if it is a power of 2.
 */
uint32_t find_next_positive_power_of_two(uint32_t value);

struct uint64_hashmap uint64_hashmap_with_capacity(uint32_t capacity);
struct uint64_hashmap uint64_hashmap_new();
void uint64_hashmap_free(struct uint64_hashmap* map);
void* uint64_hashmap_put(struct uint64_hashmap* map, uint64_t key, void* value);
void* uint64_hashmap_get(struct uint64_hashmap* map, uint64_t key);
void* uint64_hashmap_remove(struct uint64_hashmap* map, uint64_t key);

char* uint64_hashmap_print(struct uint64_hashmap* map);

#endif