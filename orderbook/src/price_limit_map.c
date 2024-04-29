#include "price_limit_map.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

size_t hash(price_limit_map* map, uint64_t key);
int probe(price_limit_map* map, uint64_t key);
void resize(price_limit_map* map, uint32_t capacity);

price_limit_map price_limit_map_with_capacity(uint32_t capacity) {
  srand(time(NULL));
  uint32_t prime = DEFAULT_PRIME_FACTOR;

  // initialise the table to be empty
  entry* table = malloc(sizeof(entry) * capacity);
  for (int i = 0; i < capacity; i++)
    table[i].key = DEFAULT_EMPTY_KEY;

  price_limit_map map = {.max_load_factor = DEFAULT_MAX_LOAD_FACTOR,
                         .capacity = capacity,
                         .prime = prime,
                         // required to make the bound one less and +1 to make
                         // sure scale % prime != 0
                         .scale = (rand() % (prime - 1)) + 1,
                         .shift = rand() % prime,
                         .table = table};
  return map;
}

price_limit_map price_limit_map_new() {
  return price_limit_map_with_capacity(DEFAULT_CAPACITY);
}

void price_limit_map_free(price_limit_map* map) {
  free(map->table);
}

limit price_limit_map_put(price_limit_map* map, uint64_t price, limit limit) {
  int i = probe(map, price);
  if (i >= 0) {  // found existing, update
    struct limit previous_value = map->table[i].value;
    map->table[i] = (entry){.key = price, .value = limit};
    return previous_value;
  }
  map->table[-(i + 1)] = (entry){.key = price, .value = limit};
  map->size++;  // increment the current size

  // Since linear probing works best when the load factor is low, we resize
  // the table once the load factor exceeds this specified limit.
  uint32_t load_factor = (map->size * 100) / map->capacity;
  if (load_factor > map->max_load_factor)
    resize(map, 2 * map->capacity - 1);

  return limit_default();
}
limit price_limit_map_get(price_limit_map* map, uint64_t price) {
  int i = probe(map, price);
  if (i < 0)  // unable to find the key
    return limit_default();
  return map->table[i].value;
}
limit price_limit_map_remove(price_limit_map* map, uint64_t price) {
  int i = probe(map, price);
  if (i < 0)  // not able to find the element to remove
    return limit_default();
  struct limit removed = map->table[i].value;
  map->table[i] = (entry){.key = DEFAULT_EMPTY_KEY};
  map->size--;
  return removed;
}

/**
 * Hash function to get the index of the hash table given a key.
 * Note that this function uses a combination of the MAD
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
 * hash: ((ka + b) mod p) mod N
 */
size_t hash(price_limit_map* map, uint64_t key) {
  return (abs(key * map->scale + map->shift) % map->prime) % map->capacity;
}

/**
 * Search the table in a cyclic fashion to find if a key exist. The function
 * returns positive integer if a matching key is found, the returned value is
 * the index. Otherwise, it returns a negative integer indicating a free slot in
 * the table.
 */
int probe(price_limit_map* map, uint64_t key) {
  int free = -1;
  size_t _hash = hash(map, key);
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

void resize(price_limit_map* map, uint32_t capacity) {
  // Make a copy of the existing table
  entry* temp = malloc(sizeof(entry) * map->capacity);
  memcpy(temp, map->table, sizeof(entry) * map->capacity);
  free(map->table);

  // Replace the old capacity with new capacity
  uint32_t old_capacity = map->capacity;
  map->capacity = capacity;

  // Copy back the items in temp into the new
  map->table = malloc(sizeof(entry) * capacity);
  memcpy(map->table, temp, sizeof(entry) * old_capacity);
  for (int i = old_capacity; i < capacity; i++)
    map->table[i].key = DEFAULT_EMPTY_KEY;

  free(temp);
}