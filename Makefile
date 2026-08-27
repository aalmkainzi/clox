.PHONY: all
SRC := $(wildcard *.c) external/cgs/cgs.c
OBJ := $(addprefix build/obj/,$(notdir $(SRC:.c=.o)))
VPATH := external/cgs

SANITIZE := -fsanitize=address,undefined
CFLAGS := -Wall -Wextra -Iinclude -Iexternal -Iexternal/STC/include -ggdb $(SANITIZE)
LDFLAGS := -lm $(SANITIZE)

.PHONY: all

all: clox

clox: $(OBJ)
	$(CC) $(OBJ) -o clox $(LDFLAGS)

build/obj/%.o: %.c | build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/obj:
	mkdir -p $@
