#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <raylib.h>
#include "types.h"

/* Line Functions ***********************************************************/
Test      line_intersects     (Line a, Line b);
Result    line_intersection   (Line a, Line b, Vector2 * out_result);
usize     lines_intersections (const ListLine lines, const Line line, ListVector2 * result);
Rectangle line_bounds         (const Line line);
Result    lines_bounds        (ListLine *const lines, Rectangle * result);
Test      lines_check_hit     (ListLine *const lines, Vector2 point, float distance);
void      bevel_lines         (ListLine *lines, usize resolution, float depth, bool enclosed);

/* Area Functions ***********************************************************/
void      clamp_area           (Area *area, Rectangle rect);
Test      area_line_intersects (Area *const area, Line line);
bool      area_contains_point  (const Area *const area, const Vector2 point);
Rectangle area_bounds          (const Area *const area);

/* Building Functions ******************************************************/
void   place_building            (Building * building, BuildingType type);
void   upgrade_building          (Building * building);
void   demolish_building         (Building * building);
usize  building_buy_cost         (BuildingType type);
usize  building_upgrade_cost     (Building *const building);
usize  building_upgrade_cost_raw (BuildingType type, usize level);
usize  building_cost_to_spawn    (Building *const building);
usize  building_generated_income (Building *const building);
float  building_trigger_interval (Building *const building);

float       building_size            ();
Rectangle   building_bounds          (Building *const building);
Building  * get_building_by_position (Map * map, Vector2 position);

/* Path Functions ************************************************************/
Result path_by_position (Map *const map, Vector2 position, Path ** result);

/* Region Functions ********************************************************/
void     region_change_ownership (GameState * state, Region * region, usize player_id);
Region * region_by_unit          (Unit *const guardian);
Result   region_by_position      (Map *const map, Vector2 position, Region ** result);

/* Map Functions *********************************************************/
void     map_clamp           (Map * map);
Result   map_clone           (Map * dst, Map *const src);
Result   map_prepare_to_play (Map * map);
void     map_deinit          (Map * map);
void     render_map          (Map * map);
void     render_map_mesh     (Map * map);
Region * map_get_region_at   (Map *const map, Vector2 point);

float get_expected_income           (Map *const map, usize player);
float get_expected_maintenance_cost (Map *const map, usize player);

#endif // GEOMETRY_H_
