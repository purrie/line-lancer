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

OptionalUsize convert_slice_usize(StringSlice slice) {
    OptionalUsize ret = {0};
    for (usize i = 0; i < slice.len; i++) {
        if (slice.start[i] >= '0' && slice.start[i] <= '9') {
            if (ret.has_value) {
                ret.value = ret.value * 10 + (slice.start[i] - '0');
            } else {
                ret.value = slice.start[i] - '0';
                ret.has_value = true;
            }
        } else {
            ret.has_value = false;
            break;
        }
    }
    return ret;
}

OptionalFloat convert_slice_float(StringSlice slice) {
    OptionalFloat ret = {0};
    int dot = -1;
    bool neg = false;
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
                ret.value += digit / scalar;
            }
            else if (ret.has_value) {
                ret.value = ret.value * 10.0f + digit;
            } 
            else {
                ret.value = digit;
            }
            ret.has_value = true;
        } 
        else if (c == '.') {
            if (dot >= 0) {
                ret.has_value = false;
                break;
            } else {
                dot = i;
            }
        }
        else if (c == '-' && i == 0) {
            neg = true; 
        }
        else {
            ret.has_value = false;
            break;
        }
    }
    if (ret.has_value && neg) { ret.value *= -1.0f; }
    return ret;
}

void log_slice(TraceLogLevel log_level, char * text, StringSlice slice) {
    char s[slice.len + 1];
    copy_memory(s, slice.start, sizeof(char) * slice.len);
    s[slice.len] = '\0';
    TraceLog(log_level, "%s %s", text, s);
}
