#ifndef ARENA_H
#define ARENA_H

#include "cgs/cgs.h"
#include <stddef.h>

typedef struct ArenaNode
{
    struct ArenaNode *next;
    size_t size;
    alignas(8) unsigned char mem[];
} ArenaNode;

typedef struct Arena
{
    size_t offset_in_last_arena;
    ArenaNode *head;
    ArenaNode *tail;
    ArenaNode *node_pool;
} Arena;

typedef struct ArenaAllocator
{
    CGS_Allocator base;
    Arena *arena;
} ArenaAllocator;

void arena_init(Arena *arena);
void *arena_alloc(Arena *arena, size_t n);
void arena_reset(Arena *arena);
void arena_free(Arena *arena);

// sets the callbacks of the passed ArenaAllocator, and returns it as a CGS_Allocator*
CGS_Allocator *arena_allocator_set_callbacks(ArenaAllocator *aa);

#endif
