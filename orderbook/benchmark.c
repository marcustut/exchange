#include <cJSON.h>
#include <ubench.h>

#include "orderbook.h"

#define DATA "data/l3_orderbook_100k.ndjson"

int get_line_count(const char* path) {
#define BUF_SIZE 65536
  FILE* file = fopen(path, "r");
  if (file == NULL) {
    fprintf(stderr, "failed to open file");
    exit(EXIT_FAILURE);
  }

  char buf[BUF_SIZE];
  int counter = 0;
  for (;;) {
    size_t res = fread(buf, 1, BUF_SIZE, file);
    if (ferror(file))
      return -1;

    int i;
    for (i = 0; i < res; i++)
      if (buf[i] == '\n')
        counter++;

    if (feof(file))
      break;
  }

  fclose(file);
  return counter;
#undef BUF_SIZE
}

enum message_type {
  MESSAGE_TYPE_CREATED,
  MESSAGE_TYPE_DELETED,
  MESSAGE_TYPE_CHANGED,
};

struct message {
  uint64_t order_id, price, size;
  enum side side;
  enum message_type message_type;
};

struct ob_benchmark {
  struct orderbook orderbook;
  struct message* messages;
  size_t messages_len;
};

UBENCH_F_SETUP(ob_benchmark) {
  FILE* file = fopen(DATA, "r");
  if (file == NULL) {
    fprintf(stderr, "failed to open file");
    exit(EXIT_FAILURE);
  }

  // allocate enough memory for that many messages
  ubench_fixture->messages_len = get_line_count(DATA);
  ubench_fixture->messages =
      malloc(sizeof(struct message) * ubench_fixture->messages_len);

  // parse the messages
  char* line = NULL;
  size_t len;
  ssize_t read;
  int i = 0;
  while ((read = getline(&line, &len, file)) != -1) {
    cJSON* message = cJSON_Parse(line);
    if (message == NULL) {
      fprintf(stderr, "failed to parse line as JSON");
      exit(EXIT_FAILURE);
    }

    const char* event =
        cJSON_GetObjectItemCaseSensitive(message, "event")->valuestring;
    const cJSON* data = cJSON_GetObjectItem(message, "data");

    const uint64_t order_id = cJSON_GetObjectItem(data, "id")->valuedouble;
    const uint64_t price = cJSON_GetObjectItem(data, "price")->valueint;
    const uint64_t size =
        cJSON_GetObjectItem(data, "amount")->valuedouble * 1e8;
    const enum side side =
        cJSON_GetObjectItem(data, "order_type")->valueint == 0 ? SIDE_BID
                                                               : SIDE_ASK;

    ubench_fixture->messages[i] = (struct message){
        .order_id = order_id,
        .price = price,
        .size = size,
        .side = side,
        .message_type =
            strcmp(event, "order_created") == 0   ? MESSAGE_TYPE_CREATED
            : strcmp(event, "order_deleted") == 0 ? MESSAGE_TYPE_DELETED
                                                  : MESSAGE_TYPE_CHANGED};

    cJSON_Delete(message);
    i++;
  }

  ubench_fixture->orderbook = orderbook_new();

  fclose(file);
  if (line)
    free(line);
}

UBENCH_F_TEARDOWN(ob_benchmark) {
  free(ubench_fixture->messages);
}

UBENCH_F(ob_benchmark, bar) {
  struct orderbook* ob = &ubench_fixture->orderbook;
  for (int i = 0; i < ubench_fixture->messages_len; i++) {
    printf("i: %d\n", i);
    const struct message message = ubench_fixture->messages[i];
    switch (message.message_type) {
      case MESSAGE_TYPE_CREATED:
        if (message.size != 0)  // limit
          orderbook_limit(ob, (struct order){.order_id = message.order_id,
                                             .side = message.side,
                                             .price = message.price,
                                             .size = message.size});
        else  // market
          orderbook_market(ob, message.side, message.size);
        break;
      case MESSAGE_TYPE_DELETED:
        orderbook_cancel(ob, message.order_id);
        break;
      case MESSAGE_TYPE_CHANGED:
        orderbook_amend_size(ob, message.order_id, message.size);
        break;
    }
  }
}

UBENCH_MAIN();