#include "types.h"
#include "std.h"

implementList(ushort, Ushort)
implementList(usize, Usize)
implementList(Vector2, Vector2)
implementList(Unit*, Unit)
implementList(Region, Region)
implementList(Path, Path)
implementList(PathEntry, PathEntry)
implementList(Building, Building)
implementList(Line, Line)
implementList(Bridge, Bridge)

StringSlice make_slice(uchar * from, usize start, usize end) {
    StringSlice s;
    s.start = from + start;
    s.len = end - start;
    return s;
}
