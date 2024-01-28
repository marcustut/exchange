#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include <cjson/cJSON.h>
#include <hiredis/hiredis.h>

#include "engine.h"
#include "publisher.h"

#define MAX_DEPTH UINT16_MAX
#define ORDERS 10
#define SYMBOLS 21

static volatile bool running = true;
const int SIGNALS[] = {SIGINT, SIGTERM};

void signal_handler(int sig) {
  char* signame = sig_to_string(sig);
  printf("Caught signal %s\n", signame);
  running = false;
  free(signame);
}

redisContext* connect_redis() {
  redisOptions redisOptions = {0};
  REDIS_OPTIONS_SET_TCP(&redisOptions, "localhost", 6379);
  redisContext* redis = redisConnectWithOptions(&redisOptions);
  if (redis == NULL || redis->err) {
    if (redis) {
      fprintf(stderr, "[ERROR] Failed connecting to Redis: %s\n",
              redis->errstr);
      redisFree(redis);
    } else
      fprintf(stderr, "[ERROR] Can't allocate redis context\n");
    return NULL;
  }
  redisReply* reply =
      redisCommand(redis, "AUTH %s", "redis");  // TODO: Replace with config
  if (reply->type == REDIS_REPLY_ERROR) {
    fprintf(stderr, "[ERROR] Failed authenticating to Redis: %s\n", reply->str);
    freeReplyObject(reply);
    redisFree(redis);
    return NULL;
  }
  printf("[DEBUG] connected to redis successfully\n");
  freeReplyObject(reply);
  return redis;
}

typedef struct {
  const char* market;
  const char* topic;
  const char* symbol;
} channel_t;

channel_t* channel_from_str(char* str) {
  channel_t* channel = malloc(sizeof(channel_t));

  const char* token = strtok(str, ":");
  if (token != NULL)
    channel->market = token;
  else
    goto err;

  token = strtok(NULL, ":");
  if (token != NULL)
    channel->topic = token;
  else
    goto err;

  token = strtok(NULL, ":");
  if (token != NULL)
    channel->symbol = token;
  else
    goto err;

  return channel;

err:
  free(channel);
  return NULL;
}

struct engine_map_s {
  engine_t engine;
};

engine_t* engine_map_get(struct engine_map_s* map, const char* symbol) {
  for (int i = 0; i < SYMBOLS; i++)
    if (strcmp(map[i].engine.symbol, symbol) == 0)
      return &map[i].engine;
  return NULL;
}

int main(void) {
  // Register signal handlers
  for (int i = 0; i < sizeof(SIGNALS) / sizeof(SIGNALS[0]); i++) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    if (sigaction(SIGNALS[i], &sa, NULL) == -1) {
      perror("failed to register signal handler");
      exit(1);
    }
  }

  struct engine_map_s engine_map[SYMBOLS] = {
      {.engine = engine_new("BTCUSDT", MAX_DEPTH)},
      {.engine = engine_new("ETHUSDT", MAX_DEPTH)},
      {.engine = engine_new("BNBUSDT", MAX_DEPTH)},
      {.engine = engine_new("DOGEUSDT", MAX_DEPTH)},
      {.engine = engine_new("ADAUSDT", MAX_DEPTH)},
      {.engine = engine_new("XRPUSDT", MAX_DEPTH)},
      {.engine = engine_new("DOTUSDT", MAX_DEPTH)},
      {.engine = engine_new("UNIUSDT", MAX_DEPTH)},
      {.engine = engine_new("BCHUSDT", MAX_DEPTH)},
      {.engine = engine_new("LTCUSDT", MAX_DEPTH)},
      {.engine = engine_new("LINKUSDT", MAX_DEPTH)},
      {.engine = engine_new("MATICUSDT", MAX_DEPTH)},
      {.engine = engine_new("SOLUSDT", MAX_DEPTH)},
      {.engine = engine_new("ETCUSDT", MAX_DEPTH)},
      {.engine = engine_new("THETAUSDT", MAX_DEPTH)},
      {.engine = engine_new("ICPUSDT", MAX_DEPTH)},
      {.engine = engine_new("XLMUSDT", MAX_DEPTH)},
      {.engine = engine_new("VETUSDT", MAX_DEPTH)},
      {.engine = engine_new("FILUSDT", MAX_DEPTH)},
      {.engine = engine_new("TRXUSDT", MAX_DEPTH)},
      {.engine = engine_new("EOSUSDT", MAX_DEPTH)},
  };

  redisContext* publisher = connect_redis();
  if (publisher == NULL)
    return 1;

  publisher_set_redis(publisher);
  publisher_add(PUBLISHER_CONSOLE);
  publisher_add(PUBLISHER_REDIS);
  pthread_t publisher_thread = publisher_thread_start();

  /* order_t orders[ORDERS] = { */
  /*     {.side = BID, .price = 98e10, .quantity = 1e3}, */
  /*     {.side = ASK, .price = 102e10, .quantity = 1e3}, */
  /*     {.side = BID, .price = 99e10, .quantity = 1e3}, */
  /*     {.side = ASK, .price = 103e10, .quantity = 1e3}, */
  /*     {.side = BID, .price = 100e10, .quantity = 1e3}, */
  /*     {.side = ASK, .price = 101e10, .quantity = 1e3}, */
  /*     {.side = BID, .price = 0, .quantity = 2e3}, */
  /*     {.side = ASK, .price = 0, .quantity = 1e3}, */
  /*     {.side = BID, .price = 0, .quantity = 1e3}, */
  /*     {.side = ASK, .price = 0, .quantity = 1e3}, */
  /* }; */

  /* for (int i = 0; i < ORDERS; i++) { */
  /*   engine_new_order(&engine, orders[i]); */
  /*   orderbook_print(&engine.orderbook); */
  /* } */

  // TODO: Change to async (at the moment it's blocking)
  redisContext* subscriber = connect_redis();
  redisReply* reply =
      redisCommand(subscriber, "PSUBSCRIBE %s:orders:*", "futures");
  freeReplyObject(reply);
  while (running) {
    if (redisGetReply(subscriber, (void*)&reply) != REDIS_OK)
      fprintf(stderr, "[ERROR] Failed reading from Redis: %s\n",
              subscriber->errstr);

    if (reply->type != REDIS_REPLY_ARRAY)
      fprintf(stderr, "[ERROR] Unexpected reply type: %d (%s)\n", reply->type,
              reply->str);

    if (reply->elements != 4)
      fprintf(stderr, "[ERROR] Unexpected number of elements: %zu\n",
              reply->elements);

    // Ignore replies that are not our responses
    if (strcmp(reply->element[0]->str, "pmessage") != 0) {
      freeReplyObject(reply);
      continue;
    }

    redisReply* channel_elem = reply->element[2];
    if (channel_elem->type != REDIS_REPLY_STRING) {
      fprintf(stderr, "[ERROR] Unexpected reply type: %d (%s)\n",
              channel_elem->type, channel_elem->str);
      freeReplyObject(reply);
      continue;
    }
    char* _channel = channel_elem->str;
    if (_channel == NULL || strlen(_channel) == 0) {
      fprintf(stderr, "[ERROR] Received NULL or empty channel\n");
      freeReplyObject(reply);
      continue;
    }
    channel_t* channel = channel_from_str(strdup(_channel));

    redisReply* msg_elem = reply->element[3];
    if (msg_elem->type != REDIS_REPLY_STRING) {
      fprintf(stderr, "[ERROR] Unexpected reply type: %d (%s)\n",
              msg_elem->type, msg_elem->str);
      freeReplyObject(reply);
      continue;
    }
    const char* msg = msg_elem->str;
    if (msg == NULL || strlen(msg) == 0) {
      fprintf(stderr, "[ERROR] Received NULL or empty message\n");
      freeReplyObject(reply);
      continue;
    }

    printf("[DEBUG] Received message from '%s': %s\n", _channel, msg);
    const char* parse_end = NULL;
    cJSON* value = cJSON_ParseWithOpts(msg, &parse_end, true);
    if (value == NULL) {
      fprintf(stderr, "[ERROR] Failed parsing JSON at '%s'\n", parse_end);
      freeReplyObject(reply);
      continue;
    }

    if (!cJSON_IsObject(value))
      fprintf(stderr, "[ERROR] Message received is not a JSON object (%d)\n",
              value->type);

    order_t* order = order_from_json(value);
    if (order == NULL) {
      fprintf(stderr, "[ERROR] Failed parsing order from JSON\n");
      cJSON_Delete(value);
      freeReplyObject(reply);
      continue;
    }
    printf("[DEBUG] Received order: %s at %.2f for %.3f\n",
           side_to_string(order->side), order->price / 1e9,
           order->quantity / 1e6);

    engine_t* engine = engine_map_get(engine_map, channel->symbol);
    if (engine == NULL) {
      fprintf(stderr, "[ERROR] engine for symbol '%s' has not been started\n",
              channel->symbol);
    }
    engine_new_order(engine, *order);
    orderbook_print(&engine->orderbook);

    cJSON_Delete(value);
    free(order);
    free(channel);
    freeReplyObject(reply);
  }

  publisher_thread_stop(publisher_thread);
  redisFree(publisher);
  redisFree(subscriber);

  return 0;
}
