#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

enum side_e { BID, ASK };

enum side_e* side_from_string(const char* side_str) {
  enum side_e* side = (enum side_e*)malloc(sizeof(enum side_e));
  if (strcmp(side_str, "BUY") == 0)
    *side = BID;
  else if (strcmp(side_str, "SELL") == 0)
    *side = ASK;
  else {
    free(side);
    return NULL;
  }

  return side;
}

enum side_e side_inverse(const enum side_e side) {
  switch (side) {
    case BID:
      return ASK;
    case ASK:
      return BID;
  }
}

const char* side_to_string(const enum side_e side) {
  switch (side) {
    case BID:
      return "BUY";
    case ASK:
      return "SELL";
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
                   const int64_t price,
                   const uint32_t quantity) {
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

  if (*count >= orderbook->max_depth) {
    log_error("Failed to add to orderbook, it is full");
    return;
  }

  level_t level = (level_t){.price = price, .quantity = quantity};
  int i = *count - 1;

  // If the orderbook is empty, just add the level and return
  if (*count == 0) {
    levels[*count] = level;
    *count = *count + 1;
    return;
  }

// This macro finds the position of the level in the orderbook
#define FIND_POS(side, price, level_price) \
  ((side) == BID ? (price) < (level_price) : (price) > (level_price))

  while (i >= 0) {
    // If the level already exists, just add the quantity
    if (level.price == levels[i].price) {
      levels[i].quantity += level.quantity;
      return;
    }

    if (FIND_POS(side, level.price, levels[i].price))
      levels[i + 1] = level;
    // if can't find the position, keep shifting the levels until the position
    // is right
    else {
      level_t old_level = levels[i];
      levels[i] = level;
      levels[i + 1] = old_level;
    }

    i--;
  }

  (*count)++;  // increment count since a new level is added
#undef FIND_POS
}

/**
 * Finds the best price to fill the given quantity.
 * Note that this function returns `0` if no price is found.
 *
 * @param orderbook The orderbook
 * @param side The side of the order
 * @param quantity The quantity to fill
 */
int64_t orderbook_best_price_to_fill(const orderbook_t* orderbook,
                                     const enum side_e side,
                                     const uint32_t quantity) {
  size_t size = 0;
  level_t* levels = NULL;

  switch (side) {
    case BID:
      size = orderbook->asks_count;
      levels = orderbook->asks;
      break;
    case ASK:
      size = orderbook->bids_count;
      levels = orderbook->bids;
      break;
  }

  int64_t remaining_qty = quantity;

  for (size_t i = 0; i < size; i++) {
    remaining_qty -= levels[i].quantity;
    if (remaining_qty <= 0)
      return levels[i].price;
  }

  return 0;
}

level_t* orderbook_top_n(const orderbook_t* orderbook,
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

void level_print(const enum side_e side, const level_t* level) {
  int color = side == BID ? 32 /* GREEN */ : 31 /* RED */;
  // printf("%.9f (%.6f)\n", level->price / 1e9, level->quantity / 1e6);
  printf("| \033[1;%dm%.2f (%.3f)\033[1;0m |\n",
         color,
         level->price / 1e9,
         level->quantity / 1e6);
}

void orderbook_print(const orderbook_t* orderbook) {
  if (orderbook->asks_count == 0 && orderbook->bids_count == 0)
    return;
  printf("-------------------\n");
  if (orderbook->asks_count == 0)
    printf("| NO ASKS         |\n");
  for (int i = orderbook->asks_count - 1; i >= 0; i--)
    level_print(ASK, &orderbook->asks[i]);
  printf("-------------------\n");
  if (orderbook->bids_count == 0)
    printf("| NO BIDS         |\n");
  for (size_t i = 0; i < orderbook->bids_count; i++)
    level_print(BID, &orderbook->bids[i]);
  printf("-------------------\n");
  printf("\n");
}

#endif /* ORDERBOOK_H */
