#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include <stdint.h>

// enum event_type {
//   EVENT_TYPE_FILLED,
//   EVENT_TYPE_PARTIALLY_FILLED,
// };

struct event_filled {
  uint64_t order_id, filled_size, remaining_size, price;
};

struct event_partially_filled {
  uint64_t order_id, filled_size, remaining_size, price;
};

struct event_handler {
  void (*handle_filled)(struct event_filled event);
  void (*handle_partially_filled)(struct event_partially_filled event);
};

struct event_handler event_handler_new(
    void (*handle_filled)(struct event_filled),
    void (*handle_partially_filled)(struct event_partially_filled));

#endif