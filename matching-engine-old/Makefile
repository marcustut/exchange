CC = clang
CCFLAGS = -std=c99 -DLOG_USE_COLOR
INCLUDES = -I. -I/usr/local/include
LFLAGS = -L/usr/local/lib -Xlinker -rpath -Xlinker /usr/local/lib
LIBS = -lhiredis -lcjson -lcyaml

ifdef RELEASE
	CCFLAGS += -O3
else
	CCFLAGS += -g3
	CCFLAGS += -Wall -Wextra -Wconversion -Wdouble-promotion -Wno-unused-parameter -Wno-unused-function -Wno-sign-conversion
	CCFLAGS += -fsanitize=address,undefined -fsanitize-trap
	CCFLAGS += --system-header-prefix=/usr/local/include
endif

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CCFLAGS += -D _POSIX_C_SOURCE=200809L
endif
ifeq ($(UNAME_S),Darwin)
endif

SRCS = main.c
OBJS = $(SRCS:.c=.o)
MAIN = main

all: $(MAIN)

$(MAIN): $(OBJS) 
	$(CC) $(CCFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CCFLAGS) -c $< -o $@

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
		./$(MAIN) config.yaml

run: clean all
	./$(MAIN) config.yaml

clean:
	$(RM) $(MAIN) *.o
