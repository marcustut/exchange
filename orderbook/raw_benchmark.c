#include <stdio.h>

#include "orderbook.h"
#include "tests/orderbook_message.h"
#include "time.h"

#define DATA "data/l3_orderbook_100k.ndjson"

struct state {
  struct message* messages;
  size_t messages_len;
};

uint64_t benchmark(struct state* state) {
  struct orderbook orderbook = orderbook_new();
  struct orderbook* ob = &orderbook;

  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);

  for (int i = 0; i < state->messages_len; i++) {
    const struct message message = state->messages[i];

    switch (message.message_type) {
      case MESSAGE_TYPE_CREATED:
        if (message.price == 0)  // market
          orderbook_market(ob, message.side, message.size);
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