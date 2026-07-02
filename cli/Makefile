CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L -I../compiler/src
LDFLAGS = -lm -lpthread

SRC = src/main.c
OBJ = $(SRC:.c=.o)
BIN = bin/cobra

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf bin
	rm -rf src/*.o
