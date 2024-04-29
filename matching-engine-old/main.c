#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <cjson/cJSON.h>
#include <hiredis/adapters/poll.h>
#include <hiredis/async.h>
#include <hiredis/hiredis.h>

#include "include/config.h"
#include "include/engine.h"
#include "include/evloop.h"
#include "include/orderbook.h"
#include "include/publisher.h"
#include "include/utils.h"

#define MAX_DEPTH UINT16_MAX
#define ORDERS 10
#define SYMBOLS 21

static struct {
  engine_t engines[SYMBOLS];
} state;

void signal_handler(int sig) {
  const char* signame = sig_to_string(sig);
  log_warn("Caught signal %s", signame);

  if (sig == SIGPIPE) {
    log_warn("Ignoring SIGPIPE");
    return;
  }

  evloop_stop();
}

void redis_connect_callback(const redisAsyncContext* c, int status) {
  if (status != REDIS_OK) {
    log_error("Failed connecting to redis: %s", c->errstr);
    evloop_stop();
    return;
  }
  log_info("Connected to redis successfully");
}

void redis_disconnect_callback(const redisAsyncContext* c, int status) {
  evloop_stop();  // TODO: Should make a connection pool (so we can reconnect)
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
        evloop_stop();
        return;
      },
      "Failed authenticating with redis: %s", reply->str);
  log_info("authenticated with redis successfully");
}

redisAsyncContext* connect_redis_async(redis_config_t* redis_config) {
  redisOptions redisOptions = {0};

  if (redis_config->unix_socket_path)
    REDIS_OPTIONS_SET_UNIX(&redisOptions, redis_config->unix_socket_path);
  else if (redis_config->tcp_host && redis_config->tcp_port)
    REDIS_OPTIONS_SET_TCP(&redisOptions, redis_config->tcp_host,
                          redis_config->tcp_port);

  redisAsyncContext* redis = redisAsyncConnectWithOptions(&redisOptions);
  if (redis == NULL || redis->err) {
    if (redis) {
      log_error("Failed connecting to Redis: %s", redis->errstr);
      redisAsyncFree(redis);
    } else
      log_error("Can't allocate redis context");
    return NULL;
  }

  redisAsyncCommand(redis, redis_auth_callback, NULL, "AUTH %s %s",
                    redis_config->user, redis_config->password);
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
    char* _channel = reply->element[2]->str;
    channel_t* channel = channel_from_str(_channel);

    const char* msg = reply->element[3]->str;
    CHECK_LOG(LOG_WARN, msg == NULL || strlen(msg) == 0, FREE_RETURN(, channel),
              "Received NULL or empty msg");

    log_trace("Received message from '%s:%s:%s': %s", channel->market,
              channel->topic, channel->symbol, msg);
    const char* parse_end = NULL;
    cJSON* value = cJSON_ParseWithOpts(msg, &parse_end, true);
    CHECK_LOG(LOG_WARN, value == NULL, FREE_RETURN(, channel),
              "Failed parsing JSON at '%s'", parse_end);
    CHECK_LOG(
        LOG_WARN, !cJSON_IsObject(value),
        {
          cJSON_Delete(value);
          FREE_RETURN(, channel);
        },
        "Message received is not a JSON object (%d)", value->type);

    order_t* order = order_from_json(value);
    CHECK_LOG(
        LOG_WARN, order == NULL,
        {
          cJSON_Delete(value);
          FREE_RETURN(, channel);
        },
        "Failed parsing order from JSON");
    log_trace("Received order: %s at %.2f for %.3f",
              side_to_string(order->side), order->price / 1e9,
              order->quantity / 1e6);

    engine_t* engine = NULL;
    for (int i = 0; i < SYMBOLS; i++)
      if (strcmp(state.engines[i].symbol, channel->symbol) == 0)
        engine = &state.engines[i];
    CHECK_ERR(
        engine == NULL,
        {
          cJSON_Delete(value);
          FREE_RETURN(, channel, order);
        },
        "Failed finding engine for symbol: %s", channel->symbol);

    engine_new_order(engine, *order);
    orderbook_print(&engine->orderbook);

    cJSON_Delete(value);
    FREE(channel, order);
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

#define CLEANUP()                   \
  for (int i = 0; i < SYMBOLS; i++) \
    engine_free(state.engines[i]);  \
  config_free(config);              \
  publisher_free();

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s [CONFIG_PATH]\n", argv[0]);
    return 1;
  }

  // Parse config file
  config_t* config = config_from_path(argv[1]);
  CHECK_ERR(config == NULL, return 1, "Failed to load config file");

  log_set_level(parse_log_level(config->log_level));
  /* log_add_fp(); */  // add log to file

  // Register signal handlers
  for (int i = 0; i < sizeof(SIGNALS) / sizeof(SIGNALS[0]); i++) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));  // force sigaction struct to be initialized
                                 // (avoid error in valgrind)
    sa.sa_handler = signal_handler;
    CHECK_ERR(
        sigaction(SIGNALS[i].value, &sa, NULL) == -1,
        {
          config_free(config);
          return 1;
        },
        "Failed registering signal handler for %s", SIGNALS[i].name);

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

  redisAsyncContext* publisher = connect_redis_async(config->redis);
  CHECK_ERR(
      publisher == NULL,
      {
        CLEANUP();
        return 1;
      },
      "Failed connecting to Redis");
  CHECK_ERR(
      redisPollAttach(publisher) == REDIS_ERR,
      {
        CLEANUP();
        redisAsyncFree(publisher);
      },
      "Failed attaching publisher to redis: %s", publisher->errstr);

  redisAsyncContext* subscriber = connect_redis_async(config->redis);
  CHECK_ERR(subscriber == NULL, CLEANUP(), "Failed connecting to Redis");
  CHECK_ERR(
      redisPollAttach(subscriber) == REDIS_ERR,
      {
        CLEANUP();
        redisAsyncFree(subscriber);
      },
      "Failed attaching subscriber to redis: %s", subscriber->errstr);

  // Subscribe to a pattern of channels
  CHECK_ERR(redisAsyncCommand(subscriber, redis_handle_order_message, NULL,
                              "PSUBSCRIBE %s:orders:*", "futures") == REDIS_ERR,
            return 1, "Failed PSUBSCRIBE to '%s:orders:*'", "futures");

  evloop_start();

  // Start the publisher in a separate thread managed by evloop
  publisher_set_redis(publisher);
  publisher_add(PUBLISHER_CONSOLE);
  publisher_add(PUBLISHER_REDIS);
  CHECK_ERR(evloop_spawn("publisher", publisher_thread, NULL) != 0, return 1,
            "Failed to spawn publisher thread");

  // Use the main thread to polling for subscriber and publisher
  while (evloop_is_running()) {
    CHECK_ERR(redisPollTick(subscriber, 0.01) == REDIS_ERR,  // polling at 10ms
              continue, "Failed polling redis: %s", subscriber->errstr);
    CHECK_ERR(redisPollTick(publisher, 0.01) == REDIS_ERR,  // polling at 10ms
              continue, "Failed polling redis: %s", publisher->errstr);
  }

  CLEANUP();
  redisAsyncFree(publisher);
  redisAsyncFree(subscriber);

  return 0;
}
