#include <criterion/criterion.h>

#include "orderbook.h"

struct orderbook ob;
uint64_t current_order_id = 1;

uint64_t next_order_id() {
  return current_order_id++;
}

void orderbook_setup(void) {
  ob = orderbook_new();
}

void orderbook_teardown(void) {
  orderbook_free(&ob);
}

Test(orderbook,
     limit_new_best,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  cr_assert_eq(ob.bid->map.size, 0);

  uint64_t oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid, .price = 10, .size = 1});
  cr_assert_eq(ob.bid->map.size, 1);
  cr_assert(ob.bid->best != NULL);
  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->order_head->order_id, oid);
  cr_assert_eq(ob.bid->best->order_tail->order_id, oid);

  oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid, .price = 11, .size = 1});
  cr_assert_eq(ob.bid->map.size, 2);
  cr_assert(ob.bid->best != NULL);
  cr_assert_eq(ob.bid->best->price, 11);
  cr_assert_eq(ob.bid->best->order_head->order_id, oid);
  cr_assert_eq(ob.bid->best->order_tail->order_id, oid);

  cr_assert_eq(ob.ask->map.size, 0);

  oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_ASK, .order_id = oid, .price = 13, .size = 1});
  cr_assert_eq(ob.ask->map.size, 1);
  cr_assert(ob.ask->best != NULL);
  cr_assert_eq(ob.ask->best->price, 13);
  cr_assert_eq(ob.ask->best->order_head->order_id, oid);
  cr_assert_eq(ob.ask->best->order_tail->order_id, oid);

  oid = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_ASK, .order_id = oid, .price = 12, .size = 1});
  cr_assert_eq(ob.ask->map.size, 2);
  cr_assert(ob.ask->best != NULL);
  cr_assert_eq(ob.ask->best->price, 12);
  cr_assert_eq(ob.ask->best->order_head->order_id, oid);
  cr_assert_eq(ob.ask->best->order_tail->order_id, oid);
}

Test(orderbook,
     limit_existing_order,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  cr_assert_eq(ob.bid->map.size, 0);

  uint64_t oid0 = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid0, .price = 10, .size = 1});
  cr_assert_eq(ob.bid->map.size, 1);
  cr_assert(ob.bid->best != NULL);
  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->order_head->order_id, oid0);
  cr_assert_eq(ob.bid->best->order_tail->order_id, oid0);

  uint64_t oid1 = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid1, .price = 10, .size = 2});
  cr_assert_eq(ob.bid->map.size, 1);
  cr_assert(ob.bid->best != NULL);
  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->order_head->order_id, oid0);
  cr_assert_eq(ob.bid->best->order_tail->order_id, oid1);

  uint64_t oid2 = next_order_id();
  orderbook_limit(
      &ob, (struct order){
               .side = SIDE_BID, .order_id = oid2, .price = 10, .size = 3});
  cr_assert_eq(ob.bid->map.size, 1);
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

  cr_assert_eq(orderbook_market(&ob, SIDE_ASK, 1), 0);

  cr_assert_eq(ob.bid->best->price, 11);
  cr_assert_eq(ob.bid->best->volume, 2);
  cr_assert_eq(ob.bid->best->order_count, 1);
  cr_assert_eq(ob.bid->size, 2);

  cr_assert_eq(orderbook_market(&ob, SIDE_ASK, 1), 0);

  cr_assert_eq(ob.bid->best->price, 11);
  cr_assert_eq(ob.bid->best->volume, 1);
  cr_assert_eq(ob.bid->best->order_count, 1);
  cr_assert_eq(ob.bid->size, 2);

  cr_assert_eq(orderbook_market(&ob, SIDE_ASK, 2), 0);

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 5);
  cr_assert_eq(ob.bid->best->order_count, 3);
  cr_assert_eq(ob.bid->size, 1);

  cr_assert_eq(orderbook_market(&ob, SIDE_ASK, 2), 0);

  cr_assert_eq(ob.bid->best->price, 10);
  cr_assert_eq(ob.bid->best->volume, 3);
  cr_assert_eq(ob.bid->best->order_count, 2);
  cr_assert_eq(ob.bid->size, 1);

  cr_assert_eq(orderbook_market(&ob, SIDE_ASK, 3), 0);

  cr_assert_eq(ob.bid->best, NULL);
  cr_assert_eq(ob.bid->size, 0);
}

Test(orderbook,
     market_no_liquidity,
     .init = orderbook_setup,
     .fini = orderbook_teardown) {
  cr_assert_eq(ob.ask->best, NULL);

  cr_assert_eq(orderbook_market(&ob, SIDE_BID, 3), 3);

  cr_assert_eq(ob.ask->best, NULL);

  orderbook_limit(&ob, (struct order){.side = SIDE_ASK,
                                      .order_id = next_order_id(),
                                      .price = 10,
                                      .size = 3});

  cr_assert_eq(ob.ask->best->price, 10);
  cr_assert_eq(ob.ask->best->volume, 3);

  cr_assert_eq(orderbook_market(&ob, SIDE_BID, 4), 1);

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
