#include "alloc.h"
#include <raylib.h>
#include <stddef.h>

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
    #ifdef DEBUG
    if (size > MAX_ARENA_SIZE) {
        TraceLog(LOG_FATAL, "Attempted to allocate more memory than arena is configured to store at a time. MAX = %zu, REQUESTED = %zu", MAX_ARENA_SIZE, size);
        return (void*)0;
    }
    #endif
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
            size_t position = (size_t) ptr;
            position -= (size_t) &arena->mem[0];
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
            size_t position = (size_t) ptr;
            position -= (size_t) &arena->mem[0];
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
