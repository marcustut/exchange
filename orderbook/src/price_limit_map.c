#include "price_limit_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int _price_limit_map_probe(struct price_limit_map* map, uint64_t key);
void _price_limit_map_resize(struct price_limit_map* map, uint32_t capacity);

struct price_limit_map price_limit_map_with_capacity(uint32_t capacity) {
  srand(time(NULL));
  uint32_t prime = DEFAULT_PRIME_FACTOR;

  // initialise the table to be empty
  struct price_limit_map_entry* table =
      malloc(sizeof(struct price_limit_map_entry) * capacity);
  for (int i = 0; i < capacity; i++)
    table[i].key = DEFAULT_EMPTY_KEY;

  struct price_limit_map map = {.max_load_factor = DEFAULT_MAX_LOAD_FACTOR,
                                .capacity = capacity,
                                .prime = prime,
                                // required to make the bound one less and +1 to
                                // make sure scale % prime != 0
                                .scale = (rand() % (prime - 1)) + 1,
                                .shift = rand() % prime,
                                .table = table};
  return map;
}

struct price_limit_map price_limit_map_new() {
  return price_limit_map_with_capacity(DEFAULT_CAPACITY);
}

void price_limit_map_free(struct price_limit_map* map) {
  free(map->table);
}

struct limit price_limit_map_put(struct price_limit_map* map,
                                 uint64_t price,
                                 struct limit* limit) {
  if (price == TOMBSTONE_KEY) {
    fprintf(stderr, "price cannot be TOMBSTONE_KEY(%ld)\n", TOMBSTONE_KEY);
    exit(EXIT_FAILURE);
  }

  int i = _price_limit_map_probe(map, price);
  if (i >= 0) {  // found existing, update
    struct limit previous_value = *map->table[i].value;
    map->table[i] =
        (struct price_limit_map_entry){.key = price, .value = limit};
    return previous_value;
  }
  map->table[-(i + 1)] =
      (struct price_limit_map_entry){.key = price, .value = limit};
  map->size++;  // increment the current size

  // Since linear probing works best when the load factor is low, we
  // _price_limit_map_resize the table once the load factor exceeds this
  // specified limit.
  uint32_t load_factor =
      ((map->size + map->tombstone_count) * 100) / map->capacity;
  if (load_factor > map->max_load_factor)
    _price_limit_map_resize(map, 2 * map->capacity - 1);

  return limit_default();
}

struct limit price_limit_map_get(struct price_limit_map* map, uint64_t price) {
  int i = _price_limit_map_probe(map, price);
  if (i < 0)  // unable to find the key
    return limit_default();
  return *map->table[i].value;
}

struct limit* price_limit_map_get_mut(struct price_limit_map* map,
                                      uint64_t price) {
  int i = _price_limit_map_probe(map, price);
  if (i < 0)  // unable to find the key
    return NULL;
  return map->table[i].value;
}

struct limit* price_limit_map_remove(struct price_limit_map* map,
                                     uint64_t price) {
  int i = _price_limit_map_probe(map, price);
  if (i < 0)  // not able to find the element to remove
    return NULL;
  struct limit* removed = map->table[i].value;
  map->table[i] = (struct price_limit_map_entry){.key = TOMBSTONE_KEY};
  map->size--;
  map->tombstone_count++;
  return removed;
}

char* price_limit_map_print(struct price_limit_map* map) {
  char* str = malloc(map->capacity * sizeof(char) * 100);
  size_t len = 0;

  len += sprintf(str + len, "{");
  for (int i = 0; i < map->capacity; i++) {
    if (map->table[i].key == DEFAULT_EMPTY_KEY ||
        map->table[i].key == TOMBSTONE_KEY)
      continue;

    len += sprintf(str + len, "%ld[%d]: %ld, ", map->table[i].key, i,
                   map->table[i].value->volume);
  }
  if (len > 2) {  // remove the last ", "
    memmove(&str[len - 2], &str[len], 2);
    len -= 2;
  }
  len += sprintf(str + len, "}");

  return str;
}

/**
 * Search the table in a cyclic fashion to find if a key exist. The function
 * returns positive integer if a matching key is found, the returned value is
 * the index. Otherwise, it returns a negative integer indicating a free slot in
 * the table.
 */
int _price_limit_map_probe(struct price_limit_map* map, uint64_t key) {
  MAP_PROBE(map, key)
}

void _price_limit_map_resize(struct price_limit_map* map, uint32_t capacity) {
  // Make a copy of the existing table
  struct price_limit_map_entry* temp =
      malloc(sizeof(struct price_limit_map_entry) * map->capacity);
  memcpy(temp, map->table,
         sizeof(struct price_limit_map_entry) * map->capacity);
  free(map->table);

  // Replace the old capacity with new capacity
  uint32_t old_capacity = map->capacity;
  map->capacity = capacity;

  // Copy back the items in temp into the new
  map->table = malloc(sizeof(struct price_limit_map_entry) * capacity);
  for (int i = 0; i < capacity; i++)
    map->table[i].key = DEFAULT_EMPTY_KEY;
  map->size = 0;
  for (int i = 0; i < old_capacity; i++)
    if (temp[i].key != DEFAULT_EMPTY_KEY && temp[i].key != TOMBSTONE_KEY)
      price_limit_map_put(map, temp[i].key, temp[i].value);

  map->tombstone_count = 0;  // reset tombstone count

  free(temp);
}