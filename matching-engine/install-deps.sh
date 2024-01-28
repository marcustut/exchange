#!/bin/bash

set -e

INCLUDE_DIR=/usr/local/include
LIB_DIR=/usr/local/lib

echo "Installing hiredis..."

[[ ! -d $INCLUDE_DIR/hiredis ]] && \
    git clone https://github.com/redis/hiredis.git && \
    cd hiredis && \
    make && \
    sudo make install && \
    cd .. && \
    sudo rm -r hiredis

echo "Installed hiredis!"

echo "Installing cJSON..."

[[ ! -d $INCLUDE_DIR/cjson ]] && \
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

echo "Installing log.h..."

[[ ! -d $INCLUDE_DIR/log ]] && \
    git clone https://github.com/rxi/log.c.git && \
    cd log.c && \
    clang -DLOG_USE_COLOR -c -o log.o src/log.c && \
    sudo ar r $LIB_DIR/liblog.a log.o && \
    sudo clang -dynamiclib -o $LIB_DIR/liblog.dylib log.o && \
    sudo mkdir $INCLUDE_DIR/log && \
    sudo cp src/log.h $INCLUDE_DIR/log/log.h && \
    cd .. && \
    sudo rm -r log.c

echo "Installed log.h!"
