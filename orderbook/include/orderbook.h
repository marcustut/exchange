#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <stdint.h>
#include <stdio.h>

#include "limit.h"
#include "price_limit_map.h"

struct limit_tree {
  side side;            // indicate whether bid / ask
  limit* best;          // the best limit level (best bid / ask)
  price_limit_map map;  // keeping track of price levels

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