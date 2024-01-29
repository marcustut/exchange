CC = clang
CFLAGS = -Wall -g
INCLUDES = -I. -I/usr/local/include
LFLAGS = -L/usr/local/lib -Xlinker -rpath -Xlinker /usr/local/lib
LIBS = -lhiredis -lcjson -llog

SRCS = main.c
OBJS = $(SRCS:.c=.o)
MAIN = main

all: $(MAIN)

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: valgrind run clean

valgrind: clean all
	valgrind \
		--leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		--verbose \
		--log-file=vg.out \
		./main

run: clean all
	./main

clean:
	$(RM) main *.o
