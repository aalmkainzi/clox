#include "arena.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

void arena_init(Arena *arena)
{
    memset(arena, 0, sizeof(Arena));
}

static ArenaNode *new_node(Arena *arena, size_t n)
{
    arena->offset_in_last_arena = 0;
    if (arena->node_pool == NULL)
    {
    alloc_new_node:
        size_t size = 1024;
        if (size < n)
            size = n;
        ArenaNode *ret = malloc(sizeof(ArenaNode) + size);
        ret->next      = NULL;
        return ret;
    }
    else
    {
        ArenaNode *node = arena->node_pool;
        while (node && node->size < n)
            node = node->next;
        if (node)
            return node;
        else
            goto alloc_new_node;
    }
}

void *arena_alloc(Arena *arena, size_t n)
{
    if (arena->tail == NULL)
    {
        arena->tail = new_node(arena, n);
    }
    else if (arena->tail->size - arena->offset_in_last_arena >= n)
    {
        arena->tail = arena->tail->next = new_node(arena, n);
    }

    void *ret = arena->tail->mem + arena->offset_in_last_arena;
    arena->offset_in_last_arena += n;

    arena->offset_in_last_arena = (arena->offset_in_last_arena + 7) & ~(size_t)7;

    return ret;
}

void arena_reset(Arena *arena)
{
#if defined(NDEBUG)
    arena_free(arena);
#else
    arena->node_pool            = arena->head;
    arena->head                 = NULL;
    arena->offset_in_last_arena = 0;
#endif
}

void free_node(ArenaNode *node)
{
    if (node == NULL)
        return;
    free_node(node->next);
    free(node);
}

void arena_free(Arena *arena)
{
    free_node(arena->head);
    free_node(arena->node_pool);
    memset(arena, 0, sizeof(Arena));
}

#include "cgs/cgs.h"

static CGS_Allocation arena_alloc_callback(CGS_Allocator *a, size_t alignment, size_t n)
{
    (void)alignment;
    ArenaAllocator *aa = (ArenaAllocator *)a;
    void *p            = arena_alloc(aa->arena, n);
    return (CGS_Allocation) {.ptr = p, .n = n};
}

static void arena_dealloc_callback(CGS_Allocator *a, void *p, size_t n)
{
    (void)a;
    (void)p;
    (void)n;
    return;
}

static CGS_Allocation
arena_realloc_callback(CGS_Allocator *a, void *p, size_t alignment, size_t old_n, size_t new_n)
{
    (void)p;
    (void)old_n;
    return arena_alloc_callback(a, alignment, new_n);
}

CGS_Allocator *arena_allocator_set_callbacks(ArenaAllocator *aa)
{
    aa->base.alloc   = arena_alloc_callback;
    aa->base.dealloc = arena_dealloc_callback;
    aa->base.realloc = arena_realloc_callback;
    return (CGS_Allocator*) aa;
}
