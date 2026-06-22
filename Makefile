.PHONY: all
SRC :=  $(wildcard *.c) external/cgs/cgs.c
CFLAGS := -Iexternal -Iexternal/STC/include -o clox

all: $(SRC)
	$(CC) $(CFLAGS) $(SRC)


