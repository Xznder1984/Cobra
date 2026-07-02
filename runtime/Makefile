CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L -fPIC
AR = ar

SRC = src/runtime.c src/thread.c src/io.c
OBJ = $(SRC:.c=.o)
LIB = libcobra_runtime.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ) $(LIB)
