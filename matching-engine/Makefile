CC = gcc
CFLAGS = -Wall -g


INCLUDES = -I.

LFLAGS = 

LIBS = 

SRCS = main.c

OBJS = $(SRCS:.c=.o)

MAIN = main

all: $(MAIN)
	@echo  Compiled to main

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) main *.o
