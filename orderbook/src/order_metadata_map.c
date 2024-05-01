#include "order_metadata_map.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

size_t _order_metadata_map_hash(struct order_metadata_map* map, uint64_t key);
int _order_metadata_map_probe(struct order_metadata_map* map, uint64_t key);
void _order_metadata_map_resize(struct order_metadata_map* map,
                                uint32_t capacity);

struct order_metadata_map order_metadata_map_with_capacity(uint32_t capacity) {
  srand(time(NULL));
  uint32_t prime = DEFAULT_PRIME_FACTOR;

  // initialise the table to be empty
  struct order_metadata_map_entry* table =
      malloc(sizeof(struct order_metadata_map_entry) * capacity);
  for (int i = 0; i < capacity; i++)
    table[i].key = DEFAULT_EMPTY_KEY;

  struct order_metadata_map map = {.max_load_factor = DEFAULT_MAX_LOAD_FACTOR,
                                   .capacity = capacity,
                                   .prime = prime,
                                   // required to make the bound one less and +1
                                   // to make sure scale % prime != 0
                                   .scale = (rand() % (prime - 1)) + 1,
                                   .shift = rand() % prime,
                                   .table = table};
  return map;
}

struct order_metadata order_metadata_default() {
  return (struct order_metadata){};
}

struct order_metadata_map order_metadata_map_new() {
  return order_metadata_map_with_capacity(DEFAULT_CAPACITY);
}

void order_metadata_map_free(struct order_metadata_map* map) {
  free(map->table);
}

struct order_metadata order_metadata_map_put(
    struct order_metadata_map* map,
    uint64_t order_id,
    struct order_metadata* order_metadata) {
  int i = _order_metadata_map_probe(map, order_id);
  if (i >= 0) {  // found existing, update
    struct order_metadata previous_value = *map->table[i].value;
    map->table[i] = (struct order_metadata_map_entry){.key = order_id,
                                                      .value = order_metadata};
    return previous_value;
  }
  map->table[-(i + 1)] = (struct order_metadata_map_entry){
      .key = order_id, .value = order_metadata};
  map->size++;  // increment the current size

  // Since linear probing works best when the load factor is low, we
  // _order_metadata_map_resize the table once the load factor exceeds this
  // specified threshold.
  uint32_t load_factor = (map->size * 100) / map->capacity;
  if (load_factor > map->max_load_factor)
    _order_metadata_map_resize(map, 2 * map->capacity - 1);

  return order_metadata_default();
}

struct order_metadata order_metadata_map_get(struct order_metadata_map* map,
                                             uint64_t order_id) {
  int i = _order_metadata_map_probe(map, order_id);
  if (i < 0)  // unable to find the key
    return order_metadata_default();
  return *map->table[i].value;
}

struct order_metadata* order_metadata_map_get_mut(
    struct order_metadata_map* map,
    uint64_t order_id) {
  int i = _order_metadata_map_probe(map, order_id);
  if (i < 0)  // unable to find the key
    return NULL;
  return map->table[i].value;
}

struct order_metadata* order_metadata_map_remove(struct order_metadata_map* map,
                                                 uint64_t order_id) {
  int i = _order_metadata_map_probe(map, order_id);
  if (i < 0)  // not able to find the element to remove
    return NULL;
  struct order_metadata* removed = map->table[i].value;
  map->table[i] = (struct order_metadata_map_entry){.key = DEFAULT_EMPTY_KEY};
  map->size--;
  return removed;
}

/**
 * Hash function to get the index of the _order_metadata_map_hash table given a
 * key. Note that this function uses a combination of the MAD
 * (Multiply-Add-and-Divide) and division compression method:
 *
 * MAD: (ka + b) mod p
 *
 * where:
 * k = key
 * a = scale
 * b = shift
 * p = a prime number
 *
 * Division: k mod N
 *
 * where:
 * k = key
 * N = capacity
 *
 * The reason we combine them two because MAD requires that N to be a prime
 * number but our capacity is not necessarily a prime number. To summarize,
 * the function is described as:
 *
 * _order_metadata_map_hash: ((ka + b) mod p) mod N
 */
size_t _order_metadata_map_hash(struct order_metadata_map* map, uint64_t key) {
  return (abs(key * map->scale + map->shift) % map->prime) % map->capacity;
}

/**
 * Search the table in a cyclic fashion to find if a key exist. The function
 * returns positive integer if a matching key is found, the returned value is
 * the index. Otherwise, it returns a negative integer indicating a free slot in
 * the table.
 */
int _order_metadata_map_probe(struct order_metadata_map* map, uint64_t key) {
  int free = -1;
  size_t _hash = _order_metadata_map_hash(map, key);
  size_t i = _hash;

  do {
    if (map->table[i].key == DEFAULT_EMPTY_KEY) {
      free = i;
      break;
    } else if (map->table[i].key == key)
      return i;

    i = (i + 1) % map->capacity;
  } while (i != _hash);

  return -(free + 1);
}

void _order_metadata_map_resize(struct order_metadata_map* map,
                                uint32_t capacity) {
  // Make a copy of the existing table
  struct order_metadata_map_entry* temp =
      malloc(sizeof(struct order_metadata_map_entry) * map->capacity);
  memcpy(temp, map->table,
         sizeof(struct order_metadata_map_entry) * map->capacity);
  free(map->table);

  // Replace the old capacity with new capacity
  uint32_t old_capacity = map->capacity;
  map->capacity = capacity;

  // Copy back the items in temp into the new
  map->table = malloc(sizeof(struct order_metadata_map_entry) * capacity);
  memcpy(map->table, temp,
         sizeof(struct order_metadata_map_entry) * old_capacity);
  for (int i = old_capacity; i < capacity; i++)
    map->table[i].key = DEFAULT_EMPTY_KEY;

  free(temp);
}