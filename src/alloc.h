#ifndef ALLOC_H_
#define ALLOC_H_

typedef void * ( * Alloc ) ( unsigned int size );
typedef void ( * Free ) ( void * ptr );
typedef void * ( * Realloc ) ( void * ptr, unsigned int new_size );

typedef struct {
    Alloc   alloc;
    Free    free;
    Realloc realloc;
} Allocator;

Allocator temp_allocator ();
Allocator perm_allocator ();

void * temp_alloc ( unsigned int size );
void * temp_realloc ( void * ptr, unsigned int new_size );
void   temp_free ( void * mem );
void   temp_reset ();

#endif // ALLOC_H_
