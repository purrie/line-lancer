#ifndef BRIDGE_H_
#define BRIDGE_H_

#include "level.h"
#include "units.h"

bool   bridge_is_enemy_present (Bridge *const bridge, usize player);

Result bridge_region    (Region * region);
Result bridge_over_path (Path   * path);
void   bridge_deinit    (Bridge * b);
void   render_bridge    (Bridge *const b, Color mid, Color start, Color end);

#endif // BRIDGE_H_
