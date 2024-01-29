CC = clang
CFLAGS = -std=c99 -Wall -g -D_POSIX_C_SOURCE=200809L
INCLUDES = -I. -I/usr/local/include
LFLAGS = -L/usr/local/lib -Xlinker -rpath -Xlinker /usr/local/lib
LIBS = -lhiredis -lcjson -llog -lcyaml

SRCS = main.c
OBJS = $(SRCS:.c=.o)
MAIN = main

all: $(MAIN)

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: deps format valgrind run clean

deps:
	./install-deps.sh

format:
	find . -type f \( -iname \*.c -o -iname \*.h \) | xargs clang-format -i

valgrind: clean all
	valgrind \
		--leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--verbose \
		--log-file=vg.out \
		./main config.yaml

run: clean all
	./main config.yaml

clean:
	$(RM) main *.o
