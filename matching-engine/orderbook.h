#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

enum side_e { BID, ASK };

enum side_e inverse_side(const enum side_e side) {
  switch (side) {
    case BID:
      return ASK;
    case ASK:
      return BID;
  }
}

const char* to_string(const enum side_e side) {
  switch (side) {
    case BID:
      return "BID";
    case ASK:
      return "ASK";
  }
}

typedef struct {
  int64_t price;
  uint32_t quantity;
} level_t;

typedef struct {
  level_t* bids;
  level_t* asks;
  size_t bids_count;
  size_t asks_count;
  size_t max_depth;
} orderbook_t;

orderbook_t orderbook_new(const size_t max_depth) {
  return (orderbook_t){
      .bids = (level_t*)malloc(sizeof(level_t) * max_depth),
      .asks = (level_t*)malloc(sizeof(level_t) * max_depth),
      .bids_count = 0,
      .asks_count = 0,
      .max_depth = max_depth,
  };
}

void orderbook_free(orderbook_t orderbook) {
  free(orderbook.bids);
  free(orderbook.asks);
}

void orderbook_add(orderbook_t* orderbook,
                   const enum side_e side,
                   int64_t price,
                   uint32_t quantity) {
  size_t* count = NULL;
  level_t* levels = NULL;

  switch (side) {
    case BID:
      count = &orderbook->bids_count;
      levels = orderbook->bids;
      break;
    case ASK:
      count = &orderbook->asks_count;
      levels = orderbook->asks;
      break;
  }

  if (*count >= orderbook->max_depth)
    return;

  level_t level = (level_t){.price = price, .quantity = quantity};
  size_t i = *count - 1;

  switch (side) {
    case BID:
      if (*count == 0)
        levels[*count] = level;
      else if (level.price < levels[i].price)
        levels[*count] = level;
      else if (level.price > levels[i].price) {
        level_t old_level = levels[i];
        levels[i] = level;
        levels[*count] = old_level;
      } else {
        levels[i].quantity += level.quantity;
        return;
      }
      break;
    case ASK:
      if (*count == 0)
        levels[*count] = level;
      else if (level.price > levels[i].price)
        levels[*count] = level;
      else if (level.price < levels[i].price) {
        level_t old_level = levels[i];
        levels[i] = level;
        levels[*count] = old_level;
      } else {
        levels[i].quantity += level.quantity;
        return;
      }
      break;
  }

  // increment count
  *count = *count + 1;
}

level_t* orderbook_top_n(orderbook_t* orderbook,
                         const enum side_e side,
                         const size_t count) {
  size_t size = 0;
  level_t* levels = NULL;

  switch (side) {
    case BID:
      size = MIN(count, orderbook->bids_count);
      levels = orderbook->bids;
      break;
    case ASK:
      size = MIN(count, orderbook->asks_count);
      levels = orderbook->asks;
      break;
  }
  if (size == 0)
    return NULL;

  level_t* top_levels = (level_t*)malloc(sizeof(level_t) * size);

  for (size_t i = 0; i < size; i++)
    top_levels[i] = levels[i];

  return top_levels;
}

void level_print(level_t* level) {
  // printf("%.9f (%.6f)\n", level->price / 1e9, level->quantity / 1e6);
  printf("%.2f (%.3f)\n", level->price / 1e9, level->quantity / 1e6);
}

void orderbook_print(orderbook_t* orderbook) {
  if (orderbook->asks_count == 0 && orderbook->bids_count == 0)
    return;
  for (size_t i = 0; i < orderbook->asks_count; i++)
    level_print(&orderbook->asks[i]);
  printf("-------------------------------------------\n");
  for (size_t i = 0; i < orderbook->bids_count; i++)
    level_print(&orderbook->bids[i]);
  printf("\n");
}

#endif /* ORDERBOOK_H */
