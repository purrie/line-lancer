#include "std.h"

void * copy_memory_backward(void * dest, void * source, usize bytes) {
    uchar * d = (uchar *) dest;
    uchar * s = (uchar *) source;
    while (bytes --> 0) {
        d[bytes] = s[bytes];
    }
    return dest;
}

void * copy_memory(void * dest, void * source, usize bytes) {
    uchar * d = (uchar *) dest;
    uchar * s = (uchar *) source;
    for (usize i = 0; i < bytes; i++) {
        d[i] = s[i];
    }
    return dest;
}
void * set_memory(void * ptr, uchar val, usize bytes) {
    uchar * mem = (uchar *) ptr;
    while (bytes --> 0) {
        mem[bytes] = val;
    }
    return ptr;
}

void * clear_memory(void * ptr, usize bytes) {
    uchar * mem = (uchar *) ptr;
    while (bytes --> 0) {
        mem[bytes] = 0;
    }
    return ptr;
}

bool compare_literal(StringSlice slice, char *const literal) {
    usize i = 0;
    for(; i < slice.len; i++) {
        if (literal[i] == '\0') {
            return false;
        }
        if (slice.start[i] != literal[i]) {
            return false;
        }
    }
    return literal[i] == '\0';
}

Result convert_slice_usize(StringSlice slice, usize * value) {
    bool first = true;
    for (usize i = 0; i < slice.len; i++) {
        if (slice.start[i] >= '0' && slice.start[i] <= '9') {
            if (first == false) {
                *value *= 10;
                *value += slice.start[i] - '0';
            } else {
                *value = slice.start[i] - '0';
                first = false;
            }
        } else {
            return FAILURE;
        }
    }
    return SUCCESS;
}

Result convert_slice_float(StringSlice slice, float * value) {
    int dot = -1;
    bool neg = false;
    bool first = true;
    for (usize i = 0; i < slice.len; i++) {
        uchar c = slice.start[i];
        if (c >= '0' && c <= '9') {
            float digit = (float)(c - '0');
            if (dot >= 0) {
                usize dot_spot = i - dot;
                float scalar = 1.0f;
                for (usize p = 0; p < dot_spot; p++) {
                    scalar *= 10.0f;
                }
                *value += digit / scalar;
            }
            else if (first == false) {
                *value *= 10.0f;
                *value += digit;
            } 
            else {
                *value = digit;
            }
            first = false;
        }
        else if (c == '.') {
            if (dot >= 0) {
                return FAILURE;
            } else {
                dot = i;
            }
        }
        else if (c == '-' && i == 0) {
            neg = true; 
        }
        else {
            return FAILURE;
        }
    }
    if (neg) { *value *= -1.0f; }
    return SUCCESS;
}

char * convert_int_to_ascii (int number, Allocator alloc) {
    char buffer[64];
    char count = 0;
    int test = number;
    char negative = number < 0;
    if (negative) {
        count ++;
        buffer[0] = '-';
        test = -test;
    }

    while (test != 0) {
        char num = test % 10;
        buffer[count++] = '0' + num;
        test /= 10;
    }

    if (count == 0) {
        buffer[0] = '0';
        count = 1;
    }

    {
        char total = count;
        char length = (total - negative) / 2;
        char last = negative ? 0 : 1;

        for (char start = negative; start < length; start++) {
            char other = total - start - last;
            char temp = buffer[start];
            buffer[start] = buffer[other];
            buffer[other] = temp;
        }
    }

    char * result = alloc(sizeof(char) * (count + 1));
    copy_memory(result, buffer, count);
    result[count] = '\0';
    return result;
}

void log_slice(TraceLogLevel log_level, char * text, StringSlice slice) {
    char s[slice.len + 1];
    copy_memory(s, slice.start, sizeof(char) * slice.len);
    s[slice.len] = '\0';
    TraceLog(log_level, "%s %s", text, s);
}
