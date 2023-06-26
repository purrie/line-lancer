#include "alloc.h"

#ifndef ALLOC_MEMORY_SIZE
#define ALLOC_MEMORY_SIZE 16777216
#endif

static char mem[ALLOC_MEMORY_SIZE];
static unsigned long long int cursor = 0;

void * temp_alloc (unsigned int size) {
    if (cursor + size > ALLOC_MEMORY_SIZE) return (void*)0;

    unsigned long long int ptr = cursor;
    cursor += size;
    return &mem[ptr];
}

void temp_free () {
    cursor = 0;
}
