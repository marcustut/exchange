#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>

#include "orderbook.h"

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

struct message* parse_messages(const char* path) {
  FILE* file = fopen(path, "r");
  if (file == NULL) {
    fprintf(stderr, "failed to open file");
    exit(EXIT_FAILURE);
  }

  // allocate enough memory for that many messages
  size_t messages_len = get_line_count(path);
  struct message* messages =
      (struct message*)malloc(sizeof(struct message) * messages_len);

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

    messages[i] = (struct message){
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

  fclose(file);
  if (line)
    free(line);

  return messages;
}