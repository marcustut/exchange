#include <criterion/criterion.h>

#include "price_limit_map.h"

struct price_limit_map map;

void price_limit_map_setup(void) {
  map = price_limit_map_new();
}

void price_limit_map_teardown(void) {
  price_limit_map_free(&map);
}

Test(price_limit_map,
     put_get_remove,
     .init = price_limit_map_setup,
     .fini = price_limit_map_teardown) {
  cr_assert_eq(map.size, 0);

  struct limit to_put = {.price = 1000, .volume = 5};
  price_limit_map_put(&map, 1000, &to_put);
  cr_assert_eq(map.size, 1);

  struct limit got = price_limit_map_get(&map, 1000);
  cr_assert_eq(to_put.price, got.price);

  struct limit removed = price_limit_map_remove(&map, 1000);
  cr_assert_eq(to_put.price, removed.price);
  cr_assert_eq(map.size, 0);
}

Test(price_limit_map,
     get_mut,
     .init = price_limit_map_setup,
     .fini = price_limit_map_teardown) {
  cr_assert_eq(map.size, 0);

  struct limit to_put = {.price = 1000, .volume = 5};
  price_limit_map_put(&map, 1000, &to_put);
  cr_assert_eq(map.size, 1);

  struct limit* got = price_limit_map_get_mut(&map, 1000);
  cr_assert_eq(to_put.price, got->price);

  got->volume = 10;
  struct limit got2 = price_limit_map_get(&map, 1000);
  cr_assert_eq(got->volume, got2.volume);
}

Test(price_limit_map,
     update_existing,
     .init = price_limit_map_setup,
     .fini = price_limit_map_teardown) {
  cr_assert_eq(map.size, 0);

  struct limit to_put = {.price = 1000, .volume = 5};
  price_limit_map_put(&map, 1000, &to_put);
  cr_assert_eq(map.size, 1);

  struct limit got = price_limit_map_get(&map, 1000);
  cr_assert_eq(to_put.price, got.price);

  struct limit to_update = {.price = 1000, .volume = 10};
  price_limit_map_put(&map, 1000, &to_update);
  cr_assert_eq(map.size, 1);

  got = price_limit_map_get(&map, 1000);
  cr_assert_eq(to_update.price, got.price);
}

Test(price_limit_map, resize, .fini = price_limit_map_teardown) {
  uint32_t capacity = 3;
  map = price_limit_map_with_capacity(capacity);
  cr_assert_eq(map.size, 0);
  cr_assert_eq(map.capacity, capacity);

  price_limit_map_put(&map, 1000, &(struct limit){.price = 1000, .volume = 5});
  cr_assert_eq(map.size, 1);
  cr_assert_eq(map.capacity, capacity);

  price_limit_map_put(&map, 1001, &(struct limit){.price = 1001, .volume = 5});
  cr_assert_eq(map.size, 2);
  cr_assert_eq(map.capacity, 2 * capacity - 1);
}