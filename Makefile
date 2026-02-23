CC = gcc
CFLAGS = -g -Wall -Wextra -Werror
BENCH_CFLAGS = -O3 -DNDEBUG -DHASHMAP_BENCH

MAIN_BIN = main
TEST_BIN = test_hashmap
BENCH_BIN = bench_hashmap

.PHONY: all main test bench valgrind clean

all: main test

main:	main.c hashmap.h
	$(CC) $(CFLAGS) main.c -o $(MAIN_BIN)

test:	test_hashmap.c hashmap.h
	$(CC) $(CFLAGS) test_hashmap.c -o $(TEST_BIN)

bench:	bench_hashmap.c hashmap.h
	$(CC) $(BENCH_CFLAGS) bench_hashmap.c -o $(BENCH_BIN)

bench-quick: bench_hashmap.c hashmap.h
	$(CC) $(BENCH_CFLAGS) -DBENCH_QUICK bench_hashmap.c -o $(BENCH_BIN)

bench-debug: bench_hashmap.c hashmap.h
	$(CC) -g $(BENCH_CFLAGS) -DBENCH_QUICK bench_hashmap.c -o $(BENCH_BIN)

valgrind: main
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./$(MAIN_BIN)

clean:
	rm -f $(MAIN_BIN) $(TEST_BIN) $(BENCH_BIN)
