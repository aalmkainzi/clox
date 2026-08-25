.PHONY: all
SRC :=  $(wildcard *.c) external/cgs/cgs.c
CFLAGS := -Iinclude -Iexternal -Iexternal/STC/include -ggdb -fsanitize=address,undefined -o clox
LDFLAGS := -lm

all: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS)


