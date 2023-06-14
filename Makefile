CFLAGS = -O2 -Wall -Wextra

all: flhex.c
	$(CC) $(CFLAGS) $^ -o flhex

clean:
	rm flhex
