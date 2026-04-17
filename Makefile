SHELL = /bin/sh

#prefix=/home/joule/deberg/pendulum/pendvm

CC=gcc
INSTALL=install
LATEX=latex
DVIPS=dvips

DEBUG=-DDEBUG
CFLAGS=-g -std=c89 -Wno-pointer-sign -Wno-implicit-function-declaration -Wno-int-conversion
LDFLAGS=
LIBS=

OBJS=main.o memory.o machine.o commands.o pal_parse.o

TEST_SRCS      = $(wildcard tests/test_*.c)
TEST_BINS      = $(TEST_SRCS:.c=)
# main_for_tests.o is main.c compiled with main() renamed out of the way, so
# tests can reuse EXTRACT/power/sign_extend and the progname/output_radix
# globals without duplicating code or modifying main.c.
TEST_LINK_OBJS = memory.o machine.o pal_parse.o commands.o tests/main_for_tests.o
UNITY_SRC      = tests/vendor/unity/unity.c
UNITY_INC      = -Itests/vendor/unity

all: pendvm

.c.o:
	$(CC) $(DEBUG) $(CFLAGS) -c $<

pendvm: $(OBJS)
	$(CC) $(OBJS) $(LIBS) $(LDFLAGS) -o $@

pendvm.dvi: pendvm.tex
	$(LATEX) $<

pendvm.ps: pendvm.dvi
	$(DVIPS) -o $@ $<

test: test-unit test-integration

test-unit: $(TEST_BINS)
	@sh tests/run_unit.sh

test-integration: tests/check_reversible
	@sh tests/run_integration.sh

tests/main_for_tests.o: main.c
	$(CC) $(DEBUG) $(CFLAGS) -Dmain=pendvm_app_main -c main.c -o $@

tests/test_%: tests/test_%.c tests/test_support.h $(UNITY_SRC) $(TEST_LINK_OBJS)
	$(CC) $(DEBUG) $(CFLAGS) -I. $(UNITY_INC) $< $(UNITY_SRC) $(TEST_LINK_OBJS) -o $@

tests/check_reversible: tests/check_reversible.c $(TEST_LINK_OBJS)
	$(CC) $(DEBUG) $(CFLAGS) -I. $< $(TEST_LINK_OBJS) -o $@

clean:
	rm -f $(OBJS) pendvm pendvm.dvi pendvm.aux $(TEST_BINS) tests/check_reversible tests/main_for_tests.o
