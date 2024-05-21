#include <cJSON.h>
#include <ubench.h>

#include "orderbook.h"
#include "tests/orderbook_message.h"

#define DATA "data/l3_orderbook_100k.ndjson"

struct ob_benchmark {
  struct orderbook orderbook;
  struct message* messages;
  size_t messages_len;
};

UBENCH_F_SETUP(ob_benchmark) {
  ubench_fixture->messages_len = get_line_count(DATA);
  ubench_fixture->messages = parse_messages(DATA);
  ubench_fixture->orderbook = orderbook_new();
}

UBENCH_F_TEARDOWN(ob_benchmark) {
  free(ubench_fixture->messages);
}

UBENCH_F(ob_benchmark, process_ob_message) {
  struct orderbook* ob = &ubench_fixture->orderbook;

  // for (int i = 0; i < ubench_fixture->messages_len; i++) {
  for (int i = 0; i < 1000; i++) {
    const struct message message = ubench_fixture->messages[i];
    printf("%d: ", i);
    if (message.message_type == MESSAGE_TYPE_CREATED)
      printf("%s (%s)", "order_created",
             message.side == SIDE_ASK ? "ask" : "bid");
    else if (message.message_type == MESSAGE_TYPE_DELETED)
      printf("%s", "order_deleted");
    else
      printf("%s", "order_changed");

    printf(" %ld (%ld)", message.price, message.size);

    printf("\n");
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

    char* str = orderbook_print(ob);
    printf("%s\n", str);
    free(str);
  }

  exit(EXIT_SUCCESS);  // remove this (only for debugging)
}

UBENCH_MAIN();