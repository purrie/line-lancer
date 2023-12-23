#include "std.h"

void * copy_memory_backward(void * dest, void * source, usize bytes) {
    uchar * d = (uchar *) dest;
    uchar * s = (uchar *) source;
    while (bytes --> 0) {
        d[bytes] = s[bytes];
    }
    return dest;
}

void * copy_memory(void * dest, const void * source, usize bytes) {
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

char * convert_int_to_ascii (int number, Alloc alloc) {
    char buffer[64];
    usize count = 0;
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
        usize length = (total - negative) / 2;
        char last = negative ? 0 : 1;

        for (usize start = negative; start < length; start++) {
            usize other = total - start - last;
            char temp = buffer[start];
            buffer[start] = buffer[other];
            buffer[other] = temp;
        }
    }

    char * result = alloc(sizeof(char) * (count + 1));
    if (result == NULL) {
        return NULL;
    }
    copy_memory(result, buffer, count);
    result[count] = '\0';
    return result;
}
char * convert_float_to_ascii (float number, unsigned int dot_count, Alloc alloc) {
    char buffer[256] = {0};

    usize count = 0;
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
        usize length = (total - negative) / 2;
        char last = negative ? 0 : 1;

        for (usize start = negative; start < length; start++) {
            usize other = total - start - last;
            char temp = buffer[start];
            buffer[start] = buffer[other];
            buffer[other] = temp;
        }
    }

    if (dot_count > 0){
        buffer[count++] = '.';
        float after_dot = number;
        while (dot_count --> 0) {
            after_dot *= 10.0f;
            int rounded = (int)after_dot;
            char num = (char)( rounded % 10 );
            buffer[count++] = '0' + num;
        }
    }

    char * result = alloc(sizeof(char) * (count + 1));
    if (result == NULL) {
        return NULL;
    }
    copy_memory(result, buffer, count);
    result[count] = '\0';
    return result;
}

usize string_length (const char * string) {
    usize guard = 2048;
    usize len = 0;
    while (--guard && string[len] != '\0') {
        len ++;
    }
    if (guard == 0) {
        return 0;
    }

    return len;
}

float get_time () {
    static float start_time = 0.0f;
    static int started = 0;
    if (started) {
        return GetTime() - start_time;
    }
    start_time = GetTime();
    started = 1;
    return start_time;
}
