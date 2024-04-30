#include <stdio.h>

#include "orderbook.h"

int main(int argc, char** argv) {
  struct orderbook ob = orderbook_new();
  orderbook_limit(
      &ob,
      (struct order){.side = SIDE_BID, .order_id = 0, .price = 10, .size = 1});

  return 0;
}