#ifndef STD_H_
#define STD_H_

#include <raylib.h>
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

bool compare_literal(StringSlice slice, char *const literal);

OptionalUsize convert_slice_usize(StringSlice slice);
OptionalFloat convert_slice_float(StringSlice slice);

void log_slice(TraceLogLevel log_level, char * text, StringSlice slice);

#endif // STD_H_
