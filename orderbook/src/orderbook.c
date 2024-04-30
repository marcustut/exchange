#include "orderbook.h"

#include <stdlib.h>

struct orderbook orderbook_new() {
  struct limit_tree* bid = malloc(sizeof(struct limit_tree));
  struct limit_tree* ask = malloc(sizeof(struct limit_tree));
  *bid = limit_tree_new(SIDE_BID);
  *ask = limit_tree_new(SIDE_ASK);
  return (struct orderbook){.bid = bid, .ask = ask};
}

void orderbook_free(struct orderbook* ob) {
  limit_tree_free(ob->bid);
  limit_tree_free(ob->ask);
  free(ob->bid);
  free(ob->ask);
}

void orderbook_limit(struct orderbook* ob, struct order _order) {
  struct limit_tree* tree;
  switch (_order.side) {
    case SIDE_BID:
      tree = ob->bid;
      break;
    case SIDE_ASK:
      tree = ob->ask;
      break;
    default:
      fprintf(stderr, "received unrecognised order side");
      exit(1);
  }

  // Make a copy of the order internally
  struct order* order = malloc(sizeof(struct order));
  *order = _order;

  // Check if the price limit exists
  struct limit* found = price_limit_map_get_mut(&tree->map, order->price);

  if (found != NULL && found->order_tail != NULL) {  // limit exist
    // append to the queue
    found->order_tail->next = order;
    found->order_tail = order;
  } else {
    struct limit* limit = malloc(sizeof(struct limit));
    *limit = (struct limit){.price = order->price,
                            .volume = order->size,
                            .order_head = order,
                            .order_tail = order};

    limit_tree_add(tree, limit);                           // add limit to tree
    price_limit_map_put(&tree->map, order->price, limit);  // add limit to map
    limit_tree_update_best(tree, limit);                   // update best limit
  }
}

void orderbook_market() {}
