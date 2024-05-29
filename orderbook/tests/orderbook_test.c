#include <criterion/criterion.h>
#include <criterion/new/assert.h>

#include "orderbook.h"
#include "tests/orderbook_message.h"

#define DATA "data/l3_orderbook_100k.ndjson"
#define MAX_EVENT 100

struct orderbook ob;
uint64_t current_order_id = 1;
struct events {
  struct event_handler handler;
  struct order_event order_events[MAX_EVENT];
  size_t order_events_len;
  struct trade_event trade_events[MAX_EVENT];
  size_t trade_events_len;
} events;

uint64_t next_order_id() {
  return current_order_id++;
}

void handle_order_event(uint64_t ob_id,
                        struct order_event event,
                        void* user_data) {
  events.order_events[events.order_events_len] = event;
  events.order_events_len++;
}

void handle_trade_event(uint64_t ob_id,
                        struct trade_event event,
                        void* user_data) {
  events.trade_events[events.trade_events_len] = event;
  events.trade_events_len++;
}

void orderbook_setup(void) {
  ob = orderbook_new();
}

void orderbook_setup_with_event_handler(void) {
  ob = orderbook_new();
  events.handler = event_handler_new();
  events.handler.handle_order_event = handle_order_event;
  events.handler.handle_trade_event = handle_trade_event;
  orderbook_set_event_handler(&ob, &events.handler);
  events.order_events_len = 0;
}

void orderbook_teardown(void) {
  orderbook_free(&ob);
}

Test(orderbook,
     limit_new_best,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  cr_assert_eq(ob.bid->price_limit_map.size, 0);

  uint64_t oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid, .price = 10, .size = 1});
  cr_assert_eq(ob.bid->price_limit_map.size, 1);
  cr_assert(ob.bid->best != NULL);
  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->order_head->order_id, oid);
  cr_assert_eq(ob.bid->best->order_tail->order_id, oid);

  oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid, .price = 11, .size = 1});
  cr_assert_eq(ob.bid->price_limit_map.size, 2);
  cr_assert(ob.bid->best != NULL);
  cr_assert_eq(ob.bid->best->price, 11);
  cr_assert_eq(ob.bid->best->order_head->order_id, oid);
  cr_assert_eq(ob.bid->best->order_tail->order_id, oid);

  cr_assert_eq(ob.ask->price_limit_map.size, 0);

  oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_ASK, .order_id = oid, .price = 13, .size = 1});
  cr_assert_eq(ob.ask->price_limit_map.size, 1);
  cr_assert(ob.ask->best != NULL);
  cr_assert_eq(ob.ask->best->price, 13);
  cr_assert_eq(ob.ask->best->order_head->order_id, oid);
  cr_assert_eq(ob.ask->best->order_tail->order_id, oid);

  oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_ASK, .order_id = oid, .price = 12, .size = 1});
  cr_assert_eq(ob.ask->price_limit_map.size, 2);
  cr_assert(ob.ask->best != NULL);
  cr_assert_eq(ob.ask->best->price, 12);
  cr_assert_eq(ob.ask->best->order_head->order_id, oid);
  cr_assert_eq(ob.ask->best->order_tail->order_id, oid);
}

Test(orderbook,
     limit_existing_order,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  cr_assert_eq(ob.bid->price_limit_map.size, 0);

  uint64_t oid0 = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid0, .price = 10, .size = 1});
  cr_assert_eq(ob.bid->price_limit_map.size, 1);
  cr_assert(ob.bid->best != NULL);
  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->order_head->order_id, oid0);
  cr_assert_eq(ob.bid->best->order_tail->order_id, oid0);

  uint64_t oid1 = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid1, .price = 10, .size = 2});
  cr_assert_eq(ob.bid->price_limit_map.size, 1);
  cr_assert(ob.bid->best != NULL);
  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->order_head->order_id, oid0);
  cr_assert_eq(ob.bid->best->order_tail->order_id, oid1);

  uint64_t oid2 = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid2, .price = 10, .size = 3});
  cr_assert_eq(ob.bid->price_limit_map.size, 1);
  cr_assert(ob.bid->best != NULL);
  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->order_head->order_id, oid0);
  cr_assert_eq(ob.bid->best->order_tail->order_id, oid2);
}

Test(orderbook,
     market_has_liquidity,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
#define N 5
  struct order orders[N] = {
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 10,
                     .size = 3},
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 10,
                     .size = 2},
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 10,
                     .size = 1},
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 11,
                     .size = 2},
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 12,
                     .size = 1},
  };

  for (int i = 0; i < N; i++)
    orderbook_limit(&ob, orders[i]);

  cr_assert_eq(ob.bid->best->price, 12);
  cr_assert_eq(ob.bid->best->volume, 1);
  cr_assert_eq(ob.bid->best->order_count, 1);
  cr_assert_eq(ob.bid->size, 3);

  cr_assert_eq(orderbook_execute(&ob, next_order_id(), SIDE_ASK, 1, 1, true),
               0);

  cr_assert_eq(ob.bid->best->price, 11);
  cr_assert_eq(ob.bid->best->volume, 2);
  cr_assert_eq(ob.bid->best->order_count, 1);
  cr_assert_eq(ob.bid->size, 2);

  cr_assert_eq(orderbook_execute(&ob, next_order_id(), SIDE_ASK, 1, 1, true),
               0);

  cr_assert_eq(ob.bid->best->price, 11);
  cr_assert_eq(ob.bid->best->volume, 1);
  cr_assert_eq(ob.bid->best->order_count, 1);
  cr_assert_eq(ob.bid->size, 2);

  cr_assert_eq(orderbook_execute(&ob, next_order_id(), SIDE_ASK, 2, 2, true),
               0);

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 5);
  cr_assert_eq(ob.bid->best->order_count, 3);
  cr_assert_eq(ob.bid->size, 1);

  cr_assert_eq(orderbook_execute(&ob, next_order_id(), SIDE_ASK, 2, 2, true),
               0);

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 3);
  cr_assert_eq(ob.bid->best->order_count, 2);
  cr_assert_eq(ob.bid->size, 1);

  cr_assert_eq(orderbook_execute(&ob, next_order_id(), SIDE_ASK, 3, 3, true),
               0);

  cr_assert_eq(ob.bid->best, NULL);
  cr_assert_eq(ob.bid->size, 0);
}

Test(orderbook,
     market_no_liquidity,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  cr_assert_eq(ob.ask->best, NULL);

  cr_assert_eq(orderbook_execute(&ob, next_order_id(), SIDE_BID, 3, 3, true),
               3);

  cr_assert_eq(ob.ask->best, NULL);

  orderbook_limit(&ob, (struct order){.side = SIDE_ASK,
                                      .order_id = next_order_id(),
                                      .price = 10,
                                      .size = 3});

  cr_assert_eq(ob.ask->best->price, 10);
  cr_assert_eq(ob.ask->best->volume, 3);

  cr_assert_eq(orderbook_execute(&ob, next_order_id(), SIDE_BID, 4, 4, true),
               1);

  cr_assert_eq(ob.ask->best, NULL);
}

Test(orderbook,
     cancel_exist,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
#define N 5
  struct order orders[N] = {
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 10,
                     .size = 3},
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 10,
                     .size = 2},
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 10,
                     .size = 1},
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 11,
                     .size = 2},
      (struct order){.side = SIDE_BID,
                     .order_id = next_order_id(),
                     .price = 12,
                     .size = 1},
  };

  for (int i = 0; i < N; i++)
    orderbook_limit(&ob, orders[i]);

  cr_assert_eq(ob.bid->best->price, 12);
  cr_assert_eq(ob.bid->best->volume, 1);
  cr_assert_eq(ob.bid->best->order_count, 1);
  cr_assert_eq(ob.bid->size, 3);

  cr_assert_eq(orderbook_cancel(&ob, orders[4].order_id), OBERR_OKAY);

  cr_assert_eq(ob.bid->best->price, 11);
  cr_assert_eq(ob.bid->best->volume, 2);
  cr_assert_eq(ob.bid->best->order_count, 1);
  cr_assert_eq(ob.bid->size, 2);

  cr_assert_eq(orderbook_cancel(&ob, orders[3].order_id), OBERR_OKAY);

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 6);
  cr_assert_eq(ob.bid->best->order_count, 3);
  cr_assert_eq(ob.bid->best->order_head->size, 3);
  cr_assert_eq(ob.bid->best->order_head->next->size, 2);
  cr_assert_eq(ob.bid->best->order_head->next->next->size, 1);
  cr_assert_eq(ob.bid->size, 1);

  cr_assert_eq(orderbook_cancel(&ob, orders[0].order_id),
               OBERR_OKAY);  // cancel order head

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 3);
  cr_assert_eq(ob.bid->best->order_count, 2);
  cr_assert_eq(ob.bid->best->order_head->size, 2);
  cr_assert_eq(ob.bid->best->order_head->next->size, 1);
  cr_assert_eq(ob.bid->size, 1);

  cr_assert_eq(orderbook_cancel(&ob, orders[2].order_id),
               OBERR_OKAY);  // cancel order tail

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 2);
  cr_assert_eq(ob.bid->best->order_count, 1);
  cr_assert_eq(ob.bid->best->order_head->size, 2);
  cr_assert_eq(ob.bid->size, 1);

  cr_assert_eq(orderbook_cancel(&ob, orders[1].order_id),
               OBERR_OKAY);  // cancel last order

  cr_assert_eq(ob.bid->best, NULL);
  cr_assert_eq(ob.bid->size, 0);
}

Test(orderbook,
     cancel_not_exist,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  cr_assert_eq(orderbook_cancel(&ob, 1), OBERR_ORDER_NOT_FOUND);
}

Test(orderbook,
     amend_size_exist,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  uint64_t oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid, .price = 10, .size = 3});

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 3);
  cr_assert_eq(ob.bid->best->order_head->size, 3);

  cr_assert_eq(orderbook_amend_size(&ob, oid, 5), OBERR_OKAY);

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 5);
  cr_assert_eq(ob.bid->best->order_head->size, 5);
}

Test(orderbook,
     amend_size_invalid_size,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  uint64_t oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid, .price = 10, .size = 3});

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 3);
  cr_assert_eq(ob.bid->best->order_head->size, 3);

  cr_assert_eq(orderbook_amend_size(&ob, oid, 0), OBERR_INVALID_ORDER_SIZE);

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 3);
  cr_assert_eq(ob.bid->best->order_head->size, 3);
}

Test(orderbook,
     amend_size_not_exist,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  cr_assert_eq(orderbook_amend_size(&ob, 1, 5), OBERR_ORDER_NOT_FOUND);
}

Test(orderbook,
     top_n_bids,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  for (int i = 11; i <= 20; i++)
    orderbook_limit(&ob, (struct order){.side = SIDE_BID,
                                        .order_id = next_order_id(),
                                        .price = i,
                                        .size = 1});

  uint32_t n = 5;
  struct limit* bids1 = malloc(sizeof(struct limit) * n);
  orderbook_top_n(&ob, SIDE_BID, n, bids1);
  for (int i = 0; i < n; i++)
    cr_assert_eq(bids1[i].price, 20 - i);
  free(bids1);

  for (int i = 21; i <= 30; i++)
    orderbook_limit(&ob, (struct order){.side = SIDE_BID,
                                        .order_id = next_order_id(),
                                        .price = i,
                                        .size = 1});

  n = 10;
  struct limit* bids2 = malloc(sizeof(struct limit) * n);
  orderbook_top_n(&ob, SIDE_BID, n, bids2);
  for (int i = 0; i < n; i++)
    cr_assert_eq(bids2[i].price, 30 - i);
  free(bids2);
}

Test(orderbook,
     top_n_asks,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  for (int i = 11; i <= 20; i++)
    orderbook_limit(&ob, (struct order){.side = SIDE_ASK,
                                        .order_id = next_order_id(),
                                        .price = i,
                                        .size = 1});

  uint32_t n = 5;
  struct limit* asks1 = malloc(sizeof(struct limit) * n);
  orderbook_top_n(&ob, SIDE_ASK, n, asks1);
  for (int i = 0; i < n; i++)
    cr_assert_eq(asks1[i].price, 11 + i);
  free(asks1);

  for (int i = 1; i <= 10; i++)
    orderbook_limit(&ob, (struct order){.side = SIDE_ASK,
                                        .order_id = next_order_id(),
                                        .price = i,
                                        .size = 1});

  n = 10;
  struct limit* asks2 = malloc(sizeof(struct limit) * n);
  orderbook_top_n(&ob, SIDE_ASK, n, asks2);
  for (int i = 0; i < n; i++)
    cr_assert_eq(asks2[i].price, 1 + i);
  free(asks2);
}

#define CHECK_FILE(count)                                                      \
  size_t messages_len = get_line_count(DATA);                                  \
  struct message* messages = parse_messages(DATA);                             \
  for (int i = 0; i < messages_len; i++) {                                     \
    struct message message = messages[i];                                      \
    switch (message.message_type) {                                            \
      case MESSAGE_TYPE_CREATED:                                               \
        if (message.price == 0)                                                \
          orderbook_execute(&ob, message.order_id, message.side, message.size, \
                            message.size, true);                               \
        else                                                                   \
          orderbook_limit(&ob, (struct order){.order_id = message.order_id,    \
                                              .side = message.side,            \
                                              .price = message.price,          \
                                              .size = message.size});          \
        break;                                                                 \
      case MESSAGE_TYPE_DELETED:                                               \
        orderbook_cancel(&ob, message.order_id);                               \
        break;                                                                 \
      case MESSAGE_TYPE_CHANGED:                                               \
        orderbook_amend_size(&ob, message.order_id, message.size);             \
        break;                                                                 \
    }                                                                          \
    if (i == count - 1) {                                                      \
      char* state = orderbook_print(&ob);                                      \
      FILE* file = fopen("data/valid_ob_state_" #count ".log", "r");           \
      char* buffer = malloc(strlen(state) * sizeof(char));                     \
      size_t len = fread(buffer, sizeof(char), strlen(state), file);           \
      if (ferror(file) != 0)                                                   \
        fputs("Error reading file", stderr);                                   \
      else                                                                     \
        buffer[len++] = '\0';                                                  \
      cr_assert(eq(str, state, buffer));                                       \
      free(buffer);                                                            \
      fclose(file);                                                            \
      free(state);                                                             \
    }                                                                          \
  }                                                                            \
  free(messages);

Test(orderbook,
     state_after_100_messages,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  CHECK_FILE(100);
}

Test(orderbook,
     state_after_1000_messages,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  CHECK_FILE(1000);
}

Test(orderbook,
     state_after_10000_messages,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  CHECK_FILE(10000);
}

Test(orderbook,
     state_after_100000_messages,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  CHECK_FILE(100000);
}

Test(orderbook,
     order_event_created,
     .init = orderbook_setup_with_event_handler,
     .fini = orderbook_teardown) {
  struct order order = {
      .side = SIDE_BID, .order_id = 1, .price = 10000, .size = 1};
  orderbook_limit(&ob, order);

  cr_assert(eq(events.order_events_len, 1));
  cr_assert(eq(events.order_events[0].status, ORDER_STATUS_CREATED));
  cr_assert(eq(events.order_events[0].order_id, order.order_id));
  cr_assert(eq(events.order_events[0].side, order.side));
  cr_assert(eq(events.order_events[0].price, order.price));
  cr_assert(eq(events.order_events[0].filled_size, 0));
  cr_assert(eq(events.order_events[0].cum_filled_size, 0));
  cr_assert(eq(events.order_events[0].remaining_size, order.size));
}

Test(orderbook,
     order_event_cancelled,
     .init = orderbook_setup_with_event_handler,
     .fini = orderbook_teardown) {
  struct order order = {
      .side = SIDE_BID, .order_id = 1, .price = 10000, .size = 1};
  orderbook_limit(&ob, order);

  cr_assert(eq(events.order_events_len, 1));

  orderbook_cancel(&ob, order.order_id);

  cr_assert(eq(events.order_events_len, 2));

  cr_assert(eq(events.order_events[1].status, ORDER_STATUS_CANCELLED));
  cr_assert(eq(events.order_events[1].order_id, order.order_id));
  cr_assert(eq(events.order_events[1].side, order.side));
  cr_assert(eq(events.order_events[1].price, order.price));
  cr_assert(eq(events.order_events[1].filled_size, 0));
  cr_assert(eq(events.order_events[0].cum_filled_size, 0));
  cr_assert(eq(events.order_events[1].remaining_size, order.size));
}

Test(orderbook,
     order_event_rejected,
     .init = orderbook_setup_with_event_handler,
     .fini = orderbook_teardown) {
  orderbook_execute(&ob, 1, SIDE_ASK, 1, 1, true);

  cr_assert(eq(events.order_events_len, 1));

  cr_assert(eq(events.order_events[0].status, ORDER_STATUS_REJECTED));
  cr_assert(eq(events.order_events[0].order_id, 1));
  cr_assert(eq(events.order_events[0].side, SIDE_ASK));
  cr_assert(eq(events.order_events[0].price, 0));
  cr_assert(eq(events.order_events[0].filled_size, 0));
  cr_assert(eq(events.order_events[0].cum_filled_size, 0));
  cr_assert(eq(events.order_events[0].remaining_size, 1));
  cr_assert(
      eq(events.order_events[0].reject_reason, REJECT_REASON_NO_LIQUIDITY));
}

Test(orderbook,
     order_event_filled,
     .init = orderbook_setup_with_event_handler,
     .fini = orderbook_teardown) {
  struct order order = {
      .side = SIDE_BID, .order_id = 1, .price = 10000, .size = 1};
  orderbook_limit(&ob, order);

  cr_assert(eq(events.order_events_len, 1));

  orderbook_execute(&ob, 2, SIDE_ASK, 1, 1, true);

  cr_assert(eq(events.order_events_len, 4));

  cr_assert(eq(events.order_events[1].status, ORDER_STATUS_CREATED));
  cr_assert(eq(events.order_events[1].order_id, 2));
  cr_assert(eq(events.order_events[1].side, SIDE_ASK));
  cr_assert(eq(events.order_events[1].price, order.price));
  cr_assert(eq(events.order_events[1].filled_size, 0));
  cr_assert(eq(events.order_events[1].cum_filled_size, 0));
  cr_assert(eq(events.order_events[1].remaining_size, order.size));

  cr_assert(eq(events.order_events[2].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[2].order_id, order.order_id));
  cr_assert(eq(events.order_events[2].side, order.side));
  cr_assert(eq(events.order_events[2].price, order.price));
  cr_assert(eq(events.order_events[2].filled_size, order.size));
  cr_assert(eq(events.order_events[2].cum_filled_size, order.size));
  cr_assert(eq(events.order_events[2].remaining_size, 0));

  cr_assert(eq(events.order_events[3].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[3].order_id, 2));
  cr_assert(eq(events.order_events[3].side, SIDE_ASK));
  cr_assert(eq(events.order_events[3].price, order.price));
  cr_assert(eq(events.order_events[3].filled_size, 1));
  cr_assert(eq(events.order_events[3].cum_filled_size, 1));
  cr_assert(eq(events.order_events[3].remaining_size, 0));
}

Test(orderbook,
     order_event_partially_filled,
     .init = orderbook_setup_with_event_handler,
     .fini = orderbook_teardown) {
  struct order order = {
      .side = SIDE_BID, .order_id = 1, .price = 10000, .size = 5};
  orderbook_limit(&ob, order);

  cr_assert(eq(events.order_events_len, 1));

  orderbook_execute(&ob, 2, SIDE_ASK, 1, 1, true);

  cr_assert(eq(events.order_events_len, 4));

  cr_assert(eq(events.order_events[1].status, ORDER_STATUS_CREATED));
  cr_assert(eq(events.order_events[1].order_id, 2));
  cr_assert(eq(events.order_events[1].side, SIDE_ASK));
  cr_assert(eq(events.order_events[1].price, order.price));
  cr_assert(eq(events.order_events[1].filled_size, 0));
  cr_assert(eq(events.order_events[1].cum_filled_size, 0));
  cr_assert(eq(events.order_events[1].remaining_size, 1));

  cr_assert(eq(events.order_events[2].status, ORDER_STATUS_PARTIALLY_FILLED));
  cr_assert(eq(events.order_events[2].order_id, order.order_id));
  cr_assert(eq(events.order_events[2].side, order.side));
  cr_assert(eq(events.order_events[2].price, order.price));
  cr_assert(eq(events.order_events[2].filled_size, 1));
  cr_assert(eq(events.order_events[2].cum_filled_size, 1));
  cr_assert(eq(events.order_events[2].remaining_size, 4));

  cr_assert(eq(events.order_events[3].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[3].order_id, 2));
  cr_assert(eq(events.order_events[3].side, SIDE_ASK));
  cr_assert(eq(events.order_events[3].price, order.price));
  cr_assert(eq(events.order_events[3].filled_size, 1));
  cr_assert(eq(events.order_events[3].cum_filled_size, 1));
  cr_assert(eq(events.order_events[3].remaining_size, 0));

  orderbook_execute(&ob, 3, SIDE_ASK, 2, 2, true);

  cr_assert(eq(events.order_events_len, 7));

  cr_assert(eq(events.order_events[4].status, ORDER_STATUS_CREATED));
  cr_assert(eq(events.order_events[4].order_id, 3));
  cr_assert(eq(events.order_events[4].side, SIDE_ASK));
  cr_assert(eq(events.order_events[4].price, order.price));
  cr_assert(eq(events.order_events[4].filled_size, 0));
  cr_assert(eq(events.order_events[4].cum_filled_size, 0));
  cr_assert(eq(events.order_events[4].remaining_size, 2));

  cr_assert(eq(events.order_events[5].status, ORDER_STATUS_PARTIALLY_FILLED));
  cr_assert(eq(events.order_events[5].order_id, order.order_id));
  cr_assert(eq(events.order_events[5].side, order.side));
  cr_assert(eq(events.order_events[5].price, order.price));
  cr_assert(eq(events.order_events[5].filled_size, 2));
  cr_assert(eq(events.order_events[5].cum_filled_size, 3));
  cr_assert(eq(events.order_events[5].remaining_size, 2));

  cr_assert(eq(events.order_events[6].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[6].order_id, 3));
  cr_assert(eq(events.order_events[6].side, SIDE_ASK));
  cr_assert(eq(events.order_events[6].price, order.price));
  cr_assert(eq(events.order_events[6].filled_size, 2));
  cr_assert(eq(events.order_events[6].cum_filled_size, 2));
  cr_assert(eq(events.order_events[6].remaining_size, 0));

  orderbook_execute(&ob, 4, SIDE_ASK, 2, 2, true);

  cr_assert(eq(events.order_events_len, 10));

  cr_assert(eq(events.order_events[7].status, ORDER_STATUS_CREATED));
  cr_assert(eq(events.order_events[7].order_id, 4));
  cr_assert(eq(events.order_events[7].side, SIDE_ASK));
  cr_assert(eq(events.order_events[7].price, order.price));
  cr_assert(eq(events.order_events[7].filled_size, 0));
  cr_assert(eq(events.order_events[7].cum_filled_size, 0));
  cr_assert(eq(events.order_events[7].remaining_size, 2));

  cr_assert(eq(events.order_events[8].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[8].order_id, order.order_id));
  cr_assert(eq(events.order_events[8].side, order.side));
  cr_assert(eq(events.order_events[8].price, order.price));
  cr_assert(eq(events.order_events[8].filled_size, 2));
  cr_assert(eq(events.order_events[8].cum_filled_size, 5));
  cr_assert(eq(events.order_events[8].remaining_size, 0));

  cr_assert(eq(events.order_events[9].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[9].order_id, 4));
  cr_assert(eq(events.order_events[9].side, SIDE_ASK));
  cr_assert(eq(events.order_events[9].price, order.price));
  cr_assert(eq(events.order_events[9].filled_size, 2));
  cr_assert(eq(events.order_events[9].cum_filled_size, 2));
  cr_assert(eq(events.order_events[9].remaining_size, 0));
}

Test(orderbook,
     order_event_partially_filled_cancelled,
     .init = orderbook_setup_with_event_handler,
     .fini = orderbook_teardown) {
  struct order order1 = {
      .side = SIDE_BID, .order_id = 1, .price = 10000, .size = 5};
  struct order order2 = {
      .side = SIDE_BID, .order_id = 2, .price = 10001, .size = 2};
  orderbook_limit(&ob, order1);
  orderbook_limit(&ob, order2);

  cr_assert(eq(events.order_events_len, 2));

  orderbook_execute(&ob, 3, SIDE_ASK, 1, 1, true);

  cr_assert(eq(events.order_events_len, 5));

  cr_assert(eq(events.order_events[2].status, ORDER_STATUS_CREATED));
  cr_assert(eq(events.order_events[2].order_id, 3));
  cr_assert(eq(events.order_events[2].side, SIDE_ASK));
  cr_assert(eq(events.order_events[2].price, order2.price));
  cr_assert(eq(events.order_events[2].filled_size, 0));
  cr_assert(eq(events.order_events[2].cum_filled_size, 0));
  cr_assert(eq(events.order_events[2].remaining_size, 1));

  cr_assert(eq(events.order_events[3].status, ORDER_STATUS_PARTIALLY_FILLED));
  cr_assert(eq(events.order_events[3].order_id, order2.order_id));
  cr_assert(eq(events.order_events[3].side, order2.side));
  cr_assert(eq(events.order_events[3].price, order2.price));
  cr_assert(eq(events.order_events[3].filled_size, 1));
  cr_assert(eq(events.order_events[3].cum_filled_size, 1));
  cr_assert(eq(events.order_events[3].remaining_size, 1));

  cr_assert(eq(events.order_events[4].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[4].order_id, 3));
  cr_assert(eq(events.order_events[4].side, SIDE_ASK));
  cr_assert(eq(events.order_events[4].price, order2.price));
  cr_assert(eq(events.order_events[4].filled_size, 1));
  cr_assert(eq(events.order_events[4].cum_filled_size, 1));
  cr_assert(eq(events.order_events[4].remaining_size, 0));

  orderbook_execute(&ob, 4, SIDE_ASK, 2, 2, true);

  cr_assert(eq(events.order_events_len, 10));

  cr_assert(eq(events.order_events[5].status, ORDER_STATUS_CREATED));
  cr_assert(eq(events.order_events[5].order_id, 4));
  cr_assert(eq(events.order_events[5].side, SIDE_ASK));
  cr_assert(eq(events.order_events[5].price, order2.price));
  cr_assert(eq(events.order_events[5].filled_size, 0));
  cr_assert(eq(events.order_events[5].cum_filled_size, 0));
  cr_assert(eq(events.order_events[5].remaining_size, 2));

  cr_assert(eq(events.order_events[6].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[6].order_id, order2.order_id));
  cr_assert(eq(events.order_events[6].side, order2.side));
  cr_assert(eq(events.order_events[6].price, order2.price));
  cr_assert(eq(events.order_events[6].filled_size, 1));
  cr_assert(eq(events.order_events[6].cum_filled_size, 2));
  cr_assert(eq(events.order_events[6].remaining_size, 0));

  cr_assert(eq(events.order_events[7].status, ORDER_STATUS_PARTIALLY_FILLED));
  cr_assert(eq(events.order_events[7].order_id, 4));
  cr_assert(eq(events.order_events[7].side, SIDE_ASK));
  cr_assert(eq(events.order_events[7].price, order2.price));
  cr_assert(eq(events.order_events[7].filled_size, 1));
  cr_assert(eq(events.order_events[7].cum_filled_size, 1));
  cr_assert(eq(events.order_events[7].remaining_size, 1));

  cr_assert(eq(events.order_events[8].status, ORDER_STATUS_PARTIALLY_FILLED));
  cr_assert(eq(events.order_events[8].order_id, order1.order_id));
  cr_assert(eq(events.order_events[8].side, order1.side));
  cr_assert(eq(events.order_events[8].price, order1.price));
  cr_assert(eq(events.order_events[8].filled_size, 1));
  cr_assert(eq(events.order_events[8].cum_filled_size, 1));
  cr_assert(eq(events.order_events[8].remaining_size, 4));

  cr_assert(eq(events.order_events[9].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[9].order_id, 4));
  cr_assert(eq(events.order_events[9].side, SIDE_ASK));
  cr_assert(eq(events.order_events[9].price, order1.price));
  cr_assert(eq(events.order_events[9].filled_size, 1));
  cr_assert(eq(events.order_events[9].cum_filled_size, 2));
  cr_assert(eq(events.order_events[9].remaining_size, 0));

  orderbook_execute(&ob, 5, SIDE_ASK, 5, 5, true);

  cr_assert(eq(events.order_events_len, 14));

  cr_assert(eq(events.order_events[10].status, ORDER_STATUS_CREATED));
  cr_assert(eq(events.order_events[10].order_id, 5));
  cr_assert(eq(events.order_events[10].side, SIDE_ASK));
  cr_assert(eq(events.order_events[10].price, order1.price));
  cr_assert(eq(events.order_events[10].filled_size, 0));
  cr_assert(eq(events.order_events[10].cum_filled_size, 0));
  cr_assert(eq(events.order_events[10].remaining_size, 5));

  cr_assert(eq(events.order_events[11].status, ORDER_STATUS_FILLED));
  cr_assert(eq(events.order_events[11].order_id, order1.order_id));
  cr_assert(eq(events.order_events[11].side, order1.side));
  cr_assert(eq(events.order_events[11].price, order1.price));
  cr_assert(eq(events.order_events[11].filled_size, 4));
  cr_assert(eq(events.order_events[11].cum_filled_size, 5));
  cr_assert(eq(events.order_events[11].remaining_size, 0));

  cr_assert(eq(events.order_events[12].status, ORDER_STATUS_PARTIALLY_FILLED));
  cr_assert(eq(events.order_events[12].order_id, 5));
  cr_assert(eq(events.order_events[12].side, SIDE_ASK));
  cr_assert(eq(events.order_events[12].price, order1.price));
  cr_assert(eq(events.order_events[12].filled_size, 4));
  cr_assert(eq(events.order_events[12].cum_filled_size, 4));
  cr_assert(eq(events.order_events[12].remaining_size, 1));

  cr_assert(eq(events.order_events[13].status,
               ORDER_STATUS_PARTIALLY_FILLED_CANCELLED));
  cr_assert(eq(events.order_events[13].order_id, 5));
  cr_assert(eq(events.order_events[13].side, SIDE_ASK));
  cr_assert(eq(events.order_events[13].price, 0));
  cr_assert(eq(events.order_events[13].filled_size, 0));
  cr_assert(eq(events.order_events[13].cum_filled_size, 4));
  cr_assert(eq(events.order_events[13].remaining_size, 1));
}

Test(orderbook,
     trade_event,
     .init = orderbook_setup_with_event_handler,
     .fini = orderbook_teardown) {
  struct order order = {
      .side = SIDE_BID, .order_id = 1, .price = 10000, .size = 1};
  orderbook_limit(&ob, order);

  orderbook_execute(&ob, 2, SIDE_ASK, 1, 1, true);

  cr_assert(eq(events.trade_events_len, 1));

  cr_assert(eq(events.trade_events[0].size, 1));
  cr_assert(eq(events.trade_events[0].side, SIDE_ASK));
  cr_assert(eq(events.trade_events[0].price, order.price));
}