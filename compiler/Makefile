CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L -I./src
LDFLAGS = -lm

SRC = src/main.c src/lexer.c src/parser.c src/ast.c src/semantic.c src/typechecker.c src/codegen.c src/ir.c src/compiler.c src/utils.c
OBJ = $(SRC:.c=.o)
BIN = bin/cobrac

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
