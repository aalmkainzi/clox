.PHONY: all
SRC := $(wildcard *.c) external/cgs/cgs.c
OBJ := $(patsubst %.c,build/obj/%.o,$(SRC))
OBJDIRS := $(sort $(dir $(OBJ)))
CFLAGS := -Wall -Wextra -Iinclude -Iexternal -Iexternal/STC/include -ggdb -fsanitize=address,undefined
LDFLAGS := -lm

all: clox

clox: $(OBJ)
	$(CC) $(OBJ) -o clox $(LDFLAGS)

build/obj/%.o: %.c | $(OBJDIRS)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIRS):
	mkdir -p $@