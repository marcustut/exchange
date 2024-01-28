#ifndef PUBLISHER_H
#define PUBLISHER_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <cjson/cJSON.h>
#include <hiredis/hiredis.h>

#include "orderbook.h"

typedef struct {
  char* channel;
  char* message;
} message_t;

#define G_MESSAGES_MAX UINT16_MAX
message_t g_messages[G_MESSAGES_MAX];
uint64_t g_messages_count = 0;
uint64_t g_processed_messages_count = 0;

void publish_message(message_t message) {
  if (g_messages_count == G_MESSAGES_MAX) {
    while (g_messages_count != g_processed_messages_count) {
      printf(
          "[WARN] resetting g_messages_count to 0: waiting for %llu remaining "
          "messages to be handled\n",
          g_messages_count - g_processed_messages_count);
      struct timespec ts = {.tv_sec = 0, .tv_nsec = 10000000};  // 10ms
      nanosleep(&ts, NULL);
    }
    g_messages_count = 0;
    g_processed_messages_count = 0;
  }

  g_messages[g_messages_count] = message;
  printf("[DEBUG] added message %llu to queue\n", g_messages_count);
  g_messages_count++;
}

void publish_trade(const char* symbol,
                   const uint64_t trade_id,
                   const enum side_e side,
                   const int64_t price,
                   const uint32_t quantity,
                   const uint64_t timestamp) {
  cJSON* root = cJSON_CreateObject();
  char price_str[19], quantity_str[12];
  snprintf(price_str, 19, "%.9f", price / 1e9);
  snprintf(quantity_str, 12, "%.6f", quantity / 1e6);
  cJSON_AddItemToObject(root, "symbol", cJSON_CreateString(symbol));
  cJSON_AddItemToObject(root, "id", cJSON_CreateNumber(trade_id));
  cJSON_AddItemToObject(root, "side", cJSON_CreateString(side_to_string(side)));
  cJSON_AddItemToObject(root, "price", cJSON_CreateString(price_str));
  cJSON_AddItemToObject(root, "quantity", cJSON_CreateString(quantity_str));
  cJSON_AddItemToObject(root, "timestamp", cJSON_CreateNumber(timestamp));
  char* message = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  char* channel = (char*)malloc(sizeof(char) * 50);
  snprintf((char*)channel, 50, "%s:trades:%s", "futures", symbol);

  publish_message((message_t){.channel = channel, .message = message});
}

#define PUBLISHER_CONSOLE 0
#define PUBLISHER_REDIS 1

static struct {
  int* publishers;
  uint8_t publishers_count;
  redisContext* redis;
} PUBLISHER = {
    .publishers = NULL,
    .publishers_count = 0,
    .redis = NULL,
};

void publisher_add(const int publisher) {
  PUBLISHER.publishers_count++;
  PUBLISHER.publishers =
      realloc(PUBLISHER.publishers, sizeof(int) * PUBLISHER.publishers_count);
  PUBLISHER.publishers[PUBLISHER.publishers_count - 1] = publisher;
}

void publisher_set_redis(redisContext* context) {
  PUBLISHER.redis = context;
}

int console_publish_message(const message_t message) {
  printf("[%s] %s\n", message.channel, message.message);
  return 0;
}

int redis_publish_message(const message_t message, redisContext* context) {
  redisCommand(context, "PUBLISH %s %s", message.channel, message.message);
  return 0;
}

void* publisher_thread() {
  while (true) {
    struct timespec ts = {.tv_sec = 0, .tv_nsec = 10000000};  // 10ms
    nanosleep(&ts, NULL);

    uint64_t offset = g_messages_count - g_processed_messages_count;

    if (offset == 0)
      continue;
    else if (offset < 0) {
      printf("[ERROR] offset < 0\n");
    }

    for (int i = 0; i < offset; i++)
      for (int j = 0; j < PUBLISHER.publishers_count; j++) {
        const message_t msg = g_messages[g_processed_messages_count + i];
        switch (PUBLISHER.publishers[j]) {
          case PUBLISHER_CONSOLE:
            console_publish_message(msg);
            break;
          case PUBLISHER_REDIS:
            redis_publish_message(msg, PUBLISHER.redis);
            break;
        }
      }
    g_processed_messages_count += offset;
  }
}

pthread_t publisher_thread_start() {
  pthread_t thread;
  pthread_create(&thread, NULL, publisher_thread, NULL);
  printf("[DEBUG] publisher thread has been started\n");
  return thread;
}

void publisher_thread_stop(pthread_t thread) {
  pthread_cancel(thread);
  printf("[DEBUG] publisher thread has been stopped\n");
}

#endif /* PUBLISHER_H */
