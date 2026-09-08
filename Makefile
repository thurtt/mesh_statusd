CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?=-Wall -Werror -std=gnu11 -D_GNU_SOURCE -g -I $(CURDIR)/Unity/src
LDFLAGS ?=-lpthread
PROG=mesh_statusd
TEST=test_main

ALL_SRC := $(wildcard *.c)
UNITY_SRC = $(CURDIR)/Unity/src/unity.c
TEST_SRC = $(wildcard test_*.c)
SRCS := $(filter-out test_%.c, $(ALL_SRC))
OBJS = $(SRCS:.c=.o)


all: $(PROG)

clean:
	-rm *.o $(PROG) $(TEST)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(PROG)
	$(CC) $(CFLAGS) $(TEST_SRC) $(UNITY_SRC) -o $(TEST)
	$(CURDIR)/test_main
