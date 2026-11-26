#include "arena.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#ifndef ARENA_DEBUG
    #ifndef NDEBUG
        #define ARENA_DEBUG 1
    #endif
#endif

#if defined(__SANITIZE_ADDRESS__)
    #include <sanitizer/asan_interface.h>
    #define POISON_MEM(p, n) ASAN_POISON_MEMORY_REGION((p), (n))
    #define UNPOISON_MEM(p, n) ASAN_UNPOISON_MEMORY_REGION((p), (n))
#else
    #define POISON_MEM(p, n)
    #define UNPOISON_MEM(p, n)
#endif

static void poison_starting_from(ArenaNode *node)
{
    while (node)
    {
        POISON_MEM(node->mem, node->size);
        node = node->next;
    }
}

void arena_init(Arena *arena)
{
    memset(arena, 0, sizeof(Arena));
}

static ArenaNode *new_node(Arena *arena, size_t n)
{
    arena->bytes_used_in_last_node = 0;

    // TODO clean this up
    ArenaNode *prev = NULL;
    ArenaNode *node = arena->node_pool;
    while (node && node->size < n)
    {
        prev = node;
        node = node->next;
    }
    if (node)
    {
        if (prev)
        {
            prev->next = node->next;
            node->next = NULL;
        }
        else
        {
            arena->node_pool = node->next;
            node->next       = NULL;
        }
        return node;
    }
    else
    {
        size_t size = 1024;
        if (size < n)
        {
            size = n;
            size += 7;
            size &= ~7;
        }

        ArenaNode *ret = malloc(sizeof(ArenaNode) + size);
        ret->next      = NULL;
        ret->size      = size;
        POISON_MEM(ret->mem, ret->size);
        return ret;
    }
}

void *arena_alloc(Arena *arena, size_t n)
{
    if (arena->tail == NULL)
    {
        arena->head = arena->tail = new_node(arena, n);
    }
    else if (arena->tail->size - arena->bytes_used_in_last_node < n)
    {
        arena->tail = arena->tail->next = new_node(arena, n);
    }

    UNPOISON_MEM(arena->tail->mem + arena->bytes_used_in_last_node, n);

    void *ret = arena->tail->mem + arena->bytes_used_in_last_node;
    arena->bytes_used_in_last_node += n;

    arena->bytes_used_in_last_node = (arena->bytes_used_in_last_node + 7) & ~(size_t)7;

    return ret;
}

void *arena_calloc(Arena *arena, size_t n)
{
    void *p = arena_alloc(arena, n);
    memset(p, 0, n);
    return p;
}

void arena_reset(Arena *arena)
{
#if ARENA_DEBUG
    arena_free(arena);
#else
    if (arena->tail)
    {
        poison_starting_from(arena->head);

        arena->tail->next = arena->node_pool;
        arena->node_pool  = arena->head;
    }

    arena->head = arena->tail   = NULL;
    arena->offset_in_last_arena = 0;
#endif
}

void free_node(ArenaNode *node)
{
    while (node)
    {
        ArenaNode *next = node->next;
        free(node);
        node = next;
    }
}

void arena_free(Arena *arena)
{
    free_node(arena->head);
    free_node(arena->node_pool);
    memset(arena, 0, sizeof(Arena));
}

ArenaCheckpoint arena_checkpoint(Arena *arena)
{
    return (ArenaCheckpoint) {
        .node = arena->tail,
        .used = arena->bytes_used_in_last_node,
    };
}

void arena_rollback(Arena *arena, ArenaCheckpoint checkpoint)
{
    if (checkpoint.node)
    {
        if (arena->tail != checkpoint.node)
        {
            poison_starting_from(checkpoint.node->next);

            arena->tail->next     = arena->node_pool;
            arena->node_pool      = checkpoint.node->next;
            checkpoint.node->next = NULL;
            arena->tail           = checkpoint.node;
        }
        if (checkpoint.used != checkpoint.node->size)
        {
            size_t poison_len = checkpoint.node->size - checkpoint.used;
            POISON_MEM(checkpoint.node->mem + checkpoint.used, poison_len);
        }
    }
    else
    {
        poison_starting_from(arena->head);

        arena->tail->next = arena->node_pool;
        arena->node_pool  = arena->head;
        arena->head = arena->tail = NULL;
    }

    arena->bytes_used_in_last_node = checkpoint.used;
}

CGS_Allocation arena_alloc_callback(CGS_Allocator *a, size_t alignment, size_t n)
{
    (void)alignment;
    ArenaAllocator *aa = (ArenaAllocator *)a;
    void *p            = arena_alloc(aa->arena, n);
    return (CGS_Allocation) {.ptr = p, .n = n};
}

void arena_dealloc_callback(CGS_Allocator *a, void *p, size_t n)
{
    (void)a;
    (void)p;
    (void)n;
    return;
}

CGS_Allocation arena_realloc_callback(CGS_Allocator *a, void *p, size_t alignment, size_t old_n, size_t new_n)
{
    CGS_Allocation new_mem = arena_alloc_callback(a, alignment, new_n);
    size_t copy_n          = old_n < new_n ? old_n : new_n;
    if (copy_n)
        memcpy(new_mem.ptr, p, copy_n);
    return new_mem;
}
