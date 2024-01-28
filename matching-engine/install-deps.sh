#!/bin/bash

set -e

echo "Installing hiredis..."

git clone https://github.com/redis/hiredis.git && \
    cd hiredis && \
    make && \
    sudo make install && \
    cd .. && \
    sudo rm -r hiredis

echo "Installed hiredis!"

echo "Installing cJSON..."

git clone https://github.com/DaveGamble/cJSON.git && \
    cd cJSON && \
    mkdir build && \
    cd build && \
    cmake .. \
        -DBUILD_SHARED_AND_STATIC_LIBS=On \
        -DENABLE_CJSON_TEST=Off && \
    make && \
    sudo make install && \
    cd ../.. && \
    sudo rm -r cJSON

echo "Installed cJSON!"
