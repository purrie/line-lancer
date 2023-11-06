#ifndef MESH_H_
#define MESH_H_

#include <raylib.h>
#include "types.h"

Model generate_line_mesh     (const ListLine lines, float thickness, ushort cap_resolution, const float layer);
Model generate_area_mesh     (const Area * area, const float layer);
void  generate_map_mesh      (Map * map);

#endif // MESH_H_
