#include <criterion/criterion.h>

#include "orderbook.h"

struct orderbook ob;
uint64_t current_order_id = 0;

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