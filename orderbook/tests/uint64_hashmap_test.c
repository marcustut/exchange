#include <criterion/criterion.h>

#include "uint64_hashmap.h"

struct uint64_hashmap map;

void uint64_hashmap_setup(void) {
  map = uint64_hashmap_new();
}

void uint64_hashmap_teardown(void) {
  uint64_hashmap_free(&map);
}

Test(uint64_hashmap,
     put_get_remove,
     .init = uint64_hashmap_setup,
     .fini = uint64_hashmap_teardown) {
  cr_assert_eq(map.size, 0);

  struct limit to_put = {.price = 1000, .volume = 5};
  uint64_hashmap_put(&map, 1000, &to_put);
  cr_assert_eq(map.size, 1);

  struct limit got = *(struct limit*)uint64_hashmap_get(&map, 1000);
  cr_assert_eq(to_put.price, got.price);

  struct limit* removed = uint64_hashmap_remove(&map, 1000);
  cr_assert_eq(to_put.price, removed->price);
  cr_assert_eq(map.size, 0);
}

Test(uint64_hashmap,
     get_mut,
     .init = uint64_hashmap_setup,
     .fini = uint64_hashmap_teardown) {
  cr_assert_eq(map.size, 0);

  struct limit to_put = {.price = 1000, .volume = 5};
  uint64_hashmap_put(&map, 1000, &to_put);
  cr_assert_eq(map.size, 1);

  struct limit* got = (struct limit*)uint64_hashmap_get(&map, 1000);
  cr_assert_eq(to_put.price, got->price);

  got->volume = 10;
  struct limit got2 = *(struct limit*)uint64_hashmap_get(&map, 1000);
  cr_assert_eq(got->volume, got2.volume);
}

Test(uint64_hashmap,
     update_existing,
     .init = uint64_hashmap_setup,
     .fini = uint64_hashmap_teardown) {
  cr_assert_eq(map.size, 0);

  struct limit to_put = {.price = 1000, .volume = 5};
  uint64_hashmap_put(&map, 1000, &to_put);
  cr_assert_eq(map.size, 1);

  struct limit got = *(struct limit*)uint64_hashmap_get(&map, 1000);
  cr_assert_eq(to_put.price, got.price);

  struct limit to_update = {.price = 1000, .volume = 10};
  uint64_hashmap_put(&map, 1000, &to_update);
  cr_assert_eq(map.size, 1);

  got = *(struct limit*)uint64_hashmap_get(&map, 1000);
  cr_assert_eq(to_update.price, got.price);
}

Test(uint64_hashmap, resize, .fini = uint64_hashmap_teardown) {
  uint32_t capacity = 3;
  map = uint64_hashmap_with_capacity(capacity);
  cr_assert_eq(map.size, 0);
  cr_assert_eq(map.capacity, 1 << 2);

  uint64_hashmap_put(&map, 1000, &(struct limit){.price = 1000, .volume = 5});
  cr_assert_eq(map.size, 1);
  cr_assert_eq(map.capacity, 1 << 2);

  uint64_hashmap_put(&map, 1001, &(struct limit){.price = 1001, .volume = 5});
  cr_assert_eq(map.size, 2);
  cr_assert_eq(map.capacity, 1 << 2);

  uint64_hashmap_put(&map, 1002, &(struct limit){.price = 1002, .volume = 5});
  cr_assert_eq(map.size, 3);
  cr_assert_eq(map.capacity, 1 << 3);
}