#!/bin/bash

set -e 

# Create a directory for redis to use as unix socket
[[ ! -d /tmp/redis ]] && \
    mkdir /tmp/redis.sock && \
    chmod -R 777 /tmp/redis.sock

# Start docker compose
docker compose up -d
