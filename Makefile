.PHONY: all
SRC := $(wildcard *.c) external/cgs/cgs.c
OBJ := $(addprefix build/obj/,$(notdir $(SRC:.c=.o)))
DEP := $(OBJ:.o=.d)
VPATH := external/cgs

SANITIZE := -fsanitize=address,undefined
CFLAGS := -Wall -Wextra -Iinclude -Iexternal -Iexternal/STC/include -ggdb $(SANITIZE)
CPPFLAGS := -MMD -MP
LDFLAGS := -lm $(SANITIZE)

all: clox

clox: $(OBJ)
	$(CC) $(OBJ) -o clox $(LDFLAGS)

build/obj/%.o: %.c | build/obj
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

build/obj:
	mkdir -p $@

-include $(DEP)
