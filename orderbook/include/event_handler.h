#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H
#include <stdint.h>

#include "limit.h"

enum reject_reason {
  REJECT_REASON_NO_ERROR,  // 0 so it is the default if unspecified
  REJECT_REASON_NO_LIQUIDITY
};

enum order_status {
  ORDER_STATUS_CREATED,
  ORDER_STATUS_CANCELLED,
  ORDER_STATUS_REJECTED,
  ORDER_STATUS_FILLED,
  ORDER_STATUS_PARTIALLY_FILLED,
  ORDER_STATUS_PARTIALLY_FILLED_CANCELLED,
};

struct order_event {
  enum order_status status;
  uint64_t order_id, filled_size, cum_filled_size, remaining_size, price;
  enum side side;
  enum reject_reason reject_reason;
  // TODO: order_user_data
};

struct trade_event {
  uint64_t size, price;
  enum side side;  // taker side
  // TODO: buyer_order_id
  // TODO: buyer_user_data
  // TODO: buyer_is_maker
  // TODO: seller_order_id
  // TODO: seller_user_data
  // TODO: seller_is_maker
};

struct event_handler {
  void (*handle_order_event)(uint64_t ob_id,
                             struct order_event event,
                             void* user_data);
  void (*handle_trade_event)(uint64_t ob_id,
                             struct trade_event event,
                             void* user_data);
  void*
      user_data;  // a place to store extra information, useful for passing
                  // closures across FFI boundaries, idea taken from
                  // https://adventures.michaelfbryan.com/posts/rust-closures-in-ffi/
};

struct event_handler event_handler_new();

#endif