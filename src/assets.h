#ifndef ASSETS_H_
#define ASSETS_H_

#include <raylib.h>
#include "level.h"

/* Loading *******************************************************************/
Result load_levels   (ListMap * maps);
Result load_graphics (Assets * assets);
Result load_settings (Settings * settings);
Result save_settings (const Settings * settings);

/* Asset Management **********************************************************/
void   assets_deinit (Assets * assets);
char * file_name_from_path (char * path, Alloc alloc);

/* Audio *********************************************************************/
Result load_music (Assets * assets);
Result load_sound_effects (Assets * assets);

#endif // ASSETS_H_
