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

struct orderbook orderbook_new();
void orderbook_free(struct orderbook* ob);
void orderbook_limit(struct orderbook* ob, struct order order);
void orderbook_market();

#endif