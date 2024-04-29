#include "orderbook.h"

#include <stdio.h>

void limit_tree_update_best(limit_tree*, limit*);

orderbook orderbook_new() {
  orderbook ob = {.bid = (limit_tree){.side = SIDE_BID},
                  .ask = (limit_tree){.side = SIDE_ASK}};
  return ob;
}

void orderbook_limit(orderbook* ob, order order) {
  printf("orderbook_limit\n");

  limit_tree tree;
  switch (order.side) {
    case SIDE_BID:
      tree = ob->bid;
      break;
    case SIDE_ASK:
      tree = ob->ask;
      break;
  }

  // TODO: Check if the price limit exists

  if (0)  // TODO: add order to that limit
    ;
  else {
    limit limit = {.price = order.price,
                   .volume = order.size,
                   .order_head = &order,
                   .order_tail = &order};
    // TODO: add limit to tree
    // TODO: add limit to price_limit_map

    // update best limit
    limit_tree_update_best(&tree, &limit);
  }
}

void orderbook_market() {}

void limit_tree_update_best(limit_tree* tree, limit* limit) {
  printf("limit_tree_update_best\n");

  if (tree->best == NULL) {
    tree->best = limit;
    return;
  }

  switch (tree->side) {
    case SIDE_BID:
      if (limit->price > tree->best->price)
        tree->best = limit;
      break;
    case SIDE_ASK:
      if (limit->price < tree->best->price)
        tree->best = limit;
      break;
  }
}