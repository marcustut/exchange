#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <stdint.h>
#include <stdio.h>

#include "limit.h"
#include "limit_tree.h"
#include "price_limit_map.h"

struct orderbook {
  struct limit_tree* bid;
  struct limit_tree* ask;

  // order_metadata
};

/**
 * Creates a new orderbook. Note that it is the caller's responsibility to call
 * `orderbook_free()` when the book is no longer in use.
 */
struct orderbook orderbook_new();

/**
 * Deallocates memory used by the given book.
 */
void orderbook_free(struct orderbook* ob);

/**
 * Place a limit order. Always a maker order (adding volume to the book)
 */
void orderbook_limit(struct orderbook* ob, struct order order);

/**
 * Place a market order. Always a taker order (reducing volume from the book)
 */
uint64_t orderbook_market(struct orderbook* ob,
                          const enum side side,
                          uint64_t size);

/**
 * Read the top N bids or asks from the book. Note that the limits will be
 * written to the buffer provided, it is the caller's responsibility to make
 * sure the provided buffer has enough size for N limits.
 *
 * For example, to read the top 5 bids:
 *
 ```
 uint32_t n = 5;
 struct limit* bids = malloc(sizeof(struct limit) * n);
 orderbook_top_n(&ob, SIDE_BID, n, bids);
 ```
 */
void orderbook_top_n(struct orderbook* ob,
                     const enum side side,
                     const uint32_t n,
                     struct limit* buffer);

#endif