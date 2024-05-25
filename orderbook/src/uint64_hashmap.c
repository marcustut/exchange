#include "uint64_hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint64_t uint64_hash(uint64_t key) {
  key = (key ^ (key >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
  key = (key ^ (key >> 27)) * UINT64_C(0x94d049bb133111eb);
  key = key ^ (key >> 31);
  return key;
}

uint32_t find_next_positive_power_of_two(uint32_t value) {
  uint8_t num_leading_zero_bits = 0;
  value--;

  while ((value & (1 << (UINT32_WIDTH - 1))) == 0) {
    num_leading_zero_bits++;
    value <<= 1;
  }

  return 1 << (UINT32_WIDTH - num_leading_zero_bits);
}

void _uint64_hashmap_compact_chain(struct uint64_hashmap* map,
                                   uint64_t delete_index);
void _uint64_hashmap_resize(struct uint64_hashmap* map, uint32_t capacity);

struct uint64_hashmap uint64_hashmap_with_capacity(uint32_t capacity) {
  capacity = find_next_positive_power_of_two(capacity);

  // initialise the table to be empty
  struct uint64_hashmap_entry* table =
      calloc(capacity, sizeof(struct uint64_hashmap_entry));

  struct uint64_hashmap map = {.capacity = capacity, .table = table};
  return map;
}

struct uint64_hashmap uint64_hashmap_new() {
  return uint64_hashmap_with_capacity(UINT64_HASHMAP_DEFAULT_CAPACITY);
}

void uint64_hashmap_free(struct uint64_hashmap* map) {
  free(map->table);
}

void* uint64_hashmap_put(struct uint64_hashmap* map,
                         uint64_t key,
                         void* value) {
  if (value == NULL)
    return NULL;

  const uint64_t mask = map->capacity - 1;
  uint64_t index = uint64_hash(key) & mask;

  void* previous_value;

  while ((previous_value = map->table[index].value) != NULL) {
    if (key == map->table[index].key)
      break;
    index = (index + 1) & mask;
  }

  if (previous_value == NULL) {
    ++map->size;
    map->table[index].key = key;
  }

  map->table[index].value = value;

  if (map->size > (UINT64_HASHMAP_MAX_LOAD_FACTOR * map->capacity) / 100)
    _uint64_hashmap_resize(map, map->capacity << 1);

  return previous_value;
}

void* uint64_hashmap_get(struct uint64_hashmap* map, uint64_t key) {
  const uint64_t mask = map->capacity - 1;
  uint64_t index = uint64_hash(key) & mask;

  void* value;
  while ((value = map->table[index].value) != NULL) {
    if (key == map->table[index].key)
      break;
    index = (index + 1) & mask;
  }

  return value;
}

void* uint64_hashmap_remove(struct uint64_hashmap* map, uint64_t key) {
  const uint64_t mask = map->capacity - 1;
  uint64_t index = uint64_hash(key) & mask;

  void* value;
  while ((value = map->table[index].value) != NULL) {
    if (key == map->table[index].key) {
      map->table[index].value = NULL;
      map->size--;
      _uint64_hashmap_compact_chain(map, index);
      break;
    }
    index = (index + 1) & mask;
  }

  return value;
}

char* uint64_hashmap_print(struct uint64_hashmap* map) {
  char* str = malloc(map->capacity * sizeof(char) * 100);
  size_t len = 0;

  len += sprintf(str + len, "{");
  for (int i = 0; i < map->capacity; i++) {
    if (map->table[i].value == NULL)
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

void _uint64_hashmap_compact_chain(struct uint64_hashmap* map,
                                   uint64_t delete_index) {
  const uint64_t mask = map->capacity - 1;
  uint64_t index = delete_index;

  while (true) {
    index = (index + 1) & mask;
    void* value = map->table[index].value;
    if (value == NULL)
      break;

    const uint64_t key = map->table[index].key;
    const uint64_t hash = uint64_hash(key) & mask;

    if ((index < hash && (hash <= delete_index || delete_index <= index)) ||
        (hash <= delete_index && delete_index <= index)) {
      map->table[delete_index].key = key;
      map->table[delete_index].value = value;

      map->table[index].value = NULL;
      delete_index = index;
    }
  }
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
  map->table = calloc(capacity, sizeof(struct uint64_hashmap_entry));
  map->size = 0;
  for (int i = 0; i < old_capacity; i++)
    if (temp[i].value != NULL)
      uint64_hashmap_put(map, temp[i].key, temp[i].value);

  free(temp);
}