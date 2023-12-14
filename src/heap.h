#include "alloc.h"
#include <stddef.h>

#ifndef HEAP_TYPE
#define HEAP_TYPE int
#error Define HEAP_TYPE. If its a pointer, you need to define HEAP_NAME too.
#endif

#ifndef HEAP_NAME
#define HEAP_NAME HEAP_TYPE
#endif

#ifndef NULL
#define NULL (void*)0
#endif

#define MACROS_ARE_TRASH(a, b) a ## b
#define MACROS_BAD(a, b, c) a ## b ## c
#define MACROS_SUCK(x, y) MACROS_ARE_TRASH(x, y)
#define MACROS_BOO(x, y, z) MACROS_BAD(x, y, z)

#define HEAP_TYPE_NAME MACROS_SUCK(Heap, HEAP_NAME)
#define heap_fun(name) MACROS_BOO(heap, HEAP_NAME, name)

#define HEAP_COMPARE MACROS_SUCK(HEAP_NAME, Compare)
#define HEAP_EQL MACROS_SUCK(HEAP_NAME, Equal)

#ifdef HEAP_DECLARATION

typedef int (*HEAP_COMPARE)(HEAP_TYPE a, HEAP_TYPE b);
typedef int (*HEAP_EQL)(HEAP_TYPE a, HEAP_TYPE b);

typedef struct {
    HEAP_TYPE * items;
    unsigned long cap;
    unsigned long len;
    Allocator mem;
    HEAP_COMPARE compare;
    HEAP_EQL equals;
} HEAP_TYPE_NAME;

int heap_fun(Init)(unsigned long cap, HEAP_TYPE_NAME * result, Allocator mem, HEAP_COMPARE compare, HEAP_EQL equal);
void heap_fun(Deinit)(HEAP_TYPE_NAME * heap);
int heap_fun(Append)(HEAP_TYPE_NAME * heap, HEAP_TYPE item);
int heap_fun(Pop)(HEAP_TYPE_NAME * heap, HEAP_TYPE * item);
int heap_fun(Find)(HEAP_TYPE_NAME * heap, HEAP_TYPE comparable, size_t * index_found, HEAP_TYPE * item_found);
int heap_fun(Update)(HEAP_TYPE_NAME * heap, unsigned long index, HEAP_TYPE item);

#undef HEAP_DECLARATION
#endif

#ifdef HEAP_IMPLEMENTATION

int heap_fun(Grow)(HEAP_TYPE_NAME * heap, unsigned long new_cap) {
    if (heap->cap >= new_cap)
        return 0;

    HEAP_TYPE * new;
    if (heap->items) {
        if (heap->mem.realloc == NULL)
            return 1;
        new = heap->mem.realloc(heap->items, sizeof(HEAP_TYPE) * new_cap);
        if (new == NULL) {
            if (heap->mem.alloc == NULL)
                return 1;
            new = heap->mem.alloc(sizeof(HEAP_TYPE) * new_cap);
        }
    }
    else {
        if (heap->mem.alloc == NULL)
            return 1;
        new = heap->mem.alloc(sizeof(HEAP_TYPE) * new_cap);
    }
    if (new == NULL)
        return 1;

    heap->items = new;
    heap->cap = new_cap;
    return 0;
}
int heap_fun(Init)(unsigned long cap, HEAP_TYPE_NAME * result, Allocator mem, HEAP_COMPARE compare, HEAP_EQL equal) {
    HEAP_TYPE_NAME heap = {0};
    heap.equals = equal;
    heap.compare = compare;
    heap.mem = mem;
    if (heap_fun(Grow)(&heap, cap)) {
        return 1;
    }
    *result = heap;
    return 0;
}
void heap_fun(Downgrade)(HEAP_TYPE_NAME * heap, unsigned long index) {
    unsigned long child_l = index * 2 + 1;
    unsigned long child_r = index * 2 + 2;
    if (child_l >= heap->len)
        return;

    unsigned long try_swap;
    if (child_r >= heap->len)
        try_swap = child_l;
    else {
        try_swap = heap->compare(heap->items[child_l], heap->items[child_r]) > 0 ? child_r : child_l;
    }

    if (heap->compare(heap->items[index], heap->items[try_swap]) > 0) {
        HEAP_TYPE swap = heap->items[index];
        heap->items[index] = heap->items[try_swap];
        heap->items[try_swap] = swap;
        heap_fun(Downgrade)(heap, try_swap);
    }
}
void heap_fun(Upgrade)(HEAP_TYPE_NAME * heap, unsigned long index) {
    if (index == 0)
        return;

    unsigned long parent = ( index - 1 ) / 2;
    if (heap->compare(heap->items[parent], heap->items[index]) > 0) {
        HEAP_TYPE swap = heap->items[parent];
        heap->items[parent] = heap->items[index];
        heap->items[index] = swap;
        heap_fun(Upgrade)(heap, parent);
    }
}

int heap_fun(Append)(HEAP_TYPE_NAME * heap, HEAP_TYPE item) {
    if (heap == NULL) {
        return 1;
    }
    if (heap->items == NULL) {
        if (heap_fun(Grow)(heap, 6)) {
            return 1;
        }
    }
    if (heap->len >= heap->cap) {
        if (heap_fun(Grow)(heap, heap->len + 6)) {
            return 1;
        }
    }
    heap->items[heap->len] = item;
    heap->len ++;
    heap_fun(Upgrade)(heap, heap->len - 1);
    return 0;
}
int heap_fun(Pop)(HEAP_TYPE_NAME * heap, HEAP_TYPE * item) {
    if (heap->len == 0)
        return 1;
    *item = heap->items[0];
    heap->items[0] = heap->items[heap->len - 1];
    heap->len --;
    heap_fun(Downgrade)(heap, 0);
    return 0;
}

int heap_fun(Find)(HEAP_TYPE_NAME * heap, HEAP_TYPE comparable, size_t * index_found, HEAP_TYPE * item_found) {
    for (unsigned long i = 0; i < heap->len; i++) {
        if (heap->equals(heap->items[i], comparable)) {
            if (index_found)
                *index_found = i;
            if (item_found)
                *item_found = heap->items[i];
            return 0;
        }
    }
    return 1;
}
int heap_fun(Update)(HEAP_TYPE_NAME * heap, unsigned long index, HEAP_TYPE item) {
    int compare = heap->compare(heap->items[index], item);
    if (compare > 0) {
        heap->items[index] = item;
        heap_fun(Downgrade)(heap, index);
        return 0;
    }
    else if (compare < 0) {
        heap->items[index] = item;
        heap_fun(Upgrade)(heap, index);
        return 0;
    }
    return 1;
}

#undef HEAP_IMPLEMENTATION
#endif

#undef HEAP_TYPE
#undef HEAP_NAME
#undef HEAP_TYPE_NAME
#undef HEAP_COMPARE
#undef HEAP_EQL
#undef heap_fun

#undef MACROS_ARE_TRASH
#undef MACROS_SUCK
#undef MACROS_BAD
#undef MACROS_BOO
