#ifndef PRICE_LIMIT_MAP_H
#define PRICE_LIMIT_MAP_H

#include "orderbook.h"

static const uint64_t DEFAULT_EMPTY_KEY = UINT64_MAX;
static const uint32_t DEFAULT_PRIME_FACTOR = 109345121;
static const uint32_t DEFAULT_CAPACITY = 17;
static const double DEFAULT_MAX_LOAD_FACTOR = 0.5;

struct entry {
  uint64_t key;
  limit* value;
};
typedef struct entry entry;

struct price_limit_map {
  uint32_t size, capacity, prime, scale, shift;
  double max_load_factor;
  entry* table;
};
typedef struct price_limit_map price_limit_map;

price_limit_map price_limit_map_new();
limit* price_limit_map_put(price_limit_map* map, uint64_t price, limit* limit);
limit* price_limit_map_get(price_limit_map* map, uint64_t price);
limit* price_limit_map_remove(price_limit_map* map, uint64_t price);

#endif