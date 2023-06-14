#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <raylib.h>
#include "types.h"

/* Line Functions ***********************************************************/
Line            make_line               (Vector2 a, Vector2 b);
float           get_line_length         (Line line);
bool            get_line_intersects     (Line a, Line b);
Result          get_line_intersection   (Line a, Line b, Vector2 * out_result);
usize           get_lines_intersections (const ListLine lines, const Line line, ListVector2 * result);
Rectangle       get_line_bounds         (const Line line);
Rectangle       get_lines_bounds        (const ListLine lines);
void            bevel_lines             (ListLine *lines, usize resolution, float depth, bool enclosed);

/* Path Functions ***********************************************************/
Vector2           path_start_point (Path *const path, Region *const from);
Node            * path_start_node  (Path *const path, Region *const from, Movement * out_direction_forward);
Result            path_follow      (Path *const path, Region *const from, float distance, Vector2 * out_result);
Region          * path_end_region  (Path *const path, Region *const from);
float             path_length      (Path *const path);

/* Area Functions ***********************************************************/
void            clamp_area              (Area *area, Rectangle rect);
void            area_flip_y             (Area *area, float height);
void            area_scale              (Area *area, Vector2 scalar);
void            area_move               (Area *area, Vector2 move_by);
ListLine        area_line_intersections (Area *const area, Line line);
bool            area_contains_point     (const Area *const area, const Vector2 point);
Rectangle       area_bounds             (const Area *const area);

/* Building Functions ******************************************************/
float       building_size            ();
Rectangle   building_bounds          (Building *const building);
Building  * get_building_by_position (Map * map, Vector2 position);

/* Region Functions ********************************************************/

/* Map Functions *********************************************************/
void     set_cursor_to_camera_scale (Camera2D *const cam);
void     map_clamp                  (Map * map);
Camera2D setup_camera               (Map * map);
void     render_map                 (Map * map);
void     render_map_mesh            (Map * map);

#endif // GEOMETRY_H_
