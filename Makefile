.PHONY: all
SRC :=  $(wildcard *.c) external/cgs/cgs.c
CFLAGS := -Wall -Wextra -Iexternal -Iexternal/STC/include -fsanitize=address,undefined -ggdb -o clox
LDFLAGS := -lm

all: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS)


