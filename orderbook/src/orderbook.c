#include "orderbook.h"

#include <stdlib.h>
#include <string.h>

#include "order_metadata.h"

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

void _orderbook_handle_order_event(struct orderbook* ob,
                                   enum order_event_type type,
                                   struct order_event event) {
  if (ob->handler)
    ob->handler->handle_order_event(type, event);
}

struct orderbook orderbook_new() {
  struct limit_tree* bid = malloc(sizeof(struct limit_tree));
  struct limit_tree* ask = malloc(sizeof(struct limit_tree));
  *bid = limit_tree_new(SIDE_BID);
  *ask = limit_tree_new(SIDE_ASK);
  return (struct orderbook){
      .bid = bid, .ask = ask, .order_metadata_map = uint64_hashmap_new()};
}

void orderbook_set_event_handler(struct orderbook* ob,
                                 struct event_handler* handler) {
  ob->handler = handler;
}

void orderbook_free(struct orderbook* ob) {
  limit_tree_free(ob->bid);
  limit_tree_free(ob->ask);
  free(ob->bid);
  free(ob->ask);

  // Free the order metadatas
  for (int i = 0; i < ob->order_metadata_map.capacity; i++)
    if (ob->order_metadata_map.table[i].value != NULL)
      free(ob->order_metadata_map.table[i].value);
  uint64_hashmap_free(&ob->order_metadata_map);
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
  uint64_hashmap_put(&ob->order_metadata_map, order->order_id, order_metadata);

  // Check if the price limit exists
  struct limit* found =
      (struct limit*)uint64_hashmap_get(&tree->price_limit_map, order->price);

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
    uint64_hashmap_put(&tree->price_limit_map, order->price,
                       limit);            // add limit to map
    limit_tree_update_best(tree, limit);  // update best limit
  }

  // Emit an order created event
  _orderbook_handle_order_event(ob, ORDER_EVENT_TYPE_CREATED,
                                (struct order_event){
                                    .order_id = order->order_id,
                                    .side = order->side,
                                    .filled_size = 0,
                                    .cum_filled_size = 0,
                                    .remaining_size = order->size,
                                    .price = order->price,
                                });
}

uint64_t orderbook_market(struct orderbook* ob,
                          const uint64_t order_id,
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

  // reject if no liquidity
  if (tree->best == NULL) {
    _orderbook_handle_order_event(
        ob, ORDER_EVENT_TYPE_REJECTED,
        (struct order_event){.order_id = order_id,
                             .side = side,
                             .filled_size = 0,
                             .cum_filled_size = 0,
                             .remaining_size = size,
                             .price = 0,
                             .reject_reason = REJECT_REASON_NO_LIQUIDITY});
    return size;
  }

  uint64_t cum_filled_size = 0;  // cumulative filled size

  // keep matching until no liquidity left or market order is fulfilled
  while (tree->best != NULL && size > 0) {
    struct order* match = tree->best->order_head;  // always match top in queue
    uint64_t fill_size = MIN(size, match->size);   // fill only available size

    // fill order
    match->size -= fill_size;
    size -= fill_size;
    match->cum_filled_size += fill_size;
    cum_filled_size += fill_size;
    tree->best->volume -= fill_size;  // update limit volume

    // emit order event for order in book
    _orderbook_handle_order_event(
        ob,
        match->size == 0 ? ORDER_EVENT_TYPE_FILLED
                         : ORDER_EVENT_TYPE_PARTIALLY_FILLED,
        (struct order_event){.order_id = match->order_id,
                             .side = match->side,
                             .filled_size = fill_size,
                             .cum_filled_size = match->cum_filled_size,
                             .remaining_size = match->size,
                             .price = match->price});
    // emit order event for the market order
    _orderbook_handle_order_event(
        ob,
        size == 0 ? ORDER_EVENT_TYPE_FILLED : ORDER_EVENT_TYPE_PARTIALLY_FILLED,
        (struct order_event){.order_id = order_id,
                             .side = side,
                             .filled_size = fill_size,
                             .cum_filled_size = cum_filled_size,
                             .remaining_size = size,
                             .price = match->price});

    if (match->size == 0) {  // order in book is fully filled

      if (tree->best->order_count == 1) {  // limit has no other orders

        free(uint64_hashmap_remove(&ob->order_metadata_map,
                                   match->order_id));  // remove order metadata
        uint64_hashmap_remove(&tree->price_limit_map,
                              match->price);  // remove price
        limit_tree_remove(tree, tree->best);  // remove the limit
        limit_tree_update_best(tree, NULL);   // find next best

      } else {  // limit still has other orders

        free(uint64_hashmap_remove(&ob->order_metadata_map,
                                   match->order_id));  // remove order metadata
        tree->best->order_head = match->next;  // replace top with next in queue
        free(match);                           // free filled order
        tree->best->order_head->prev = NULL;   // remove dangling pointer
        tree->best->order_count--;             // decrement limit order count

        // find next best if it was filled
        if (tree->best->order_head == NULL)
          limit_tree_update_best(tree, NULL);
      }
    }
  }

  // not enough liquidity to fulfill market order
  if (size > 0)
    _orderbook_handle_order_event(
        ob, ORDER_EVENT_TYPE_PARTIALLY_FILLED_CANCELLED,
        (struct order_event){.order_id = order_id,
                             .side = side,
                             .filled_size = 0,
                             .cum_filled_size = cum_filled_size,
                             .remaining_size = size,
                             .price = 0});

  return size;
}

enum orderbook_error orderbook_cancel(struct orderbook* ob,
                                      const uint64_t order_id) {
  struct order_metadata* order_metadata =
      (struct order_metadata*)uint64_hashmap_get(&ob->order_metadata_map,
                                                 order_id);
  if (order_metadata == NULL)
    return OBERR_ORDER_NOT_FOUND;

  if (order_metadata->order == NULL) {
    fprintf(stderr, "order is NULL orderbook_cancel: order_id: %ld\n",
            order_id);
    exit(EXIT_FAILURE);
  }
  if (order_metadata->order->limit == NULL) {
    fprintf(stderr, "order->limit is NULL orderbook_cancel: order_id: %ld\n",
            order_id);
    exit(EXIT_FAILURE);
  }
  struct order* order = order_metadata->order;
  struct limit* limit = order->limit;
  struct order_event event = {.order_id = order->order_id,
                              .side = order->side,
                              .filled_size = 0,
                              .cum_filled_size = order->cum_filled_size,
                              .remaining_size = order->size,
                              .price = order->price};

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
    uint64_hashmap_remove(&tree->price_limit_map,
                          limit->price);  // remove price
    limit_tree_remove(tree, limit);       // remove the limit

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

  free(uint64_hashmap_remove(&ob->order_metadata_map,
                             order_id));  // remove order metadata
  _orderbook_handle_order_event(ob, ORDER_EVENT_TYPE_CANCELLED,
                                event);  // emit order cancelled event

  return OBERR_OKAY;
}

enum orderbook_error orderbook_amend_size(struct orderbook* ob,
                                          const uint64_t order_id,
                                          uint64_t size) {
  if (size <= 0)
    return OBERR_INVALID_ORDER_SIZE;

  struct order_metadata* order_metadata =
      (struct order_metadata*)uint64_hashmap_get(&ob->order_metadata_map,
                                                 order_id);
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

// a helper function to traverse the tree reverse in-order
void _orderbook_print_limit_tree(char* str, size_t* len, struct limit* node) {
  if (node == NULL)
    return;

  _orderbook_print_limit_tree(str, len, node->right);
  // *len += sprintf(str + *len, "%ld (%ld) [%ld]\n", node->price, node->volume,
  //                 node->order_count);
  *len += sprintf(str + *len, "%ld (%ld)\n", node->price, node->volume);
  _orderbook_print_limit_tree(str, len, node->left);
}

char* orderbook_print(struct orderbook* ob) {
  uint64_t size = (ob->bid->size + ob->ask->size + 1) * 100;
  char* str = malloc(size * sizeof(char));
  size_t len = 0;

  _orderbook_print_limit_tree(str, &len, ob->ask->root);
  len += sprintf(str + len, "-----------------------\n");
  _orderbook_print_limit_tree(str, &len, ob->bid->root);

  return str;
}