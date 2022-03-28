CFLAGS=-std=c++17 -g -O2
CC = g++

SRCS=$(wildcard *.cc)
OBJS=$(SRCS:.cc=.o)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

asmlai: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJS): codegen.h parser.h token.h typesystem.h types.h

test/%.exe: asmlai test/%.c
	$(CC) -o- -E -P -C test/$*.c | ./asmlai -o test/$*.s -
	$(CC) -o $@ test/$*.s -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/run_tests.sh

clean:
	rm -rf asmlai tmp* $(TESTS) test/*.s test/*.exe
	find * -type f '(' -name '*~' -o -name '*.o' ')' -exec rm {} ';'

.PHONY: test clean
