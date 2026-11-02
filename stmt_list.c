#include "arena.h"
#include "stmt.h"
#include "stmt_list.h"

typedef struct StmtListNode
{
    struct StmtListNode *next;
    struct Stmt stmts[64];
} StmtListNode;

#define stmt_list_node_cap(list) ((ptrdiff_t)(sizeof((list)->tail->stmts) / sizeof(Stmt)))

StmtList stmt_list(Arena *arena)
{
    return (StmtList) {.arena = arena};
}

static StmtListNode *new_node(StmtList *list)
{
    StmtListNode *node = arena_alloc(list->arena, sizeof(StmtListNode));
    node->next         = NULL;
    return node;
}

StmtListIter stmt_list_push(StmtList *list)
{
    if (list->tail)
    {
        if (list->next_free_slot_in_tail >= stmt_list_node_cap(list))
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

    StmtListIter pushed = {
        .ptr  = &list->tail->stmts[list->next_free_slot_in_tail],
        .node = list->tail,
    };

    list->count++;
    list->next_free_slot_in_tail++;

    return pushed;
}

StmtListIter stmt_list_begin(StmtList *list)
{
    return (StmtListIter) {
        .node = list->head,
        .ptr  = list->head ? &list->head->stmts[0] : NULL,
    };
}

StmtListIter stmt_list_end()
{
    return (StmtListIter) {
        .node = NULL,
        .ptr  = NULL,
    };
}

StmtListIter stmt_list_next(StmtList *list, StmtListIter it)
{
    if ((it.ptr - it.node->stmts) + 1 >= stmt_list_node_cap(list))
    {
        if (it.node->next)
        {
            return (StmtListIter) {
                .node = it.node->next,
                .ptr  = it.node->next->stmts,
            };
        }
        else
        {
            return stmt_list_end();
        }
    }
    else
    {
        if (it.node == list->tail && (it.ptr - it.node->stmts == list->next_free_slot_in_tail - 1))
        {
            return stmt_list_end();
        }
        return (StmtListIter) {
            .node = it.node,
            .ptr  = it.ptr + 1,
        };
    }
}

void stmt_list_erase_after(StmtList *list, StmtListIter iter)
{
    StmtListNode *node = iter.node;
    if (node == list->tail)
    {
        list->next_free_slot_in_tail = (iter.ptr - node->stmts) + 1;
    }
    else
    {
        StmtListNode *old_tail = list->tail;
        old_tail->next         = list->pool;
        list->pool             = node->next;
        list->tail             = node;
        list->tail->next       = NULL;

        list->next_free_slot_in_tail = (iter.ptr - node->stmts) + 1;
    }
}

void stmt_list_restore_checkpoint(StmtList *list, StmtListCheckpoint checkpoint)
{
    stmt_list_erase_after(list, checkpoint.it);
    list->count = checkpoint.count;
}
