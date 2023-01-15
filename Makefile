CC=gcc
LDFLAGS=-pthread
DFLAGS=-g -O0
CFLAGS = -Wall -Wextra

main: main.c
	$(CC) $(CFLAGS) -o main main.c

clean:
	rm -f main

all: clean main

run:
	./main

debug:
	$(CC) $(CFLAGS) $(DFLAGS) $(LDFLAGS) -o main main.c
	gdb ./main
