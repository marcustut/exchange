#include "orderbook.h"

#include <stdlib.h>

#define MIN(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })

#define MAX(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })

#define CLAMP(x, min, max) (MIN(max, MAX(x, min)))

struct orderbook orderbook_new() {
  struct limit_tree* bid = malloc(sizeof(struct limit_tree));
  struct limit_tree* ask = malloc(sizeof(struct limit_tree));
  *bid = limit_tree_new(SIDE_BID);
  *ask = limit_tree_new(SIDE_ASK);
  return (struct orderbook){
      .bid = bid, .ask = ask, .map = order_metadata_map_new()};
}

void orderbook_free(struct orderbook* ob) {
  limit_tree_free(ob->bid);
  limit_tree_free(ob->ask);
  free(ob->bid);
  free(ob->ask);

  // Free the order metadatas
  for (int i = 0; i < ob->map.capacity; i++)
    if (ob->map.table[i].key != DEFAULT_EMPTY_KEY)
      free(ob->map.table[i].value);
  order_metadata_map_free(&ob->map);
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

  // Put the order onto the metadata map
  struct order_metadata* order_metadata = malloc(sizeof(struct order_metadata));
  *order_metadata = (struct order_metadata){.order = order};
  order_metadata_map_put(&ob->map, order->order_id, order_metadata);

  // Check if the price limit exists
  struct limit* found = price_limit_map_get_mut(&tree->map, order->price);

  if (found != NULL && found->order_tail != NULL) {
    found->order_tail->next = order;  // append order to queue
    order->prev = found->order_tail;  // backlink to previous order
    found->order_tail = order;        // make order last in queue
    order->limit = found;             // backlink to containing limit
    found->order_count++;             // increment order count
    found->volume += order->size;     // increment limit volume
  } else {
    // Make a new limit on the heap, will be deallocated in `limit_tree_free()`
    struct limit* limit = malloc(sizeof(struct limit));
    *limit = (struct limit){.price = order->price,
                            .volume = order->size,
                            .order_head = order,
                            .order_tail = order,
                            .order_count = 1};

    order->limit = limit;         // backlink to containing limit
    limit_tree_add(tree, limit);  // add limit to tree
    price_limit_map_put(&tree->map, order->price, limit);  // add limit to map
    limit_tree_update_best(tree, limit);                   // update best limit
  }
}

uint64_t orderbook_market(struct orderbook* ob,
                          const enum side side,
                          uint64_t size) {
  struct limit_tree* tree;
  switch (side) {
    case SIDE_BID:
      tree = ob->ask;
      break;
    case SIDE_ASK:
      tree = ob->bid;
      break;
    default:
      fprintf(stderr, "received unrecognised order side");
      exit(1);
  }

  while (tree->best != NULL && size > 0) {
    if (tree->best->order_head == NULL) {
      limit_tree_update_best(tree, NULL);  // find next best
      if (tree->best == NULL || tree->best->order_head == NULL)  // no liquidity
        return size;
    }

    struct order* match = tree->best->order_head;  // always match top in queue
    uint64_t fill_size = MIN(size, match->size);   // fill only available size

    // fill order
    match->size -= fill_size;
    size -= fill_size;

    // order is fully filled and limit has no other orders
    if (match->size == 0 && tree->best->order_count == 1) {
      // TODO: emit event
      printf("%ld of order %ld is fully filled at %ld.\n", fill_size,
             match->order_id, match->price);

      free(order_metadata_map_remove(
          &ob->map, match->order_id));      // remove order metadata
      limit_tree_remove(tree, tree->best);  // remove the limit
      limit_tree_update_best(tree, NULL);   // find next best
      continue;
    }

    // order is fully filled but limit still has other orders
    if (match->size == 0) {
      // TODO: emit event
      printf("%ld of order %ld is fully filled at %ld.\n", fill_size,
             match->order_id, match->price);

      free(order_metadata_map_remove(
          &ob->map, match->order_id));       // remove order metadata
      tree->best->order_head = match->next;  // replace top with next in queue
      free(match);                           // free filled order
      tree->best->order_head->prev = NULL;   // remove dangling pointer
      tree->best->order_count--;             // decrement limit order count

    } else {  // order is partially filled
      // TODO: emit event
      printf("%ld of order %ld is partially filled at %ld.\n", fill_size,
             match->order_id, match->price);
    }

    tree->best->volume -= fill_size;  // update limit volume
  }

  // TODO: emit event
  // printf("%ld of %ld is fully filled at %ld.\n", fill_size, match->order_id,
  //        match->price);

  return size;
}

enum orderbook_error orderbook_cancel(struct orderbook* ob,
                                      const uint64_t order_id) {
  struct order_metadata* order_metadata =
      order_metadata_map_get_mut(&ob->map, order_id);
  if (order_metadata == NULL)
    return OBERR_ORDER_NOT_FOUND;

  struct order* order = order_metadata->order;
  struct limit* limit = order->limit;

  struct limit_tree* tree;
  switch (order->side) {
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

  if (limit->order_count == 1) {  // only order in the limit

    bool is_best = tree->best == limit;  // check if limit is best
    limit_tree_remove(tree, limit);      // remove the limit

    if (is_best)                           // if the limit is previous best
      limit_tree_update_best(tree, NULL);  // find next best

  } else {  // has other orders in the limit

    if (order == limit->order_head) {   // order is head
      limit->order_head = order->next;  // replace head with next in queue
      limit->order_head->prev = NULL;   // remove dangling pointer

    } else if (order == limit->order_tail) {  // order is tail
      limit->order_tail = order->prev;        // replace tail with prev in queue
      limit->order_tail->next = NULL;         // remove dangling pointer

    } else {                            // order is middle
      order->prev->next = order->next;  // replace next with order next
      order->next->prev = order->prev;  // replace prev with order prev
    }

    limit->volume -= order->size;  // decrease total limit volume
    limit->order_count--;          // decrement order count
    free(order);                   // free cancelled order
  }

  free(order_metadata_map_remove(&ob->map, order_id));  // remove order metadata
  return OBERR_OKAY;
}

enum orderbook_error orderbook_amend_size(struct orderbook* ob,
                                          const uint64_t order_id,
                                          uint64_t size) {
  if (size <= 0)
    return OBERR_INVALID_ORDER_SIZE;

  struct order_metadata* order_metadata =
      order_metadata_map_get_mut(&ob->map, order_id);
  if (order_metadata == NULL)
    return OBERR_ORDER_NOT_FOUND;

  struct order* order = order_metadata->order;
  struct limit* limit = order->limit;

  limit->volume += size - order->size;
  order->size = size;

  return OBERR_OKAY;
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