#ifndef TYPES_H_
#define TYPES_H_

#include <raylib.h>
#include "array.h"

#define STRINGIFY(x) #x
#define STRINGIFY_VALUE(x) STRINGIFY(x)

typedef struct Line Line;
typedef struct Path Path;
typedef struct Building Building;
typedef struct Region Region;
typedef struct Map Map;
typedef struct Area Area;
typedef struct GameState GameState;
typedef struct Unit Unit;
typedef struct PlayerData PlayerData;
typedef struct MagicEffect MagicEffect;
typedef struct Attack Attack;
typedef struct WayPoint WayPoint;
typedef struct FindPoint FindPoint;
typedef struct NavGraph NavGraph;
typedef struct GlobalNavGrid GlobalNavGrid;

typedef enum BuildingType BuildingType;
typedef enum PlayerState PlayerState;
typedef enum MagicType MagicType;
typedef enum UnitType UnitType;
typedef enum UnitState UnitState;
typedef enum Result Result;
typedef enum Test Test;
typedef enum GraphType GraphType;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long usize;
typedef long isize;
typedef unsigned short ushort;

enum Result {
    SUCCESS = 0,
    FAILURE,
    FATAL,
};

enum Test {
    NO = 0,
    YES,
};

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

makeList(Vector2, Vector2);
makeList(ushort, Ushort);
makeList(usize, Usize);
makeList(Line, Line);
makeList(Path, Path);
makeList(Building, Building);
makeList(Region, Region);
makeList(PlayerData, PlayerData);
makeList(Map, Map);
makeList(MagicEffect, MagicEffect);
makeList(Attack, Attack);
makeList(Path*, PathP);

makeList(FindPoint, FindPoint);
makeList(WayPoint*, WayPoint);
makeList(NavGraph, NavGraph);
makeList(Unit*, Unit);
makeList(Region*, RegionP);

typedef enum FactionType {
    FACTION_KNIGHTS,
    FACTION_MAGES,
} FactionType;

enum UnitType {
    UNIT_FIGHTER,
    UNIT_ARCHER,
    UNIT_SUPPORT,
    UNIT_SPECIAL,
    UNIT_GUARDIAN,
};

enum UnitState {
    UNIT_STATE_IDLE = 0,
    UNIT_STATE_MOVING,
    UNIT_STATE_CHASING,
    UNIT_STATE_FIGHTING,
    UNIT_STATE_SUPPORTING,
    UNIT_STATE_GUARDING,
};

enum MagicType {
    MAGIC_HEALING = 0,
    MAGIC_CURSE,
    MAGIC_WEAKNESS,
    MAGIC_STRENGTH,
    MAGIC_HELLFIRE,
    MAGIC_TYPE_LAST = MAGIC_HELLFIRE,
};

enum GraphType {
    GRAPH_REGION,
    GRAPH_PATH,
};

struct WayPoint {
    Vector2    world_position;
    usize      nav_world_pos_x;
    usize      nav_world_pos_y;
    NavGraph * graph;
    Unit     * unit;
    bool       blocked;
};

struct FindPoint {
    FindPoint * from;
    float cost;
    bool  visited;
    bool  queued;
};

struct NavGraph {
    GraphType type;
    usize width;
    usize height;
    usize offset_x;
    usize offset_y;
    ListWayPoint waypoints;
    union {
        Region * region;
        Path * path;
    };
    GlobalNavGrid * global;
};

struct GlobalNavGrid {
    usize width;
    usize height;
    ListWayPoint waypoints;
    ListFindPoint find_buffer;
} ;

struct MagicEffect {
    MagicType type;
    float     strength;
    usize     source_player;
};

struct Attack {
    usize   damage;
    float   timer;
    float   delay;
    usize   origin_attacker;
    Vector2 origin_position;
};

struct Unit {
    UnitType  type;
    ushort    upgrade;

    usize       player_owned;
    FactionType faction;

    usize     cooldown;
    float     health;
    UnitState state;
    Vector2   position;

    WayPoint * waypoint;
    ListWayPoint pathfind;
    usize current_path;

    ListMagicEffect effects;
    ListAttack incoming_attacks;

    Building * origin;
};

struct Line {
    Vector2 a;
    Vector2 b;
};

enum BuildingType {
    BUILDING_EMPTY = 0,
    BUILDING_FIGHTER,
    BUILDING_ARCHER,
    BUILDING_SUPPORT,
    BUILDING_SPECIAL,
    BUILDING_RESOURCE,
    BUILDING_TYPE_LAST = BUILDING_RESOURCE,
};

struct Building {
    Vector2        position;
    BuildingType   type;
    Model          model;
    ushort         upgrades;
    float          spawn_timer;
    usize          units_spawned;
    Region       * region;
    ListWayPoint   spawn_points;
};

struct Area {
    ListLine lines;
    Model    model;
};

struct Path {
    ListLine lines;
    Model    model;
    Region * region_a;
    Region * region_b;
    NavGraph nav_graph;
    Map    * map;
};

struct Region {
    FactionType   faction;
    usize         player_id;
    Area          area;
    Unit          castle;
    ListBuilding  buildings;
    ListPathP     paths;
    usize         active_path;
    NavGraph      nav_graph;
    Map         * map;
};

struct Map {
    char        * name;
    usize         width;
    usize         height;
    uchar         player_count;
    ListRegion    regions;
    ListPath      paths;
    GlobalNavGrid nav_grid;
};

typedef enum PlayerType {
    PLAYER_NEUTRAL,
    PLAYER_LOCAL,
    PLAYER_AI,
} PlayerType;

struct PlayerData {
    usize resource_gold;
    PlayerType type;
    FactionType faction;
    usize seed;
};

typedef enum {
    EXE_MODE_MAIN_MENU,
    EXE_MODE_SINGLE_PLAYER_MAP_SELECT,
    EXE_MODE_IN_GAME,
    EXE_MODE_EXIT,
} ExecutionMode;

typedef struct {
    ListMap maps;
} GameAssets;

enum PlayerState {
    INPUT_NONE = 0,
    INPUT_CLICKED_BUILDING,
    INPUT_CLICKED_PATH,
    INPUT_OPEN_BUILDING,
    INPUT_MOVE_MAP,
};

struct GameState {
    PlayerState      current_input;
    Vector2          selected_point;
    Building       * selected_building;
    Path           * selected_path;
    Region         * selected_region;
    Map              map;
    ListUnit         units;
    ListPlayerData   players;
    usize            turn;
    Camera2D         camera;
};

/* Utils *********************************************************************/
char * faction_to_string (FactionType faction);
char * building_type_to_string (BuildingType type);
#endif // TYPES_H_
