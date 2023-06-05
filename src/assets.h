#ifndef ASSETS_H_
#define ASSETS_H_

#include <raylib.h>
#include "level.h"

OptionalMap load_level(char * path);
void level_unload(Map * map);

#endif // ASSETS_H_
