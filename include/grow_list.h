// never reallocates a buffer when growing. made to be backed by an arena allocator
// mimics STC style containers

#include "stc/common.h"
#include "stc/priv/linkage.h"
#include "stc/types.h"

#include "arena.h"
#include <stddef.h>

#ifndef _i_prefix
    #define _i_prefix glist_
#endif

#include "stc/priv/template.h"

#define GL_ITER _c_MEMB(_iter)
#define GL_NODE _c_MEMB(_node)

typedef struct GL_NODE GL_NODE;

typedef i_key _m_value;

typedef struct Self
{
    Arena *arena;
    GL_NODE *head;
    GL_NODE *tail;
    GL_NODE *pool;
    unsigned int next_free_slot_in_tail;
    size_t size;
} Self;

typedef struct GL_ITER
{
    typeof(_m_value) *ref;
    GL_NODE *node;
    unsigned int next_free_slot_in_tail; // this is needed to know when to stop
} GL_ITER;

Self _c_MEMB(_init)(Arena *arena);
GL_ITER _c_MEMB(_push)(Self *list);
GL_ITER _c_MEMB(_back)(const Self *list);
void _c_MEMB(_drop)(Self *list);
GL_ITER _c_MEMB(_begin)(const Self *list);
GL_ITER _c_MEMB(_end)();
void _c_MEMB(_next)(GL_ITER *it);
void _c_MEMB(_prev)(GL_ITER *it);

#if defined i_implement

struct GL_NODE
{
    GL_NODE *next;
    GL_NODE *prev;
    typeof(_m_value) elms[(512 - sizeof(void *[2])) / sizeof(_m_value)];
};

static_assert(sizeof(GL_NODE) <= 512 && sizeof(GL_NODE) != sizeof(void *[2]));

    #define gl_node_cap(node) ((ptrdiff_t)(sizeof((node)->elms) / sizeof(_m_value)))

Self _c_MEMB(_init)(Arena *arena)
{
    return (Self) {.arena = arena};
}

static GL_NODE *_c_MEMB(_new_node)(Self *list)
{
    GL_NODE *node = arena_alloc(list->arena, sizeof(GL_NODE));
    node->next    = NULL;
    return node;
}

GL_ITER _c_MEMB(_push)(Self *list)
{
    if (list->tail)
    {
        if (list->next_free_slot_in_tail >= gl_node_cap(list->tail))
        {
            if (list->pool)
            {
                list->tail->next = list->pool;
                list->tail->next->prev = list->tail;
                list->tail = list->tail->next;
                list->pool                    = list->pool->next;
                list->tail->next              = NULL;
            }
            else
            {
                list->tail->next = _c_MEMB(_new_node)(list);
                list->tail->next->prev = list->tail;
                list->tail =  list->tail->next;
            }
            list->next_free_slot_in_tail = 0;
        }
    }
    else
    {
        assert(list->head == NULL);
        if (list->pool)
        {
            list->head = list->tail = list->pool;
            list->pool = list->pool->next;
            list->head->next = NULL;
        }
        else
        {
            list->head = list->tail = _c_MEMB(_new_node)(list);
        }
        assert(list->next_free_slot_in_tail == 0);
    }

    GL_ITER pushed = {
        .ref                    = &list->tail->elms[list->next_free_slot_in_tail],
        .node                   = list->tail,
        .next_free_slot_in_tail = list->next_free_slot_in_tail
    };

    list->size++;
    list->next_free_slot_in_tail++;

    return pushed;
}

GL_ITER _c_MEMB(_back)(const Self *list)
{
    if (list->size == 0)
        return _c_MEMB(_end)();

    return (GL_ITER) {
        .ref                    = &list->tail->elms[list->next_free_slot_in_tail - 1],
        .node                   = list->tail,
        .next_free_slot_in_tail = list->next_free_slot_in_tail
    };
}

void _c_MEMB(_drop)(Self *list)
{
    #ifdef i_keydrop
    for (c_each(it, Self, *list))
    {
        i_keydrop(it.ref);
    }
    #endif
}

GL_ITER _c_MEMB(_begin)(const Self *list)
{
    return (GL_ITER) {
        .ref                    = list->head ? &list->head->elms[0] : NULL,
        .node                   = list->head,
        .next_free_slot_in_tail = list->next_free_slot_in_tail
    };
}

GL_ITER _c_MEMB(_end)()
{
    return (GL_ITER) {
        .ref  = NULL,
        .node = NULL,
    };
}

void _c_MEMB(_next)(GL_ITER *it)
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
            *it = _c_MEMB(_end)();
        }
    }
    else
    {
        // we are at tail, and next slot is the next free slot = next is the end
        if (it->node->next == NULL && (it->ref - it->node->elms == it->next_free_slot_in_tail - 1))
        {
            *it = _c_MEMB(_end)();
        }
        else
        {
            it->ref += 1;
        }
    }
}

void _c_MEMB(_prev)(GL_ITER *it)
{
    if (it->ref - it->node->elms == 0)
    {
        it->node = it->node->prev;
        if (it->node)
            it->ref = &it->node->elms[gl_node_cap(it->node) - 1];
        else
            it->ref = NULL;
    }
    else
    {
        it->ref -= 1;
    }
}

#endif // i_implement

#include "stc/sys/finalize.h"
