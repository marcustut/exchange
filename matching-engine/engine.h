#ifndef ENGINE_H
#define ENGINE_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include <cjson/cJSON.h>

#include "orderbook.h"
#include "publisher.h"
#include "utils.h"

typedef struct {
  enum side_e side;
  int64_t price;  // If price is 0, then it's a taker order
  uint32_t quantity;
} order_t;

order_t* order_from_json(cJSON* json) {
  order_t* order = (order_t*)malloc(sizeof(order_t));

  cJSON* side_elem = cJSON_GetObjectItemCaseSensitive(json, "side");
  if (side_elem == NULL || !cJSON_IsString(side_elem)) {
    fprintf(stderr, "[ERROR] side is missing or not a string\n");
    free(order);
    return NULL;
  }

  order->side = side_from_string(side_elem->valuestring);
  if (order->side == -1) {
    fprintf(stderr, "[ERROR] side '%s' is invalid\n", side_elem->valuestring);
    free(order);
    return NULL;
  }

  cJSON* price_elem = cJSON_GetObjectItemCaseSensitive(json, "price");
  if (price_elem == NULL || !cJSON_IsString(price_elem)) {
    fprintf(stderr, "[ERROR] price is missing or not a string\n");
    free(order);
    return NULL;
  }
  order->price = strtoimax(price_elem->valuestring, NULL, 10);

  cJSON* quantity_elem = cJSON_GetObjectItemCaseSensitive(json, "quantity");
  if (quantity_elem == NULL || !cJSON_IsString(quantity_elem)) {
    fprintf(stderr, "[ERROR] quantity is missing or not a string\n");
    free(order);
    return NULL;
  }
  order->quantity =
      strtoumax(cJSON_GetObjectItem(json, "quantity")->valuestring, NULL, 10);

  return order;
}

typedef struct {
  const char* symbol;
  orderbook_t orderbook;
  uint64_t current_order_id;
  uint64_t current_trade_id;
} engine_t;

engine_t engine_new(const char* symbol, const size_t max_depth) {
  return (engine_t){.symbol = symbol,
                    .orderbook = orderbook_new(max_depth),
                    .current_order_id = 0,
                    .current_trade_id = 0};
}

uint64_t engine_next_order_id(engine_t* engine) {
  return ++engine->current_order_id;
}

uint64_t engine_next_trade_id(engine_t* engine) {
  return ++engine->current_trade_id;
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

#define IS_MATCH(side, price, order_price) \
  ((side) == BID ? (price) <= (order_price) : (price) >= (order_price))

  for (size_t i = 0; i < *count; i++) {
    if (order.quantity <= 0 ||
        !IS_MATCH(order.side, levels[i].price, order.price))
      break;

    uint32_t matched_qty = CLAMP(order.quantity, 0, levels[i].quantity);
    order.quantity -= matched_qty;
    levels[i].quantity -= matched_qty;

    // TODO: Publish O1 OrderStatus::Filled /
    // OrderStatus::PartiallyFilled)
    // TODO: Publish O2 OrderStatus::Filled /
    // OrderStatus::PartiallyFilled)

    publish_trade(engine->symbol, engine_next_trade_id(engine), order.side,
                  levels[i].price, matched_qty, timestamp_nanos());

    // Delete if the level is fully filled
    if (levels[i].quantity == 0) {
      for (size_t j = i; j < *count - 1; j++)
        levels[j] = levels[j + 1];
      (*count)--, i--;
    }
  }

  if (order.quantity > 0)
    orderbook_add(ob, order.side, order.price, order.quantity);

#undef IS_MATCH
}

void engine_new_order(engine_t* engine, order_t order) {
  // if the order is a taker order, then use the current best price
  if (order.price == 0) {
    order.price = orderbook_best_price_to_fill(&engine->orderbook, order.side,
                                               order.quantity);

    // the function returns 0 if there is no liquidity
    if (order.price == 0) {
      printf("[DEBUG] No liquidity\n");
      // TODO: Publish OrderStatus::Rejected (no liquidity)
      return;
    }
  }

  // TODO: Publish OrderStatus::Created
  uint64_t order_id = engine_next_order_id(engine);

  engine_match(engine, order);
}

void engine_free(engine_t engine) {
  orderbook_free(engine.orderbook);
  printf("[DEBUG] engine_t has been freed\n");
}

#endif /* ENGINE_H */
