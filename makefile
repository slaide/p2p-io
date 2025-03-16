
CC=clang -std=c23 -Wall -Wpedantic -Wextra -O0

main: main.c
	$(CC) -o main main.c

.PHONY: clean
clean:
	rm -f main
