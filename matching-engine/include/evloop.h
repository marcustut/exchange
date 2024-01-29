#ifndef EVLOOP_H
#define EVLOOP_H

#include <pthread.h>

#include <log/log.h>

#include "utils.h"

#define MAX_THREADS UINT8_MAX  // 255

typedef struct {
  pthread_t tid;
  bool running;
  bool should_stop;
} thread_t;

typedef struct {
  uint8_t id;
  const char* name;
  void* arg;
} thread_state_t;

static volatile struct evloop {
  bool running;
  uint8_t num_threads;
  thread_t threads[MAX_THREADS];
  thread_state_t thread_states[MAX_THREADS];
} evloop = {.running = false,
            .num_threads = 0,
            .threads = {},
            .thread_states = {}};

bool evloop_is_running() {
  return evloop.running;
}

void evloop_start() {
  evloop.running = true;
}

void evloop_stop() {
  log_warn("signaling all threads to stop");
  // send a signal to stop all threads at once
  for (int i = 0; i < evloop.num_threads; i++)
    evloop.threads[i].should_stop = true;

  log_warn("waiting for %d threads to stop", evloop.num_threads);
  // keep polling until all threads stopped running
  for (int i = 0; i < evloop.num_threads; i++) {
    while (evloop.threads[i].running)
      sleep_ns(10 * MILLISECOND);
    log_warn("thread %d (%s) stopped", evloop.thread_states[i].id,
             evloop.thread_states[i].name);
  }

  evloop.running = false;
  log_warn("all threads stopped");
}

int16_t evloop_spawn(const char* name,
                     void* (*routine)(void*),
                     void* __restrict arg) {
  if (!evloop.running)
    return -1;

  pthread_t tid;
  uint8_t id = evloop.num_threads;
  evloop.thread_states[id] =
      (thread_state_t){.id = id, .name = name, .arg = arg};

  CHECK_ERR(pthread_create(&tid, NULL, routine,
                           (void*)&evloop.thread_states[id]) != 0,
            return -1, "Failed to create thread %d (%s)", id, name);
  CHECK_ERR(pthread_detach(tid) != 0, return -1,
            "Failed to detach thread %d (%s)", id, name);

  evloop.threads[id] =
      (thread_t){.tid = tid, .running = true, .should_stop = false};
  evloop.num_threads++;

  log_info("spawned thread %d (%s)", id, name);
  return id;
}

#endif /* EVLOOP_H */
