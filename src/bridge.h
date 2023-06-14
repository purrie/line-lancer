#ifndef BRIDGE_H_
#define BRIDGE_H_

#include "level.h"
#include "units.h"

Result bridge_region    (Region * region);
Result bridge_over_path (Path   * path);
void   clean_up_bridge  (Bridge * b);
void   render_bridge    (Bridge *const b, Color mid, Color start, Color end);

#endif // BRIDGE_H_
