#ifndef ORDER_METADATA_MAP_H
#define ORDER_METADATA_MAP_H

#include "limit.h"
#include "map_utils.h"

struct order_metadata {
  struct order* order;
};

struct order_metadata order_metadata_default();

struct order_metadata_map_entry {
  uint64_t key;                  // order_id
  struct order_metadata* value;  // metadata
};

struct order_metadata_map {
  uint32_t size, capacity, prime, scale, shift;
  double max_load_factor;
  struct order_metadata_map_entry* table;
};

struct order_metadata_map order_metadata_map_with_capacity(uint32_t capacity);
struct order_metadata_map order_metadata_map_new();
void order_metadata_map_free(struct order_metadata_map* map);
struct order_metadata order_metadata_map_put(
    struct order_metadata_map* map,
    uint64_t order_id,
    struct order_metadata* order_metadata);
struct order_metadata order_metadata_map_get(struct order_metadata_map* map,
                                             uint64_t order_id);
struct order_metadata* order_metadata_map_get_mut(
    struct order_metadata_map* map,
    uint64_t order_id);
struct order_metadata* order_metadata_map_remove(struct order_metadata_map* map,
                                                 uint64_t order_id);

#endif