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

CGS_Allocation arena_alloc_callback(CGS_Allocator *a, size_t alignment, size_t n);
void arena_dealloc_callback(CGS_Allocator *a, void *p, size_t n);
CGS_Allocation arena_realloc_callback(CGS_Allocator *a, void *p, size_t alignment, size_t old_n, size_t new_n);

extern CGS_Allocator *arena_allocator_stack;

#define arena_allocator(arena_p)                   \
    ((CGS_Allocator *)&(ArenaAllocator) {          \
        .base =                                    \
            {                                      \
                .alloc   = arena_alloc_callback,   \
                .dealloc = arena_dealloc_callback, \
                .realloc = arena_realloc_callback, \
            },                                     \
        .arena = (arena_p)                         \
    })

#endif
