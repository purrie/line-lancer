#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <raylib.h>
#include <raymath.h>
#include "types.h"

typedef struct {
    Vector2 a;
    Vector2 b;
} Line;

makeList(Line, Line);

typedef enum {
    Empty = 0,
    Fighter,
    Archer,
    Support,
    Special,
    Resource,
} BuildingType;

typedef struct {
    Vector2 position;
    BuildingType type;
    Model model;
} Building;

typedef struct {
    Vector2 position;
    Model model;
} Castle;

makeList(Building, Building);

typedef struct {
    ListLine lines;
    Model model;
} Area;

typedef struct {
    Area          area;
    OptionalUsize player_owned;
    Castle        castle;
    ListBuilding  buildings;
} Region;

typedef struct {
    ListLine lines;
    Model model;
} Path;

makeList(Region, Region);
makeList(Path, Path);

typedef struct {
    usize      width;
    usize      height;
    uchar      player_count;
    ListRegion regions;
    ListPath   paths;
} Map;

makeOptional(Map, Map);

//* Line Functions ***********************************************************/
Line             make_line               (Vector2 a, Vector2 b);
OptionalVector2  get_line_point          (Line line, float t);
float            get_line_length         (Line line);
bool             get_line_intersects     (Line a, Line b);
OptionalVector2  get_line_intersection   (Line a, Line b);
bool             get_lines_intersections (const ListLine lines, const Line line, ListVector2 * result);
Rectangle        get_line_bounds         (const Line line);
Rectangle        get_lines_bounds        (const ListLine lines);
void             bevel_lines             (ListLine *lines, usize resolution, float depth, bool enclosed);

//* Path Functions ***********************************************************/

//* Area Functions ***********************************************************/
void             clamp_area              (Area *area, Rectangle rect);
void             area_flip_y             (Area *area, float height);
void             area_scale              (Area *area, Vector2 scalar);
void             area_move               (Area *area, Vector2 move_by);
ListLine         area_line_intersections (Area *const area, Line line);
bool             area_contains_point     (const Area *const area, const Vector2 point);
OptionalVector2  area_center             (Area *const area); // won't have value unless area is valid
Rectangle        area_bounds             (const Area *const area);

//* Region Functions *********************************************************/
void map_clamp(Map * map);
Camera2D setup_camera(Map * map);
void render_map(Map * map);
void render_map_mesh(Map * map);

#endif // GEOMETRY_H_
