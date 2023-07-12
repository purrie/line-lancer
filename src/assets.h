#ifndef ASSETS_H_
#define ASSETS_H_

#include <raylib.h>
#include "level.h"

Result  load_level   (Map * map, char * path);
Result  finalize_map (Map * map);
void    assets_deinit (GameAssets * assets);

char * map_name_from_path (char * path, Allocator alloc);

#endif // ASSETS_H_
