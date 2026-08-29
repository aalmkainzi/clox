.PHONY: all format
SRC := $(wildcard *.c) external/cgs/cgs.c
OBJ := $(addprefix build/obj/,$(notdir $(SRC:.c=.o)))
DEP := $(OBJ:.o=.d)
VPATH := external/cgs

HDR := $(wildcard include/*.h) $(wildcard *.h)
FMT_SRC := $(wildcard *.c)

SANITIZE := -fsanitize=address,undefined
CFLAGS := -Wall -Wextra -Iinclude -Iexternal -Iexternal/STC/include -ggdb $(SANITIZE)
CPPFLAGS := -MMD -MP
LDFLAGS := -lm $(SANITIZE)

all: clox

format:
	clang-format -i $(FMT_SRC) $(HDR)

clox: $(OBJ)
	$(CC) $(OBJ) -o clox $(LDFLAGS)

build/obj/%.o: %.c | build/obj
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

build/obj:
	mkdir -p $@

-include $(DEP)
