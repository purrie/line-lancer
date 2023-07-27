#include "alloc.h"
#include <raylib.h>

#ifndef MAX_ARENA_SIZE
#define MAX_ARENA_SIZE 32 * 1024
#endif

typedef struct Arena Arena;

struct Arena {
    unsigned char mem[MAX_ARENA_SIZE];
    unsigned long cursor;
    Arena * next;
};

Arena global_arena = {0};


void * temp_alloc (unsigned int size) {
    Arena * arena = &global_arena;
    while (arena->cursor + size > MAX_ARENA_SIZE) {
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

    unsigned long int ptr = arena->cursor;
    arena->cursor += size;
    return &arena->mem[ptr];
}

void temp_free () {
    Arena * arena = &global_arena;
    do {
        arena->cursor = 0;
        arena = arena->next;
    } while (arena);
}
