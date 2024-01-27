#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>

#include "orderbook.h"
#include "utils.h"

typedef struct {
  enum side_e side;
  int64_t price;  // If price is 0, then it's a taker order
  uint32_t quantity;
} order_t;

typedef struct {
  orderbook_t orderbook;
  uint64_t current_order_id;
  uint64_t current_trade_id;
  // TODO: Some redis handle that we can use to publish events
} engine_t;

engine_t engine_new(const size_t max_depth) {
  return (engine_t){.orderbook = orderbook_new(max_depth)};
}

uint64_t engine_next_order_id(engine_t* engine) {
  return ++engine->current_order_id;
}

uint64_t engine_next_trade_id(engine_t* engine) {
  return ++engine->current_trade_id;
}

void publish_trade(const uint64_t trade_id,
                   const enum side_e side,
                   const uint64_t price,
                   const uint32_t quantity) {
  printf("[TRADE] %lld: %.2f (%s %.3f) at %lld\n", trade_id, price / 1e9,
         to_string(side), quantity / 1e6, timestamp_nanos());
}

bool is_match(const enum side_e side,
              const int64_t price,
              const int64_t order_price) {
  switch (side) {
    case BID:
      return price <= order_price;
    case ASK:
      return price >= order_price;
  }
}

void engine_match(engine_t* engine, order_t order) {
  orderbook_t* ob = &engine->orderbook;

  size_t* count = NULL;
  level_t* levels = NULL;

  switch (order.side) {
    case BID:
      count = &ob->asks_count;
      levels = ob->asks;
      break;
    case ASK:
      count = &ob->bids_count;
      levels = ob->bids;
      break;
  }

  for (size_t i = 0; i < *count; i++) {
    if (order.quantity <= 0 ||
        !is_match(order.side, levels[i].price, order.price))
      break;

    uint32_t matched_qty = CLAMP(order.quantity, 0, levels[i].quantity);
    order.quantity -= matched_qty;
    levels[i].quantity -= matched_qty;

    // TODO: Publish O1 OrderStatus::Filled /
    // OrderStatus::PartiallyFilled)
    // TODO: Publish O2 OrderStatus::Filled /
    // OrderStatus::PartiallyFilled)

    publish_trade(engine_next_trade_id(engine), order.side, levels[i].price,
                  matched_qty);

    // Clean up levels that have been fully filled
    if (levels[i].quantity == 0) {
      for (size_t j = i; j < *count - 1; j++)
        levels[j] = levels[j + 1];
      (*count)--;
    }
  }

  if (order.quantity > 0)
    orderbook_add(ob, order.side, order.price, order.quantity);
}

void engine_new_order(engine_t* engine, order_t order) {
  // if the order is a taker order, then use the current best price
  if (order.price == 0) {
    level_t* top_level =
        orderbook_top_n(&engine->orderbook, inverse_side(order.side), 1);

    if (top_level == NULL) {
      // TODO: Publish OrderStatus::Rejected (no liquidity)
      return;
    }

    order.price = top_level->price;
    free(top_level);
  }

  // TODO: Publish OrderStatus::Created
  uint64_t order_id = engine_next_order_id(engine);

  engine_match(engine, order);
}

void engine_free(engine_t engine) {
  orderbook_free(engine.orderbook);
}

#endif /* ENGINE_H */
