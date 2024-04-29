#ifndef LIMIT_H
#define LIMIT_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Order side
 */
enum side { SIDE_BID, SIDE_ASK };
typedef enum side side;

/**
 * Order at each limit level
 */
struct order {
  uint64_t order_id;
  uint64_t price;
  uint64_t size;
  side side;

  // orders are organised as a linked list
  struct order* next;
};
typedef struct order order;

struct limit {
  uint64_t price;
  uint64_t volume;
  order* order_head;  // top of the orders (oldest) - first to execute
  order* order_tail;  // end of the orders (newest) - last to execute

  // limits are organised as a binary search tree
  struct limit* left;
  struct limit* right;
};
typedef struct limit limit;

limit limit_default();
bool limit_not_found(limit result);

#endif