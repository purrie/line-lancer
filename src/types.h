#ifndef TYPES_H_
#define TYPES_H_

#include <raylib.h>
#include "optional.h"
#include "array.h"

typedef struct Node Node;
typedef struct Bridge Bridge;
typedef struct Line Line;
typedef struct Path Path;
typedef struct PathEntry PathEntry;
typedef struct Building Building;
typedef struct Castle Castle;
typedef struct Region Region;
typedef struct Map Map;
typedef struct Area Area;
typedef struct GameState GameState;
typedef struct Unit Unit;

typedef enum BuildingType BuildingType;
typedef enum PlayerState PlayerState;
typedef enum UnitType UnitType;
typedef enum UnitState UnitState;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long int usize;
typedef long int size;
typedef unsigned short ushort;

typedef void * ( * Allocator )(uint size);
typedef void ( * Deallocator )(void * ptr);

defOptional(Float);
defOptional(Uint);
defOptional(Usize);
defOptional(Vector2);
defOptional(Ushort);
defOptional(Map);

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef bool
#define bool int
#endif

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

makeOptional(float, Float);
makeOptional(uint, Uint);
makeOptional(usize, Usize);
makeOptional(ushort, Ushort);
makeOptional(Vector2, Vector2);

makeList(Vector2, Vector2);
makeList(ushort, Ushort);
makeList(usize, Usize);
makeList(Line, Line);
makeList(Path, Path);
makeList(PathEntry, PathEntry);
makeList(Building, Building);
makeList(Region, Region);
makeList(Bridge, Bridge);
makeList(Unit*, Unit);

struct Line {
    Vector2 a;
    Vector2 b;
};

struct Path {
    ListLine lines;
    Model model;
    Region * region_a;
    Region * region_b;
};

struct PathEntry {
    Path * path;
    Path * redirect;
};

enum BuildingType {
    BUILDING_EMPTY = 0,
    BUILDING_FIGHTER,
    BUILDING_ARCHER,
    BUILDING_SUPPORT,
    BUILDING_SPECIAL,
    BUILDING_RESOURCE,
    BUILDING_TYPE_COUNT = BUILDING_RESOURCE,
};

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

struct Map {
    usize      width;
    usize      height;
    uchar      player_count;
    ListRegion regions;
    ListPath   paths;
};

makeOptional(Map, Map);

enum UnitType {
    UNIT_FIGHTER,
    UNIT_ARCHER,
    UNIT_SUPPORT,
    UNIT_SPECIAL,
};

enum UnitState {
    UNIT_STATE_APPROACHING_ROAD = 0,
    UNIT_STATE_FOLLOWING_ROAD,
    UNIT_STATE_ATTACKING,
    UNIT_STATE_HURT,
    UNIT_STATE_DEFENDING_REGION,
    UNIT_STATE_ATTACKING_REGION,
};

struct Unit {
    float       health;
    Vector2     position;
    UnitType    type;
    ushort      upgrade;
    UnitState   state;
    float       progress;
    Path      * path;
    Region    * region;
    Unit      * target;
    usize       player_owned;
};

struct Node {
    Node    * previous;
    Node    * next;
    Unit    * unit;
    Vector2   position;
};

struct Bridge {
    Node * start;
    Node * end;
};

enum PlayerState {
    INPUT_NONE = 0,
    INPUT_OPEN_BUILDING,
    INPUT_START_SET_PATH,
    INPUT_SET_PATH,
};

struct GameState {
    PlayerState   current_input;
    Building    * selected_building;
    Map         * current_map;
    ListUnit      units;
};

typedef struct {
    uchar * start;
    usize   len;
} StringSlice;

StringSlice make_slice(uchar * from, usize start, usize end);

#endif // TYPES_H_
