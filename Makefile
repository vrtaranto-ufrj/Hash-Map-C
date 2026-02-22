CC = gcc
CFLAGS = -g -Wall -Wextra -Werror

MAIN_BIN = main
TEST_BIN = test_hashmap

.PHONY: all main test valgrind clean

all: main test

main:	main.c hashmap.h
	$(CC) $(CFLAGS) main.c -o $(MAIN_BIN)

test:	test_hashmap.c hashmap.h
	$(CC) $(CFLAGS) test_hashmap.c -o $(TEST_BIN)

valgrind: main
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 ./$(MAIN_BIN)

clean:
	rm -f $(MAIN_BIN) $(TEST_BIN)
