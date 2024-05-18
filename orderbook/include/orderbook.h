#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <stdint.h>
#include <stdio.h>

#include "limit.h"
#include "limit_tree.h"
#include "order_metadata_map.h"
#include "price_limit_map.h"

enum orderbook_error {
  OBERR_OKAY = 0,                 // Successful
  OBERR_ORDER_NOT_FOUND = -1,     // Order is not found
  OBERR_INVALID_ORDER_SIZE = -2,  // Order size <= 0
};

struct orderbook {
  struct limit_tree* bid;
  struct limit_tree* ask;
  struct order_metadata_map map;
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
 * Place a limit order. Always a maker order (adding volume to the book).
 */
void orderbook_limit(struct orderbook* ob, struct order order);

/**
 * Place a market order. Always a taker order (reducing volume from the book).
 *
 * Note that the function returns a `uint64_t` representing the reamining size
 * that has not been filled. If the market request has been fully filled then
 * `0` will be returned.
 */
uint64_t orderbook_market(struct orderbook* ob,
                          const enum side side,
                          uint64_t size);

/**
 * Cancel a limit order.
 *
 * `OBERR_OKAY` - successful operation.
 * `OBERR_ORDER_NOT_FOUND` - order id does not exist.
 */
enum orderbook_error orderbook_cancel(struct orderbook* ob,
                                      const uint64_t order_id);

/**
 * Amend a limit order.
 *
 * `OBERR_OKAY` - successful operation.
 * `OBERR_ORDER_NOT_FOUND` - order id does not exist.
 * `OBERR_INVALID_ORDER_SIZE` - new order size <= 0
 */
enum orderbook_error orderbook_amend_size(struct orderbook* ob,
                                          const uint64_t order_id,
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

/**
 * Prints the orderbook state, it will allocate a string. Once it returns,
 * it is the caller's responsibility to deallocate it after use with `free`.
 */
char* orderbook_print(struct orderbook* ob);

#endif