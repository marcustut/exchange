#ifndef PRICE_LIMIT_MAP_H
#define PRICE_LIMIT_MAP_H

#include "limit.h"
#include "map_utils.h"

struct price_limit_map_entry {
  uint64_t key;         // price
  struct limit* value;  // limit
};

struct price_limit_map {
  uint32_t size, capacity, prime, scale, shift;
  double max_load_factor;
  struct price_limit_map_entry* table;
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
struct limit* price_limit_map_remove(struct price_limit_map* map,
                                     uint64_t price);

#endif