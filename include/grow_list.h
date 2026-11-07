// never reallocates a buffer when growing. made to be backed by an arena allocator

#ifndef GL_TYPE
    #error "define GL_TYPE before including"
#endif
#ifndef GL_NAME
    #error "define GL_NAME before including"
#endif
#ifndef GL_PREFIX
    #define GL_PREFIX GL_NAME
#endif

#include "arena.h"
#include "stc/common.h"
#include <stddef.h>

#define GL_ITER c_JOIN(GL_NAME, _iter)
#define GL_NODE c_JOIN(GL_NAME, _node)

typedef struct GL_NODE GL_NODE;

typedef struct GL_NAME
{
    Arena *arena;
    GL_NODE *head;
    GL_NODE *tail;
    GL_NODE *pool;
    unsigned int next_free_slot_in_tail;
    size_t count;
} GL_NAME;

typedef struct GL_ITER
{
    GL_NODE *node;
    typeof(GL_TYPE) *ptr;
} GL_ITER;

GL_NAME c_JOIN(GL_PREFIX, _create)(Arena *arena);
GL_ITER c_JOIN(GL_PREFIX, _push)(GL_NAME *list);
GL_ITER c_JOIN(GL_PREFIX, _begin)(GL_NAME *list);
GL_ITER c_JOIN(GL_PREFIX, _end)();
GL_ITER c_JOIN(GL_PREFIX, _next)(GL_NAME *list, GL_ITER it);

#if defined(GROW_LIST_IMPL)

struct GL_NODE
{
    GL_NODE *next;
    typeof(GL_TYPE) elms[(512 - sizeof(void*)) / sizeof(GL_TYPE)];
};

static_assert(sizeof(GL_NODE) <= 512 && sizeof(GL_NODE) != sizeof(void*));

#define gl_node_cap(list) ((ptrdiff_t)(sizeof((list)->tail->elms) / sizeof(GL_TYPE)))

GL_NAME c_JOIN(GL_PREFIX, _create)(Arena *arena)
{
    return (GL_NAME) {.arena = arena};
}

static GL_NODE *c_JOIN(GL_NAME, _new_node)(GL_NAME *list)
{
    GL_NODE *node = arena_alloc(list->arena, sizeof(GL_NODE));
    node->next    = NULL;
    return node;
}

GL_ITER c_JOIN(GL_PREFIX, _push)(GL_NAME *list)
{
    if (list->tail)
    {
        if (list->next_free_slot_in_tail >= gl_node_cap(list))
        {
            if (list->pool)
            {
                list->tail = list->tail->next = list->pool;
                list->pool                    = list->pool->next;
                list->tail->next              = NULL;
            }
            else
            {
                list->tail = list->tail->next = c_JOIN(GL_NAME, _new_node)(list);
            }
            list->next_free_slot_in_tail = 0;
        }
    }
    else
    {
        assert(list->head == NULL);
        list->head = list->tail = c_JOIN(GL_NAME, _new_node)(list);
        assert(list->next_free_slot_in_tail == 0);
    }

    GL_ITER pushed = {
        .ptr  = &list->tail->elms[list->next_free_slot_in_tail],
        .node = list->tail,
    };

    list->count++;
    list->next_free_slot_in_tail++;

    return pushed;
}

GL_ITER c_JOIN(GL_PREFIX, _begin)(GL_NAME *list)
{
    return (GL_ITER) {
        .node = list->head,
        .ptr  = list->head ? &list->head->elms[0] : NULL,
    };
}

GL_ITER c_JOIN(GL_PREFIX, _end)()
{
    return (GL_ITER) {
        .node = NULL,
        .ptr  = NULL,
    };
}

GL_ITER c_JOIN(GL_PREFIX, _next)(GL_NAME *list, GL_ITER it)
{
    if ((it.ptr - it.node->elms) + 1 >= gl_node_cap(list))
    {
        if (it.node->next)
        {
            return (GL_ITER) {
                .node = it.node->next,
                .ptr  = it.node->next->elms,
            };
        }
        else
        {
            return c_JOIN(GL_PREFIX, _end)();
        }
    }
    else
    {
        if (it.node == list->tail && (it.ptr - it.node->elms == list->next_free_slot_in_tail - 1))
        {
            return c_JOIN(GL_PREFIX, _end)();
        }
        return (GL_ITER) {
            .node = it.node,
            .ptr  = it.ptr + 1,
        };
    }
}

#endif // IMPL

#undef GL_NAME
#undef GL_TYPE
#undef GL_PREFIX
#undef GL_NODE
#undef GL_ITER