#ifndef TYPES_H_
#define TYPES_H_

#include <raylib.h>

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long int usize;

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

#define makeOptional(type, name) typedef struct {\
        type value;\
        bool has_value;\
    } Optional ## name

#define makeOptionals(type, size, name) typedef struct {\
        type value[size];\
        bool has_value;\
        } Optionals ## name

makeOptional(usize, Usize);
makeOptional(float, Float);
makeOptional(Vector2, Vector2);
makeOptional(uint, Uint);

#define arrayLength(arr) (sizeof(arr) / sizeof(arr[0]))

typedef struct {
    uchar * start;
    usize   len;
} StringSlice;

StringSlice make_slice(uchar * from, usize start, usize end);

#endif // TYPES_H_
