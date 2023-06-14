#ifndef ASSETS_H_
#define ASSETS_H_

#include <raylib.h>
#include "level.h"

Result load_level(char * path, Map * out_result);
void level_unload(Map * map);

#endif // ASSETS_H_
