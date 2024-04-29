#include <stdio.h>

#include "orderbook.h"
#include "price_limit_map.h"

int main(int argc, char** argv) {
  // orderbook ob = orderbook_new();
  // orderbook_limit(
  //     &ob, (order){.side = SIDE_BID, .order_id = 0, .price = 10, .size = 1});

  price_limit_map map = price_limit_map_new();

  for (int i = 0; i < 19; i++) {
    printf("before size: %d\n", map.size);
    limit to_insert = (limit){.price = i};
    printf("to_insert - price: %ld, volume: %ld\n", to_insert.price,
           to_insert.volume);
    price_limit_map_put(&map, i, to_insert);

    limit first = price_limit_map_get(&map, i);
    printf("first - price: %ld, volume: %ld\n", first.price, first.volume);
    printf("after size: %d\n", map.size);

    for (int i = 0; i < map.capacity; i++)
      if (map.table[i].key != DEFAULT_EMPTY_KEY)
        printf("%d: key=%ld,price=%ld\n", i, map.table[i].key,
               map.table[i].value.price);

    printf("\n");

    // price_limit_map_remove(&map, i);
    // printf("size: %d\n", map.size);
  }

  price_limit_map_free(&map);

  return 0;
}