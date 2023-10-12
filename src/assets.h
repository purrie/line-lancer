#ifndef ASSETS_H_
#define ASSETS_H_

#include <raylib.h>
#include "level.h"

/* Loading *******************************************************************/
Result load_levels (ListMap * maps);
Result load_graphics (Assets * assets);

/* Asset Management **********************************************************/
void   assets_deinit (Assets * assets);
char * file_name_from_path (char * path, Alloc alloc);

#endif // ASSETS_H_
