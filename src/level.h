#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <raylib.h>
#include <raymath.h>
#include "types.h"

typedef struct Line Line;
typedef struct Path Path;
typedef struct PathEntry PathEntry;
typedef struct Building Building;
typedef struct Castle Castle;
typedef struct Region Region;
typedef struct Map Map;
typedef struct Area Area;

struct Line {
    Vector2 a;
    Vector2 b;
};

makeList(Line, Line);

struct Path {
    ListLine lines;
    Model model;
    Region * region_a;
    Region * region_b;
};

makeList(Path, Path);

struct PathEntry {
    Path * path;
    Path * redirect;
};

makeList(PathEntry, PathEntry);

typedef enum {
    BUILDING_EMPTY = 0,
    BUILDING_FIGHTER,
    BUILDING_ARCHER,
    BUILDING_SUPPORT,
    BUILDING_SPECIAL,
    BUILDING_RESOURCE,
    BUILDING_TYPE_COUNT = BUILDING_RESOURCE,
} BuildingType;

struct Building {
    Vector2        position;
    BuildingType   type;
    Model          model;
    ushort         upgrades;
    float          spawn_timer;
    Path         * spawn_target;
    Region       * region;
};

struct Castle {
    Vector2 position;
    Model   model;
};

makeList(Building, Building);

struct Area {
    ListLine lines;
    Model    model;
};

struct Region {
    Area          area;
    OptionalUsize player_owned;
    Castle        castle;
    ListBuilding  buildings;
    ListPathEntry paths;
};

makeList(Region, Region);

struct Map {
    usize      width;
    usize      height;
    uchar      player_count;
    ListRegion regions;
    ListPath   paths;
};

makeOptional(Map, Map);


/* Line Functions ***********************************************************/
Line            make_line               (Vector2 a, Vector2 b);
OptionalVector2 get_line_point          (Line line, float t);
float           get_line_length         (Line line);
bool            get_line_intersects     (Line a, Line b);
OptionalVector2 get_line_intersection   (Line a, Line b);
bool            get_lines_intersections (const ListLine lines, const Line line, ListVector2 * result);
Rectangle       get_line_bounds         (const Line line);
Rectangle       get_lines_bounds        (const ListLine lines);
void            bevel_lines             (ListLine *lines, usize resolution, float depth, bool enclosed);

/* Path Functions ***********************************************************/
Vector2           path_start_point (Path * path, Region * from);
OptionalVector2   path_follow      (Path * path, Region * from, float distance);
Region          * path_end_region  (Path * path, Region * from);
float             path_length      (Path * path);

/* Area Functions ***********************************************************/
void            clamp_area              (Area *area, Rectangle rect);
void            area_flip_y             (Area *area, float height);
void            area_scale              (Area *area, Vector2 scalar);
void            area_move               (Area *area, Vector2 move_by);
ListLine        area_line_intersections (Area *const area, Line line);
bool            area_contains_point     (const Area *const area, const Vector2 point);
OptionalVector2 area_center             (Area *const area); // won't have value unless area is valid
Rectangle       area_bounds             (const Area *const area);

/* Building Functions ******************************************************/
float       building_size            ();
Rectangle   building_bounds          (Building *const building);
Building  * get_building_by_position (Map * map, Vector2 position);

/* Region Functions ********************************************************/
Path * region_redirect_path(Region * region, Path * from);

/* Map Functions *********************************************************/
void     set_cursor_to_camera_scale (Camera2D *const cam);
void     map_clamp                  (Map * map);
Camera2D setup_camera               (Map * map);
void     render_map                 (Map * map);
void     render_map_mesh            (Map * map);

#endif // GEOMETRY_H_
