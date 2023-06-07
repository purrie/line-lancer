#ifndef ARRAY_H_
#define ARRAY_H_

#define makeList(type, name) typedef struct {\
        type * items;\
        usize len;\
        usize cap;\
        Allocator alloc;\
        Deallocator dealloc;\
    } List ## name;\
    List ## name list ## name ## Init(usize cap, Allocator alloc, Deallocator dealloc);\
    void list ## name ## Deinit(List ## name * list);\
    int list ## name ## Grow(List ## name * list, usize new_cap);\
    int list ## name ## Append(List ## name * list, type item);\
    void list ## name ## Remove(List ## name * list, usize index)\


#define implementList(type, name) \
List ## name list ## name ## Init(usize cap, Allocator alloc, Deallocator dealloc) {\
    List ## name r = {0};\
    r.cap = cap;\
    r.alloc = alloc;\
    r.dealloc = dealloc;\
    list ## name ## Grow(&r, cap);\
    return r;\
}\
\
void list ## name ## Deinit(List ## name * list) {\
    if (list->items != NULL) {\
        list->dealloc(list->items);\
        list->items = NULL;\
        list->cap = 0;\
        list->len = 0;\
    }\
}\
\
int list ## name ## Grow(List ## name * list, usize new_cap) {\
    if (new_cap < list->cap) {\
        return 1;\
    }\
    type * new_list = ( type * ) list->alloc(sizeof(type) * new_cap);\
    if (new_list == NULL) {\
        return 1;\
    }\
    if (list->items != NULL) {\
        copy_memory(new_list, list->items, sizeof(type) * list->len);\
        list->dealloc(list->items);\
        list->items = new_list;\
    } else {\
        list->items = new_list;\
    }\
    return 0;\
}\
\
int list ## name ## Append(List ## name * list, type item) {\
    if (list == NULL) {\
        return 1;\
    }\
    if (list->len == list->cap) {\
        if (list ## name ## Grow(list, list->cap + 10)) {\
            return 1;\
        }\
    }\
    list->items[list->len] = item;\
    list->len ++;\
    return 0;\
}\
\
void list ## name ## Remove(List ## name * list, usize index) {\
    if (index >= list->len) {\
        return;\
    }\
\
    list->len --;\
    if (index < list->len)\
        copy_memory(list->items + index, list->items + index + 1, sizeof(type) * (list->len - index));\
    return;\
}\

#endif // ARRAY_H_
