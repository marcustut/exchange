#!/bin/bash

# Start the Redis stack
docker run \
    -d \
    -e REDIS_ARGS="--requirepass redis --unixsocket /tmp/docker/redis.sock --unixsocketperm 777" \
    --name redis-stack \
    -p 6379:6379 \
    -p 8001:8001 \
    redis/redis-stack:latest
