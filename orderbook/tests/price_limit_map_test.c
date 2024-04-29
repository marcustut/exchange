#include <criterion/criterion.h>

#include "price_limit_map.h"

price_limit_map map;

void setup(void) {
  map = price_limit_map_new();
}

void teardown(void) {
  price_limit_map_free(&map);
}

Test(put_get_remove, pass, .init = setup, .fini = teardown) {
  cr_assert_eq(map.size, 0);

  limit to_put = {.price = 1000, .volume = 5};
  price_limit_map_put(&map, 1000, to_put);
  cr_assert_eq(map.size, 1);

  limit got = price_limit_map_get(&map, 1000);
  cr_assert_eq(to_put.price, got.price);

  limit removed = price_limit_map_remove(&map, 1000);
  cr_assert_eq(to_put.price, removed.price);
  cr_assert_eq(map.size, 0);
}

Test(update_existing, pass, .init = setup, .fini = teardown) {
  cr_assert_eq(map.size, 0);

  limit to_put = {.price = 1000, .volume = 5};
  price_limit_map_put(&map, 1000, to_put);
  cr_assert_eq(map.size, 1);

  limit got = price_limit_map_get(&map, 1000);
  cr_assert_eq(to_put.price, got.price);

  limit to_update = {.price = 1000, .volume = 10};
  price_limit_map_put(&map, 1000, to_update);
  cr_assert_eq(map.size, 1);

  got = price_limit_map_get(&map, 1000);
  cr_assert_eq(to_update.price, got.price);
}

Test(resize, pass, .fini = teardown) {
  uint32_t capacity = 3;
  map = price_limit_map_with_capacity(capacity);
  cr_assert_eq(map.size, 0);
  cr_assert_eq(map.capacity, capacity);

  price_limit_map_put(&map, 1000, (limit){.price = 1000, .volume = 5});
  cr_assert_eq(map.size, 1);
  cr_assert_eq(map.capacity, capacity);

  price_limit_map_put(&map, 1001, (limit){.price = 1001, .volume = 5});
  cr_assert_eq(map.size, 2);
  cr_assert_eq(map.capacity, 2 * capacity - 1);
}