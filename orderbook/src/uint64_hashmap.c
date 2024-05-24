#include "uint64_hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int _uint64_hashmap_probe(struct uint64_hashmap* map, uint64_t key);
void _uint64_hashmap_resize(struct uint64_hashmap* map, uint32_t capacity);

struct uint64_hashmap uint64_hashmap_with_capacity(uint32_t capacity) {
  srand(time(NULL));
  uint32_t prime = DEFAULT_PRIME_FACTOR;

  // initialise the table to be empty
  struct uint64_hashmap_entry* table =
      malloc(sizeof(struct uint64_hashmap_entry) * capacity);
  for (int i = 0; i < capacity; i++)
    table[i].key = DEFAULT_EMPTY_KEY;

  struct uint64_hashmap map = {.max_load_factor = DEFAULT_MAX_LOAD_FACTOR,
                               .capacity = capacity,
                               .prime = prime,
                               // required to make the bound one less and +1
                               // to make sure scale % prime != 0
                               .scale = (rand() % (prime - 1)) + 1,
                               .shift = rand() % prime,
                               .table = table};
  return map;
}

struct uint64_hashmap uint64_hashmap_new() {
  return uint64_hashmap_with_capacity(DEFAULT_CAPACITY);
}

void uint64_hashmap_free(struct uint64_hashmap* map) {
  free(map->table);
}

void* uint64_hashmap_put(struct uint64_hashmap* map,
                         uint64_t key,
                         void* value) {
  if (key == TOMBSTONE_KEY) {
    fprintf(stderr, "key cannot be TOMBSTONE_KEY(%ld)\n", TOMBSTONE_KEY);
    exit(EXIT_FAILURE);
  }

  int i = _uint64_hashmap_probe(map, key);
  if (i >= 0) {  // found existing, update
    void* previous_value = map->table[i].value;
    map->table[i] = (struct uint64_hashmap_entry){.key = key, .value = value};
    return previous_value;
  }
  map->table[-(i + 1)] =
      (struct uint64_hashmap_entry){.key = key, .value = value};
  map->size++;  // increment the current size

  // Since linear probing works best when the load factor is low, we
  // _uint64_hashmap_resize the table once the load factor exceeds this
  // specified threshold.
  uint32_t load_factor =
      ((map->size + map->tombstone_count) * 100) / map->capacity;
  if (load_factor > map->max_load_factor)
    _uint64_hashmap_resize(map, 2 * map->capacity - 1);

  return NULL;
}

void* uint64_hashmap_get(struct uint64_hashmap* map, uint64_t key) {
  int i = _uint64_hashmap_probe(map, key);
  if (i < 0)  // unable to find the key
    return NULL;
  return map->table[i].value;
}

void* uint64_hashmap_remove(struct uint64_hashmap* map, uint64_t key) {
  int i = _uint64_hashmap_probe(map, key);
  if (i < 0)  // not able to find the element to remove
    return NULL;
  void* removed = map->table[i].value;
  map->table[i] = (struct uint64_hashmap_entry){.key = TOMBSTONE_KEY};
  map->size--;
  map->tombstone_count++;
  return removed;
}

char* uint64_hashmap_print(struct uint64_hashmap* map) {
  char* str = malloc(map->capacity * sizeof(char) * 100);
  size_t len = 0;

  len += sprintf(str + len, "{");
  for (int i = 0; i < map->capacity; i++) {
    if (map->table[i].key == DEFAULT_EMPTY_KEY ||
        map->table[i].key == TOMBSTONE_KEY)
      continue;

    len += sprintf(str + len, "%ld[%d]: %p, ", map->table[i].key, i,
                   map->table[i].value);
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
int _uint64_hashmap_probe(struct uint64_hashmap* map, uint64_t key) {
  MAP_PROBE(map, key)
}

void _uint64_hashmap_resize(struct uint64_hashmap* map, uint32_t capacity) {
  // Make a copy of the existing table
  struct uint64_hashmap_entry* temp =
      malloc(sizeof(struct uint64_hashmap_entry) * map->capacity);
  memcpy(temp, map->table, sizeof(struct uint64_hashmap_entry) * map->capacity);
  free(map->table);

  // Replace the old capacity with new capacity
  uint32_t old_capacity = map->capacity;
  map->capacity = capacity;

  // Copy back the items in temp into the new
  map->table = malloc(sizeof(struct uint64_hashmap_entry) * capacity);
  for (int i = 0; i < capacity; i++)
    map->table[i].key = DEFAULT_EMPTY_KEY;
  map->size = 0;
  for (int i = 0; i < old_capacity; i++)
    if (temp[i].key != DEFAULT_EMPTY_KEY && temp[i].key != TOMBSTONE_KEY)
      uint64_hashmap_put(map, temp[i].key, temp[i].value);

  map->tombstone_count = 0;  // reset tombstone count

  free(temp);
}