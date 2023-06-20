#include "alloc.h"

#ifndef ALLOC_MEMORY_SIZE
#define ALLOC_MEMORY_SIZE 65536
#endif

char mem[ALLOC_MEMORY_SIZE];
unsigned long int cursor = 0;

void * temp_alloc (unsigned int size) {
    if (cursor + size > ALLOC_MEMORY_SIZE) return (void*)0;

    unsigned long int ptr = cursor;
    cursor += size;
    return &mem[ptr];
}

void temp_free () {
    cursor = 0;
}
