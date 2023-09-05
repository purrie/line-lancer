#include "alloc.h"
#include <raylib.h>

#ifndef MAX_ARENA_SIZE
#define MAX_ARENA_SIZE 32 * 1024
#endif

typedef struct Arena Arena;

struct Arena {
    unsigned char mem[MAX_ARENA_SIZE];
    unsigned long cursor;
    void * last_alloc;
    Arena * next;
};

Arena global_arena = {0};

int max_arena_depth = 10;

Allocator temp_allocator () {
    return (Allocator){
        .alloc = &temp_alloc,
        .free = &temp_free,
        .realloc = &temp_realloc,
    };
}
Allocator perm_allocator () {
    return (Allocator) {
        .alloc = &MemAlloc,
        .free = &MemFree,
        .realloc = &MemRealloc,
    };
}
void * temp_alloc (unsigned int size) {
    Arena * arena = &global_arena;
    int depth = 0;
    while (arena->cursor + size > MAX_ARENA_SIZE) {
        if (depth ++> max_arena_depth) {
            return (void*)0;
        }
        if (arena->next) {
            arena = arena->next;
        }
        else {
            arena->next = MemAlloc(sizeof(Arena));
            arena = arena->next;
            arena->cursor = 0;
            arena->next = (void*)0;
        }
    }

    unsigned long ptr = arena->cursor;
    arena->cursor += size;
    arena->last_alloc = &arena->mem[ptr];
    return arena->last_alloc;
}
void * temp_realloc ( void * ptr, unsigned int new_size ) {
    if (ptr == (void*)0) {
        return (void*)0;
    }
    Arena * arena = &global_arena;
    while (arena) {
        if (arena->last_alloc == ptr) {
            unsigned long position = (unsigned long) ptr;
            position -= (unsigned long) &arena->mem[0];
            if (position + new_size >= MAX_ARENA_SIZE) {
                return (void*)0;
            }
            arena->cursor = position + new_size;
            return ptr;
        }
        arena = arena->next;
    }
    return (void*)0;
}
void temp_free(void * ptr) {
    Arena * arena = &global_arena;
    while (arena) {
        if (arena->last_alloc == ptr) {
            unsigned long position = (unsigned long) ptr;
            position -= (unsigned long) &arena->mem[0];
            arena->cursor = position;
            return;
        }
        arena = arena->next;
    }
}
void temp_reset () {
    Arena * arena = &global_arena;
    do {
        arena->cursor = 0;
        arena = arena->next;
    } while (arena);
}
