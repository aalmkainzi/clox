.PHONY: all format test
CGS_SRC := external/cgs/cgs.c
CLOX_SRC := $(wildcard *.c)
CLOX_HDR := $(wildcard include/*.h) $(wildcard *.h)
SRC := $(wildcard *.c) $(CGS_SRC)
OBJ := $(addprefix build/obj/,$(notdir $(SRC:.c=.o)))
DEP := $(OBJ:.o=.d)
VPATH := external/cgs

TEST_SRC := test/runner.c

SANITIZE := -fsanitize=address,undefined
CFLAGS := -std=gnu23 -Wall -Wextra -ggdb $(SANITIZE)
INCLUDE_DIRS := -Iinclude -Iexternal -Iexternal/STC/include
CPPFLAGS := -MMD -MP
LDFLAGS := -lm $(SANITIZE)

all: clox

format:
	clang-format -i $(CLOX_SRC) $(CLOX_HDR)

clox: $(OBJ)
	$(CC) $(OBJ) -o clox $(LDFLAGS)

build/obj/%.o: %.c | build/obj
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(CPPFLAGS) -c $< -o $@

build/obj:
	mkdir -p $@

test/run: $(TEST_SRC)
	$(CC) $(TEST_SRC) $(CGS_SRC) $(CFLAGS) $(INCLUDE_DIRS) -o test/run

test: test/run clox
	test/run

-include $(DEP)
