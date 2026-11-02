#include "grow_list.h"
#include "arena.h"
#include "stmt.h"

typedef struct GrowListNode
{
    struct GrowListNode *next;
    struct Stmt stmts[64];
} GrowListNode;

#define grow_list_node_cap(list) ((ptrdiff_t)(sizeof((list)->tail->stmts) / sizeof(Stmt)))

GrowList grow_list(Arena *arena)
{
    return (GrowList){.arena = arena};
}

static GrowListNode *new_node(GrowList *list)
{
    GrowListNode *node = arena_alloc(list->arena, sizeof(GrowListNode));
    node->next         = NULL;
    return node;
}

GrowListIter grow_list_push(GrowList *list)
{
    if (list->tail)
    {
        if (list->next_free_slot_in_tail >= grow_list_node_cap(list))
        {
            if (list->pool)
            {
                list->tail = list->tail->next = list->pool;
                list->pool                    = list->pool->next;
                list->tail->next              = NULL;
            }
            else
            {
                list->tail = list->tail->next = new_node(list);
            }
            list->next_free_slot_in_tail = 0;
        }
    }
    else
    {
        assert(list->head == NULL);
        list->head = list->tail = new_node(list);
        assert(list->next_free_slot_in_tail == 0);
    }

    GrowListIter pushed = {
        .ptr  = &list->tail->stmts[list->next_free_slot_in_tail],
        .node = list->tail,
    };

    list->count++;
    list->next_free_slot_in_tail++;

    return pushed;
}

GrowListIter grow_list_begin(GrowList *list)
{
    return (GrowListIter) {
        .node = list->head,
        .ptr  = list->head ? &list->head->stmts[0] : NULL,
    };
}

GrowListIter grow_list_end()
{
    return (GrowListIter) {
        .node = NULL,
        .ptr  = NULL,
    };
}

GrowListIter grow_list_next(GrowList *list, GrowListIter it)
{
    if ((it.ptr - it.node->stmts) + 1 >= grow_list_node_cap(list))
    {
        if (it.node->next)
        {
            return (GrowListIter) {
                .node = it.node->next,
                .ptr  = it.node->next->stmts,
            };
        }
        else
        {
            return grow_list_end();
        }
    }
    else
    {
        if (it.node == list->tail && (it.ptr - it.node->stmts == list->next_free_slot_in_tail - 1))
        {
            return grow_list_end();
        }
        return (GrowListIter) {
            .node = it.node,
            .ptr  = it.ptr + 1,
        };
    }
}

void grow_list_erase_after(GrowList *list, GrowListIter iter)
{
    GrowListNode *node = iter.node;
    if (node == list->tail)
    {
        list->next_free_slot_in_tail = (iter.ptr - node->stmts) + 1;
    }
    else
    {
        GrowListNode *old_tail = list->tail;
        old_tail->next         = list->pool;
        list->pool             = node->next;
        list->tail             = node;
        list->tail->next       = NULL;

        list->next_free_slot_in_tail = (iter.ptr - node->stmts) + 1;
    }
}

void grow_list_restore_checkpoint(GrowList *list, GrowListCheckpoint checkpoint)
{
    grow_list_erase_after(list, checkpoint.it);
    list->count = checkpoint.count;
}
