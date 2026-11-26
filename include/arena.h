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
    size_t bytes_used_in_last_node;
    ArenaNode *head;
    ArenaNode *tail;
    ArenaNode *node_pool;
} Arena;

typedef struct ArenaCheckpoint
{
    ArenaNode *node;
    size_t used;
} ArenaCheckpoint;

typedef struct ArenaAllocator
{
    CGS_Allocator base;
    Arena *arena;
} ArenaAllocator;

void *arena_alloc(Arena *arena, size_t n);
void *arena_calloc(Arena *arena, size_t n);
void arena_reset(Arena *arena);
void arena_free(Arena *arena);

ArenaCheckpoint arena_checkpoint(Arena *arena);
void arena_rollback(Arena *arena, ArenaCheckpoint checkpoint);

CGS_Allocation arena_alloc_callback(CGS_Allocator *a, size_t alignment, size_t n);
void arena_dealloc_callback(CGS_Allocator *a, void *p, size_t n);
CGS_Allocation arena_realloc_callback(CGS_Allocator *a, void *p, size_t alignment, size_t old_n, size_t new_n);

#define arena_allocator(arena_ptr)                 \
    (ArenaAllocator)                               \
    {                                              \
        .base =                                    \
            {                                      \
                .alloc   = arena_alloc_callback,   \
                .realloc = arena_realloc_callback, \
                .dealloc = arena_dealloc_callback, \
            },                                     \
        .arena = arena_ptr                         \
    }

#endif
