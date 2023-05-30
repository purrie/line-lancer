#include "types.h"

StringSlice make_slice(uchar * from, usize start, usize end) {
    StringSlice s;
    s.start = from + start;
    s.len = end - start;
    return s;
}
