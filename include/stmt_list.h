#ifndef GROW_LIST_H
#define GROW_LIST_H

#include "arena.h"
#include <stddef.h>

// never reallocates a buffer when growing
// TODO grow list should use the arena of a Stmt (each Stmt should have an arena)

typedef struct StmtList
{
    struct StmtListNode *head;
    struct StmtListNode *tail;
    struct StmtListNode *pool;
    Arena *arena;
    unsigned int next_free_slot_in_tail;
    size_t count;
} StmtList;

typedef struct StmtListIter
{
    struct StmtListNode *node;
    struct Stmt *ptr;
} StmtListIter;

typedef struct StmtListCheckpoint
{
    StmtListIter it;
    size_t count;
} StmtListCheckpoint;

StmtList stmt_list(Arena *arena);

StmtListIter stmt_list_push(StmtList *list);

StmtListIter stmt_list_begin(StmtList *list);
StmtListIter stmt_list_end();
StmtListIter stmt_list_next(StmtList *list, StmtListIter it);

void stmt_list_erase_after(StmtList *list, StmtListIter iter);

StmtListCheckpoint stmt_list_checkpoint(StmtList *list);
void stmt_list_restore_checkpoint(StmtList *list, StmtListCheckpoint checkpoint);

#define stmt_list_each(name, list)                                            \
    StmtListIter name = stmt_list_begin(list), name##_end_ = stmt_list_end(); \
    name.ptr != name##_end_.ptr;                                              \
    name = stmt_list_next((list), name)

#define stmt_list_range(name, begin, end) \
    StmtListIter name = (begin);          \
    name != end.ptr;                      \
    name = stmt_list_next((list), name)

#endif
