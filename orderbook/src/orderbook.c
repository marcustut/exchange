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

  // Make a copy of order on the heap, will be deallocated in `limit_free()`
  struct order* order = malloc(sizeof(struct order));
  *order = _order;

  // Check if the price limit exists
  struct limit* found = price_limit_map_get_mut(&tree->map, order->price);

  if (found != NULL && found->order_tail != NULL) {
    found->order_tail->next = order;  // append order to queue
    found->order_tail = order;        // make order last in queue
  } else {
    // Make a new limit on the heap, will be deallocated in `limit_tree_free()`
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

void orderbook_market(struct orderbook* ob,
                      const enum side side,
                      uint64_t size) {
  struct limit_tree* tree;
  switch (side) {
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
}

/**
 * Traverse (post-order) the limit tree to find the top n bids.
 */
void _orderbook_top_n_bid(struct limit* node,
                          uint32_t* i,
                          const uint32_t n,
                          struct limit* buffer) {
  if (node == NULL)
    return;

  _orderbook_top_n_bid(node->right, i, n, buffer);

  if (*i < n)
    buffer[(*i)++] = *node;

  _orderbook_top_n_bid(node->left, i, n, buffer);
}

/**
 * Traverse (in-order) the limit tree to find the top n asks.
 */
void _orderbook_top_n_ask(struct limit* node,
                          uint32_t* i,
                          const uint32_t n,
                          struct limit* buffer) {
  if (node == NULL)
    return;

  _orderbook_top_n_ask(node->left, i, n, buffer);

  if (*i < n)
    buffer[(*i)++] = *node;

  _orderbook_top_n_ask(node->right, i, n, buffer);
}

void orderbook_top_n(struct orderbook* ob,
                     const enum side side,
                     const uint32_t n,
                     struct limit* buffer) {
  uint32_t i = 0;

  switch (side) {
    case SIDE_BID:
      _orderbook_top_n_bid(ob->bid->root, &i, n, buffer);
      break;
    case SIDE_ASK:
      _orderbook_top_n_ask(ob->ask->root, &i, n, buffer);
      break;
    default:
      fprintf(stderr, "received unrecognised order side");
      exit(1);
  }
}