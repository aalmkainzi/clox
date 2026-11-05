#ifndef ARENA_H
#define ARENA_H

#include "cgs/cgs.h"
#include "stc/types.h"
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

typedef struct ArenaCheckpoint
{
    ArenaNode *node;
    size_t offset;
} ArenaCheckpoint;

typedef struct ArenaAllocator
{
    CGS_Allocator base;
    Arena *arena;
} ArenaAllocator;

void arena_init(Arena *arena);
void *arena_alloc(Arena *arena, size_t n);
void *arena_calloc(Arena *arena, size_t n);
void arena_reset(Arena *arena);
void arena_free(Arena *arena);

ArenaCheckpoint arena_checkpoint(Arena *arena);
void arena_rollback(Arena *arena, ArenaCheckpoint checkpoint);

CGS_Allocation arena_alloc_callback(CGS_Allocator *a, size_t alignment, size_t n);
void arena_dealloc_callback(CGS_Allocator *a, void *p, size_t n);
CGS_Allocation arena_realloc_callback(CGS_Allocator *a, void *p, size_t alignment, size_t old_n, size_t new_n);

c_declare_list(Arenas, Arena);

// begin a region that should be freed together (e.g. scope)
Arenas_iter begin_arena();
void end_arena(Arenas_iter arena);

// ignores the current stack of arenas, uses the passed one until unset
// void set_current_arena(Arena *arena);
// void unset_current_arena();

// extern CGS_Allocator *arena_stack_allocator;

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
