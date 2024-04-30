#ifndef PRICE_LIMIT_MAP_H
#define PRICE_LIMIT_MAP_H

#include "limit.h"

static const uint64_t DEFAULT_EMPTY_KEY = 0;
static const uint32_t DEFAULT_PRIME_FACTOR = 109345121;
static const uint32_t DEFAULT_CAPACITY = 17;
static const uint32_t DEFAULT_MAX_LOAD_FACTOR = 50;  // 50%

struct entry {
  uint64_t key;
  struct limit* value;
};

struct price_limit_map {
  uint32_t size, capacity, prime, scale, shift;
  double max_load_factor;
  struct entry* table;
};

struct price_limit_map price_limit_map_with_capacity(uint32_t capacity);
struct price_limit_map price_limit_map_new();
void price_limit_map_free(struct price_limit_map* map);
struct limit price_limit_map_put(struct price_limit_map* map,
                                 uint64_t price,
                                 struct limit* limit);
struct limit price_limit_map_get(struct price_limit_map* map, uint64_t price);
struct limit* price_limit_map_get_mut(struct price_limit_map* map,
                                      uint64_t price);
struct limit price_limit_map_remove(struct price_limit_map* map,
                                    uint64_t price);

#endif