#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <raylib.h>
#include "types.h"

/* Building Functions ********************************************************/
void place_building    (Building * building, BuildingType type);
void upgrade_building  (Building * building);
void demolish_building (Building * building);

/* Line Functions ***********************************************************/
Line      make_line           (Vector2 a, Vector2 b);
Test      line_intersects     (Line a, Line b);
Result    line_intersection   (Line a, Line b, Vector2 * out_result);
usize     lines_intersections (const ListLine lines, const Line line, ListVector2 * result);
Rectangle line_bounds         (const Line line);
Rectangle lines_bounds        (const ListLine lines);
void      bevel_lines         (ListLine *lines, usize resolution, float depth, bool enclosed);

/* Path Functions ***********************************************************/
Path    * path_on_point    (Map *const map, Vector2 point, Movement * out_direction);
Vector2   path_start_point (Path *const path, Region *const from);
Node    * path_start_node  (Path *const path, Region *const from, Movement * out_direction_forward);
Result    path_follow      (Path *const path, Region *const from, float distance, Vector2 * out_result);
Region  * path_end_region  (Path *const path, Region *const from);
float     path_length      (Path *const path);

/* Area Functions ***********************************************************/
void      clamp_area              (Area *area, Rectangle rect);
void      area_flip_y             (Area *area, float height);
void      area_scale              (Area *area, Vector2 scalar);
void      area_move               (Area *area, Vector2 move_by);
ListLine  area_line_intersections (Area *const area, Line line);
Test      area_line_intersects    (Area *const area, Line line);
bool      area_contains_point     (const Area *const area, const Vector2 point);
Rectangle area_bounds             (const Area *const area);

/* Building Functions ******************************************************/
float       building_size            ();
Rectangle   building_bounds          (Building *const building);
Building  * get_building_by_position (Map * map, Vector2 position);

/* Region Functions ********************************************************/
void     region_change_ownership (Region * region, usize player_id);
void     region_update_paths     (Region * region);
Region * region_by_guardian      (ListRegion *const regions, Unit *const guardian);

/* Map Functions *********************************************************/
void     map_clamp                  (Map * map);
void     render_map                 (Map * map);
void     render_map_mesh            (Map * map);

#endif // GEOMETRY_H_
