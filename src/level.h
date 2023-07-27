#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <raylib.h>
#include "types.h"

/* Building Functions ********************************************************/
void   place_building            (Building * building, BuildingType type);
void   upgrade_building          (Building * building);
void   demolish_building         (Building * building);
usize  building_buy_cost         (BuildingType type);
usize  building_upgrade_cost     (Building *const building);
usize  building_upgrade_cost_raw (BuildingType type, usize level);
usize  building_cost_to_spawn    (Building *const building);
usize  building_generated_income (Building *const building);
float  building_trigger_interval (Building *const building);

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
Result      building_set_spawn_path  (Building * building, Path *const path);
char      * building_type_to_string  (BuildingType type);

/* Region Functions ********************************************************/
void        region_change_ownership       (GameState * state, Region * region, usize player_id);
void        region_update_paths           (Region * region);
Result      region_connect_paths          (Region * region, Path * from, Path * to);
Region    * region_by_guardian            (ListRegion *const regions, Unit *const guardian);
PathEntry * region_path_entry             (Region *const region, Path *const path);
PathEntry * region_path_entry_from_bridge (Region *const region, Bridge *const bridge);

/* Map Functions *********************************************************/
void     map_clamp           (Map * map);
Result   map_clone           (Map * dst, Map *const src);
Result   map_prepare_to_play (Map * map);
void     map_deinit          (Map * map);
void     render_map          (Map * map);
void     render_map_mesh     (Map * map);
Region * map_get_region_at   (Map *const map, Vector2 point);
size     get_expected_income (Map *const map, usize player);

#endif // GEOMETRY_H_
