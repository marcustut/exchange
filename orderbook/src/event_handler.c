#include "event_handler.h"

#include <stdlib.h>

struct event_handler event_handler_new() {
  struct event_handler event_handler = {.handle_order_event = NULL,
                                        .handle_trade_event = NULL};
  return event_handler;
}