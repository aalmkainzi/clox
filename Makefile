.PHONY: all format
CGS_SRC := external/cgs/cgs.c
SRC := $(wildcard *.c) $(CGS_SRC)
OBJ := $(addprefix build/obj/,$(notdir $(SRC:.c=.o)))
DEP := $(OBJ:.o=.d)
VPATH := external/cgs

TEST_SRC := test/test.c

HDR := $(wildcard include/*.h) $(wildcard *.h)
FMT_SRC := $(wildcard *.c)

SANITIZE := -fsanitize=address,undefined
CFLAGS := -Wall -Wextra -Iinclude -Iexternal -Iexternal/STC/include -std=gnu23 -ggdb $(SANITIZE)
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

test: all
	$(CC) $(TEST_SRC) $(CGS_SRC) $(CFLAGS) -o test/test
	test/test

-include $(DEP)
