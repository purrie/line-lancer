#ifndef BRIDGE_H_
#define BRIDGE_H_

#include "level.h"
#include "units.h"

Bridge  bridge_from_path         (Path *const path);
Bridge  bridge_building_and_path (Path *const path, Building *const building);
Bridge  bridge_castle_and_path   (Path *const path, Castle *const castle);
void    clean_up_bridge          (Bridge * b);

#endif // BRIDGE_H_
