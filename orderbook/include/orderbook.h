#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <stdint.h>
#include <stdio.h>

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

struct limit_tree {
  side side;    // indicate whether bid / ask
                //   uint32_t depth;  // the current depth (number of limits)
  limit* best;  // the best limit level (best bid / ask)

  // TODO: price_limit_map

  limit* root;  // root of the limit tree
};
typedef struct limit_tree limit_tree;

struct orderbook {
  limit_tree bid;
  limit_tree ask;
  // order_metadata
};
typedef struct orderbook orderbook;

orderbook orderbook_new();
void orderbook_limit(orderbook* ob, order order);
void orderbook_market();

#endif