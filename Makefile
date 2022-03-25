CFLAGS=-std=c++17 -g -Wall -O2
CC = g++

SRCS=$(wildcard *.cc)
OBJS=$(SRCS:.cc=.o)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.out)

asmlai: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS) : parser.h typesystem.h codegen.h token.h types.h

test/%.out: asmlai test/%.c
	$(CC) -o- -E -P -C test/$*.c | ./asmlai -o test/$*.s -
	$(CC) -o $@ test/$*.s -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh

clean:
	rm -rf asmlai tmp* $(TESTS) test/*.s
	find * -type f '(' -name '*~' -o -name '*.o' ')' -exec rm {} ';'

.PHONY: test clean
