#ifndef ARRAY_H_
#define ARRAY_H_

#include "alloc.h"

#define makeList(type, name) typedef struct {\
        type * items;\
        unsigned long len;\
        unsigned long cap;\
        Allocator mem;\
    } List ## name;\
    List ## name list ## name ## Init(unsigned long cap, Allocator mem);\
    void list ## name ## Deinit(List ## name * list);\
    int list ## name ## Grow(List ## name * list, unsigned long new_cap);\
    int list ## name ## Append(List ## name * list, type item);\
    void list ## name ## Remove(List ## name * list, unsigned long index);\
    void list ## name ## Bubblesort(List ## name * list, int (*predicate)(type *a, type *b))\


#define implementList(type, name) \
List ## name list ## name ## Init(unsigned long cap, Allocator mem) {\
    List ## name r = {0};\
    r.mem = mem;\
    if (list ## name ## Grow(&r, cap)) {\
        return (List ## name){0};\
    }\
    return r;\
}\
\
void list ## name ## Deinit(List ## name * list) {\
    if (list->items != NULL && list->mem.free != NULL) {\
        list->mem.free(list->items);\
        list->items = NULL;\
        list->cap = 0;\
        list->len = 0;\
    }\
}\
int list ## name ## Grow(List ## name * list, unsigned long new_cap) {\
    if (new_cap <= list->cap) {\
        return 0;\
    }\
    type * new_list;\
    if (list->items) {\
        new_list = ( type * ) list->mem.realloc(list->items, sizeof(type) * new_cap);\
        if (new_list == NULL) {\
            new_list = (type *) list->mem.alloc(sizeof(type) * new_cap);\
            if (new_list == NULL) {\
                return 1;\
            }\
            for (unsigned long i = 0; i < list->len; i++) {\
                new_list[i] = list->items[i];\
            }\
        }\
    }\
    else {\
        new_list = list->mem.alloc(sizeof(type) * new_cap);\
        if (new_list == NULL) {\
            return 1;\
        }\
    }\
    list->cap = new_cap;\
    list->items = new_list;\
    return 0;\
}\
int list ## name ## Append(List ## name * list, type item) {\
    if (list == NULL) {\
        return 1;\
    }\
    if (list->len >= list->cap) {\
        if (list ## name ## Grow(list, list->len + 10)) {\
            return 1;\
        }\
    }\
    list->items[list->len] = item;\
    list->len ++;\
    return 0;\
}\
void list ## name ## Remove(List ## name * list, unsigned long index) {\
    if (index >= list->len) {\
        return;\
    }\
\
    list->len --;\
    if (index < list->len)\
        copy_memory(list->items + index, list->items + index + 1, sizeof(type) * (list->len - index));\
}\
void list ## name ## RemoveSwap(List ## name * list, unsigned long index) {\
    if (index >= list->len) {\
        return;\
    }\
    list->len --;\
    if (index < list->len) {\
        list->items[index] = list->items[list->len];\
    }\
}\
void list ## name ## Bubblesort(List ## name * list, int (*predicate)(type *a, type *b)) {\
    if (list->len < 2) return;\
    unsigned long len = list->len - 1;\
    while (len --> 0) { \
        for (unsigned long i = 0; i < len; i++) {\
            if (predicate(&list->items[i], &list->items[i + 1]) > 0) {\
                type swap = list->items[i];\
                list->items[i] = list->items[i + 1];\
                list->items[i + 1] = swap;\
            }\
        }\
    }\
}\


#endif // ARRAY_H_
