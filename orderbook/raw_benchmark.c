#include <stdio.h>

#include "orderbook.h"
#include "tests/orderbook_message.h"
#include "time.h"

#define DATA "data/l3_orderbook_100k.ndjson"

struct state {
  struct message* messages;
  size_t messages_len;
};

// void handle_order_event(enum order_event_type type, struct order_event event)
// {
//   struct timespec start;
//   clock_gettime(CLOCK_MONOTONIC, &start);

//   switch (type) {
//     case ORDER_EVENT_TYPE_CREATED:
//       printf("[order_created] %ld at %ld (%ld)", event.order_id, event.price,
//              event.remaining_size);
//       break;
//     case ORDER_EVENT_TYPE_CANCELLED:
//       printf("[order_cancelled] %ld at %ld (%ld/%ld)", event.order_id,
//              event.price, event.filled_size,
//              event.filled_size + event.remaining_size);
//       break;
//     case ORDER_EVENT_TYPE_FILLED:
//       printf("[order_filled] %ld of %ld filled at %ld, %ld remains",
//              event.filled_size, event.order_id, event.price,
//              event.remaining_size);
//       break;
//     case ORDER_EVENT_TYPE_PARTIALLY_FILLED:
//       printf("[order_partially_filled] %ld of %ld filled at %ld, %ld
//       remains",
//              event.filled_size, event.order_id, event.price,
//              event.remaining_size);
//       break;
//     case ORDER_EVENT_TYPE_PARTIALLY_FILLED_CANCELLED:
//       printf(
//           "[order_partially_filled_cancelled] %ld of %ld filled at %ld, %ld "
//           "remains",
//           event.filled_size, event.order_id, event.price,
//           event.remaining_size);
//       break;
//     default:
//       fprintf(stderr, "does not know how to handle order_event_type %d",
//       type); break;
//   }

//   struct timespec end;
//   clock_gettime(CLOCK_MONOTONIC, &end);

//   uint64_t elapsed_ns =
//       (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);

//   printf(" in %ldns\n", elapsed_ns);
// }

uint64_t benchmark(struct state* state) {
  struct orderbook orderbook = orderbook_new();
  struct orderbook* ob = &orderbook;
  // struct event_handler handler = event_handler_new(handle_order_event);
  // orderbook_set_event_handler(ob, &handler);

  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);

  for (int i = 0; i < state->messages_len; i++) {
    const struct message message = state->messages[i];

    switch (message.message_type) {
      case MESSAGE_TYPE_CREATED:
        if (message.price == 0)  // market
          orderbook_market(ob, message.order_id, message.side, message.size);
        else  // limit
          orderbook_limit(ob, (struct order){.order_id = message.order_id,
                                             .side = message.side,
                                             .price = message.price,
                                             .size = message.size});
        break;
      case MESSAGE_TYPE_DELETED:
        orderbook_cancel(ob, message.order_id);
        break;
      case MESSAGE_TYPE_CHANGED:
        orderbook_amend_size(ob, message.order_id, message.size);
        break;
    }
  }

  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &end);

  uint64_t elapsed_ns =
      (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);

  orderbook_free(&orderbook);

  return elapsed_ns;
}

#define SAMPLE_SIZE 100

int main() {
  struct state state = {.messages_len = get_line_count(DATA),
                        .messages = parse_messages(DATA)};

  uint64_t elapsed_ns = 0;

  for (int i = 0; i < SAMPLE_SIZE; i++) {
    elapsed_ns += benchmark(&state);
    if (i != 0)
      elapsed_ns /= 2;
  }

  printf("Over %d samples,\n", SAMPLE_SIZE);
  printf("Took %.2fms to process %ld messages\n", elapsed_ns * 1e-6,
         state.messages_len);
  printf("Took %ldns to process 1 message\n", elapsed_ns / state.messages_len);

  // Deallocate memory
  free(state.messages);

  return 0;
}