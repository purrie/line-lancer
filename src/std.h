#ifndef STD_H_
#define STD_H_

#include "types.h"

/// Copies memory from source to dest starting from end of the array
/// This is used if you need to move memory backward,
/// as in making space to insert something in middle of a list
void * copy_memory_backward(void * dest, void * source, usize bytes);

/// Copies memory from source to dest starting from the beginning
void * copy_memory(void * dest, void * source, usize bytes);

/// Fills memory with specitied byte value
void * set_memory(void * ptr, uchar val, usize bytes);

/// Zero initializes memory space
void * clear_memory(void * ptr, usize bytes);

char * convert_int_to_ascii (int number, Alloc alloc);
char * convert_float_to_ascii (float number, unsigned int dot_count, Alloc alloc);

usize string_length (char * string);

#endif // STD_H_
