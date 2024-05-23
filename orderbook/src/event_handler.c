#include "event_handler.h"

struct event_handler event_handler_new(
    void (*handle_order_event)(enum order_event_type type,
                               struct order_event event)) {
  struct event_handler event_handler;
  event_handler.handle_order_event = handle_order_event;
  return event_handler;
}