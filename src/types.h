#ifndef TYPES_H_
#define TYPES_H_

#include <raylib.h>
#include "optional.h"
#include "array.h"

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long int usize;
typedef long int size;
typedef unsigned short ushort;

typedef void * ( * Allocator )(uint size);
typedef void ( * Deallocator )(void * ptr);

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef bool
#define bool int
#endif

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

makeOptional(float, Float);
makeOptional(uint, Uint);

makeList(Vector2, Vector2);
makeList(ushort, Ushort);
makeList(usize, Usize);

typedef struct {
    uchar * start;
    usize   len;
} StringSlice;

StringSlice make_slice(uchar * from, usize start, usize end);

#endif // TYPES_H_
