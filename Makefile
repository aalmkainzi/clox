.PHONY: all format
SRC := $(wildcard *.c) external/cgs/cgs.c
OBJ := $(addprefix build/obj/,$(notdir $(SRC:.c=.o)))
DEP := $(OBJ:.o=.d)
VPATH := external/cgs
vpath %.h include

HDR := $(wildcard include/*.h) $(wildcard *.h)
FMT_SRC := $(wildcard *.c)

SANITIZE := -fsanitize=address,undefined
CFLAGS := -Wall -Wextra -Iinclude -Iexternal -Iexternal/STC/include -ggdb $(SANITIZE)
CPPFLAGS := -MMD -MP
LDFLAGS := -lm $(SANITIZE)

all: format clox

FMT_STAMPS := $(addprefix build/fmt/,$(notdir $(FMT_SRC))) $(addprefix build/fmt/,$(notdir $(HDR)))
FMT_STAMPS := $(addsuffix .fmt,$(FMT_STAMPS))

format: $(FMT_STAMPS)

clox: $(OBJ)
	$(CC) $(OBJ) -o clox $(LDFLAGS)

build/fmt/%.c.fmt: %.c | build/fmt
	clang-format -i $<
	touch $@

build/fmt/%.h.fmt: %.h | build/fmt
	clang-format -i $<
	touch $@

build/fmt:
	mkdir -p $@


build/obj/cgs.o: external/cgs/cgs.c | build/obj
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@


build/obj/%.o: %.c build/fmt/%.c.fmt | build/obj
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

build/obj:
	mkdir -p $@

-include $(DEP)