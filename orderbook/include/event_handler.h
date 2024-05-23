#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <stdint.h>

#include "limit.h"

enum order_event_type {
  ORDER_EVENT_TYPE_CREATED,
  ORDER_EVENT_TYPE_CANCELLED,
  ORDER_EVENT_TYPE_REJECTED,
  ORDER_EVENT_TYPE_FILLED,
  ORDER_EVENT_TYPE_PARTIALLY_FILLED,
  ORDER_EVENT_TYPE_PARTIALLY_FILLED_CANCELLED,
};

struct order_event {
  uint64_t order_id, filled_size, cum_filled_size, remaining_size, price;
  enum side side;
  // TODO: Add reject reason
};

struct event_handler {
  void (*handle_order_event)(enum order_event_type type,
                             struct order_event event);
};

struct event_handler event_handler_new(void (
    *handle_order_event)(enum order_event_type type, struct order_event event));

#endif