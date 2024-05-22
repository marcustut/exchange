#include "event_handler.h"

struct event_handler event_handler_new(
    void (*handle_filled)(struct event_filled),
    void (*handle_partially_filled)(struct event_partially_filled)) {
  struct event_handler event_handler;
  event_handler.handle_filled = handle_filled;
  event_handler.handle_partially_filled = handle_partially_filled;
  return event_handler;
}