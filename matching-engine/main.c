#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <cjson/cJSON.h>
#include <hiredis/adapters/poll.h>
#include <hiredis/async.h>
#include <hiredis/hiredis.h>

#include "engine.h"
#include "orderbook.h"
#include "publisher.h"
#include "utils.h"

#define MAX_DEPTH UINT16_MAX
#define ORDERS 10
#define SYMBOLS 21

static struct {
  engine_t engines[SYMBOLS];
} state;

static volatile bool running = true;

void signal_handler(int sig) {
  const char* signame = sig_to_string(sig);
  log_warn("Caught signal %s", signame);

  if (sig == SIGPIPE) {
    log_warn("Ignoring SIGPIPE");
    return;
  }

  running = false;
}

redisContext* connect_redis() {
  redisOptions redisOptions = {0};
  REDIS_OPTIONS_SET_TCP(&redisOptions, "localhost", 6379);

  redisContext* redis = redisConnectWithOptions(&redisOptions);
  if (redis == NULL || redis->err) {
    if (redis) {
      log_error("Failed connecting to Redis: %s", redis->errstr);
      redisFree(redis);
    } else
      log_error("Can't allocate redis context");
    return NULL;
  }

  redisReply* reply =
      redisCommand(redis, "AUTH %s", "redis");  // TODO: Replace with config
  CHECK_ERR(
      reply->type == REDIS_REPLY_ERROR,
      {
        freeReplyObject(reply);
        redisFree(redis);
        return NULL;
      },
      "Failed authenticating to Redis: %s", reply->str);

  log_info("Connected to redis successfully");
  freeReplyObject(reply);
  return redis;
}

void redis_connect_callback(const redisAsyncContext* c, int status) {
  if (status != REDIS_OK) {
    log_error("Failed connecting to redis: %s", c->errstr);
    running = false;
    return;
  }
  log_info("Connected to redis successfully");
}

void redis_disconnect_callback(const redisAsyncContext* c, int status) {
  running = false;  // TODO: Should make a connection pool (so we can reconnect)
  if (status != REDIS_OK) {
    log_error("Failed connecting to redis: %s", c->errstr);
    return;
  }
  log_warn("Disconnected from redis");
}

void redis_auth_callback(redisAsyncContext* c, void* r, void* privdata) {
  redisReply* reply = r;
  if (reply == NULL)
    return;
  CHECK_ERR(
      reply->type == REDIS_REPLY_ERROR,
      {
        running = false;
        return;
      },
      "Failed authenticating with redis: %s", reply->str);
}

redisAsyncContext* connect_redis_async() {
  redisOptions redisOptions = {0};
  REDIS_OPTIONS_SET_TCP(&redisOptions, "localhost", 6379);

  redisAsyncContext* redis = redisAsyncConnectWithOptions(&redisOptions);
  if (redis == NULL || redis->err) {
    if (redis) {
      log_error("Failed connecting to Redis: %s", redis->errstr);
      redisAsyncFree(redis);
    } else
      log_error("Can't allocate redis context");
    return NULL;
  }

  redisAsyncCommand(redis, redis_auth_callback, NULL, "AUTH %s",
                    "redis");  // TODO: Replace with config
  redisAsyncSetConnectCallback(redis, redis_connect_callback);
  redisAsyncSetDisconnectCallback(redis, redis_connect_callback);

  return redis;
}

void redis_handle_order_message(redisAsyncContext* c, void* r, void* privdata) {
  redisReply* reply = r;
  if (reply == NULL)
    return;

  CHECK_LOG(LOG_TRACE, reply->type != REDIS_REPLY_ARRAY, return,
            "Unexpected reply type: %d, expected: %d", reply->type,
            REDIS_REPLY_ARRAY);

  CHECK_ERR(reply->elements < 3, return, "Unexpected number of elements: %zu",
            reply->elements)

  if (strcmp(reply->element[0]->str, "pmessage") == 0) {
    const char* _channel = reply->element[2]->str;
    channel_t* channel = channel_from_str(strdup(_channel));

    const char* msg = reply->element[3]->str;
    CHECK_LOG(LOG_WARN, msg == NULL || strlen(msg) == 0, return,
              "Received NULL or empty msg");

    log_trace("Received message from '%s': %s", _channel, msg);
    const char* parse_end = NULL;
    cJSON* value = cJSON_ParseWithOpts(msg, &parse_end, true);
    CHECK_LOG(LOG_WARN, value == NULL, return, "Failed parsing JSON at '%s'",
              parse_end);
    CHECK_LOG(LOG_WARN, !cJSON_IsObject(value), return,
              "Message received is not a JSON object (%d)", value->type);

    order_t* order = order_from_json(value);
    CHECK_LOG(LOG_WARN, order == NULL, return,
              "Failed parsing order from JSON");
    log_trace("Received order: %s at %.2f for %.3f",
              side_to_string(order->side), order->price / 1e9,
              order->quantity / 1e6);

    engine_t* engine = NULL;
    for (int i = 0; i < SYMBOLS; i++)
      if (strcmp(state.engines[i].symbol, channel->symbol) == 0)
        engine = &state.engines[i];
    CHECK_ERR(engine == NULL, return, "Failed finding engine for symbol: %s",
              channel->symbol);

    engine_new_order(engine, *order);
    orderbook_print(&engine->orderbook);

    cJSON_Delete(value);
    free(order);
    free(channel);
  } else if (strcmp(reply->element[0]->str, "psubscribe") == 0) {
    const char* pattern = reply->element[1]->str;
    log_info("Subscribed to pattern: %s", pattern);
  } else {
    log_warn("Unexpected reply type: %s", reply->element[0]->str);
    for (int i = 0; i < reply->elements; i++)
      log_warn("Element[%d]: %d (%s)", i, reply->element[i]->type,
               reply->element[i]->str);
  }
}

int main(void) {
  log_set_level(LOG_TRACE);
  /* log_add_fp(); */  // add log to file

  // Register signal handlers
  for (int i = 0; i < sizeof(SIGNALS) / sizeof(SIGNALS[0]); i++) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    if (sigaction(SIGNALS[i].value, &sa, NULL) == -1) {
      log_error("Failed registering signal handler for %s", SIGNALS[i].name);
      return 1;
    }
    log_info("Registered signal handler for %s", SIGNALS[i].name);
  }

  const char* symbols[SYMBOLS] = {
      "BTCUSDT", "ETHUSDT", "BNBUSDT",   "DOGEUSDT", "ADAUSDT",  "XRPUSDT",
      "DOTUSDT", "UNIUSDT", "BCHUSDT",   "LTCUSDT",  "LINKUSDT", "MATICUSDT",
      "SOLUSDT", "ETCUSDT", "THETAUSDT", "ICPUSDT",  "XLMUSDT",  "VETUSDT",
      "FILUSDT", "TRXUSDT", "EOSUSDT",
  };
  for (int i = 0; i < SYMBOLS; i++)
    state.engines[i] = engine_new(symbols[i], MAX_DEPTH);

  redisContext* publisher = connect_redis();
  CHECK_ERR(publisher == NULL, return 1, "Failed connecting to Redis");

  publisher_set_redis(publisher);
  publisher_add(PUBLISHER_CONSOLE);
  publisher_add(PUBLISHER_REDIS);
  pthread_t publisher_thread = publisher_thread_start();

  redisAsyncContext* subscriber = connect_redis_async();
  CHECK_ERR(subscriber == NULL, return 1, "Failed connecting to Redis");
  CHECK_ERR(redisPollAttach(subscriber) == REDIS_ERR, return 1,
            "Failed attaching to redis: %s", subscriber->errstr);

  // Subscribe to a pattern of channels
  CHECK_ERR(redisAsyncCommand(subscriber, redis_handle_order_message, NULL,
                              "PSUBSCRIBE %s:orders:*", "futures") == REDIS_ERR,
            return 1, "Failed PSUBSCRIBE to '%s:orders:*'", "futures");

  // Our event loop which keep polling for new messages
  while (running)
    CHECK_ERR(redisPollTick(subscriber, 0.1) == REDIS_ERR, continue,
              "Failed polling redis: %s", subscriber->errstr);

  publisher_thread_stop(publisher_thread);
  redisFree(publisher);
  redisAsyncFree(subscriber);

  return 0;
}
