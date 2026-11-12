// never reallocates a buffer when growing. made to be backed by an arena allocator

#ifndef GL_TYPE
    #error "define GL_TYPE before including"
#endif
#ifndef GL_NAME
    #error "define GL_NAME before including"
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
    typeof(GL_TYPE) *ref;
    GL_NODE *node;
    unsigned int next_free_slot_in_tail; // this is needed to know when to stop
} GL_ITER;

GL_NAME c_JOIN(GL_NAME, _init)(Arena *arena);
GL_ITER c_JOIN(GL_NAME, _push)(GL_NAME *list);
GL_ITER c_JOIN(GL_NAME, _begin)(GL_NAME *list);
GL_ITER c_JOIN(GL_NAME, _end)();
void c_JOIN(GL_NAME, _next)(GL_ITER *it);

#if defined(GL_IMPL)

struct GL_NODE
{
    GL_NODE *next;
    typeof(GL_TYPE) elms[(512 - sizeof(void *)) / sizeof(GL_TYPE)];
};

static_assert(sizeof(GL_NODE) <= 512 && sizeof(GL_NODE) != sizeof(void *));

    #define gl_node_cap(node) ((ptrdiff_t)(sizeof((node)->elms) / sizeof(GL_TYPE)))

GL_NAME c_JOIN(GL_NAME, _init)(Arena *arena)
{
    return (GL_NAME) {.arena = arena};
}

static GL_NODE *c_JOIN(GL_NAME, _new_node)(GL_NAME *list)
{
    GL_NODE *node = arena_alloc(list->arena, sizeof(GL_NODE));
    node->next    = NULL;
    return node;
}

GL_ITER c_JOIN(GL_NAME, _push)(GL_NAME *list)
{
    if (list->tail)
    {
        if (list->next_free_slot_in_tail >= gl_node_cap(list->tail))
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
        .ref                    = &list->tail->elms[list->next_free_slot_in_tail],
        .node                   = list->tail,
        .next_free_slot_in_tail = list->next_free_slot_in_tail
    };

    list->count++;
    list->next_free_slot_in_tail++;

    return pushed;
}

GL_ITER c_JOIN(GL_NAME, _begin)(GL_NAME *list)
{
    return (GL_ITER) {
        .ref                    = list->head ? &list->head->elms[0] : NULL,
        .node                   = list->head,
        .next_free_slot_in_tail = list->next_free_slot_in_tail
    };
}

GL_ITER c_JOIN(GL_NAME, _end)()
{
    return (GL_ITER) {
        .ref  = NULL,
        .node = NULL,
    };
}

void c_JOIN(GL_NAME, _next)(GL_ITER *it)
{
    if ((it->ref - it->node->elms) + 1 >= gl_node_cap(it->node))
    {
        if (it->node->next)
        {
            it->ref  = it->node->next->elms;
            it->node = it->node->next;
        }
        else
        {
            *it = c_JOIN(GL_NAME, _end)();
        }
    }
    else
    {
        // we are at tail, and next slot is the next free slot = next is the end
        if (it->node->next == NULL && (it->ref - it->node->elms == it->next_free_slot_in_tail - 1))
        {
            *it = c_JOIN(GL_NAME, _end)();
        }
        else
        {
            it->ref += 1;
        }
    }
}

#endif // IMPL

#undef GL_NAME
#undef GL_TYPE
#undef GL_NODE
#undef GL_ITER
#undef GL_IMPL
