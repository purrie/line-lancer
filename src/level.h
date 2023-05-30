#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <raylib.h>
#include "array.h"
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
} Building;

typedef struct {
    Vector2 position;
} Castle;

makeList(Building, Building);

typedef struct {
    ListLine lines;
} Area;

typedef struct {
    Area          area;
    OptionalUsize player_owned;
    Castle        castle;
    ListBuilding  buildings;
} Region;

typedef struct {
    ListLine lines;
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
Line             make_line             (Vector2 a, Vector2 b);
OptionalVector2  get_line_point        (Line line, float t);
float            get_line_length       (Line line);
bool             get_line_intersects   (Line a, Line b);
OptionalVector2  get_line_intersection (Line a, Line b);
Rectangle        get_line_bounds       (Line line);
ListLine         bevel_lines           (Line *lines, usize len, usize resolution, float depth);

//* Path Functions ***********************************************************/

//* Area Functions ***********************************************************/
void             clamp_area              (Area *area, Rectangle rect);
void             area_flip_y             (Area *area, float height);
void             area_scale              (Area *area, Vector2 scalar);
void             area_move               (Area *area, Vector2 move_by);
ListLine         area_line_intersections (Area *const area, Line line);
bool             area_contains_point     (Area *const area, Vector2 point);
OptionalVector2  area_center             (Area *const area); // won't have value unless area is valid
Mesh             area_to_mesh            (Area *const area);
Rectangle        area_bounds             (Area *const area);

//* Region Functions *********************************************************/
void map_deinit(Map * map);
void map_resize(Map * map, usize size_x, usize size_y);
void map_transl(Map * map, int x, int y);

#endif // GEOMETRY_H_
