#include "arena.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#define T Arenas, Arena
#include "stc/list.h"

static Arenas arena_stack;
static Arena *manually_set_arena;

CGS_Allocation top_arena_alloc(CGS_Allocator *a, size_t alignment, size_t n);
void top_arena_dealloc(CGS_Allocator *a, void *p, size_t n);
CGS_Allocation top_arena_realloc(CGS_Allocator *a, void *p, size_t alignment, size_t old_n, size_t new_n);

CGS_Allocator *arena_allocator_stack = &(CGS_Allocator) {
    .alloc   = top_arena_alloc,
    .dealloc = top_arena_dealloc,
    .realloc = top_arena_realloc,
};

void arena_init(Arena *arena)
{
    memset(arena, 0, sizeof(Arena));
}

static ArenaNode *new_node(Arena *arena, size_t n)
{
    arena->offset_in_last_arena = 0;

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
        return ret;
    }
}

void *arena_alloc(Arena *arena, size_t n)
{
    if (arena->tail == NULL)
    {
        arena->head = arena->tail = new_node(arena, n);
    }
    else if (arena->tail->size - arena->offset_in_last_arena < n)
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
#if !defined(NDEBUG)
    arena_free(arena);
#else
    if (arena->tail)
    {
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
    memcpy(new_mem.ptr, p, old_n < new_n ? old_n : new_n);
    return new_mem;
}

Arena *begin_arena()
{
    return Arenas_push(&arena_stack, (Arena) {});
}

static void end_arena_node(Arenas_iter it)
{
    if (it.ref == NULL)
        return;

    Arenas_iter next = it;
    Arenas_next(&next);

    end_arena_node(next);
    arena_free(it.ref);
}

void end_arena(Arena *arena)
{
    Arenas_iter it = {.ref = arena, ._last = &arena_stack.last};
    end_arena_node(it);
}

CGS_Allocation top_arena_alloc(CGS_Allocator *a, size_t alignment, size_t n)
{
    (void)a;
    (void)alignment;

    Arena *arena;
    if (manually_set_arena)
        arena = manually_set_arena;
    else
        arena = Arenas_back_mut(&arena_stack);
    void *p = arena_alloc(arena, n);
    return (CGS_Allocation) {.ptr = p, .n = n};
}

void top_arena_dealloc(CGS_Allocator *a, void *p, size_t n)
{
    (void)a;
    (void)p;
    (void)n;
}

CGS_Allocation top_arena_realloc(CGS_Allocator *a, void *p, size_t alignment, size_t old_n, size_t new_n)
{
    (void)a;
    (void)p;
    (void)old_n;

    Arena *arena;
    if (manually_set_arena)
        arena = manually_set_arena;
    else
        Arenas_back_mut(&arena_stack);

    return arena_realloc_callback(arena_allocator(arena), p, alignment, old_n, new_n);
}

void set_current_arena(Arena *arena)
{
    manually_set_arena = arena;
}

void unset_current_arena()
{
    manually_set_arena = NULL;
}
