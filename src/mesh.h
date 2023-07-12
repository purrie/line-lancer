#ifndef MESH_H_
#define MESH_H_

#include <raylib.h>
#include "types.h"

Model generate_building_mesh (const Vector2 pos, const float size, const float layer);
Model generate_line_mesh     (const ListLine lines, float thickness, ushort cap_resolution, const float layer);
Model generate_area_mesh     (Area *const area, const float layer);
void  generate_map_mesh      (Map * map);

#endif // MESH_H_
