#include <criterion/criterion.h>

#include "order_metadata_map.h"

struct order_metadata_map om_map;

void order_metadata_map_setup(void) {
  om_map = order_metadata_map_new();
}

void order_metadata_map_teardown(void) {
  order_metadata_map_free(&om_map);
}

Test(order_metadata_map,
     put_get_remove,
     .init = order_metadata_map_setup,
     .fini = order_metadata_map_teardown) {
  cr_assert_eq(om_map.size, 0);

  struct order order = (struct order){.order_id = 1000};
  struct order_metadata to_put = {.order = &order};
  order_metadata_map_put(&om_map, 1000, &to_put);
  cr_assert_eq(om_map.size, 1);

  struct order_metadata got = order_metadata_map_get(&om_map, 1000);
  cr_assert_eq(to_put.order->order_id, got.order->order_id);

  struct order_metadata* removed = order_metadata_map_remove(&om_map, 1000);
  cr_assert_eq(to_put.order->order_id, removed->order->order_id);
  cr_assert_eq(om_map.size, 0);
}

Test(order_metadata_map,
     get_mut,
     .init = order_metadata_map_setup,
     .fini = order_metadata_map_teardown) {
  cr_assert_eq(om_map.size, 0);

  struct order order = (struct order){.order_id = 1000, .size = 1};
  struct order_metadata to_put = {.order = &order};
  order_metadata_map_put(&om_map, 1000, &to_put);
  cr_assert_eq(om_map.size, 1);

  struct order_metadata* got = order_metadata_map_get_mut(&om_map, 1000);
  cr_assert_eq(to_put.order->order_id, got->order->order_id);

  got->order->size = 2;
  struct order_metadata got2 = order_metadata_map_get(&om_map, 1000);
  cr_assert_eq(got->order->size, got2.order->size);
}

Test(order_metadata_map,
     update_existing,
     .init = order_metadata_map_setup,
     .fini = order_metadata_map_teardown) {
  cr_assert_eq(om_map.size, 0);

  struct order order = (struct order){.order_id = 1000, .size = 1};
  struct order_metadata to_put = {.order = &order};
  order_metadata_map_put(&om_map, 1000, &to_put);
  cr_assert_eq(om_map.size, 1);

  struct order_metadata got = order_metadata_map_get(&om_map, 1000);
  cr_assert_eq(to_put.order->order_id, got.order->order_id);

  struct order order2 = (struct order){.order_id = 1000, .size = 2};
  struct order_metadata to_update = {.order = &order2};
  order_metadata_map_put(&om_map, 1000, &to_update);
  cr_assert_eq(om_map.size, 1);

  got = order_metadata_map_get(&om_map, 1000);
  cr_assert_eq(to_update.order->size, got.order->size);
}

Test(order_metadata_map, resize, .fini = order_metadata_map_teardown) {
  uint32_t capacity = 3;
  om_map = order_metadata_map_with_capacity(capacity);
  cr_assert_eq(om_map.size, 0);
  cr_assert_eq(om_map.capacity, capacity);

  struct order order = (struct order){.order_id = 1000, .size = 1};
  order_metadata_map_put(&om_map, 1000,
                         &(struct order_metadata){.order = &order});
  cr_assert_eq(om_map.size, 1);
  cr_assert_eq(om_map.capacity, capacity);

  struct order order2 = (struct order){.order_id = 1000, .size = 2};
  order_metadata_map_put(&om_map, 1001,
                         &(struct order_metadata){.order = &order2});
  cr_assert_eq(om_map.size, 2);
  cr_assert_eq(om_map.capacity, 2 * capacity - 1);
}