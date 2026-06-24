.PHONY: all
SRC :=  $(wildcard *.c) external/cgs/cgs.c
CFLAGS := -Iexternal -Iexternal/STC/include -ggdb -o clox
LDFLAGS := -lm

all: $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS)


