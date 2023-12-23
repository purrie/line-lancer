#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <raylib.h>
#include "types.h"

/* Line Functions ***********************************************************/
Test      line_intersects     (Line a, Line b);
Result    line_intersection   (Line a, Line b, Vector2 * out_result);
usize     lines_intersections (const ListLine lines, const Line line, ListVector2 * result);
Rectangle line_bounds         (const Line line);
Result    lines_bounds        (const ListLine * lines, Rectangle * result);
Test      lines_check_hit     (const ListLine * lines, Vector2 point, float distance);
void      bevel_lines         (ListLine *lines, usize resolution, float depth, bool enclosed);

/* Area Functions ***********************************************************/
void      clamp_area           (Area *area, Rectangle rect);
Test      area_line_intersects (const Area * area, Line line);
bool      area_contains_point  (const Area * area, const Vector2 point);
Rectangle area_bounds          (const Area * area);

/* Building Functions ******************************************************/
void      place_building            (Building * building, BuildingType type);
void      upgrade_building          (Building * building);
void      demolish_building         (Building * building);
usize     building_buy_cost         (BuildingType type);
usize     building_upgrade_cost     (const Building * building);
usize     building_upgrade_cost_raw (BuildingType type, usize level);
usize     building_cost_to_spawn    (const Building * building);
usize     building_generated_income (const Building * building);
float     building_trigger_interval (const Building * building);
usize     building_max_units        (const Building * building);
Texture2D building_image            (const Assets * assets, FactionType faction, BuildingType building, usize level);

Building   * get_building_by_position (const Map * map, Vector2 position, float range);
const char * building_name            (BuildingType building, FactionType faction, usize upgrade);

/* Path Functions ************************************************************/
Result path_by_position (const Map * map, Vector2 position, Path ** result);

/* Region Functions ********************************************************/
void     region_reset_unit_pathfinding (Region * region);
void     region_change_ownership       (GameState * state, Region * region, usize player_id);
Region * region_by_unit                (const Unit * guardian);
Result   region_by_position            (const Map * map, Vector2 position, Region ** result);

/* Map Functions *********************************************************/
void     map_clamp           (Map * map);
Result   map_clone           (Map * dst, const Map * src);
Result   map_prepare_to_play (const Assets * assets, Map * map);
void     map_deinit          (Map * map);
void     render_map          (Map * map);
void     render_map_mesh     (const GameState * state);
Region * map_get_region_at   (const Map * map, Vector2 point);

float get_expected_income           (const Map * map, usize player);
float get_expected_maintenance_cost (const Map * map, usize player);

#endif // GEOMETRY_H_
