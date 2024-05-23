#ifndef LIMIT_H
#define LIMIT_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Order side
 */
enum side { SIDE_BID, SIDE_ASK };

/**
 * Order at each limit level
 */
struct order {
  uint64_t order_id;
  uint64_t price;
  uint64_t size;
  uint64_t cum_filled_size;
  enum side side;
  struct limit* limit;  // backlink to the containing limit

  // orders are organised as a linked list
  struct order* prev;
  struct order* next;
};

/**
 * Limit level identified by price
 */
struct limit {
  uint64_t price;
  uint64_t volume;
  struct order* order_head;  // top of the orders (oldest) - first to execute
  struct order* order_tail;  // end of the orders (newest) - last to execute
  uint64_t order_count;      // total orders in the limit

  // limits are organised as a binary search tree
  struct limit* left;
  struct limit* right;
};

void limit_free(struct limit* limit);
struct limit limit_default();

#endif