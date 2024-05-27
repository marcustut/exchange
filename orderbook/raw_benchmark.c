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

struct benchmark_result {
  uint64_t market_elapsed_ns, market_count;
  uint64_t limit_elapsed_ns, limit_count;
  uint64_t cancel_elapsed_ns, cancel_count;
  uint64_t amend_size_elapsed_ns, amend_size_count;
};

struct benchmark_result benchmark(struct state* state) {
  struct orderbook orderbook = orderbook_new();
  struct orderbook* ob = &orderbook;
  // struct event_handler handler = event_handler_new();
  // handler.handle_order_event = handle_order_event;
  // handler.handle_trade_event = handle_trade_event;
  // orderbook_set_event_handler(ob, &handler);

  struct benchmark_result result = {};

  for (int i = 0; i < state->messages_len; i++) {
    const struct message message = state->messages[i];

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);

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

    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);

    uint64_t elapsed_ns =
        (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);

    switch (message.message_type) {
      case MESSAGE_TYPE_CREATED:
        if (message.price == 0) {
          result.market_count++;
          result.market_elapsed_ns += elapsed_ns;
        } else {
          result.limit_count++;
          result.limit_elapsed_ns += elapsed_ns;
        }
        break;
      case MESSAGE_TYPE_DELETED:
        result.cancel_count++;
        result.cancel_elapsed_ns += elapsed_ns;
        break;
      case MESSAGE_TYPE_CHANGED:
        result.amend_size_count++;
        result.amend_size_elapsed_ns += elapsed_ns;
        break;
    }
  }

  orderbook_free(&orderbook);

  return result;
}

#define SAMPLE_SIZE 100

int main() {
  struct state state = {.messages_len = get_line_count(DATA),
                        .messages = parse_messages(DATA)};

  struct benchmark_result result;
  uint64_t market_elapsed_ns = 0;
  uint64_t limit_elapsed_ns = 0;
  uint64_t cancel_elapsed_ns = 0;
  uint64_t amend_size_elapsed_ns = 0;

  for (int i = 0; i < SAMPLE_SIZE; i++) {
    result = benchmark(&state);
    market_elapsed_ns += result.market_elapsed_ns;
    limit_elapsed_ns += result.limit_elapsed_ns;
    cancel_elapsed_ns += result.cancel_elapsed_ns;
    amend_size_elapsed_ns += result.amend_size_elapsed_ns;
    if (i != 0) {
      market_elapsed_ns /= 2;
      limit_elapsed_ns /= 2;
      cancel_elapsed_ns /= 2;
      amend_size_elapsed_ns /= 2;
    }
  }

  uint64_t total_elapsed_ns = market_elapsed_ns + limit_elapsed_ns +
                              cancel_elapsed_ns + amend_size_elapsed_ns;

  printf("Over %d samples,\n", SAMPLE_SIZE);
  printf("Took %.2fms to process %ld messages\n", total_elapsed_ns * 1e-6,
         state.messages_len);
  printf("Took %ldns to process 1 message\n",
         total_elapsed_ns / state.messages_len);
  printf("-------------------------------\n");
  printf("orderbook_market: %ldns/op over %ld calls\n",
         market_elapsed_ns / result.market_count, result.market_count);
  printf("orderbook_limit: %ldns/op over %ld calls\n",
         limit_elapsed_ns / result.limit_count, result.limit_count);
  printf("orderbook_cancel: %ldns/op over %ld calls\n",
         cancel_elapsed_ns / result.cancel_count, result.cancel_count);
  printf("orderbook_amend_size: %ldns/op over %ld calls\n",
         amend_size_elapsed_ns / result.amend_size_count,
         result.amend_size_count);

  // Deallocate memory
  free(state.messages);

  return 0;
}