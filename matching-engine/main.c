#include "engine.h"

#define MAX_DEPTH 65535

int main(void) {
  engine_t engine = engine_new(MAX_DEPTH);

  // Maker order
  engine_new_order(&engine, (order_t){.side = BID, .price = 1e12, .quantity = 1e3});

  orderbook_print(&engine.orderbook);

  // Taker order
  engine_new_order(&engine, (order_t){.side = ASK, .price = 0, .quantity = 1e3});

  orderbook_print(&engine.orderbook);

  /* orderbook_t orderbook = orderbook_new(100); */

  /* orderbook_add(&orderbook, BID, 1e12, 1e3); */
  /* orderbook_add(&orderbook, BID, 1e12, 1e3); */
  /* orderbook_add(&orderbook, BID, 1e12, 1e3); */
  /* orderbook_add(&orderbook, BID, 2e12, 1e3); */
  /* orderbook_add(&orderbook, ASK, 1e12, 1e3); */

  /* orderbook_print(&orderbook); */

  /* level_t *top_bids = orderbook_top_n(&orderbook, BID, 2); */

  /* orderbook_free(orderbook); */
  engine_free(engine);

  return 0;
}
