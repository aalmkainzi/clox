#ifndef GROW_LIST_H
#define GROW_LIST_H

#include "arena.h"
#include <stddef.h>

// never reallocates a buffer when growing
// TODO grow list should use the arena of a Stmt (each Stmt should have an arena)

typedef struct GrowList
{
    struct GrowListNode *head;
    struct GrowListNode *tail;
    struct GrowListNode *pool;
    Arena *arena;
    unsigned int next_free_slot_in_tail;
    size_t count;
} GrowList;

typedef struct GrowListIter
{
    struct GrowListNode *node;
    struct Stmt *ptr;
} GrowListIter;

typedef struct GrowListCheckpoint
{
    GrowListIter it;
    size_t count;
} GrowListCheckpoint;

GrowList grow_list(Arena *arena);

GrowListIter grow_list_push(GrowList *list);

GrowListIter grow_list_begin(GrowList *list);
GrowListIter grow_list_end();
GrowListIter grow_list_next(GrowList *list, GrowListIter it);

void grow_list_erase_after(GrowList *list, GrowListIter iter);

GrowListCheckpoint grow_list_checkpoint(GrowList *list);
void grow_list_restore_checkpoint(GrowList *list, GrowListCheckpoint checkpoint);

#define grow_list_each(name, list)                                            \
    GrowListIter name = grow_list_begin(list), name##_end_ = grow_list_end(); \
    name.ptr != name##_end_.ptr;                                              \
    name = grow_list_next((list), name)

#define grow_list_range(name, begin, end) \
    GrowListIter name = (begin);          \
    name != end.ptr;                      \
    name = grow_list_next((list), name)

#endif
