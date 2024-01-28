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

.PHONY: run clean

run: all
	./main

clean:
	$(RM) main *.o
