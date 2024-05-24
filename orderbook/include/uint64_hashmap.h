#ifndef UINT64_HASHMAP_H
#define UINT64_HASHMAP_H

#include "limit.h"
#include "map_utils.h"

struct uint64_hashmap_entry {
  uint64_t key;
  void* value;
};

struct uint64_hashmap {
  uint32_t size, capacity, prime, scale, shift, tombstone_count;
  double max_load_factor;
  struct uint64_hashmap_entry* table;
};

struct uint64_hashmap uint64_hashmap_with_capacity(uint32_t capacity);
struct uint64_hashmap uint64_hashmap_new();
void uint64_hashmap_free(struct uint64_hashmap* map);
void* uint64_hashmap_put(struct uint64_hashmap* map, uint64_t key, void* value);
void* uint64_hashmap_get(struct uint64_hashmap* map, uint64_t key);
void* uint64_hashmap_remove(struct uint64_hashmap* map, uint64_t key);

char* uint64_hashmap_print(struct uint64_hashmap* map);

#endif