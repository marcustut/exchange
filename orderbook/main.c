#include <stdio.h>

#include "orderbook.h"
#include "price_limit_map.h"

int main(int argc, char** argv) {
  orderbook ob = orderbook_new();
  orderbook_limit(
      &ob, (order){.side = SIDE_BID, .order_id = 0, .price = 10, .size = 1});

  // price_limit_map map = price_limit_map_new();

  // printf("size: %d\n", map.size);
  // limit to_insert = (limit){.price = 100};
  // printf("to_insert - price: %ld, volume: %ld\n", to_insert.price,
  //        to_insert.volume);
  // price_limit_map_put(&map, 10, &to_insert);
  // printf("size: %d\n", map.size);

  // limit* first = price_limit_map_get(&map, 10);
  // printf("first - price: %ld, volume: %ld\n", first->price, first->volume);

  // price_limit_map_remove(&map, 10);
  // printf("size: %d\n", map.size);

  return 0;
}